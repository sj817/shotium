import assert from 'node:assert/strict';
import test from 'node:test';
import {ENGINE_IDS, PROFILES, SHARD_IDS, SHARD_SCENARIOS} from '../src/constants.ts';
import {buildGroups} from '../src/groups.ts';

test('benchmark shards partition every scenario exactly once', () => {
  assert.deepEqual(SHARD_IDS, ['all', 'startup', 'throughput', 'parallel', 'resident', 'resilience']);
  assert.deepEqual(SHARD_SCENARIOS, {
    startup: ['cold', 'cold-settled', 'lifecycle'],
    throughput: ['warm', 'batch'],
    parallel: ['parallel'],
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

test('resident keeps seven client samples but pays for one warm host per engine', () => {
  const groups = buildGroups(ENGINE_IDS, PROFILES.full, {shard: 'resident', seed: 'resident-test'});
  const resident = groups.filter((group) => group.scenario === 'resident');
  const reusePage = groups.filter((group) => group.scenario === 'reuse-page');
  assert.equal(resident.length, 1);
  assert.equal(resident[0].iterations, PROFILES.full.repeats);
  assert.equal(resident[0].engines.length, ENGINE_IDS.length);
  assert.equal(reusePage.length, 1);
  assert.equal(reusePage[0].engines.includes('shotium'), false);
  assert.equal(groups.reduce((sum, group) => sum + group.engines.length, 0), 9);
});

test('steady-state batch and parallel keep every round without repeated engine launches', () => {
  const throughput = buildGroups(ENGINE_IDS, PROFILES.full, {
    shard: 'throughput', seed: 'throughput-test',
  });
  const batch = throughput.filter((group) => group.scenario === 'batch');
  assert.equal(batch.length, 1);
  assert.equal(batch[0].iterations, PROFILES.full.batchRounds);

  const parallel = buildGroups(ENGINE_IDS, PROFILES.full, {
    shard: 'parallel', seed: 'parallel-test',
  });
  assert.equal(parallel.length, PROFILES.full.concurrencies.length);
  assert.deepEqual(parallel.map((group) => group.concurrency), PROFILES.full.concurrencies);
  assert.ok(parallel.every((group) => group.iterations === PROFILES.full.batchRounds));
  assert.equal(parallel.reduce((sum, group) => sum + group.engines.length, 0), 15);
});
