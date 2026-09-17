// The index prices a change without the build directory, so what it says
// about a synthetic directory is pinned exactly: which deps-log paths count
// as sources, how a header's cost accumulates over the objects that read
// it, what makes a change unpriceable, and how minutes turn into runners.
import assert from 'node:assert/strict';
import {mkdirSync, mkdtempSync, rmSync, writeFileSync} from 'node:fs';
import {tmpdir} from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {type BuildIndex, NEW_SOURCE_MS, buildIndex, estimate, parseIndex, shardsFor, sourcePath} from './build-index.ts';
import {LOG_HEADER, formatDeps, formatLog} from './ninja-state.ts';

function directory(): string {
  const dir = mkdtempSync(path.join(tmpdir(), 'build-index-'));
  // Three objects: a.o and b.o both read common.h; c.o exists in the log but
  // was deleted from disk and must not count; the link edge has no deps.
  writeFileSync(path.join(dir, '.ninja_log'), formatLog(LOG_HEADER, [
    {start: 0, end: 10_000, mtime: 1n, output: 'obj/a.o', hash: 'h1'},
    {start: 0, end: 4_000, mtime: 1n, output: 'obj/b.o', hash: 'h2'},
    {start: 0, end: 99_000, mtime: 1n, output: 'obj/c.o', hash: 'h3'},
    {start: 0, end: 30_000, mtime: 1n, output: 'shotium', hash: 'h4'},
  ]));
  writeFileSync(path.join(dir, '.ninja_deps'), formatDeps({paths: [], records: new Map([
    ['obj/a.o', {mtime: 1n, deps: ['../../shot/a.cc', '../../shot/common.h', 'gen/generated.h']}],
    ['obj/b.o', {mtime: 1n, deps: ['../../shot/b.cc', '../../shot/common.h', '../../../outside/x.h']}],
    ['obj/c.o', {mtime: 1n, deps: ['../../shot/c.cc', '../../shot/common.h']}],
  ])}));
  mkdirSync(path.join(dir, 'obj'));
  for (const file of ['obj/a.o', 'obj/b.o', 'shotium']) writeFileSync(path.join(dir, file), '');
  return dir;
}

test('deps-log paths: only the source tree counts; generated and out-of-tree paths do not', () => {
  assert.equal(sourcePath('../../third_party/blink/renderer/core/dom/document.h'), 'third_party/blink/renderer/core/dom/document.h');
  assert.equal(sourcePath('..\\..\\shot\\shot_engine.cc'), 'shot/shot_engine.cc');
  assert.equal(sourcePath('gen/third_party/blink/renderer/core/core_shot_jumbo_dom_0.cc'), null);
  assert.equal(sourcePath('../../../sdk/include/x.h'), null);
  assert.equal(sourcePath('../../out/Shot/gen/x.h'), null);
});

test('the index sums each object over the sources it read and skips outputs that are gone', () => {
  const dir = directory();
  try {
    const index = buildIndex(dir, {platform: 'linux-amd64', headSha: 'abc', runId: 7, complete: true});
    assert.deepEqual(index.paths, {'shot/a.cc': 10_000, 'shot/common.h': 14_000, 'shot/b.cc': 4_000});
    assert.equal(index.total_ms, 44_000, 'a.o + b.o + the link; the deleted c.o does not count');
    assert.deepEqual([index.version, index.platform, index.head_sha, index.run_id, index.complete], [1, 'linux-amd64', 'abc', 7, true]);
    assert.deepEqual(parseIndex(JSON.stringify(index)), index);
    assert.throws(() => parseIndex(JSON.stringify({...index, version: 2})), /version 2/);
  } finally {
    rmSync(dir, {recursive: true, force: true});
  }
});

test('an empty build directory indexes to nothing rather than failing', () => {
  const dir = mkdtempSync(path.join(tmpdir(), 'build-index-'));
  try {
    const index = buildIndex(dir, {platform: 'macos-arm64', headSha: '', runId: 0, complete: false});
    assert.deepEqual([index.total_ms, index.paths], [0, {}]);
  } finally {
    rmSync(dir, {recursive: true, force: true});
  }
});

const INDEX: BuildIndex = {
  version: 1, platform: 'linux-amd64', head_sha: 'abc', run_id: 7, complete: true, total_ms: 600_000,
  paths: {'shot/a.cc': 10_000, 'shot/common.h': 14_000, 'shot/b.cc': 4_000},
};

test('pricing: known sources add up, unknown headers are free, new sources cost one unit, the rest is unpriceable', () => {
  assert.deepEqual(estimate(INDEX, []), {ms: 0, unknown: []});
  assert.deepEqual(estimate(INDEX, ['shot/common.h', 'shot/b.cc']), {ms: 18_000, unknown: []});
  assert.deepEqual(estimate(INDEX, ['shot/never_included.h']), {ms: 0, unknown: []});
  assert.deepEqual(estimate(INDEX, ['shot/new_file.cc', 'shot/a.cc']), {ms: NEW_SOURCE_MS + 10_000, unknown: []});
  assert.deepEqual(estimate(INDEX, ['shot/BUILD.gn', 'shot/a.cc']), {ms: null, unknown: ['shot/BUILD.gn']});
  assert.deepEqual(estimate(INDEX, ['third_party/blink/renderer/platform/runtime_enabled_features.json5']).ms, null);
  assert.deepEqual(estimate(INDEX, ['DEPS']).ms, null);
  // The sum is capped at one cold build.
  assert.equal(estimate({...INDEX, total_ms: 12_000}, ['shot/common.h']).ms, 12_000);
  // A directory whose run did not link prices nothing, however small the diff.
  assert.equal(estimate({...INDEX, complete: false}, []).ms, null);
});

test('runners: one per tenth of a cold build, never below one nor above the default', () => {
  assert.equal(shardsFor(0, INDEX, 3), 1);
  assert.equal(shardsFor(1, INDEX, 3), 1);
  assert.equal(shardsFor(60_000, INDEX, 3), 1);
  assert.equal(shardsFor(60_001, INDEX, 3), 2);
  assert.equal(shardsFor(120_001, INDEX, 3), 3);
  assert.equal(shardsFor(5_000_000, INDEX, 3), 3);
  assert.equal(shardsFor(60_000, INDEX, 2), 1);
  assert.equal(shardsFor(60_001, INDEX, 2), 2);
  assert.equal(shardsFor(0, INDEX, 1), 1);
  assert.equal(shardsFor(100, {...INDEX, total_ms: 0}, 3), 3, 'a directory with no logged work cannot be priced against');
});
