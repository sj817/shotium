import assert from 'node:assert/strict';
import test from 'node:test';
import {changesNeedCompilation, shardCount} from './ci-select-shards.ts';

test('warm documentation/version changes avoid duplicate runners; unknown inputs do not', () => {
  assert.equal(changesNeedCompilation(['apps/demo/shotium/package.json', 'docs/release.md', 'apps/demo/shotium/README.md']), false);
  for (const file of ['DEPS', 'shot/BUILD.gn', 'shot/shot_renderer.cc', '.github/actions/macos-source/action.yml', 'scripts/build-engine.ts', 'third_party/blink/test.md']) {
    assert.equal(changesNeedCompilation([file]), true, file);
  }
});

test('cold defaults retain platform concurrency, explicit input is bounded', () => {
  assert.deepEqual(['windows', 'linux', 'macos'].flatMap((p) => ['x64', 'arm64'].map((c) => shardCount('auto', p, c))), [4, 4, 4, 3, 2, 3]);
  assert.equal(shardCount('1', 'windows', 'x64'), 1);
  for (const value of ['0', '21', '1.5', '-1', 'abc']) assert.throws(() => shardCount(value, 'windows', 'x64'));
});
