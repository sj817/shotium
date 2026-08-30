import {describe, expect, it} from 'vitest';
import {assignRanks, buildPlatformReport, buildVerdict, matrixCell} from '../docs/lib/report';
import type {
  EngineSummary,
  PlatformData,
  PlatformId,
  PlatformSummary,
  ResultStatus,
  ScenarioSummary,
} from '../docs/lib/types';

function scenario(engine: string, name: string, p50: number, options: {
  status?: Exclude<ResultStatus, 'n/a'>;
  eligible?: boolean;
  concurrency?: number;
} = {}): ScenarioSummary {
  const distribution = {n: 1, min: p50, p50, p95: p50, max: p50, mean: p50, mad: 0};
  return {
    engine,
    scenario: name,
    concurrency: options.concurrency ?? 1,
    status: options.status ?? 'pass',
    ranking_eligible: options.eligible ?? true,
    runs: 1,
    shots: 1,
    wall_time_ms: distribution,
    orchestration_wall_time_ms: distribution,
    latency_ms: distribution,
    peak_rss_bytes: null,
    rss_slope_bytes_per_minute: null,
    throughput_per_second: null,
    failure_rate: null,
  };
}

function engine(name: string, status: ResultStatus = 'pass', reason: string | null = null): EngineSummary {
  return {engine: name, status, reason, executable: status === 'n/a' ? null : `/${name}`, architectures: ['x64']};
}

function summary(platform: PlatformId, engines: EngineSummary[], scenarios: ScenarioSummary[]): PlatformSummary {
  return {
    schema_version: 2,
    generated_utc: '2026-08-30T00:00:00Z',
    platform,
    status: 'pass',
    shotium_version: '0.3.3',
    profile: 'full',
    seed: 'seed',
    source_revision: 'abc',
    engines,
    scenarios,
    raw_samples: scenarios.length,
    failures: 0,
  };
}

function data(platform: PlatformId, value: PlatformSummary | null): PlatformData {
  return {id: platform, summary: value, failures: [], loadError: value ? null : '404 Not Found'};
}

const rankedLinux = data('linux-x64', summary('linux-x64', [
  engine('shotium'), engine('playwright-shell'), engine('puppeteer-shell', 'noisy'), engine('puppeteer-chrome', 'n/a', 'no native'),
], [
  scenario('shotium', 'warm', 10),
  scenario('shotium', 'batch', 20),
  scenario('playwright-shell', 'warm', 40),
  scenario('playwright-shell', 'batch', 90),
  scenario('puppeteer-shell', 'warm', 30),
  scenario('puppeteer-shell', 'batch', 60, {status: 'noisy', eligible: false}),
]));

const rankedMac = data('darwin-arm64', summary('darwin-arm64', [
  engine('shotium'), engine('playwright-shell'), engine('playwright-chrome'),
], [
  scenario('shotium', 'lifecycle', 100),
  scenario('playwright-shell', 'lifecycle', 350),
  scenario('playwright-chrome', 'lifecycle', 1500),
]));

const noCompetitor = data('win32-arm64', summary('win32-arm64', [
  engine('shotium'), engine('playwright-shell', 'n/a', 'x64 only'), engine('puppeteer-shell', 'n/a', 'none'),
], [
  scenario('shotium', 'warm', 10),
]));

const allNoisy = data('win32-x64', summary('win32-x64', [
  engine('shotium', 'noisy'), engine('playwright-shell', 'noisy'),
], [
  scenario('shotium', 'warm', 10, {status: 'noisy', eligible: false}),
  scenario('playwright-shell', 'warm', 40, {status: 'noisy', eligible: false}),
]));

const partialOnly = data('linux-arm64', summary('linux-arm64', [
  engine('shotium'), engine('playwright-shell', 'noisy'), engine('playwright-chrome', 'noisy'),
], [
  scenario('shotium', 'warm', 10),
  scenario('shotium', 'batch', 20),
  scenario('playwright-shell', 'warm', 40),
  scenario('playwright-shell', 'batch', 90, {status: 'noisy', eligible: false}),
  scenario('playwright-chrome', 'warm', 50, {status: 'noisy', eligible: false}),
  scenario('playwright-chrome', 'batch', 100),
]));

const missing = data('darwin-x64', null);

