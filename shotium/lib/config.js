'use strict';

const os = require('os');
const path = require('path');

const platform = require('./platform');

// The one place that decides what "no options" means.
//
// It is shared rather than duplicated because the daemon's address is a hash of
// its configuration: if the library and the CLI filled in defaults even
// slightly differently, a client would compute an address no daemon is
// listening on and start a second pool next to the first one that was already
// warm. See endpoint.js.
//
// Three places, in the order a caller means them: what they said, what npm
// installed, and what they unpacked by hand. The middle one is the normal case
// and the only one that needs no instructions.
function defaultBinary() {
  if (process.env.SHOTIUM_BINARY) {
    return process.env.SHOTIUM_BINARY;
  }
  const dir = platform.packageDir();
  if (dir) {
    return path.join(dir, platform.binaryName());
  }
  // No platform package: an archive from the releases page, unpacked into
  // bin/ beside this file. This is also the path a checkout takes, where
  // nothing was installed from a registry at all.
  return path.join(__dirname, '..', 'bin', platform.binaryName());
}

// How many worker processes, when nobody said.
//
// Half the cores, capped. The cap is there because a worker is a process with
// blink in it, not a thread: measured on this tree it settles around 14 MB of
// private working set and holds a further ~30 MB of shotium.exe resident, so
// "half the cores" on a 32-core machine is sixteen of them and most of a
// gigabyte for a queue that is almost never sixteen deep. Four is past the
// point where a screenshot workload gets much from another one -- the corpus
// runs at 41 pages/s on four -- and anyone who has measured otherwise passes
// `workers`.
const MAXIMUM_DEFAULT_WORKERS = 4;

function defaultWorkers() {
  const half = Math.floor((os.cpus().length || 2) / 2);
  return Math.max(1, Math.min(MAXIMUM_DEFAULT_WORKERS, half));
}

function defaultCacheDir() {
  return path.join(os.tmpdir(), 'shotium-cache');
}

// binary / workers / cacheDir / args, filled in and normalised. `cacheDir:
// null` survives as null -- it means "no disk cache", which is not the same
// request as "use the default one".
function resolveStartOptions(options = {}) {
  return {
    binary: options.binary || defaultBinary(),
    workers: options.workers || defaultWorkers(),
    cacheDir: options.cacheDir === null ?
        null :
        (options.cacheDir || defaultCacheDir()),
    args: options.args || [],
  };
}

module.exports = {
  defaultBinary,
  defaultCacheDir,
  defaultWorkers,
  resolveStartOptions,
};
