import * as binding from './binding.js';
import type {Engine as Handle} from './binding.js';
import {toRequest} from './request.js';
import type {WireRequest} from './request.js';
import type {
  CaptureStats,
  ReleaseMemoryOptions,
  ScreenshotOptions,
  ScreenshotResult,
  StartOptions,
  StartResult,
} from '../types.js';

import type {ResolvedStartOptions} from './config.js';
import {resolveStartOptions} from './config.js';

// The engine this process has, held above every Engine object that uses it.
//
// Blink is initialised once and has no undo: it writes process-wide statics
// that shot_engine_destroy() cannot take back, and the C API refuses a second
// create for the lifetime of the process whether or not the first is still
// alive.
//
// That fact used to be exposed directly -- `stop()` destroyed the engine and
// every later `start()` threw. It was the wrong shape. `stop()` and `start()`
// are a caller saying "I am done for now" and "I want it again", and a library
// whose engine can be asked for exactly once turns an ordinary pair of calls
// into a thing that has to be rationed. It also made the disk cache
// nonsensical: the whole point of a cache is the *next* run, and the next run
// could not have the engine that reads it.
//
// So the handle lives here rather than on the instance. `stop()` stands the
// engine down -- the queue drains, the memory goes back, nothing more is
// accepted -- and `start()` picks the same one up again, as many times as a
// caller likes. The process is the engine's lifetime, which is what it always
// was; the difference is that the API no longer pretends to offer a shorter
// one.
let shared: Handle|null = null;

// What `shared` was created with. Kept because those options are fixed for the
// life of the process -- a later `start()` asking for a different cache
// directory cannot be given one, and is told so rather than handed an engine
// that quietly uses the first caller's.
let sharedOptions: ResolvedStartOptions|null = null;

// Whether the one create this process gets has been spent.
//
// Separate from `shared` being non-null because `dispose()` clears the handle
// and does not give the ability back: after a real teardown there is no engine
// and there cannot be another. Nothing in the public surface calls dispose()
// -- the daemon does, on its way out of a process it owns.
let spent = false;

/** The engine handle this process has, or null if it has none. */
function sharedHandle(): Handle|null {
  return shared;
}

// The options that are fixed at create time, and the report a mismatch gets.
//
// Only the ones the caller actually named are checked: `start()` with no
// arguments is a caller saying "whatever is there", which is exactly what
// adopting a running engine gives them. Naming an option that disagrees is
// different -- it is a request that cannot be honoured, and silently rendering
// with the other value is the failure this exists to prevent.
function conflictingOption(
    options: StartOptions, current: ResolvedStartOptions): string|null {
  const wanted = resolveStartOptions(options);
  for (const key of ['cacheDir', 'cacheMaxBytes', 'userAgent', 'resourceDir'] as
       const) {
    if (options[key] === undefined || wanted[key] === current[key]) {
      continue;
    }
    return `${key} is ${JSON.stringify(current[key])}, and this start() ` +
        `asked for ${JSON.stringify(wanted[key])}`;
  }
  return null;
}

/**
 * Blink, in this process, and the queue in front of it.
 *
 * There is one renderer and there is no way to have two. Blink is a
 * process-wide singleton: it is initialised once, there is no path to a second
 * one, and `worker_threads` do not change that because they share the process.
 * So captures are serialised however many callers there are, and a program
 * that wants four at once wants four processes.
 *
 * The queue is not about fairness. Each capture occupies a libuv thread pool
 * thread for as long as the render takes, and there are four of those by
 * default, shared with fs and dns -- so letting four screenshots go at once
 * would stall the host's file reads for a fifth of a second at a time while
 * gaining nothing, since the engine serialises them anyway.
 */
export class Engine {
  // Whether *this* object considers itself started. The engine behind it may
  // well be up for somebody else; `running` is about this lifecycle, not about
  // whether the process has an engine.
  private active = false;
  private tail: Promise<unknown> = Promise.resolve();

  get running(): boolean {
    return this.active && shared !== null;
  }

  /**
   * The addon's engine handle, or null when this process has never had one.
   *
   * Deliberately not conditional on `running`. It is the cache that asks, and
   * what the cache needs to know is whether a backend exists in this process
   * -- because within one process a cache directory has one backend, so
   * reading or clearing the directory the engine holds means borrowing it
   * rather than opening a second one. A stood-down engine still holds its
   * directory, so a caller who calls `stop()` and then `cache.getFiles()` is
   * asking about a live backend and has to be routed to it. Nothing else
   * should reach for this.
   */
  get nativeHandle(): Handle|null {
    return sharedHandle();
  }

