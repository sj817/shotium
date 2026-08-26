'use strict';

// Exercises the resident daemon: the pool that outlives the process that
// started it.
//
// node_check.cjs covers the in-process pool, and everything the daemon renders
// goes through that same pool -- so what is left to check here is the part that
// is genuinely new. That a client in another process finds the daemon instead
// of starting a second one; that one connection can have several requests in
// flight, which the worker protocol underneath cannot; that the daemon is the
// same renderer as the in-process pool rather than a second path that could
// drift from it; and that it goes away, both when asked and when left alone.
//
//   node tools/shot/daemon_check.cjs out/ShotWip/shotium.exe

const {execFileSync} = require('child_process');
const crypto = require('crypto');
const fs = require('fs');
const os = require('os');
const path = require('path');
const {pathToFileURL} = require('url');

const shotium = require('../../shotium');

const exe = path.resolve(process.argv[2] || 'out/Shot/shotium.exe');
const corpus = path.resolve('shot/testdata/render_corpus.html');
const entry = pathToFileURL(path.resolve('shotium/dist/index.js')).href;

// A second node process, which is what this check is actually about: a caller
// in a process that did not start the daemon has to find it rather than start
// another one. It is spelled out here rather than run from a script in the
// package because the package no longer ships one -- the command line is a
// binary of its own now -- and this is what a caller writes anyway.
//
// Its argument is one JSON blob for the same reason the daemon's own is: the
// endpoint is a hash of the configuration, so a value mangled on a Windows
// command line would send it looking for a daemon nobody is running.
const SECOND_PROCESS = `
import {writeFileSync} from 'node:fs';
import shotium from ${JSON.stringify(entry)};

const [config, request, output] = JSON.parse(process.argv[1]);
const started = Date.now();
const image = await shotium.daemon.screenshot({...request, daemon: config});
writeFileSync(output, image);
process.stdout.write(JSON.stringify(
    {bytes: image.length, elapsedMs: Date.now() - started}));
`;

// A name of this run's own, so that a daemon left over from an earlier run --
// or a developer's own, started by hand -- can never be mistaken for the one
// under test.
const config = {
  binary: exe,
  workers: 2,
  name: `check-${process.pid}`,
  cacheDir: path.join(os.tmpdir(), `shotium-daemon-check-${process.pid}`),
};

let failures = 0;

function check(ok, label, detail = '') {
  console.log(`  ${ok ? 'PASS' : 'FAIL'}  ${label}${detail ? '   ' + detail : ''}`);
  if (!ok) {
    failures += 1;
  }
}

function sha256(buffer) {
  return crypto.createHash('sha256').update(buffer).digest('hex');
}

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

const request = {
  file: corpus,
  viewport: {width: 1248, height: 1320},
  allowFileAccess: true,
};

async function main() {
  console.log('== nothing is running ==');
  const before = await shotium.daemon.status(config);
  check(before.running === false, 'status reports no daemon at the endpoint',
        before.endpoint);

  console.log('\n== the first request starts one ==');
  const coldStarted = Date.now();
  const client = await shotium.daemon.connect(config);
  const cold = Date.now() - coldStarted;
  const first = await client.screenshot(request);
  check(Buffer.isBuffer(first) &&
            first.subarray(0, 8).equals(Buffer.from('89504e470d0a1a0a', 'hex')),
        'a screenshot comes back as a PNG');
  const started = await client.status();
  check(started.ok && started.workers === 2,
        'and the daemon is up with the workers it was asked for',
        `pid ${started.pid}, ${cold}ms to connect`);

  console.log('\n== it is the same renderer as the in-process pool ==');
  const runtime = new shotium.Runtime();
  runtime.start({binary: exe, workers: 1, cacheDir: null});
  const inProcess = await runtime.screenshot(request);
  await runtime.stop();
  check(sha256(inProcess) === sha256(first),
        'the daemon and the in-process pool produce the same bytes',
        sha256(first).slice(0, 16));

  console.log('\n== a second process finds it instead of starting one ==');
  const output = path.join(os.tmpdir(), `shotium-daemon-check-${process.pid}.png`);
  const secondStarted = Date.now();
  const printed = execFileSync(
      process.execPath,
      ['--input-type=module', '-e', SECOND_PROCESS,
       JSON.stringify([config, request, output])],
      {encoding: 'utf8'});
  const secondWall = Date.now() - secondStarted;
  const reported = JSON.parse(printed);
  const after = await client.status();
  check(after.pid === started.pid,
        'the daemon that answered is the one that was already up',
        `pid ${after.pid}`);
  check(after.served > started.served,
        'and it counted the request',
        `${started.served} -> ${after.served}`);
  const written = fs.readFileSync(output);
  check(sha256(written) === sha256(first),
        'the image it wrote is the same image',
        `${sha256(written).slice(0, 16)} vs ${sha256(first).slice(0, 16)}`);
  check(reported.elapsedMs < 1000,
        'a whole second process rendered in well under a second',
        `${reported.elapsedMs}ms in-process, ${secondWall}ms including node`);

  console.log('\n== one connection, several requests at once ==');
  // The worker protocol cannot do this: a worker renders one document at a
  // time. The pool in the middle is what makes it possible, and `id` is what
  // makes the answers findable.
  const batchStarted = Date.now();
  const batch = await Promise.all(
      Array.from({length: 10}, () => client.screenshot(request)));
  check(batch.length === 10 && batch.every((png) => sha256(png) === sha256(first)),
        'ten concurrent requests all come back, all identical',
        `${Date.now() - batchStarted}ms for ten over 2 workers`);

  console.log('\n== a failed request is not a dead daemon ==');
  let rejected = null;
  try {
    await client.screenshot({file: corpus, selector: '#nothing-here'});
  } catch (error) {
    rejected = error;
  }
  check(rejected !== null && /no element matches/.test(rejected.message),
        'a selector that matches nothing rejects with the worker\'s message',
        rejected ? rejected.message.slice(0, 60) : 'no error');
  const afterError = await client.screenshot(request);
  check(sha256(afterError) === sha256(first), 'and the next request still works');

  console.log('\n== shutdown ==');
  client.close();
  const stopped = await shotium.daemon.stop(config);
  check(stopped.stopped === true, 'stop() shuts the daemon down');
  await sleep(200);
  const gone = await shotium.daemon.status(config);
  check(gone.running === false, 'and the endpoint stops answering');

  console.log('\n== it also goes away on its own ==');
  const idle = {...config, name: `${config.name}-idle`, idleTimeoutMs: 1500};
  const idleClient = await shotium.daemon.connect(idle);
  await idleClient.screenshot(request);
  idleClient.close();
  // Idle is "nobody connected and nothing rendering", so the clock only starts
  // once the last client has gone.
  const held = await shotium.daemon.status(idle);
  check(held.running === true, 'a daemon with an idle timeout is up while used');
  await sleep(3500);
  const expired = await shotium.daemon.status(idle);
  check(expired.running === false, 'and exits after its idle timeout');

  fs.rmSync(config.cacheDir, {recursive: true, force: true});
  fs.rmSync(output, {force: true});
  console.log(`\n${failures ? failures + ' CHECK(S) FAILED' : 'ALL CHECKS PASSED'}`);
  process.exit(failures ? 1 : 0);
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
