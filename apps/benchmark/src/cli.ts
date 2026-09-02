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
  SHARD_IDS,
  currentPlatformId,
} from './constants.ts';
import {packageVersions, probeEngine} from './engines.ts';
import {benchmarkDaemonName} from './daemon-name.ts';
import {ProductError, isProductError} from './errors.ts';
import {startFixtureServer, loadCases} from './fixtures.ts';
import {buildGroups} from './groups.ts';
import {ensureShotium} from './install-target.ts';
import {
  ProcessMonitor,
  calibrateHostLoad,
  sampleHostLoad,
  terminateOwnedProcesses,
  verifyProcessMonitoring,
  waitForIdentitiesToExit,
  waitForSystemStable,
} from './process-tree.ts';
import {distribution} from './statistics.ts';
import {validatePlatformResult} from './schema.ts';

const options = parseArgs(process.argv.slice(2), {
  shotiumVersion: 'latest',
  profile: 'full',
  output: path.join(APP_ROOT, 'out', `${Date.now()}-${process.pid}`),
  seed: process.env.GITHUB_SHA || process.env.GITHUB_RUN_ID || 'local',
  shard: 'all',
});
recoverNpmRunValues(options, ['shotiumVersion', 'profile', 'output', 'seed', 'shard', 'skipInstall']);
const require = createRequire(import.meta.url);
const tsxCli = require.resolve('tsx/cli');

const platform = currentPlatformId();
const shard = String(options.shard);
if (!SHARD_IDS.includes(shard)) {
  throw new Error(`shard must be ${SHARD_IDS.join(', ')}, got ${shard}`);
}
const outputDirectory = path.resolve(options.output);
const shardPath = shard === 'all' ? [] : [shard];
const permanentDirectory = path.join(outputDirectory, 'permanent', platform, ...shardPath);
const artifactDirectory = path.join(outputDirectory, 'artifact', platform, ...shardPath);
const telemetryDirectory = path.join(artifactDirectory, 'telemetry');
const logsDirectory = path.join(artifactDirectory, 'logs');
const tempProfileDirectory = path.join(artifactDirectory, 'temp-profiles');
const samplesFile = path.join(permanentDirectory, 'samples.jsonl');
const workerFile = path.join(APP_ROOT, 'src', 'worker.ts');
const benchmarkRunId = String(process.env.GITHUB_RUN_ID || process.pid);
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
  fs.appendFileSync(samplesFile, `${JSON.stringify({...value, shard})}\n`);
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

