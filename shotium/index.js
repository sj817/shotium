import {EventEmitter} from 'node:events';

import * as client from './lib/client.js';
import {Pool} from './lib/pool.js';
import {resolveStartOptions} from './lib/config.js';
import {SUPERVISOR_MARGIN_MS, timeoutFor, toRequest} from './lib/request.js';

// The library's one runtime: a pool of worker processes plus its lifecycle.
//
// It is a singleton because the expensive part is the processes, and a second
// runtime would double them for no gain. Anyone who genuinely wants two can
// construct a Runtime directly.
//
// Its pool lives and dies with this process. `daemon` below is the same pool
// behind a socket, for callers whose process does not live long enough to be
// worth starting one.
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
  //   binary    path to shotium.exe (default: $SHOTIUM_BINARY, then the
  //             platform package, then ./bin/shotium.exe)
  //   workers   how many processes (default: half the cores, 1..4)
  //   cacheDir  root for the per-worker HTTP disk caches; null disables caching
  //   args      extra flags for every worker
  start(options = {}) {
    if (this._pool) {
      return this;
    }
    const resolved = resolveStartOptions(options);
    this._options = resolved;
    this._pool = new Pool(resolved);
    for (const event
             of ['ready', 'exit', 'crash', 'timeout', 'worker-restart',
                 'worker-error', 'stderr']) {
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

const screenshot = (options) => runtime.screenshot(options);

// The resident pool: workers that outlive the process that started them, for
// callers that are short-lived themselves. See lib/daemon.js.
const daemon = {
  connect: client.connect,
  screenshot: client.screenshot,
  start: client.start,
  status: client.status,
  stop: client.stop,
};

export {Runtime, runtime, screenshot, daemon};

// A default as well as the names, because `import shotium from` is what a
// caller coming from `require` writes first, and the two have to be the same
// object rather than two views that drift.
export default {Runtime, runtime, screenshot, daemon};
