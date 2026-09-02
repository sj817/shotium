import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execaNode} from 'execa';
import {Bench} from 'tinybench';
import {parseArgs} from './args.ts';
import {APP_ROOT, SETTLE} from './constants.ts';
import {createEngine} from './engines.ts';
import {benchmarkDaemonName} from './daemon-name.ts';
import {InfrastructureError, isInfrastructureError} from './errors.ts';
import {comparePng, inspectPng, isDeterministicComparison, saveBaseline} from './image.ts';
import {
  ProcessMonitor,
  processSnapshot,
  terminateOwnedProcesses,
  waitForSystemStable,
  waitForProcessTree,
  waitForIdentitiesToExit,
} from './process-tree.ts';
import {startResident} from './resident.ts';
import {readinessDiagnostics, settleEngine} from './settle.ts';
import {distribution, round} from './statistics.ts';

const options = parseArgs(process.argv.slice(2));
const require = createRequire(import.meta.url);
const tsxCli = require.resolve('tsx/cli');
const config = JSON.parse(fs.readFileSync(options.config, 'utf8'));
const scenario = options.scenario;
const engineName = options.engine;
const repeat = Number(options.repeat) || 1;
const attempt = Number(options.attempt) || 1;
const concurrency = Number(options.concurrency) || 1;
const iterations = Number(options.iterations) || 1;
const scenarioTimeoutMs = Number(options.scenarioTimeoutMs) || config.profile.soakTimeoutMs;
const dispatchedAtMs = Number(options.dispatchedAtMs) || Date.now();
const hostCpuLimit = Number(config.stability?.cpu_limit) || SETTLE.cpuLimit;
const caseDefinitions = config.cases;
const sampleDirectory = path.join(config.artifactDirectory, 'samples');
const baselineDirectory = path.join(config.artifactDirectory, 'baselines');
const clientFile = path.join(APP_ROOT, 'src', 'client-once.ts');
const pendingEvidence = [];
const evidenceMemoryTimeline = [];
let retainedEvidenceBytes = 0;
fs.mkdirSync(sampleDirectory, {recursive: true});

function daemonName(nameScenario = scenario, nameRepeat = repeat, variant = 0) {
  return benchmarkDaemonName({
    runId: String(config.daemonIdentity.runId),
    platform: String(config.daemonIdentity.platform),
    scenario: nameScenario,
    repeat: nameRepeat,
    variant,
  });
}

function errorText(error) {
  // Follow the cause chain. InfrastructureError carries the error it wrapped,
  // and dropping it meant a CI artifact reported "PID ownership could not be
  // established" with no way to tell a ten-second timeout from a process that
  // had already exited -- the only two things it can mean.
  const seen = new Set();
  const parts = [];
  let current = error;
  while (current && !seen.has(current)) {
    seen.add(current);
    parts.push(String(current?.stack || current));
    current = current?.cause;
  }
  return parts.join('\ncaused by: ');
}

async function closeWithin(operation, label = 'engine close') {
  let timer;
  try {
    await Promise.race([
      operation(),
      new Promise((_, reject) => {
        timer = setTimeout(() => reject(new Error(`${label} exceeded ${SETTLE.gracefulExitMs}ms`)),
            SETTLE.gracefulExitMs);
      }),
    ]);
  } catch (error) {
    if (String(error).includes(`${label} exceeded`)) {
      try {
        const snapshot = await processSnapshot([process.pid]);
        const children = snapshot.processes.filter((entry) => entry.pid !== process.pid);
        const remaining = await terminateOwnedProcesses(children);
        if (remaining.length) {
        throw new InfrastructureError(`${label} timed out and ${remaining.length} owned processes survived`);
        }
      } catch (cleanupError) {
        throw new InfrastructureError(
            `${errorText(error)}\nforced close cleanup failed: ${errorText(cleanupError)}`, cleanupError);
      }
    }
    throw error;
  } finally {
    if (timer) clearTimeout(timer);
  }
}

async function closeAfterForcedExit(operation, label) {
  try {
    await closeWithin(operation, label);
  } catch (error) {
    if (isInfrastructureError(error) || String(error).includes(`${label} exceeded`)) {
      throw new InfrastructureError(`${label} could not finish after the intentional process exit`, error);
    }
    // Browser control channels normally reject close() after their exact
    // process has already been terminated and verified absent.
  }
}

async function waitForRequestStarted(token, timeoutMs) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      const response = await fetch(`${config.baseUrl}/request-status?token=${encodeURIComponent(token)}`);
      if (response.ok && (await response.json()).started === true) return;
    } catch (error) {
      throw new InfrastructureError('fixture request-status endpoint failed', error);
    }
    await new Promise((resolve) => setTimeout(resolve, 25));
  }
  throw new Error(`fixture server did not observe request ${token}`);
}

