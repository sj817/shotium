import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import process from 'node:process';
import {pathToFileURL} from 'node:url';
import {execa} from 'execa';
import {parseArgs, recoverNpmRunValues} from './args.ts';
import {APP_ROOT, FIXTURE_ROOT} from './constants.ts';
import {startFixtureServer, loadCases} from './fixtures.ts';
import {comparePng, inspectPng} from './image.ts';
import {ProcessMonitor, terminateOwnedProcesses} from './process-tree.ts';
import {distribution, round} from './statistics.ts';

const TIMEOUT_MS = 30_000;
const HARD_TIMEOUT_MS = TIMEOUT_MS + 10_000;
const SAMPLE_INTERVAL_MS = 10;
const BASELINE_ENGINES = new Set(['headless-shell', 'system-chrome']);

export const nativeUsage = `Usage:
  npm run benchmark:native -- \\
    --baseline-executable PATH \\
    --baseline-engine headless-shell|system-chrome \\
    --shot-executable PATH [options]

Options:
  --iterations N          Recorded captures per engine and case (default: 5)
  --warmup-iterations N   Unrecorded captures per engine and case (default: 1)
  --output DIRECTORY      Report and PNG directory (default: apps/benchmark/out-native)
  --help                  Show this help`;

function integerOption(value: unknown, name: string, minimum: number, maximum: number) {
  const parsed = Number(value);
  if (!Number.isInteger(parsed) || parsed < minimum || parsed > maximum) {
    throw new Error(`--${name} must be an integer from ${minimum} to ${maximum}`);
  }
  return parsed;
}

export function parseNativeOptions(argv: string[], cwd = process.cwd()) {
  const args = parseArgs(argv, {
    baselineEngine: 'headless-shell',
    iterations: 5,
    warmupIterations: 1,
    output: path.join(APP_ROOT, 'out-native'),
  });
  recoverNpmRunValues(args, [
    'baselineExecutable', 'baselineEngine', 'shotExecutable', 'iterations',
    'warmupIterations', 'output',
  ]);
  const allowed = new Set([
    'baselineExecutable', 'baselineEngine', 'shotExecutable', 'iterations',
    'warmupIterations', 'output', 'help',
  ]);
  for (const key of Object.keys(args)) {
    if (key === '_') throw new Error(`unexpected argument ${args._[0]}`);
    if (!allowed.has(key)) throw new Error(`unknown option --${key}`);
  }
  if (args.help) return {...args, help: true};
  if (typeof args.baselineExecutable !== 'string') {
    throw new Error('--baseline-executable is required');
  }
  if (!BASELINE_ENGINES.has(args.baselineEngine)) {
    throw new Error('--baseline-engine must be headless-shell or system-chrome');
  }
  if (typeof args.shotExecutable !== 'string') throw new Error('--shot-executable is required');
  if (typeof args.output !== 'string') throw new Error('--output requires a directory');
  return {
    baselineExecutable: path.resolve(cwd, args.baselineExecutable),
    baselineEngine: args.baselineEngine,
    shotExecutable: path.resolve(cwd, args.shotExecutable),
    iterations: integerOption(args.iterations, 'iterations', 1, 1000),
    warmupIterations: integerOption(args.warmupIterations, 'warmup-iterations', 0, 100),
    output: path.resolve(cwd, args.output),
    help: false,
  };
}

function sha256File(file: string) {
  return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
}

function requireExecutable(file: string, option: string) {
  let stat;
  try {
    stat = fs.statSync(file);
  } catch {
    throw new Error(`${option} does not exist: ${file}`);
  }
  if (!stat.isFile()) throw new Error(`${option} is not a file: ${file}`);
  return stat;
}

async function executableVersion(executable: string) {
  try {
    const result = await execa(executable, ['--version'], {
      cwd: path.dirname(executable),
      timeout: 5000,
      reject: false,
      windowsHide: true,
    });
    if (result.exitCode === 0) {
      return (result.stdout || result.stderr).trim().split(/\r?\n/, 1)[0] || null;
    }
  } catch {}
  return null;
}

async function executableIdentity(role: string, engine: string, executable: string) {
  const stat = requireExecutable(executable, `--${role}-executable`);
  return {
    role,
    engine,
    executable,
    binary_bytes: stat.size,
    binary_sha256: sha256File(executable),
    binary_version: await executableVersion(executable),
  };
}

