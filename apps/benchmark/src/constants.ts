import path from 'node:path';
import {fileURLToPath} from 'node:url';

export const HERE = path.dirname(fileURLToPath(import.meta.url));
export const APP_ROOT = path.dirname(HERE);
export const FIXTURE_ROOT = path.join(APP_ROOT, 'fixtures');
export const VIEWPORT = Object.freeze({width: 1280, height: 720});

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
  },
});

export const SETTLE = Object.freeze({
  minimumWarmups: 3,
  maximumWarmups: 10,
  latencyCvLimit: 0.10,
  rssDriftLimit: 0.03,
  // Host CPU gate. GitHub's Windows and macOS runners idle at 28-41% CPU, so a
  // fixed 25% ceiling could never be met there: every cell burned the whole
  // cooldown timeout, produced zero samples and was retried once. The limit is
  // therefore calibrated per shard from the idle baseline measured before the
  // first cell: max(cpuLimit, p95(idle) + cpuLimitMargin), capped at cpuLimitMax.
  cpuLimit: 25,
  cpuLimitMargin: 10,
  cpuLimitMax: 80,
  calibrationMs: 5_000,
  memoryDriftLimit: 0.02,
  stableSamples: 3,
  // Three one-second samples spanning at least two seconds: a three-second
  // window is enough to tell "the previous browser is still exiting" from
  // "quiet". The old 45 s cooldown was a timeout for a condition the runner
  // could not satisfy, not time the measurement needed.
  stableSpanMs: 2_000,
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
