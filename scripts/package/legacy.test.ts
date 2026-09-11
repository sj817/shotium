import assert from 'node:assert/strict';
import {mkdtemp, readFile, readdir, rm} from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import test, {type TestContext} from 'node:test';

import {LEGACY, PRIMARY, type PrimaryManifest, shimEntry, shimManifest, writeShim} from './legacy.ts';

// The shape apps/typescript/package.json has today, reduced to what the shim reads.
function primary(overrides: Partial<PrimaryManifest> = {}): PrimaryManifest {
  return {
    name: PRIMARY,
    version: '0.7.3',
    keywords: ['screenshot', 'chromium'],
    homepage: 'https://github.com/sj817/shotium#readme',
    bugs: 'https://github.com/sj817/shotium/issues',
    repository: {type: 'git', url: 'git+https://github.com/sj817/shotium.git', directory: 'apps/typescript'},
    license: 'BSD-3-Clause',
    engines: {node: '>=18'},
    exports: {'.': {types: './dist/index.d.ts', default: './dist/index.js'}, './package.json': './package.json'},
    ...overrides,
  };
}

async function temporary(t: TestContext): Promise<string> {
  const directory = await mkdtemp(path.join(os.tmpdir(), 'shotium-legacy-test-'));
  t.after(async () => {
    assert.equal(path.dirname(path.resolve(directory)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(directory).startsWith('shotium-legacy-test-'));
    await rm(directory, {recursive: true, force: true});
  });
  return directory;
}

test('the shim pins the primary at exactly its version and carries its provenance fields', () => {
  const source = primary();
  const shim = shimManifest(source);
  assert.equal(shim.name, LEGACY);
  assert.equal(shim.version, '0.7.3');
  assert.deepEqual(shim.dependencies, {[PRIMARY]: '0.7.3'});
  assert.equal(shim.optionalDependencies, undefined);
  assert.deepEqual(shim.repository, source.repository);
  assert.equal(shim.license, source.license);
  assert.deepEqual(shim.engines, source.engines);
  assert.deepEqual(shim.exports, {
    '.': {types: './index.d.ts', default: './index.js'},
    './package.json': './package.json',
  });
  assert.deepEqual(shim.files, ['index.js', 'index.d.ts', 'README.md']);
  assert.equal(shim.bin, undefined);
});

test('a subpath the shim does not mirror is refused rather than dropped', () => {
  const source = primary({exports: {'.': {}, './package.json': './package.json', './daemon': './dist/daemon.js'}});
  assert.throws(() => shimManifest(source), /\.\/daemon/);
});

test('a manifest that is not the primary is refused', () => {
  assert.throws(() => shimManifest(primary({name: '@shotkit/shotium'})), /mirrors @pixel\.js\/shotium/);
});

test('the entry forwards the named exports and the default', () => {
  const entry = shimEntry();
  assert.match(entry, /^export \* from '@pixel\.js\/shotium';$/m);
  assert.match(entry, /^export \{default\} from '@pixel\.js\/shotium';$/m);
});

test('writeShim produces exactly the four files the manifest lists, and nothing stale', async t => {
  const directory = await temporary(t);
  const dest = path.join(directory, 'shotium');
  await writeShim(dest, primary({version: '0.7.4'}));
  // A second run replaces the first: the directory is reset, not appended to.
  const files = writeShim(dest, primary({version: '0.7.4'}));
  assert.deepEqual((await readdir(dest)).sort(), [...files].sort());
  const manifest = JSON.parse(await readFile(path.join(dest, 'package.json'), 'utf8')) as {files: string[]; version: string};
  assert.deepEqual([...manifest.files, 'package.json'].sort(), (await readdir(dest)).sort());
  assert.equal(manifest.version, '0.7.4');
  assert.equal(await readFile(path.join(dest, 'index.js'), 'utf8'), await readFile(path.join(dest, 'index.d.ts'), 'utf8'));
  const readme = await readFile(path.join(dest, 'README.md'), 'utf8');
  assert.match(readme, /@pixel\.js\/shotium@0\.7\.4/);
  assert.match(readme, /npm uninstall @shotkit\/shotium/);
});
