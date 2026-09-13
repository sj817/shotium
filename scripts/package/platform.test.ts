import assert from 'node:assert/strict';
import {mkdir, mkdtemp, readFile, rm, writeFile} from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {execa} from 'execa';

import {root} from '../lib/repo.ts';

async function runPackage(args: string[]): Promise<void> {
  await execa('pnpm', ['package:platform', ...args], {cwd: root});
}

test('Linux platform packages carry an exact libc identity and keep glibc names stable', async t => {
  const temporary = await mkdtemp(path.join(os.tmpdir(), 'shotium-platform-package-'));
  t.after(async () => rm(temporary, {recursive: true, force: true}));
  const build = path.join(temporary, 'build');
  const dest = path.join(temporary, 'stage');
  const packageVersion = (JSON.parse(await readFile(
      path.join(root, 'apps/typescript/package.json'), 'utf8')) as {version: string}).version;
  await mkdir(build);
  await Promise.all([
    writeFile(path.join(build, 'shotium.node'), 'addon'),
    writeFile(path.join(build, 'shotium_data.pak'), 'data'),
    writeFile(path.join(build, 'shotium_strings.pak'), 'strings'),
  ]);

  for (const libc of ['glibc', 'musl'] as const) {
    await runPackage([
      '--build', build, '--addon', path.join(build, 'shotium.node'),
      '--os', 'linux', '--arch', 'x64', '--libc', libc, '--dest', dest,
    ]);
    const suffix = libc === 'musl' ? '-musl' : '';
    const manifest = JSON.parse(await readFile(
        path.join(dest, `shotium-linux-x64${suffix}`, 'package.json'), 'utf8')) as {
      name: string; version: string; os: string[]; cpu: string[]; libc: string[]; files: string[];
    };
    assert.equal(manifest.name, `@pixel.js/shotium-linux-x64${suffix}`);
    assert.equal(manifest.version, packageVersion);
    assert.deepEqual(manifest.os, ['linux']);
    assert.deepEqual(manifest.cpu, ['x64']);
    assert.deepEqual(manifest.libc, [libc]);
    assert.deepEqual(manifest.files, ['shotium.node', 'shotium_data.pak', 'shotium_strings.pak']);
  }

  await assert.rejects(
      runPackage(['--build', build, '--addon', path.join(build, 'shotium.node'),
        '--os', 'linux', '--arch', 'x64', '--dest', dest]),
      /--libc must be glibc or musl for Linux/);
  await assert.rejects(
      runPackage(['--build', build, '--addon', path.join(build, 'shotium.node'),
        '--os', 'win', '--arch', 'x64', '--libc', 'glibc', '--dest', dest]),
      /--libc is valid only with --os linux/);
});
