// Native-entry lifecycle tests run in fresh processes: Blink cannot restart
// after teardown, and a timeout must fail the suite rather than hang the host.
import assert from 'node:assert/strict';
import path from 'node:path';
import {cac} from 'cac';
import {execa} from 'execa';
import {resolve} from './lib/repo.ts';

const prelude = `
const assert = require('node:assert/strict');
const {createHash} = require('node:crypto');
const native = require(process.env.SHOT_ADDON);
const request = {file: process.env.SHOT_FIXTURE, allowFileAccess: true, width: 1280, height: 720};
const options = {resourceDir: require('node:path').dirname(process.env.SHOT_ADDON)};
const hash = image => createHash('sha256').update(image).digest('hex');
`;
const scenarios: Record<string, string> = {
  'parameter getter can destroy the engine safely': `
const engine = native.create(options);
const reentrant = {...request, get width() { native.destroy(engine); return 1280; }};
assert.throws(() => native.capture(engine, reentrant), /destroyed/);
console.log('completed');`,
  'cache diagnostics before and after engine initialization': `
(async () => {
  const fs = require('node:fs');
  const path = require('node:path');
  const dir = fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'shot-node-cache-'));
  try {
    assert.deepEqual(await native.cache(null, false, {cacheDir: dir}), []);
    assert.deepEqual(await native.cache(null, false, {cacheDir: dir}), []);
    const engine = native.create({...options, cacheDir: dir});
    assert.deepEqual(await native.cache(engine, false, {cacheDir: dir}), []);
    const cleared = await native.cache(engine, true, {cacheDir: dir});
    assert.equal(cleared.removed, -1); // whole-cache clear does not enumerate
    assert.equal(cleared.bytesAfter, 0);
    native.destroy(engine);
    console.log('completed');
  } finally { fs.rmSync(dir, {recursive: true, force: true}); }
})();`,
  'natural exit and pending Promise': `
const engine = native.create(options);
native.capture(engine, request).then(result => {
  assert(result.image.length > 0);
  assert.equal(typeof result.stats, 'object');
  console.log('completed');
});`,
  'FIFO, rejection recovery, destroy and Buffer lifetime': `
(async () => {
  const engine = native.create(options);
  const order = [];
  await assert.rejects(native.capture(engine, {...request, type: 'invalid'}), error => {
    assert.equal(error.constructor, Error);
    assert.match(error.message, /type must be one of/);
    return true;
  });
  const one = native.capture(engine, request).then(r => { order.push(1); return r; });
  const bad = native.capture(engine, {...request, selector: '#does-not-exist'}).then(
    () => assert.fail('should reject'), error => { assert.equal(typeof error.stats, 'object'); order.push(2); });
  const three = native.capture(engine, request).then(r => { order.push(3); return r; });
  native.destroy(engine); // must drain accepted work without needing JS callbacks
  const [a, , b] = await Promise.all([one, bad, three]);
  assert.deepEqual(order, [1, 2, 3]);
  assert.equal(hash(a.image), hash(b.image));
  const digest = hash(a.image);
  global.gc();
  assert.equal(hash(a.image), digest);
  assert.throws(() => native.capture(engine, request), /destroyed/);
  console.log('completed');
})();`,
  'libuv pool remains available during capture': `
(async () => {
  const http = require('node:http');
  const fs = require('node:fs/promises');
  const engine = native.create(options);
  await native.capture(engine, request);
  let response;
  let arrived;
  const received = new Promise(resolve => { arrived = resolve; });
  const server = http.createServer((req, res) => { response = res; arrived(); });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  let finished = false;
  const captured = native.capture(engine, {...request, file: 'http://127.0.0.1:' + server.address().port + '/', timeout: 10000})
    .then(result => { finished = true; return result; });
  await received;
  let timer;
  try {
    await Promise.race([
      fs.readFile(process.env.SHOT_FIXTURE),
      new Promise((_, reject) => { timer = setTimeout(() => reject(new Error('libuv pool blocked by capture')), 2000); }),
    ]);
    assert.equal(finished, false);
  } finally {
    clearTimeout(timer);
    response.end('<p>ready</p>');
  }
  await captured;
  await new Promise(resolve => server.close(resolve));
  console.log('completed');
})();`,
  'Worker termination with an in-flight native request': `
(async () => {
  const {Worker} = require('node:worker_threads');
  const http = require('node:http');
  let arrived;
  const received = new Promise(resolve => { arrived = resolve; });
  const server = http.createServer((req, res) => { arrived(); setTimeout(() => res.end('<p>done</p>'), 100); });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const source = 'const n = require(process.env.SHOT_ADDON); const e = n.create(' + JSON.stringify(options) + '); n.capture(e, {file: ' + JSON.stringify('http://127.0.0.1:' + server.address().port + '/') + ', timeout: 3000});';
  const worker = new Worker(source, {eval: true});
  await Promise.race([received, new Promise((_, reject) => worker.once('error', reject))]);
  await worker.terminate();
  await new Promise(resolve => server.close(resolve));
  assert.throws(() => native.create(options), /already had an engine/);
  console.log('completed');
})();`,
  'shared Runtime stop and restart': `
(async () => {
  const {Runtime} = await import(require('node:url').pathToFileURL(require('node:path').join(process.env.SHOT_PACKAGE, 'dist/index.js')).href);
  const a = new Runtime(); const b = new Runtime();
  a.start({resourceDir: options.resourceDir, cacheDir: null}); b.start();
  const input = {file: request.file, allowFileAccess: true};
  const [one, two] = await Promise.all([a.screenshot(input), b.screenshot(input)]);
  assert.equal(hash(one.image), hash(two.image));
  await a.stop();
  const three = await b.screenshot(input);
  a.start();
  assert.equal(hash(three.image), hash((await a.screenshot(input)).image));
  await a.stop(); await b.stop();
  console.log('completed');
})();`,
};

async function main(addon: string, node: string): Promise<void> {
  for (const [name, source] of Object.entries(scenarios)) {
    const result = await execa(node, ['--expose-gc', '-e', prelude + source], {
      env: {UV_THREADPOOL_SIZE: '1', SHOT_ADDON: resolve(addon),
        SHOT_FIXTURE: resolve('shot/testdata/render_corpus.html'), SHOT_PACKAGE: resolve('apps/demo/shotium')},
      timeout: 30000,
      reject: false,
    });
    assert.equal(result.exitCode, 0, `${name}: ${result.timedOut ? 'timed out' : 'failed'}\n${result.stdout}\n${result.stderr}`);
    assert.match(result.stdout, /completed/, name);
    console.log(`PASS ${name}`);
  }
}
const cli = cac('check-node-entry');
cli.command('[addon]', 'verify the GN-built Node entry in isolated processes')
    .option('--node <path>', 'Node executable for compatibility checks', {default: process.execPath})
    .action(async (addon = path.join('out', 'Shot', 'shotium.node'), options: {node: string}) => {
      try { await main(addon, resolve(options.node)); } catch (error) { console.error(error); process.exitCode = 1; }
    });
cli.help();
cli.parse();
