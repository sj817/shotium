import fs from 'node:fs';
import path from 'node:path';

import type {
  CacheClearOptions,
  CacheClearResult,
  CacheEntry,
  CacheTarget,
} from '../types.js';

import * as binding from './binding.js';
import type {Engine as Handle} from './binding.js';
import {cacheRoot, defaultCacheDir, normalizePath} from './config.js';

/**
 * Turns one glob into a regular expression over a URL.
 *
 * The dialect is the small one everybody already knows -- `*`, `**`, `?`,
 * `{a,b}` -- and it is implemented here rather than depended on because this
 * package has no runtime dependencies and a matcher is thirty lines. `*` stops
 * at `/` and `**` does not, which is the distinction that makes
 * `https://example.com/*` mean one level and `https://example.com/**` mean the
 * site.
 *
 * Everything else is escaped, which matters more than usual here: the subjects
 * are URLs, and a URL is mostly characters that mean something to a regular
 * expression.
 */
function globToRegExp(pattern: string): RegExp {
  let out = '';
  for (let i = 0; i < pattern.length; i++) {
    const c = pattern[i];
    if (c === '*') {
      if (pattern[i + 1] === '*') {
        out += '.*';
        i++;
        // `/**/` should also match the zero-segment case, so that
        // `https://x/**/y` matches `https://x/y`.
        if (pattern[i + 1] === '/') {
          out += '/?';
          i++;
        }
      } else {
        out += '[^/]*';
      }
    } else if (c === '?') {
      out += '[^/]';
    } else if (c === '{') {
      const end = pattern.indexOf('}', i);
      if (end === -1) {
        out += '\\{';
      } else {
        const alternatives =
            pattern.slice(i + 1, end).split(',').map(escapeLiteral);
        out += `(?:${alternatives.join('|')})`;
        i = end;
      }
    } else {
      out += escapeLiteral(c);
    }
  }
  return new RegExp(`^${out}$`);
}

function escapeLiteral(text: string): string {
  return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

/** Whether `url` matches any of `patterns`. No patterns matches nothing. */
function matchesAny(url: string, patterns: RegExp[]): boolean {
  return patterns.some((pattern) => pattern.test(url));
}

/**
 * What a cache directory occupies, for the one path that reports a size
 * without a backend to ask.
 *
 * The sum of the files rather than the sum of the entries, so it will differ
 * from what `clear()` reports through the backend by the index and by whatever
 * rounding the filesystem does. It is the honest number for "what is about to
 * be deleted", which is what it is used for.
 */
function directorySize(dir: string): number {
  let total = 0;
  let names: fs.Dirent[] = [];
  try {
    names = fs.readdirSync(dir, {withFileTypes: true});
  } catch {
    return 0;
  }
  for (const entry of names) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      total += directorySize(full);
      continue;
    }
    try {
      total += fs.statSync(full).size;
    } catch {
      // Raced with something else clearing the same directory. Not an error:
      // a file that is already gone contributes nothing to what is left.
    }
  }
  return total;
}

/**
 * Which directories an operation covers.
 *
 * `current` is this project's, `all` is every directory under the shared root,
 * and anything else is taken as a project hash. `all` reads the root rather
 * than remembering what it created: another process's directory is as much
 * shotium's as this one's, and a caller asking to clear them all means the
 * ones on disk.
 */
function resolveTargets(target: CacheTarget['target']): string[] {
  if (target === 'all') {
    const root = cacheRoot();
    let names: string[] = [];
    try {
      names = fs.readdirSync(root);
    } catch {
      // No root means nothing has been cached yet, which is an empty list and
      // not an error: a caller clearing an empty cache asked for a state that
      // already holds.
      return [];
    }
    return names.map((name) => normalizePath(path.join(root, name)))
        .filter((dir) => {
          try {
            return fs.statSync(dir).isDirectory();
          } catch {
            return false;
          }
        });
  }
  if (target === undefined || target === 'current') {
    return [defaultCacheDir()];
  }
  // A directory, if it looks like one. `start({cacheDir})` takes any path, so
  // a caller who chose their own has to be able to name it here -- otherwise
  // the cache they configured is the one cache these methods cannot see.
  if (path.isAbsolute(target)) {
    return [normalizePath(target)];
  }
  // Otherwise a project hash. Resolved against the root rather than used as a
  // path, so that a relative string cannot reach outside it by accident.
  return [normalizePath(path.join(cacheRoot(), target))];
}

/**
 * The cache, from the outside.
 *
 * Every method takes the engine handle if there is one, and that is not an
 * optimisation. Within one process a cache directory has one backend: asking
 * for a second one on the directory the engine holds waits for the engine's to
 * go away, which it will not do while the engine is up. Borrowing is the only
 * thing that returns.
 *
 * "If there is one" means the process, not the lifecycle. `stop()` stands the
 * engine down without tearing it down, so an engine that has been stopped
 * still holds its directory and still has to be borrowed from -- which is also
 * what makes the cache survive a stop, and outlive one, and be worth having.
 *
 * Across processes there is no such constraint -- several of them may share a
 * directory and all of them cache.
 *
 * The engine is fetched through a callback rather than held, because this
 * object is built once at import time and the engine comes and goes.
 */
export class Cache {
  constructor(private readonly engineHandle: () => Handle | null) {}

