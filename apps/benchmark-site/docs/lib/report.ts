import {rankPlatform, type RankingRow} from './ranking';
import {orderEngines} from './labels';
import {
  PLATFORM_IDS,
  type LoadedRun,
  type ManifestPlatform,
  type PlatformData,
  type PlatformId,
} from './types';

/*
 * Run-level aggregation on top of rankPlatform(). Everything the verdict
 * sentence and the overview matrix say comes from here, so a unit test can
 * pin the wording's inputs without touching the DOM.
 */

export type NoRankingReason =
  'not-archived' | 'quality' | 'no-competitor' | 'no-comparable' | 'partial-coverage';

export interface PlatformReport {
  id: PlatformId;
  data: PlatformData;
  manifest: ManifestPlatform | null;
  /** Result status shown in row headers: summary status, or 'missing'. */
  status: string;
  rows: RankingRow[];
  /** Engine → official rank (1-based, ties share a rank). Reference rows are absent. */
  ranks: Map<string, number>;
  comparable: number;
  officialCount: number;
  ranked: boolean;
  noRankingReason: NoRankingReason | null;
  /** Best official competitor, i.e. the engine to beat. */
  runnerUp: RankingRow | null;
  shotiumFirst: boolean | null;
  /** Ordered engine names present in this platform's summary. */
  engines: string[];
}

export interface RunVerdict {
  tested: number;
  ranked: PlatformReport[];
  unranked: PlatformReport[];
  shotiumFirst: number;
  ratioMin: number | null;
  ratioMax: number | null;
  closest: {platform: PlatformId; engine: string; ratio: number} | null;
}

export interface RunReport {
  run: LoadedRun;
  platforms: PlatformReport[];
  byId: Record<PlatformId, PlatformReport>;
  verdict: RunVerdict;
}

const EPSILON = 1e-9;

export function assignRanks(rows: RankingRow[]): Map<string, number> {
  const ranks = new Map<string, number>();
  const official = rows
    .filter((row) => row.official && row.geometricMean !== null)
    .sort((left, right) => (left.geometricMean as number) - (right.geometricMean as number));
  let rank = 0;
  let previous: number | null = null;
  official.forEach((row, index) => {
    const value = row.geometricMean as number;
    if (previous === null || Math.abs(value - previous) > EPSILON) rank = index + 1;
    ranks.set(row.engine, rank);
    previous = value;
  });
  return ranks;
}

export function buildPlatformReport(data: PlatformData, manifest: ManifestPlatform | null): PlatformReport {
  const summary = data.summary;
  if (!summary) {
    return {
      id: data.id,
      data,
      manifest,
      status: 'missing',
      rows: [],
      ranks: new Map(),
      comparable: 0,
      officialCount: 0,
      ranked: false,
      noRankingReason: 'not-archived',
      runnerUp: null,
      shotiumFirst: null,
      engines: [],
    };
  }
  const rows = rankPlatform(summary);
  const ranks = assignRanks(rows);
  const comparable = rows[0]?.totalBaselines ?? 0;
  const officialCount = rows.filter((row) => row.official).length;
  const manifestQuality = manifest?.quality_status;
  const qualityPassed = (manifestQuality === undefined ? summary.status === 'pass' :
    ['pass', 'noisy'].includes(manifestQuality)) && summary.shards_complete !== false &&
    manifest?.missing !== true && manifest?.shards_complete !== false &&
    manifest?.evidence_complete !== false;
  const ranked = qualityPassed && officialCount >= 2;
  const competitors = summary.engines.filter((engine) => engine.engine !== 'shotium');
  const competitorAvailable = competitors.some((engine) => engine.status !== 'n/a');
  let noRankingReason: NoRankingReason | null = null;
  if (!ranked) {
    if (!qualityPassed) noRankingReason = 'quality';
    else if (!competitorAvailable) noRankingReason = 'no-competitor';
    else if (comparable === 0) noRankingReason = 'no-comparable';
    else noRankingReason = 'partial-coverage';
  }
  const officialCompetitors = rows.filter((row) =>
    row.official && row.engine !== 'shotium' && row.geometricMean !== null);
  const runnerUp = ranked ? (officialCompetitors[0] ?? null) : null;
  const shotiumRow = rows.find((row) => row.engine === 'shotium');
  const shotiumFirst = ranked && shotiumRow?.official
    ? officialCompetitors.every((row) => (row.geometricMean as number) >= 1 - EPSILON)
    : ranked ? false : null;
  return {
    id: data.id,
    data,
    manifest,
    status: summary.status,
    rows,
    ranks,
    comparable,
    officialCount,
    ranked,
    noRankingReason,
    runnerUp,
    shotiumFirst,
    engines: orderEngines([
      ...summary.engines.map((engine) => engine.engine),
      ...summary.scenarios.map((scenario) => scenario.engine),
    ]),
  };
}