// Establishes ownership of a client up front instead of hoping a periodic
// sample lands while it is alive. The two resilience clients are killed on
// purpose moments after they start, and a process-table query costs about
// 700 ms on Windows, so the sampling loop can easily never see one: the
// telemetry then comes back untrusted with an empty error list, and the shard
// dies reporting `PID monitor was unavailable:` with nothing after the colon.
// Waiting for the tree once, before the request goes out, is the difference
// between the harness being unable to watch and it not having got round to it.
async function startOwnershipMonitor(monitor, subprocess, label) {
  try {
    await monitor.start({requireRoots: true, timeoutMs: 10_000});
  } catch (error) {
    const alreadyExited = subprocess.exitCode !== null || subprocess.signalCode !== null;
    if (!alreadyExited) subprocess.kill('SIGTERM');
    await subprocess.catch(() => {});
    if (alreadyExited) {
      throw new Error(`${label} exited before its request started ` +
        `(code ${subprocess.exitCode}, signal ${subprocess.signalCode}): ${errorText(error)}`);
    }
    throw new InfrastructureError(`${label} PID ownership could not be established`, error);
  }
}

async function finishOwnedChild(subprocess, label) {
  const monitor = new ProcessMonitor([subprocess.pid], 50);
  let releasedAtMs;
  let monitorStarted = false;
  try {
    await monitor.start({requireRoots: true, timeoutMs: 10_000});
    monitorStarted = true;
    if (!subprocess.stdin || subprocess.stdin.destroyed || subprocess.stdin.writableEnded) {
      throw new InfrastructureError(`${label} start gate was unavailable`);
    }
    releasedAtMs = Date.now();
    subprocess.stdin.end('go\n');
  } catch (error) {
    if (monitorStarted) {
      await monitor.stop().catch(() => {});
      monitorStarted = false;
    }
    if (subprocess.stdin && !subprocess.stdin.destroyed) subprocess.stdin.destroy();
    const alreadyExited = subprocess.exitCode !== null || subprocess.signalCode !== null;
    if (!alreadyExited) subprocess.kill('SIGTERM');
    await subprocess.catch(() => {});
    if (alreadyExited) {
      // The client died at the start gate, before it was told to go. Not being
      // able to observe a process that is no longer there is the engine failing
      // to start, not the harness failing to watch -- and only the second of
      // those means this run measured anything wrongly.
      throw new Error(`${label} client exited before the start gate ` +
        `(code ${subprocess.exitCode}, signal ${subprocess.signalCode}): ${errorText(error)}`);
    }
    throw new InfrastructureError(`${label} PID ownership could not be established before start`, error);
  }
  let completed;
  let telemetry;
  try {
    completed = await subprocess;
  } finally {
    telemetry = await monitor.stop();
    monitorStarted = false;
  }
  if (!telemetry.trusted) {
    throw new InfrastructureError(`${label} PID ownership monitor was unavailable: ` +
      `${telemetry.errors.join('; ') || 'no sample observed the client PID'}`);
  }
  let remaining = await waitForIdentitiesToExit(telemetry.identities, SETTLE.forcedExitMs);
  if (remaining.length) remaining = await terminateOwnedProcesses(remaining);
  if (remaining.length) {
    throw new InfrastructureError(`${label} left an owned process tree running after cleanup`);
  }
  return {completed, releasedAtMs};
}

async function residentClient(endpoint, definition, label, timeoutMs = 120_000, evidenceFile = null) {
  const encodedEndpoint = Buffer.from(JSON.stringify(endpoint)).toString('base64url');
  const childArguments = [clientFile,
    '--start-gated', 'true',
    '--engine', engineName,
    '--url', definition.url,
    '--endpoint', encodedEndpoint,
    '--workers', String(concurrency),
    '--timeout-ms', String(Math.min(30_000, timeoutMs)),
  ];
  if (evidenceFile) childArguments.push('--evidence-file', evidenceFile);
  const subprocess = execaNode(tsxCli, childArguments,
      {stdin: 'pipe', timeout: timeoutMs, killDescendants: true, reject: false});
  const {completed, releasedAtMs} = await finishOwnedChild(subprocess, label);
  let payload;
  try {
    payload = completed.stdout.trim() ?
      JSON.parse(completed.stdout.trim().split(/\r?\n/).at(-1)) : null;
  } catch (error) {
    throw new InfrastructureError(`${label} returned malformed harness JSON`, error);
  }
  if (!payload?.ok || completed.exitCode !== 0) {
    throw new Error(payload?.error || completed.stderr || `${label} failed`);
  }
  return {
    payload,
    to_png_ms: Number.isFinite(payload.shot_completed_epoch_ms) ?
      payload.shot_completed_epoch_ms - releasedAtMs :
      payload.timings.connect_or_launch_ms + payload.timings.shot_ms,
  };
}

function clientEvidence(definition, evidenceFile, image) {
  const bytes = fs.readFileSync(evidenceFile);
  inspectPng(bytes, {width: definition.width, height: definition.height});
  const baselineFile = saveBaseline(baselineDirectory, engineName, definition.name, bytes);
  const comparison = comparePng(fs.readFileSync(baselineFile), bytes);
  return {
    image: {
      ...image,
      deterministic_against_first: isDeterministicComparison(comparison),
      ...comparison,
    },
    sample_file: path.relative(config.outputDirectory, evidenceFile).replaceAll('\\', '/'),
  };
}

