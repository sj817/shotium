import crypto from 'node:crypto';

const PLATFORM_SLUGS = Object.freeze({
  'linux-x64': 'lx',
  'linux-arm64': 'la',
  'win32-x64': 'wx',
  'win32-arm64': 'wa',
  'darwin-x64': 'dx',
  'darwin-arm64': 'da',
});

const SCENARIO_SLUGS = Object.freeze({
  cold: 'cd',
  'cold-settled': 'cs',
  warm: 'wm',
  'reuse-page': 'rp',
  batch: 'bt',
  resident: 'rs',
  lifecycle: 'lc',
  faults: 'ft',
  parallel: 'pl',
  soak: 'sk',
  recovery: 'rc',
  interrupted: 'in',
  'process-exit': 'pe',
  'process-exit-recovered': 'pr',
});

function shortHash(value: string): string {
  return crypto.createHash('sha256').update(value).digest('hex').slice(0, 8);
}

function boundedOrdinal(value: number, label: string): string {
  if (!Number.isInteger(value) || value < 0 || value >= 36 ** 2) {
    throw new Error(`${label} must be an integer from 0 through ${36 ** 2 - 1}`);
  }
  return value.toString(36).padStart(2, '0');
}

export function benchmarkDaemonName({
  runId,
  platform,
  scenario,
  repeat = 1,
  variant = 0,
}: {
  runId: string;
  platform: string;
  scenario: string;
  repeat?: number;
  variant?: number;
}): string {
  const platformSlug = PLATFORM_SLUGS[platform];
  const scenarioSlug = SCENARIO_SLUGS[scenario];
  if (!platformSlug) throw new Error(`unsupported daemon platform ${platform}`);
  if (!scenarioSlug) throw new Error(`unsupported daemon scenario ${scenario}`);
  return `sb-${shortHash(String(runId))}-${platformSlug}-${scenarioSlug}-${
    boundedOrdinal(repeat, 'repeat')}-${boundedOrdinal(variant, 'variant')}`;
}

export const BENCHMARK_DAEMON_NAME_MAX_BYTES = 23;