export function buildVerdict(platforms: PlatformReport[]): RunVerdict {
  const tested = platforms.filter((platform) => platform.data.summary !== null).length;
  const ranked = platforms.filter((platform) => platform.ranked);
  const unranked = platforms.filter((platform) => !platform.ranked);
  let ratioMin: number | null = null;
  let ratioMax: number | null = null;
  let closest: RunVerdict['closest'] = null;
  for (const platform of ranked) {
    for (const row of platform.rows) {
      if (!row.official || row.engine === 'shotium' || row.geometricMean === null) continue;
      const ratio = row.geometricMean;
      if (ratioMin === null || ratio < ratioMin) {
        ratioMin = ratio;
        closest = {platform: platform.id, engine: row.engine, ratio};
      }
      if (ratioMax === null || ratio > ratioMax) ratioMax = ratio;
    }
  }
  return {
    tested,
    ranked,
    unranked,
    shotiumFirst: ranked.filter((platform) => platform.shotiumFirst).length,
    ratioMin,
    ratioMax,
    closest,
  };
}

export function buildRunReport(run: LoadedRun): RunReport {
  const manifestById = new Map(run.manifest.platforms.map((platform) => [platform.platform, platform]));
  const platforms = PLATFORM_IDS.map((id) =>
    buildPlatformReport(run.platforms[id], manifestById.get(id) ?? null));
  const byId = Object.fromEntries(platforms.map((platform) => [platform.id, platform])) as
    Record<PlatformId, PlatformReport>;
  return {run, platforms, byId, verdict: buildVerdict(platforms)};
}

/** What the overview matrix shows for one engine on one platform. */
export type MatrixCell =
  | {kind: 'baseline'}
  | {kind: 'official'; ratio: number; rank: number; runnerUp: boolean}
  | {kind: 'reference'; ratio: number; compared: number; total: number}
  | {kind: 'unavailable'; reason: string | null}
  | {kind: 'none'; code: string | null};

export function matrixCell(platform: PlatformReport, engine: string): MatrixCell {
  const row = platform.rows.find((candidate) => candidate.engine === engine);
  if (!row) return {kind: 'none', code: null};
  if (engine === 'shotium') return {kind: 'baseline'};
  if (row.status === 'n/a') {
    return {kind: 'unavailable', reason: row.exclusions.find((e) => e.code === 'engine-unavailable')?.detail ?? null};
  }
  if (row.geometricMean === null) return {kind: 'none', code: row.exclusions[0]?.code ?? null};
  if (row.official && platform.ranked) {
    return {
      kind: 'official',
      ratio: row.geometricMean,
      rank: platform.ranks.get(engine) ?? 0,
      runnerUp: platform.runnerUp?.engine === engine,
    };
  }
  return {kind: 'reference', ratio: row.geometricMean, compared: row.compared, total: row.totalBaselines};
}

/** Largest ratio drawn anywhere in the matrix, for one shared bar scale. */
export function matrixMaxRatio(platforms: PlatformReport[]): number {
  let max = 1;
  for (const platform of platforms) {
    for (const row of platform.rows) {
      if (row.geometricMean !== null && row.geometricMean > max) max = row.geometricMean;
    }
  }
  return max;
}
