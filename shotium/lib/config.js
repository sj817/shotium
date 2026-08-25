'use strict';

const os = require('os');
const path = require('path');

// The one place that decides what "no options" means.
//
// It is shared rather than duplicated because the daemon's address is a hash of
// its configuration: if the library and the CLI filled in defaults even
// slightly differently, a client would compute an address no daemon is
// listening on and start a second pool next to the first one that was already
// warm. See endpoint.js.
function defaultBinary() {
  if (process.env.SHOTIUM_BINARY) {
    return process.env.SHOTIUM_BINARY;
  }
  const name = process.platform === 'win32' ? 'shot.exe' : 'shot';
  return path.join(__dirname, '..', 'bin', name);
}

// How many worker processes, when nobody said.
//
// Half the cores, capped. The cap is there because a worker is a process with
// blink in it, not a thread: measured on this tree it settles around 14 MB of
// private working set and holds a further ~30 MB of shot.exe resident, so
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