async function capture(engine, definition, samples, label = definition.name, timeoutOverrideMs = null) {
  const started = performance.now();
  const result = await engine.shot(definition.url, {
    timeoutMs: timeoutOverrideMs || definition.timeout_ms || 30_000,
  });
  const elapsed = performance.now() - started;
  const sampleId = `${scenario}.c${concurrency}.r${repeat}.a${attempt}.s${String(samples.length + 1).padStart(5, '0')}`;
  const record = {
    case: definition.name,
    label,
    ms: round(elapsed),
    shot_completed_epoch_ms: Date.now(),
    engine_stats: result.stats,
  };
  samples.push(record);
  const sampleFile = path.join(sampleDirectory,
      `${engineName}.${definition.name}.${sampleId}.png`);
  const evidenceBytes = Buffer.isBuffer(result.image) ? result.image : Buffer.from(result.image);
  retainedEvidenceBytes += evidenceBytes.length;
  evidenceMemoryTimeline.push({at_epoch_ms: Date.now(), retained_bytes: retainedEvidenceBytes});
  pendingEvidence.push({
    record,
    definition,
    sampleId,
    sampleFile,
    image: evidenceBytes,
  });
  return record;
}

async function flushEvidence() {
  const errors: any[] = [];
  for (const evidence of pendingEvidence.splice(0)) {
    try {
      fs.writeFileSync(evidence.sampleFile, evidence.image);
      const evidenceBytes = fs.readFileSync(evidence.sampleFile);
      const image = inspectPng(evidenceBytes, {
        width: evidence.definition.width,
        height: evidence.definition.height,
      });
      const baselineFile = saveBaseline(
          baselineDirectory, engineName, evidence.definition.name, evidenceBytes);
      const comparison = comparePng(fs.readFileSync(baselineFile), evidenceBytes);
      evidence.record.image = {
        ...image,
        deterministic_against_first: isDeterministicComparison(comparison),
        ...comparison,
      };
      evidence.record.sample_file = path.relative(
          config.outputDirectory, evidence.sampleFile).replaceAll('\\', '/');
    } catch (error) {
      evidence.record.evidence_error = errorText(error);
      errors.push({error, text: `${evidence.definition.name}/${evidence.sampleId}: ${errorText(error)}`});
    }
    retainedEvidenceBytes -= evidence.image.length;
  }
  if (errors.length) {
    const message = errors.map((entry) => entry.text).join('\n');
    if (errors.some((entry) => isInfrastructureError(entry.error))) {
      throw new InfrastructureError(message, errors.find((entry) => isInfrastructureError(entry.error)).error);
    }
    throw new Error(message);
  }
}

async function mapConcurrent(items, limit, operation) {
  let cursor = 0;
  const workers = Array.from({length: Math.min(limit, items.length)}, async () => {
    while (cursor < items.length) {
      const index = cursor++;
      await operation(items[index], index);
    }
  });
  await Promise.all(workers);
}

function rssSlopePerMinute(timeline, field) {
  if (timeline.length < 3) return null;
  const origin = Date.parse(timeline[0].at);
  const points = timeline.map((sample) => ({
    x: Date.parse(sample.at) - origin,
    y: Number(sample[field]),
  })).filter((point) => Number.isFinite(point.x) && Number.isFinite(point.y));
  if (points.length < 3 || points.at(-1).x - points[0].x < 1000) return null;
  const meanX = points.reduce((sum, point) => sum + point.x, 0) / points.length;
  const meanY = points.reduce((sum, point) => sum + point.y, 0) / points.length;
  const denominator = points.reduce((sum, point) => sum + ((point.x - meanX) ** 2), 0);
  if (!denominator) return null;
  const perMillisecond = points.reduce((sum, point) =>
    sum + ((point.x - meanX) * (point.y - meanY)), 0) / denominator;
  return round(perMillisecond * 60_000);
}