  /**
   * Starts the engine, or picks the running one back up.
   *
   * Callable as often as a caller likes, in any order with `stop()`. The first
   * call in a process builds the engine; every later one adopts it, which is
   * the same engine and the same warm cache. Library code can call it
   * defensively.
   *
   * The one thing that cannot be adopted is a different configuration. The
   * options below are fixed when the engine is built and there is no second
   * build, so naming one that disagrees with what is running throws rather
   * than rendering with a value the caller did not ask for.
   */
  start(options: StartOptions = {}): StartResult {
    if (shared) {
      const conflict = conflictingOption(options, sharedOptions!);
      if (conflict) {
        throw new Error(
            'shotium: this process already has an engine, and its ' +
            conflict + '. Blink is initialised once per process and cannot ' +
            'be built again, so an engine\'s options are fixed for as long ' +
            'as the process lives -- stop() does not undo them. Use the ' +
            'engine that is up, or run another process.');
      }
      this.active = true;
      return this.status();
    }
    if (spent) {
      throw new Error(
          'shotium: this process had an engine and it was disposed of. ' +
          'Blink is initialised once per process and cannot be built again. ' +
          'Run another process.');
    }

    const native = binding.load();
    const resolved = resolveStartOptions(options);

    const engineOptions: Record<string, unknown> = {};
    if (resolved.cacheDir !== null) {
      engineOptions.cacheDir = resolved.cacheDir;
      engineOptions.cacheMaxBytes = resolved.cacheMaxBytes;
    }
    if (resolved.userAgent !== undefined) {
      engineOptions.userAgent = resolved.userAgent;
    }
    // The packs sit beside the library, and the library cannot find itself on
    // Linux -- the path the engine resolves for "this module" goes through
    // /proc/self/exe, which names node. Saying it here is cheaper than
    // teaching the engine a second way to look. See shot_api.h.
    engineOptions.resourceDir = resolved.resourceDir ?? binding.directory();

    shared = native.create(JSON.stringify(engineOptions));
    sharedOptions = resolved;
    this.active = true;
    return this.status();
  }

  /**
   * What the engine came up as: whether this lifecycle is started, which cache
   * directory the engine has, and whether it actually got it.
   *
   * The last of those is the one worth reading. A directory that cannot be
   * created or written to -- no permission, no space, a path that is a file --
   * fails invisibly: the engine renders exactly as well without a cache, only
   * slower, and every capture pays the network again for a reason nothing
   * reports. The engine opens the cache during `start()` so that this is
   * answerable before the first screenshot rather than after it.
   *
   * The cache half is answered from the engine whenever this process has one,
   * including after `stop()`. A stood-down engine still holds its directory,
   * and reporting `null` for it would say the cache had gone away when what
   * went away was the willingness to render.
   */
  status(): StartResult {
    if (!shared) {
      return {running: false, cacheDir: null, cacheActive: false};
    }
    const reported =
        JSON.parse(binding.load().status(shared)) as Omit<StartResult, 'running'>;
    return {running: this.running, ...reported};
  }

  /**
   * Stands the engine down, after whatever is queued.
   *
   * The queue drains, the memory the engine can rebuild goes back to the OS,
   * and `running` becomes false. What does not happen is a teardown of Blink,
   * because there is no such thing -- see the note at the top of this file --
   * so the disk cache stays where it is and `start()` picks the same engine up
   * again whenever the caller wants it.
   *
   * Which makes this exactly what it says: not a destructor, a caller saying
   * they are done for now. A program that will want another screenshot in a
   * moment can equally well stay started and call `releaseMemory()`; the two
   * do the same work, and this one also stops accepting captures.
   */
  async stop(): Promise<void> {
    if (!this.active) {
      return;
    }
    this.active = false;
    // After the queue, not before: a caller's last screenshot should resolve
    // rather than race the stand-down, and the memory is not worth handing
    // back until the thing still using it has finished.
    await this.tail.catch(() => {});
    if (shared) {
      binding.load().purge(shared, /*releaseWorkingSet=*/ true);
    }
  }

