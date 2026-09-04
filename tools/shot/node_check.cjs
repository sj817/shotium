'use strict';

// Exercises the shotium package against a real engine.
//
// serve_check.py and net_check.py cover the executable. This covers the half
// that only exists in JavaScript: the addon seam, the queue in front of it,
// option validation, and the lifecycle the package is shaped around.
//
// It replaces two suites. node_check.cjs used to drive a pool of worker
// processes -- retry, a worker killed mid-request, a slot refilled -- and
// a second suite covered the in-process engine beside it. There is no pool
// any more: a pnpm install carries the shared library and the addon and not
// the executable, so this path is the only one, and the two suites had become
// two names for it.
//
// The .cjs extension is not decoration: chromium's root package.json says
// "type": "module", which would otherwise make this file an ES module and
// require() a syntax error.
//
// The package under test is an ES module and this is not, which is the point:
// require() of an ES module is what a CommonJS caller of @shotkit/shotium does,
// so checking it here is checking that. It needs node 22.12 or 20.19; the
// workflows pin 22, and a caller on anything older uses await import()
// instead.
//
//   node tools/shot/node_check.cjs out/Shot/shotium.exe
//
// The argument is the *executable*, which is what the other suites take and
// what this one compares against -- it is not what the package loads. The
// library, the addon and the resource packs are found where
// shotium/src/lib/binding.ts looks for them, and the build directory the
// executable sits in is where the packs are.

const assert = require('assert');
const {execFileSync} = require('child_process');
const crypto = require('crypto');
const fs = require('fs');
const os = require('os');
const path = require('path');

const shotium = require('../../shotium');

const exe = path.resolve(process.argv[2] || 'out/Shot/shotium.exe');
const buildDir = path.dirname(exe);
const corpus = path.resolve('shot/testdata/render_corpus.html');
const features = path.resolve('shot/testdata/features.html');
const output = path.join(os.tmpdir(), `shot-node-check-${process.pid}.png`);

let failures = 0;

function check(ok, label, detail = '') {
  console.log(`  ${ok ? 'PASS' : 'FAIL'}  ${label}${detail ? '   ' + detail : ''}`);
  if (!ok) {
    failures += 1;
  }
}

function sha(buffer) {
  return crypto.createHash('sha256').update(buffer).digest('hex');
}

const request = {
  file: corpus,
  viewport: {width: 1248, height: 1320},
  allowFileAccess: true,
};

