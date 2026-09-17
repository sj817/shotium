import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {copyFileSync, mkdtempSync, readFileSync, rmSync, statSync, utimesSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test, {type TestContext} from 'node:test';

import {createSourceMtimes} from './input-mtimes.ts';

const sourcePath = 'src/third_party/blink/renderer/platform/web_test_support.cc';

function fixture(t: TestContext): string {
  const dir = mkdtempSync(path.join(os.tmpdir(), 'shot-source-mtimes-'));
  t.after(() => rmSync(dir, {recursive: true, force: true}));
  writeFileSync(path.join(dir, 'source.cc'), 'int value() { return 1; }');
  return dir;
}

function stampLog(dir: string, when: number): void {
  const file = path.join(dir, '.ninja_log');
  writeFileSync(file, '# ninja log v6\n');
  utimesSync(file, when, when);
}

test('source mtimes: a cold build uses the revision time', (t) => {
  const dir = fixture(t);
  assert.equal(createSourceMtimes(dir, 1000).time(path.join(dir, 'source.cc'), sourcePath), 1000);
});

test('source mtimes: legacy ordinary C++ sources invalidate newer cached objects', (t) => {
  const dir = fixture(t);
  stampLog(dir, 2000);
  const times = createSourceMtimes(dir, 1000);
  assert.equal(times.time(path.join(dir, 'source.cc'), sourcePath), 2001);
  times.save();
  assert.equal(createSourceMtimes(dir, 1000).time(path.join(dir, 'source.cc'), sourcePath), 2001);
});

test('source mtimes: unchanged bytes survive a newer checkout and build log', (t) => {
  const dir = fixture(t);
  stampLog(dir, 2000);
  const first = createSourceMtimes(dir, 1000);
  const time = first.time(path.join(dir, 'source.cc'), sourcePath);
  first.save();
  stampLog(dir, 3000);
  assert.equal(createSourceMtimes(dir, 4000).time(path.join(dir, 'source.cc'), sourcePath), time);
});

test('source mtimes: older branch content and a subsequent rollback both invalidate', (t) => {
  const dir = fixture(t);
  const file = path.join(dir, 'source.cc');
  const original = readFileSync(file);
  stampLog(dir, 2000);
  const first = createSourceMtimes(dir, 1000);
  first.time(file, sourcePath);
  first.save();
  stampLog(dir, 3000);
  writeFileSync(file, 'int value() { return 2; }');
  const branch = createSourceMtimes(dir, 500);
  assert.equal(branch.time(file, sourcePath), 3001);
  branch.save();
  stampLog(dir, 4000);
  writeFileSync(file, original);
  assert.equal(createSourceMtimes(dir, 1000).time(file, sourcePath), 4001);
});

test('source mtimes: newly tracked and reintroduced paths cannot reuse old dates', (t) => {
  const dir = fixture(t);
  const file = path.join(dir, 'source.cc');
  const first = createSourceMtimes(dir, 1000);
  first.time(file, sourcePath);
  first.save();
  stampLog(dir, 2000);
  const added = createSourceMtimes(dir, 500);
  assert.equal(added.time(file, 'src/new.cc'), 2001);
  added.save(); // sourcePath is no longer in the checkout.
  stampLog(dir, 3000);
  assert.equal(createSourceMtimes(dir, 1000).time(file, sourcePath), 3001);
});

test('source mtimes: DEPS source bytes change independently of their revision date', (t) => {
  const dir = fixture(t);
  const file = path.join(dir, 'source.cc');
  const key = 'src/third_party/freetype/src/base.c';
  const first = createSourceMtimes(dir, 1000);
  first.time(file, key);
  first.save();
  stampLog(dir, 2000);
  writeFileSync(file, 'int value() { return 3; }');
  assert.equal(createSourceMtimes(dir, 500).time(file, key), 2001);
});

test('source mtimes: workers restoring one cache derive identical changed timestamps', (t) => {
  const a = fixture(t);
  const b = fixture(t);
  const first = createSourceMtimes(a, 1000);
  first.time(path.join(a, 'source.cc'), sourcePath);
  first.save();
  copyFileSync(path.join(a, 'ci-source-inputs.json'), path.join(b, 'ci-source-inputs.json'));
  for (const dir of [a, b]) {
    stampLog(dir, 2000);
    writeFileSync(path.join(dir, 'source.cc'), 'int value() { return 4; }');
  }
  assert.equal(
    createSourceMtimes(a, 500).time(path.join(a, 'source.cc'), sourcePath),
    createSourceMtimes(b, 500).time(path.join(b, 'source.cc'), sourcePath),
  );
});

const compiler = [process.env.CXX, 'clang++', 'c++'].find((command) => command
  && spawnSync(command, ['--version'], {encoding: 'utf8'}).status === 0);
const hasNinja = spawnSync('ninja', ['--version'], {encoding: 'utf8'}).status === 0;

test('source mtimes: real ninja/C++ reproduces and repairs the missing WebTestSupport symbol',
  {skip: !compiler || !hasNinja}, (t) => {
    const dir = fixture(t);
    const provider = path.join(dir, 'provider.cc');
    const app = process.platform === 'win32' ? 'app.exe' : 'app';
    const declaration = 'namespace blink { class WebTestSupport { public: static bool CanRegisterUkmRecorderDelegateForWebTest(); }; }\n';
    writeFileSync(provider, declaration + 'int old_symbol() { return 0; }\n');
    writeFileSync(path.join(dir, 'caller.cc'), declaration
      + 'int main() { return blink::WebTestSupport::CanRegisterUkmRecorderDelegateForWebTest() ? 0 : 1; }\n');
    const cxx = compiler!.replace(/\\/g, '/').replace(/\$/g, '$$');
    writeFileSync(path.join(dir, 'build.ninja'), `rule cxx
  command = "${cxx}" -O0 -c $in -o $out
rule link
  command = "${cxx}" $in -o $out
build provider.o: cxx provider.cc
build caller.o: cxx caller.cc
build ${app}: link provider.o caller.o
`);
    const ninja = (target: string) => spawnSync('ninja', [target], {cwd: dir, encoding: 'utf8'});
    const old = ninja('provider.o');
    assert.equal(old.status, 0, old.stdout + old.stderr);
    writeFileSync(provider, declaration
      + 'bool blink::WebTestSupport::CanRegisterUkmRecorderDelegateForWebTest() { return true; }\n');
    utimesSync(provider, 1000, 1000); // The old Git-date policy hides the change.
    const stale = ninja(app);
    assert.notEqual(stale.status, 0);
    assert.match(stale.stdout + stale.stderr, /CanRegisterUkmRecorderDelegateForWebTest/);

    const times = createSourceMtimes(dir, 1000);
    const when = times.time(provider, sourcePath);
    assert.ok(when > statSync(path.join(dir, '.ninja_log')).mtimeMs / 1000);
    utimesSync(provider, when, when);
    times.save();
    const rebuilt = ninja(app);
    assert.equal(rebuilt.status, 0, rebuilt.stdout + rebuilt.stderr);
    assert.equal(spawnSync(path.join(dir, app)).status, 0);
  });