describe('platform report', () => {
  it('produces a ranking when two engines cover the whole comparable set', () => {
    const report = buildPlatformReport(rankedLinux, null);
    expect(report.comparable).toBe(2);
    expect(report.officialCount).toBe(2);
    expect(report.ranked).toBe(true);
    expect(report.noRankingReason).toBeNull();
    expect(report.shotiumFirst).toBe(true);
    expect(report.runnerUp?.engine).toBe('playwright-shell');
    expect(report.ranks.get('shotium')).toBe(1);
    expect(report.ranks.get('playwright-shell')).toBe(2);
    expect(report.ranks.has('puppeteer-shell')).toBe(false);
  });

  it('explains why a platform has no ranking', () => {
    expect(buildPlatformReport(noCompetitor, null).noRankingReason).toBe('no-competitor');
    expect(buildPlatformReport(allNoisy, null).noRankingReason).toBe('no-comparable');
    expect(buildPlatformReport(partialOnly, null).noRankingReason).toBe('partial-coverage');
    expect(buildPlatformReport(missing, null).noRankingReason).toBe('not-archived');
    expect(buildPlatformReport(missing, null).status).toBe('missing');
  });

  it('keeps reference scores on an unranked platform without a runner-up', () => {
    const report = buildPlatformReport(partialOnly, null);
    expect(report.ranked).toBe(false);
    expect(report.runnerUp).toBeNull();
    expect(report.shotiumFirst).toBeNull();
    const cell = matrixCell(report, 'playwright-shell');
    expect(cell.kind).toBe('reference');
    if (cell.kind === 'reference') expect(cell.ratio).toBeCloseTo(4);
  });

  it('shares a rank between tied engines', () => {
    const ranks = assignRanks([
      {engine: 'a', status: 'pass', geometricMean: 1, official: true, compared: 1, champions: 1, totalBaselines: 1, excluded: 0, exclusions: []},
      {engine: 'b', status: 'pass', geometricMean: 1, official: true, compared: 1, champions: 1, totalBaselines: 1, excluded: 0, exclusions: []},
      {engine: 'c', status: 'pass', geometricMean: 3, official: true, compared: 1, champions: 0, totalBaselines: 1, excluded: 0, exclusions: []},
    ]);
    expect(ranks.get('a')).toBe(1);
    expect(ranks.get('b')).toBe(1);
    expect(ranks.get('c')).toBe(3);
  });
});

describe('matrix cells', () => {
  it('classifies every engine of a ranked platform', () => {
    const report = buildPlatformReport(rankedLinux, null);
    expect(matrixCell(report, 'shotium')).toEqual({kind: 'baseline'});
    expect(matrixCell(report, 'playwright-shell')).toMatchObject({kind: 'official', rank: 2, runnerUp: true});
    expect(matrixCell(report, 'puppeteer-shell')).toMatchObject({kind: 'reference', compared: 1, total: 2});
    expect(matrixCell(report, 'puppeteer-chrome')).toEqual({kind: 'unavailable', reason: 'no native'});
    expect(matrixCell(report, 'unknown')).toEqual({kind: 'none', code: null});
  });
});

describe('run verdict', () => {
  it('counts platforms, ranked platforms, first places, the ratio range and the closest competitor', () => {
    const platforms = [rankedLinux, rankedMac, noCompetitor, allNoisy, partialOnly, missing]
      .map((platform) => buildPlatformReport(platform, null));
    const verdict = buildVerdict(platforms);
    expect(verdict.tested).toBe(5);
    expect(verdict.ranked.map((platform) => platform.id)).toEqual(['linux-x64', 'darwin-arm64']);
    expect(verdict.unranked.map((platform) => platform.id)).toEqual(['win32-arm64', 'win32-x64', 'linux-arm64', 'darwin-x64']);
    expect(verdict.shotiumFirst).toBe(2);
    expect(verdict.ratioMin).toBeCloseTo(3.5);
    expect(verdict.ratioMax).toBeCloseTo(15);
    expect(verdict.closest).toMatchObject({platform: 'darwin-arm64', engine: 'playwright-shell'});
  });

  it('ignores reference scores when computing the range', () => {
    const platforms = [rankedLinux].map((platform) => buildPlatformReport(platform, null));
    const verdict = buildVerdict(platforms);
    // puppeteer-shell's partial score (3×) must not become the minimum.
    expect(verdict.ratioMin).toBeCloseTo(Math.sqrt(4 * 4.5));
    expect(verdict.closest?.engine).toBe('playwright-shell');
  });

  it('reports no ranking at all without crashing', () => {
    const verdict = buildVerdict([buildPlatformReport(noCompetitor, null), buildPlatformReport(missing, null)]);
    expect(verdict.tested).toBe(1);
    expect(verdict.ranked).toHaveLength(0);
    expect(verdict.ratioMin).toBeNull();
    expect(verdict.closest).toBeNull();
  });
});
