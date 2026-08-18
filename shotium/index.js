'use strict';

const {EventEmitter} = require('events');
const os = require('os');
const path = require('path');

const {Pool, defaultCacheDir} = require('./lib/pool');

const DEFAULT_TIMEOUT_MS = 30000;
// How much longer than the page's own deadline the supervisor waits before
// deciding the worker is not going to answer at all. The worker fails a slow
// page by itself and replies; this margin covers process startup and the
// encode, and firing it means something worse than a slow page.
const SUPERVISOR_MARGIN_MS = 10000;

function defaultBinary() {
  if (process.env.SHOTIUM_BINARY) {
    return process.env.SHOTIUM_BINARY;
  }
  const name = process.platform === 'win32' ? 'shot.exe' : 'shot';
  return path.join(__dirname, 'bin', name);
}

// Everything the worker understands, and nothing else. An unknown field is a
// typo, and a typo that is silently dropped is a screenshot that quietly
// ignored what was asked for -- so this rejects rather than filters.
const WIRE_FIELDS = new Set([
  'file',
  'type',
  'fullPage',
  'selector',
  'quality',
  'scale',
  'omitBackground',
  'path',
  'pageGotoParams',
  'clip',
  'viewport',
  'allowFileAccess',
]);

function toRequest(options) {
  if (!options || typeof options !== 'object') {
    throw new TypeError('shotium: screenshot(options) needs an object');
  }
  if (typeof options.file !== 'string' || options.file.length === 0) {
    throw new TypeError('shotium: options.file is required');
  }

  const request = {};
  for (const [key, value] of Object.entries(options)) {
    if (value === undefined) {
      continue;
    }
    // retry is the supervisor's, not the worker's: it decides how many times a
    // request is re-sent, which is not something the worker could act on.
    if (key === 'retry') {
      continue;
    }
    if (!WIRE_FIELDS.has(key)) {
      throw new TypeError(`shotium: unknown option "${key}"`);
    }
    request[key] = value;
  }

  // The viewport is flattened because the worker takes width and height at the
  // top level: it is one screenshot's frame, not a nested object on the wire.
  if (request.viewport) {
    const {width, height} = request.viewport;
    delete request.viewport;
    if (width !== undefined) {
      request.width = width;
    }
    if (height !== undefined) {
      request.height = height;
    }
  }
  return request;
}

function timeoutFor(options) {
  const timeout = options.pageGotoParams && options.pageGotoParams.timeout;
  return typeof timeout === 'number' ? timeout : DEFAULT_TIMEOUT_MS;
}

// The library's one runtime: a pool of worker processes plus its lifecycle.
//
// It is a singleton because the expensive part is the processes, and a second
// runtime would double them for no gain. Anyone who genuinely wants two can
// construct a Runtime directly.
class Runtime extends EventEmitter {
  constructor() {
    super();
    this._pool = null;
    this._options = null;
  }

  get running() {
    return this._pool !== null;
  }

  // Starts the pool. Safe to call twice; the second call is a no-op so that
  // library code can call it defensively.
  //
  //   binary    path to shot.exe (default: $SHOTIUM_BINARY, then ./bin/shot.exe)
  //   workers   how many processes (default: half the cores, at least one)
  //   cacheDir  root for the per-worker HTTP disk caches; null disables caching
  //   args      extra flags for every worker
  start(options = {}) {
    if (this._pool) {
      return this;
    }
    const resolved = {
      binary: options.binary || defaultBinary(),
      workers: options.workers ||
          Math.max(1, Math.floor((os.cpus().length || 2) / 2)),
      cacheDir: options.cacheDir === null ?
          null :
          (options.cacheDir || defaultCacheDir()),
      args: options.args || [],
    };
    this._options = resolved;
    this._pool = new Pool(resolved);
    for (const event
             of ['ready', 'exit', 'crash', 'timeout', 'worker-restart',
                 'stderr']) {
      this._pool.on(event, (payload) => this.emit(event, payload));
    }
    this._pool.start();
    return this;
  }

  async stop() {
    if (!this._pool) {
      return;
    }
    const pool = this._pool;
    this._pool = null;
    await pool.stop();
  }

  // Renders one screenshot. Resolves to the encoded image, or to null when
  // `path` was given and the worker wrote the file itself.
  async screenshot(options) {
    // Validate before starting anything. A malformed request should not cost a
    // pool of worker processes to discover, and toRequest() is the only check
    // that can be made without one.
    const request = toRequest(options);
    if (!this._pool) {
      this.start();
    }
    const retry = typeof options.retry === 'number' ? options.retry : 0;
    const result = await this._pool.submit(request, {
      timeout: timeoutFor(options) + SUPERVISOR_MARGIN_MS,
      retry,
    });
    return result.image;
  }
}

const runtime = new Runtime();

module.exports = {
  Runtime,
  runtime,
  screenshot: (options) => runtime.screenshot(options),
};
