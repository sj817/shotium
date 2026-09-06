// Compare two installed shotium packages, including their real native
// libraries, case by case. Run on an otherwise idle host, after building the
// candidate and staging its DLL/.so:
//
//   pnpm perf:compare BASELINE_PACKAGE CANDIDATE_PACKAGE OUTPUT.json
//   Optional: --samples=20 --min-seconds=3 --max-seconds=8 --max-samples=1000 --filter=corpus
//             --calibrate --check
//
// Every case takes at least --samples pairs and goes on until each side has
// spent --min-seconds capturing, up to --max-samples pairs: a 3 ms case gets
// a thousand pairs and intervals narrow enough to see a few percent, a 300 ms
// case gets its hundred. --calibrate first times the candidate against itself
// to measure this machine's noise; the equivalence bands every case is judged
// against come from that (see lib/perf-gate.ts). --check exits non-zero
// unless every case is accepted: faster for engine cases, not slower for the
// two pinned to an external wait. A filtered run is diagnostic and can never
// pass the complete matrix gate.
//
// Each side runs in a forked worker (this file, `worker` mode) so the two
// packages never share a process: Blink is a process-wide singleton.
// Relative paths are resolved against the repository root.

import {execFileSync, fork, type ChildProcess} from 'node:child_process';
import {createHash} from 'node:crypto';
import {copyFileSync, existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, statSync, writeFileSync, createReadStream} from 'node:fs';
import http from 'node:http';
import {createRequire} from 'node:module';
import type {AddressInfo} from 'node:net';
import os from 'node:os';
import path from 'node:path';

import {cac} from 'cac';

import {calibrate, compare, type Bands, type Comparison, type Status} from './lib/perf-gate.ts';
import {libraryName, root} from './lib/repo.ts';

import type * as Shotium from '../shotium/src/index.ts';
import type {CaptureStats, ScreenshotOptions} from '../shotium/src/types.ts';

// Cases whose wall time is pinned to an external wait that no code in this
// process can shorten: a server that sleeps 250 ms before answering, and a
// 500 ms quiet window that has to elapse. These are asked not to be slower;
// every other case is asked to be faster.
const EXTERNAL_WAIT = new Set(['http-slow', 'http-networkidle']);

// The cases an A/A calibration runs: quick, static, and spread across the
// paths the suite exercises -- raster, encode, file output, selector, queue.
// Their point is to measure this machine's noise, not to be fast, so they are
// the ones that finish in milliseconds and expose the noise most.
const CALIBRATION_CASES = ['card-png', 'card-jpeg', 'corpus-png', 'output-file-png', 'selector-jpeg', 'queue-1'];

// The verdicts a case can end with, worst first, so that a case with several
// timed metrics (a cold start has four) reports its least favourable one.
const VERDICT_RANK: Status[] = ['slower', 'unproven', 'insufficient-samples', 'equivalent', 'faster'];
const worstVerdict = (statuses: Status[]): Status =>
    statuses.reduce((worst, s) => VERDICT_RANK.indexOf(s) < VERDICT_RANK.indexOf(worst) ? s : worst, 'faster' as Status);
const hash = (bytes: Buffer | string) => createHash('sha256').update(bytes).digest('hex');

type Request = ScreenshotOptions & {path?: string};

interface Case {
  name: string;
  group: string;
  class: 'external' | 'engine';
  request: Request;
  large?: boolean;
  outputFile?: boolean;
  operation?: string;
  batch?: number;
  concurrency?: number;
  cold?: boolean;
  count?: number;
  daemon?: boolean;
  rejectRequest?: Request;
  expectedError?: string;
  expectedFailed?: number;
  cacheEnabled?: boolean;
  prime?: Request;
  evidenceDirectory?: string;
}

const accepted = (item: Case, status: Status) => item.class === 'external' ? ['faster', 'equivalent'].includes(status) : status === 'faster';

// ---------------------------------------------------------------------------
// The worker: one package, one engine, driven over IPC.

interface Command {
  metadata?: boolean;
  stop?: boolean;
  operation?: string;
  rejectRequest?: Request;
  expectedError?: string;
  batch?: number;
  request: Request;
  expectedFailed?: number;
  evidencePath?: string;
}

interface Row {
  wall: number;
  stats: CaptureStats;
  bytes: number;
  rss: number;
  sha256: string;
  evidence?: string;
}