async function runResident(samples) {
  const hostEvidenceFile = path.join(sampleDirectory,
      `${engineName}.resident-host-prewarm.r${repeat}.a${attempt}.png`);
  const host = await startResident(engineName, caseDefinitions[0].url, {
    workers: concurrency,
    daemonName: daemonName('resident'),
    evidenceFile: hostEvidenceFile,
  });
  try {
    const deadline = Date.now() + SETTLE.timeoutMs;
    const warmupLatency = [];
    const warmupRss = [];
    for (let warmup = 1; warmup <= SETTLE.minimumWarmups; warmup += 1) {
      const remainingMs = deadline - Date.now();
      if (remainingMs <= 0) break;
      const warmupEvidenceFile = path.join(sampleDirectory,
          `${engineName}.resident-warmup.r${repeat}.a${attempt}.s${String(warmup).padStart(2, '0')}.png`);
      const warm = await residentClient(
          host.endpoint, caseDefinitions[0], `resident warmup ${warmup}`, remainingMs,
          warmupEvidenceFile);
      warmupLatency.push(round(warm.to_png_ms));
      warmupRss.push((await processSnapshot([process.pid])).rss_bytes);
    }
    const settled = warmupLatency.length === SETTLE.minimumWarmups ? await waitForSystemStable({
      timeoutMs: Math.min(SETTLE.cooldownTimeoutMs, Math.max(0, deadline - Date.now())),
      cpuLimit: hostCpuLimit,
      memoryDriftLimit: Number.POSITIVE_INFINITY,
    }) : {stable: false, cpu_limit: hostCpuLimit, samples: []};
    const residentSettle = {
      stable: Boolean(settled?.stable),
      warmups: warmupLatency.length,
      latency_ms: warmupLatency,
      rss_bytes: warmupRss,
      ...readinessDiagnostics(warmupLatency, warmupRss),
      readiness_gate: 'fixed-warmups-plus-host-cpu',
      system: settled,
    };
    if (!residentSettle.stable) return residentSettle;
    const measurementStarted = Date.now();
    for (let index = 0; index < iterations; index += 1) {
      const definition = caseDefinitions[index % caseDefinitions.length];
      const evidenceFile = path.join(sampleDirectory,
          `${engineName}.resident.c${concurrency}.r${repeat}.a${attempt}.s${String(index + 1).padStart(5, '0')}.png`);
      const {payload, to_png_ms: wall} = await residentClient(
          host.endpoint, definition, `resident client ${index + 1}`, 120_000, evidenceFile);
      samples.push({
        case: definition.name,
        ms: round(wall),
        connect_ms: round(payload.timings.connect_or_launch_ms),
        shot_ms: round(payload.timings.shot_ms),
        ...clientEvidence(definition, evidenceFile, payload.image),
      });
    }
    return {
      ...residentSettle,
      measurement_started_epoch_ms: measurementStarted,
      measurement_completed_epoch_ms: Date.now(),
      measurement_wall_time_ms: round(samples.reduce((sum, sample) => sum + sample.ms, 0)),
    };
  } finally {
    await closeWithin(host.close, 'resident host close');
  }
}

async function runLifecycle(samples) {
  const measurementStarted = Date.now();
  for (let cycle = 0; cycle < iterations; cycle += 1) {
    const definition = caseDefinitions[cycle % caseDefinitions.length];
    const started = performance.now();
    const evidenceFile = path.join(sampleDirectory,
        `${engineName}.lifecycle.c${concurrency}.r${repeat}.a${attempt}.s${String(cycle + 1).padStart(5, '0')}.png`);
    const subprocess = execaNode(tsxCli, [clientFile,
      '--start-gated', 'true',
      '--engine', engineName,
      '--url', definition.url,
      '--workers', String(concurrency),
      '--daemon-name', daemonName('lifecycle', repeat, cycle + 1),
      '--evidence-file', evidenceFile,
    ], {stdin: 'pipe', timeout: 120_000, killDescendants: true, reject: false});
    const {completed: result} = await finishOwnedChild(subprocess, `lifecycle cycle ${cycle + 1}`);
    let payload;
    try {
      payload = result.stdout.trim() ? JSON.parse(result.stdout.trim().split(/\r?\n/).at(-1)) : null;
    } catch (error) {
      throw new InfrastructureError(`lifecycle cycle ${cycle + 1} returned malformed harness JSON`, error);
    }
    if (!payload?.ok || result.exitCode !== 0) {
      throw new Error(payload?.error || result.stderr || `lifecycle cycle ${cycle} failed`);
    }
    const engineCycleMs = Object.values(payload.timings)
        .filter(Number.isFinite).reduce((sum: number, value: number) => sum + value, 0);
    samples.push({cycle, case: definition.name, ms: round(engineCycleMs), ...payload.timings,
      ...clientEvidence(definition, evidenceFile, payload.image),
      orchestration_ms: round(performance.now() - started)});
  }
  return {
    stable: true,
    cycles: iterations,
    measurement_started_epoch_ms: measurementStarted,
    measurement_completed_epoch_ms: Date.now(),
    measurement_wall_time_ms: round(samples.reduce((sum, sample) => sum + sample.ms, 0)),
  };
}

