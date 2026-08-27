import * as binding from './binding.js';
import type {Engine as Handle} from './binding.js';
import {toRequest} from './request.js';
import type {WireRequest} from './request.js';
import type {PurgeOptions, ScreenshotOptions, StartOptions} from '../types.js';

import {resolveStartOptions} from './config.js';

// One per process, ever. Not one at a time -- one.
//
// This is not a rule of this file, it is what Blink is: initialising it writes
// process-wide statics it has no path to undo, so shot_engine_destroy() gives
// back what it can and the process still cannot make another. The C API
// returns SHOT_ERR_STATE for a second create whether or not the first is
// still alive. See shot/shot_api.h.
//
// So `stop()` is final for the process, and this flag exists to say that in
// words at the call site. Without it a caller who stops and starts again gets
// SHOT_ERR_STATE out of the addon -- a true error, arriving one layer too deep
// to explain that the answer is a second process rather than a retry.
let startedInThisProcess = false;

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
  private handle: Handle|null = null;
  private stopped = false;
  private tail: Promise<unknown> = Promise.resolve();

  get running(): boolean {
    return this.handle !== null;
  }

  /**
   * Starts the engine. Safe to call twice; the second call is a no-op, so that
   * library code can call it defensively.
   *
   * Not safe to call after `stop()`, and not because of anything here: Blink
   * starts once per process and cannot be restarted. Another engine means
   * another process.
   */
  start(options: StartOptions = {}): this {
    if (this.handle) {
      return this;
    }
    if (this.stopped) {
      throw new Error(
          'shotium: this engine was stopped, and Blink cannot be started ' +
          'again in a process that has already run it. Start another ' +
          'process, or keep the engine up between screenshots.');
    }
    if (startedInThisProcess) {
      throw new Error(
          'shotium: an engine has already run in this process. Blink is a ' +
          'process-wide singleton -- there is one per process, ever -- so a ' +
          'second Runtime cannot have one. Use the shared `runtime`, or run ' +
          'another process.');
    }
    const native = binding.load();
    const resolved = resolveStartOptions(options);

    const engineOptions: Record<string, unknown> = {};
    if (resolved.cacheDir !== null) {
      engineOptions.cacheDir = resolved.cacheDir;
    }
    if (resolved.userAgent !== undefined) {
      engineOptions.userAgent = resolved.userAgent;
    }
    // The packs sit beside the library, and the library cannot find itself on
    // Linux -- the path the engine resolves for "this module" goes through
    // /proc/self/exe, which names node. Saying it here is cheaper than
    // teaching the engine a second way to look. See shot_api.h.
    engineOptions.resourceDir = resolved.resourceDir ?? binding.directory();

    this.handle = native.create(JSON.stringify(engineOptions));
    startedInThisProcess = true;
    return this;
  }

  /**
   * Stops the engine, after whatever is queued.
   *
   * Final for this process: see the note above. A program that will want
   * another screenshot later should leave the engine up and call `purge()`
   * instead, which hands back the memory without giving up the engine.
   */
  async stop(): Promise<void> {
    if (!this.handle) {
      return;
    }
    this.stopped = true;
    // After the queue, not before: destroy() waits for a capture in flight
    // anyway, and doing it in order means a caller's last screenshot resolves
    // rather than racing the shutdown.
    const handle = this.handle;
    this.handle = null;
    await this.tail.catch(() => {});
    binding.load().destroy(handle);
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
  purge({releaseWorkingSet = false}: PurgeOptions = {}): void {
    if (!this.handle) {
      return;
    }
    binding.load().purge(this.handle, releaseWorkingSet);
  }

  /**
   * Renders one screenshot. Resolves to the encoded image, or to `null` when
   * `path` was given and the engine wrote the file itself.
   */
  // `async` and not a plain function returning capture()'s promise: toRequest()
  // throws, and a caller who wrote `screenshot(bad).catch(...)` would get the
  // throw past the catch and into the surrounding frame. The whole surface is
  // promise-shaped, so a bad request is a rejection like everything else.
  async screenshot(options: ScreenshotOptions): Promise<Buffer|null> {
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
  async capture(request: WireRequest): Promise<Buffer|null> {
    if (!this.handle) {
      this.start();
    }
    const handle = this.handle;
    const native = binding.load();

    // Chain onto the tail so that captures run one at a time. The catch keeps
    // one failure from poisoning everything queued behind it.
    const result = this.tail.catch(() => {}).then(
        () => native.capture(handle, JSON.stringify(request)));
    this.tail = result.catch(() => {});
    const image = await result;
    return request.path ? null : image;
  }
}
