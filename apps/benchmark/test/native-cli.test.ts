import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import {parseNativeOptions, sameMachineRatios} from '../src/native-cli.ts';

test('native CLI parses required paths and bounded iteration counts', () => {
  const options = parseNativeOptions([
    '--baseline-executable', 'baseline.exe',
    '--baseline-engine=system-chrome',
    '--shot-executable', 'shot.exe',
    '--iterations', '7',
    '--warmup-iterations=2',
    '--output', 'native-out',
  ], 'C:\\work');
  assert.equal(options.baselineEngine, 'system-chrome');
  assert.equal(options.baselineExecutable, path.resolve('C:\\work', 'baseline.exe'));
  assert.equal(options.shotExecutable, path.resolve('C:\\work', 'shot.exe'));
  assert.equal(options.iterations, 7);
  assert.equal(options.warmupIterations, 2);
  assert.equal(options.output, path.resolve('C:\\work', 'native-out'));
});

test('native CLI rejects missing, unknown and invalid options', () => {
  assert.throws(() => parseNativeOptions([]), /baseline-executable is required/);
  assert.throws(() => parseNativeOptions([
    '--baseline-executable', 'baseline', '--shot-executable', 'shot',
    '--baseline-engine', 'other',
  ]), /headless-shell or system-chrome/);
  assert.throws(() => parseNativeOptions([
    '--baseline-executable', 'baseline', '--shot-executable', 'shot',
    '--iterations', '0',
  ]), /iterations must be an integer from 1 to 1000/);
  assert.throws(() => parseNativeOptions(['--surprise']), /unknown option/);
});

test('same-machine ratios use baseline p50 divided by Shot p50', () => {
  const ratios = sameMachineRatios([
    {engine: 'baseline', case: 'simple', wall_time_ms: {p50: 40}, peak_rss_bytes: {p50: 200}},
    {engine: 'shot', case: 'simple', wall_time_ms: {p50: 10}, peak_rss_bytes: {p50: 100}},
  ], 'baseline', 'shot');
  assert.deepEqual(ratios, [{
    case: 'simple',
    baseline_wall_p50_ms: 40,
    shot_wall_p50_ms: 10,
    baseline_to_shot_wall_p50_ratio: 4,
    baseline_peak_rss_p50_bytes: 200,
    shot_peak_rss_p50_bytes: 100,
    baseline_to_shot_peak_rss_p50_ratio: 2,
  }]);
});
