// Static render regression: decoded-pixel comparison of the engine against
// a locally generated baseline.
//
// The fixtures in tests/render/cases contain no JavaScript, animation, clocks,
// random values, or public-network dependencies. They run at a fixed viewport
// and scale. Baselines are generated on this machine from a pinned engine
// (a source-built headless_shell, or an installed Chrome, which the manifest
// records as external-system-chrome) and are gitignored: two lossless PNG
// encoders can produce different bytes for identical pixels, so the
// comparison is on decoded pixels and SHA-256 is only an artifact-integrity
// signal.
//
//   pnpm render:regression update-baselines --baseline-executable out/Release/headless_shell.exe --accept
//   pnpm render:regression update-baselines --baseline-engine system-chrome --baseline-executable "C:/.../chrome.exe" --accept
//   pnpm render:regression run --shot out/Shot/shotium.exe
//   pnpm render:regression diff expected.png actual.png [--diff diff.png] [--json result.json]
//
// Thresholds default to exact decoded pixel equality. Only relax a per-case
// threshold after reviewing the generated red-on-black diff image and
// documenting the reason in cases.json. Relative paths are resolved against
// the repository root.

import {copyFileSync, existsSync, mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, statSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

import {cac} from 'cac';
import {execaSync} from 'execa';
import {PNG} from 'pngjs';

import {measure, type Measurement} from './lib/measured-process.ts';
import {decodePng} from './lib/png.ts';
import {resolve} from './lib/repo.ts';
import {sha256} from './lib/report.ts';

type Engine = 'system-chrome' | 'headless-shell' | 'shot';
const ENGINES: Engine[] = ['system-chrome', 'headless-shell', 'shot'];

interface Case {
  name: string;
  file: string;
  resources?: string[];
  width: number;
  height: number;
  scale: number;
  max_changed_fraction: number;
  max_channel_delta: number;
}

interface PngInfo {
  path: string;
  width: number;
  height: number;
  bytes: number;
  sha256: string;
}

interface Identity {
  identity: string;
  engine: Engine;
  executable: string;
  binary_bytes: number;
  binary_sha256: string;
  file_version: string | null;
}

// A URL is passed through; anything else is a file, made absolute.
function resolveCaptureInput(value: string): string {
  try {
    const url = new URL(value);
    if (['http:', 'https:', 'file:', 'data:'].includes(url.protocol)) return url.href;
  } catch {
    // not a URL
  }
  const file = resolve(value);
  if (!existsSync(file)) throw new Error(`no such file: ${file}`);
  return pathToFileURL(file).href;
}

function fileVersion(executable: string): string | null {
  if (process.platform !== 'win32') return null;
  const r = execaSync('powershell', ['-NoProfile', '-Command', `(Get-Item -LiteralPath '${executable.replace(/'/g, "''")}').VersionInfo.FileVersion`], {reject: false});
  const v = r.stdout.trim();
  return v || null;
}

function engineIdentity(engine: Engine, executable: string): Identity {
  const file = resolve(executable);
  const identity = {'system-chrome': 'external-system-chrome', 'headless-shell': 'source-build-headless-shell-baseline', shot: 'source-build-shot'}[engine];
  return {identity, engine, executable: file, binary_bytes: statSync(file).size, binary_sha256: sha256(readFileSync(file)), file_version: fileVersion(file)};
}

function pngInfo(file: string): PngInfo {
  const bytes = readFileSync(file);
  if (bytes.length < 24 || !bytes.subarray(0, 8).equals(Buffer.from('89504e470d0a1a0a', 'hex')) || bytes.subarray(12, 16).toString('ascii') !== 'IHDR') {
    throw new Error(`Not a valid PNG with an IHDR chunk: ${file}`);
  }
  return {path: file, width: bytes.readUInt32BE(16), height: bytes.readUInt32BE(20), bytes: bytes.length, sha256: sha256(bytes)};
}

interface Capture {
  engine: Engine;
  input: string;
  output: string;
  viewport_width: number;
  viewport_height: number;
  scale: number;
  png: PngInfo;
  measurement: Measurement;
}

async function screenshotCapture(engine: Engine, executable: string, input: string, output: string,
                                 width: number, height: number, scale: number, timeoutMs: number, samplingIntervalMs: number): Promise<Capture> {
  if (engine === 'shot' && Math.abs(scale - 1) > Number.EPSILON) {
    throw new Error('Shot v1 only supports --scale 1. Non-1 DPR requires explicit screen configuration and is not benchmarked yet.');
  }
  const inputUri = resolveCaptureInput(input);
  const outputFile = path.resolve(output);
  mkdirSync(path.dirname(outputFile), {recursive: true});
  rmSync(outputFile, {force: true});
  const exe = resolve(executable);
  const args: string[] = [];
  let profileDirectory: string | null = null;
  if (engine === 'shot') {
    args.push(inputUri, '--width', String(width), '--height', String(height), '--scale', String(scale), '--timeout-ms', String(timeoutMs), '--output', outputFile);
  } else {
    if (engine === 'system-chrome') {
      args.push('--headless=new');
      profileDirectory = mkdtempSync(path.join(os.tmpdir(), 'shot-chrome-profile-'));
      args.push(`--user-data-dir=${profileDirectory}`);
    }
    args.push(
        '--disable-background-networking', '--disable-component-update', '--disable-default-apps', '--disable-extensions', '--disable-sync',
        '--no-first-run', '--no-default-browser-check', '--allow-file-access-from-files', '--hide-scrollbars',
        '--run-all-compositor-stages-before-draw', `--force-device-scale-factor=${scale}`, `--window-size=${width},${height}`,
        `--screenshot=${outputFile}`, inputUri);
  }
  const hardTimeoutMs = Math.min(900000, timeoutMs + 10000);
  let measurement: Measurement;
  try {
    measurement = await measure(exe, args, {cwd: path.dirname(exe), samplingIntervalMs, timeoutMs: hardTimeoutMs});
  } finally {
    if (profileDirectory && existsSync(profileDirectory)) {
      if (!path.resolve(profileDirectory).toLowerCase().startsWith(path.resolve(os.tmpdir()).toLowerCase())) {
        throw new Error(`Refusing to remove profile outside the temp directory: ${profileDirectory}`);
      }
      rmSync(profileDirectory, {recursive: true, force: true});
    }
  }
  if (measurement.timed_out) throw new Error(`${engine} exceeded the ${hardTimeoutMs} ms hard process timeout.`);
  if (measurement.exit_code !== 0) throw new Error(`${engine} exited with code ${measurement.exit_code}: ${measurement.stderr}`);
  if (!existsSync(outputFile)) throw new Error(`${engine} exited successfully but did not produce ${outputFile}`);
  return {engine, input: inputUri, output: outputFile, viewport_width: width, viewport_height: height, scale, png: pngInfo(outputFile), measurement};
}

interface Comparison {
  dimensions_match: boolean;
  expected: PngInfo;
  actual: PngInfo;
  exact_sha256_match: boolean;
  changed_pixels: number | null;
  changed_fraction: number | null;
  max_channel_delta: number | null;
  mean_absolute_channel_delta: number | null;
  root_mean_square_channel_delta: number | null;
  diff_path: string | null;
}

// Decoded RGBA comparison; the diff image is red where a pixel changed,
// brighter for a bigger delta, on black.
export function comparePng(expectedPath: string, actualPath: string, diffPath?: string): Comparison {
  const expected = pngInfo(expectedPath), actual = pngInfo(actualPath);
  const none = {changed_pixels: null, changed_fraction: null, max_channel_delta: null, mean_absolute_channel_delta: null, root_mean_square_channel_delta: null, diff_path: null};
  if (expected.width !== actual.width || expected.height !== actual.height) {
    return {dimensions_match: false, expected, actual, exact_sha256_match: false, ...none};
  }
  if (expected.sha256 === actual.sha256) {
    return {dimensions_match: true, expected, actual, exact_sha256_match: true, changed_pixels: 0, changed_fraction: 0, max_channel_delta: 0, mean_absolute_channel_delta: 0, root_mean_square_channel_delta: 0, diff_path: null};
  }
  const a = decodePng(readFileSync(expectedPath)), b = decodePng(readFileSync(actualPath));
  const pixels = a.width * a.height;
  const diff = diffPath ? new PNG({width: a.width, height: a.height}) : null;
  let changed = 0, absolute = 0, squared = 0, maxDelta = 0;
  for (let i = 0; i < pixels; i++) {
    let pixelChanged = false, pixelMax = 0;
    for (let c = 0; c < 4; c++) {
      const delta = Math.abs(a.data[i * 4 + c] - b.data[i * 4 + c]);
      if (delta !== 0) pixelChanged = true;
      pixelMax = Math.max(pixelMax, delta);
      maxDelta = Math.max(maxDelta, delta);
      absolute += delta;
      squared += delta * delta;
    }
    if (pixelChanged) changed++;
    if (diff) {
      diff.data[i * 4] = pixelChanged ? Math.max(64, pixelMax) : 0;
      diff.data[i * 4 + 1] = 0;
      diff.data[i * 4 + 2] = 0;
      diff.data[i * 4 + 3] = 255;
    }
  }
  let resolvedDiff: string | null = null;
  if (diff && diffPath) {
    resolvedDiff = path.resolve(diffPath);
    mkdirSync(path.dirname(resolvedDiff), {recursive: true});
    writeFileSync(resolvedDiff, PNG.sync.write(diff));
  }
  const channels = pixels * 4;
  return {
    dimensions_match: true, expected, actual, exact_sha256_match: false,
    changed_pixels: changed, changed_fraction: pixels ? changed / pixels : 0, max_channel_delta: maxDelta,
    mean_absolute_channel_delta: channels ? absolute / channels : 0, root_mean_square_channel_delta: channels ? Math.sqrt(squared / channels) : 0,
    diff_path: resolvedDiff,
  };
}

const readJson = <T>(file: string) => JSON.parse(readFileSync(file, 'utf8')) as T;
const writeJson = (file: string, value: unknown) => writeFileSync(file, JSON.stringify(value, null, 2) + '\n');

interface BaselineManifest {
  schema_version: number;
  baseline_engine: Omit<Identity, 'executable'> & {executable_name: string};
  cases_manifest_sha256: string;
  cases: Array<{name: string; source: string; source_sha256: string; resources: Array<{path: string; sha256: string}>; png: string; png_width: number; png_height: number; png_bytes: number; png_sha256: string}>;
}

async function run(o: {shot: string; cases: string; baselines: string; artifacts: string; timeoutMs: number; samplingIntervalMs: number; maxChangedFraction?: number; maxChannelDelta?: number}): Promise<number> {
  const casesPath = resolve(o.cases);
  const casesRoot = path.dirname(casesPath);
  const baselineRoot = resolve(o.baselines);
  const baselineManifestPath = path.join(baselineRoot, 'manifest.json');
  if (!existsSync(baselineManifestPath)) throw new Error(`Missing baseline manifest. Run update-baselines --accept first: ${baselineManifestPath}`);
  const cases = readJson<Case[]>(casesPath);
  const manifest = readJson<BaselineManifest>(baselineManifestPath);
  if (manifest.cases_manifest_sha256 !== sha256(readFileSync(casesPath))) {
    throw new Error('cases.json changed after the baselines were generated. Regenerate and review the baselines.');
  }
  const artifactRoot = resolve(o.artifacts);
  mkdirSync(artifactRoot, {recursive: true});
  const engine = engineIdentity('shot', o.shot);
  const results: Array<Record<string, unknown> & {name: string; passed: boolean; diff: Comparison}> = [];
  let failures = 0;
  for (const c of cases) {
    const source = path.resolve(casesRoot, c.file);
    const entries = manifest.cases.filter((e) => e.name === c.name);
    if (entries.length !== 1) throw new Error(`Baseline manifest has ${entries.length} entries for ${c.name}.`);
    const entry = entries[0];
    const baseline = path.join(baselineRoot, entry.png);
    if (sha256(readFileSync(source)) !== entry.source_sha256) throw new Error(`Fixture source drifted after baseline generation: ${source}`);
    for (const resource of entry.resources ?? []) {
      const resourcePath = path.resolve(casesRoot, resource.path);
      if (sha256(readFileSync(resourcePath)) !== resource.sha256) throw new Error(`Fixture resource drifted after baseline generation: ${resourcePath}`);
    }
    if (sha256(readFileSync(baseline)) !== entry.png_sha256) throw new Error(`Baseline PNG does not match its manifest: ${baseline}`);

    const actual = path.join(artifactRoot, `${c.name}.actual.png`);
    const diffImage = path.join(artifactRoot, `${c.name}.diff.png`);
    console.log(`render ${c.name}: source-build-shot`);
    const capture = await screenshotCapture('shot', o.shot, source, actual, c.width, c.height, c.scale, o.timeoutMs, o.samplingIntervalMs);
    const diff = comparePng(baseline, actual, diffImage);
    const fractionLimit = o.maxChangedFraction ?? c.max_changed_fraction;
    const channelLimit = o.maxChannelDelta ?? c.max_channel_delta;
    const passed = diff.dimensions_match && (diff.changed_fraction ?? Infinity) <= fractionLimit && (diff.max_channel_delta ?? Infinity) <= channelLimit;
    if (!passed) failures++;
    results.push({name: c.name, passed, limits: {max_changed_fraction: fractionLimit, max_channel_delta: channelLimit}, capture, diff});
  }
  const report = {schema_version: 1, engine, baseline_engine: manifest.baseline_engine, passed: failures === 0, failures, results};
  const reportPath = path.join(artifactRoot, 'report.json');
  writeJson(reportPath, report);
  console.table(results.map((r) => ({
    case: r.name, pass: r.passed, dimensions: r.diff.dimensions_match ? 'match' : 'mismatch', changed_pixels: r.diff.changed_pixels,
    changed_fraction: r.diff.changed_fraction, max_delta: r.diff.max_channel_delta, sha_match: r.diff.exact_sha256_match,
  })));
  console.log(`Report: ${reportPath}`);
  if (failures !== 0) throw new Error(`${failures} render regression case(s) exceeded their pixel thresholds.`);
  return 0;
}

async function updateBaselines(o: {baselineExecutable: string; baselineEngine: Engine; cases: string; baselines: string; timeoutMs: number; samplingIntervalMs: number; accept: boolean}): Promise<number> {
  if (!o.accept) throw new Error('Baseline replacement is intentional and destructive. Re-run with --accept after reviewing the pinned baseline executable.');
  if (o.baselineEngine === 'shot') throw new Error('--baseline-engine must be headless-shell or system-chrome');
  const manifestPath = resolve(o.cases);
  const manifestRoot = path.dirname(manifestPath);
  const cases = readJson<Case[]>(manifestPath);
  const baselineRoot = resolve(o.baselines);
  mkdirSync(baselineRoot, {recursive: true});
  const staging = mkdtempSync(path.join(os.tmpdir(), 'shot-baselines-'));
  const engine = engineIdentity(o.baselineEngine, o.baselineExecutable);
  const caseResults: BaselineManifest['cases'] = [];
  try {
    for (const c of cases) {
      const source = path.resolve(manifestRoot, c.file);
      const output = path.join(staging, `${c.name}.png`);
      console.log(`baseline ${c.name}: ${o.baselineEngine}`);
      const capture = await screenshotCapture(o.baselineEngine, o.baselineExecutable, source, output, c.width, c.height, c.scale, o.timeoutMs, o.samplingIntervalMs);
      const expectedWidth = Math.round(c.width * c.scale), expectedHeight = Math.round(c.height * c.scale);
      if (capture.png.width !== expectedWidth || capture.png.height !== expectedHeight) {
        throw new Error(`Baseline ${c.name} has ${capture.png.width}x${capture.png.height}, expected ${expectedWidth}x${expectedHeight}.`);
      }
      const resources = (c.resources ?? []).map((r) => ({path: r, sha256: sha256(readFileSync(path.resolve(manifestRoot, r)))}));
      caseResults.push({
        name: c.name, source: c.file, source_sha256: sha256(readFileSync(source)), resources, png: `${c.name}.png`,
        png_width: capture.png.width, png_height: capture.png.height, png_bytes: capture.png.bytes, png_sha256: capture.png.sha256,
      });
    }
    const manifest: BaselineManifest = {
      schema_version: 1,
      baseline_engine: {
        identity: engine.identity, engine: engine.engine, executable_name: path.basename(engine.executable),
        binary_bytes: engine.binary_bytes, binary_sha256: engine.binary_sha256, file_version: engine.file_version,
      },
      cases_manifest_sha256: sha256(readFileSync(manifestPath)),
      cases: caseResults,
    };
    writeJson(path.join(staging, 'manifest.json'), manifest);
    for (const name of readdirSync(staging)) {
      if (name.endsWith('.png')) copyFileSync(path.join(staging, name), path.join(baselineRoot, name));
    }
    copyFileSync(path.join(staging, 'manifest.json'), path.join(baselineRoot, 'manifest.json'));
  } finally {
    if (!path.resolve(staging).toLowerCase().startsWith(path.resolve(os.tmpdir()).toLowerCase())) {
      throw new Error(`Refusing to remove baseline staging directory outside temp: ${staging}`);
    }
    rmSync(staging, {recursive: true, force: true});
  }
  console.log(`Updated ${caseResults.length} baselines in ${baselineRoot}`);
  return 0;
}

function diffCommand(expected: string, actual: string, o: {diff?: string; maxChangedFraction: number; maxChannelDelta: number; json?: string}): number {
  const result = comparePng(resolve(expected), resolve(actual), o.diff ? resolve(o.diff) : undefined);
  if (o.json) {
    const jsonPath = resolve(o.json);
    mkdirSync(path.dirname(jsonPath), {recursive: true});
    writeJson(jsonPath, result);
  }
  for (const [key, value] of Object.entries(result)) console.log(`${key.padEnd(32)}: ${typeof value === 'object' && value !== null ? JSON.stringify(value) : String(value)}`);
  const passed = result.dimensions_match && (result.changed_fraction ?? Infinity) <= o.maxChangedFraction && (result.max_channel_delta ?? Infinity) <= o.maxChannelDelta;
  if (!passed) throw new Error('PNG comparison exceeded the requested threshold.');
  return 0;
}

// cac 7 registers boolean options under their camelCase name only, so a
// value after `--accept` would be swallowed; the flag is read off argv.
const argv = process.argv.slice(2);
const accept = argv.includes('--accept');
const cli = cac('render-regression');
cli.command('run', 'render every case with the engine and compare it with the baseline')
    .option('--shot <exe>', 'the engine executable', {default: 'out/Shot/shotium.exe'})
    .option('--cases <file>', 'the case manifest', {default: 'tests/render/cases.json'})
    .option('--baselines <dir>', 'where the baselines are', {default: 'tests/render/baselines'})
    .option('--artifacts <dir>', 'where renders, diffs and report.json go', {default: 'tests/render/out'})
    .option('--timeout-ms <ms>', 'per-render timeout', {default: 30000})
    .option('--sampling-interval-ms <ms>', 'process sampling interval', {default: 10})
    .option('--max-changed-fraction <n>', 'override every case\'s changed-pixel limit')
    .option('--max-channel-delta <n>', 'override every case\'s channel-delta limit')
    .action(async (options: Record<string, unknown>) => {
      process.exitCode = await run({
        shot: String(options.shot), cases: String(options.cases), baselines: String(options.baselines), artifacts: String(options.artifacts),
        timeoutMs: Number(options.timeoutMs), samplingIntervalMs: Number(options.samplingIntervalMs),
        maxChangedFraction: options.maxChangedFraction === undefined ? undefined : Number(options.maxChangedFraction),
        maxChannelDelta: options.maxChannelDelta === undefined ? undefined : Number(options.maxChannelDelta),
      });
    });
cli.command('update-baselines', 'replace the baselines with renders from a pinned engine')
    .option('--baseline-executable <exe>', 'the engine that produces the baselines')
    .option('--baseline-engine <name>', 'headless-shell or system-chrome', {default: 'headless-shell'})
    .option('--cases <file>', 'the case manifest', {default: 'tests/render/cases.json'})
    .option('--baselines <dir>', 'where the baselines go', {default: 'tests/render/baselines'})
    .option('--timeout-ms <ms>', 'per-render timeout', {default: 30000})
    .option('--sampling-interval-ms <ms>', 'process sampling interval', {default: 10})
    .option('--accept', 'confirm the replacement')
    .action(async (options: Record<string, unknown>) => {
      if (!options.baselineExecutable) throw new Error('--baseline-executable is required');
      if (!ENGINES.includes(options.baselineEngine as Engine)) throw new Error('--baseline-engine must be headless-shell or system-chrome');
      process.exitCode = await updateBaselines({
        baselineExecutable: String(options.baselineExecutable), baselineEngine: options.baselineEngine as Engine, cases: String(options.cases),
        baselines: String(options.baselines), timeoutMs: Number(options.timeoutMs), samplingIntervalMs: Number(options.samplingIntervalMs), accept,
      });
    });
cli.command('diff <expected> <actual>', 'compare two PNGs pixel by pixel')
    .option('--diff <file>', 'where the red-on-black difference image goes')
    .option('--max-changed-fraction <n>', 'pass threshold', {default: 0})
    .option('--max-channel-delta <n>', 'pass threshold', {default: 0})
    .option('--json <file>', 'write the comparison as JSON')
    .action((expected: string, actual: string, options: Record<string, unknown>) => {
      process.exitCode = diffCommand(expected, actual, {
        diff: options.diff === undefined ? undefined : String(options.diff), maxChangedFraction: Number(options.maxChangedFraction),
        maxChannelDelta: Number(options.maxChannelDelta), json: options.json === undefined ? undefined : String(options.json),
      });
    });
cli.help();
try {
  cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '--accept')], {run: false});
  if (!cli.matchedCommand && !cli.options.help) {
    cli.outputHelp();
    process.exitCode = 2;
  } else {
    await cli.runMatchedCommand();
  }
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exitCode = 1;
}