async function runBrowserProcessExit(samples) {
  const preconditionFile = path.join(sampleDirectory,
      `${engineName}.process-exit-precondition.r${repeat}.a${attempt}.png`);
  const host = await startResident(engineName, caseDefinitions[0].url, {
    workers: concurrency,
    daemonName: daemonName('process-exit'),
    evidenceFile: preconditionFile,
  });
  const token = `${engineName}-process-exit-${repeat}-${attempt}-${process.pid}`;
  let interrupted;
  let interruptedMonitor;
  let interruptedResult;
  let interruptedTelemetry;
  try {
    if (!host.rootPids?.length) {
      throw new InfrastructureError(`${engineName} did not expose its owned browser PID`);
    }
    let tree;
    try {
      tree = await waitForProcessTree(host.rootPids);
    } catch (error) {
      throw new InfrastructureError(`${engineName} browser process tree was not observable`, error);
    }
    const encodedEndpoint = Buffer.from(JSON.stringify(host.endpoint)).toString('base64url');
    interrupted = execaNode(tsxCli, [clientFile,
      '--engine', engineName,
      '--url', `${config.baseUrl}/slow?ms=5000&token=${encodeURIComponent(token)}`,
      '--endpoint', encodedEndpoint,
      '--workers', String(concurrency),
      '--timeout-ms', '10000',
    ], {timeout: 15_000, killDescendants: true, reject: false});
    interruptedMonitor = new ProcessMonitor([interrupted.pid], 50);
    await startOwnershipMonitor(interruptedMonitor, interrupted,
        `${engineName} forced-exit client`);
    await waitForRequestStarted(token, 10_000);
    const browserRemaining = await terminateOwnedProcesses(tree.processes);
    if (browserRemaining.length) {
      throw new InfrastructureError(`${engineName} browser process survived the forced-exit test`);
    }
    interruptedResult = await interrupted;
  } finally {
    if (interrupted && !interruptedResult) {
      interrupted.kill('SIGTERM');
      await interrupted.catch(() => {});
    }
    if (interruptedMonitor) interruptedTelemetry = await interruptedMonitor.stop();
    await closeAfterForcedExit(host.close, `${engineName} terminated browser close`);
  }
  if (!interruptedTelemetry?.trusted) {
    throw new InfrastructureError(
        `${engineName} forced-exit client PID monitor was unavailable: ${interruptedTelemetry?.errors?.join('; ') || 'no samples'}`);
  }
  let clientRemaining = await waitForIdentitiesToExit(
      interruptedTelemetry.identities, SETTLE.forcedExitMs);
  if (clientRemaining.length) clientRemaining = await terminateOwnedProcesses(clientRemaining);
  if (clientRemaining.length) {
    throw new InfrastructureError(`${engineName} forced-exit client left owned processes running`);
  }
  const requestFailed = interruptedResult.exitCode !== 0;
  samples.push({
    fault: 'engine-process-exited-during-request',
    request_started: true,
    rejected: requestFailed,
  });
  if (!requestFailed) {
    throw new Error(`${engineName} request succeeded after its browser process was terminated`);
  }

  const recoveryFile = path.join(sampleDirectory,
      `${engineName}.process-exit-recovered.r${repeat}.a${attempt}.png`);
  const recovered = await startResident(engineName, caseDefinitions[0].url, {
    workers: concurrency,
    daemonName: daemonName('process-exit-recovered'),
    evidenceFile: recoveryFile,
  });
  try {
    const clientFilePath = path.join(sampleDirectory,
        `${engineName}.process-exit-recovered-client.r${repeat}.a${attempt}.png`);
    const response = await residentClient(
        recovered.endpoint, caseDefinitions[0], `${engineName} process-exit recovery`,
        120_000, clientFilePath);
    samples.push({
      fault: 'engine-process-restarted',
      rejected: true,
      recovered: true,
      ms: round(response.to_png_ms),
      ...clientEvidence(caseDefinitions[0], clientFilePath, response.payload.image),
    });
  } finally {
    await closeWithin(recovered.close, `${engineName} recovered browser close`);
  }
}

