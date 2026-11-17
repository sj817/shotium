import assert from 'node:assert/strict';
import {mkdirSync, mkdtempSync, realpathSync, rmSync, symlinkSync, utimesSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import {HostInputMtimes, hostInputs, InputMtimes} from './input-mtimes.ts';

test('unchanged downloads survive DEPS edits; changed bytes invalidate even an older commit', () => {
  const dir = mkdtempSync(path.join(os.tmpdir(), 'shot-mtimes-'));
  try {
    const file = path.join(dir, 'compiler');
    const state = path.join(dir, 'state.json');
    const log = path.join(dir, '.ninja_log');
    writeFileSync(file, 'original compiler');
    const initial = new InputMtimes(state, 100, 100);
    assert.equal(initial.time(file, 'compiler'), 100);
    initial.save();
    writeFileSync(log, 'cached build');
    utimesSync(log, 300, 300);
    const warm = new InputMtimes(state, 200, 200);
    assert.equal(warm.time(file, 'compiler'), 100);
    warm.save();
    writeFileSync(file, 'different compiler');
    const changed = new InputMtimes(state, 50, 50);
    assert.equal(changed.time(file, 'compiler'), 301);
    const otherShard = new InputMtimes(state, 50, 50);
    assert.equal(otherShard.time(file, 'compiler'), 301);
    changed.save();
    const next = new InputMtimes(state, 50, 50);
    assert.equal(next.time(file, 'compiler'), 301);
    writeFileSync(path.join(dir, 'new-input'), 'newly downloaded');
    assert.equal(next.time(path.join(dir, 'new-input'), 'new-input'), 301);
  } finally { rmSync(dir, {recursive: true, force: true}); }
});

test('host toolchain files: unchanged bytes get the cached time back, changed bytes a newer one, unseen ones keep theirs', () => {
  const dir = mkdtempSync(path.join(os.tmpdir(), 'shot-host-mtimes-'));
  try {
    const build = path.join(dir, 'build');
    const header = path.join(dir, 'header.h');
    mkdirSync(build);
    writeFileSync(header, 'int f();');
    utimesSync(header, 100, 100);
    const first = new HostInputMtimes(build, 50);
    assert.equal(first.time(header, 'C:/sdk/header.h'), null, 'no state: ninja compares the real time, as before');
    first.save();
    writeFileSync(path.join(build, '.ninja_log'), 'cached build');
    utimesSync(path.join(build, '.ninja_log'), 300, 300);
    // A new runner image: same bytes, a newer time.
    utimesSync(header, 900, 900);
    const refreshed = new HostInputMtimes(build, 50);
    assert.equal(refreshed.time(header, 'C:/sdk/header.h'), 100);
    refreshed.save();
    writeFileSync(header, 'int f(int);');
    utimesSync(header, 10, 10);
    const changed = new HostInputMtimes(build, 50);
    assert.equal(changed.time(header, 'C:/sdk/header.h'), 301, 'changed bytes are newer than the cached log, whatever the image says');
  } finally { rmSync(dir, {recursive: true, force: true}); }
});

test('host inputs: only graph paths that resolve outside the workspace, through directory links too', () => {
  const dir = mkdtempSync(path.join(os.tmpdir(), 'shot-host-inputs-'));
  try {
    const workspace = path.join(dir, 'ws');
    const build = path.join(workspace, 'src', 'out', 'Shot');
    const sdk = path.join(dir, 'Xcode.app', 'SDKs', 'MacOSX.sdk');
    mkdirSync(path.join(build, 'gen'), {recursive: true});
    mkdirSync(path.join(build, 'sdk', 'xcode_links'), {recursive: true});
    mkdirSync(sdk, {recursive: true});
    writeFileSync(path.join(sdk, 'SDKSettings.json'), '{}');
    writeFileSync(path.join(build, 'gen', 'generated.h'), '');
    writeFileSync(path.join(dir, 'msvc.h'), '');
    symlinkSync(sdk, path.join(build, 'sdk', 'xcode_links', 'MacOSX.sdk'), 'junction');
    // ninja writes absolute paths with forward slashes on Windows too.
    const msvc = path.join(dir, 'msvc.h').split(path.sep).join('/');
    const found = hostInputs(build, workspace, [
      '../../shot/a.cc', 'gen/generated.h', 'obj/a.o',
      'sdk/xcode_links/MacOSX.sdk/SDKSettings.json',
      msvc,
      path.join(dir, 'gone.h'),
    ]);
    assert.deepEqual(found.map(([key]) => key), ['sdk/xcode_links/MacOSX.sdk/SDKSettings.json', msvc]);
    assert.equal(realpathSync.native(found[0][1]), realpathSync.native(path.join(sdk, 'SDKSettings.json')));
  } finally { rmSync(dir, {recursive: true, force: true}); }
});
