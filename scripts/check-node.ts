// Exercises the shotium package against a real engine.
//
// check-serve and check-net cover the executable. This covers the half that
// only exists in JavaScript: the addon seam, the queue in front of it, option
// validation, and the lifecycle the package is shaped around.
//
// The package is loaded with require(), not import, on purpose: require() of
// an ES module is what a CommonJS caller of @shotkit/shotium does, so checking
// it here is checking that. It needs node 22.12 or 20.19; the workflows pin
// 22, and a caller on anything older uses await import() instead.
//
//   pnpm verify:node out/Shot/shotium.exe
//
// The argument is the *executable*, which is what the other suites take and
// what this one compares against -- it is not what the package loads. The
// library, the addon and the resource packs are found where
// shotium/src/lib/binding.ts looks for them, and the build directory the
// executable sits in is where the packs are. PATH must contain that directory
// (the addon links shotium.dll); without it the load fails with
// ERR_DLOPEN_FAILED. Relative paths are resolved against the repository root.

import assert from 'node:assert';
import {execFileSync} from 'node:child_process';
import {chmodSync, existsSync, mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, rmdirSync, statSync, writeFileSync} from 'node:fs';
import {createRequire} from 'node:module';
import os from 'node:os';
import path from 'node:path';

import {cac} from 'cac';

import {resolve} from './lib/repo.ts';
import {Checks, sha256 as sha} from './lib/report.ts';

import type * as Shotium from '../shotium/src/index.ts';

type Package = typeof Shotium & {default: typeof Shotium};