async function runFaults(engine, samples) {
  const expected = [
    {name: 'missing-file', url: path.join(config.fixtureRoot, '__missing__.html'), timeoutMs: 3000},
    {name: 'unreachable', url: 'http://127.0.0.1:9/unreachable', timeoutMs: 3000},
    {name: 'timeout', url: `${config.baseUrl}/slow?ms=5000`, timeoutMs: 100},
  ];
  for (const fault of expected) {
    const started = performance.now();
    let rejected = false;
    let message = null;
    try {
      await engine.shot(fault.url, {timeoutMs: fault.timeoutMs});
    } catch (error) {
      rejected = true;
      message = String(error?.message || error);
    }
    samples.push({fault: fault.name, rejected, ms: round(performance.now() - started), error: message});
    if (!rejected) throw new Error(`${engineName} accepted fault case ${fault.name}`);
  }
  if (engineName === 'shotium') {
    const shotium = await import('@shotkit/shotium');
    let rejected = false;
    try {
      await shotium.screenshot({});
    } catch {
      rejected = true;
    }
    samples.push({fault: 'invalid-options', rejected});
    if (!rejected) throw new Error('shotium accepted an invalid request');

    const daemon: any = await createEngine('shotium-daemon', {
      workers: 2,
      daemonName: daemonName('recovery'),
    });
    try {
      await daemon.launch();
      const precondition = await daemon.shot(caseDefinitions[0].url);
      inspectPng(precondition.image, {width: 1280, height: 720});
      fs.writeFileSync(path.join(sampleDirectory,
          `${engineName}.daemon-recovery-precondition.r${repeat}.a${attempt}.png`),
      precondition.image);
      const status = await daemon.status();
      const daemonPid = Number(status.pid);
      // systeminformation caches the Windows process table. The daemon can be
      // started after the last settle sample and serve its precondition before
      // that cache refreshes, so a one-shot lookup deterministically misses the
      // live PID on fast Windows runners. Wait for the exact reported root and
      // keep the identity from that same owned-tree snapshot.
      const tree = await waitForProcessTree([daemonPid]);
      const daemonIdentity = tree.processes.find((entry) => entry.pid === daemonPid);
      if (!daemonIdentity) throw new Error('Shotium daemon PID identity could not be verified');
      const worker = tree.processes.find((entry) => entry.pid !== daemonPid);
      if (worker) {
        const remaining = await terminateOwnedProcesses([worker]);
        if (remaining.length) throw new Error('Shotium daemon worker could not be terminated for recovery test');
        const recovered = await daemon.shot(caseDefinitions[0].url, {timeoutMs: 10_000});
        inspectPng(recovered.image, {width: 1280, height: 720});
        fs.writeFileSync(path.join(sampleDirectory,
            `${engineName}.daemon-worker-recovered.r${repeat}.a${attempt}.png`), recovered.image);
        samples.push({fault: 'daemon-worker-exit', rejected: true, recovered: true, victim: worker});
      } else {
        await daemon.close({stopDaemon: false});
        const remaining = await terminateOwnedProcesses([daemonIdentity]);
        if (remaining.length) throw new Error('Shotium daemon could not be terminated for restart test');
        await daemon.launch();
        const recovered = await daemon.shot(caseDefinitions[0].url, {timeoutMs: 10_000});
        inspectPng(recovered.image, {width: 1280, height: 720});
        fs.writeFileSync(path.join(sampleDirectory,
            `${engineName}.daemon-process-restarted.r${repeat}.a${attempt}.png`), recovered.image);
        samples.push({
          fault: 'daemon-process-exit-restart',
          rejected: true,
          recovered: true,
          victim: daemonIdentity,
          daemon_worker_recovery: 'n/a',
          reason: 'this Shotium version renders inside the daemon process and exposes no worker child',
        });
      }
    } finally {
      await closeWithin(() => daemon.close({stopDaemon: true}), 'Shotium recovery daemon close');
    }
  } else {
    await runBrowserProcessExit(samples);
  }

  const requestToken = `${engineName}-${repeat}-${attempt}-${process.pid}`;
  const interrupted = execaNode(tsxCli, [clientFile,
    '--engine', engineName,
    '--url', `${config.baseUrl}/slow?ms=5000&token=${encodeURIComponent(requestToken)}`,
    '--workers', String(concurrency),
    '--daemon-name', daemonName('interrupted'),
  ], {timeout: 15_000, killDescendants: true, reject: false});
  const interruptedMonitor = new ProcessMonitor([interrupted.pid], 50);
  await startOwnershipMonitor(interruptedMonitor, interrupted,
      `${engineName} request cancellation client`);
  let interruptedResult;
  let interruptedTelemetry;
  try {
    await waitForRequestStarted(requestToken, 10_000);
    interrupted.kill('SIGTERM');
    interruptedResult = await interrupted;
  } finally {
    if (!interruptedResult) {
      interrupted.kill('SIGTERM');
      await interrupted.catch(() => {});
    }
    interruptedTelemetry = await interruptedMonitor.stop();
  }
  if (!interruptedTelemetry.trusted) {
    throw new InfrastructureError(
        'request cancellation PID monitor was unavailable: ' +
        `${interruptedTelemetry.errors.join('; ') || 'no sample observed the client PID'}`);
  }
  let interruptedRemaining = await waitForIdentitiesToExit(
      interruptedTelemetry.identities, SETTLE.forcedExitMs);
  if (interruptedRemaining.length) interruptedRemaining = await terminateOwnedProcesses(interruptedRemaining);
  if (interruptedRemaining.length) throw new Error('cancelled request left an owned process tree running');
  const cancelled = interruptedResult.exitCode !== 0 || interruptedResult.signal !== undefined;
  samples.push({fault: 'request-cancelled-by-client-exit', request_started: true, rejected: cancelled});
  if (!cancelled) throw new Error(`${engineName} request continued after its client exited`);
  return {stable: true};
}

