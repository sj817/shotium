// Exercises shotium's network stack against a local server.
//
// check-serve covers the protocol and the capture geometry, both over file:.
// This covers the part that only exists once //net is linked in: that an http
// URL is fetched at all, that redirects are followed, that the disk cache is
// used across worker processes, and that networkidle waits for something a
// plain load would not.
//
// The strongest check here is the last one in section 1: the same document,
// served over http and read off the disk, must render to the same bytes. The
// transport is not supposed to be visible in the picture, and comparing
// digests is how that stops being an assumption.
//
//   pnpm verify:net out/Shot/shotium.exe
//
// Relative paths are resolved against the repository root.

import {copyFileSync, mkdtempSync, readFileSync, rmSync, writeFileSync} from 'node:fs';
import http from 'node:http';
import type {AddressInfo} from 'node:net';
import os from 'node:os';
import path from 'node:path';

import {cac} from 'cac';

import {resolve, sleep} from './lib/repo.ts';
import {Checks, sha256} from './lib/report.ts';
import {ServeWorker} from './lib/serve.ts';

const INDEX_HTML = `<!DOCTYPE html>
<html lang="en">
<meta charset="utf-8">
<title>shot network corpus</title>
<link rel="stylesheet" href="style.css">
<div id="box"></div>
<img src="pic.png" width="64" height="64">
</html>
`;

// Kept separate from index.html because index.html is also rendered off the
// disk, and a redirect is something only the server can do: the file: version
// would come back with a broken image and the comparison would be measuring
// the missing file rather than the transport.
const REDIRECT_HTML = `<!DOCTYPE html>
<html lang="en">
<meta charset="utf-8">
<title>shot redirect corpus</title>
<img src="r/pic.png" width="64" height="64">
</html>
`;

const STYLE_CSS = `
html, body { margin: 0; padding: 0; background: #ffffff; }
#box { width: 200px; height: 100px; background: #3366cc; }
img { display: block; image-rendering: pixelated; }
`;

const CONTENT_TYPES: Record<string, string> = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css',
  '.png': 'image/png',
};

interface Fixture {
  counts: Record<string, number>;
  activeRedirectTargets: number;
  peakRedirectTargets: number;
}

function serve(root: string, state: Fixture): Promise<http.Server> {
  const server = http.createServer(async (req, res) => {
    const url = req.url ?? '/';
    state.counts[url] = (state.counts[url] ?? 0) + 1;
    const port = (server.address() as AddressInfo).port;

    if (url.startsWith('/limit-source/')) {
      // Both 127.0.0.1 and localhost source URLs end here. They converge on
      // localhost so that the worker has to transfer a per-host slot when the
      // source was 127.0.0.1.
      const suffix = url.slice('/limit-source/'.length);
      res.writeHead(302, {Location: `http://localhost:${port}/limit-target/${suffix}`, 'Content-Length': '0'});
      res.end();
      return;
    }
    if (url.startsWith('/limit-target/')) {
      state.activeRedirectTargets += 1;
      state.peakRedirectTargets = Math.max(state.peakRedirectTargets, state.activeRedirectTargets);
      try {
        // Long enough for another redirect to overlap if it bypassed the
        // destination host's slot.
        await sleep(200);
        const body = readFileSync(path.join(root, 'pic.png'));
        res.writeHead(200, {'Content-Type': 'image/png', 'Content-Length': String(body.length), 'Cache-Control': 'no-store'});
        await new Promise<void>((done) => res.end(body, done));
      } finally {
        state.activeRedirectTargets -= 1;
      }
      return;
    }
    if (url === '/r/pic.png') {
      res.writeHead(302, {Location: '/pic.png', 'Content-Length': '0'});
      res.end();
      return;
    }
    if (url === '/slow.css') {
      // Arrives well after the document has finished parsing, which is what
      // separates networkidle from load.
      await sleep(700);
      const body = Buffer.from('#box { background: #cc3366; }');
      res.writeHead(200, {'Content-Type': 'text/css', 'Content-Length': String(body.length), 'Cache-Control': 'no-store'});
      res.end(body);
      return;
    }

    const name = url.replace(/^\/+/, '');
    const file = path.join(root, name);
    let body: Buffer;
    try {
      body = readFileSync(file);
    } catch {
      res.writeHead(404);
      res.end();
      return;
    }
    res.writeHead(200, {
      'Content-Type': CONTENT_TYPES[path.extname(name)] ?? 'application/octet-stream',
      'Content-Length': String(body.length),
      // Long enough that a second worker must hit the cache rather than the
      // network, and without must-revalidate so there is no conditional
      // request either.
      'Cache-Control': 'max-age=3600',
    });
    res.end(body);
  });
  return new Promise((done) => server.listen(0, '127.0.0.1', () => done(server)));
}

