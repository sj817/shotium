import {describe, expect, it} from 'vitest';
import {geometricMean, rankPlatform} from '../docs/lib/ranking';
import type {PlatformSummary, ResultStatus, ScenarioSummary} from '../docs/lib/types';

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

function summary(scenarios: ScenarioSummary[]): PlatformSummary {
  return {
    schema_version: 2,
    generated_utc: '2026-08-30T00:00:00Z',
    platform: 'linux-x64',
    status: 'pass',
    shotium_version: '0.3.2',
    profile: 'full',
    seed: 'test',
    source_revision: 'abc',
    engines: [
      {engine: 'shotium', status: 'pass', reason: null, executable: '/shot', architectures: ['x64']},
      {engine: 'fast', status: 'pass', reason: null, executable: '/fast', architectures: ['x64']},
      {engine: 'partial', status: 'noisy', reason: null, executable: '/partial', architectures: ['x64']},
      {engine: 'missing', status: 'n/a', reason: 'no native browser', executable: null, architectures: []},
    ],
    scenarios,
    raw_samples: scenarios.length,
    failures: 0,
  };
}

describe('platform ranking', () => {
  it('uses the geometric mean of same-platform ratios', () => {
    expect(geometricMean([0.5, 2])).toBeCloseTo(1);
    const rows = rankPlatform(summary([
      scenario('shotium', 'warm', 10),
      scenario('shotium', 'batch', 40),
      scenario('fast', 'warm', 5),
      scenario('fast', 'batch', 20),
    ]));
    const fast = rows.find((row) => row.engine === 'fast');
    expect(fast?.geometricMean).toBeCloseTo(0.5);
    expect(fast?.official).toBe(true);
    expect(fast?.compared).toBe(2);
    expect(fast?.champions).toBe(2);
  });

  it('excludes noisy cells and unavailable engines without hiding the reason', () => {
    const rows = rankPlatform(summary([
      scenario('shotium', 'warm', 10),
      scenario('shotium', 'batch', 20),
      scenario('partial', 'warm', 8),
      scenario('partial', 'batch', 18, {status: 'noisy', eligible: false}),
      scenario('fast', 'warm', 11),
      scenario('fast', 'batch', 21),
    ]));
    const partial = rows.find((row) => row.engine === 'partial');
    const missing = rows.find((row) => row.engine === 'missing');
    expect(partial).toMatchObject({compared: 1, excluded: 1, official: false});
    expect(partial?.exclusions).toContainEqual({code: 'invalid-cell', count: 1});
    expect(partial?.exclusions).toContainEqual({code: 'partial-coverage', count: 1});
    expect(missing?.geometricMean).toBeNull();
    expect(missing?.exclusions[0]).toMatchObject({
      code: 'engine-unavailable', detail: 'no native browser',
    });
  });

  it('ranks only complete coverage and keeps a faster partial score as reference', () => {
    const rows = rankPlatform(summary([
      scenario('shotium', 'warm', 10),
      scenario('shotium', 'batch', 20),
      scenario('fast', 'warm', 12),
      scenario('fast', 'batch', 24),
      scenario('partial', 'warm', 5),
    ]));
    const partial = rows.find((row) => row.engine === 'partial');
    const fast = rows.find((row) => row.engine === 'fast');
    expect(partial?.geometricMean).toBeCloseTo(0.5);
    expect(partial?.official).toBe(false);
    expect(fast).toMatchObject({official: true, compared: 2, totalBaselines: 2});
    expect(rows.indexOf(fast!)).toBeLessThan(rows.indexOf(partial!));
  });

  it('counts tied scenario champions for every tied engine', () => {
    const rows = rankPlatform(summary([
      scenario('shotium', 'warm', 10),
      scenario('fast', 'warm', 10),
    ]));
    expect(rows.find((row) => row.engine === 'shotium')?.champions).toBe(1);
    expect(rows.find((row) => row.engine === 'fast')?.champions).toBe(1);
  });
});
