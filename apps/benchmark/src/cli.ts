import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execa, execaNode} from 'execa';
import si from 'systeminformation';
import {parseArgs, booleanArg, recoverNpmRunValues} from './args.ts';
import {
  APP_ROOT,
  ENGINE_IDS,
  FIXTURE_ROOT,
  PROFILES,
  SETTLE,
  currentPlatformId,
} from './constants.ts';
import {packageVersions, probeEngine} from './engines.ts';
import {ProductError, isProductError} from './errors.ts';
import {startFixtureServer, loadCases} from './fixtures.ts';
import {ensureShotium} from './install-target.ts';
import {
  ProcessMonitor,
  terminateOwnedProcesses,
  verifyProcessMonitoring,
  waitForIdentitiesToExit,
  waitForSystemStable,
} from './process-tree.ts';
import {distribution, balancedOrder} from './statistics.ts';
import {validatePlatformResult} from './schema.ts';
import {verifyConsumerInstall} from './verify-install.ts';

const options = parseArgs(process.argv.slice(2), {
  shotiumVersion: 'latest',
  profile: 'full',
  output: path.join(APP_ROOT, 'out', `${Date.now()}-${process.pid}`),
  seed: process.env.GITHUB_SHA || process.env.GITHUB_RUN_ID || 'local',
});
recoverNpmRunValues(options, ['shotiumVersion', 'profile', 'output', 'seed', 'skipInstall']);
const require = createRequire(import.meta.url);
const tsxCli = require.resolve('tsx/cli');

const platform = currentPlatformId();
const outputDirectory = path.resolve(options.output);
const permanentDirectory = path.join(outputDirectory, 'permanent', platform);
const artifactDirectory = path.join(outputDirectory, 'artifact', platform);
const telemetryDirectory = path.join(artifactDirectory, 'telemetry');
const logsDirectory = path.join(artifactDirectory, 'logs');
const tempProfileDirectory = path.join(artifactDirectory, 'temp-profiles');
const samplesFile = path.join(permanentDirectory, 'samples.jsonl');
const workerFile = path.join(APP_ROOT, 'src', 'worker.ts');
const benchmarkDaemonName = `shot-bench-${process.env.GITHUB_RUN_ID || process.pid}-${platform}`;
let outputInitialized = false;
let resolvedShotiumVersion: string | null = null;
let completedRows: any[] = [];
let completedQuality: any[] = [];
let completedFailures: any[] = [];

function writeJson(file, value) {
  fs.mkdirSync(path.dirname(file), {recursive: true});
  fs.writeFileSync(file, `${JSON.stringify(value, null, 2)}\n`);
}

function appendJsonLine(value) {
  fs.appendFileSync(samplesFile, `${JSON.stringify(value)}\n`);
}

function parseWorkerOutput(stdout) {
  const line = stdout.trim().split(/\r?\n/).filter(Boolean).at(-1);
  if (!line) throw new Error('benchmark worker produced no JSON');
  return JSON.parse(line);
}

async function gitRevision() {
  if (process.env.GITHUB_SHA) return process.env.GITHUB_SHA;
  const result = await execa('git', ['rev-parse', 'HEAD'], {cwd: path.resolve(APP_ROOT, '..', '..'), reject: false});
  return result.exitCode === 0 ? result.stdout.trim() : null;
}

function buildGroups(engines, profile) {
  const groups = [];
  let rotation = 0;
  const add = (scenario, repeat, concurrency, iterations = 1, timeoutMs = undefined,
      selectedEngines = engines) => {
    groups.push({
      scenario, repeat, concurrency, iterations, timeoutMs,
      engines: balancedOrder(selectedEngines, rotation++, `${options.seed}:${scenario}`),
    });
  };
  const chunks = (total, count) => Array.from({length: count}, (_, index) =>
    Math.floor(total / count) + (index < total % count ? 1 : 0));
  for (const scenario of ['cold', 'cold-settled']) {
    for (let repeat = 1; repeat <= profile.repeats; repeat += 1) {
      add(scenario, repeat, 1);
    }
  }
  chunks(profile.warmIterations, profile.warmChunks)
      .forEach((iterations, index) => add('warm', index + 1, 1, iterations));
  add('reuse-page', 1, 1, profile.warmIterations, undefined,
      engines.filter((engine) => engine !== 'shotium'));
  for (let repeat = 1; repeat <= profile.batchRounds; repeat += 1) add('batch', repeat, 1);
  for (let repeat = 1; repeat <= profile.repeats; repeat += 1) add('resident', repeat, 1);
  chunks(profile.lifecycleCycles, profile.lifecycleChunks)
      .forEach((iterations, index) => add('lifecycle', index + 1, 1, iterations));
  add('faults', 1, 1);
  for (const concurrency of profile.concurrencies) {
    for (let repeat = 1; repeat <= profile.batchRounds; repeat += 1) {
      add('parallel', repeat, concurrency, 1);
    }
  }
  add('soak', 1, 4, profile.soakIterations, profile.soakTimeoutMs);
  return groups;
}