async function runEngineScenario(samples) {
  if (scenario === 'resident') return runResident(samples);
  if (scenario === 'lifecycle') return runLifecycle(samples);

  const engine = await createEngine(engineName, {
    workers: concurrency,
    daemonName: daemonName(),
    reusePage: scenario === 'reuse-page',
    profileDir: path.join(config.tempProfileDirectory,
        `${engineName}.${scenario}.r${repeat}.a${attempt}`),
  });
  const quality: Record<string, any> = {stable: true};
  try {
    const launchStartedEpochMs = Date.now();
    const launchStarted = performance.now();
    await engine.launch();
    quality.launch_ms = round(performance.now() - launchStarted);
    quality.dispatch_to_launch_ms = Math.max(0, launchStartedEpochMs - dispatchedAtMs);
    if (scenario === 'cold') {
      const cold = await capture(engine, caseDefinitions[0], samples);
      quality.measurement_started_epoch_ms = launchStartedEpochMs;
      quality.measurement_completed_epoch_ms = cold.shot_completed_epoch_ms;
      quality.cold_start_to_png_ms = round(cold.shot_completed_epoch_ms - launchStartedEpochMs);
      quality.measurement_wall_time_ms = quality.cold_start_to_png_ms;
      return quality;
    }

    let settleEvidenceIndex = 0;
    const inspect = (image) => {
      inspectPng(image, {width: 1280, height: 720});
      settleEvidenceIndex += 1;
      fs.writeFileSync(path.join(sampleDirectory,
          `${engineName}.${scenario}.settle.r${repeat}.a${attempt}.s${String(settleEvidenceIndex).padStart(2, '0')}.png`),
      image);
    };
    const settle = await settleEngine(engine, caseDefinitions[0].url, inspect, {cpuLimit: hostCpuLimit});
    Object.assign(quality, settle);
    if (!settle.stable) return quality;
    if (scenario === 'faults') return {...quality, ...await runFaults(engine, samples)};
    if (scenario === 'cold-settled') {
      const measured = performance.now();
      quality.measurement_started_epoch_ms = Date.now();
      await capture(engine, caseDefinitions[0], samples);
      quality.measurement_completed_epoch_ms = Date.now();
      quality.measurement_wall_time_ms = round(performance.now() - measured);
      return quality;
    }
    if (scenario === 'warm' || scenario === 'reuse-page') {
      const measured = performance.now();
      quality.measurement_started_epoch_ms = Date.now();
      const bench = new Bench({time: 0, iterations, warmup: false, throws: true});
      bench.add('screenshot', async () => {
        await capture(engine, caseDefinitions[0], samples, `warm-${samples.length}`);
      });
      await bench.run();
      quality.measurement_completed_epoch_ms = Date.now();
      quality.tinybench = bench.tasks[0]?.result || null;
      if (quality.tinybench?.error || samples.length !== iterations) {
        throw new Error(`Tinybench completed ${samples.length}/${iterations} screenshots`);
      }
      quality.measurement_wall_time_ms = round(performance.now() - measured);
      return quality;
    }
    if (scenario === 'batch') {
      const measured = performance.now();
      quality.measurement_started_epoch_ms = Date.now();
      for (let round = 0; round < iterations; round += 1) {
        for (const definition of caseDefinitions) await capture(engine, definition, samples);
      }
      quality.measurement_completed_epoch_ms = Date.now();
      quality.measurement_wall_time_ms = round(performance.now() - measured);
      return quality;
    }
    if (scenario === 'parallel') {
      const measured = performance.now();
      quality.measurement_started_epoch_ms = Date.now();
      const jobs = [];
      for (let round = 0; round < iterations; round += 1) jobs.push(...caseDefinitions);
      await mapConcurrent(jobs, concurrency, (definition, index) =>
        capture(engine, definition, samples, `parallel-${concurrency}-${index}`));
      quality.measurement_completed_epoch_ms = Date.now();
      quality.measurement_wall_time_ms = round(performance.now() - measured);
      return quality;
    }
    if (scenario === 'soak') {
      const measured = performance.now();
      quality.measurement_started_epoch_ms = Date.now();
      const deadline = Date.now() + scenarioTimeoutMs;
      const jobs = Array.from({length: iterations}, (_, index) => index);
      let succeeded = 0;
      let failed = 0;
      await mapConcurrent(jobs, concurrency, async (index) => {
        const remainingMs = deadline - Date.now();
        if (remainingMs <= 0) return;
        const definition = caseDefinitions[index % caseDefinitions.length];
        try {
          await capture(
              engine, definition, samples, `soak-${index}`,
              Math.max(1, Math.min(remainingMs, definition.timeout_ms || 30_000)));
          succeeded += 1;
        } catch (error) {
          failed += 1;
          samples.push({case: definition.name, label: `soak-${index}`, failed: true, error: errorText(error)});
        }
      });
      quality.requested_soak_iterations = iterations;
      quality.completed_soak_iterations = succeeded + failed;
      quality.succeeded_soak_iterations = succeeded;
      quality.failed_soak_iterations = failed;
      quality.failure_rate = (succeeded + failed) ? failed / (succeeded + failed) : null;
      quality.soak_stop_reason = succeeded + failed >= iterations ?
        'iteration-limit' : 'time-limit';
      quality.measurement_completed_epoch_ms = Date.now();
      quality.measurement_wall_time_ms = round(performance.now() - measured);
      return quality;
    }
    throw new Error(`unknown scenario ${scenario}`);
  } finally {
    await closeWithin(() => engine.close());
  }
}

