// The vocabulary of the package: what a caller passes in and what comes back.
//
// It lives in one file rather than beside the code that reads each field
// because these types are the published API: index.ts re-exports them, so
// there is one place where adding an option means adding it.

/** A region of the document, in CSS pixels. */
export interface Clip {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface PageGotoParams {
  /** Milliseconds before the load is abandoned. Default 30000. */
  timeout?: number;
  /**
   * `load` waits for parsing to finish, the load event to fire and every
   * request to complete. `networkidle` additionally waits for a 500ms window
   * with nothing in flight, which matters for documents that keep fetching
   * after the load event -- CSS that pulls in more CSS, or a font a late style
   * change brought in.
   */
  waitUntil?: 'load'|'networkidle';
}

/** The viewport the document is laid out in. */
export interface Viewport {
  /** CSS pixels. Default 1280. */
  width?: number;
  /** CSS pixels. Default 720. */
  height?: number;
}

/**
 * What a capture may do with the HTTP cache, spelled the way `fetch` spells
 * it.
 *
 * - `default`: ordinary HTTP semantics. A fresh entry is used without asking,
 *   a stale one is revalidated, and the response updates the cache.
 * - `reload`: read nothing, write everything -- the browser's reload button.
 *   The next capture is fast again.
 * - `no-store`: neither read nor write. For a page that should not be left on
 *   this machine's disk, which an authenticated one usually should not.
 * - `only-if-cached`: the network may not be touched and a miss is an error.
 *   Useful for a deterministic re-render of something already fetched.
 */
export type CacheMode = 'default'|'reload'|'no-store'|'only-if-cached';

/** Where the milliseconds went. */
export interface CaptureTiming {
  /**
   * Fetching the top-level document. For a cold `https:` URL this is DNS, TCP,
   * TLS and a round trip, and it is routinely larger than everything below --
   * which is the single most useful thing this object says.
   */
  fetch: number;
  /** Parse, subresources, style, layout, prepaint, paint. */
  render: number;
  encode: number;
  /** Wall clock for the whole capture, so the three above can be checked. */
  total: number;
}

/** What one capture cost, and where its bytes came from. */
export interface CaptureStats {
  /** Every resource the document asked for, itself included. */
  requests: number;
  /**
   * Answered from the HTTP cache -- the body came from disk.
   *
   * Not the same as "no network was touched". A stale entry that can be
   * revalidated costs a conditional request and a 304, and counts here too;
   * what the cache saved is the download rather than the round trip. That is
   * why `timing.fetch` can be tens of milliseconds with this set.
   */
  fromCache: number;
  failed: number;
  /** Decoded body bytes, summed -- not the transfer size. */
  bytes: number;
  /** The document's own status. 0 for a `file:` URL. */
  httpStatus: number;
  /** After redirects, which is what relative URLs resolved against. */
  finalUrl: string;
  timing: CaptureTiming;
}

/** One screenshot, and what taking it cost. */
export interface ScreenshotResult {
  /**
   * The encoded image, or `null` when `path` was given: the engine wrote the
   * file itself and there is nothing left to hand back.
   */
  image: Buffer|null;
  stats: CaptureStats;
}

export interface ScreenshotOptions {
  /** An http/https/file URL, or a local path. */
  file: string;
  /** Default `png`. */
  type?: 'png'|'jpeg'|'webp';
  /** Capture the whole document rather than the viewport. */
  fullPage?: boolean;
  /**
   * Capture the box of the first element matching this CSS selector. Resolved
   * inside the renderer with Document::querySelector -- there is no JavaScript
   * engine, so nothing is injected into the page.
   */
  selector?: string;
  /** 1-100, `jpeg` and `webp` only. Default 90. */
  quality?: number;
  /** Device scale factor, 0.01-8. Default 1. */
  scale?: number;
  /**
   * Keep the alpha channel instead of painting the page's white backdrop.
   * Rejected for `jpeg`, which has no alpha channel.
   */
  omitBackground?: boolean;
  /** Write the image here instead of returning it, saving a round trip. */
  path?: string;
  pageGotoParams?: PageGotoParams;
  /** A region of the document, in CSS pixels. */
  clip?: Clip;
  /** The viewport the document is laid out in. */
  viewport?: Viewport;
  /**
   * Let the document read `file:` subresources. Off by default: a library does
   * not get to decide for its caller that a document may read the filesystem it
   * is rendered on.
   */
  allowFileAccess?: boolean;
  /**
   * What this capture may do with the HTTP cache. Default `default`.
   *
   * It applies to the subresources as well as the document: a `reload` that
   * refreshed the HTML and reused yesterday's stylesheet would be a confusing
   * thing to have asked for.
   */
  cache?: CacheMode;
  /**
   * Extra request headers, sent with the document and with the subresources
   * that are same-origin with it.
   *
   * Same-origin is the whole rule and it is not configurable. A caller passing
   * `Authorization` or `Cookie` means it for the site being photographed; a
   * page that pulls a script from a CDN must not have the credential
   * forwarded there.
   */
  headers?: Record<string, string>;
}

export interface StartOptions {
  /**
   * Root of the HTTP disk cache. `null` disables caching entirely.
   *
   * The default is a per-project directory under the system temporary
   * directory -- see `cache.getDir()`. Caching is on by default because the
   * alternative turned out to be worse: without it every capture of an
   * `https:` URL pays for DNS, TLS and a round trip, which for a small page is
   * most of the time the call takes and all of the time the caller did not
   * expect to spend.
   */
  cacheDir?: string|null;
  /**
   * Ceiling on the cache directory, in bytes. Default 256 MB.
   *
   * Zero is not "unlimited" -- it hands the decision to the backend, which
   * sizes itself against the volume's free space. That was a reasonable
   * default when every user of the cache had named a directory on purpose; for
   * one that appears by default under `~/.shotium` because somebody imported a
   * library, a number somebody chose is better than a number nobody did.
   */
  cacheMaxBytes?: number;
  /** Overrides the built-in user agent string. */
  userAgent?: string;
  /**
   * Where `shotium_data.pak` and `shotium_strings.pak` are. Defaults to the
   * directory the engine was loaded from, which is where they ship.
   */
  resourceDir?: string;
}

/** What `start()` reports about the engine it brought up. */
export interface StartResult {
  /**
   * Whether this lifecycle is started.
   *
   * A process has at most one engine, so `false` here does not mean there is
   * nothing running -- it means this `Runtime` is stood down. `cacheDir` below
   * is still answered from the engine, because a stood-down engine keeps its
   * cache directory and reporting `null` would say the cache had gone away
   * when what went away was the willingness to render.
   */
  running: boolean;
  /** The directory in use, or `null` when caching is off. */
  cacheDir: string|null;
  /**
   * Whether that directory is actually being cached into.
   *
   * A directory that cannot be created or written to costs nothing visible:
   * the engine renders exactly as well without a cache, only slower, and every
   * capture pays for the network again for a reason nothing reports. `false`
   * with a `cacheDir` set means the open failed; `false` with `cacheDir: null`
   * means no cache was asked for.
   *
   * It is not about sharing. Several processes may use one directory and all
   * of them cache -- the backend takes no cross-process lock -- so `true` in
   * two processes at once is the ordinary answer.
   */
  cacheActive: boolean;
}

export interface DaemonOptions extends StartOptions {
  /**
   * Address the daemon by name instead of by configuration. Without it the
   * endpoint is a hash of `cacheDir`, `userAgent` and `resourceDir`, so a
   * client never attaches to a daemon that renders with something other than
   * what it asked for.
   */
  name?: string;
  /** The pipe or socket to use, overriding both the name and the hash. */
  endpoint?: string;
  /**
   * Exit after this long with no connections and nothing rendering. Default
   * 300000; `0` never exits.
   */
  idleTimeoutMs?: number;
  /**
   * Render one throwaway document at startup, so the first real request does
   * not pay for whatever the engine initialises lazily. Default true.
   */
  prewarm?: boolean;
  /** Fail instead of starting a daemon when none is listening. */
  spawn?: boolean;
  /** Where a spawned daemon's diagnostics go. Default `$SHOTIUM_DAEMON_LOG`. */
  logFile?: string;
  /** How long to wait for a daemon this process started to bind. */
  startTimeoutMs?: number;
}

export interface DaemonStatus {
  ok?: boolean;
  running?: boolean;
  spawned?: boolean;
  pid: number;
  endpoint: string;
  cacheDir: string|null;
  userAgent?: string;
  resourceDir?: string;
  /** The engine has rendered at least once. */
  warm: boolean;
  uptimeMs: number;
  connections: number;
  inFlight: number;
  served: number;
  idleTimeoutMs: number;
  version: string;
}

/**
 * Options for `releaseMemory()`.
 *
 * Named for what it does rather than for `purge`, which it was called until
 * 0.3. With `cache.clear()` in the API the old name reads as though it clears
 * the cache, and it does not: it hands back blink's heap, skia's caches and
 * PartitionAlloc's free lists, all of which the engine rebuilds on demand.
 * Nothing on disk is touched.
 */
export interface ReleaseMemoryOptions {
  /**
   * Also ask the OS to take the engine's pages back. The next screenshot pays
   * them back in soft page faults -- a few milliseconds -- so this is for when
   * there may not be a next one soon.
   */
  releaseWorkingSet?: boolean;
}

/** Which cache directory an operation is about. */
export interface CacheTarget {
  /**
   * `current` (the default) is this project's directory, `all` is every
   * directory shotium has created under the shared root -- `~/.shotium/cache`
   * -- and a string is either an absolute path or one project hash as
   * `getDir()` reports it.
   *
   * The absolute path is there because `start({cacheDir})` accepts any
   * directory: without it, a caller who chose their own cache would have the
   * one cache these methods could not see.
   *
   * `all` exists because the directories are per-project by default, so
   * "clear shotium's caches" is otherwise something a caller cannot express
   * without already knowing where the other projects were.
   */
  target?: 'current'|'all'|(string&{});
}

/** One resource the cache is holding. */
export interface CacheEntry {
  /** The resource, not the backend's key -- see `cache.getFiles()`. */
  url: string;
  /** Milliseconds since the Unix epoch. */
  lastUsedMs: number;
  bytes: number;
  /** Which cache directory it was found in. */
  dir: string;
}

export interface CacheClearOptions extends CacheTarget {
  /**
   * Glob patterns matched against entry URLs -- not against filenames, which
   * are hashes and would match nothing anybody would think to write.
   *
   * Supports `*` (within a path segment), `**` (across segments), `?` and
   * `{a,b}`. Matching happens here rather than in the engine: the entries come
   * back first, the patterns are applied to their URLs, and the ones that
   * matched are what gets removed.
   */
  glob?: string[];
  /**
   * Remove entries not used for this many seconds. `0`, the default, means no
   * age limit.
   */
  maxAge?: number;
  /**
   * Evict least-recently-used entries until the directory is at or below this
   * many bytes. `0`, the default, means no size limit.
   */
  maxSize?: number;
}

export interface CacheClearResult {
  /**
   * How many entries went. `-1` when the whole directory was dropped in one
   * operation, which the backend does without counting them.
   */
  removed: number;
  bytesBefore: number;
  bytesAfter: number;
  /** Which directory this result is for. */
  dir: string;
}