async function worker(packagePath: string): Promise<void> {
  const require = createRequire(import.meta.url);
  const importedAt = process.hrtime.bigint();
  const shotium = require(path.resolve(packagePath)) as typeof Shotium;
  const {Runtime} = shotium;
  const runtime = new Runtime();
  const started = process.hrtime.bigint();
  const cacheDir = process.env.SHOT_PERF_CACHE || null;
  const info = runtime.start({cacheDir});
  const startMs = Number(process.hrtime.bigint() - started) / 1e6;
  const importAndStartMs = Number(process.hrtime.bigint() - importedAt) / 1e6;
  const addon = Object.keys(require.cache).find((p) => path.basename(p) === 'shotium.node');
  if (!addon) throw new Error('Could not identify the loaded native addon');
  const directory = info.enginePath ?? path.dirname(addon);
  const library = path.join(directory, libraryName);
  let daemonClient: Awaited<ReturnType<typeof shotium.daemon.connect>> | undefined;
  const daemonOptions = {name: `perf-${process.pid}`, cacheDir: null, resourceDir: directory, idleTimeoutMs: 60000};
  if (process.env.SHOT_PERF_DAEMON) daemonClient = await shotium.daemon.connect(daemonOptions);
  const send = (value: unknown) => process.send!(value);
  // Send readiness before reading/hash-checking the native artifacts. Hashing
  // 50 MB is verification work, not package startup work.
  const metadata = () => ({
    startMs, importAndStartMs, library,
    librarySha256: hash(readFileSync(library)),
    addonSha256: hash(readFileSync(addon)),
    bundleSha256: hash(readFileSync(path.join(packagePath, 'dist/index.js'))),
    packageVersion: (JSON.parse(readFileSync(path.join(packagePath, 'package.json'), 'utf8')) as {version: string}).version,
  });
  process.on('message', async (command: Command) => {
    if (command.metadata) {
      send(metadata());
      return;
    }
    if (command.stop) {
      if (daemonClient) {
        daemonClient.close();
        await shotium.daemon.stop(daemonOptions);
      }
      await runtime.stop();
      process.exit(0);
    }
    try {
      const start = process.hrtime.bigint();
      if (command.operation === 'purge') runtime.releaseMemory();
      if (command.operation === 'purge-working-set') runtime.releaseMemory({releaseWorkingSet: true});
      if (command.operation === 'restart') {
        await runtime.stop();
        runtime.start({cacheDir});
      }
      if (command.rejectRequest) {
        let rejected = false;
        try {
          await runtime.screenshot(command.rejectRequest);
        } catch (error) {
          if (!new RegExp(command.expectedError!).test((error as Error).message)) throw error;
          rejected = true;
        }
        if (!rejected) throw new Error('Expected request to reject');
      }
      const screenshot = (request: Request) => command.operation === 'daemon-reconnect' ?
          shotium.daemon.screenshot({...request, daemon: {...daemonOptions, spawn: false}}) :
          (daemonClient ?? runtime).screenshot(request);
      const results = await Promise.all(Array.from({length: command.batch || 1}, () => screenshot(command.request)));
      const wall = Number(process.hrtime.bigint() - start) / 1e6;
      const result = results[0];
      const bytes = result.image ?? readFileSync(command.request.path!);
      if (!bytes.length || results.some((r) => r.stats.failed !== (command.expectedFailed || 0))) {
        throw new Error(`Empty image or unexpected resource failures: ${JSON.stringify(results.map((r) => r.stats))}`);
      }
      let evidence: string | undefined;
      if (command.evidencePath) {
        writeFileSync(command.evidencePath, bytes);
        evidence = command.evidencePath;
      }
      send({wall, stats: result.stats, bytes: bytes.length, rss: process.memoryUsage().rss, sha256: hash(bytes), evidence} satisfies Row);
    } catch (error) {
      send({error: (error as Error).message});
    }
  });
  send({startMs, importAndStartMs});
}

// ---------------------------------------------------------------------------

interface Child extends ChildProcess {
  label?: string;
  directory?: string;
  diagnostics?: string;
  ready?: {startMs: number; importAndStartMs: number; processStartMs?: number};
}

function receive<T = Record<string, unknown>>(child: Child): Promise<T> {
  return new Promise((resolve, reject) => {
    const cleanup = () => {
      clearTimeout(timer);
      child.off('message', message);
      child.off('exit', exited);
      child.off('error', failed);
    };
    const message = (value: unknown) => {
      cleanup();
      resolve(value as T);
    };
    const exited = (code: number | null) => {
      cleanup();
      reject(new Error(`Worker exited: ${code}\n${child.diagnostics || ''}`));
    };
    const failed = (error: Error) => {
      cleanup();
      reject(error);
    };
    const timer = setTimeout(() => {
      cleanup();
      child.kill();
      reject(new Error('Capture timed out after 120s'));
    }, 120000);
    child.once('message', message);
    child.once('exit', exited);
    child.once('error', failed);
  });
}

