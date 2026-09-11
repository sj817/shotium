import assert from 'node:assert/strict';
import {existsSync} from 'node:fs';
import {mkdir, mkdtemp, readFile, readdir, rm, writeFile} from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import test, {type TestContext} from 'node:test';
import {execa} from 'execa';
import {
  archiveMarkdown, collectNativeArchives, releaseArchives, releasePlatforms,
  sha256File, stageNativeDelivery, verifyNativeDelivery, verifyReleaseChecksums, writeReleaseChecksums,
} from '../lib/release-artifacts.ts';

async function temporary(t: TestContext): Promise<string> {
  const directory = await mkdtemp(path.join(os.tmpdir(), 'shotium-package-test-'));
  t.after(async () => {
    assert.equal(path.dirname(path.resolve(directory)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(directory).startsWith('shotium-package-test-'));
    await rm(directory, {recursive: true, force: true});
  });
  return directory;
}

async function nativeFixture(directory: string): Promise<{build: string; sourceRoot: string}> {
  const build = path.join(directory, 'build'), sourceRoot = path.join(directory, 'source');
  await mkdir(build, {recursive: true});
  for (const name of ['shotium', 'shotium.exe', 'shotium.dll', 'shotium.dll.lib',
    'libshotium.so', 'libshotium.dylib', 'shotium.node', 'shotium_data.pak', 'shotium_strings.pak']) {
    await writeFile(path.join(build, name), 'native fixture: ' + name);
  }
  for (const name of ['LICENSE', 'shot/shot_api.h', 'apps/c-abi/README.md', 'apps/c-abi/README.zh.md']) {
    await mkdir(path.dirname(path.join(sourceRoot, name)), {recursive: true});
    await writeFile(path.join(sourceRoot, name), 'source fixture: ' + name);
  }
  return {build, sourceRoot};
}

const sevenzip = process.env.SHOTIUM_SEVENZIP ||
  (process.platform === 'win32' ? 'C:/Program Files/7-Zip/7z.exe' : '7z');

test('all six native platforms: real 7z extraction preserves isolated contents and stable roots', async t => {
  const directory = await temporary(t);
  const fixture = await nativeFixture(directory);
  for (const platform of releasePlatforms) {
    const os = platform.startsWith('windows') ? 'win' : platform.startsWith('macos') ? 'mac' : 'linux';
    for (const kind of ['cli', 'c-abi', 'node'] as const) {
      const stem = 'shotium-' + kind + '-' + platform;
      const dest = path.join(directory, stem);
      const expected = kind === 'cli'
        ? [os === 'win' ? 'shotium.exe' : 'shotium', 'shotium_data.pak', 'shotium_strings.pak', 'LICENSE']
        : kind === 'node'
        ? ['shotium.node', 'shotium_data.pak', 'shotium_strings.pak', 'LICENSE']
        : [os === 'win' ? 'shotium.dll' : os === 'mac' ? 'libshotium.dylib' : 'libshotium.so',
          ...(os === 'win' ? ['shotium.dll.lib'] : []),
          'shotium_data.pak', 'shotium_strings.pak', 'shot_api.h', 'C_ABI.md', 'C_ABI.zh.md', 'LICENSE'];
      await stageNativeDelivery({...fixture, kind, os, dest});
      assert.deepEqual((await readdir(dest)).sort(), expected.sort(), stem);
      const archive = path.join(directory, stem + '.7z');
      await execa(sevenzip, ['a', '-t7z', archive, stem], {cwd: directory});
      assert.equal((await readFile(archive)).subarray(0, 6).toString('hex'), '377abcaf271c');
      const extracted = path.join(directory, 'extract-' + stem);
      await execa(sevenzip, ['x', archive, '-o' + extracted, '-y']);
      assert.deepEqual(await readdir(extracted), [stem]);
      assert.deepEqual((await readdir(path.join(extracted, stem))).sort(), expected);
      await verifyNativeDelivery(kind, os, path.join(extracted, stem));
      for (const name of expected) {
        assert.deepEqual(await readFile(path.join(extracted, stem, name)), await readFile(path.join(dest, name)));
      }
    }
  }
});

test('missing native input and a nonempty stage fail without merging a delivery', async t => {
  const directory = await temporary(t);
  const fixture = await nativeFixture(directory);
  await rm(path.join(fixture.build, 'shotium.dll.lib'));
  const dest = path.join(directory, 'stage');
  await assert.rejects(stageNativeDelivery({...fixture, kind: 'c-abi', os: 'win', dest}), /missing.*shotium.dll.lib/);
  assert.equal(existsSync(dest), false);
  // CLI does not depend on the import library, header or C ABI documentation.
  await rm(path.join(fixture.sourceRoot, 'shot/shot_api.h'));
  await stageNativeDelivery({...fixture, kind: 'cli', os: 'win', dest});
  await assert.rejects(stageNativeDelivery({...fixture, kind: 'cli', os: 'win', dest}), /must be empty/);
  assert.equal((await readdir(dest)).length, 4);
  await writeFile(path.join(dest, 'shotium.dll'), 'accidental combined package');
  await assert.rejects(verifyNativeDelivery('cli', 'win', dest), /unexpected cli delivery contents/);
  await rm(path.join(fixture.build, 'shotium_data.pak'));
  await assert.rejects(stageNativeDelivery({...fixture, kind: 'cli', os: 'win', dest: dest + '-new'}), /missing.*shotium_data.pak/);
});

test('packaged markdown links point to the guides, header and license actually shipped', () => {
  const guide = '[中文](./README.zh.md) [header](../../shot/shot_api.h) [license](../../LICENSE)';
  assert.equal(archiveMarkdown(guide, true), '[中文](C_ABI.zh.md) [header](shot_api.h) [license](LICENSE)');
  const example = '[中文](./README.zh.md) [guide](../c-abi/README.md) [license](../../LICENSE)';
  assert.equal(archiveMarkdown(example, false), '[中文](./README.zh.md) [guide](C_ABI.md) [license](LICENSE)');
});

async function releaseFixture(t: TestContext): Promise<string> {
  const directory = await temporary(t);
  for (const name of releaseArchives) await writeFile(path.join(directory, name), 'release fixture: ' + name);
  return directory;
}

test('one standard, sorted SHA256SUMS covers exactly 17 fixed archive names', async t => {
  const directory = await releaseFixture(t);
  assert.equal(releaseArchives.length, 17);
  assert.equal(releaseArchives.filter(name => name.startsWith('shotium-cli-')).length, 6);
  assert.equal(releaseArchives.filter(name => name.startsWith('shotium-c-abi-')).length, 6);
  assert.deepEqual(releaseArchives.filter(name => name.startsWith('shotium-example-')), [
    'shotium-example-csharp.7z', 'shotium-example-go.7z', 'shotium-example-java.7z',
    'shotium-example-python.7z', 'shotium-example-rust.7z',
  ]);
  await writeReleaseChecksums(directory);
  const contents = await readFile(path.join(directory, 'SHA256SUMS'), 'utf8');
  assert.ok(contents.endsWith('\n'));
  const lines = contents.trimEnd().split('\n');
  assert.equal(lines.length, 17);
  assert.ok(lines.every(line => /^[0-9a-f]{64}  shotium-[a-z-0-9]+\.7z$/.test(line)));
  assert.deepEqual(lines.map(line => line.slice(66)), [...releaseArchives].sort());
  for (const line of lines) assert.equal(line.slice(0, 64), await sha256File(path.join(directory, line.slice(66))));
  assert.equal((await readdir(directory)).length, 18);
  await verifyReleaseChecksums(directory);
});

test('missing, extra or tampered archives stop checksum generation/verification', async t => {
  const directory = await releaseFixture(t);
  const name = 'shotium-example-python.7z';
  await writeReleaseChecksums(directory);
  await writeFile(path.join(directory, name), 'tampered content');
  await assert.rejects(verifyReleaseChecksums(directory), /checksum mismatch/);
  await rm(path.join(directory, name));
  await assert.rejects(verifyReleaseChecksums(directory), /missing release archives/);
  await assert.rejects(writeReleaseChecksums(directory), /missing release archives/);
  await writeFile(path.join(directory, name), 'replaced');
  for (const extra of ['example.sha256', 'shotium-python-example-v0.7.0.zip', 'package.tgz']) {
    await writeFile(path.join(directory, extra), 'unexpected');
    await assert.rejects(writeReleaseChecksums(directory), /unexpected release entry/);
    await rm(path.join(directory, extra));
  }
});

test('duplicate, unordered, missing and malformed checksum entries are rejected', async t => {
  const directory = await releaseFixture(t);
  await writeReleaseChecksums(directory);
  const checksum = path.join(directory, 'SHA256SUMS');
  const contents = await readFile(checksum, 'utf8');
  const lines = contents.trimEnd().split('\n');
  const cases: Array<[string, RegExp]> = [
    [contents + lines[0] + '\n', /duplicate/],
    [[...lines].reverse().join('\n') + '\n', /filename order/],
    [lines.slice(1).join('\n') + '\n', /exactly the 17/],
    [contents.replace('  shotium-', ' shotium-'), /invalid/],
    [contents.replace('  shotium-', '  dist/shotium-'), /invalid/],
    [contents.slice(0, -1), /newline/],
    [contents.replaceAll('\n', '\r\n'), /invalid/],
  ];
  for (const [invalid, message] of cases) {
    await writeFile(checksum, invalid);
    await assert.rejects(verifyReleaseChecksums(directory), message);
  }
});

test('native artifact collection rejects incomplete inputs, wrong contents and duplicate destinations', async t => {
  const directory = await temporary(t);
  const source = path.join(directory, 'artifacts'), dest = path.join(directory, 'release');
  await mkdir(source);
  await assert.rejects(collectNativeArchives(source, dest), /six platform directories/);
  for (const platform of releasePlatforms) {
    await mkdir(path.join(source, platform));
    for (const kind of ['cli', 'c-abi']) {
      const name = 'shotium-' + kind + '-' + platform + '.7z';
      await writeFile(path.join(source, platform, name), name);
    }
  }
  const extra = path.join(source, 'linux-amd64', 'shotium-cli-windows-amd64.7z');
  await writeFile(extra, 'duplicate in another artifact');
  await assert.rejects(collectNativeArchives(source, dest), /only its CLI and C ABI/);
  assert.equal(existsSync(dest), false);
  await rm(extra);
  await collectNativeArchives(source, dest);
  assert.equal((await readdir(dest)).length, 12);
  await assert.rejects(collectNativeArchives(source, dest), /duplicate release archive/);
});
