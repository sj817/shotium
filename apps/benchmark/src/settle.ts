import {SETTLE} from './constants.ts';
import {processSnapshot, waitForSystemStable} from './process-tree.ts';
import {coefficientOfVariation, relativeDrift, round} from './statistics.ts';

export async function settleEngine(engine, url, inspect) {
  const deadline = Date.now() + SETTLE.timeoutMs;
  const latencies = [];
  const rss = [];
  let latencyCv = null;
  let rssDrift = null;
  for (let attempt = 1; attempt <= SETTLE.maximumWarmups; attempt += 1) {
    const remainingMs = deadline - Date.now();
    if (remainingMs <= 0) break;
    const started = performance.now();
    const result = await engine.shot(url, {timeoutMs: Math.min(30_000, remainingMs)});
    const elapsed = performance.now() - started;
    inspect(result.image);
    latencies.push(round(elapsed));
    const snapshot = await processSnapshot([process.pid]);
    rss.push(snapshot.rss_bytes);
    if (attempt < SETTLE.minimumWarmups) continue;
    latencyCv = coefficientOfVariation(latencies.slice(-SETTLE.stableSamples));
    rssDrift = relativeDrift(rss.slice(-SETTLE.stableSamples));
    if (latencyCv <= SETTLE.latencyCvLimit && rssDrift <= SETTLE.rssDriftLimit) {
      const system = await waitForSystemStable({timeoutMs: Math.max(0, deadline - Date.now())});
      return {
        stable: system.stable,
        warmups: attempt,
        latency_ms: latencies,
        rss_bytes: rss,
        latency_cv: round(latencyCv, 5),
        rss_drift: round(rssDrift, 5),
        system,
      };
    }
  }
  return {
    stable: false,
    warmups: latencies.length,
    latency_ms: latencies,
    rss_bytes: rss,
    latency_cv: round(latencyCv, 5),
    rss_drift: round(rssDrift, 5),
    system: {stable: false, samples: []},
  };
}
