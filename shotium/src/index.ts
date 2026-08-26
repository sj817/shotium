import {EventEmitter} from 'node:events';

import * as client from './lib/client.js';
import type {DaemonClient} from './lib/client.js';
import {resolveStartOptions} from './lib/config.js';
import {Pool} from './lib/pool.js';
import {SUPERVISOR_MARGIN_MS, timeoutFor, toRequest} from './lib/request.js';
import type {
  DaemonOptions,
  DaemonStatus,
  ScreenshotOptions,
  StartOptions,
  WorkerEvent,
} from './types.js';

export type {
  Clip,
  DaemonOptions,
  DaemonStatus,
  PageGotoParams,
  PurgeOptions,
  ScreenshotOptions,
  StartOptions,
  Viewport,
  WorkerEvent,
} from './types.js';
export type {DaemonClient} from './lib/client.js';

/** The five things a caller does with the resident pool. */
export interface Daemon {
  /** Connects, starting a daemon if none is listening. */
  connect(options?: DaemonOptions): Promise<DaemonClient>;
  /** One screenshot through the daemon, connection and all. */
  screenshot(options: ScreenshotOptions&{daemon?: DaemonOptions}):
      Promise<Buffer|null>;
  /** Starts one if it is not up, and reports what is there either way. */
  start(options?: DaemonOptions): Promise<DaemonStatus&{spawned: boolean}>;
  status(options?: DaemonOptions):
      Promise<Partial<DaemonStatus>&{running: boolean, endpoint: string}>;
  stop(options?: DaemonOptions): Promise<{stopped: boolean, endpoint: string}>;
}

// The events the pool forwards, and the only ones. Declared as an interface
// merged into the class below rather than as a catch-all `on(string, ...)`,
// so that a listener for an event this runtime never emits is a compile error
// rather than a callback nobody ever calls.
export interface Runtime {
  on(event: 'ready', listener: (info: {workers: number}) => void): this;
  on(event: 'exit', listener: (event: WorkerEvent) => void): this;
  on(event: 'crash', listener: (event: WorkerEvent) => void): this;
  on(event: 'timeout',
     listener: (event: {worker: number, timeout: number}) => void): this;
  on(event: 'worker-restart',
     listener: (event: {worker: number, reason: string, delay: number}) => void):
      this;
  /** A worker could not be started at all -- a missing or unusable binary. */
  on(event: 'worker-error',
     listener: (event: {worker: number, error: Error}) => void): this;
  on(event: 'stderr',
     listener: (event: {worker: number, line: string}) => void): this;
}

/**
 * The library's one runtime: a pool of worker processes plus its lifecycle.
 *
 * `runtime` below is the singleton, because the expensive part is the
 * processes and a second runtime would double them for no gain. Anyone who
 * genuinely wants two constructs a Runtime directly.
 *
 * Its pool lives and dies with this process. `daemon` is the same pool behind
 * a socket, for callers whose process does not live long enough to be worth
 * starting one.
 */
export class Runtime extends EventEmitter {
  private pool: Pool|null = null;

  get running(): boolean {
    return this.pool !== null;
  }

  /**
   * Starts the pool. Safe to call twice; the second call is a no-op, so that
   * library code can call it defensively.
   *
   * Every option has a default: the binary is `$SHOTIUM_BINARY`, then the
   * platform package, then `./bin/shotium.exe`; the worker count is half the
   * cores, at least one and at most four; the cache root is a directory under
   * the system temp, and `null` disables caching.
   */
  start(options: StartOptions = {}): this {
    if (this.pool) {
      return this;
    }
    const pool = new Pool(resolveStartOptions(options));
    this.pool = pool;
    for (const event
             of ['ready', 'exit', 'crash', 'timeout', 'worker-restart',
                 'worker-error', 'stderr']) {
      pool.on(event, (payload) => this.emit(event, payload));
    }
    pool.start();
    return this;
  }

  /** Stops every worker. The pool can be started again afterwards. */
  async stop(): Promise<void> {
    if (!this.pool) {
      return;
    }
    const pool = this.pool;
    this.pool = null;
    await pool.stop();
  }

  /**
   * Renders one screenshot. Resolves to the encoded image, or to `null` when
   * `path` was given and the worker wrote the file itself.
   */
  async screenshot(options: ScreenshotOptions): Promise<Buffer|null> {
    // Validate before starting anything. A malformed request should not cost a
    // pool of worker processes to discover, and toRequest() is the only check
    // that can be made without one.
    const request = toRequest(options);
    if (!this.pool) {
      this.start();
    }
    const retry = typeof options.retry === 'number' ? options.retry : 0;
    const result = await this.pool!.submit(request, {
      timeout: timeoutFor(options) + SUPERVISOR_MARGIN_MS,
      retry,
    });
    return result.image;
  }
}

/** The shared pool: one per process, started on first use. */
const runtime = new Runtime();

/** One screenshot through the shared pool, starting it if it is not up. */
const screenshot = (options: ScreenshotOptions): Promise<Buffer|null> =>
    runtime.screenshot(options);

/**
 * The resident pool: workers that outlive the process that started them,
 * reachable over a named pipe on Windows and a unix socket elsewhere. For
 * callers that are short-lived themselves. See lib/daemon.ts.
 */
const daemon: Daemon = {
  connect: client.connect,
  screenshot: client.screenshot,
  start: client.start,
  status: client.status,
  stop: client.stop,
};

export {runtime, screenshot, daemon};

// A default as well as the names, because `import shotium from` is what a
// caller coming from `require` writes first, and the two have to be the same
// object rather than two views that drift.
export default {Runtime, runtime, screenshot, daemon};
