'use strict';

// Exercises the shotium package against a real shotium.exe.
//
// serve_check.py and net_check.py cover the worker. This covers the half that
// only exists in JavaScript: the pool, the queue, retry, and the claim the
// whole out-of-process design rests on -- that a worker can be killed
// mid-request without taking anything else with it.
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
//   node tools/shot/node_check.cjs out/ShotSize/shotium.exe

const assert = require('assert');
const os = require('os');
const path = require('path');

const shotium = require('../../shotium');

const exe = path.resolve(process.argv[2] || 'out/ShotSize/shotium.exe');
const corpus = path.resolve('shot/testdata/render_corpus.html');
const features = path.resolve('shot/testdata/features.html');

let failures = 0;

function check(ok, label, detail = '') {
  console.log(`  ${ok ? 'PASS' : 'FAIL'}  ${label}${detail ? '   ' + detail : ''}`);
  if (!ok) {
    failures += 1;
  }
}

function sha256(buffer) {
  return require('crypto').createHash('sha256').update(buffer).digest('hex');
}

async function main() {
  const runtime = new shotium.Runtime();
  const events = [];
  for (const name of ['ready', 'crash', 'timeout', 'worker-restart']) {
    runtime.on(name, (payload) => events.push({name, ...payload}));
  }

  runtime.start({
    binary: exe,
    workers: 3,
    cacheDir: path.join(os.tmpdir(), 'shotium-node-check'),
  });
  check(runtime.running, 'the runtime starts');
  check(events.some((e) => e.name === 'ready'), 'and says so');

  console.log('\n== one screenshot ==');
  const first = await runtime.screenshot({
    file: corpus,
    viewport: {width: 1248, height: 1320},
    allowFileAccess: true,
  });
  check(Buffer.isBuffer(first), 'a screenshot comes back as a Buffer');
  check(first.subarray(0, 8).equals(Buffer.from('89504e470d0a1a0a', 'hex')),
        'and it is a PNG');
  console.log(`        sha256 ${sha256(first).slice(0, 32)}`);

  console.log('\n== concurrency ==');
  // More requests than workers, so the queue has to hold some of them, and
  // every worker has to render more than one document.
  const started = Date.now();
  const many = await Promise.all(Array.from({length: 9}, () => runtime.screenshot({
    file: corpus,
    viewport: {width: 1248, height: 1320},
    allowFileAccess: true,
  })));
  check(many.length === 9, 'nine requests over three workers all complete');
  check(many.every((png) => sha256(png) === sha256(first)),
        'and every one is byte-identical to the first');
  console.log(`        ${Date.now() - started}ms for nine`);

  console.log('\n== the geometry options reach the worker ==');
  const clipped = await runtime.screenshot({
    file: features,
    viewport: {width: 400, height: 300},
    clip: {x: 40, y: 60, width: 200, height: 120},
    allowFileAccess: true,
  });
  check(clipped.readUInt32BE(16) === 200 && clipped.readUInt32BE(20) === 120,
        'clip arrives as a 200x120 image',
        `${clipped.readUInt32BE(16)}x${clipped.readUInt32BE(20)}`);

  const written = path.join(os.tmpdir(), 'shotium-node-check.png');
  const viaPath = await runtime.screenshot({
    file: features,
    viewport: {width: 400, height: 300},
    path: written,
    allowFileAccess: true,
  });
  check(viaPath === null, 'a request with `path` resolves to null');
  check(require('fs').existsSync(written), 'and the worker wrote the file');

  console.log('\n== errors are errors, not crashes ==');
  await assert.rejects(
      () => runtime.screenshot({file: features, selector: '#nothing-here'}),
      /no element matches/, 'a selector that matches nothing rejects');
  check(true, 'a failed request rejects with the worker\'s own message');
  const afterError = await runtime.screenshot({
    file: corpus,
    viewport: {width: 1248, height: 1320},
    allowFileAccess: true,
  });
  check(sha256(afterError) === sha256(first),
        'and the pool still works afterwards');

  let threw = null;
  try {
    await runtime.screenshot({file: corpus, fullpage: true});
  } catch (error) {
    threw = error;
  }
  check(threw instanceof TypeError && /fullpage/.test(threw.message),
        'a misspelled option is refused rather than dropped',
        threw ? threw.message : 'no error');

  console.log('\n== a killed worker does not take the pool with it ==');
  // This is the whole reason rendering is out of process. Kill one worker
  // mid-flight and the request it owed must come back on another one, because
  // retry cannot tell a crash from a hang.
  const before = events.filter((e) => e.name === 'worker-restart').length;
  const pending = runtime.screenshot({
    file: corpus,
    viewport: {width: 1248, height: 1320},
    allowFileAccess: true,
    retry: 2,
  });
  // No delay before the kill: screenshot() runs synchronously up to its first
  // await, so by the time it has returned a promise the request is already
  // written to a worker's stdin and that worker is marked busy. Waiting would
  // be a race in the other direction -- a viewport render of this corpus takes
  // about 17ms, so a sleep long enough to be reliable is long enough for the
  // answer to have arrived.
  const victim = runtime._pool._slots.find((w) => w.busy);
  if (victim) {
    victim.kill();
  }
  const recovered = await pending;
  check(victim !== undefined, 'a worker was busy when it was killed');
  check(sha256(recovered) === sha256(first),
        'the retried request produced the same image');
  check(events.filter((e) => e.name === 'worker-restart').length > before,
        'and the pool refilled the slot');

  console.log('\n== shutdown ==');
  await runtime.stop();
  check(!runtime.running, 'stop() leaves the runtime down');

  console.log(`\n${failures ? failures + ' CHECK(S) FAILED' : 'ALL CHECKS PASSED'}`);
  process.exit(failures ? 1 : 0);
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
