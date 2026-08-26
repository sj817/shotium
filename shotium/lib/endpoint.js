import crypto from 'node:crypto';
import os from 'node:os';
import path from 'node:path';

// Where a daemon listens, derived from what it was asked to be.
//
// The address is a hash of the configuration -- binary, worker count, cache
// root, extra flags -- rather than a fixed name, because attaching to whatever
// daemon happens to be up would mean rendering with someone else's binary and
// someone else's flags. Two configurations are two daemons; the same
// configuration, from any process, is one.
//
// A caller who wants a daemon by name instead of by configuration passes
// `name`, which replaces the hash. That is the escape hatch for a service that
// starts its daemon deliberately and wants clients to find it without
// repeating the configuration.
function endpointKey(options) {
  if (options.name) {
    return String(options.name);
  }
  const identity = JSON.stringify([
    path.resolve(options.binary),
    options.workers,
    options.cacheDir === null ? null : path.resolve(options.cacheDir || ''),
    options.args || [],
  ]);
  return crypto.createHash('sha256').update(identity).digest('hex').slice(0, 16);
}

// Windows has named pipes and no filesystem sockets; POSIX has the reverse.
// Both are net.connect() addresses, which is the only reason the rest of the
// daemon can ignore the difference.
//
// The pipe namespace is per-machine but the socket path is per-user, so the
// uid goes in the POSIX name to keep two users on one host from colliding on a
// path only one of them can open.
function endpointFor(options = {}) {
  if (options.endpoint) {
    return options.endpoint;
  }
  if (process.env.SHOTIUM_ENDPOINT) {
    return process.env.SHOTIUM_ENDPOINT;
  }
  const key = endpointKey(options);
  if (process.platform === 'win32') {
    return `\\\\.\\pipe\\shotium-${key}`;
  }
  const uid = typeof process.getuid === 'function' ? process.getuid() : 0;
  return path.join(os.tmpdir(), `shotium-${uid}-${key}.sock`);
}

export {endpointFor, endpointKey};