  /**
   * The real teardown: joins the engine thread, unwinds the network stack, and
   * lets the disk cache write its index.
   *
   * Final, and final for the process rather than for this object -- which is
   * why it is not on the public surface. The daemon calls it as it exits a
   * process it owns, where the index flush is worth having and nothing is
   * going to ask for another screenshot. Everything else wants `stop()`.
   */
  async dispose(): Promise<void> {
    this.active = false;
    await this.tail.catch(() => {});
    const handle = shared;
    shared = null;
    sharedOptions = null;
    if (handle) {
      spent = true;
      binding.load().destroy(handle);
    }
  }

  /**
   * Hands back what the engine is holding but can rebuild.
   * `releaseWorkingSet` additionally asks the OS for the pages, which the next
   * screenshot pays back in soft faults -- worth it when there may not be a
   * next one soon.
   *
   * The daemon does this for itself on a timer because it can watch its own
   * request stream go quiet. Here the queue belongs to the caller, so the
   * caller is the one who knows a batch has ended.
   */
  releaseMemory({releaseWorkingSet = false}: ReleaseMemoryOptions = {}): void {
    if (!shared) {
      return;
    }
    binding.load().purge(shared, releaseWorkingSet);
  }

  /**
   * Renders one screenshot. Resolves to the encoded image, or to `null` when
   * `path` was given and the engine wrote the file itself.
   */
  // `async` and not a plain function returning capture()'s promise: toRequest()
  // throws, and a caller who wrote `screenshot(bad).catch(...)` would get the
  // throw past the catch and into the surrounding frame. The whole surface is
  // promise-shaped, so a bad request is a rejection like everything else.
  async screenshot(options: ScreenshotOptions): Promise<ScreenshotResult> {
    // Before anything else, and before the queue: a malformed request should
    // be a rejection now rather than one that waits its turn.
    return this.capture(toRequest(options));
  }

  /**
   * The same, for a request that is already in wire form.
   *
   * The daemon reads these off a socket, where they arrived having been
   * validated by the client that sent them. Re-deriving one from
   * ScreenshotOptions would mean the daemon validating a request it cannot see
   * the original of, and rejecting fields a newer client legitimately sent.
   */
  async capture(request: WireRequest): Promise<ScreenshotResult> {
    // Starts, or restarts, or adopts -- a screenshot after `stop()` is an
    // ordinary thing to ask for and gets the engine back.
    if (!this.running) {
      this.start();
    }
    const handle = shared!;
    const native = binding.load();

    // Chain onto the tail so that captures run one at a time. The catch keeps
    // one failure from poisoning everything queued behind it.
    const result = this.tail.catch(() => {}).then(
        () => native.capture(handle, JSON.stringify(request)));
    this.tail = result.catch(() => {});

    let captured;
    try {
      captured = await result;
    } catch (error) {
      // The addon attaches the capture's statistics to the rejection as
      // unparsed JSON, the same way it hands them back on success -- see
      // NativeCapture. Parsing them here rather than leaving a string on the
      // error is what makes `error.stats` the same CaptureStats a successful
      // call returns, which is the whole point of attaching it: the failure is
      // the case where the counters explain the most.
      const withStats = error as Error&{stats?: string | CaptureStats};
      if (typeof withStats.stats === 'string') {
        withStats.stats = JSON.parse(withStats.stats) as CaptureStats;
      }
      throw error;
    }

    return {
      image: request.path ? null : captured.image,
      stats: parseStats(captured.stats),
    };
  }
}

// A zeroed set of counters.
//
// Zeroes rather than undefined because the alternative is every caller writing
// `stats?.timing?.total ?? 0` around a field that is present for every capture
// that actually ran. The only case that produces none is a request rejected
// before it started, and that path throws rather than returning.
function emptyStats(): CaptureStats {
  return {
    requests: 0,
    fromCache: 0,
    failed: 0,
    bytes: 0,
    httpStatus: 0,
    finalUrl: '',
    timing: {fetch: 0, render: 0, encode: 0, total: 0},
  };
}

// The addon hands statistics over as unparsed JSON -- see NativeCapture -- so
// this is where the string becomes an object. The daemon's client has them
// parsed already, from its own response header, and uses emptyStats directly.
function parseStats(json: string|undefined): CaptureStats {
  return json ? JSON.parse(json) as CaptureStats : emptyStats();
}

export {emptyStats, parseStats, sharedHandle};