async function main(exeArg: string, httpsProbe: string): Promise<number> {
  const exe = resolve(exeArg);
  const checks = new Checks();
  const err = (h: {error?: string}) => h.error ?? '';
  const viewport = {width: 400, height: 300};

  const root = mkdtempSync(path.join(os.tmpdir(), 'shot-net-'));
  const cache = mkdtempSync(path.join(os.tmpdir(), 'shot-cache-'));
  const state: Fixture = {counts: {}, activeRedirectTargets: 0, peakRedirectTargets: 0};
  let server: http.Server | null = null;
  try {
    writeFileSync(path.join(root, 'index.html'), INDEX_HTML);
    writeFileSync(path.join(root, 'style.css'), STYLE_CSS);
    writeFileSync(path.join(root, 'redirect.html'), REDIRECT_HTML);
    copyFileSync(resolve('shot/testdata/checker.png'), path.join(root, 'pic.png'));

    server = await serve(root, state);
    const port = (server.address() as AddressInfo).port;
    const base = `http://127.0.0.1:${port}`;
    console.log(`\nserving ${root} at ${base}`);

    checks.section('1. an http document, its subresources and a redirect');
    let worker = new ServeWorker(exe, [`--cache-dir=${cache}`]);
    let [header, httpPng] = await worker.ask({file: `${base}/index.html`, ...viewport});
    checks.check(header.ok === true, 'the page renders over http', err(header));
    let counts = {...state.counts};
    checks.check(counts['/index.html'] === 1, 'the document was fetched', String(counts['/index.html']));
    checks.check(counts['/style.css'] === 1, 'the stylesheet was fetched', String(counts['/style.css']));
    checks.check(counts['/pic.png'] === 1, 'the image was fetched', String(counts['/pic.png']));

    [header] = await worker.ask({file: `${base}/redirect.html`, ...viewport});
    checks.check(header.ok === true, 'a redirecting image renders', err(header));
    counts = {...state.counts};
    checks.check(counts['/r/pic.png'] === 1, 'the redirecting URL was requested', String(counts['/r/pic.png']));
    checks.check((counts['/pic.png'] ?? 0) >= 1, 'and the redirect was followed to its target', String(counts['/pic.png']));

    // The same bytes off the disk. Different URL, same picture -- if these
    // differ, something about the transport is reaching the pixels.
    let filePng: Buffer;
    [header, filePng] = await worker.ask({file: path.join(root, 'index.html'), allowFileAccess: true, ...viewport});
    checks.check(header.ok === true, 'the same page renders over file:', err(header));
    checks.check(sha256(httpPng) === sha256(filePng), 'http and file: produce byte-identical images');
    await worker.close();

    checks.section('2. the disk cache survives the worker that filled it');
    const before = {...state.counts};
    worker = new ServeWorker(exe, [`--cache-dir=${cache}`]);
    let cachedPng: Buffer;
    [header, cachedPng] = await worker.ask({file: `${base}/index.html`, ...viewport});
    checks.check(header.ok === true, 'a second worker renders it', err(header));
    const after = {...state.counts};
    checks.check(after['/style.css'] === before['/style.css'], 'and did not re-request the cacheable stylesheet', `${before['/style.css']} -> ${after['/style.css']}`);
    checks.check(after['/pic.png'] === before['/pic.png'], 'or the cacheable image', `${before['/pic.png']} -> ${after['/pic.png']}`);
    checks.check(sha256(cachedPng) === sha256(httpPng), 'and the cached render is identical to the uncached one');
    await worker.close();

    checks.section('3. networkidle waits for what load does not');
    // A stylesheet that arrives late changes the box from blue to pink. A
    // render that stops at `load` may or may not have it; one that waits for
    // the network to go quiet must.
    writeFileSync(path.join(root, 'late.html'), INDEX_HTML.replace(
        '<link rel="stylesheet" href="style.css">',
        '<link rel="stylesheet" href="style.css"><link rel="stylesheet" href="slow.css">'));
    worker = new ServeWorker(exe, [`--cache-dir=${cache}`]);
    let idlePng: Buffer;
    [header, idlePng] = await worker.ask({file: `${base}/late.html`, pageGotoParams: {waitUntil: 'networkidle'}, ...viewport});
    checks.check(header.ok === true, 'networkidle renders', err(header));
    checks.check(sha256(idlePng) !== sha256(httpPng), 'and the late stylesheet is in the picture');
    await worker.close();

    checks.section("4. redirects keep the destination host's concurrency limit");
    // Two source hostnames converge on localhost. With a limit of one, the
    // 127.0.0.1 redirect has to give back its old slot and queue behind any
    // localhost request already serving its final response.
    const images = Array.from({length: 8}, (_, i) => {
      const source = i % 2 === 0 ? '127.0.0.1' : 'localhost';
      return `<img src="http://${source}:${port}/limit-source/${i}.png" width="64" height="64">`;
    });
    writeFileSync(path.join(root, 'redirect-limit.html'), `<!doctype html><body>${images.join('')}</body>`);
    state.activeRedirectTargets = 0;
    state.peakRedirectTargets = 0;
    worker = new ServeWorker(exe, [], {...process.env, SHOT_FETCH_CONCURRENCY: '1'});
    [header] = await worker.ask({file: `${base}/redirect-limit.html`, pageGotoParams: {waitUntil: 'networkidle'}, ...viewport});
    checks.check(header.ok === true, 'the converging redirects render', err(header));
    checks.check(state.peakRedirectTargets === 1, 'only one final response ran at its host', `peak ${state.peakRedirectTargets}`);
    await worker.close();

    checks.section('5. https, against a real server');
    worker = new ServeWorker(exe, [`--cache-dir=${cache}`]);
    let payload: Buffer;
    [header, payload] = await worker.ask({file: httpsProbe, pageGotoParams: {timeout: 15000}, ...viewport});
    if (header.ok) {
      checks.check(payload.length > 0, `${httpsProbe} renders over TLS`, `${payload.length} bytes`);
    } else {
      // No internet is not a failure of this binary, and pretending it is
      // would make the check useless on a machine that is offline.
      checks.skip(`${httpsProbe} could not be reached`, err(header));
    }
    await worker.close();
  } finally {
    server?.closeAllConnections();
    await new Promise<void>((done) => server ? server.close(() => done()) : done());
    rmSync(root, {recursive: true, force: true});
    rmSync(cache, {recursive: true, force: true});
  }
  return checks.finish();
}

const cli = cac('check-net');
cli.command('<exe>', "exercise shotium's network stack against a local server")
    .option('--https-probe <url>', 'a public https URL, to check TLS end to end', {default: 'https://example.com'})
    .action(async (exe: string, options: {httpsProbe: string}) => {
      process.exitCode = await main(exe, options.httpsProbe);
    });
cli.help();
cli.parse();
