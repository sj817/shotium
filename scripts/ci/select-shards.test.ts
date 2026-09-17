import assert from 'node:assert/strict';
import test from 'node:test';

import {type BuildIndex} from '../lib/build-index.ts';
import {type Candidate, type Sources, outputs, select, shardCount} from './select-shards.ts';

test('cold defaults retain platform concurrency, explicit input is bounded', () => {
  assert.deepEqual(['windows', 'linux', 'macos'].flatMap((p) => ['x64', 'arm64'].map((c) => shardCount('auto', p, c))), [3, 2, 3, 2, 2, 3]);
  assert.equal(shardCount('1', 'windows', 'x64'), 1);
  for (const value of ['0', '21', '1.5', '-1', 'abc']) assert.throws(() => shardCount(value, 'windows', 'x64'));
});

const candidate = (runId: number, extra: Partial<Candidate> = {}): Candidate =>
  ({runId, exact: false, headSha: `sha${runId}`, branch: 'main', createdAt: `2026-09-${String(runId).padStart(2, '0')}T00:00:00Z`, ...extra});

// A 60-minute cold build where document.h is read by 50 minutes of objects.
const index = (runId: number, extra: Partial<BuildIndex> = {}): BuildIndex => ({
  version: 1, platform: 'windows-amd64', head_sha: `sha${runId}`, run_id: runId, complete: true, total_ms: 3_600_000,
  paths: {'shot/a.cc': 60_000, 'third_party/blink/renderer/core/dom/document.h': 3_000_000}, ...extra,
});

// The default sources: every commit reachable, every run indexed, the
// diff is whatever the test says per commit.
function sources(candidates: Candidate[], diffs: Record<string, string[]|null>, indexes: Record<number, BuildIndex|null> = {}): Sources {
  return {
    lookup: async () => candidates,
    changes: async (sha) => sha in diffs ? diffs[sha] : [],
    index: async (runId) => runId in indexes ? indexes[runId] : index(runId),
  };
}

test('no directory, or a lookup failure, means the platform default from nothing', async () => {
  const cold = await select({requested: 'auto', target: 'linux-arm64-musl', fingerprint: 'abc', ...sources([], {})});
  assert.deepEqual([cold.count, cold.buildDirRunId], [2, null]);
  assert.match(cold.reason, /linux-arm64-musl/);
  const broken = await select({requested: 'auto', target: 'macos-amd64', fingerprint: undefined, ...sources([], {}), lookup: async () => { throw new Error('503'); }});
  assert.deepEqual([broken.count, broken.buildDirRunId], [2, null]);
  assert.match(broken.reason, /503/);
  await assert.rejects(select({requested: 'auto', target: 'plan9-x64', fingerprint: undefined, ...sources([], {})}), /unknown platform/);
});

test('a directory saved at this fingerprint means one runner, unless the count was forced', async () => {
  const exact = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7, {exact: true})], {})});
  assert.deepEqual([exact.count, exact.buildDirRunId], [1, 7]);
  const forced = await select({requested: '2', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7, {exact: true})], {})});
  assert.deepEqual([forced.count, forced.buildDirRunId], [2, 7]);
});

test('the runner count follows the priced work: a source file is one runner, a hot header the full default', async () => {
  const one = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: ['shot/a.cc']})});
  assert.deepEqual([one.count, one.buildDirRunId], [1, 7]);
  assert.match(one.reason, /1 engine input\(s\) differ, about 1\.0 of 60\.0 compile minutes/);
  const two = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: ['shot/a.cc', 'shot/new.cc', 'shot/x.cc']})});
  assert.equal(two.count, 1, '1.5 minutes is still one runner');
  const hot = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: ['shot/a.cc']}, {7: index(7, {paths: {'shot/a.cc': 400_000}})})});
  assert.equal(hot.count, 2, 'more than a tenth of the build (6 of 60 minutes) is worth a second runner');
  const all = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: ['third_party/blink/renderer/core/dom/document.h']})});
  assert.deepEqual([all.count, all.buildDirRunId], [3, 7]);
  const docs = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: []})});
  assert.equal(docs.count, 1, 'a tree whose engine inputs match a complete directory needs no compiling');
});

test('what cannot be priced keeps the default: GN changes, an incomplete directory, no index, an unreachable commit', async () => {
  const gn = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: ['shot/BUILD.gn']})});
  assert.deepEqual([gn.count, gn.buildDirRunId], [3, 7]);
  assert.match(gn.reason, /not priced \(shot\/BUILD\.gn\)/);
  const partial = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: []}, {7: index(7, {complete: false})})});
  assert.deepEqual([partial.count, partial.buildDirRunId], [3, 7]);
  assert.match(partial.reason, /incomplete/);
  const unindexed = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: []}, {7: null})});
  assert.deepEqual([unindexed.count, unindexed.buildDirRunId], [3, 7]);
  assert.match(unindexed.reason, /no build index/);
  const gone = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {sha7: null})});
  assert.deepEqual([gone.count, gone.buildDirRunId], [3, 7]);
  assert.match(gone.reason, /not reachable/);
  const failing = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources([candidate(7)], {}), changes: async () => { throw new Error('git died'); }});
  assert.deepEqual([failing.count, failing.buildDirRunId], [3, 7]);
  assert.match(failing.reason, /git died/);
});

test('the cheapest directory wins over the newest; ties go to the closer tree, then the newer one', async () => {
  // Run 9 is newest but from another branch that rewrote document.h; run 5
  // is main's, one source file away.
  const cheapest = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources(
    [candidate(9, {branch: 'feat/x'}), candidate(5)],
    {sha9: ['third_party/blink/renderer/core/dom/document.h'], sha5: ['shot/a.cc']},
  )});
  assert.deepEqual([cheapest.count, cheapest.buildDirRunId], [1, 5]);
  const closer = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources(
    [candidate(9), candidate(5)],
    {sha9: ['shot/a.cc', 'shot/unused.h', 'shot/other.h'], sha5: ['shot/a.cc']},
  )});
  assert.equal(closer.buildDirRunId, 5);
  const newer = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources(
    [candidate(5), candidate(9)], {sha9: ['shot/a.cc'], sha5: ['shot/a.cc']},
  )});
  assert.equal(newer.buildDirRunId, 9);
  // A priced directory beats an unpriced one however new the latter is.
  const priced = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources(
    [candidate(9), candidate(5)], {sha9: ['shot/BUILD.gn'], sha5: ['third_party/blink/renderer/core/dom/document.h']},
  )});
  assert.deepEqual([priced.count, priced.buildDirRunId], [3, 5]);
  // Two unpriced: the newest.
  const unpriced = await select({requested: 'auto', target: 'windows-amd64', fingerprint: 'abc', ...sources(
    [candidate(5), candidate(9)], {sha9: ['DEPS'], sha5: ['DEPS']},
  )});
  assert.equal(unpriced.buildDirRunId, 9);
});

test('the outputs name every slice and the run to restore from', () => {
  assert.equal(outputs({count: 3, buildDirRunId: 42, reason: ''}), 'count=3\nfinal=2\nlist=[0,1]\nbuild_dir_run_id=42\n');
  assert.equal(outputs({count: 1, buildDirRunId: null, reason: ''}), 'count=1\nfinal=0\nlist=[]\nbuild_dir_run_id=\n');
});
