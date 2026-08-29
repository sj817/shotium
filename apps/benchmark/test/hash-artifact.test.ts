import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import {hashDirectory} from '../src/hash-artifact.ts';

test('refuses to describe missing or empty evidence as a real artifact', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-artifact-test-'));
  try {
    assert.throws(() => hashDirectory(path.join(root, 'missing')), /does not exist/);
    assert.throws(() => hashDirectory(root), /is empty/);
    fs.writeFileSync(path.join(root, 'evidence.txt'), 'evidence');
    const record = hashDirectory(root);
    assert.equal(record.files.length, 1);
    assert.match(record.sha256, /^[a-f0-9]{64}$/);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});
