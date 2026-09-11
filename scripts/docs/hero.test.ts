import assert from 'node:assert/strict';
import test from 'node:test';

import {bytes, type Cell, figures, multiplier, type PlatformSummary, render, seconds} from './hero.ts';

function cell(engine: string, overrides: Partial<Cell> = {}): Cell {
  return {
    engine,
    scenario: 'soak',
    concurrency: 4,
    status: 'pass',
    ranking_eligible: true,
    shots: 1000,
    wall_time_ms: {n: 1, p50: 41_502, max: 41_502},
    peak_rss_bytes: {n: 1, p50: 359_902_391, max: 359_902_391},
    ...overrides,
  };
}

// linux-x64 in the v0.7.2 archive, reduced to what the cards read.
function summary(overrides: Partial<PlatformSummary> = {}): PlatformSummary {
  return {
    platform: 'linux-x64',
    status: 'pass',
    shotium_version: '0.7.2',
    packages: {puppeteer: '25.8.0', playwright: '1.62.1'},
    engines: [
      {engine: 'shotium', status: 'pass', binary_version: '0.7.2'},
      {engine: 'puppeteer-chrome', status: 'pass', binary_version: 'Google Chrome for Testing 152.0.7977.42'},
    ],
    scenarios: [
      cell('shotium'),
      cell('shotium', {scenario: 'warm', concurrency: 1, wall_time_ms: {n: 5, p50: 100, max: 100}}),
      cell('puppeteer-chrome', {
        wall_time_ms: {n: 1, p50: 179_800, max: 179_800},
        peak_rss_bytes: {n: 1, p50: 3_111_129_415, max: 3_111_129_415},
      }),
    ],
    ...overrides,
  };
}

test('the figures come from the two soak cells, with the versions a user would install', () => {
  const f = figures(summary(), 'puppeteer-chrome');
  assert.equal(f.platform, 'linux-x64');
  assert.equal(f.shots, 1000);
  assert.equal(f.concurrency, 4);
  assert.equal(f.competitor, 'Puppeteer 25.8.0 + Chrome 152.0.7977.42');
  assert.deepEqual(f.seconds, {ours: 41.502, theirs: 179.8});
  assert.deepEqual(f.bytes, {ours: 359_902_391, theirs: 3_111_129_415});
});

test('a cell the archive does not trust is refused, not drawn', () => {
  const noisy = summary({scenarios: [cell('shotium'), cell('puppeteer-chrome', {status: 'noisy', ranking_eligible: false, shots: 0})]});
  assert.throws(() => figures(noisy, 'puppeteer-chrome'), /puppeteer-chrome is noisy, not ranking-eligible \(0 shots\)/);

  const failed = summary({scenarios: [cell('shotium'), cell('puppeteer-chrome', {status: 'fail', ranking_eligible: false, shots: 832})]});
  assert.throws(() => figures(failed, 'puppeteer-chrome'), /is fail/);

  const missing = summary({scenarios: [cell('shotium')]});
  assert.throws(() => figures(missing, 'puppeteer-chrome'), /no soak cell for puppeteer-chrome/);

  const unequal = summary({scenarios: [cell('shotium'), cell('puppeteer-chrome', {shots: 500})]});
  assert.throws(() => figures(unequal, 'puppeteer-chrome'), /like with like/);

  assert.throws(() => figures(summary(), 'chrome'), /unknown competitor/);
});

test('the multiplier rounds down and the values keep their precision', () => {
  assert.equal(multiplier(179.8, 41.5), '4×');
  assert.equal(multiplier(3_111_129_415, 359_902_391), '8×');
  assert.equal(multiplier(9.99, 1), '9×');
  assert.equal(seconds(41.502), '41.5 s');
  assert.equal(seconds(179.8), '180 s');
  assert.equal(bytes(359_902_391), '360 MB');
  assert.equal(bytes(3_111_129_415), '3.1 GB');
});

test('both cards carry the numbers, the multipliers and the source line', () => {
  const f = figures(summary(), 'puppeteer-chrome');
  const en = render(f, 'en');
  for (const expected of ['1,000 captures', '4×', 'faster', '41.5 s', '180 s', 'Memory peak', '8×', '360 MB', '3.1 GB',
    'shotium 0.7.2 vs Puppeteer 25.8.0 + Chrome 152.0.7977.42', 'linux-x64', 'concurrency 4']) {
    assert.ok(en.includes(expected), `English card lacks ${expected}`);
  }
  const zh = render(f, 'zh');
  for (const expected of ['1000 张截图', '更快', '内存峰值', '更省', '41.5 s', '3.1 GB', '并发 4', 'linux-x64']) {
    assert.ok(zh.includes(expected), `Chinese card lacks ${expected}`);
  }
  assert.ok(!zh.includes('1,000'), 'Chinese text does not group digits');
  // The shotium bar is drawn to scale against the competitor's full bar.
  const widths = [...en.matchAll(/<rect x="58" y="\d+" width="(\d+)"/g)].map((m) => Number(m[1]));
  assert.deepEqual(widths, [Math.round(256 * 41.502 / 179.8), 256, Math.round(256 * 359_902_391 / 3_111_129_415), 256]);
});
