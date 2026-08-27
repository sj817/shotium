import {Cache} from './lib/cache.js';
import * as client from './lib/client.js';
import type {DaemonClient} from './lib/client.js';
import {Engine} from './lib/engine.js';
import type {
  DaemonOptions,
  DaemonStatus,
  ReleaseMemoryOptions,
  ScreenshotOptions,
  ScreenshotResult,
  StartOptions,
  StartResult,
} from './types.js';

export type {
  CacheClearOptions,
  CacheClearResult,
  CacheEntry,
  CacheMode,
  CacheTarget,
  CaptureStats,
  CaptureTiming,
  Clip,
  DaemonOptions,
  DaemonStatus,
  PageGotoParams,
  ReleaseMemoryOptions,
  ScreenshotOptions,
  ScreenshotResult,
  StartOptions,
  StartResult,
  Viewport,
} from './types.js';
export type {DaemonClient} from './lib/client.js';
export {Cache} from './lib/cache.js';

/** The five things a caller does with the resident engine. */
export interface Daemon {
  /** Connects, starting a daemon if none is listening. */
  connect(options?: DaemonOptions): Promise<DaemonClient>;
  /** One screenshot through the daemon, connection and all. */
  screenshot(options: ScreenshotOptions&{daemon?: DaemonOptions}):
      Promise<ScreenshotResult>;
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
 *     shotium.start();
 *     const {image, stats} = await shotium.screenshot({
 *       file: 'https://example.com',
 *     });
 *     await shotium.stop();
 *
 * `start` and `stop` are explicit because starting Blink is the expensive part
 * -- tens of milliseconds and a working set that stays resident -- and only
 * the caller knows whether the next screenshot is coming in a moment or never.
 * Neither call is required: a screenshot starts the engine if it is not up.
 * What they buy is control over when that cost is paid, and the certainty that
 * it has been given back.
 *
 * Neither is rationed, either. They may be called in any order and as often as
 * a program likes: `stop()` stands the engine down and `start()` picks the
 * same one back up, warm cache and all. What cannot happen is a *second*
 * engine -- Blink is initialised once per process and there is no undo -- but
 * that is a fact about how many there are, not about how many times the one
 * may be asked for.
 *
 * The methods are on the module rather than under a `runtime` namespace, which
 * they were until 0.3. There was never anything else to start, so the word
 * carried nothing; and `runtime.cache` would have been the wrong place for the
 * cache besides, since a cache directory outlives every engine that writes to
 * it and can be read when no engine is running at all.
 *
 * `Runtime` is still exported for a caller who wants to own a lifecycle rather
 * than share the module's. It is a lifecycle and not an engine: there is one
 * engine per process, and a second Runtime that starts adopts the same one
 * rather than building another. Parallelism is more processes, not more
 * Runtimes.
 *
 * `daemon` is the same engine in a process of its own, behind a socket, for
 * callers whose own process does not live long enough to be worth starting
 * one.
 */
export class Runtime {
  private engine = new Engine();

  /**
   * The HTTP cache: where it is, what is in it, and how to empty it.
   *
   * On the Runtime as well as on the module because a caller holding their own
   * Runtime needs the engine handle to reach a directory that engine has open:
   * within one process a directory has one backend, so borrowing is the only
   * way in.
   */
  readonly cache = new Cache(() => this.engine.nativeHandle);

  get running(): boolean {
    return this.engine.running;
  }

  /**
   * Starts the engine, or picks the running one back up.
   *
   * Callable as often as you like, in any order with `stop()`; library code
   * can call it defensively. The first call in a process builds the engine and
   * every later one adopts it -- the same engine, the same warm cache. The one
   * thing it will refuse is a *different* configuration: the options below are
   * fixed when the engine is built, and there is no second build, so naming
   * one that disagrees with what is running throws rather than rendering with
   * a value you did not ask for.
   *
   * Every option has a default. `cacheDir` is the HTTP disk cache and defaults
   * to a per-project directory under the system temporary directory; `null`
   * turns it off. `resourceDir` is where `shotium_data.pak` and
   * `shotium_strings.pak` are, and defaults to the directory the engine was
   * loaded from, which is where they ship.
   *
   * The return value is worth reading once. `cacheActive: false` with a
   * `cacheDir` set means the directory could not be opened and this engine is
   * running without a cache -- correctly, silently, and a round trip slower on
   * everything.
   */
  start(options: StartOptions = {}): StartResult {
    return this.engine.start(options);
  }