function cases(card: string, baseUrl: string): Case[] {
  const data = path.join(root, 'shot/testdata');
  const items: Array<Omit<Case, 'group' | 'class'> & {group?: string}> = [];
  for (const type of ['png', 'jpeg', 'webp'] as const) {
    for (const [name, file, viewport, extra] of [
      ['card', card, {width: 800, height: 450}, {}],
      ['corpus', path.join(data, 'render_corpus.html'), {width: 1248, height: 1320}, {}],
      ['corpus-scale2', path.join(data, 'render_corpus.html'), {width: 1248, height: 1320}, {scale: 2}],
      ['corpus-clip', path.join(data, 'render_corpus.html'), {width: 1248, height: 1320}, {clip: {x: 60, y: 120, width: 800, height: 450}}],
    ] as Array<[string, string, {width: number; height: number}, Partial<Request>]>) {
      items.push({name: `${name}-${type}`, request: {file, viewport, type, allowFileAccess: true, ...extra}});
    }
  }
  items.push({name: 'features-alpha', request: {file: path.join(data, 'features.html'), viewport: {width: 400, height: 300}, type: 'png', omitBackground: true, allowFileAccess: true}});
  for (const id of ['1788403871415', '1788434008828']) {
    const request: Request = {file: path.join(data, 'bilibili', `${id}.html`), viewport: {width: 1440, height: 900}, type: 'png', allowFileAccess: true};
    items.push({name: `bili-${id.slice(-4)}-viewport`, request, large: true});
    for (const type of ['png', 'jpeg', 'webp'] as const) {
      items.push({name: `bili-${id.slice(-4)}-clip-${type}`, large: true, request: {...request, type, clip: {x: 0, y: 0, width: 1440, height: 12000}}});
    }
  }
  for (const name of ['simple', 'css-heavy', 'flex', 'grid', 'text', 'fonts', 'images', 'gradient', 'filter', 'long-page']) {
    for (const type of ['png', 'jpeg', 'webp'] as const) {
      items.push({name: `standard-${name}-${type}`, request: {
        file: path.join(root, 'apps/benchmark/fixtures', `${name}.html`), type,
        viewport: {width: 1280, height: 720}, fullPage: name === 'long-page', allowFileAccess: true,
      }});
    }
  }
  const common: Request = {file: card, viewport: {width: 800, height: 450}, allowFileAccess: true};
  for (const type of ['png', 'jpeg', 'webp'] as const) {
    items.push({name: `output-file-${type}`, outputFile: true, request: {...common, type}});
    items.push({name: `selector-${type}`, request: {...common, type, selector: 'article'}});
    for (const scale of [0.5, 1.5, 4, 8]) items.push({name: `scale-${scale}-${type}`, request: {...common, type, scale}});
  }
  for (const operation of ['purge', 'purge-working-set', 'restart']) {
    items.push({name: `lifecycle-${operation}`, group: 'lifecycle', operation, request: {...common, type: 'jpeg'}});
  }
  for (const batch of [1, 2, 4, 16]) items.push({name: `queue-${batch}`, group: 'parallel', batch, request: {...common, type: 'jpeg'}});
  for (const concurrency of [1, 2, 4]) {
    items.push({name: `processes-${concurrency}`, group: 'parallel', concurrency, batch: 8, request: {...common, type: 'jpeg'}});
  }
  for (const type of ['png', 'jpeg', 'webp'] as const) items.push({name: `startup-${type}`, group: 'startup', cold: true, request: {...common, type}});
  items.push({name: 'soak-1000', group: 'soak', count: 1000, request: {...common, type: 'jpeg'}});
  for (const type of ['png', 'jpeg', 'webp'] as const) {
    for (const outputFile of [false, true]) {
      items.push({name: `daemon-${type}-${outputFile ? 'file' : 'buffer'}`, group: 'daemon', daemon: true, outputFile, request: {...common, type}});
    }
  }
  items.push({name: 'daemon-queue-4', group: 'daemon', daemon: true, batch: 4, request: {...common, type: 'jpeg'}});
  items.push({name: 'daemon-reconnect', group: 'daemon', daemon: true, operation: 'daemon-reconnect', request: {...common, type: 'jpeg'}});
  for (const [name, rejectRequest, expectedError] of [
    ['missing-file', {...common, file: `${card}.missing`}, 'could not read'],
    ['missing-selector', {...common, selector: '#not-present'}, 'no element matches'],
    ['invalid-options', {...common, scale: 0}, 'scale'],
    ['timeout', {...common, file: `${baseUrl}/slow?ms=250`, pageGotoParams: {timeout: 1}}, 'timed out|timeout'],
  ] as Array<[string, Request, string]>) {
    items.push({name: `recovery-${name}`, group: 'resilience', rejectRequest, expectedError, request: {...common, type: 'png'}});
  }
  for (const route of ['remote-page.html', 'missing-resource.html', 'redirect', 'slow?ms=250']) {
    // A 404 is an HTTP response, not a failed transport in CaptureStats.
    items.push({name: `http-${route.split(/[.?]/)[0]}`, group: 'network', expectedFailed: 0, request: {...common, file: `${baseUrl}/${route}`}});
  }
  items.push({name: 'http-networkidle', group: 'network', request: {...common, file: `${baseUrl}/remote-page.html`, pageGotoParams: {waitUntil: 'networkidle'}}});
  for (const cache of ['default', 'reload', 'no-store', 'only-if-cached'] as const) {
    items.push({name: `cache-${cache}`, group: 'network', cacheEnabled: true, prime: {...common, file: `${baseUrl}/remote-page.html`}, request: {...common, file: `${baseUrl}/remote-page.html`, cache}});
  }
  // Many independent cache entries exercise the per-host admission limit;
  // the three-resource network page cannot detect serialization behind it.
  for (const cache of ['default', 'reload'] as const) {
    for (const type of ['png', 'webp'] as const) {
      items.push({name: `many-resources-${cache}-${type}`, group: 'network', cacheEnabled: true, prime: {...common, file: `${baseUrl}/many-resources.html`}, request: {...common, file: `${baseUrl}/many-resources.html`, cache, type}});
    }
  }
  return items.map((item) => ({group: 'render', class: EXTERNAL_WAIT.has(item.name) ? 'external' : 'engine', ...item} as Case));
}

