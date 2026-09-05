'use strict';
const {test} = require('node:test');
const assert = require('node:assert/strict');
const {compare, calibrate} = require('./node_perf_gate.cjs');

// Timings the way a resident worker produces them: a stable body with a
// scattering of slow samples, deterministic so the verdicts reproduce.
function noisy(base, count = 100, spread = 0.05) {
  let seed = 0x5eed;
  const random = () => {
    seed ^= seed << 13; seed ^= seed >>> 17; seed ^= seed << 5;
    return (seed >>> 0) / 0x100000000;
  };
  return Array.from({length: count}, () => base * (1 + (random() - 0.5) * 2 * spread));
}

test('a binary timed against itself is equivalent, never slower', () => {
  // The user-facing bug this gate replaces: noise alone put one of three
  // statistics above 1, and the old gate reported that as a regression --
  // for npm 0.3.4 against npm 0.3.4, on five cases of seven.
  const a = noisy(4.2);
  const b = noisy(4.2, 100, 0.05).map((v, i) => i % 2 ? v * 1.004 : v * 0.996);
  const result = compare(a, b, {tolerance: 0.03});
  assert.equal(result.status, 'equivalent');
  assert.ok(Object.values(result.verdicts).every(v => v !== 'slower'));
});

test('a real regression is slower whatever the tolerance', () => {
  const baseline = noisy(5.3);
  const candidate = baseline.map(v => v * 2.8);
  for (const tolerance of [0, 0.03, 0.1, {primary: 0.03, tail: 0.4}]) {
    assert.equal(compare(baseline, candidate, {tolerance}).status, 'slower');
  }
});

test('a measured improvement is faster even with tail noise inside the band', () => {
  const baseline = noisy(722);
  // 17% quicker on the body, one slow tail sample the same as the baseline's.
  const candidate = baseline.map((v, i) => i === 3 ? v : v * 0.83);
  const result = compare(baseline, candidate, {tolerance: 0.03});
  assert.equal(result.status, 'faster');
  assert.equal(result.verdicts.p50, 'faster');
  assert.equal(result.verdicts.mean, 'faster');
});

test('a faster median cannot hide a slower tail or lower throughput', () => {
  const baseline = Array(100).fill(10);
  // A tail heavy enough to be certain: the median halves, the mean triples.
  const certain = compare(baseline, Array(70).fill(5).concat(Array(30).fill(100)),
                          {tolerance: 0.03});
  assert.ok(certain.ratios.p50 < 1);
  assert.ok(certain.ratios.mean > 1);
  assert.equal(certain.status, 'slower');
  // Six outliers in a hundred: the mean's direction is not settled, and an
  // unsettled mean is not a pass either way.
  const sparse = compare(baseline, Array(94).fill(5).concat(Array(6).fill(100)),
                         {tolerance: 0.03});
  assert.ok(sparse.ratios.mean > 1);
  assert.notEqual(sparse.status, 'faster');
  assert.notEqual(sparse.status, 'equivalent');
});

test('an unsettled tail does not hold up a settled median', () => {
  // A fifth quicker throughout, with six samples that landed on a scheduler
  // interrupt. p95's interval spans from well under 1 to well over it, which
  // is what a hundred-sample tail looks like on any machine; the median and
  // the mean are settled, and the case is faster.
  const baseline = noisy(10);
  const candidate = baseline.map((v, i) => i % 16 === 0 ? v * 2 : v * 0.8);
  const result = compare(baseline, candidate, {tolerance: {primary: 0.03, tail: 0.03}});
  assert.equal(result.verdicts.p50, 'faster');
  assert.equal(result.verdicts.mean, 'faster');
  assert.equal(result.verdicts.p95, 'uncertain');
  assert.equal(result.status, 'faster');
});

test('a settled mean carries a case whose median is still noisy', () => {
  // A page that loads with a wide spread of times: every sample a fifth
  // quicker, but with the spread the median's interval at a hundred samples
  // still straddles the band while the mean's does not. Throughput is up,
  // and the case is faster.
  const baseline = noisy(300, 100, 0.35);
  const candidate = baseline.map((v, i) => v * (i % 3 === 0 ? 0.55 : 0.92));
  const result = compare(baseline, candidate, {tolerance: 0.02});
  assert.equal(result.verdicts.mean, 'faster');
  assert.equal(result.status, 'faster');
  // The other way round -- a quicker median bought with a heavier tail, the
  // mean straddling its band -- is not a pass.
  const heavier = baseline.map((v, i) => i % 5 === 0 ? v * 1.6 : v * 0.9);
  const bought = compare(baseline, heavier, {tolerance: 0.02});
  assert.equal(bought.verdicts.p50, 'faster');
  assert.notEqual(bought.verdicts.mean, 'faster');
  assert.notEqual(bought.status, 'faster');
});