function temporaryProfile() {
  return fs.mkdtempSync(path.join(os.tmpdir(), 'shot-native-chrome-'));
}

function removeTemporaryProfile(directory: string) {
  const tempRoot = path.resolve(os.tmpdir());
  const resolved = path.resolve(directory);
  const relative = path.relative(tempRoot, resolved);
  if (relative.startsWith('..') || path.isAbsolute(relative) ||
      !path.basename(resolved).startsWith('shot-native-chrome-')) {
    throw new Error(`refusing to remove profile outside the temporary root: ${resolved}`);
  }
  fs.rmSync(resolved, {recursive: true, force: true});
}

function commandFor(engine: string, executable: string, fixtureCase: any, output: string) {
  if (engine === 'shot') {
    return {
      executable,
      profile: null,
      arguments: [
        fixtureCase.url,
        '--width', String(fixtureCase.width),
        '--height', String(fixtureCase.height),
        '--scale', String(fixtureCase.scale),
        '--timeout-ms', String(TIMEOUT_MS),
        '--output', output,
      ],
    };
  }
  const profile = engine === 'system-chrome' ? temporaryProfile() : null;
  const arguments_ = [];
  if (engine === 'system-chrome') {
    arguments_.push('--headless=new', `--user-data-dir=${profile}`);
  }
  arguments_.push(
      '--disable-background-networking', '--disable-component-update',
      '--disable-default-apps', '--disable-extensions', '--disable-sync',
      '--no-first-run', '--no-default-browser-check',
      '--allow-file-access-from-files', '--hide-scrollbars',
      '--run-all-compositor-stages-before-draw',
      `--force-device-scale-factor=${fixtureCase.scale}`,
      `--window-size=${fixtureCase.width},${fixtureCase.height}`,
      `--screenshot=${output}`, fixtureCase.url);
  return {executable, profile, arguments: arguments_};
}

async function capture(engine: string, executable: string, fixtureCase: any, output: string) {
  fs.mkdirSync(path.dirname(output), {recursive: true});
  if (fs.existsSync(output)) fs.unlinkSync(output);
  const command = commandFor(engine, executable, fixtureCase, output);
  const started = performance.now();
  let monitorResult: any = null;
  let result;
  try {
    const child = execa(command.executable, command.arguments, {
      cwd: path.dirname(command.executable),
      timeout: HARD_TIMEOUT_MS,
      reject: false,
      windowsHide: true,
    });
    if (!child.pid) throw new Error(`${engine} did not expose a process id`);
    const monitor = new ProcessMonitor([child.pid], SAMPLE_INTERVAL_MS);
    await monitor.start();
    try {
      result = await child;
    } finally {
      monitorResult = await monitor.stop();
      if (monitorResult.identities.length) {
        const remaining = await terminateOwnedProcesses(monitorResult.identities);
        if (remaining.length) {
          throw new Error(`${engine} left owned processes running: ${remaining.map((entry) => entry.pid).join(', ')}`);
        }
      }
    }
  } finally {
    if (command.profile) removeTemporaryProfile(command.profile);
  }
  const wallTimeMs = round(performance.now() - started);
  if (result.timedOut) throw new Error(`${engine} exceeded the ${HARD_TIMEOUT_MS} ms hard timeout`);
  if (result.exitCode !== 0) {
    throw new Error(`${engine} exited with code ${result.exitCode}: ${result.stderr.trim()}`);
  }
  if (!monitorResult.trusted) {
    throw new Error(`${engine} process-tree measurement was not trustworthy: ${
      monitorResult.errors.join('; ') || 'the root process was not observed'}`);
  }
  if (!fs.existsSync(output)) throw new Error(`${engine} did not produce ${output}`);
  const png = inspectPng(fs.readFileSync(output), {
    width: fixtureCase.width,
    height: fixtureCase.height,
  });
  const validTimeline = monitorResult.timeline.filter((sample) => Number.isFinite(sample.rss_bytes));
  return {
    wall_time_ms: wallTimeMs,
    peak_rss_bytes: monitorResult.peak_rss_bytes,
    peak_processes: validTimeline.length ?
      Math.max(...validTimeline.map((sample) => sample.processes.length)) : null,
    process_samples: validTimeline.length,
    process_sample_requested_interval_ms: SAMPLE_INTERVAL_MS,
    process_observed_mean_sample_period_ms: monitorResult.observed_mean_period_ms,
    process_monitor_trusted: monitorResult.trusted,
    process_monitor_errors: monitorResult.errors,
    processes_seen: monitorResult.identities.length,
    process_names: [...new Set(monitorResult.identities.map((entry) => entry.name))].sort(),
    png,
  };
}