async function cleanupOwnedShotiumEndpoints(profile) {
  const shotium = await import('@shotkit/shotium');
  const names = [
    ...Array.from({length: profile.repeats}, (_, index) => `${benchmarkDaemonName}-resident-${index + 1}`),
    `${benchmarkDaemonName}-recovery-1`,
  ];
  const stale = [];
  for (const name of names) {
    const before = await shotium.daemon.status({name});
    const endpointExists = process.platform !== 'win32' && before.endpoint && fs.existsSync(before.endpoint);
    if (!before.running && !endpointExists) continue;
    stale.push({name, running: before.running, endpoint: before.endpoint});
    if (before.running) await shotium.daemon.stop({name});
    if (process.platform !== 'win32' && before.endpoint && fs.existsSync(before.endpoint)) {
      fs.rmSync(before.endpoint, {force: true});
    }
    const after = await shotium.daemon.status({name});
    if (after.running || (process.platform !== 'win32' && after.endpoint && fs.existsSync(after.endpoint))) {
      throw new Error(`owned Shotium endpoint survived cleanup: ${name}`);
    }
  }
  return stale;
}

async function runCell(group, attempt, configFile, profile) {
  const orchestrationStarted = performance.now();
  const before = await waitForSystemStable();
  if (!before.stable) {
    return {
      ok: true,
      status: 'noisy',
      engine: group.engine,
      scenario: group.scenario,
      repeat: group.repeat,
      concurrency: group.concurrency,
      samples: [],
      quality: {stable: false, phase: 'preflight-system', system: before},
      attempt,
      external_wall_time_ms: performance.now() - orchestrationStarted,
      process_cleanup: {force_killed: false, remaining: []},
      system_before: before,
      system_after: before,
    };
  }
  if (group.engine === 'shotium') {
    const staleEndpoints = await cleanupOwnedShotiumEndpoints(profile);
    if (staleEndpoints.length) {
      return {
        ok: true,
        status: 'noisy',
        engine: group.engine,
        scenario: group.scenario,
        repeat: group.repeat,
        concurrency: group.concurrency,
        samples: [],
        quality: {stable: false, phase: 'preflight-endpoint', stale_endpoints: staleEndpoints},
        attempt,
        external_wall_time_ms: performance.now() - orchestrationStarted,
        process_cleanup: {force_killed: false, remaining: []},
        system_before: before,
        system_after: before,
      };
    }
  }
  const started = performance.now();
  const dispatchedAtMs = Date.now();
  const subprocess = execaNode(tsxCli, [workerFile,
    '--config', configFile,
    '--engine', group.engine,
    '--scenario', group.scenario,
    '--repeat', String(group.repeat),
    '--attempt', String(attempt),
    '--concurrency', String(group.concurrency),
    '--iterations', String(group.iterations),
    '--scenario-timeout-ms', String(group.timeoutMs || 0),
    '--dispatched-at-ms', String(dispatchedAtMs),
  ], {
    cwd: APP_ROOT,
    timeout: group.scenario === 'soak' ? 720_000 : 240_000,
    killDescendants: true,
    reject: false,
    env: {...process.env, SHOT_BENCH_CELL: `${group.engine}:${group.scenario}:${group.repeat}:${attempt}`},
  });
  const monitor = new ProcessMonitor([subprocess.pid], 100);
  await monitor.start();
  const completed = await subprocess;
  const outerTelemetry = await monitor.stop();
  fs.mkdirSync(logsDirectory, {recursive: true});
  const logBase = `${group.engine}.${group.scenario}.c${group.concurrency}.r${group.repeat}.a${attempt}`;
  fs.writeFileSync(path.join(logsDirectory, `${logBase}.stdout.log`), completed.stdout || '');
  fs.writeFileSync(path.join(logsDirectory, `${logBase}.stderr.log`), completed.stderr || '');
  let result;
  try {
    result = parseWorkerOutput(completed.stdout);
  } catch (error) {
    result = {
      ok: false,
      status: completed.timedOut ? 'fail' : 'infra-error',
      engine: group.engine,
      scenario: group.scenario,
      repeat: group.repeat,
      concurrency: group.concurrency,
      error: `${error}; ${completed.stderr || ''}`.trim(),
    };
  }
  let remaining = await waitForIdentitiesToExit(outerTelemetry.identities, SETTLE.gracefulExitMs);
  let forceKilled = false;
  if (remaining.length) {
    forceKilled = true;
    remaining = await terminateOwnedProcesses(remaining);
  }
  const leftoverProfiles = fs.existsSync(tempProfileDirectory) ?
    fs.readdirSync(tempProfileDirectory).map((name) => path.join(tempProfileDirectory, name)) : [];
  const profileCleanupErrors = [];
  for (const profile of leftoverProfiles) {
    try {
      fs.rmSync(profile, {recursive: true, force: true});
    } catch (error) {
      profileCleanupErrors.push(`${profile}: ${error}`);
    }
  }
  const after = await waitForSystemStable();
  result.attempt = attempt;
  result.external_wall_time_ms = performance.now() - started;
  result.process_cleanup = {
    force_killed: forceKilled,
    remaining: remaining.map(({pid, started: birth, name}) => ({pid, started: birth, name})),
    leftover_profiles: leftoverProfiles.map((profile) => path.basename(profile)),
    profile_cleanup_errors: profileCleanupErrors,
  };
  result.monitor = {trusted: outerTelemetry.trusted, errors: outerTelemetry.errors};
  result.system_before = before;
  result.system_after = after;
  if (remaining.length) {
    result.ok = false;
    result.status = 'fail';
    result.error = `${result.error || ''}\nowned process tree did not exit`.trim();
  }
  if (!outerTelemetry.trusted) {
    result.ok = false;
    result.status = 'infra-error';
    result.error = `${result.error || ''}\nPID ownership monitor was unavailable`.trim();
  }
  if (profileCleanupErrors.length) {
    result.ok = false;
    result.status = 'fail';
    result.error = `${result.error || ''}\ntemporary browser profiles could not be removed`.trim();
  }
  if (!before.stable || !after.stable || result.status === 'noisy') result.status = result.ok ? 'noisy' : result.status;
  return result;
}

