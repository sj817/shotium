import type {EngineSummary, PlatformSummary, ResultStatus, ScenarioSummary} from './types';

export type ExclusionCode =
  'engine-unavailable' | 'missing-baseline' | 'missing-cell' | 'invalid-cell' | 'partial-coverage';

export interface RankingExclusion {
  code: ExclusionCode;
  count: number;
  detail?: string;
}

export interface RankingRow {
  engine: string;
  status: ResultStatus;
  geometricMean: number | null;
  official: boolean;
  compared: number;
  champions: number;
  totalBaselines: number;
  excluded: number;
  exclusions: RankingExclusion[];
}

export function scenarioKey(row: Pick<ScenarioSummary, 'scenario' | 'concurrency'>): string {
  return `${row.scenario}|${row.concurrency}`;
}

export function scenarioP50(row: ScenarioSummary): number | null {
  const value = row.latency_ms?.p50 ?? row.wall_time_ms?.p50;
  return Number.isFinite(value) && Number(value) > 0 ? Number(value) : null;
}

export function geometricMean(values: number[]): number | null {
  if (!values.length || values.some((value) => !Number.isFinite(value) || value <= 0)) return null;
  return Math.exp(values.reduce((sum, value) => sum + Math.log(value), 0) / values.length);
}

function engineMap(summary: PlatformSummary): Map<string, EngineSummary> {
  return new Map(summary.engines.map((engine) => [engine.engine, engine]));
}

export function rankPlatform(summary: PlatformSummary): RankingRow[] {
  const engines = engineMap(summary);
  const names = [...new Set([
    ...summary.engines.map((engine) => engine.engine),
    ...summary.scenarios.map((scenario) => scenario.engine),
  ])];
  const shotiumBaselines = new Map<string, ScenarioSummary>();
  for (const scenario of summary.scenarios) {
    if (scenario.engine === 'shotium' && scenario.status === 'pass' &&
        scenario.ranking_eligible && scenarioP50(scenario) !== null) {
      shotiumBaselines.set(scenarioKey(scenario), scenario);
    }
  }
  const scenariosByEngine = new Map<string, Map<string, ScenarioSummary>>();
  for (const scenario of summary.scenarios) {
    const rows = scenariosByEngine.get(scenario.engine) ?? new Map<string, ScenarioSummary>();
    rows.set(scenarioKey(scenario), scenario);
    scenariosByEngine.set(scenario.engine, rows);
  }
  const baselines = new Map([...shotiumBaselines].filter(([key]) =>
    names.some((name) => {
      if (name === 'shotium') return false;
      const candidate = scenariosByEngine.get(name)?.get(key);
      return candidate?.status === 'pass' && candidate.ranking_eligible && scenarioP50(candidate) !== null;
    })));

  const ratiosByEngine = new Map<string, Map<string, number>>();
  const exclusionsByEngine = new Map<string, RankingExclusion[]>();
  for (const name of names) {
    const engine = engines.get(name);
    const ratios = new Map<string, number>();
    const counts = new Map<ExclusionCode, number>();
    if (engine?.status === 'n/a') {
      counts.set('engine-unavailable', baselines.size || 1);
    } else if (!baselines.size) {
      counts.set('missing-baseline', 1);
    } else {
      const rows = scenariosByEngine.get(name);
      for (const [key, baseline] of baselines) {
        const row = rows?.get(key);
        if (!row) {
          counts.set('missing-cell', (counts.get('missing-cell') ?? 0) + 1);
          continue;
        }
        const p50 = scenarioP50(row);
        if (row.status !== 'pass' || !row.ranking_eligible || p50 === null) {
          counts.set('invalid-cell', (counts.get('invalid-cell') ?? 0) + 1);
          continue;
        }
        const baselineP50 = scenarioP50(baseline);
        if (baselineP50 !== null) ratios.set(key, p50 / baselineP50);
      }
    }
    ratiosByEngine.set(name, ratios);
    const exclusions = [...counts].map(([code, count]) => ({
      code,
      count,
      ...(code === 'engine-unavailable' && engine?.reason ? {detail: engine.reason} : {}),
    }));
    if (engine?.status !== 'n/a' && baselines.size > 0 && ratios.size < baselines.size) {
      exclusions.push({code: 'partial-coverage', count: baselines.size - ratios.size});
    }
    exclusionsByEngine.set(name, exclusions);
  }

  const champions = new Map(names.map((name) => [name, 0]));
  for (const key of baselines.keys()) {
    const candidates = names.flatMap((name) => {
      const ratio = ratiosByEngine.get(name)?.get(key);
      return ratio === undefined ? [] : [{name, ratio}];
    });
    if (!candidates.length) continue;
    const best = Math.min(...candidates.map((candidate) => candidate.ratio));
    for (const candidate of candidates) {
      if (Math.abs(candidate.ratio - best) <= 1e-9) {
        champions.set(candidate.name, (champions.get(candidate.name) ?? 0) + 1);
      }
    }
  }

  return names.map((name) => {
    const ratios = [...(ratiosByEngine.get(name)?.values() ?? [])];
    return {
      engine: name,
      status: engines.get(name)?.status ?? 'infra-error',
      geometricMean: geometricMean(ratios),
      official: baselines.size > 0 && ratios.length === baselines.size,
      compared: ratios.length,
      champions: champions.get(name) ?? 0,
      totalBaselines: baselines.size,
      excluded: Math.max(0, baselines.size - ratios.length),
      exclusions: exclusionsByEngine.get(name) ?? [],
    };
  }).sort((left, right) => {
    if (left.official !== right.official) return left.official ? -1 : 1;
    if (left.geometricMean === null && right.geometricMean !== null) return 1;
    if (left.geometricMean !== null && right.geometricMean === null) return -1;
    if (left.geometricMean !== null && right.geometricMean !== null &&
        left.geometricMean !== right.geometricMean) return left.geometricMean - right.geometricMean;
    return left.engine.localeCompare(right.engine);
  });
}

export function ratioForScenario(row: ScenarioSummary, summary: PlatformSummary): number | null {
  if (!row.ranking_eligible || row.status !== 'pass') return null;
  const baseline = summary.scenarios.find((candidate) =>
    candidate.engine === 'shotium' && candidate.status === 'pass' && candidate.ranking_eligible &&
    scenarioKey(candidate) === scenarioKey(row));
  const value = scenarioP50(row);
  const baselineValue = baseline ? scenarioP50(baseline) : null;
  return value !== null && baselineValue !== null ? value / baselineValue : null;
}