function groupSummary(samples: any[]) {
  const groups = new Map<string, any[]>();
  for (const sample of samples) {
    const key = `${sample.engine}\u0000${sample.case}`;
    const rows = groups.get(key) || [];
    rows.push(sample);
    groups.set(key, rows);
  }
  return [...groups.values()].map((rows) => ({
    engine: rows[0].engine,
    case: rows[0].case,
    samples: rows.length,
    wall_time_ms: distribution(rows.map((row) => row.wall_time_ms)),
    peak_rss_bytes: distribution(rows.map((row) => row.peak_rss_bytes)),
  }));
}

export function sameMachineRatios(summary: any[], baselineLabel: string, shotLabel: string) {
  const cases = new Set<string>(summary.map((row) => row.case));
  return [...cases].sort().map((caseName) => {
    const baseline = summary.find((row) => row.engine === baselineLabel && row.case === caseName);
    const shot = summary.find((row) => row.engine === shotLabel && row.case === caseName);
    return {
      case: caseName,
      baseline_wall_p50_ms: baseline?.wall_time_ms?.p50 ?? null,
      shot_wall_p50_ms: shot?.wall_time_ms?.p50 ?? null,
      baseline_to_shot_wall_p50_ratio: baseline && shot ?
        round(baseline.wall_time_ms.p50 / shot.wall_time_ms.p50) : null,
      baseline_peak_rss_p50_bytes: baseline?.peak_rss_bytes?.p50 ?? null,
      shot_peak_rss_p50_bytes: shot?.peak_rss_bytes?.p50 ?? null,
      baseline_to_shot_peak_rss_p50_ratio: baseline?.peak_rss_bytes && shot?.peak_rss_bytes ?
        round(baseline.peak_rss_bytes.p50 / shot.peak_rss_bytes.p50) : null,
    };
  });
}