test('an interval that straddles the body band is unproven, not passed', () => {
  const baseline = noisy(10);
  // 1.5% slower on the body: the interval straddles a 3% band's far edge.
  const candidate = baseline.map((v, i) => v * (i % 2 ? 1.05 : 0.98));
  const result = compare(baseline, candidate, {tolerance: 0.03});
  assert.ok(['uncertain', 'equivalent'].includes(result.verdicts.p50));
  assert.ok(['uncertain', 'equivalent'].includes(result.verdicts.mean));
  assert.ok(['unproven', 'equivalent'].includes(result.status));
  assert.notEqual(result.status, 'faster');
});

test('the tail is judged against its own band', () => {
  const baseline = noisy(10);
  // The median unchanged, every seventh sample half as slow again: a tail
  // regression if the tail band is tight, noise if the calibration said the
  // tail wanders that far on this machine.
  const candidate = baseline.map((v, i) => i % 7 === 0 ? v * 1.5 : v);
  assert.equal(compare(baseline, candidate, {tolerance: {primary: 0.05, tail: 0.1}}).status, 'slower');
  const tolerated = compare(baseline, candidate, {tolerance: {primary: 0.05, tail: 0.8}});
  assert.notEqual(tolerated.status, 'slower');
  assert.notEqual(tolerated.status, 'faster');
});

test('zero tolerance is the strict gate the calibration floors at', () => {
  const baseline = noisy(10);
  const same = noisy(10, 100, 0.05).map((v, i) => i % 2 ? v * 1.003 : v * 0.997);
  const strict = compare(baseline, same, {tolerance: 0});
  assert.notEqual(strict.status, 'faster');
  assert.notEqual(strict.status, 'slower');
});

test('too few samples and invalid or unpaired results fail closed', () => {
  assert.equal(compare(Array(20).fill(10), Array(20).fill(5)).status, 'insufficient-samples');
  assert.throws(() => compare([1, 2], [1]), /Unpaired/);
  assert.throws(() => compare([1], [NaN]), /finite/);
  assert.throws(() => compare([1], [1], {tolerance: -1}), /non-negative/);
  assert.throws(() => compare([1], [1], {tolerance: {primary: 0.02}}), /non-negative/);
});

test('a consistent improvement passes reproducibly and its ratios are honest', () => {
  const baseline = Array.from({length: 100}, (_, i) => 10 + i % 7);
  const candidate = baseline.map(v => v * 0.8);
  const result = compare(baseline, candidate);
  assert.equal(result.status, 'faster');
  assert.ok(Math.abs(result.ratios.p50 - 0.8) < 1e-12);
  assert.ok(Math.abs(result.upper99 - 0.8) < 1e-12);
  assert.deepEqual(compare(baseline, candidate), result);
});

test('calibration reads each band off its own statistic and never drops below the floor', () => {
  const aa = (p50, mean, p95) => ({intervals: {p50: {hi: p50}, mean: {hi: mean}, p95: {hi: p95}}});
  const bands = calibrate([aa(1.010, 1.025, 1.10), aa(1.018, 1.041, 1.35), aa(1.005, 1.012, 1.21)]);
  assert.ok(bands.primary >= 0.02 && bands.primary <= 0.041 + 1e-12);
  assert.ok(bands.tail >= 0.21 && bands.tail <= 0.35 + 1e-12);
  assert.deepEqual(calibrate([aa(1.001, 1.001, 1.001)]), {primary: 0.02, tail: 0.02});
  assert.deepEqual(calibrate([]), {primary: 0.02, tail: 0.02});
  const floored = calibrate([aa(1.08, 1.03, 1.02)], {floor: 0.05});
  assert.ok(floored.primary > 0.07 && floored.primary <= 0.08 + 1e-12);
  assert.equal(floored.tail, 0.05);
});
