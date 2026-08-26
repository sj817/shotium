import fs from 'node:fs';
import {createRequire} from 'node:module';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

import * as platformPackage from './lib/platform.js';
import {toRequest} from './lib/request.js';
import type {
  NativeStartOptions,
  PurgeOptions,
  ScreenshotOptions,
} from './types.js';

export type {
  NativeStartOptions,
  PurgeOptions,
  ScreenshotOptions,
} from './types.js';

// A .node addon is a CommonJS artefact: there is no ESM loader for one.
const require = createRequire(import.meta.url);

// ESM has no __dirname. This is the same thing, from the module's own URL.
const HERE = path.dirname(fileURLToPath(import.meta.url));

// The engine handle the addon hands back. It is opaque on purpose: everything
// that can be done with it is a call on the binding below.
type Engine = unknown;

// What native/binding.cc exports. See shot/shot_api.h for the C ABI under it.
interface NativeBinding {
  create(optionsJson: string): Engine;
  destroy(engine: Engine): void;
  purge(engine: Engine, releaseWorkingSet: boolean): void;
  capture(engine: Engine, requestJson: string): Promise<Buffer>;
}

// shot in this process, instead of in workers beside it.
//
// The difference from `runtime` is not the API, which is the same
// screenshot(options), and not the request format, which is byte for byte the
// same JSON. It is where blink is:
//
//   runtime   N worker processes, one screenshot each at a time, a crash is a
//             retry, memory is N copies of an engine
//   native    one engine in this process, one screenshot at a time ever, a
//             crash takes the program with it, memory is one copy
//
// One at a time is not a limitation of this file. Blink is a process-wide
// singleton -- it is initialised once and there is no path to a second one --
// so an in-process engine is one renderer no matter how it is driven, and
// worker_threads do not change that because they share the process. A caller
// who wants four screenshots at once wants four processes, which is what the
// pool is for.
//
// What it buys is that there is no process to start, nothing to find on disk,
// no pipe, and no supervisor: a program that takes a handful of screenshots
// and exits pays for one engine and talks to it directly.

// Where the addon and the library beside it live.
//
// The platform package is what ships -- the .node sits next to the shared
// library it is linked against, which is the whole reason the two travel in
// one package rather than two. native/build/Release is where node-gyp puts a
// local build; it exists in a checkout and not in an install, so the two never
// compete in practice. Both paths are relative to this file's build output,
// which is one directory below the package root.
function candidates(): string[] {
  const found: string[] = [];
  const dir = platformPackage.packageDir();
  if (dir) {
    found.push(path.join(dir, 'shotium.node'));
  }
  found.push(
      path.join(HERE, '..', 'native', 'build', 'Release', 'shotium.node'));
  return found;
}

let binding: NativeBinding|null = null;
let bindingDir: string|null = null;

function load(): NativeBinding {
  if (binding) {
    return binding;
  }
  const tried = candidates();
  for (const candidate of tried) {
    if (!fs.existsSync(candidate)) {
      continue;
    }
    // Not wrapped in a try: a .node that is there and will not load is a
    // broken installation, and the loader's own message -- a missing
    // dependency, an architecture mismatch -- says more than anything that
    // could be substituted for it.
    binding = require(candidate) as NativeBinding;
    bindingDir = path.dirname(candidate);
    return binding;
  }
  const expected = platformPackage.packageName();
  throw new Error(
      'shotium: no native engine for this platform.\n' +
      `  looked in:\n    ${tried.join('\n    ')}\n` +
      (expected ?
           `  It ships in ${expected}, which npm installs as an optional ` +
               'dependency of this package.\n' :
           `  There is no build for ${process.platform}-${process.arch}.\n`) +
      '  import("@shotkit/shotium") uses worker processes instead and needs ' +
      'no addon.');
}

