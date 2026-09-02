import {SETTLE} from './constants.ts';
import {processSnapshot, waitForSystemStable} from './process-tree.ts';
import {coefficientOfVariation, relativeDrift, round} from './statistics.ts';

export function readinessDiagnostics(latencies, rss) {
  const recentLatency = latencies.slice(-SETTLE.stableSamples);
  const recentRss = rss.slice(-SETTLE.stableSamples);
  const latencyCv = coefficientOfVariation(recentLatency);
  const rssDrift = relativeDrift(recentRss);
  return {
    latency_cv: round(latencyCv, 5),
    rss_drift: round(rssDrift, 5),
    warmup_latency_stable: latencyCv <= SETTLE.latencyCvLimit,
    warmup_rss_stable: rssDrift <= SETTLE.rssDriftLimit,
  };
}

export async function settleEngine(engine, url, inspect,
    {cpuLimit = SETTLE.cpuLimit as number}: {cpuLimit?: number} = {}) {
  const deadline = Date.now() + SETTLE.timeoutMs;
  const latencies = [];
  const rss = [];
  for (let attempt = 1; attempt <= SETTLE.minimumWarmups; attempt += 1) {
    const remainingMs = deadline - Date.now();
    if (remainingMs <= 0) break;
    const started = performance.now();
    const result = await engine.shot(url, {timeoutMs: Math.min(30_000, remainingMs)});
    const elapsed = performance.now() - started;
    inspect(result.image);
    latencies.push(round(elapsed));
    const snapshot = await processSnapshot([process.pid]);
    rss.push(snapshot.rss_bytes);
  }
  const system = latencies.length === SETTLE.minimumWarmups ? await waitForSystemStable({
    timeoutMs: Math.min(SETTLE.cooldownTimeoutMs, Math.max(0, deadline - Date.now())),
    cpuLimit,
    // Process-tree RSS churn is an engine property (Chrome creates and reaps
    // renderer processes for new-page captures), not evidence that the shared
    // runner changed underneath the cell. Keep it as a diagnostic and measure
    // RSS during the scenario; only the preflight host gate uses free-memory
    // drift to decide whether a cell is eligible.
    memoryDriftLimit: Number.POSITIVE_INFINITY,
  }) : {stable: false, cpu_limit: cpuLimit, samples: []};
  const diagnostics = readinessDiagnostics(latencies, rss);
  return {
    stable: system.stable,
    warmups: latencies.length,
    latency_ms: latencies,
    rss_bytes: rss,
    ...diagnostics,
    readiness_gate: 'fixed-warmups-plus-host-cpu',
    system,
  };
}