async function fixtureServer(): Promise<{server: http.Server; baseUrl: string}> {
  const fixtures = path.join(root, 'apps/benchmark/fixtures');
  const server = http.createServer((request, response) => {
    const url = new URL(request.url ?? '/', 'http://localhost');
    if (url.pathname === '/redirect') {
      response.writeHead(302, {location: '/simple.html'}).end();
      return;
    }
    if (url.pathname === '/slow') {
      setTimeout(() => response.writeHead(200, {'content-type': 'text/html'}).end('<body style="background:#def"><h1>Delayed response</h1>'), 250);
      return;
    }
    if (url.pathname === '/many-resources.html') {
      response.writeHead(200, {'content-type': 'text/html', 'cache-control': 'max-age=3600'})
          .end('<body style="margin:0;background:#eef;font:18px Arial"><h1>Independent cached images</h1>' +
               '<div style="display:grid;grid-template-columns:repeat(8,90px);gap:5px">' +
               Array.from({length: 48}, (_, i) => `<img width="90" height="50" src="/resource.svg?index=${i}">`).join('') + '</div>');
      return;
    }
    if (url.pathname === '/resource.svg') {
      const i = Number(url.searchParams.get('index'));
      response.writeHead(200, {'content-type': 'image/svg+xml', 'cache-control': 'max-age=3600'})
          .end(`<svg xmlns="http://www.w3.org/2000/svg" width="90" height="50"><rect width="90" height="50" fill="hsl(${i * 7},70%,60%)"/><circle cx="45" cy="25" r="${8 + i % 12}" fill="#fff"/></svg>`);
      return;
    }
    const file = path.resolve(fixtures, `.${decodeURIComponent(url.pathname)}`);
    if (!file.startsWith(`${fixtures}${path.sep}`) || !existsSync(file) || !statSync(file).isFile()) {
      response.writeHead(404).end('not found');
      return;
    }
    const mime: Record<string, string> = {'.html': 'text/html', '.css': 'text/css', '.svg': 'image/svg+xml', '.woff2': 'font/woff2'};
    response.writeHead(200, {'content-type': mime[path.extname(file)] || 'application/octet-stream', 'cache-control': 'max-age=3600'});
    createReadStream(file).pipe(response);
  });
  await new Promise<void>((done) => server.listen(0, '127.0.0.1', done));
  return {server, baseUrl: `http://127.0.0.1:${(server.address() as AddressInfo).port}`};
}

interface Options {
  samples: number;
  minSeconds: number;
  maxSamples: number;
  precision: number;
  maxSeconds: number;
  filter?: string;
  shard: string;
  calibrate: boolean;
  check: boolean;
}

