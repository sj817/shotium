import assert from 'node:assert/strict';
import test from 'node:test';
import {outputs, select, shardCount} from './select-shards.ts';

test('cold defaults retain platform concurrency, explicit input is bounded', () => {
  assert.deepEqual(['windows', 'linux', 'macos'].flatMap((p) => ['x64', 'arm64'].map((c) => shardCount('auto', p, c))), [4, 4, 4, 3, 2, 3]);
  assert.equal(shardCount('1', 'windows', 'x64'), 1);
  for (const value of ['0', '21', '1.5', '-1', 'abc']) assert.throws(() => shardCount(value, 'windows', 'x64'));
});

test('a build directory saved at this fingerprint means one runner; anything else keeps the default', async () => {
  const exact = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', lookup: async () => ({runId: 7, exact: true})});
  assert.deepEqual([exact.count, exact.buildDirRunId], [1, 7]);
  const warm = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', lookup: async () => ({runId: 7, exact: false})});
  assert.deepEqual([warm.count, warm.buildDirRunId], [4, 7]);
  const cold = await select({requested: 'auto', target: 'linux-arm64-musl', fingerprint: 'abc', lookup: async () => null});
  assert.deepEqual([cold.count, cold.buildDirRunId], [3, null]);
  assert.match(cold.reason, /linux-arm64-musl/);
  const forced = await select({requested: '2', target: 'windows-amd64', fingerprint: 'abc', lookup: async () => ({runId: 7, exact: true})});
  assert.deepEqual([forced.count, forced.buildDirRunId], [2, 7]);
  const broken = await select({requested: 'auto', target: 'macos-amd64', fingerprint: undefined, lookup: async () => { throw new Error('503'); }});
  assert.deepEqual([broken.count, broken.buildDirRunId], [2, null]);
  assert.match(broken.reason, /503/);
  await assert.rejects(select({requested: 'auto', target: 'plan9-x64', fingerprint: undefined, lookup: async () => null}), /unknown platform/);
});

test('the outputs name every slice and the run to restore from', () => {
  assert.equal(outputs({count: 3, buildDirRunId: 42, reason: ''}), 'count=3\nfinal=2\nlist=[0,1]\nbuild_dir_run_id=42\n');
  assert.equal(outputs({count: 1, buildDirRunId: null, reason: ''}), 'count=1\nfinal=0\nlist=[]\nbuild_dir_run_id=\n');
});