async function cleanupOwnedShotiumEndpoints(profile) {
  const shotium = await import('@shotkit/shotium');
  const names = [
    ...Array.from({length: profile.repeats}, (_, index) => benchmarkDaemonName({
      runId: benchmarkRunId, platform, scenario: 'resident', repeat: index + 1,
    })),
    benchmarkDaemonName({runId: benchmarkRunId, platform, scenario: 'recovery'}),
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

function progress(message) {
  process.stdout.write(`[${new Date().toISOString()}] ${message}\n`);
}

function cellLabel(group, attempt) {
  return `${group.engine} ${group.scenario} c${group.concurrency} r${group.repeat} a${attempt}`;
}

async function runCell(group, attempt, configFile, profile, stability) {
  const orchestrationStarted = performance.now();
  // A cell rejected because the host was busy used to be retried six seconds
  // later, against the same busy window, so those rejections came in pairs all
  // through the log: `faults` leaves four browsers dying and `soak` starts into
  // the middle of that. waitForSystemStable returns the moment the host is
  // quiet, so a longer deadline costs nothing when it already is - it is only
  // spent on the retries that exist because it was not.
  const before = await waitForSystemStable({
    cpuLimit: stability.cpu_limit,
    timeoutMs: attempt > 1 ? SETTLE.timeoutMs : SETTLE.cooldownTimeoutMs,
  });
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
  // The outer monitor exists to prove ownership of the worker tree and to clean
  // it up, not to time anything; the worker samples its own RSS. Poll it slowly
  // so the expensive process-table query is not competing with the measurement.
  const monitor = new ProcessMonitor([subprocess.pid], 1000);
  await monitor.start();
  // A cell that stops producing output is the shape every Windows job took. Say
  // so while it is happening instead of leaving a silent gap in the log.
  const heartbeat = setInterval(() => {
    progress(`  ${cellLabel(group, attempt)}: still running after ` +
      `${Math.round((performance.now() - started) / 1000)}s`);
  }, SETTLE.heartbeatMs);
  let completed;
  let outerTelemetry;
  try {
    completed = await subprocess;
  } finally {
    clearInterval(heartbeat);
    outerTelemetry = await monitor.stop();
  }
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
  // One sample for the record only. The next cell's preflight is the real
  // post-condition; waiting here as well doubled the fixed cost of every cell.
  const after = {stable: true, samples: [await sampleHostLoad()]};
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
  if (!before.stable || result.status === 'noisy') result.status = result.ok ? 'noisy' : result.status;
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
      shard,
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
  // Print before the first host probe. Windows used to block inside the process
  // table query with nothing on stdout, which is indistinguishable from a slow
  // shard until the job timeout kills it 90 minutes later.
  progress(`${platform}/${shard}: profile ${options.profile}, budget ` +
    `${Math.round(profile.shardBudgetMs / 60_000)} min, node ${process.version}`);
  await verifyProcessMonitoring();

  const shotiumVersion = booleanArg(options.skipInstall, false) ? options.shotiumVersion :
    await ensureShotium(options.shotiumVersion);
  resolvedShotiumVersion = shotiumVersion;
  let install;
  try {
    // Load the consumer probe only after npm has installed the requested package.
    // tsx resolves imports while loading a module, so importing this probe at
    // process startup can retain a missing-package resolution from before the
    // no-save install completes.
    const {verifyConsumerInstall} = await import('./verify-install.ts');
    install = await verifyConsumerInstall(
        shotiumVersion, path.join(artifactDirectory, 'samples', 'shotium.api-smoke.png'));
  } catch (error) {
    throw new ProductError(
        `installed Shotium failed its consumer API smoke test: ${String(error?.stack || error)}`, error);
  }
  const fixtureServer = await startFixtureServer();
  const cases = loadCases(fixtureServer.baseUrl, profile.caseLimit);
  // Calibrate here rather than at job start. The three browser installs, the npm
  // install and the consumer smoke test all run first, and five seconds sampled
  // in their wake measures them rather than the host: darwin-arm64/resident read
  // a 97.7% *median* idle floor that way and produced a 110% gate, while other
  // shards on the same runner image read 40% and 67%. A gate that swings from 70
  // to 110 across one run is not measuring the host.
  const stability = await calibrateHostLoad();
  progress(`host idle CPU p50 ${stability.idle_cpu_p50}% p95 ${stability.idle_cpu_p95}% ` +
    `-> stability gate ${stability.cpu_limit}% (${stability.samples.length} samples, ` +
    `${stability.sampler_monitors} process monitors at ` +
    `${stability.sampler_mean_period_ms ?? '?'}ms mean period)`);
  if (stability.cpu_limit_exceeds_ceiling) {
    progress(`warning: the idle floor pushed the CPU gate past ${SETTLE.cpuLimitMax}%; ` +
      'this runner is too loud for the host CPU check to mean much');
  }
  const config = {
    shard,
    profile,
    cases,
    baseUrl: fixtureServer.baseUrl,
    fixtureRoot: FIXTURE_ROOT,
    outputDirectory,
    artifactDirectory,
    telemetryDirectory,
    tempProfileDirectory,
    daemonIdentity: {runId: benchmarkRunId, platform},
    stability: {
      cpu_limit: stability.cpu_limit,
      idle_cpu_p50: stability.idle_cpu_p50,
      idle_cpu_p95: stability.idle_cpu_p95,
    },
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
      shard,
      engine: probe.engine,
      phase: 'availability-probe',
      error: String(probe.reason || `${probe.engine} availability probe failed`),
    });
  }
  const executionOrder = [];
  const groups = buildGroups(runnable, profile, {shard, seed: String(options.seed)});
  const totalCells = groups.reduce((sum, group) => sum + group.engines.length, 0);
  let cellIndex = 0;
  progress(`${platform}/${shard}: ${totalCells} cells across ${runnable.length} engines ` +
    `(${runnable.join(', ') || 'none'})`);
  const unavailable = probes.filter((probe) => probe.status !== 'pass');
  for (const probe of unavailable) {
    // An engine that cannot run here used to vanish from the log entirely, so a
    // three-engine linux-arm64 shard looked the same as a five-engine one.
    progress(`  ${probe.engine}: ${probe.status} - ${probe.reason || 'no reason recorded'}`);
  }
  const budgetDeadline = Date.now() + profile.shardBudgetMs;
  let budgetExhausted = false;
  try {
    for (const group of groups) {
      for (const engine of group.engines) {
        if (Date.now() >= budgetDeadline) {
          if (!budgetExhausted) {
            budgetExhausted = true;
            // Stop scheduling rather than letting the GitHub job timeout kill the
            // process: a killed job loses every upload, including the evidence
            // for the cells that did complete.
            progress(`shard budget of ${Math.round(profile.shardBudgetMs / 60_000)} min is spent ` +
              `after ${cellIndex}/${totalCells} cells; writing what completed`);
            failures.push({
              shard,
              phase: 'shard-budget',
              error: `shard budget of ${Math.round(profile.shardBudgetMs / 60_000)} min expired ` +
                `after ${cellIndex}/${totalCells} cells`,
            });
          }
          continue;
        }
        const cell = {...group, engine};
        cellIndex += 1;
        for (let attempt = 1; attempt <= 2; attempt += 1) {
          executionOrder.push({
            shard,
            engine, scenario: group.scenario, repeat: group.repeat, attempt,
            concurrency: group.concurrency, iterations: group.iterations,
          });
          const result = await runCell(cell, attempt, configFile, profile, stability);
          progress(`[${cellIndex}/${totalCells}] ${cellLabel(cell, attempt)}: ${result.status} ` +
            `in ${Math.round(result.external_wall_time_ms / 1000)}s` +
            (result.quality?.phase ? ` (${result.quality.phase})` : '') +
            (result.error ? ` - ${String(result.error).split('\n')[0].slice(0, 160)}` : ''));
          result.ranking_eligible = result.status === 'pass' &&
            !['reuse-page', 'faults'].includes(group.scenario);
          if (result.status === 'noisy' && attempt === 1) result.excluded = true;
          appendJsonLine(result);
          rows.push(result);
          quality.push({
            shard,
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
              shard,
              engine,
              scenario: group.scenario,
              repeat: group.repeat,
              attempt,
              concurrency: group.concurrency,
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
  // What the shard measured and what should fail the CI job are two different
  // questions. A baseline engine that renders the same static page differently
  // twice, blanks a screenshot 200 iterations into a soak, or cannot warm up its
  // resident mode is a result about that engine - recording it is the point of
  // this benchmark, and it stays in summary.json and failures.json either way.
  // The run itself is only untrustworthy when our own engine fails, when the
  // harness or host breaks, or when the shard ran out of time to finish.
  const probeStatus = new Map(probes.map((probe) => [probe.engine, probe.status]));
  const blockingReasons = [];
  if (!shotium) blockingReasons.push('shotium was never probed');
  else if (shotium.status === 'fail') blockingReasons.push('shotium failed its own scenarios');
  else if (shotium.status === 'infra-error') blockingReasons.push('shotium hit an infrastructure error');
  for (const engine of engineStates) {
    if (engine.engine === 'shotium' || engine.status !== 'infra-error') continue;
    // A baseline engine that never became available - no arm64 build, a browser
    // download that failed - is recorded in summary.json and printed above; it
    // does not mean this run measured anything wrongly. An infrastructure error
    // raised after the engine probed clean is ours: the process monitor lost the
    // tree, a query timed out, evidence could not be written.
    if (probeStatus.get(engine.engine) !== 'pass') continue;
    blockingReasons.push(`${engine.engine} hit an infrastructure error while running`);
  }
  if (budgetExhausted) blockingReasons.push('the shard budget expired before every cell ran');
  const baselineResultFailures = engineStates
      .filter((engine) => engine.engine !== 'shotium' && ['fail', 'infra-error'].includes(engine.status))
      .map((engine) => `${engine.engine}=${engine.status}`);
  if (baselineResultFailures.length) {
    progress(`recorded baseline engine outcomes (not fatal): ${baselineResultFailures.join(', ')}`);
  }
  const [cpu, osInfo, memory, versions, sourceRevision] = await Promise.all([
    si.cpu(), si.osInfo(), si.mem(), packageVersions(), gitRevision(),
  ]);
  const summary = validatePlatformResult({
    schema_version: 2,
    generated_utc: new Date().toISOString(),
    platform,
    shard,
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
      idle_cpu_baseline: {
        p50_percent: stability.idle_cpu_p50,
        p95_percent: stability.idle_cpu_p95,
        cpu_limit_percent: stability.cpu_limit,
        samples: stability.samples.length,
        sampler_monitors: stability.sampler_monitors,
        sampler_mean_period_ms: stability.sampler_mean_period_ms,
        cpu_limit_exceeds_ceiling: stability.cpu_limit_exceeds_ceiling,
      },
    },
    packages: versions,
    measurement_contract: {
      viewport: {width: 1280, height: 720},
      scale: 1,
      output: 'png',
      wait_until: 'load',
      visual_readiness: 'fonts-ready-and-two-animation-frames-or-native-paint-clean',
      warmup_policy: 'three-fixed-warmups; latency-and-rss-variation-recorded-not-gated',
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
  if (blockingReasons.length) {
    progress(`shard failed: ${blockingReasons.join('; ')}`);
    process.exitCode = 1;
  }
}

main().catch(async (error) => {
  if (!outputInitialized) {
    process.stderr.write(`${String(error?.stack || error)}\n`);
    process.exitCode = 1;
    return;
  }
  fs.mkdirSync(permanentDirectory, {recursive: true});
  const fatalStatus = isProductError(error) ? 'fail' : 'infra-error';
  const failure = {
    at: new Date().toISOString(), shard, status: fatalStatus, error: String(error?.stack || error),
  };
  const failures = [...completedFailures, failure];
  writeJson(path.join(permanentDirectory, 'failures.json'), failures);
  writeJson(path.join(permanentDirectory, 'quality.json'), completedQuality);
  if (!fs.existsSync(samplesFile)) fs.writeFileSync(samplesFile, '');
  writeJson(path.join(permanentDirectory, 'summary.json'), {
    schema_version: 2,
    generated_utc: new Date().toISOString(),
    platform,
    shard,
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