  /**
   * This project's cache directory, absolute and with forward slashes.
   *
   * It exists whether or not anything has been written to it -- the answer is
   * "where the cache goes", not "where a cache is".
   */
  getDir(options: CacheTarget = {}): string {
    const targets = resolveTargets(options.target);
    return targets.length > 0 ? targets[0] : defaultCacheDir();
  }

  /** Every directory the target names. `all` can be several; the rest, one. */
  getDirs(options: CacheTarget = {}): string[] {
    return resolveTargets(options.target);
  }

  /**
   * What the cache is holding, by URL.
   *
   * Named `getFiles` for the operation callers reach for, and deliberately not
   * returning filenames: the files in a cache directory are called things like
   * `5349fbae98c6d9a1_0`, because the name is a hash of the entry key. A list
   * of those answers no question anybody has. The URLs are what the entries
   * are, and they are what `clear({glob})` matches against.
   *
   * This opens every entry to read its key and size, so it is a diagnostic
   * rather than something to put on a request path.
   */
  async getFiles(options: CacheTarget = {}): Promise<CacheEntry[]> {
    const native = binding.load();
    const entries: CacheEntry[] = [];
    for (const dir of resolveTargets(options.target)) {
      if (!fs.existsSync(dir)) {
        continue;
      }
      const json = await native.cache(
          this.handleFor(), /*clearing=*/ false, JSON.stringify({
            cacheDir: dir,
          }));
      const listed = JSON.parse(json) as Array<Omit<CacheEntry, 'dir'>>;
      for (const entry of listed) {
        entries.push({...entry, dir});
      }
    }
    return entries;
  }

  /**
   * Removes what the options select. With no options, everything.
   *
   * The three filters compose, and `glob` is applied here rather than in the
   * engine: the entries are listed, their URLs are matched, and the ones that
   * matched are what the engine is asked to remove. That keeps the pattern
   * dialect in the layer whose users have opinions about pattern dialects, and
   * keeps the engine's interface to exact URLs.
   *
   * Removal goes through the cache backend, never through the filesystem.
   * Deleting the files directly would leave the backend's index naming entries
   * that are no longer there, and the next process to open the directory
   * either rebuilds the index from disk or, having found it inconsistent,
   * discards it. That is the difference between clearing a cache and
   * corrupting one.
   */
  async clear(options: CacheClearOptions = {}): Promise<CacheClearResult[]> {
    const native = binding.load();
    const patterns = (options.glob ?? []).map(globToRegExp);
    const results: CacheClearResult[] = [];

    // Clearing everything, in a process that has no engine at all: remove the
    // directory.
    //
    // This is the one case where touching the filesystem is correct rather
    // than reckless. The danger in deleting cache files by hand is a partial
    // delete -- an index left naming entries that are gone -- and there is no
    // such thing when the index goes with them. What is left is a directory
    // that does not exist, which is exactly what an empty cache looks like
    // before anything has written to it.
    //
    // It is also the fast path a short script gets: emptying a cache without
    // starting Blink to do it costs a few milliseconds instead of the tens
    // that building an engine does.
    const unfiltered = patterns.length === 0 && !options.maxAge &&
        !options.maxSize;
    if (unfiltered && !this.handleFor()) {
      for (const dir of resolveTargets(options.target)) {
        const before = directorySize(dir);
        fs.rmSync(dir, {recursive: true, force: true});
        results.push(
            {removed: -1, bytesBefore: before, bytesAfter: 0, dir});
      }
      return results;
    }

    for (const dir of resolveTargets(options.target)) {
      if (!fs.existsSync(dir)) {
        continue;
      }
      const request: Record<string, unknown> = {cacheDir: dir};

      if (patterns.length > 0) {
        const json = await native.cache(
            this.handleFor(), /*clearing=*/ false,
            JSON.stringify({cacheDir: dir}));
        const listed = JSON.parse(json) as Array<Omit<CacheEntry, 'dir'>>;
        const urls =
            listed.filter((entry) => matchesAny(entry.url, patterns))
                .map((entry) => entry.url);
        // Nothing matched, so nothing is asked for. Falling through with an
        // empty `urls` would be read by the engine as "no URL filter", which
        // combined with no other filter empties the directory -- the opposite
        // of what a pattern that matched nothing means.
        if (urls.length === 0 && options.maxAge === undefined &&
            options.maxSize === undefined) {
          results.push({removed: 0, bytesBefore: 0, bytesAfter: 0, dir});
          continue;
        }
        request.urls = urls;
      }

      if (options.maxAge) {
        request.unusedSinceMs = Date.now() - options.maxAge * 1000;
      }
      if (options.maxSize) {
        request.maxBytes = options.maxSize;
      }

      const json = await native.cache(
          this.handleFor(), /*clearing=*/ true, JSON.stringify(request));
      results.push({
        ...(JSON.parse(json) as Omit<CacheClearResult, 'dir'>),
        dir,
      });
    }
    return results;
  }

  /**
   * The engine handle, when there is an engine.
   *
   * Passed for every directory and not only the engine's own. It is never
   * wrong to pass it -- the engine's thread can open any directory, and for
   * the one it already has open, borrowing its backend is the only thing that
   * returns. It is passing `null` while an engine is up that hangs, which is
   * why this is conditional on neither the directory asked for nor on whether
   * the engine is currently accepting captures.
   */
  private handleFor(): Handle|null {
    return this.engineHandle();
  }
}
