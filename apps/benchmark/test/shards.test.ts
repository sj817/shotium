import assert from 'node:assert/strict';
import test from 'node:test';
import {SHARD_IDS, SHARD_SCENARIOS} from '../src/constants.ts';

test('benchmark shards partition every scenario exactly once', () => {
  assert.deepEqual(SHARD_IDS, ['all', 'startup', 'throughput', 'resident', 'resilience']);
  assert.deepEqual(SHARD_SCENARIOS, {
    startup: ['cold', 'cold-settled', 'lifecycle'],
    throughput: ['warm', 'batch', 'parallel'],
    resident: ['resident', 'reuse-page'],
    resilience: ['faults', 'soak'],
  });
  const scenarios = Object.values(SHARD_SCENARIOS).flat();
  assert.equal(new Set(scenarios).size, scenarios.length);
  assert.deepEqual([...scenarios].sort(), [
    'batch', 'cold', 'cold-settled', 'faults', 'lifecycle',
    'parallel', 'resident', 'reuse-page', 'soak', 'warm',
  ]);
});