function summarizeScenarios(rows) {
  const groups = new Map();
  for (const row of rows.filter((entry) => !entry.excluded)) {
    const key = `${row.engine}|${row.scenario}|${row.concurrency}`;
    const bucket = groups.get(key) || [];
    bucket.push(row);
    groups.set(key, bucket);
  }
  return [...groups.values()].map((bucket) => {
    const first = bucket[0];
    const samples = bucket.flatMap((row) => row.samples || []);
    const shots = samples.filter((sample) => Number.isFinite(sample.ms) && !sample.failed && !sample.fault);
    const wall = bucket.map((row) => row.quality?.measurement_wall_time_ms ?? row.wall_time_ms)
        .filter(Number.isFinite);
    const orchestrationWall = bucket.map((row) => row.external_wall_time_ms).filter(Number.isFinite);
    const latencyValues = first.scenario === 'cold' ?
      bucket.map((row) => row.quality?.cold_start_to_png_ms).filter(Number.isFinite) :
      shots.map((shot) => shot.ms).filter(Number.isFinite);
    const succeeded = bucket.reduce((sum, row) =>
      sum + (Number(row.quality?.succeeded_soak_iterations) || 0), 0);
    const failed = bucket.reduce((sum, row) =>
      sum + (Number(row.quality?.failed_soak_iterations) || 0), 0);
    const status = bucket.some((row) => row.status === 'fail') ? 'fail' :
      bucket.some((row) => row.status === 'infra-error') ? 'infra-error' :
        bucket.some((row) => row.status === 'noisy') ? 'noisy' : 'pass';
    return {
      engine: first.engine,
      scenario: first.scenario,
      concurrency: first.concurrency,
      status,
      ranking_eligible: status === 'pass' && bucket.every((row) => row.ranking_eligible),
      runs: bucket.length,
      shots: shots.length,
      wall_time_ms: distribution(wall),
      orchestration_wall_time_ms: distribution(orchestrationWall),
      latency_ms: distribution(latencyValues),
      peak_rss_bytes: distribution(bucket.map((row) => row.peak_rss_bytes)),
      rss_slope_bytes_per_minute: distribution(
          bucket.map((row) => row.rss_slope_bytes_per_minute).filter(Number.isFinite)),
      succeeded_requests: succeeded || null,
      failed_requests: failed || null,
      failure_rate: succeeded + failed ? failed / (succeeded + failed) : null,
      throughput_per_second: shots.length && wall.reduce((sum, value) => sum + value, 0) ?
        (shots.length * 1000) / wall.reduce((sum, value) => sum + value, 0) : null,
    };
  });
}

