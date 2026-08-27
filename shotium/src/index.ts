import * as client from './lib/client.js';
import type {DaemonClient} from './lib/client.js';
import {Engine} from './lib/engine.js';
import type {
  DaemonOptions,
  DaemonStatus,
  PurgeOptions,
  ScreenshotOptions,
  StartOptions,
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
} from './types.js';
export type {DaemonClient} from './lib/client.js';

/** The five things a caller does with the resident engine. */
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

/**
 * The engine, and its lifecycle, in this process.
 *
 *     import shotium from '@shotkit/shotium';
 *
 *     shotium.runtime.start();
 *     const png = await shotium.screenshot({file: 'https://example.com'});
 *     await shotium.runtime.stop();
 *
 * `start` and `stop` are explicit because starting Blink is the expensive part
 * -- tens of milliseconds and a working set that stays resident -- and only
 * the caller knows whether the next screenshot is coming in a moment or never.
 * Neither call is required: a screenshot starts the engine if it is not up.
 * What they buy is control over when that cost is paid, and the certainty that
 * it has been given back.
 *
 * `runtime` below is the singleton because there is nothing else it could be:
 * Blink starts once per process and cannot be restarted, so a second Runtime
 * in the same process has no engine to have. Construct one directly only to
 * own the lifecycle yourself instead of using `runtime`. Parallelism is more
 * processes, not more Runtimes.
 *
 * `daemon` is the same engine in a process of its own, behind a socket, for
 * callers whose own process does not live long enough to be worth starting
 * one.
 */
export class Runtime {
  private engine = new Engine();

  get running(): boolean {
    return this.engine.running;
  }

  /**
   * Starts the engine. Safe to call twice; the second call is a no-op, so that
   * library code can call it defensively. Not safe after `stop()` -- see there.
   *
   * Every option has a default. `cacheDir` is the HTTP disk cache and `null`
   * disables it; `resourceDir` is where `shotium_data.pak` and
   * `shotium_strings.pak` are, and defaults to the directory the engine was
   * loaded from, which is where they ship.
   */
  start(options: StartOptions = {}): this {
    this.engine.start(options);
    return this;
  }

  /**
   * Stops the engine, after whatever is queued.
   *
   * Final for this process. Blink writes process-wide state that it has no
   * path to undo, so starting again -- here or on another Runtime -- throws
   * rather than quietly handing back something that cannot render. A program
   * that wants another screenshot later should stay started and `purge()`.
   */
  stop(): Promise<void> {
    return this.engine.stop();
  }

  /**
   * Hands back what the engine is holding but can rebuild. Worth calling when
   * a batch has ended and the next one may be a while away.
   */
  purge(options: PurgeOptions = {}): void {
    this.engine.purge(options);
  }

  /**
   * Renders one screenshot. Resolves to the encoded image, or to `null` when
   * `path` was given and the engine wrote the file itself.
   */
  screenshot(options: ScreenshotOptions): Promise<Buffer|null> {
    return this.engine.screenshot(options);
  }
}

/** The shared engine: one per process, started on first use. */
const runtime = new Runtime();

/** One screenshot through the shared engine, starting it if it is not up. */
const screenshot = (options: ScreenshotOptions): Promise<Buffer|null> =>
    runtime.screenshot(options);

/**
 * The resident engine: a process that outlives the one that started it,
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
