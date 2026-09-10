import assert from 'node:assert/strict';
import {mkdtempSync, rmSync, utimesSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import {InputMtimes} from './input-mtimes.ts';

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