/**
 * The engine, in this process, and the queue in front of it.
 *
 * Same options and same output as `runtime`, and a different set of trades.
 * There is no worker process, so there is nothing to start, nothing to find on
 * disk and no pipe: a screenshot costs about a third less than through the
 * pool, and the whole thing is one process instead of five.
 *
 * What it gives up is what a separate process was providing for free. One
 * renderer, because blink is a process-wide singleton and `worker_threads`
 * share the process, so requests are serialised however many callers there
 * are. And no crash isolation: a renderer that dies takes the host program
 * with it, where the pool would have retried.
 *
 * The queue is not about fairness. Each capture occupies a libuv thread pool
 * thread for as long as the render takes, and there are four of those by
 * default, shared with fs and dns -- so letting four screenshots go at once
 * would stall the host's file reads for a fifth of a second at a time while
 * gaining nothing, since the engine serialises them anyway.
 */
export class NativeRuntime {
  private engine: Engine|null = null;
  private tail: Promise<unknown> = Promise.resolve();

  get running(): boolean {
    return this.engine !== null;
  }

  /**
   * Starts the engine. Safe to call twice; the second call is a no-op.
   *
   * `cacheDir` is the HTTP disk cache and `null` disables it, which is the
   * default here. `resourceDir` is where `shotium_data.pak` and
   * `shotium_strings.pak` are, and defaults to the directory the addon was
   * loaded from, which is where they ship.
   */
  start(options: NativeStartOptions = {}): this {
    if (this.engine) {
      return this;
    }
    const native = load();

    const engineOptions: Record<string, unknown> = {};
    if (options.cacheDir !== null && options.cacheDir !== undefined) {
      engineOptions.cacheDir = options.cacheDir;
    }
    if (options.userAgent !== undefined) {
      engineOptions.userAgent = options.userAgent;
    }
    // The packs sit beside the library, and the library cannot find itself on
    // Linux -- the path the engine resolves for "this module" goes through
    // /proc/self/exe, which names node. Saying it here is cheaper than
    // teaching the engine a second way to look. See shot_api.h.
    engineOptions.resourceDir = options.resourceDir || bindingDir;

    this.engine = native.create(JSON.stringify(engineOptions));
    return this;
  }

  /** Stops the engine, after whatever is queued. */
  async stop(): Promise<void> {
    if (!this.engine) {
      return;
    }
    // After the queue, not before: destroy() waits for a capture in flight
    // anyway, and doing it in order means a caller's last screenshot resolves
    // rather than racing the shutdown.
    const engine = this.engine;
    this.engine = null;
    await this.tail.catch(() => {});
    load().destroy(engine);
  }

  /**
   * Hands back what the engine is holding but can rebuild.
   * `releaseWorkingSet` additionally asks the OS for the pages, which the next
   * screenshot pays back in soft faults -- worth it when there may not be a
   * next one soon.
   *
   * The resident worker does this for itself on a timer because it can watch
   * its own request stream go quiet. Here the queue belongs to the caller, so
   * the caller is the one who knows a batch has ended.
   */
  purge({releaseWorkingSet = false}: PurgeOptions = {}): void {
    if (!this.engine) {
      return;
    }
    load().purge(this.engine, releaseWorkingSet);
  }

  /**
   * Renders one screenshot. Resolves to the encoded image, or to `null` when
   * `path` was given and the engine wrote the file itself.
   */
  async screenshot(options: ScreenshotOptions): Promise<Buffer|null> {
    // Before anything else, and before the queue: a malformed request should
    // be a rejection now rather than one that waits its turn.
    const request = toRequest(options);
    if (!this.engine) {
      this.start();
    }
    const engine = this.engine;
    const native = load();

    // Chain onto the tail so that captures run one at a time. The catch keeps
    // one failure from poisoning everything queued behind it.
    const result = this.tail.catch(() => {}).then(
        () => native.capture(engine, JSON.stringify(request)));
    this.tail = result.catch(() => {});
    const image = await result;
    return request.path ? null : image;
  }
}

/** The shared in-process engine, started on first use. */
const native = new NativeRuntime();

/** One screenshot through the shared in-process engine. */
const screenshot = (options: ScreenshotOptions): Promise<Buffer|null> =>
    native.screenshot(options);

export {native, screenshot};

export default {NativeRuntime, native, screenshot};