interface Record_ extends Case {
  count: number;
  samples: {baseline: Array<Row | Record<string, number>>; candidate: Array<Row | Record<string, number>>};
  status: Status | 'running' | 'error';
  metrics?: Record<string, Comparison>;
  summary?: Comparison['summary'];
  accepted?: boolean;
  capturesPerSample?: number;
  error?: string;
}

async function main(baseline: string, candidate: string, output: string, o: Options): Promise<void> {
  const samples = o.samples;
  if (!Number.isInteger(samples) || samples < 10) throw new Error('--samples must be an integer >= 10');
  // How long each side of a case keeps capturing past the minimum pairs. The
  // noise of a median shrinks with the square root of the pairs, so a quick
  // case -- where a few percent is a few hundredths of a millisecond -- is
  // the one that needs more of them, and it is also the one that can afford
  // them: a thousand 3 ms captures is three seconds.
  const minSeconds = o.minSeconds;
  if (!(minSeconds >= 0)) throw new Error('--min-seconds must be a non-negative number');
  const maxSamples = o.maxSamples;
  if (!Number.isInteger(maxSamples) || maxSamples < samples) throw new Error('--max-samples must be an integer >= --samples');
  // Past the time budget, a case keeps sampling until its ratio is known to
  // within --precision (the half-width of the 99% intervals of p50 and the
  // mean) or --max-seconds per side is spent. The rule looks only at how wide
  // the interval is, never at which side of 1 it lies, so stopping does not
  // favour either answer; it just spends the samples where the noise is.
  const precision = o.precision;
  if (!(precision > 0)) throw new Error('--precision must be a positive fraction');
  const maxSeconds = o.maxSeconds;
  if (!(maxSeconds >= minSeconds)) throw new Error('--max-seconds must be >= --min-seconds');
  const filter = o.filter ? new RegExp(o.filter) : /./;
  const shard = o.shard;
  if (!['all', 'render', 'network', 'startup', 'lifecycle', 'parallel', 'soak', 'daemon', 'resilience'].includes(shard)) {
    throw new Error(`Unknown shard: ${shard}`);
  }
  // --calibrate first times the candidate against itself on a few quick cases
  // and reads the machine's noise band off the result; without it the band is
  // the gate's floor. Either way the band is recorded in the result.
  const calibrating = o.calibrate;

  const temporary = mkdtempSync(path.join(os.tmpdir(), 'shot-perf-'));
  const card = path.join(temporary, 'card.html');
  writeFileSync(card, '<body style="margin:0;background:#e8eef7;font:24px Arial"><article style="margin:35px;padding:32px;background:white;border-radius:20px;box-shadow:0 8px 22px #abc"><h1>Screenshot performance</h1><p>Offline rendering, identical input.</p><div style="height:70px;background:linear-gradient(90deg,#4263eb,#38d9a9)"></div></article>');
  const children = new Set<Child>();
  const {server, baseUrl} = await fixtureServer();
  const matrix = cases(card, baseUrl);
  const selected = matrix.filter((c) => (shard === 'all' || c.group === shard) && filter.test(c.name));
  if (!selected.length) {
    server.close();
    throw new Error('No benchmark cases matched');
  }
  const gateFile = path.join(import.meta.dirname, 'lib', 'perf-gate.ts');
  const result = {
    startedUtc: new Date().toISOString(),
    platform: process.platform,
    arch: process.arch,
    node: process.version,
    host: {release: os.release(), cpu: os.cpus()[0]?.model, logicalCpus: os.cpus().length, totalMemory: os.totalmem(), freeMemory: os.freemem()},
    revision: execFileSync('git', ['rev-parse', 'HEAD'], {cwd: root, encoding: 'utf8'}).trim(),
    sourceDiffSha256: hash(execFileSync('git', ['diff', '--', 'shot', 'cc', 'third_party/blink', 'shotium/native', 'patches', 'build'], {cwd: root})),
    harnessSha256: hash(Buffer.concat([readFileSync(import.meta.filename), readFileSync(gateFile)])),
    fixtureManifestSha256: hash(readFileSync(path.join(root, 'shot/testdata/bilibili/manifest.json'))),
    requiredCases: matrix.map((c) => c.name),
    selectedCases: selected.map((c) => c.name),
    shard,
    complete: false,
    status: 'running' as 'running' | 'pass' | 'not-passed',
    calibration: {tolerance: calibrate([]) as Bands, source: 'floor', cases: [] as Record_[]},
    metadata: {} as Record<string, {librarySha256: string}>,
    cases: [] as Record_[],
    sampling: {minimumPairs: samples, minSeconds, maxPairs: maxSamples, precision, maxSeconds},
    method: `Serial AB/BA pairs; five warmups except fresh processes; all samples retained. At least ${samples} pairs per case, continuing until each side has captured for ${minSeconds} s, then until the p50 and mean ratio intervals are within ±${precision * 100}% or ${maxSeconds} s per side, up to ${maxSamples} pairs. A cold start's processStartMs is recorded, not judged. Per-metric paired bootstrap 99% intervals of candidate/baseline for p50, mean and p95. p50 and mean are judged against a body band [1, 1 + primary] and p95 against a tail band [1, 1 + tail], each the noise an A/A calibration of the candidate against itself showed on that statistic (floored at 2%). faster: the mean's whole interval under 1, or p50's under 1 with the mean inside the body band, and no metric slower. equivalent: p50 and mean intervals inside the body band, neither under 1. slower: some interval past its band. unproven: otherwise (p50 or mean straddles the body band). p95 only makes a case slower, when its whole interval is past the tail band. Engine cases are accepted only when faster; cases pinned to an external wait when faster or equivalent. Fresh process startup includes import; OS file cache is not flushed. Multi-process wall includes IPC and verification of the batch; per-process capture wall excludes hashing. No claim about unlisted inputs or machines.`,
    finishedUtc: undefined as string | undefined,
  };
  const save = () => {
    mkdirSync(path.dirname(path.resolve(output)), {recursive: true});
    writeFileSync(output, JSON.stringify(result, null, 2));
  };
  const sources = path.resolve(output) + '.sources';
  mkdirSync(sources, {recursive: true});
  copyFileSync(import.meta.filename, path.join(sources, path.basename(import.meta.filename)));
  copyFileSync(gateFile, path.join(sources, 'perf-gate.ts'));

  const send = async <T = Row>(child: Child, command: Partial<Command>): Promise<T> => {
    const response = receive<T & {error?: string}>(child);
    child.send(command);
    const value = await response.catch((error: Error) => {
      throw new Error(`${child.label || 'worker'}: ${error.message}`);
    });
    if (value.error) throw new Error(value.error);
    return value;
  };
  let workerNumber = 0;
  // Which package each label runs. Calibration points both at the candidate,
  // so that the only difference between the two sides is the noise.
  let packages: Record<string, string> = {baseline, candidate};
  let recordMetadata = true;
  const spawn = async (label: string, item: Case): Promise<Child> => {
    const directory = path.join(temporary, `${label}-${workerNumber++}`);
    mkdirSync(directory);
    const began = process.hrtime.bigint();
    const child: Child = fork(import.meta.filename, ['worker', path.resolve(packages[label])], {
      silent: true,
      env: {...process.env, SHOT_PERF_CACHE: item.cacheEnabled ? path.join(directory, 'cache') : '', SHOT_PERF_DAEMON: item.daemon ? '1' : ''},
    });
    children.add(child);
    child.label = label;
    child.directory = directory;
    child.stdout!.resume();
    child.diagnostics = '';
    child.stderr!.on('data', (chunk: Buffer) => {
      child.diagnostics = ((child.diagnostics ?? '') + chunk).slice(-8192);
    });
    child.ready = await receive<{startMs: number; importAndStartMs: number}>(child);
    child.ready.processStartMs = Number(process.hrtime.bigint() - began) / 1e6;
    return child;
  };
  const close = async (child: Child) => {
    // stop() returns engine memory but does not tear Blink down. The owned
    // worker exits afterwards, before another cold process is started.
    const exited = new Promise<void>((done) => child.once('exit', () => done()));
    if (child.exitCode === null && !child.killed) child.send({stop: true});
    else return;
    const timer = setTimeout(() => child.kill(), 5000);
    await exited;
    clearTimeout(timer);
    children.delete(child);
  };
  const capture = async (pool: Child[], item: Case) => {
    const start = process.hrtime.bigint();
    const rows = await Promise.all(pool.map((child) => send(child, {
      ...item,
      evidencePath: item.evidenceDirectory ? path.join(item.evidenceDirectory, `${path.basename(child.directory!)}.${item.request.type || 'png'}`) : undefined,
      request: {...item.request, ...(item.outputFile ? {path: path.join(child.directory!, `capture.${item.request.type}`)} : {})},
    })));
    const wall = pool.length > 1 ? Number(process.hrtime.bigint() - start) / 1e6 : rows[0].wall;
    return {wall, workers: rows, rss: rows.reduce((sum, row) => sum + row.rss, 0)};
  };
  // Checks that the library behind `label` is the one this run started with,
  // and records it. Skipped while calibrating: both sides are the candidate
  // then, and writing that under `baseline` would misdescribe the run.
  const noteMetadata = (label: string, metadata: {librarySha256: string}) => {
    if (!recordMetadata) return;
    if (result.metadata[label] && result.metadata[label].librarySha256 !== metadata.librarySha256) {
      throw new Error('Native library changed during measurement');
    }
    result.metadata[label] = metadata;
  };
  let tolerance: Bands = result.calibration.tolerance;
  // Measures one case into `sink`. The same procedure serves calibration and
  // the real comparison; only the packages behind the labels differ.
  const measure = async (item: Case, sink: Record_[]) => {
    const count = Math.max(samples, item.count || 0);
    const rows: Record<'baseline' | 'candidate', Array<Record<string, number> & Row>> = {baseline: [], candidate: []};
    const pools: Record<string, Child[]> = {baseline: [], candidate: []};
    const record: Record_ = {...item, count, samples: rows, status: 'running'};
    sink.push(record);
    try {
      if (!item.cold) {
        for (const label of ['baseline', 'candidate'] as const) {
          for (let i = 0; i < (item.concurrency || 1); i++) {
            const child = await spawn(label, item);
            pools[label].push(child);
            noteMetadata(label, await send<{librarySha256: string}>(child, {metadata: true}));
            if (item.prime) await send(child, {request: item.prime});
          }
        }
      }
      // The minimum pairs, then more until the slower side has spent
      // minSeconds capturing, up to the cap. Both sides advance together, so
      // the pairs stay paired whatever the count ends up being.
      const budgetMs = minSeconds * 1000;
      const ceilingMs = maxSeconds * 1000;
      const limit = Math.max(count, maxSamples);
      const spent: Record<'baseline' | 'candidate', number> = {baseline: 0, candidate: 0};
      // Whether the wall ratio is already known to within `precision`,
      // re-asked every 25 pairs with a light bootstrap; a case that is settled
      // stops at the time budget, a noisy one goes on to the ceiling.
      let settled = false;
      const settledNow = () => {
        const quick = compare(rows.baseline.map((v) => v.wall), rows.candidate.map((v) => v.wall), {tolerance, resamples: 300, minimumSamples: samples});
        const halfWidth = (metric: 'p50' | 'mean') => (quick.intervals[metric].hi - quick.intervals[metric].lo) / 2;
        return halfWidth('p50') <= precision && halfWidth('mean') <= precision;
      };
      for (let i = item.cold ? 0 : -5;
           i < count || (i < limit && (Math.min(spent.baseline, spent.candidate) < budgetMs ||
                                       (Math.min(spent.baseline, spent.candidate) < ceilingMs && !settled)));
           i++) {
        if (i >= count && (i - count) % 25 === 0) settled = settledNow();
        if (i === 0) {
          item.evidenceDirectory = path.join(path.resolve(output) + '.images', item.name);
          mkdirSync(item.evidenceDirectory, {recursive: true});
        } else {
          delete item.evidenceDirectory;
        }
        const order: Array<'baseline' | 'candidate'> = i % 2 ? ['candidate', 'baseline'] : ['baseline', 'candidate'];
        for (const label of order) {
          let value: Record<string, number> & Row;
          if (item.cold) {
            const child = await spawn(label, item);
            try {
              value = {...await capture([child], item), ...child.ready} as unknown as Record<string, number> & Row;
              noteMetadata(label, await send<{librarySha256: string}>(child, {metadata: true}));
            } finally {
              await close(child);
            }
          } else {
            value = await capture(pools[label], item) as unknown as Record<string, number> & Row;
          }
          if (i >= 0) {
            rows[label].push(value);
            spent[label] += value.wall;
          }
        }
        if (i >= 0 && i % 25 === 24) save();
      }
      record.count = rows.baseline.length;
      const metrics = item.cold ? ['wall', 'startMs', 'importAndStartMs', 'processStartMs'] : ['wall'];
      record.metrics = Object.fromEntries(metrics.map((key) => [
        key, compare(rows.baseline.map((v) => v[key]), rows.candidate.map((v) => v[key]), {tolerance, minimumSamples: samples}),
      ]));
      record.summary = record.metrics.wall.summary;
      // A cold start is judged on the capture, the engine's start and the
      // import together with it. processStartMs is recorded beside them and
      // not judged: it is node's own process starting on this OS, the same
      // binary and the same 110 ms on both sides.
      const judged = metrics.filter((key) => key !== 'processStartMs');
      record.status = worstVerdict(judged.map((key) => record.metrics![key].status));
      record.accepted = accepted(item, record.status);
      record.capturesPerSample = (item.concurrency || 1) * (item.batch || 1);
    } catch (error) {
      record.status = 'error';
      record.accepted = false;
      record.error = (error as Error).stack;
    } finally {
      for (const pool of Object.values(pools)) for (const child of pool) await close(child);
    }
    save();
    const summary = record.summary;
    console.log(`${item.name}: ${summary ? `${summary.baseline.p50.toFixed(3)} -> ${summary.candidate.p50.toFixed(3)} ms` : record.error} ${record.status}${record.accepted ? '' : ' (not accepted)'}`);
    return record;
  };
  try {
    if (calibrating) {
      // The candidate against itself: every true ratio is 1, so how far the
      // worst statistic strays above 1 is this machine's noise. The band the
      // real comparison is judged against comes from that, never from a
      // number chosen before measuring.
      packages = {baseline: candidate, candidate};
      recordMetadata = false;
      const chosen = matrix.filter((c) => CALIBRATION_CASES.includes(c.name));
      for (const item of chosen) await measure(item, result.calibration.cases);
      packages = {baseline, candidate};
      recordMetadata = true;
      const usable = result.calibration.cases.filter((c) => c.metrics && c.metrics.wall);
      tolerance = calibrate(usable.map((c) => c.metrics!.wall));
      result.calibration.tolerance = tolerance;
      result.calibration.source = `A/A on ${usable.length} case(s)`;
      console.log(`calibration: equivalence bands body +${(tolerance.primary * 100).toFixed(2)}%, tail +${(tolerance.tail * 100).toFixed(2)}% from ${usable.length} A/A case(s)`);
      save();
    }
    for (const item of selected) await measure(item, result.cases);
    result.complete = !o.filter && result.cases.length === matrix.filter((c) => shard === 'all' || c.group === shard).length &&
        result.cases.every((c) => !['running', 'error'].includes(c.status));
    result.status = result.complete && result.cases.every((c) => c.accepted) ? 'pass' : 'not-passed';
    result.finishedUtc = new Date().toISOString();
    save();
    if (o.check && result.status !== 'pass') process.exitCode = 1;
  } finally {
    for (const child of children) child.kill();
    server.closeAllConnections();
    server.close();
    if (path.dirname(path.resolve(temporary)) !== path.resolve(os.tmpdir()) || !path.basename(temporary).startsWith('shot-perf-')) {
      throw new Error(`Refusing to remove unexpected temporary directory: ${temporary}`);
    }
    rmSync(temporary, {recursive: true, force: true});
  }
}

