import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import type {AddressInfo} from 'node:net';
import {pathToFileURL} from 'node:url';
import waitOn from 'wait-on';
import {FIXTURE_ROOT} from './constants.ts';

const CONTENT_TYPES = new Map([
  ['.css', 'text/css; charset=utf-8'],
  ['.html', 'text/html; charset=utf-8'],
  ['.svg', 'image/svg+xml'],
  ['.woff2', 'font/woff2'],
]);

function safeFixturePath(urlPath) {
  const relative = decodeURIComponent(urlPath).replace(/^\/+/, '');
  const candidate = path.resolve(FIXTURE_ROOT, relative);
  return candidate.startsWith(`${path.resolve(FIXTURE_ROOT)}${path.sep}`) ? candidate : null;
}

export async function startFixtureServer() {
  const startedRequests = new Set<string>();
  const server = http.createServer(async (request, response) => {
    const url = new URL(request.url || '/', 'http://127.0.0.1');
    if (url.pathname === '/healthz') {
      response.writeHead(200, {'content-type': 'text/plain'}).end('ok');
      return;
    }
    if (url.pathname === '/redirect') {
      response.writeHead(302, {location: '/simple.html'}).end();
      return;
    }
    if (url.pathname === '/request-status') {
      const token = url.searchParams.get('token') || '';
      const started = startedRequests.delete(token);
      response.writeHead(200, {'content-type': 'application/json'}).end(JSON.stringify({started}));
      return;
    }
    if (url.pathname === '/slow') {
      const token = url.searchParams.get('token');
      if (token) startedRequests.add(token);
      const delay = Math.min(30_000, Math.max(0, Number(url.searchParams.get('ms')) || 2000));
      await new Promise<void>((resolve) => {
        const timer = setTimeout(done, delay);
        function done() {
          clearTimeout(timer);
          request.off('aborted', done);
          response.off('close', done);
          resolve();
        }
        request.once('aborted', done);
        response.once('close', done);
      });
      if (request.aborted || response.destroyed) return;
      response.writeHead(200, {'content-type': 'text/html'}).end('<!doctype html><title>slow</title><p>ready</p>');
      return;
    }
    const file = safeFixturePath(url.pathname);
    if (!file || !fs.existsSync(file) || !fs.statSync(file).isFile()) {
      response.writeHead(404, {'content-type': 'text/plain'}).end('not found');
      return;
    }
    response.writeHead(200, {
      'content-type': CONTENT_TYPES.get(path.extname(file)) || 'application/octet-stream',
      'cache-control': 'no-store',
    });
    fs.createReadStream(file).pipe(response);
  });
  await new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(0, '127.0.0.1', () => resolve(undefined));
  });
  const address = server.address() as AddressInfo;
  const baseUrl = `http://127.0.0.1:${address.port}`;
  await waitOn({resources: [`${baseUrl}/healthz`], timeout: 10_000, interval: 100});
  return {
    baseUrl,
    close: () => new Promise<void>((resolve, reject) =>
      server.close((error) => error ? reject(error) : resolve())),
  };
}

export function loadCases(baseUrl, limit = Number.POSITIVE_INFINITY) {
  const definitions = JSON.parse(fs.readFileSync(path.join(FIXTURE_ROOT, 'cases.json'), 'utf8'));
  return definitions.slice(0, limit).map((entry) => ({
    ...entry,
    url: entry.loopback_http ? `${baseUrl}${entry.route || `/${entry.file}`}` :
      pathToFileURL(path.join(FIXTURE_ROOT, entry.file)).href,
  }));
}
