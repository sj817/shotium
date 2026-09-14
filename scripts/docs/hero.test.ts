import assert from 'node:assert/strict';
import test from 'node:test';

import {bytes, type Cell, figures, multiplier, type PlatformSummary, render} from './hero.ts';

function cell(scenario: string, engine: string, overrides: Partial<Cell> = {}): Cell {
  return {
    engine,
    scenario,
    concurrency: scenario === 'cold' ? 1 : 4,
    status: 'pass',
    ranking_eligible: true,
    shots: scenario === 'cold' ? 7 : 1000,
    wall_time_ms: scenario === 'cold' ? {n: 7, p50: engine === 'shotium' ? 56 : 813, max: 813} : {n: 1, p50: 36_349, max: 36_349},
    peak_rss_bytes: scenario === 'cold' ? null : {n: 1, p50: engine === 'shotium' ? 344_207_763 : 3_104_709_989, max: engine === 'shotium' ? 344_207_763 : 3_104_709_989},
    ...overrides,
  };
}

// linux-x64 in the v0.8.0 archive, reduced to what the cards read.
function summary(overrides: Partial<PlatformSummary> = {}): PlatformSummary {
  return {
    platform: 'linux-x64',
    status: 'pass',
    shotium_version: '0.8.0',
    packages: {puppeteer: '25.8.0', playwright: '1.62.1'},
    engines: [
      {engine: 'shotium', status: 'pass', binary_version: '0.8.0'},
      {engine: 'playwright-chrome', status: 'pass', binary_version: 'Chromium 1234'},
      {engine: 'puppeteer-chrome', status: 'pass', binary_version: 'Google Chrome for Testing 152.0.7977.42'},
    ],
    scenarios: [
      cell('cold', 'shotium'),
      cell('cold', 'playwright-chrome'),
      cell('soak', 'shotium'),
      cell('soak', 'puppeteer-chrome'),
    ],
    ...overrides,
  };
}

test('the figures come from the cold-start and soak cells, comparing cold latency and memory peak', () => {
  const f = figures(summary());
  assert.equal(f.platform, 'linux-x64');
  assert.equal(f.shotium, '0.8.0');
  assert.equal(f.cold.ours_ms, 56);
  assert.equal(f.cold.theirs_ms, 813);
  assert.equal(f.cold.multiplier, '14.5×');
  assert.equal(f.memory.ours_bytes, 344_207_763);
  assert.equal(f.memory.theirs_bytes, 3_104_709_989);
  assert.equal(f.memory.multiplier, '9.0×');
});

test('a cell the archive does not trust is refused, not drawn', () => {
  const noisy = summary({scenarios: [cell('cold', 'shotium'), cell('cold', 'playwright-chrome', {status: 'noisy', ranking_eligible: false, shots: 0})]});
  assert.throws(() => figures(noisy), /playwright-chrome is noisy, not ranking-eligible \(0 shots\)/);

  const failed = summary({scenarios: [cell('cold', 'shotium'), cell('cold', 'playwright-chrome', {status: 'fail', ranking_eligible: false, shots: 832})]});
  assert.throws(() => figures(failed), /is fail/);

  const missing = summary({scenarios: [cell('cold', 'shotium')]});
  assert.throws(() => figures(missing), /no cold cell for playwright-chrome/);
});

test('the multiplier keeps one decimal and the values keep their precision', () => {
  assert.equal(multiplier(813, 56), '14.5×');
  assert.equal(multiplier(3_104_709_989, 344_207_763), '9.0×');
  assert.equal(multiplier(9.99, 1), '10.0×');
  assert.equal(bytes(344_207_763), '344 MB');
  assert.equal(bytes(3_104_709_989), '3.1 GB');
});

test('both cards carry the numbers, the multipliers and the source line', () => {
  const f = figures(summary());
  const en = render(f, 'en');
  for (const expected of ['Cold start', '14.5×', 'faster', '56 ms', '813 ms', 'Memory peak', '9.0×', 'less memory', '344 MB', '3.1 GB',
    'shotium 0.8.0 vs Playwright + Chromium &amp; Puppeteer + Chrome', 'linux-x64']) {
    assert.ok(en.includes(expected), `English card lacks ${expected}`);
  }
  const zh = render(f, 'zh');
  for (const expected of ['冷启动延迟', '14.5×', '更快', '56 ms', '813 ms', '内存峰值', '9.0×', '更省', '344 MB', '3.1 GB', 'linux-x64']) {
    assert.ok(zh.includes(expected), `Chinese card lacks ${expected}`);
  }
});