async function main() {
  const profile = PROFILES[options.profile];
  if (!profile) throw new Error(`profile must be smoke or full, got ${options.profile}`);
  if (fs.existsSync(permanentDirectory) || fs.existsSync(artifactDirectory)) {
    throw new Error(`platform output already exists; choose a new --output directory: ${outputDirectory}`);
  }
  fs.mkdirSync(permanentDirectory, {recursive: true});
  fs.mkdirSync(telemetryDirectory, {recursive: true});
  fs.mkdirSync(tempProfileDirectory, {recursive: true});
  outputInitialized = true;
  fs.writeFileSync(samplesFile, '');
  await verifyProcessMonitoring();

  const shotiumVersion = booleanArg(options.skipInstall, false) ? options.shotiumVersion :
    await ensureShotium(options.shotiumVersion);
  resolvedShotiumVersion = shotiumVersion;
  let install;
  try {
    install = await verifyConsumerInstall(
        shotiumVersion, path.join(artifactDirectory, 'samples', 'shotium.api-smoke.png'));
  } catch (error) {
    throw new ProductError(
        `installed Shotium failed its consumer API smoke test: ${String(error?.stack || error)}`, error);
  }
  const fixtureServer = await startFixtureServer();
  const cases = loadCases(fixtureServer.baseUrl, profile.caseLimit);
  const config = {
    profile,
    cases,
    baseUrl: fixtureServer.baseUrl,
    fixtureRoot: FIXTURE_ROOT,
    outputDirectory,
    artifactDirectory,
    telemetryDirectory,
    tempProfileDirectory,
    daemonName: benchmarkDaemonName,
  };
  const configFile = path.join(artifactDirectory, 'run-config.json');
  writeJson(configFile, config);

  const probes = [];
  for (const engine of ENGINE_IDS) probes.push(await probeEngine(engine));
  const runnable = probes.filter((probe) => probe.status === 'pass').map((probe) => probe.engine);
  const rows = [];
  const quality = [];
  const failures = [];
  completedRows = rows;
  completedQuality = quality;
  completedFailures = failures;
  for (const probe of probes.filter((entry) => ['fail', 'infra-error'].includes(entry.status))) {
    failures.push({
      engine: probe.engine,
      phase: 'availability-probe',
      error: String(probe.reason || `${probe.engine} availability probe failed`),
    });
  }
  const executionOrder = [];
  try {
    for (const group of buildGroups(runnable, profile)) {
      for (const engine of group.engines) {
        const cell = {...group, engine};
        for (let attempt = 1; attempt <= 2; attempt += 1) {
          executionOrder.push({
            engine, scenario: group.scenario, repeat: group.repeat, attempt,
            concurrency: group.concurrency, iterations: group.iterations,
          });
          const result = await runCell(cell, attempt, configFile, profile);
          result.ranking_eligible = result.status === 'pass' &&
            !['reuse-page', 'faults'].includes(group.scenario);
          if (result.status === 'noisy' && attempt === 1) result.excluded = true;
          appendJsonLine(result);
          rows.push(result);
          quality.push({
            engine,
            scenario: group.scenario,
            repeat: group.repeat,
            attempt,
            concurrency: group.concurrency,
            status: result.status,
            settle: result.quality || null,
            cleanup: result.process_cleanup,
          });
          if (!result.ok) {
            failures.push({
              engine,
              scenario: group.scenario,
              repeat: group.repeat,
              attempt,
              error: String(result.error || 'benchmark cell failed without an error message'),
            });
            break;
          }
          if (result.status !== 'noisy') {
            break;
          }
        }
      }
    }
  } finally {
    await fixtureServer.close();
  }

  const scenarios = summarizeScenarios(rows);
  const engineStates = probes.map((probe) => {
    const relevant = scenarios.filter((scenario) => scenario.engine === probe.engine);
    let status = probe.status;
    if (status === 'pass' && relevant.some((scenario) => scenario.status === 'fail')) status = 'fail';
    else if (status === 'pass' && relevant.some((scenario) => scenario.status === 'infra-error')) status = 'infra-error';
    else if (status === 'pass' && relevant.some((scenario) => scenario.status === 'noisy')) status = 'noisy';
    return {...probe, status};
  });
  const shotium = engineStates.find((engine) => engine.engine === 'shotium');
  const status = !shotium || shotium.status === 'infra-error' ? 'infra-error' :
    shotium.status === 'fail' || engineStates.some((engine) => engine.status === 'fail') ? 'fail' :
      engineStates.some((engine) => engine.status === 'infra-error') ? 'infra-error' :
        engineStates.some((engine) => engine.status === 'noisy') ? 'noisy' : 'pass';
  const [cpu, osInfo, memory, versions, sourceRevision] = await Promise.all([
    si.cpu(), si.osInfo(), si.mem(), packageVersions(), gitRevision(),
  ]);
  const summary = validatePlatformResult({
    schema_version: 2,
    generated_utc: new Date().toISOString(),
    platform,
    status,
    shotium_version: shotiumVersion,
    profile: options.profile,
    seed: String(options.seed),
    source_revision: sourceRevision,
    install,
    host: {
      runner: process.env.RUNNER_NAME || os.hostname(),
      os: osInfo,
      cpu,
      memory_bytes: memory.total,
      logical_processors: os.cpus().length,
      node: process.version,
      npm: process.env.npm_config_user_agent || null,
    },
    packages: versions,
    measurement_contract: {
      viewport: {width: 1280, height: 720},
      scale: 1,
      output: 'png',
      wait_until: 'load',
      default_page_policy: 'new-page',
      default_cache_policy: 'disabled-or-no-store',
      reuse_page_scenario_is_separate: true,
    },
    engines: engineStates,
    scenarios,
    execution_order: executionOrder,
    raw_samples: rows.length,
    failures: failures.length,
  });
  writeJson(path.join(permanentDirectory, 'summary.json'), summary);
  writeJson(path.join(permanentDirectory, 'quality.json'), quality);
  writeJson(path.join(permanentDirectory, 'failures.json'), failures);
  if (['fail', 'infra-error'].includes(status)) process.exitCode = 1;
}

