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
// any more: an npm install carries the shared library and the addon and not
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
  for (const name of ['Runtime', 'runtime', 'screenshot', 'daemon']) {
    check(shotium[name] !== undefined, `\`${name}\` is exported`);
  }
  check(shotium.default.runtime === shotium.runtime,
        'the default export and the named ones are the same objects');

  console.log('\n== it starts and renders ==');
  // resourceDir because the packs live beside the executable in a checkout,
  // not beside the addon. An install has them in one directory and needs none
  // of this.
  shotium.runtime.start({resourceDir: buildDir, cacheDir: null});
  check(shotium.runtime.running === true, 'the engine is up');
  const first = await shotium.screenshot(request);
  check(Buffer.isBuffer(first) && first.length > 0, 'a screenshot comes back',
        `${first.length} bytes`);
  check(first.subarray(0, 8).equals(Buffer.from('89504e470d0a1a0a', 'hex')),
        'and it is a PNG');

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
  const clipped = await shotium.screenshot({
    file: features,
    viewport: {width: 400, height: 300},
    clip: {x: 40, y: 60, width: 200, height: 120},
    allowFileAccess: true,
  });
  check(clipped.readUInt32BE(16) === 200 && clipped.readUInt32BE(20) === 120,
        'clip arrives as a 200x120 image',
        `${clipped.readUInt32BE(16)}x${clipped.readUInt32BE(20)}`);

  const written = path.join(os.tmpdir(), `shot-node-check-path-${process.pid}.png`);
  const viaPath = await shotium.screenshot({
    file: features,
    viewport: {width: 400, height: 300},
    path: written,
    allowFileAccess: true,
  });
  check(viaPath === null, 'a request with `path` resolves to null');
  check(fs.existsSync(written), 'and the engine wrote the file');
  fs.rmSync(written, {force: true});

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

  const afterError = await shotium.screenshot(request);
  check(sha(afterError) === sha(first), 'the engine still works afterwards');

  let threw = null;
  try {
    await shotium.screenshot({file: corpus, fullpage: true});
  } catch (error) {
    threw = error;
  }
  check(threw instanceof TypeError && /fullpage/.test(threw.message),
        'a misspelled option is refused rather than dropped',
        threw ? threw.message : 'no error');

  console.log('\n== concurrent callers are serialised, not raced ==');
  // Blink renders one document at a time. Nine callers who do not know that
  // must still get nine correct answers rather than nine interleaved ones.
  const started = Date.now();
  const many = await Promise.all(
      Array.from({length: 9}, () => shotium.screenshot(request)));
  check(many.length === 9 && many.every((png) => sha(png) === sha(first)),
        'nine at once give nine images identical to the first');
  console.log(`        ${Date.now() - started}ms for nine`);

  console.log('\n== there is only ever one engine ==');
  // Not one at a time -- one. Blink's initialisation writes process-wide
  // statics it cannot undo, so a second Runtime has nothing to be. The
  // package says so itself rather than letting SHOT_ERR_STATE out of the
  // addon, and this is that message.
  let refused = null;
  try {
    new shotium.Runtime().start({resourceDir: buildDir});
  } catch (error) {
    refused = error;
  }
  check(refused instanceof Error, 'a second Runtime is refused');
  check(refused && /process-wide singleton/.test(refused.message),
        'and says why rather than failing obscurely',
        refused ? refused.message.slice(0, 56) : '');

  console.log('\n== purging costs nothing but memory ==');
  shotium.runtime.purge({releaseWorkingSet: true});
  const afterPurge = await shotium.screenshot(request);
  check(sha(afterPurge) === sha(first), 'the purge changed no pixels');

  console.log('\n== and it puts itself away, once ==');
  await shotium.runtime.stop();
  check(shotium.runtime.running === false, 'stop() leaves the engine down');

  let restarted = null;
  try {
    shotium.runtime.start({resourceDir: buildDir});
  } catch (error) {
    restarted = error;
  }
  check(restarted instanceof Error && /cannot be started again/.test(restarted.message),
        'and starting again is refused, in words',
        restarted ? restarted.message.slice(0, 56) : 'no error');

  fs.rmSync(output, {force: true});
  console.log(`\n${failures ? failures + ' CHECK(S) FAILED' : 'ALL CHECKS PASSED'}`);
  process.exit(failures ? 1 : 0);
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
