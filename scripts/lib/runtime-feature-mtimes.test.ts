import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {mkdtempSync, readFileSync, rmSync, statSync, utimesSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {createRuntimeFeatureMtimes, isRuntimeFeatureInput} from './runtime-feature-mtimes.ts';

const featurePath = 'third_party/blink/renderer/platform/runtime_enabled_features.json5';

function fixture(t: {after: (fn: () => void) => void}): string {
  const dir = mkdtempSync(path.join(os.tmpdir(), 'shot-feature-mtimes-'));
  t.after(() => rmSync(dir, {recursive: true, force: true}));
  writeFileSync(path.join(dir, 'features.json5'), '{"name":"AriaNotify"}');
  return dir;
}

function stampLog(dir: string, time: number): void {
  const log = path.join(dir, '.ninja_log');
  writeFileSync(log, '# ninja log v5\n');
  utimesSync(log, time, time);
}

test('feature definitions, overrides and generator helpers are content tracked', () => {
  for (const file of [
    featurePath,
    featurePath.replace('.json5', '.override.json5'),
    'third_party/blink/renderer/build/scripts/make_runtime_features.py',
    'third_party/blink/renderer/build/scripts/blinkbuild/name_style_converter.py',
    'third_party/blink/renderer/build/scripts/templates/runtime_enabled_features.h.tmpl',
    'third_party/blink/renderer/build/scripts/templates/macros.tmpl',
  ]) assert.ok(isRuntimeFeatureInput(file), file);
  assert.equal(isRuntimeFeatureInput('shot/shot_platform.cc'), false);
  assert.equal(isRuntimeFeatureInput('out/Shot/gen/runtime_enabled_features.h'), false);
});

test('a cold build uses the revision time', (t) => {
  const dir = fixture(t);
  const times = createRuntimeFeatureMtimes(dir, 1000);
  assert.equal(times.time(path.join(dir, 'features.json5'), featurePath), 1000);
});

test('legacy caches regenerate even when source commits are older', (t) => {
  const dir = fixture(t);
  stampLog(dir, 2000);
  const times = createRuntimeFeatureMtimes(dir, 1000);
  assert.equal(times.time(path.join(dir, 'features.json5'), featurePath), 2001);
  times.save();
  // An interrupted run must not reset the input to its old Git timestamp.
  const retry = createRuntimeFeatureMtimes(dir, 1000);
  assert.equal(retry.time(path.join(dir, 'features.json5'), featurePath), 2001);
});

test('unchanged content preserves its timestamp across warm builds', (t) => {
  const dir = fixture(t);
  stampLog(dir, 2000);
  const first = createRuntimeFeatureMtimes(dir, 1000);
  const time = first.time(path.join(dir, 'features.json5'), featurePath);
  first.save();
  stampLog(dir, 3000);
  const warm = createRuntimeFeatureMtimes(dir, 4000);
  assert.equal(warm.time(path.join(dir, 'features.json5'), featurePath), time);
});

test('switching to different content at an older revision invalidates the cache', (t) => {
  const dir = fixture(t);
  stampLog(dir, 2000);
  const first = createRuntimeFeatureMtimes(dir, 1000);
  first.time(path.join(dir, 'features.json5'), featurePath);
  first.save();
  stampLog(dir, 3000);
  writeFileSync(path.join(dir, 'features.json5'), '{"name":"AriaActions"}');
  const changed = createRuntimeFeatureMtimes(dir, 500);
  assert.equal(changed.time(path.join(dir, 'features.json5'), featurePath), 3001);
});

test('shards restoring the same cache derive identical input timestamps', (t) => {
  const a = fixture(t);
  const b = fixture(t);
  stampLog(a, 2000);
  stampLog(b, 2000);
  assert.equal(
    createRuntimeFeatureMtimes(a, 1000).time(path.join(a, 'features.json5'), featurePath),
    createRuntimeFeatureMtimes(b, 1000).time(path.join(b, 'features.json5'), featurePath),
  );
});

const hasNinja = spawnSync('ninja', ['--version'], {encoding: 'utf8'}).status === 0;

test('ninja rebuilds a stale feature header without deleting cached objects', {skip: !hasNinja}, (t) => {
  const dir = fixture(t);
  const input = path.join(dir, 'features.json5');
  const output = path.join(dir, 'features.h');
  writeFileSync(input, '{"name":"AriaActions"}');
  writeFileSync(path.join(dir, 'cached.o'), 'keep this compiled object');
  writeFileSync(path.join(dir, 'generate.mjs'), `
import {readFileSync, writeFileSync} from 'node:fs';
const feature = JSON.parse(readFileSync('features.json5', 'utf8')).name;
writeFileSync('features.h', 'static bool ' + feature + 'Enabled();\\n');
`);
  const node = process.execPath.replace(/\\/g, '/').replace(/\$/g, '$$');
  writeFileSync(path.join(dir, 'build.ninja'),
    `rule generate\n  command = "${node}" generate.mjs\n  restat = 1\nbuild features.h: generate features.json5 | generate.mjs\n`);
  const ninja = () => {
    const result = spawnSync('ninja', ['features.h'], {cwd: dir, encoding: 'utf8'});
    assert.equal(result.status, 0, result.stdout + result.stderr);
  };
  ninja();
  assert.match(readFileSync(output, 'utf8'), /AriaActionsEnabled/);
  // Reproduce Git-timestamp stamping against a newer generated cache.
  writeFileSync(input, '{"name":"AriaNotify"}');
  utimesSync(input, 1000, 1000);
  ninja();
  assert.doesNotMatch(readFileSync(output, 'utf8'), /AriaNotifyEnabled/);

  const times = createRuntimeFeatureMtimes(dir, 1000);
  const time = times.time(input, featurePath);
  assert.ok(time > statSync(path.join(dir, '.ninja_log')).mtimeMs / 1000);
  utimesSync(input, time, time);
  times.save();
  ninja();
  assert.match(readFileSync(output, 'utf8'), /AriaNotifyEnabled/);
  assert.equal(readFileSync(path.join(dir, 'cached.o'), 'utf8'), 'keep this compiled object');
});