const monitor = new ProcessMonitor([process.pid]);
const samples = [];
const startedAt = new Date().toISOString();
const wallStarted = performance.now();
let result;
await monitor.start();
let quality: Record<string, any> | null = null;
let scenarioError: any = null;
try {
  quality = await runEngineScenario(samples);
} catch (error) {
  scenarioError = error;
}
const telemetry = await monitor.stop();
try {
  await flushEvidence();
} catch (error) {
  if (scenarioError) {
    const message = `${errorText(scenarioError)}\nevidence validation failed: ${errorText(error)}`;
    scenarioError = isInfrastructureError(scenarioError) || isInfrastructureError(error) ?
      new InfrastructureError(message, isInfrastructureError(error) ? error : scenarioError) :
      new Error(message);
  } else {
    scenarioError = error;
  }
}
if (!scenarioError && quality) {
  const imageSamples = samples.filter((sample) => sample.image?.deterministic_against_first !== undefined);
  quality.determinism = {
    compared: imageSamples.length,
    differing: imageSamples.filter((sample) => !sample.image.deterministic_against_first).length,
  };
  const deterministic = quality.determinism.differing === 0;
  const requestsPassed = !(quality.failed_soak_iterations > 0);
  result = {
    ok: deterministic && requestsPassed,
    status: !deterministic || !requestsPassed ? 'fail' : quality.stable === false ? 'noisy' : 'pass',
    engine: engineName,
    scenario,
    repeat,
    concurrency,
    started_at: startedAt,
    wall_time_ms: round(performance.now() - wallStarted),
    quality,
    samples,
    latency_ms: distribution(samples.map((sample) => sample.ms)),
    error: !deterministic ?
      'static PNG output perceptually differed from the first image for the same engine and case' :
      !requestsPassed ? `soak recorded ${quality.failed_soak_iterations} failed requests` : undefined,
  };
} else {
  result = {
    ok: false,
    status: isInfrastructureError(scenarioError) ? 'infra-error' : 'fail',
    engine: engineName,
    scenario,
    repeat,
    concurrency,
    started_at: startedAt,
    wall_time_ms: round(performance.now() - wallStarted),
    error: errorText(scenarioError || new Error('scenario returned no quality result')),
    samples,
  };
  process.exitCode = 1;
}
{
  fs.mkdirSync(config.telemetryDirectory, {recursive: true});
  const telemetryFile = path.join(config.telemetryDirectory,
      `${engineName}.${scenario}.c${concurrency}.r${repeat}.a${attempt}.json`);
  fs.writeFileSync(telemetryFile, `${JSON.stringify(telemetry)}\n`);
  result.telemetry_file = path.relative(config.outputDirectory, telemetryFile).replaceAll('\\', '/');
  result.peak_cpu_percent = telemetry.peak_cpu_percent;
  result.process_identities = telemetry.identities;
  const measurementStart = result.quality?.measurement_started_epoch_ms;
  const measurementEnd = result.quality?.measurement_completed_epoch_ms;
  let evidenceIndex = 0;
  let retainedAtSample = 0;
  const allRssTimeline = telemetry.timeline.filter((sample) =>
    Number.isFinite(sample.rss_bytes) && Number.isFinite(Date.parse(sample.at))).map((sample) => {
    const sampleTime = Date.parse(sample.at);
    while (evidenceIndex < evidenceMemoryTimeline.length &&
        evidenceMemoryTimeline[evidenceIndex].at_epoch_ms <= sampleTime) {
      retainedAtSample = evidenceMemoryTimeline[evidenceIndex].retained_bytes;
      evidenceIndex += 1;
    }
    return {
      ...sample,
      raw_rss_bytes: sample.rss_bytes,
      rss_bytes: Math.max(0, sample.rss_bytes - retainedAtSample),
      retained_evidence_bytes: retainedAtSample,
    };
  });
  const hasMeasurementWindow = Number.isFinite(measurementStart) && Number.isFinite(measurementEnd);
  const measuredRssTimeline = hasMeasurementWindow ?
    allRssTimeline.filter((sample) => {
      const at = Date.parse(sample.at);
      return at >= measurementStart && at <= measurementEnd;
    }) : allRssTimeline;
  const rssTimeline = hasMeasurementWindow ? measuredRssTimeline : allRssTimeline;
  result.peak_rss_bytes = rssTimeline.length ?
    Math.max(...rssTimeline.map((sample) => sample.rss_bytes)) : null;
  result.raw_peak_rss_bytes = rssTimeline.length ?
    Math.max(...rssTimeline.map((sample) => sample.raw_rss_bytes)) : null;
  result.lifecycle_raw_peak_rss_bytes = telemetry.peak_rss_bytes;
  const rssSpanMs = rssTimeline.length > 1 ?
    Date.parse(rssTimeline.at(-1).at) - Date.parse(rssTimeline[0].at) : 0;
  result.rss_slope_bytes_per_minute = rssSlopePerMinute(rssTimeline, 'rss_bytes');
  result.raw_rss_slope_bytes_per_minute = rssSlopePerMinute(rssTimeline, 'raw_rss_bytes');
  result.quality = result.quality || {};
  result.quality.rss_adjustment = {
    method: 'subtract-retained-png-output-bytes',
    slope_method: 'ordinary-least-squares',
    slope_minimum_samples: 3,
    slope_minimum_span_ms: 1000,
    measurement_samples: rssTimeline.length,
    measurement_span_ms: Math.max(0, rssSpanMs),
    peak_retained_evidence_bytes: evidenceMemoryTimeline.length ?
      Math.max(...evidenceMemoryTimeline.map((entry) => entry.retained_bytes)) : 0,
  };
  result.quality.rss_slope_bytes_per_minute = result.rss_slope_bytes_per_minute;
  if (!telemetry.trusted) {
    result.ok = false;
    result.status = 'infra-error';
    result.error = `${result.error || ''}\nPID/RSS monitor was unavailable: ${telemetry.errors.join('; ')}`.trim();
  }
  if (!result.ok) process.exitCode = 1;
  await new Promise<void>((resolve) =>
    process.stdout.write(`${JSON.stringify(result)}\n`, () => resolve()));
  process.exit(result.ok ? 0 : 1);
}
