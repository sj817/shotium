// Measures the host boundary: end-to-end screenshots alongside work sharing
// libuv's pool. Run only on an idle machine after correctness checks finish.
import {createHash, pbkdf2} from 'node:crypto';
import {readFile} from 'node:fs/promises';
import {readFileSync, writeFileSync} from 'node:fs';
import {createRequire} from 'node:module';
import {monitorEventLoopDelay, performance} from 'node:perf_hooks';
import path from 'node:path';
import {promisify} from 'node:util';
import {cac} from 'cac';
import {execa} from 'execa';
import {quantile} from 'simple-statistics';
import {resolve} from '../lib/repo.ts';
import type * as Shotium from '../../apps/typescript/src/index.ts';

interface Measurements {
  package: string; addon: string; addonSha256: string;
  importMs: number; startMs: number; firstCaptureMs: number;
  imageSha256: string;
  cases: Record<string, {capture: number[]; io: number[]; elapsedMs: number;
    eventLoopP95Ms: number; peakRss: number}>;
}
async function worker(packagePath: string): Promise<void> {
  const require = createRequire(import.meta.url);
  const begin = performance.now();
  const {Runtime} = require(packagePath) as typeof Shotium;
  const imported = performance.now();
  const runtime = new Runtime();
  runtime.start({cacheDir: null});
  const started = performance.now();
  const request = {file: resolve('shot/testdata/render_corpus.html'),
    viewport: {width: 1248, height: 1320}, allowFileAccess: true};
  const first = await runtime.screenshot(request);
  const firstCaptureMs = performance.now() - started;
  const addon = Object.keys(require.cache).find(p => path.basename(p) === 'shotium.node')!;
  const hash = (bytes: Buffer) => createHash('sha256').update(bytes).digest('hex');
  const result: Measurements = {package: packagePath, addon,
    addonSha256: hash(readFileSync(addon)), importMs: imported - begin,
    startMs: started - imported, firstCaptureMs, imageSha256: hash(first.image!), cases: {}};
  for (let i = 0; i < 5; ++i) await runtime.screenshot(request);
  for (const mixed of [false, true]) {
    const capture: number[] = [], io: number[] = [];
    let peakRss = process.memoryUsage().rss;
    const histogram = monitorEventLoopDelay({resolution: 1});
    histogram.enable();
    const start = performance.now();
    for (let i = 0; i < 100; ++i) {
      const round = performance.now();
      const crypto = mixed ? promisify(pbkdf2)('password', 'salt', 100000, 32, 'sha256') : Promise.resolve();
      const screenshot = runtime.screenshot(request).then(() => capture.push(performance.now() - round));
      const reading = mixed ? readFile(request.file).then(() => io.push(performance.now() - round)) : Promise.resolve();
      await Promise.all([crypto, screenshot, reading]);
      peakRss = Math.max(peakRss, process.memoryUsage().rss);
    }
    histogram.disable();
    result.cases[mixed ? 'mixed' : 'isolated'] = {capture, io, elapsedMs: performance.now() - start,
      eventLoopP95Ms: histogram.percentile(95) / 1e6, peakRss};
  }
  await runtime.stop();
  process.stdout.write(JSON.stringify(result));
}
async function compare(baseline: string, candidate: string, output: string): Promise<void> {
  const runs: Array<{side: string; measurements: Measurements}> = [];
  for (const side of ['baseline', 'candidate', 'candidate', 'baseline']) {
    const packagePath = resolve(side === 'baseline' ? baseline : candidate);
    const result = await execa(process.execPath,
      [...process.execArgv, import.meta.filename, 'worker', packagePath],
      {env: {UV_THREADPOOL_SIZE: '1'}, timeout: 180000});
    runs.push({side, measurements: JSON.parse(result.stdout) as Measurements});
    console.log(`${side}: completed ${packagePath}`);
  }
  if (new Set(runs.map(r => r.measurements.imageSha256)).size !== 1) throw new Error('baseline/candidate image mismatch');
  const summary = Object.fromEntries(['baseline', 'candidate'].map(side => [side,
    Object.fromEntries(['isolated', 'mixed'].map(name => {
      const rows = runs.filter(r => r.side === side).map(r => r.measurements.cases[name]);
      const capture = rows.flatMap(r => r.capture), io = rows.flatMap(r => r.io);
      return [name, {captureP50Ms: quantile(capture, 0.5), captureP95Ms: quantile(capture, 0.95),
        ioP50Ms: io.length ? quantile(io, 0.5) : null, ioP95Ms: io.length ? quantile(io, 0.95) : null,
        screenshotsPerSecond: capture.length * 1000 / rows.reduce((n, r) => n + r.elapsedMs, 0),
        eventLoopP95Ms: rows.map(r => r.eventLoopP95Ms), peakRss: Math.max(...rows.map(r => r.peakRss))}];
    }))]));
  writeFileSync(resolve(output), JSON.stringify({purpose: 'boundary diagnostic, not the calibrated release gate', runs, summary}, null, 2));
  console.log(JSON.stringify(summary, null, 2));
}
if (process.argv[2] === 'worker') {
  await worker(process.argv[3]);
} else {
  const cli = cac('pnpm perf:node-entry');
  cli.command('<baseline> <candidate> <output>', 'ABBA screenshot and libuv contention measurements')
      .action(compare);
  cli.help(); cli.parse();
}
