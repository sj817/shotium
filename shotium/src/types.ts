// The vocabulary of the package: what a caller passes in and what comes back.
//
// It lives in one file rather than beside the code that reads each field
// because these types are the published API. index.ts and native.ts both
// re-export them, so a consumer sees one `ScreenshotOptions` whichever entry
// point they came through -- and, more to the point, so there is one place
// where adding an option means adding it.

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
  /** How many times to re-send after a crash or a timeout. Default 0. */
  retry?: number;
}

export interface StartOptions {
  /**
   * Path to `shotium.exe`. Default `$SHOTIUM_BINARY`, then the platform
   * package for this machine, then `./bin/shotium.exe`.
   */
  binary?: string;
  /** Worker processes. Default half the cores, at least one, at most four. */
  workers?: number;
  /** Root of the per-worker HTTP disk caches. `null` disables caching. */
  cacheDir?: string|null;
  /** Extra flags passed to every worker. */
  args?: string[];
}

export interface WorkerEvent {
  worker: number;
  code?: number|null;
  signal?: NodeJS.Signals|null;
}

export interface DaemonOptions extends StartOptions {
  /**
   * Address the daemon by name instead of by configuration. Without it the
   * endpoint is a hash of `binary`, `workers`, `cacheDir` and `args`, so a
   * client never attaches to a pool that renders with something other than
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
   * Render one throwaway document per worker at startup, so the first real
   * request does not pay for whatever a worker initialises lazily. Default
   * true.
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
  binary: string;
  workers: number;
  cacheDir: string|null;
  args: string[];
  /** Every worker has rendered at least once. */
  warm: boolean;
  uptimeMs: number;
  connections: number;
  inFlight: number;
  served: number;
  idleTimeoutMs: number;
  version: string;
}

export interface NativeStartOptions {
  /**
   * Root of the HTTP disk cache. `null` disables caching entirely, which is
   * the default here: an in-process engine is often a short-lived program, and
   * a cache it never reads twice is a directory it leaves behind.
   */
  cacheDir?: string|null;
  /** Overrides the built-in user agent string. */
  userAgent?: string;
  /**
   * Where `shotium_data.pak` and `shotium_strings.pak` are. Defaults to the
   * directory the native engine was loaded from, which is where they ship.
   */
  resourceDir?: string;
}

export interface PurgeOptions {
  /**
   * Also ask the OS to take the engine's pages back. The next screenshot pays
   * them back in soft page faults -- a few milliseconds -- so this is for when
   * there may not be a next one soon.
   */
  releaseWorkingSet?: boolean;
}
