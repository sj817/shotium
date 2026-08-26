import fs from 'node:fs';
import {createRequire} from 'node:module';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

import * as platformPackage from './lib/platform.js';
import {toRequest} from './lib/request.js';

// A .node addon is a CommonJS artefact: there is no ESM loader for one.
const require = createRequire(import.meta.url);

// ESM has no __dirname. This is the same thing, from the module's own URL.
const HERE = path.dirname(fileURLToPath(import.meta.url));

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
// one package rather than two. build/Release is where node-gyp puts a local
// build; it exists in a checkout and not in an install, so the two never
// compete in practice.
function candidates() {
  const found = [];
  const dir = platformPackage.packageDir();
  if (dir) {
    found.push(path.join(dir, 'shotium.node'));
  }
  found.push(path.join(HERE, 'native', 'build', 'Release', 'shotium.node'));
  return found;
}

let binding = null;
let bindingDir = null;

function load() {
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
    binding = require(candidate);
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
      '  require("@shotkit/shotium") uses worker processes instead and needs ' +
      'no addon.');
}

// The engine, and the queue in front of it.
//
// The queue is not about fairness. Each capture occupies a libuv thread pool
// thread for as long as the render takes, and there are four of those by
// default, shared with fs and dns -- so letting four screenshots go at once
// would stall the host's file reads for a fifth of a second at a time while
// gaining nothing, since the engine serialises them anyway.
class NativeRuntime {
  constructor() {
    this._engine = null;
    this._options = null;
    this._tail = Promise.resolve();
  }

  get running() {
    return this._engine !== null;
  }

  // Starts the engine. Safe to call twice; the second call is a no-op.
  //
  //   cacheDir     the HTTP disk cache; null disables caching
  //   userAgent    overrides the built-in string
  //   resourceDir  where shot_data.pak and shot_strings.pak are
  //                (default: beside the addon)
  start(options = {}) {
    if (this._engine) {
      return this;
    }
    const native = load();

    const engineOptions = {};
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

    this._engine = native.create(JSON.stringify(engineOptions));
    this._options = engineOptions;
    return this;
  }

  async stop() {
    if (!this._engine) {
      return;
    }
    // After the queue, not before: destroy() waits for a capture in flight
    // anyway, and doing it in order means a caller's last screenshot resolves
    // rather than racing the shutdown.
    const engine = this._engine;
    this._engine = null;
    await this._tail.catch(() => {});
    load().destroy(engine);
  }

  // Hands back what the engine is holding but can rebuild. `releaseWorkingSet`
  // additionally asks the OS for the pages, which the next screenshot pays
  // back in soft faults -- worth it when there may not be a next one soon.
  //
  // The resident worker does this for itself on a timer because it can watch
  // its own request stream. Here the queue belongs to the host, so the host is
  // the one that knows a batch has ended.
  purge({releaseWorkingSet = false} = {}) {
    if (!this._engine) {
      return;
    }
    load().purge(this._engine, releaseWorkingSet);
  }

  // Renders one screenshot. Resolves to the encoded image, or to null when
  // `path` was given and the engine wrote the file itself.
  async screenshot(options) {
    // Before anything else, and before the queue: a malformed request should
    // be a rejection now rather than one that waits its turn.
    const request = toRequest(options);
    if (!this._engine) {
      this.start();
    }
    const engine = this._engine;
    const native = load();

    // Chain onto the tail so that captures run one at a time. The catch keeps
    // one failure from poisoning everything queued behind it.
    const result = this._tail.catch(() => {}).then(
        () => native.capture(engine, JSON.stringify(request)));
    this._tail = result.catch(() => {});
    const image = await result;
    return request.path ? null : image;
  }
}

const native = new NativeRuntime();

const screenshot = (options) => native.screenshot(options);

export {NativeRuntime, native, screenshot};

export default {NativeRuntime, native, screenshot};
