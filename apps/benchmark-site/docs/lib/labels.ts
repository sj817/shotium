import type {ResultStatus, ScenarioSummary} from './types';

export const REPO_URL = 'https://github.com/sj817/shotium';

export const ENGINE_IDS = [
  'shotium',
  'playwright-shell',
  'playwright-chrome',
  'puppeteer-shell',
  'puppeteer-chrome',
] as const;

export type EngineId = typeof ENGINE_IDS[number];

export function isKnownEngine(engine: string): engine is EngineId {
  return (ENGINE_IDS as readonly string[]).includes(engine);
}

/** Engines in display order: Shotium, then the fixed competitor order, then unknown names. */
export function orderEngines(names: Iterable<string>): string[] {
  const set = new Set(names);
  const known = ENGINE_IDS.filter((id) => set.has(id));
  const unknown = [...set].filter((name) => !isKnownEngine(name)).sort();
  return [...known, ...unknown];
}

export function engineColorVar(engine: string): string {
  return isKnownEngine(engine) ? `var(--engine-${engine})` : 'var(--engine-other)';
}

export const SHARD_ORDER = ['startup', 'throughput', 'parallel', 'resident', 'resilience'] as const;

export const SCENARIO_ORDER = [
  'cold', 'cold-settled', 'warm', 'batch', 'parallel', 'resident', 'reuse-page', 'lifecycle', 'faults', 'soak',
] as const;

const SCENARIO_SHARD: Record<string, string> = {
  'cold': 'startup',
  'cold-settled': 'startup',
  'warm': 'throughput',
  'batch': 'throughput',
  'parallel': 'parallel',
  'resident': 'resident',
  'reuse-page': 'resident',
  'lifecycle': 'resilience',
  'faults': 'resilience',
  'soak': 'resilience',
};

export function scenarioShard(row: Pick<ScenarioSummary, 'scenario' | 'shard'>): string {
  return row.shard ?? SCENARIO_SHARD[row.scenario] ?? 'other';
}

function orderIndex(list: readonly string[], value: string): number {
  const index = list.indexOf(value);
  return index === -1 ? list.length : index;
}

export function compareScenarios(
  left: Pick<ScenarioSummary, 'scenario' | 'concurrency' | 'shard'>,
  right: Pick<ScenarioSummary, 'scenario' | 'concurrency' | 'shard'>,
): number {
  const shard = orderIndex(SHARD_ORDER, scenarioShard(left)) - orderIndex(SHARD_ORDER, scenarioShard(right));
  if (shard !== 0) return shard;
  const scenario = orderIndex(SCENARIO_ORDER, left.scenario) - orderIndex(SCENARIO_ORDER, right.scenario);
  if (scenario !== 0) return scenario || left.scenario.localeCompare(right.scenario);
  return left.concurrency - right.concurrency;
}

export type StatusTone = 'pass' | 'noisy' | 'fail' | 'na' | 'infra' | 'neutral';

export function statusTone(status: ResultStatus | string | null | undefined): StatusTone {
  switch (status) {
    case 'pass':
    case 'complete':
      return 'pass';
    case 'noisy':
    case 'incomplete':
      return 'noisy';
    case 'fail':
      return 'fail';
    case 'n/a':
      return 'na';
    case 'infra-error':
      return 'infra';
    default:
      return 'neutral';
  }
}

export function statusColorVar(status: ResultStatus | string | null | undefined): string {
  const tone = statusTone(status);
  return tone === 'neutral' ? 'var(--status-na)' : `var(--status-${tone})`;
}