  /** What `start()` returned, asked again. */
  status(): StartResult {
    return this.engine.status();
  }

  /**
   * Stands the engine down, after whatever is queued.
   *
   * The queue drains, the memory the engine can rebuild goes back to the OS,
   * and `running` becomes false. Blink itself stays initialised, because there
   * is no way to un-initialise it -- so the disk cache stays where it is, and
   * `start()` or the next `screenshot()` picks the same engine back up.
   *
   * Which makes this a caller saying they are done for now rather than a
   * destructor. It does the same work as `releaseMemory({releaseWorkingSet:
   * true})` and additionally stops accepting captures.
   */
  stop(): Promise<void> {
    return this.engine.stop();
  }

  /**
   * Hands back what the engine is holding but can rebuild: Blink's heap,
   * skia's caches, PartitionAlloc's free lists. Worth calling when a batch has
   * ended and the next one may be a while away.
   *
   * This is memory and nothing else. It was called `purge()` until 0.3, which
   * next to `cache.clear()` read as though it emptied the HTTP cache; it does
   * not touch the disk at all.
   */
  releaseMemory(options: ReleaseMemoryOptions = {}): void {
    this.engine.releaseMemory(options);
  }

  /**
   * Renders one screenshot, and reports what it cost.
   *
   * `image` is the encoded bytes, or `null` when `path` was given and the
   * engine wrote the file itself. `stats` says how many resources were
   * fetched, how many came from the cache, and where the milliseconds went --
   * which for an `https:` URL is usually the answer to "why did this take so
   * long", because a cold connection costs more than the render does.
   */
  screenshot(options: ScreenshotOptions): Promise<ScreenshotResult> {
    return this.engine.screenshot(options);
  }
}

/** The shared engine: one per process, started on first use. */
const runtime = new Runtime();

/** One screenshot through the shared engine, starting it if it is not up. */
const screenshot = (options: ScreenshotOptions): Promise<ScreenshotResult> =>
    runtime.screenshot(options);

const start = (options?: StartOptions): StartResult => runtime.start(options);
const status = (): StartResult => runtime.status();
const stop = (): Promise<void> => runtime.stop();
const releaseMemory = (options?: ReleaseMemoryOptions): void =>
    runtime.releaseMemory(options);

/**
 * The HTTP cache.
 *
 * At the top level rather than under the engine because it outlives one: the
 * directory is on disk whether or not anything is running, `getDir()` answers
 * before the first `start()`, and clearing it is something a program may want
 * to do without bringing Blink up at all. When an engine *is* up, these
 * borrow its cache backend, because within one process a directory has one
 * backend and that is the only way in.
 */
const cache = runtime.cache;

/**
 * The resident engine: a process that outlives the one that started it,
 * reachable over a named pipe on Windows and a unix socket elsewhere. For
 * callers that are short-lived themselves. See lib/daemon.ts.
 *
 * It has no `cache` of its own. A daemon's cache directory is reported by
 * `daemon.status()`, and clearing it is done by pointing `cache.clear()` at
 * that directory or by stopping the daemon -- a cross-process cache protocol
 * would be a second implementation of this module for something nobody does on
 * a request path.
 */
const daemon: Daemon = {
  connect: client.connect,
  screenshot: client.screenshot,
  start: client.start,
  status: client.status,
  stop: client.stop,
};

export {cache, daemon, releaseMemory, runtime, screenshot, start, status, stop};

// A default as well as the names, because `import shotium from` is what a
// caller coming from `require` writes first, and the two have to be the same
// object rather than two views that drift.
export default {
  Runtime,
  cache,
  daemon,
  releaseMemory,
  runtime,
  screenshot,
  start,
  status,
  stop,
  // A getter and not a value, because it changes. It is only on the default
  // export: a named `running` would have to be a live binding that something
  // remembered to update, and the two would disagree the first time anybody
  // forgot. Callers who import by name have `status().running`, which is the
  // same answer with the cache directory attached.
  get running(): boolean {
    return runtime.running;
  },
};
