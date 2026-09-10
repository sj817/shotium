import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

import type {StartOptions} from '../types.js';

// StartOptions with every hole filled in. `cacheDir` is still nullable here
// because null is an answer -- "no disk cache" -- and not an absent one.
export interface ResolvedStartOptions {
  cacheDir: string|null;
  cacheMaxBytes: number;
  userAgent?: string;
  resourceDir?: string;
}

// One number, chosen rather than delegated.
//
// Passing 0 hands the decision to the disk cache backend, which sizes itself
// against the volume's free space -- a defensible default for a browser
// profile the user knows about, and a poor one for a directory that appears
// under ~/.shotium because somebody imported a library. 256 MB holds a large
// corpus of pages and their fonts, and is small enough that nobody has to
// think about it.
const DEFAULT_CACHE_MAX_BYTES = 256 * 1024 * 1024;

/**
 * One spelling of a path: absolute, with forward slashes.
 *
 * Every path this module hands back goes through here. On Windows the two
 * separators are interchangeable to the filesystem and not to a caller
 * comparing strings or writing a glob, and a library that returns whichever
 * one `path.join` happened to produce makes that the caller's problem.
 */
export function normalizePath(target: string): string {
  return path.resolve(target).replace(/\\/g, '/');
}

/**
 * The project the current process belongs to: the nearest directory at or
 * above the working directory that has a package.json.
 *
 * The working directory itself would be the obvious key and is the wrong one.
 * It moves -- `process.chdir`, or a script run from a subdirectory -- and each
 * value it takes would get a cache of its own, so a project would slowly
 * accumulate directories that each know a third of its pages. The package root
 * is the thing that stays put.
 *
 * Falls back to the working directory when there is no package.json above it,
 * which is what a bare script has and is still better than nothing: it is at
 * least stable for as long as the script runs from one place.
 */
function projectRoot(): string {
  let dir = process.cwd();
  for (;;) {
    if (fs.existsSync(path.join(dir, 'package.json'))) {
      return dir;
    }
    const parent = path.dirname(dir);
    if (parent === dir) {
      return process.cwd();
    }
    dir = parent;
  }
}

/**
 * Where every shotium cache directory lives. One level up from any single
 * project's, which is what makes `target: 'all'` answerable.
 *
 * Under the home directory and not the temporary one, which is where this was
 * until 0.3 was cut. $TMPDIR is defined by not surviving: /tmp is emptied on
 * reboot, systemd-tmpfiles removes anything untouched for ten days, and macOS
 * sweeps it on a schedule of its own. The entire value of an HTTP cache is the
 * *next* run, so a default that lives somewhere designed to be cleared is a
 * cache that stops working at exactly the moment it would have started paying
 * for itself.
 *
 * `~/.shotium`, spelled the same on every platform. One place a user can look
 * for it, one path to say in a bug report, and one directory to delete.
 *
 * $TMPDIR remains only as a fallback for a process with no home to speak of --
 * some containers, some service accounts. That is a degradation and not a
 * second location: there is no home directory holding a cache that would
 * otherwise have been found.
 */
function shotiumHome(): string {
  return path.join(os.homedir() || os.tmpdir(), '.shotium');
}

export function cacheRoot(): string {
  return normalizePath(path.join(shotiumHome(), 'cache'));
}

/**
 * The identifier for a project's cache directory: a hash of its root path.
 *
 * A hash rather than the path itself because the path contains separators,
 * drive letters and whatever the user called their directory, none of which
 * survive being a directory name. It is not a security measure and does not
 * need to be one -- it is a fixed-length name for a variable-length string.
 */
export function projectKey(root: string = projectRoot()): string {
  return crypto.createHash('sha1').update(normalizePath(root)).digest('hex');
}

/** This project's cache directory. */
export function defaultCacheDir(): string {
  return normalizePath(path.join(cacheRoot(), projectKey()));
}

// The one place that decides what "no options" means.
//
// It is shared rather than duplicated because the daemon's address is a hash of
// its configuration: if two callers filled in defaults even slightly
// differently, one would compute an address no daemon is listening on and
// start a second engine next to the first one that was already warm. See
// endpoint.ts.
//
// `cacheDir` defaults to this project's directory rather than to null, which
// is the reverse of 0.2. The reason is measured: without a cache every capture
// of an `https:` URL pays DNS, TLS and a round trip, which for a small page is
// most of the wall clock and all of the surprise. The objection to a default
// -- that a short-lived program leaves a directory behind -- is answered by
// the directory being per-project, size-capped, and somewhere the platform's
// own tooling knows how to clear, rather than by there being no cache.
// `cacheDir: null` still turns it off.
function resolveStartOptions(options: StartOptions = {}): ResolvedStartOptions {
  return {
    cacheDir: options.cacheDir === null ? null :
                                          (options.cacheDir ?? defaultCacheDir()),
    cacheMaxBytes: options.cacheMaxBytes ?? DEFAULT_CACHE_MAX_BYTES,
    userAgent: options.userAgent,
    resourceDir: options.resourceDir,
  };
}

export {DEFAULT_CACHE_MAX_BYTES, resolveStartOptions};