export {cases};

if (process.argv[2] === 'worker') {
  worker(process.argv[3]).catch((error) => {
    console.error(error);
    process.exitCode = 1;
  });
} else {
  const cli = cac('perf-compare');
  cli.command('<baseline> <candidate> <output>', 'time two installed packages against each other, case by case')
      .option('--samples <n>', 'minimum pairs per case', {default: 20})
      .option('--min-seconds <s>', 'each side keeps capturing until it has spent this long', {default: 3})
      .option('--max-seconds <s>', 'and at most this long', {default: 8})
      .option('--max-samples <n>', 'pairs per case, at most', {default: 1000})
      .option('--precision <fraction>', 'stop once the p50 and mean ratio intervals are this narrow', {default: 0.02})
      .option('--filter <regex>', 'only cases whose name matches (diagnostic; never passes the gate)')
      .option('--shard <name>', 'one case group: render, network, startup, lifecycle, parallel, soak, daemon, resilience', {default: 'all'})
      .option('--calibrate', 'time the candidate against itself first and read the noise band off that')
      .option('--check', 'exit non-zero unless every case is accepted')
      .action(async (baseline: string, candidate: string, output: string, options: Record<string, unknown>) => {
        try {
          await main(baseline, candidate, output, {
            samples: Number(options.samples),
            minSeconds: Number(options.minSeconds),
            maxSeconds: Number(options.maxSeconds),
            maxSamples: Number(options.maxSamples),
            precision: Number(options.precision),
            filter: options.filter === undefined ? undefined : String(options.filter),
            shard: String(options.shard),
            calibrate: options.calibrate === true,
            check: options.check === true,
          });
        } catch (error) {
          console.error(error);
          process.exitCode = 1;
        }
      });
  cli.help();
  cli.parse();
}