async function main(exeArg: string): Promise<number> {
  const exe = resolve(exeArg);
  const buildDir = path.dirname(exe);
  const corpus = resolve('shot/testdata/render_corpus.html');
  const features = resolve('shot/testdata/features.html');
  const output = path.join(os.tmpdir(), `shot-node-check-${process.pid}.png`);
  const checks = new Checks();
  const check = checks.check.bind(checks);

  // require() of an ES module: node builds the namespace and this is what the
  // caller sees. If the exports drift -- a rename, a default that is not the
  // same object as the names -- it shows up here and nowhere else.
  const shotium = createRequire(import.meta.url)(resolve('shotium')) as Package;

  console.log(`shotium package, against ${exe}\n`);
  console.log('== what a CommonJS caller gets ==');
  for (const name of ['Runtime', 'runtime', 'screenshot', 'screenshotTiles', 'daemon', 'cache', 'start', 'stop', 'status', 'releaseMemory'] as const) {
    check(shotium[name] !== undefined, `\`${name}\` is exported`);
  }
  check(shotium.default.runtime === shotium.runtime, 'the default export and the named ones are the same objects');

  const request = {file: corpus, viewport: {width: 1248, height: 1320}, allowFileAccess: true};

  checks.section('it starts and renders');
  // resourceDir because the packs live beside the executable in a checkout,
  // not beside the addon. An install has them in one directory and needs none
  // of this.
  const came = shotium.runtime.start({resourceDir: buildDir, cacheDir: null});
  check(shotium.runtime.running === true, 'the engine is up');
  check(came.running === true && came.cacheDir === null, 'and start() reports what it came up as', JSON.stringify(came));
  // Which engine answered, checked rather than assumed. A checkout that has
  // ever run `pnpm install` has the published platform package sitting in
  // node_modules next to the build under test, and running this suite against
  // the last release instead of the working tree is a failure that looks like
  // a code bug -- an entry point added since the release is "not a function".
  const localAddon = resolve('shotium', 'native', 'build', 'Release', 'shotium.node');
  check(!existsSync(localAddon) || path.resolve(came.enginePath || '') === path.dirname(localAddon),
        'and it is the addon built from this checkout', `${came.enginePath}`);
  const {image: first, stats} = await shotium.screenshot(request);
  check(Buffer.isBuffer(first) && first.length > 0, 'a screenshot comes back', `${first!.length} bytes`);
  check(first!.subarray(0, 8).equals(Buffer.from('89504e470d0a1a0a', 'hex')), 'and it is a PNG');

  // New in 0.3, and the reason screenshot() resolves to an object rather than
  // to the buffer: the counters the engine always had now leave it.
  check(stats.requests >= 1 && stats.timing.total > 0, 'and it says what it cost', `${stats.requests} request(s), ${stats.timing.total.toFixed(1)}ms`);
  check(stats.timing.fetch + stats.timing.render <= stats.timing.total + 1, 'whose parts add up to no more than the whole');

  checks.section('it is the same renderer as the executable');
  // Not "close enough": identical. Both go through shot::Capture, and the only
  // way these bytes differ is if one of the two paths has grown a difference
  // in what it asks for -- which is exactly the drift this checks for.
  execFileSync(exe, [corpus, '-o', output, '--width=1248', '--height=1320', '--allow-file-access']);
  const fromExe = readFileSync(output);
  check(sha(fromExe) === sha(first!), 'byte for byte what the executable makes', sha(first!).slice(0, 16));

  checks.section('the geometry options reach the engine');
  const {image: clipped} = await shotium.screenshot({file: features, viewport: {width: 400, height: 300}, clip: {x: 40, y: 60, width: 200, height: 120}, allowFileAccess: true});
  check(clipped!.readUInt32BE(16) === 200 && clipped!.readUInt32BE(20) === 120, 'clip arrives as a 200x120 image', `${clipped!.readUInt32BE(16)}x${clipped!.readUInt32BE(20)}`);

  const writtenDir = mkdtempSync(path.join(os.tmpdir(), 'shot-node-check-path-'));
  const written = path.join(writtenDir, 'image.png');
  const {image: viaPath} = await shotium.screenshot({file: features, viewport: {width: 400, height: 300}, path: written, allowFileAccess: true});
  check(viaPath === null, 'a request with `path` gives back a null image');
  check(existsSync(written), 'and the engine wrote the file');
  const oldOutput = readFileSync(written);
  await assert.rejects(() => shotium.screenshot({file: features, path: written, selector: '#not-present', allowFileAccess: true}), /no element matches/);
  check(readFileSync(written).equals(oldOutput), 'a failed single-file render preserves the previous destination');
  const {image: replaced} = await shotium.screenshot({...request, path: written});
  check(replaced === null && sha(readFileSync(written)) === sha(first!), 'single-file replacement installs the complete new image');
  if (process.platform !== 'win32') {
    chmodSync(written, 0o640);
    await shotium.screenshot({...request, path: written});
    check((statSync(written).mode & 0o777) === 0o640, 'single-file replacement preserves POSIX permissions');
  }
  rmSync(written, {force: true});
  mkdirSync(written);
  await assert.rejects(() => shotium.screenshot({...request, path: written}), /could not write|could not install|could not preserve|could not read permissions/);
  check(statSync(written).isDirectory() && readdirSync(writtenDir).join() === 'image.png', 'a failed single-file install preserves the destination and cleans staging files');
  rmdirSync(written);
  rmdirSync(writtenDir);

  checks.section('backdrop effects keep display-list pairs balanced');
  for (const fixture of ['gradient', 'filter']) {
    for (const type of ['png', 'jpeg', 'webp'] as const) {
      const fixtureRequest = {file: resolve(`apps/benchmark/fixtures/${fixture}.html`), type, viewport: {width: 1280, height: 720}, allowFileAccess: true};
      const {image, stats: fixtureStats} = await shotium.screenshot(fixtureRequest);
      check(Buffer.isBuffer(image) && image.length > 1000 && fixtureStats.failed === 0, `${fixture} with nested backdrop effects renders as ${type}`);
      // Multiple tiles retain the per-item spatial index, which is where an
      // unbalanced backdrop save/restore used to underflow the pairing stack.
      const {tiles, stats: tiledStats} = await shotium.screenshotTiles({...fixtureRequest, tile: {height: 360}});
      check(tiles.length === 2 && tiledStats.failed === 0 && tiles.every((tile) => Buffer.isBuffer(tile.image) && tile.image.length > 1000),
            `${fixture} with nested backdrop effects also renders in ${type} tiles`);
    }
  }

  checks.section('tiles');
  // Taller than blink paints from one scroll position, so this exercises the
  // banded path as well as the cut: white to the bottom, then a red strip.
  const tallHeight = 36000;
  const tall = path.join(os.tmpdir(), `shot-node-check-tall-${process.pid}.html`);
  writeFileSync(tall, `<body style="margin:0"><div style="height:${tallHeight - 10}px;background:#fff"></div><div style="height:10px;background:#f00"></div></body>`);
  const pngHeight = (png: Buffer) => png.readUInt32BE(20);
  const {tiles, stats: tileStats} = await shotium.screenshotTiles({file: tall, viewport: {width: 400, height: 300}, fullPage: true, tile: {height: 8000}});
  check(tiles.length === 5, '36000px in 8000px tiles is five tiles', `${tiles.length}`);
  check(tiles.every((t) => Buffer.isBuffer(t.image) && t.width === 400), "each with an image and the region's width");
  check(tiles.map((t) => t.y).join() === '0,8000,16000,24000,32000', 'stacked top to bottom', tiles.map((t) => t.y).join());
  check(tiles.map((t) => pngHeight(t.image!)).join() === '8000,8000,8000,8000,4000', 'and each image is as tall as its tile', tiles.map((t) => pngHeight(t.image!)).join());
  check(tileStats.requests >= 1 && tileStats.timing.total > 0, 'with one set of stats for the lot');

  for (const height of [1, 32000]) {
    const {tiles: boundary} = await shotium.screenshotTiles({file: features, viewport: {width: 400, height: 300}, clip: {x: 40, y: 60, width: 1, height: 1}, tile: {height}, allowFileAccess: true});
    check(boundary.length === 1 && boundary[0].height === 1, `tile.height accepts the ${height === 1 ? 'lower' : 'upper'} boundary`, JSON.stringify(boundary.map((tile) => tile.height)));
  }

  const {image: whole} = await shotium.screenshot({file: tall, viewport: {width: 400, height: 300}, fullPage: true, scale: 0.25});
  check(pngHeight(whole!) === tallHeight / 4, 'and fullPage alone still gives the whole page as one image', `${pngHeight(whole!)}`);

  const misplaced = await shotium.screenshot({file: tall, tile: {height: 8000}} as never).then(() => null, (e: unknown) => e);
  check(misplaced instanceof TypeError && /screenshotTiles/.test(misplaced.message), 'tile on screenshot() names the call that takes it');

  const tilePath = path.join(os.tmpdir(), `shot-node-check-tile-${process.pid}-{n}.png`);
  const {tiles: onDisk} = await shotium.screenshotTiles({file: tall, viewport: {width: 400, height: 300}, fullPage: true, tile: {height: 8000}, path: tilePath});
  check(onDisk.every((t) => t.image === null && typeof t.path === 'string'), 'with `path`, tiles come back as file names');
  check(onDisk.every((t) => existsSync(t.path!)) && onDisk[0].path!.endsWith('-1.png'), 'numbered from 1 and written', onDisk[0].path);
  for (const t of onDisk) rmSync(t.path!, {force: true});
  rmSync(tall, {force: true});

  checks.section('errors are errors, not crashes');
  // A crash takes the host program with it, so "rejects" rather than "dies"
  // is the whole claim.
  await assert.rejects(() => shotium.screenshot({file: features, selector: '#nothing-here'}), /no element matches/, 'a selector that matches nothing rejects');
  check(true, "a failed request rejects with the engine's own message");

  const rejected = await shotium.screenshot({file: path.join(os.tmpdir(), 'shot-node-check-nope.html'), allowFileAccess: true}).then(() => null, (e: unknown) => e);
  check(rejected instanceof Error && /could not read/i.test(rejected.message), 'a missing document says what it could not read', rejected instanceof Error ? rejected.message.slice(0, 48) : 'no error');

  const {image: afterError} = await shotium.screenshot(request);
  check(sha(afterError!) === sha(first!), 'the engine still works afterwards');

  // The counters are attached to failures too, which is the case where they
  // explain the most: a capture that gave up after forty subresources has
  // already said what happened.
  const withStats = await shotium.screenshot({file: features, selector: '#nothing-here'}).then(() => null, (e: unknown) => e as {stats?: {requests?: number}});
  check(withStats !== null && typeof withStats.stats === 'object' && withStats.stats !== null, 'and a rejection carries the statistics as an object', withStats?.stats ? `${withStats.stats.requests} request(s)` : 'none');

  const threw = await shotium.screenshot({file: corpus, fullpage: true} as never).then(() => null, (e: unknown) => e);
  check(threw instanceof TypeError && /fullpage/.test(threw.message), 'a misspelled option is refused rather than dropped', threw instanceof Error ? threw.message : 'no error');

  // Same rule, for an option that used to be real. `retry` belonged to the
  // supervisor that re-sent a request to a fresh worker; there is no worker
  // and no supervisor, so accepting it would promise a retry that never
  // happens.
  const retried = await shotium.screenshot({file: corpus, retry: 2} as never).then(() => null, (e: unknown) => e);
  check(retried instanceof TypeError && /retry/.test(retried.message), 'and so is an option that was real in 0.1.0', retried instanceof Error ? retried.message : 'no error');

  checks.section('concurrent callers are serialised, not raced');
  // Blink renders one document at a time. Nine callers who do not know that
  // must still get nine correct answers rather than nine interleaved ones.
  const started = Date.now();
  const many = (await Promise.all(Array.from({length: 9}, () => shotium.screenshot(request)))).map((result) => result.image!);
  check(many.length === 9 && many.every((png) => sha(png) === sha(first!)), 'nine at once give nine images identical to the first');
  console.log(`        ${Date.now() - started}ms for nine`);

  checks.section('there is one engine, and it is shared');
  // Not one at a time -- one. Blink's initialisation writes process-wide
  // statics it cannot undo, so there is no second engine for a second Runtime
  // to have. It gets the first one rather than an error: a lifecycle object is
  // a lifecycle, and how many of those a program keeps is its own business.
  const second = new shotium.Runtime();
  const adopted = second.start({resourceDir: buildDir});
  check(second.running === true, 'a second Runtime adopts the engine');
  check(adopted.cacheDir === null, 'and gets the configuration the engine actually has', JSON.stringify(adopted));

  // What it cannot do is ask for a different one. The options are fixed when
  // the engine is built and there is no second build, so the alternative to
  // this error is rendering with a value the caller did not ask for.
  let refused: Error | null = null;
  try {
    second.start({resourceDir: buildDir, cacheDir: path.join(os.tmpdir(), 'no')});
  } catch (error) {
    refused = error as Error;
  }
  check(refused instanceof Error, 'a conflicting configuration is refused');
  check(refused !== null && /cacheDir is/.test(refused.message), 'and says which option disagrees', refused ? refused.message.slice(0, 56) : '');

  checks.section('releasing memory costs nothing but memory');
  shotium.runtime.releaseMemory({releaseWorkingSet: true});
  const {image: afterPurge} = await shotium.screenshot(request);
  check(sha(afterPurge!) === sha(first!), 'the release changed no pixels');

  checks.section('stop() is not a destructor');
  await shotium.runtime.stop();
  check(shotium.runtime.running === false, 'stop() leaves the engine down');

  // Blink still cannot be initialised twice, but that is a fact about how many
  // engines there are rather than about how many times one may be asked for.
  let restarted: Error | null = null;
  try {
    shotium.runtime.start({resourceDir: buildDir});
  } catch (error) {
    restarted = error as Error;
  }
  check(restarted === null && shotium.runtime.running === true, 'and starting again picks the same engine back up', restarted ? restarted.message.slice(0, 56) : '');

  const {image: afterRestart} = await shotium.screenshot(request);
  check(sha(afterRestart!) === sha(first!), 'which renders exactly what it rendered before');

  await shotium.runtime.stop();
  const {image: afterImplicit} = await shotium.screenshot(request);
  check(sha(afterImplicit!) === sha(first!), 'and a screenshot after stop() starts it without being asked');
  await shotium.runtime.stop();

  rmSync(output, {force: true});
  return checks.finish();
}

const cli = cac('check-node');
cli.command('<exe>', 'exercise the shotium package against a real engine')
    .action(async (exe: string) => {
      try {
        process.exitCode = await main(exe);
      } catch (error) {
        console.error(error);
        process.exitCode = 1;
      }
      // The engine may keep its threads alive; the verdict is in, so leave.
      process.exit(process.exitCode);
    });
cli.help();
cli.parse();