function csvCell(value: unknown) {
  const text = Array.isArray(value) ? value.join(';') : String(value ?? '');
  return /[",\r\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
}

function writeSamplesCsv(file: string, samples: any[]) {
  const fields = [
    'engine', 'case', 'iteration', 'wall_time_ms', 'peak_rss_bytes', 'peak_processes',
    'process_samples', 'process_monitor_trusted', 'processes_seen', 'process_names',
    'png_width', 'png_height', 'png_bytes', 'png_sha256', 'png_file',
  ];
  const lines = [fields.join(',')];
  for (const sample of samples) {
    const row: any = {
      ...sample,
      png_width: sample.png.width,
      png_height: sample.png.height,
      png_bytes: sample.png.bytes,
      png_sha256: sample.png.sha256,
    };
    lines.push(fields.map((field) => csvCell(row[field])).join(','));
  }
  fs.writeFileSync(file, `${lines.join('\n')}\n`);
}

async function gitRevision() {
  const result = await execa('git', ['rev-parse', 'HEAD'], {
    cwd: path.resolve(APP_ROOT, '..', '..'), reject: false,
  });
  return result.exitCode === 0 ? result.stdout.trim() : null;
}

export async function runNativeBenchmark(options: any) {
  requireExecutable(options.baselineExecutable, '--baseline-executable');
  requireExecutable(options.shotExecutable, '--shot-executable');
  fs.mkdirSync(options.output, {recursive: true});
  const imagesDirectory = path.join(options.output, 'images');
  const baselineLabel = options.baselineEngine === 'system-chrome' ?
    'external-system-chrome' : 'source-build-headless-shell-baseline';
  const shotLabel = 'source-build-shot';
  const engines = [
    {engine: options.baselineEngine, label: baselineLabel, executable: options.baselineExecutable},
    {engine: 'shot', label: shotLabel, executable: options.shotExecutable},
  ];
  const server = await startFixtureServer();
  const samples: any[] = [];
  const lastImages = new Map<string, string>();
  try {
    const cases = loadCases(server.baseUrl);
    for (const engine of engines) {
      for (const fixtureCase of cases) {
        for (let warmup = 1; warmup <= options.warmupIterations; warmup += 1) {
          process.stdout.write(`warmup ${engine.label} ${fixtureCase.name} ${warmup}/${options.warmupIterations}\n`);
          const file = path.join(imagesDirectory,
              `${engine.label}.${fixtureCase.name}.warmup-${warmup}.png`);
          await capture(engine.engine, engine.executable, fixtureCase, file);
          fs.unlinkSync(file);
        }
        for (let iteration = 1; iteration <= options.iterations; iteration += 1) {
          process.stdout.write(`measure ${engine.label} ${fixtureCase.name} ${iteration}/${options.iterations}\n`);
          const file = path.join(imagesDirectory,
              `${engine.label}.${fixtureCase.name}.${iteration}.png`);
          const measurement = await capture(engine.engine, engine.executable, fixtureCase, file);
          samples.push({
            engine: engine.label,
            engine_kind: engine.engine,
            case: fixtureCase.name,
            iteration,
            input: fixtureCase.url,
            png_file: path.relative(options.output, file).replaceAll('\\', '/'),
            ...measurement,
          });
          lastImages.set(`${engine.label}\u0000${fixtureCase.name}`, file);
        }
      }
    }
    const summary = groupSummary(samples);
    const ratios = sameMachineRatios(summary, baselineLabel, shotLabel);
    const pngComparisons = cases.map((fixtureCase) => {
      const baselineFile = lastImages.get(`${baselineLabel}\u0000${fixtureCase.name}`);
      const shotFile = lastImages.get(`${shotLabel}\u0000${fixtureCase.name}`);
      return {
        case: fixtureCase.name,
        ...comparePng(fs.readFileSync(baselineFile), fs.readFileSync(shotFile)),
      };
    });
    const identities = await Promise.all(engines.map((engine) =>
      executableIdentity(engine.engine === 'shot' ? 'shot' : 'baseline',
          engine.engine, engine.executable)));
    const report = {
      schema_version: 1,
      benchmark: 'native-cli',
      generated_utc: new Date().toISOString(),
      source_revision: await gitRevision(),
      measurement_model: {
        lifecycle: 'one-new-process-tree-per-capture',
        warmup_note: 'Warmups populate host file caches; every recorded sample includes a new process startup.',
        wall_time: 'elapsed host time from process spawn through exit and owned-process cleanup',
        rss: 'maximum sampled sum of RSS bytes across the discovered owned process tree',
        ratio: 'baseline p50 divided by Shot p50 from this report on this machine; values above 1 favor Shot',
      },
      host: {
        machine: os.hostname(),
        platform: process.platform,
        architecture: process.arch,
        os_release: os.release(),
        processor: os.cpus()[0]?.model || null,
        logical_processors: os.cpus().length,
        node: process.version,
      },
      config: {
        iterations: options.iterations,
        warmup_iterations: options.warmupIterations,
        timeout_ms: TIMEOUT_MS,
        process_sample_requested_interval_ms: SAMPLE_INTERVAL_MS,
        cases_manifest_sha256: sha256File(path.join(FIXTURE_ROOT, 'cases.json')),
      },
      engines: identities,
      summary,
      same_machine_ratios: ratios,
      png_comparisons: pngComparisons,
      samples,
    };
    const jsonFile = path.join(options.output, 'benchmark.json');
    const csvFile = path.join(options.output, 'benchmark.csv');
    fs.writeFileSync(jsonFile, `${JSON.stringify(report, null, 2)}\n`);
    writeSamplesCsv(csvFile, samples);
    process.stdout.write(`JSON: ${jsonFile}\nCSV:  ${csvFile}\n`);
    return report;
  } finally {
    await server.close();
  }
}

async function main() {
  const options = parseNativeOptions(process.argv.slice(2));
  if (options.help) {
    process.stdout.write(`${nativeUsage}\n`);
    return;
  }
  await runNativeBenchmark(options);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main().catch((error) => {
    process.stderr.write(`${error?.stack || error}\n`);
    process.exitCode = 1;
  });
}
