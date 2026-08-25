'use strict';

// Exercises the native engine: shot in this process, over the C ABI.
//
// node_check.cjs and daemon_check.cjs cover the worker pool and the socket in
// front of it. Neither says anything about this path, which shares no code
// with them below toRequest() -- it reaches blink through shot_api.h instead
// of through a pipe.
//
// So what is checked here is what the seam could get wrong. That the engine
// starts at all; that it renders the same bytes the executable does, which is
// the only thing that makes it the same engine rather than a second one; that
// a failed request comes back as a rejected promise rather than as a crash,
// because a crash in this design takes the host process with it; that
// concurrent callers are serialised rather than trampling one blink; that the
// second engine is refused; that a purge changes no pixels; and that stopping
// leaves nothing behind.
//
//   node tools/shot/native_check.cjs out/ShotWip/shot.exe
//
// The argument is the *executable*, which is what the other suites take and
// what this one compares against. The library and the addon are found where
// shotium/native.js looks for them.

const {execFileSync} = require('child_process');
const crypto = require('crypto');
const fs = require('fs');
const os = require('os');
const path = require('path');

const {NativeRuntime, native} = require('../../shotium/native');

const exe = path.resolve(process.argv[2] || 'out/Shot/shot.exe');
const corpus = path.resolve('shot/testdata/render_corpus.html');
const output = path.join(os.tmpdir(), `shot-native-check-${process.pid}.png`);

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
  viewport: {width: 1280, height: 720},
  allowFileAccess: true,
};

async function main() {
  console.log(`native engine, against ${exe}\n`);

  console.log('== it starts and renders ==');
  native.start({cacheDir: null});
  check(native.running === true, 'the engine is up');
  const first = await native.screenshot(request);
  check(Buffer.isBuffer(first) && first.length > 0, 'a screenshot comes back',
        `${first.length} bytes`);
  check(first.subarray(0, 8).equals(Buffer.from([137, 80, 78, 71, 13, 10, 26, 10])),
        'and it is a PNG');

  console.log('\n== it is the same renderer as the executable ==');
  // Not "close enough": identical. Both go through shot::Capture, and the only
  // way these bytes differ is if one of the two paths has grown a difference
  // in what it asks for -- which is exactly the drift this checks for.
  execFileSync(exe, [
    corpus, '-o', output, '--width=1280', '--height=720', '--allow-file-access',
  ]);
  const fromExe = fs.readFileSync(output);
  check(sha(fromExe) === sha(first), 'byte for byte what shot.exe produces',
        sha(first).slice(0, 16));

  console.log('\n== a bad request is an error, not a crash ==');
  let rejected = null;
  try {
    await native.screenshot({file: path.join(os.tmpdir(), 'shot-native-check-nope.html'),
                             allowFileAccess: true});
  } catch (error) {
    rejected = error;
  }
  check(rejected instanceof Error, 'a missing document rejects');
  check(rejected && /could not read/i.test(rejected.message),
        'and says what it could not read',
        rejected ? rejected.message.slice(0, 48) : '');
  const afterError = await native.screenshot(request);
  check(sha(afterError) === sha(first), 'the engine still works afterwards');

  console.log('\n== concurrent callers are serialised, not raced ==');
  // Blink renders one document at a time. Four callers who do not know that
  // must still get four correct answers rather than four interleaved ones.
  const many = await Promise.all(
      Array.from({length: 4}, () => native.screenshot(request)));
  check(many.every((image) => sha(image) === sha(first)),
        'four at once give four identical images');

  console.log('\n== there is only ever one engine ==');
  let refused = null;
  try {
    new NativeRuntime().start({cacheDir: null});
  } catch (error) {
    refused = error;
  }
  check(refused instanceof Error, 'a second engine is refused');
  check(refused && /once/i.test(refused.message),
        'and says why rather than failing obscurely',
        refused ? refused.message.slice(0, 48) : '');

  console.log('\n== purging costs nothing but memory ==');
  native.purge({releaseWorkingSet: true});
  const afterPurge = await native.screenshot(request);
  check(sha(afterPurge) === sha(first), 'the purge changed no pixels');

  console.log('\n== and it puts itself away ==');
  await native.stop();
  check(native.running === false, 'stop() leaves the engine down');

  fs.rmSync(output, {force: true});
  console.log(`\n${failures ? failures + ' CHECK(S) FAILED' : 'ALL CHECKS PASSED'}`);
  process.exit(failures ? 1 : 0);
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