async function main() {
  console.log(`shotium package, against ${exe}\n`);

  console.log('== what a CommonJS caller gets ==');
  // require() of an ES module: node builds the namespace and this is what the
  // caller sees. If the exports drift -- a rename, a default that is not the
  // same object as the names -- it shows up here and nowhere else.
  for (const name of ['Runtime', 'runtime', 'screenshot', 'screenshotTiles',
                      'daemon', 'cache', 'start', 'stop', 'status',
                      'releaseMemory']) {
    check(shotium[name] !== undefined, `\`${name}\` is exported`);
  }
  check(shotium.default.runtime === shotium.runtime,
        'the default export and the named ones are the same objects');

  console.log('\n== it starts and renders ==');
  // resourceDir because the packs live beside the executable in a checkout,
  // not beside the addon. An install has them in one directory and needs none
  // of this.
  const came = shotium.runtime.start({resourceDir: buildDir, cacheDir: null});
  check(shotium.runtime.running === true, 'the engine is up');
  check(came.running === true && came.cacheDir === null,
        'and start() reports what it came up as', JSON.stringify(came));
  // Which engine answered, checked rather than assumed. A checkout that has
  // ever run `pnpm install` has the published platform package sitting in
  // node_modules next to the build under test, and running this suite against
  // the last release instead of the working tree is a failure that looks like
  // a code bug -- an entry point added since the release is "not a function".
  const localAddon = path.resolve(
      'shotium', 'native', 'build', 'Release', 'shotium.node');
  check(!fs.existsSync(localAddon) ||
            path.resolve(came.enginePath || '') === path.dirname(localAddon),
        'and it is the addon built from this checkout',
        `${came.enginePath}`);
  const {image: first, stats} = await shotium.screenshot(request);
  check(Buffer.isBuffer(first) && first.length > 0, 'a screenshot comes back',
        `${first.length} bytes`);
  check(first.subarray(0, 8).equals(Buffer.from('89504e470d0a1a0a', 'hex')),
        'and it is a PNG');

  // New in 0.3, and the reason screenshot() resolves to an object rather than
  // to the buffer: the counters the engine always had now leave it.
  check(stats.requests >= 1 && stats.timing.total > 0,
        'and it says what it cost',
        `${stats.requests} request(s), ${stats.timing.total.toFixed(1)}ms`);
  check(stats.timing.fetch + stats.timing.render <= stats.timing.total + 1,
        'whose parts add up to no more than the whole');

  console.log('\n== it is the same renderer as the executable ==');
  // Not "close enough": identical. Both go through shot::Capture, and the only
  // way these bytes differ is if one of the two paths has grown a difference
  // in what it asks for -- which is exactly the drift this checks for.
  execFileSync(exe, [
    corpus, '-o', output, '--width=1248', '--height=1320',
    '--allow-file-access',
  ]);
  const fromExe = fs.readFileSync(output);
  check(sha(fromExe) === sha(first), 'byte for byte what the executable makes',
        sha(first).slice(0, 16));

  console.log('\n== the geometry options reach the engine ==');
  const {image: clipped} = await shotium.screenshot({
    file: features,
    viewport: {width: 400, height: 300},
    clip: {x: 40, y: 60, width: 200, height: 120},
    allowFileAccess: true,
  });
  check(clipped.readUInt32BE(16) === 200 && clipped.readUInt32BE(20) === 120,
        'clip arrives as a 200x120 image',
        `${clipped.readUInt32BE(16)}x${clipped.readUInt32BE(20)}`);

  const written = path.join(os.tmpdir(), `shot-node-check-path-${process.pid}.png`);
  const {image: viaPath} = await shotium.screenshot({
    file: features,
    viewport: {width: 400, height: 300},
    path: written,
    allowFileAccess: true,
  });
  check(viaPath === null, 'a request with `path` gives back a null image');
  check(fs.existsSync(written), 'and the engine wrote the file');
  fs.rmSync(written, {force: true});

  console.log('\n== tiles ==');
  // Taller than blink paints from one scroll position, so this exercises the
  // banded path as well as the cut: white to the bottom, then a red strip.
  const tallHeight = 36000;
  const tall = path.join(os.tmpdir(), `shot-node-check-tall-${process.pid}.html`);
  fs.writeFileSync(
      tall,
      '<body style="margin:0">' +
          `<div style="height:${tallHeight - 10}px;background:#fff"></div>` +
          '<div style="height:10px;background:#f00"></div></body>');
  const pngHeight = (png) => png.readUInt32BE(20);
  const {tiles, stats: tileStats} = await shotium.screenshotTiles({
    file: tall,
    viewport: {width: 400, height: 300},
    fullPage: true,
    tile: {height: 8000},
  });
  check(tiles.length === 5, '36000px in 8000px tiles is five tiles',
        `${tiles.length}`);
  check(tiles.every((t) => Buffer.isBuffer(t.image) && t.width === 400),
        'each with an image and the region\'s width');
  check(tiles.map((t) => t.y).join() === '0,8000,16000,24000,32000',
        'stacked top to bottom', tiles.map((t) => t.y).join());
  check(tiles.map((t) => pngHeight(t.image)).join() ===
            '8000,8000,8000,8000,4000',
        'and each image is as tall as its tile',
        tiles.map((t) => pngHeight(t.image)).join());
  check(tileStats.requests >= 1 && tileStats.timing.total > 0,
        'with one set of stats for the lot');

  const {image: whole} = await shotium.screenshot({
    file: tall,
    viewport: {width: 400, height: 300},
    fullPage: true,
    scale: 0.25,
  });
  check(pngHeight(whole) === tallHeight / 4,
        'and fullPage alone still gives the whole page as one image',
        `${pngHeight(whole)}`);

  let misplaced = null;
  try {
    await shotium.screenshot({file: tall, tile: {height: 8000}});
  } catch (error) {
    misplaced = error;
  }
  check(misplaced instanceof TypeError && /screenshotTiles/.test(misplaced.message),
        'tile on screenshot() names the call that takes it');

  const tilePath = path.join(os.tmpdir(), `shot-node-check-tile-${process.pid}-{n}.png`);
  const {tiles: onDisk} = await shotium.screenshotTiles({
    file: tall,
    viewport: {width: 400, height: 300},
    fullPage: true,
    tile: {height: 8000},
    path: tilePath,
  });
  check(onDisk.every((t) => t.image === null && typeof t.path === 'string'),
        'with `path`, tiles come back as file names');
  check(onDisk.every((t) => fs.existsSync(t.path)) &&
            onDisk[0].path.endsWith('-1.png'),
        'numbered from 1 and written', onDisk[0].path);
  for (const t of onDisk) {
    fs.rmSync(t.path, {force: true});
  }
  fs.rmSync(tall, {force: true});

  console.log('\n== errors are errors, not crashes ==');
  // This one matters more than it did with a pool behind it. A worker that
  // died took nothing with it; here a crash takes the host program, so
  // "rejects" rather than "dies" is the whole claim.
  await assert.rejects(
      () => shotium.screenshot({file: features, selector: '#nothing-here'}),
      /no element matches/, 'a selector that matches nothing rejects');
  check(true, 'a failed request rejects with the engine\'s own message');

  let rejected = null;
  try {
    await shotium.screenshot({
      file: path.join(os.tmpdir(), 'shot-node-check-nope.html'),
      allowFileAccess: true,
    });
  } catch (error) {
    rejected = error;
  }
  check(rejected instanceof Error && /could not read/i.test(rejected.message),
        'a missing document says what it could not read',
        rejected ? rejected.message.slice(0, 48) : 'no error');

  const {image: afterError} = await shotium.screenshot(request);
  check(sha(afterError) === sha(first), 'the engine still works afterwards');

  // The counters are attached to failures too, which is the case where they
  // explain the most: a capture that gave up after forty subresources has
  // already said what happened.
  let withStats = null;
  try {
    await shotium.screenshot({file: features, selector: '#nothing-here'});
  } catch (error) {
    withStats = error;
  }
  check(withStats && withStats.stats && typeof withStats.stats === 'object',
        'and a rejection carries the statistics as an object',
        withStats && withStats.stats ?
            `${withStats.stats.requests} request(s)` : 'none');

  let threw = null;
  try {
    await shotium.screenshot({file: corpus, fullpage: true});
  } catch (error) {
    threw = error;
  }
  check(threw instanceof TypeError && /fullpage/.test(threw.message),
        'a misspelled option is refused rather than dropped',
        threw ? threw.message : 'no error');

  // Same rule, for an option that used to be real. `retry` belonged to the
  // supervisor that re-sent a request to a fresh worker; there is no worker and
  // no supervisor, so accepting it would promise a retry that never happens.
  let retried = null;
  try {
    await shotium.screenshot({file: corpus, retry: 2});
  } catch (error) {
    retried = error;
  }
  check(retried instanceof TypeError && /retry/.test(retried.message),
        'and so is an option that was real in 0.1.0',
        retried ? retried.message : 'no error');

  console.log('\n== concurrent callers are serialised, not raced ==');
  // Blink renders one document at a time. Nine callers who do not know that
  // must still get nine correct answers rather than nine interleaved ones.
  const started = Date.now();
  const many = (await Promise.all(
      Array.from({length: 9}, () => shotium.screenshot(request))))
      .map((result) => result.image);
  check(many.length === 9 && many.every((png) => sha(png) === sha(first)),
        'nine at once give nine images identical to the first');
  console.log(`        ${Date.now() - started}ms for nine`);

  console.log('\n== there is one engine, and it is shared ==');
  // Not one at a time -- one. Blink's initialisation writes process-wide
  // statics it cannot undo, so there is no second engine for a second Runtime
  // to have. It gets the first one rather than an error: a lifecycle object is
  // a lifecycle, and how many of those a program keeps is its own business.
  const second = new shotium.Runtime();
  const adopted = second.start({resourceDir: buildDir});
  check(second.running === true, 'a second Runtime adopts the engine');
  check(adopted.cacheDir === null,
        'and gets the configuration the engine actually has',
        JSON.stringify(adopted));

  // What it cannot do is ask for a different one. The options are fixed when
  // the engine is built and there is no second build, so the alternative to
  // this error is rendering with a value the caller did not ask for.
  let refused = null;
  try {
    second.start({resourceDir: buildDir, cacheDir: path.join(os.tmpdir(), 'no')});
  } catch (error) {
    refused = error;
  }
  check(refused instanceof Error, 'a conflicting configuration is refused');
  check(refused && /cacheDir is/.test(refused.message),
        'and says which option disagrees',
        refused ? refused.message.slice(0, 56) : '');

  console.log('\n== releasing memory costs nothing but memory ==');
  shotium.runtime.releaseMemory({releaseWorkingSet: true});
  const {image: afterPurge} = await shotium.screenshot(request);
  check(sha(afterPurge) === sha(first), 'the release changed no pixels');

  console.log('\n== stop() is not a destructor ==');
  await shotium.runtime.stop();
  check(shotium.runtime.running === false, 'stop() leaves the engine down');

  // The whole point of the 0.3 lifecycle. Blink still cannot be initialised
  // twice, but that is a fact about how many engines there are rather than
  // about how many times one may be asked for -- and a disk cache whose value
  // is the next run would be worth very little if the next run could not have
  // the engine that reads it.
  let restarted = null;
  try {
    shotium.runtime.start({resourceDir: buildDir});
  } catch (error) {
    restarted = error;
  }
  check(restarted === null && shotium.runtime.running === true,
        'and starting again picks the same engine back up',
        restarted ? restarted.message.slice(0, 56) : '');

  const {image: afterRestart} = await shotium.screenshot(request);
  check(sha(afterRestart) === sha(first),
        'which renders exactly what it rendered before');

  await shotium.runtime.stop();
  const {image: afterImplicit} = await shotium.screenshot(request);
  check(sha(afterImplicit) === sha(first),
        'and a screenshot after stop() starts it without being asked');
  await shotium.runtime.stop();

  fs.rmSync(output, {force: true});
  console.log(`\n${failures ? failures + ' CHECK(S) FAILED' : 'ALL CHECKS PASSED'}`);
  process.exit(failures ? 1 : 0);
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
