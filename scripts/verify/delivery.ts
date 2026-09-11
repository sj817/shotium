// Validate what npm installs, outside a checkout's local-addon discovery path.
//
// The reference image comes from the standalone CLI, which is no longer in
// the npm package: pass the one extracted from the shotium-cli-<platform>
// release archive of the same build with --cli.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {existsSync, mkdirSync, mkdtempSync, readFileSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {cac} from 'cac';
import {execa} from 'execa';
import {globSync} from 'tinyglobby';
import {exeName, resolve} from '../lib/repo.ts';

async function main(platformDirectory: string, cli: string): Promise<void> {
  const tarballs = globSync('*.tgz', {cwd: resolve(platformDirectory), absolute: true});
  assert.equal(tarballs.length, 1, 'expected exactly one platform tarball');
  const output = resolve('out/delivery-check');
  mkdirSync(output, {recursive: true});
  const temporary = mkdtempSync(path.join(os.tmpdir(), 'shotium-delivery-'));
  const npm = process.platform === 'win32' ? 'npm.cmd' : 'npm';
  await execa(npm, ['pack', '--pack-destination', temporary], {cwd: resolve('apps/typescript')});
  const mainTarball = globSync('*.tgz', {cwd: temporary, absolute: true})[0]!;
  writeFileSync(path.join(temporary, 'package.json'), '{"private":true}');
  await execa(npm, ['install', '--ignore-scripts', '--no-audit', '--no-fund', mainTarball, tarballs[0]!], {cwd: temporary});
  const platform = `shotium-${process.platform}-${process.arch}`;
  const platformPath = path.join(temporary, 'node_modules/@pixel.js', platform);
  assert.equal(globSync(['**/*.dll', '**/*.so', '**/*.dylib'], {cwd: platformPath}).length, 0, 'npm platform package must not carry a C ABI library');
  assert.ok(!existsSync(path.join(platformPath, exeName)), 'npm platform package must not carry the CLI executable');
  const source = `
    const assert = require('node:assert/strict');
    const fs = require('node:fs');
    const path = require('node:path');
    const hash = bytes => require('node:crypto').createHash('sha256').update(bytes).digest('hex');
    const shot = require('@pixel.js/shotium');
    (async () => {
      assert.equal(shot.runtime.running, false);
      shot.start({cacheDir: null});
      const result = await shot.screenshot({file: process.env.SHOT_FIXTURE, allowFileAccess: true, viewport: {width: 720, height: 380}});
      assert.equal(hash(result.image), process.env.SHOT_EXPECTED);
      assert.equal(result.stats.failed, 0);
      const tiles = await shot.screenshotTiles({file: process.env.SHOT_FIXTURE, allowFileAccess: true, viewport: {width:720,height:380}, tile: {height:300}});
      assert.equal(tiles.tiles.length, 2);
      await shot.stop();
      const addon = Object.keys(require.cache).find(p => p.endsWith('.node'));
      assert.equal(fs.realpathSync(path.dirname(addon)), fs.realpathSync(process.env.SHOT_PLATFORM));
      console.log(JSON.stringify({addon, imageSha256: hash(result.image), tiles: tiles.tiles.length}));
    })().catch(e => {console.error(e); process.exitCode=1;});
  `;
  const cliPng = path.join(output, 'cli.png');
  const fixture = resolve('apps/demo-card/card.html');
  await execa(resolve(cli), ['--file', fixture, '--width', '720', '--height', '380', '-o', cliPng]);
  const result = await execa(process.execPath, ['-e', source], {cwd: temporary, timeout: 30000, env: {
    SHOT_FIXTURE: fixture, SHOT_EXPECTED: createHash('sha256').update(readFileSync(cliPng)).digest('hex'), SHOT_PLATFORM: platformPath,
  }});
  const report = {platform: process.platform, arch: process.arch, directory: temporary, ...JSON.parse(result.stdout)};
  writeFileSync(path.join(output, 'report.json'), JSON.stringify(report, null, 2) + '\n');
  console.log('PASS clean npm delivery: capture, tiles, no CLI, no C ABI library');
}
const cli = cac('pnpm verify:delivery');
cli.command('', 'verify clean npm installation from local tarballs')
    .option('--platform-dir <dir>', 'directory containing one platform tarball (required)')
    .option('--cli <file>', 'the standalone CLI from the same build, the reference renderer (required)')
    .action(async (options: {platformDir?: string; cli?: string}) => {
      try {
        if (!options.platformDir) throw new Error('--platform-dir is required');
        if (!options.cli) throw new Error('--cli is required: the CLI is no longer in the npm package');
        await main(options.platformDir, options.cli);
      } catch (error) { console.error(error); process.exitCode = 1; }
    });
cli.help();
cli.parse();