main().catch(async (error) => {
  if (!outputInitialized) {
    process.stderr.write(`${String(error?.stack || error)}\n`);
    process.exitCode = 1;
    return;
  }
  fs.mkdirSync(permanentDirectory, {recursive: true});
  const fatalStatus = isProductError(error) ? 'fail' : 'infra-error';
  const failure = {at: new Date().toISOString(), status: fatalStatus, error: String(error?.stack || error)};
  const failures = [...completedFailures, failure];
  writeJson(path.join(permanentDirectory, 'failures.json'), failures);
  writeJson(path.join(permanentDirectory, 'quality.json'), completedQuality);
  if (!fs.existsSync(samplesFile)) fs.writeFileSync(samplesFile, '');
  writeJson(path.join(permanentDirectory, 'summary.json'), {
    schema_version: 2,
    generated_utc: new Date().toISOString(),
    platform,
    status: fatalStatus,
    shotium_version: String(resolvedShotiumVersion || options.shotiumVersion),
    profile: options.profile,
    seed: String(options.seed),
    source_revision: process.env.GITHUB_SHA || null,
    install: null,
    host: null,
    packages: {},
    engines: [],
    scenarios: [],
    execution_order: [],
    raw_samples: completedRows.length,
    failures: failures.length,
    error: failure.error,
  });
  process.stderr.write(`${failure.error}\n`);
  process.exitCode = 1;
});
