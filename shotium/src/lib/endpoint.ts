import crypto from 'node:crypto';
import os from 'node:os';
import path from 'node:path';

import {DAEMON_PROTOCOL_VERSION} from './protocol.js';

// What endpointFor() needs to know: a resolved configuration, plus the two
// ways of overriding the address it would derive from one.
export interface EndpointOptions {
  cacheDir?: string|null;
  userAgent?: string;
  resourceDir?: string;
  name?: string;
  endpoint?: string;
}

// Where a daemon listens, derived from what it was asked to be.
//
// The address is a hash of the wire generation and configuration -- cache
// root, user agent, resource directory -- rather than a fixed name, because
// attaching to whatever daemon happens to be up would mean either speaking an
// incompatible protocol or rendering with someone else's settings. Two
// configurations are two daemons; the same configuration and protocol, from
// any process, is one.
//
// Every field of EndpointOptions is optional, so nothing here fails to compile
// when a field is dropped from the configuration -- it just stops being part
// of the identity, and every caller collapses onto one address. That happened
// once, when the worker pool went away and this was left hashing three fields
// that no longer existed. If a field is added to StartOptions and it changes
// what the engine renders, it belongs in the array below.
//
// A caller who wants a daemon by name instead of by configuration passes
// `name`, which replaces the configuration hash but not the wire generation.
// That is the escape hatch for a service that starts its daemon deliberately
// and wants clients to find it without repeating the configuration.
function endpointKey(options: EndpointOptions): string {
  if (options.name) {
    return `${String(options.name)}-v${DAEMON_PROTOCOL_VERSION}`;
  }
  const identity = JSON.stringify([
    DAEMON_PROTOCOL_VERSION,
    options.cacheDir === null || options.cacheDir === undefined ?
        null :
        path.resolve(options.cacheDir),
    options.userAgent ?? null,
    options.resourceDir ? path.resolve(options.resourceDir) : null,
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
function endpointFor(options: EndpointOptions = {}): string {
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
