import assert from 'node:assert/strict';
import test from 'node:test';

import {detectLinuxLibc, packageName} from '../../apps/typescript/src/lib/platform.ts';
import {performancePlatforms, platformByLabel, platforms, selectPlatforms} from './platforms.ts';

test('the native matrix has eight unique artifact and npm identities', () => {
  assert.equal(platforms.length, 8);
  assert.equal(new Set(platforms.map((p) => p.label)).size, 8);
  assert.equal(new Set(platforms.map((p) => p.npm)).size, 8);
  assert.deepEqual(performancePlatforms.map((p) => p.label), [
    'windows-amd64', 'windows-arm64', 'linux-amd64', 'linux-arm64', 'macos-amd64', 'macos-arm64',
  ]);
  assert.deepEqual(platforms.filter((p) => p.os === 'linux').map((p) => [p.label, p.libc, p.npm]), [
    ['linux-amd64', 'glibc', 'linux-x64'],
    ['linux-arm64', 'glibc', 'linux-arm64'],
    ['linux-amd64-musl', 'musl', 'linux-x64-musl'],
    ['linux-arm64-musl', 'musl', 'linux-arm64-musl'],
  ]);
  assert.equal(selectPlatforms('linux-amd64-musl,macos-arm64').length, 2);
  assert.throws(() => platformByLabel('linux-x64-musl'), /unknown platform/);
});

test('Linux runtime selection distinguishes glibc and musl without a dependency', () => {
  assert.equal(detectLinuxLibc({header: {glibcVersionRuntime: '2.39'}}), 'glibc');
  assert.equal(detectLinuxLibc({header: {}}), 'musl');
  assert.equal(detectLinuxLibc(undefined), 'musl');
  assert.equal(packageName('linux', 'x64', 'glibc'), '@pixel.js/shotium-linux-x64');
  assert.equal(packageName('linux', 'x64', 'musl'), '@pixel.js/shotium-linux-x64-musl');
  assert.equal(packageName('linux', 'arm64', 'musl'), '@pixel.js/shotium-linux-arm64-musl');
  assert.equal(packageName('win32', 'x64'), '@pixel.js/shotium-win32-x64');
  assert.equal(packageName('freebsd', 'x64'), null);
});
