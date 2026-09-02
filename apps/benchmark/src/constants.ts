import path from 'node:path';
import {fileURLToPath} from 'node:url';

export const HERE = path.dirname(fileURLToPath(import.meta.url));
export const APP_ROOT = path.dirname(HERE);
export const FIXTURE_ROOT = path.join(APP_ROOT, 'fixtures');
export const VIEWPORT = Object.freeze({width: 1280, height: 720});
// Keep navigation and screenshot failures comparable across engines. Puppeteer
// otherwise inherits CDP's 180-second protocol timeout while Shotium rejects a
// request after 30 seconds and Playwright screenshots have no timeout by
// default, turning one broken cell into a multi-minute CI straggler.
export const BROWSER_OPERATION_TIMEOUT_MS = 30_000;

export const PLATFORM_IDS = Object.freeze([
  'linux-x64',
  'linux-arm64',
  'win32-x64',
  'win32-arm64',
  'darwin-x64',
  'darwin-arm64',
]);

export const ENGINE_IDS = Object.freeze([
  'shotium',
  'puppeteer-shell',
  'puppeteer-chrome',
  'playwright-shell',
  'playwright-chrome',
]);

export const RESULT_STATUSES = Object.freeze([
  'pass',
  'fail',
  'noisy',
  'n/a',
  'infra-error',
]);

export const SHARD_IDS = Object.freeze([
  'all',
  'startup',
  'throughput',
  'parallel',
  'resident',
  'resilience',
]);

export const SHARD_SCENARIOS = Object.freeze({
  startup: Object.freeze(['cold', 'cold-settled', 'lifecycle']),
  throughput: Object.freeze(['warm', 'batch']),
  parallel: Object.freeze(['parallel']),
  resident: Object.freeze(['resident', 'reuse-page']),
  resilience: Object.freeze(['faults', 'soak']),
});

export const PROFILES = Object.freeze({
  smoke: {
    repeats: 1,
    warmIterations: 5,
    warmChunks: 1,
    batchRounds: 1,
    concurrencies: [1, 2],
    lifecycleCycles: 3,
    lifecycleChunks: 1,
    soakIterations: 50,
    soakTimeoutMs: 60_000,
    caseLimit: 3,
    shardBudgetMs: 15 * 60_000,
  },
  full: {
    repeats: 7,
    warmIterations: 20,
    warmChunks: 5,
    batchRounds: 7,
    concurrencies: [1, 2, 4],
    lifecycleCycles: 20,
    lifecycleChunks: 5,
    soakIterations: 1000,
    soakTimeoutMs: 600_000,
    caseLimit: Number.POSITIVE_INFINITY,
    // The GitHub job timeout kills the uploads with it, so stop scheduling with
    // enough margin to write and upload what we have. Parallel now has 15
    // engine/concurrency cells rather than 105 engine/round/concurrency cells,
    // and every browser operation is capped at 30 seconds. A shard still using
    // 45 minutes is a failed/noisy diagnostic, not work worth extending to the
    // old 85-minute mask.
    shardBudgetMs: 45 * 60_000,
  },
});

export const SETTLE = Object.freeze({
  minimumWarmups: 3,
  latencyCvLimit: 0.10,
  rssDriftLimit: 0.03,
  // Host CPU gate. GitHub's Windows and macOS runners idle at 28-41% CPU, so a
  // fixed 25% ceiling could never be met there: every cell burned the whole
  // cooldown timeout, produced zero samples and was retried once. The limit is
  // therefore calibrated per shard from the idle baseline measured before the
  // first cell: max(cpuLimit, p95(idle) + cpuLimitMargin).
  cpuLimit: 25,
  cpuLimitMargin: 10,
  // Soft ceiling. It keeps the gate meaningful on a quiet host, but it is never
  // allowed to sit below the host's own idle floor: a gate under the floor is
  // one nothing can pass, and that is exactly how every macOS cell was rejected
  // for load the runner produced at rest.
  cpuLimitMax: 80,
  calibrationMs: 5_000,
  // Share of one core a single process sampler may consume. si.processes()
  // enumerates the whole process table - 30-60 ms via /proc on Linux, ~700 ms
  // via PowerShell on Windows - and the sample loop used to re-enter as soon as
  // a sample returned. On Windows and macOS that pinned a core per monitor,
  // three monitors ran at once, and the host load the gate measured was our own
  // instrumentation. The requested interval is now a floor and this is the cap.
  samplerDutyCycle: 0.2,
  // A hung process-table query must not look like a slow one. Windows had no
  // bound at all: the CLI blocked forever inside systeminformation and the job
  // died at the 90-minute GitHub timeout with an empty log.
  processQueryTimeoutMs: 60_000,
  heartbeatMs: 60_000,
  memoryDriftLimit: 0.02,
  stableSamples: 3,
  // Three one-second samples spanning at least two seconds: a three-second
  // window is enough to tell "the previous browser is still exiting" from
  // "quiet". The old 45 s cooldown was a timeout for a condition the runner
  // could not satisfy, not time the measurement needed.
  stableSpanMs: 2_000,
  // A retry gets a wider window than the first preflight, but not the full
  // 45-second engine-settle budget. A persistently busy macOS runner spent
  // 45 seconds on each of nine rejected cells without producing one sample;
  // 15 seconds still covers browser teardown while bounding that waste.
  retryCooldownTimeoutMs: 15_000,
  timeoutMs: 45_000,
  cooldownTimeoutMs: 6_000,
  gracefulExitMs: 10_000,
  forcedExitMs: 5_000,
});

export function currentPlatformId() {
  const id = `${process.platform}-${process.arch}`;
  if (!PLATFORM_IDS.includes(id)) {
    throw new Error(`unsupported benchmark platform ${id}`);
  }
  return id;
}
