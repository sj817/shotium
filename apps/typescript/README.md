# @shotkit/shotium

English · [简体中文](./README.zh.md)

Static HTML/CSS rendering engine extracted from Chromium: DOM layout, styling, and painting without browser overhead or JavaScript execution

[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg?label=npm)](https://www.npmjs.com/package/@shotkit/shotium) [![Chromium baseline](https://img.shields.io/badge/chromium-155.0.8048.0-4285F4?logo=googlechrome&logoColor=white)](https://chromium.googlesource.com/chromium/src/+/refs/tags/155.0.8048.0) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://github.com/sj817/shotium/blob/main/LICENSE)

`@shotkit/shotium` is the official Node.js SDK for [shotium](https://github.com/sj817/shotium). It extracts the core layout and painting capabilities from Chromium: Blink for DOM, styling, and layout, Skia for CPU rasterisation and encoding, and `//net` for HTTP fetching and disk caching. All browser-shell overhead—V8, the compositor, GPU processes, and DevTools—has been completely removed. Documents are rendered directly within the calling Node.js process on a dedicated engine thread, returning raw PNG, JPEG, or WebP byte buffers without spawning browser subprocesses, managing WebSocket connections, or risking process leaks

The package has **zero runtime dependencies**. The engine binary is distributed via six platform packages automatically resolved by npm via `optionalDependencies` (supporting Windows, macOS, and Linux on both x64 and arm64 architectures)

## Table of Contents

| Category | Topics and Quick Links |
|:---|:---|
| **Getting Started** | [Install](#install) · [Quick start](#quick-start) · [Direct capture](#direct-capture) · [Service lifecycle](#managed-lifecycle-in-a-long-running-service) · [Resident daemon](#resident-daemon-for-short-lived-processes) |
| **Recipes** | [HTML from memory](#html-from-memory) · [Local subresources](#local-subresources) · [Stream to disk](#buffer-or-file-output) · [Tiled rendering](#very-tall-pages-tiled-rendering) · [Disk cache](#http-disk-cache-management) · [Diagnostics](#diagnosing-failures) |
| **API Reference** | [screenshot()](#screenshotoptions) · [screenshotTiles()](#screenshottilesoptions) · [Lifecycle controls](#startoptions-status-stop-releasememoryoptions) · [daemon](#daemon) · [cache](#cache) · [Configuration](#screenshotoptions-1) |
| **Specifications** | [Type definitions](#type-definitions) · [Limits and boundaries](#limits-and-boundaries) · [License](#license) |

## Install

```bash
pnpm add @shotkit/shotium
# or: npm install @shotkit/shotium · yarn add @shotkit/shotium · bun add @shotkit/shotium
```

Requires Node.js 18 or newer. The package is native ESM with comprehensive TypeScript declarations:

- In ESM environments, import directly: `import { screenshot } from '@shotkit/shotium'`
- Supported synchronously via `require('@shotkit/shotium')` in Node.js 20.19 and 22.12 or newer
- In earlier CommonJS environments, use dynamic import: `const { screenshot } = await import('@shotkit/shotium')`

Importing the package does not trigger native engine initialisation. No native code runs until the first `screenshot()` call or an explicit `start()`

## Usage

### Quick start

```ts
import { writeFileSync } from 'node:fs';
import { screenshot } from '@shotkit/shotium';

// Only URL or file path is required; other options have sensible defaults
const { image } = await screenshot({ file: 'https://example.com' });
writeFileSync('example.png', image!);
```

### Direct capture

`screenshot()` and `screenshotTiles()` automatically initialise the engine on first invocation using default settings: a persistent HTTP disk cache under `~/.shotium/cache` and the default User-Agent. For standalone scripts and one-off tasks, no extra configuration is required:

```ts
import { screenshot } from '@shotkit/shotium';

const { image } = await screenshot({ file: 'https://example.com', fullPage: true, type: 'webp', quality: 85 });
```

### Managed lifecycle in a long-running service

In long-running Web and API services, initialise the engine during application bootstrap to eliminate cold-start latency on initial requests, periodically trim memory during low-traffic periods, and cleanly shut down during service termination:

```ts
import express from 'express';
import shotium, { screenshot } from '@shotkit/shotium';

const app = express();

const { cacheDir, cacheActive } = shotium.start({ cacheMaxBytes: 512 * 1024 * 1024 });
console.log(`shotium ready, cache ${cacheDir} (active: ${cacheActive})`);

app.get('/render', async (req, res, next) => {
  try {
    const { image, stats } = await screenshot({
      file: String(req.query.url),
      viewport: { width: 1280, height: 720 },
    });
    res.setHeader('Content-Type', 'image/png');
    res.setHeader('X-Render-Ms', stats.timing.render.toFixed(1));
    res.send(image);
  } catch (error) {
    next(error);
  }
});

setInterval(() => shotium.releaseMemory({ releaseWorkingSet: true }), 10 * 60 * 1000);

process.on('SIGTERM', async () => {
  await shotium.stop();
  process.exit(0);
});
```

**Process-level singleton constraint**: Blink maintains process-wide global state that cannot be re-initialised. All `screenshot()` calls within a single process share this instance and execute sequentially via an internal rendering queue. To parallelise rendering across multiple CPU cores, run multiple Node.js worker processes (e.g., using Node cluster or a process manager like PM2)

### Resident daemon for short-lived processes

For short-lived CLI commands or CI pipeline steps, starting Blink on every invocation adds significant cold-start overhead. The `daemon` module maintains a background daemon process, accessible over a Windows named pipe or a Unix Domain Socket, which is automatically spawned on first use:

```ts
import { daemon } from '@shotkit/shotium';

// Single invocation: connect (or spawn), capture, and disconnect
const { image } = await daemon.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  daemon: { name: 'cli-pool', idleTimeoutMs: 300_000 },
});

// Batch invocations: reuse a persistent connection
const client = await daemon.connect({ name: 'batch' });
try {
  for (const url of urls) {
    const { image } = await client.screenshot({ file: url });
  }
} finally {
  client.close(); // Closes client connection; daemon stays alive until idleTimeoutMs expires
}

// Query status or stop explicitly
const status = await daemon.status({ name: 'batch' });
await daemon.stop({ name: 'batch' });
```

Distinct `StartOptions` configurations spawn separate daemon instances. If `name` is omitted, the IPC socket endpoint is deterministically hashed from `cacheDir`, `userAgent`, and `resourceDir`, ensuring clients only attach to matching daemons

## Recipes

### HTML from memory

The engine does not accept `data:` URLs as the main document. To render dynamic HTML strings, write the markup to a temporary file:

```ts
import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { screenshot, type ScreenshotOptions } from '@shotkit/shotium';

export async function renderHtml(html: string, options: Omit<ScreenshotOptions, 'file'> = {}) {
  const dir = mkdtempSync(join(tmpdir(), 'shotium-'));
  const file = join(dir, 'index.html');
  try {
    writeFileSync(file, html, 'utf8');
    return await screenshot({ file, allowFileAccess: true, ...options });
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}
```

### Local subresources

Local HTML documents cannot access the host filesystem by default. When referencing local stylesheets, images, or font files via relative paths, specify `allowFileAccess: true`; otherwise, unauthorized requests fail and are recorded in `stats.failed`

```ts
const { stats } = await screenshot({ file: './templates/report.html', allowFileAccess: true });
if (stats.failed > 0) console.warn(`${stats.failed} subresource(s) failed`);
```

### Buffer or file output

When `path` is omitted, the encoded image is returned as an in-memory `image: Buffer`. When `path` is specified, the engine streams directly to disk using an atomic rename pattern and returns `image: null`

```ts
await screenshot({ file: 'https://example.com', path: './output.png' });
```

### Very tall pages (tiled rendering)

Single images are constrained by codec dimensions: up to 65,535 pixels per side for PNG and JPEG, and 16,383 pixels for WebP. `screenshotTiles()` loads and lays out the document once, slicing the rendered output into horizontal strips up to `tile.height` CSS pixels. When `path` contains `{n}`, each tile is written to disk as soon as it is encoded, keeping peak memory capped around the size of a single tile:

```ts
import { screenshotTiles } from '@shotkit/shotium';

const { tiles } = await screenshotTiles({
  file: './long-article.html',
  allowFileAccess: true,
  fullPage: true,
  tile: { height: 8000 },
  path: 'article-{n}.png', // article-1.png, article-2.png, ...
});
for (const tile of tiles) console.log(tile.path, tile.y, tile.height);
```

When `path` is omitted, each tile object contains its own `image: Buffer`

### HTTP disk cache management

The engine uses Chromium's integrated disk cache, defaulting to `~/.shotium/cache/<project-hash>`. All processes pointing to the same cache directory share cached resources across `stop()` cycles:

```ts
import { cache } from '@shotkit/shotium';

cache.getDir();                                  // Current project's cache directory
cache.getDirs({ target: 'all' });                // All cache directories under ~/.shotium/cache
const entries = await cache.getFiles();          // Enumerated cache entries [{ url, lastUsedMs, bytes, dir }]
await cache.clear({ glob: ['https://cdn.example.com/**'] });
await cache.clear({ maxAge: 7 * 86400, maxSize: 100 * 1024 * 1024 });
```

Per-request cache behavior can be controlled with `cache: 'reload' | 'no-store' | 'only-if-cached'`, applying consistently across both document and subresource fetches

### Diagnosing failures

When a capture fails, the returned Promise rejects with an `Error` containing an `error.stats` payload. This provides detailed diagnostics captured up to the failure point (e.g., HTTP status code, request count, failed subresource count, and stage timings):

```ts
try {
  await screenshot({ file: 'https://example.com/slow', pageGotoParams: { timeout: 5000 } });
} catch (error) {
  const { stats } = error as Error & { stats?: CaptureStats };
  if (stats) console.error(stats.httpStatus, stats.requests, stats.failed, stats.timing);
}
```

## API

### Module exports

```ts
import shotium, {
  screenshot, screenshotTiles,          // captures through the shared engine
  start, status, stop, releaseMemory,   // the shared engine's lifecycle
  runtime, Runtime,                     // the shared instance and its class
  daemon,                               // the resident engine
  cache, Cache,                         // the HTTP cache
} from '@shotkit/shotium';
```

The default export is the same set of functions on one object plus a live `running` getter. Every type below is exported as well: `ScreenshotOptions`, `ScreenshotTilesOptions`, `ScreenshotResult`, `ScreenshotTilesResult`, `ScreenshotTile`, `TileOptions`, `Viewport`, `Clip`, `PageGotoParams`, `CacheMode`, `CaptureStats`, `CaptureTiming`, `StartOptions`, `StartResult`, `ReleaseMemoryOptions`, `DaemonOptions`, `DaemonStatus`, `DaemonCapability`, `DaemonClient`, `CacheTarget`, `CacheEntry`, `CacheClearOptions`, `CacheClearResult`

### screenshot(options)

`screenshot(options: ScreenshotOptions): Promise<ScreenshotResult>`

Renders one screenshot through the shared engine, starting it if it is not up. Resolves with `{ image, stats }`; `image` is `null` when `path` was given. Rejects with a `TypeError` when the options are invalid, before the engine is touched, and with an `Error` carrying `stats` when the capture fails

### screenshotTiles(options)

`screenshotTiles(options: ScreenshotTilesOptions): Promise<ScreenshotTilesResult>`

Renders the region `fullPage`, `selector`, `clip` or the viewport would have produced, cut into horizontal tiles of at most `tile.height` CSS pixels each, top to bottom. Resolves with `{ tiles, stats }`. `path`, when given, must contain `{n}`; it becomes the tile's 1-based index

### start(options), status(), stop(), releaseMemory(options)

| Function | Returns | Description |
|---|---|---|
| `start(options?: StartOptions)` | `StartResult` | Initialises the engine, or safely reuses the running instance. Throws if options conflict with the already initialised engine configuration |
| `status()` | `StartResult` | Returns current engine operational status and configuration |
| `stop()` | `Promise<void>` | Drains the capture queue, frees temporary caches, and stops accepting new requests. Underlying Blink remains initialised and disk cache is retained; subsequent calls safely reuse this engine instance |
| `releaseMemory(options?: ReleaseMemoryOptions)` | `void` | Triggers Blink garbage collection, clears Skia raster caches, and trims allocator free lists. When `releaseWorkingSet: true` is set, requests the OS to reclaim physical memory pages (without modifying disk cache) |

`shotium.running` on the default export (or `status().running`) indicates whether the engine is ready and accepting capture tasks

### Runtime

`new Runtime()` allows callers to create isolated lifecycle controllers, providing identical methods and properties as the top-level module: `start`, `status`, `stop`, `releaseMemory`, `screenshot`, `screenshotTiles`, the `running` getter, and the `cache` instance. Note that `Runtime` manages lifecycle state rather than creating independent native engines: multiple `Runtime` instances in the same process coordinate around the single underlying native engine. `runtime` is the module's default shared instance

### daemon

| Function | Returns | Description |
|---|---|---|
| `daemon.connect(options?)` | `Promise<DaemonClient>` | Connects to a daemon process, automatically spawning one if not currently listening (unless `spawn: false`) |
| `daemon.screenshot(options)` | `Promise<ScreenshotResult>` | Performs a single capture via the daemon; `options` accepts standard `ScreenshotOptions` plus optional `daemon: DaemonOptions` |
| `daemon.screenshotTiles(options)` | `Promise<ScreenshotTilesResult>` | Performs tiled capture via the daemon |
| `daemon.start(options?)` | `Promise<DaemonStatus & { spawned: boolean }>` | Explicitly starts a daemon process and reports its status |
| `daemon.status(options?)` | `Promise<Partial<DaemonStatus> & { running: boolean; endpoint: string }>` | Queries daemon status without spawning a new process |
| `daemon.stop(options?)` | `Promise<{ stopped: boolean; endpoint: string }>` | Requests the daemon process to cleanly exit |

`DaemonClient` members returned by `connect()`:

| Member | Returns | Description |
|---|---|---|
| `screenshot(options)` | `Promise<ScreenshotResult>` | Executes a capture across the persistent connection |
| `screenshotTiles(options)` | `Promise<ScreenshotTilesResult>` | Executes a tiled capture across the persistent connection |
| `status()` | `Promise<DaemonStatus>` | Queries the daemon's status over this connection |
| `shutdown()` | `Promise<{ ok: boolean }>` | Requests the remote daemon process to shut down |
| `close()` | `void` | Closes the client connection while leaving the daemon running |
| `endpoint`, `closed` | `string`, `boolean` | The active IPC endpoint and socket connection status |

Daemons share the global disk cache. A daemon's cache directory can be retrieved via `daemon.status()` and pruned with `cache.clear({ target: <dir> })`

### cache

| Function | Returns | Description |
|---|---|---|
| `cache.getDir(options?: CacheTarget)` | `string` | Returns the resolved cache directory path for the given target |
| `cache.getDirs(options?: CacheTarget)` | `string[]` | Returns all matching cache directory paths |
| `cache.getFiles(options?: CacheTarget)` | `Promise<CacheEntry[]>` | Enumerates cached resources with metadata |
| `cache.clear(options?: CacheClearOptions)` | `Promise<CacheClearResult[]>` | Purges entries matching URL glob patterns, age, or size constraints |

The cache management API operates independently of the engine lifecycle and does not require an active engine instance

### ScreenshotOptions

| Field | Type | Default | Meaning |
|---|---|---|---|
| `file` | `string` | required | An `http:`, `https:` or `file:` URL, or a local path |
| `viewport` | `{ width?, height? }` | `1280 × 720` | The layout viewport in CSS pixels |
| `type` | `'png' \| 'jpeg' \| 'webp'` | `'png'` | Encoder |
| `quality` | `number` 1-100 | `90` | `jpeg` and `webp` only |
| `scale` | `number` 0.01-8 | `1` | Device scale factor |
| `fullPage` | `boolean` | `false` | Capture the whole document rather than the viewport |
| `selector` | `string` | none | Capture the box of the first element matching this CSS selector, resolved with `Document::querySelector`; nothing is injected into the page |
| `clip` | `{ x, y, width, height }` | none | A region of the document in CSS pixels |
| `omitBackground` | `boolean` | `false` | Keep the alpha channel instead of painting the white backdrop. Rejected for `jpeg` |
| `path` | `string` | none | Write the image here instead of returning it |
| `pageGotoParams.timeout` | `number` ms | `30000` | Abandon the load after this long |
| `pageGotoParams.waitUntil` | `'load' \| 'networkidle'` | `'load'` | `networkidle` also waits for 500 ms with nothing in flight, for documents that keep fetching after the load event |
| `allowFileAccess` | `boolean` | `false` | Let the document read `file:` subresources |
| `cache` | `CacheMode` | `'default'` | `default` is ordinary HTTP semantics; `reload` reads nothing and writes everything; `no-store` neither reads nor writes; `only-if-cached` never touches the network and a miss is an error. Applies to subresources as well as the document |
| `headers` | `Record<string, string>` | none | Extra request headers, sent with the document and with subresources same-origin with it, never to third-party origins |

`fullPage`, `selector` and `clip` are mutually exclusive. Unknown fields are rejected rather than ignored

`ScreenshotTilesOptions` is `ScreenshotOptions` plus `tile: { height: number }`, at most 32000 CSS pixels

### StartOptions

| Field | Type | Default | Meaning |
|---|---|---|---|
| `cacheDir` | `string \| null` | `~/.shotium/cache/<project-hash>` | Root of the HTTP disk cache. `null` disables caching |
| `cacheMaxBytes` | `number` | 256 MB | Ceiling on the directory. `0` is not unlimited: the backend then sizes itself from the volume's free space |
| `userAgent` | `string` | built-in | Overrides the `User-Agent` header |
| `resourceDir` | `string` | the engine's directory | Where `shotium_data.pak` and `shotium_strings.pak` are; only a source checkout needs it |

### DaemonOptions

`DaemonOptions` extends `StartOptions`:

| Field | Type | Default | Meaning |
|---|---|---|---|
| `name` | `string` | none | Address the daemon by name instead of by configuration |
| `endpoint` | `string` | derived | The pipe or socket path, overriding both the name and the derived address |
| `idleTimeoutMs` | `number` | `300000` | Exit after this long with no connections and nothing rendering; `0` never exits |
| `prewarm` | `boolean` | `true` | Render one throwaway document at startup so the first real request does not pay for lazy initialisation |
| `spawn` | `boolean` | `true` | With `false`, fail instead of starting a daemon when none is listening |
| `logFile` | `string` | `$SHOTIUM_DAEMON_LOG` | Where a spawned daemon writes diagnostics |
| `startTimeoutMs` | `number` | `20000` | How long to wait for a spawned daemon to bind, or an addressed one to answer its handshake |

### Results and statistics

- `ScreenshotResult` shape: `{ image: Buffer | null, stats: CaptureStats }`
- `ScreenshotTilesResult` shape: `{ tiles: ScreenshotTile[], stats: CaptureStats }`, where each tile is `{ image: Buffer | null, x, y, width, height, path? }` (coordinates in document CSS pixels)


`CaptureStats`:

| Field | Meaning |
|---|---|
| `requests` | Every resource the document asked for, itself included |
| `fromCache` | Answered from the HTTP cache. A stale entry revalidated with a `304` counts here too, so `timing.fetch` can be non-zero with this set |
| `failed` | Requests that failed |
| `bytes` | Decoded body bytes, summed |
| `httpStatus` | The document's own status; `0` for a `file:` URL |
| `finalUrl` | After redirects |
| `timing.fetch` | Fetching the top-level document. For a cold `https:` URL this is DNS, TCP, TLS and a round trip, and routinely the largest number here |
| `timing.render` | Parse, subresources, style, layout and paint |
| `timing.setup`, `wait`, `lifecycle`, `paint`, `raster`, `encode` | The phases inside a capture |
| `timing.total` | Wall clock for the whole capture |

`StartResult`:

| Field | Meaning |
|---|---|
| `running` | Whether this lifecycle is started. `false` does not mean nothing is running; it means this `Runtime` is stood down |
| `cacheDir` | The directory in use, or `null` when caching is off |
| `enginePath` | The directory the loaded engine came from, or `null` before one is loaded. A checkout can have two engines within reach, the platform package and a local build, and this says which answered |
| `cacheActive` | Whether the directory is actually being written to. `false` with a `cacheDir` set means the directory could not be opened and every capture pays for the network again |

`DaemonStatus`: `pid`, `endpoint`, `cacheDir`, `userAgent?`, `resourceDir?`, `warm` (has rendered at least once), `uptimeMs`, `connections`, `inFlight`, `served`, `idleTimeoutMs`, `version`, `protocolVersion` (the local wire generation, independent of the package version), `capabilities` (`'screenshot'`, `'tiles'`), plus `ok?`, `running?` and `spawned?` on the calls that report them

`CacheEntry` is `{ url, lastUsedMs, bytes, dir }`. `CacheClearResult` is `{ removed, bytesBefore, bytesAfter, dir }`; `removed` is `-1` when a whole directory was dropped in one operation

`CacheTarget.target` selects `'current'` (the default), `'all'`, a project hash as `getDir()` reports it, or an absolute path. `CacheClearOptions` adds `glob` (patterns matched against entry URLs, with `*`, `**`, `?` and `{a,b}`), `maxAge` in seconds and `maxSize` in bytes; `0` means no limit

### Errors

| Situation | Type | Description |
|---|---|---|
| Invalid options (missing `file`, unknown fields, conflicting parameters, etc.) | `TypeError` | Thrown synchronously at the JavaScript layer prior to invoking native code |
| Capture failure (unreachable document, selector not found, timeout) | `Error` | Contains `error.stats` holding complete stage timings and HTTP diagnostics |
| `start()` options conflict with running engine instance | `Error` | Cannot re-initialise or reconfigure active process engine |
| Daemon unreachable or handshake failure | `Error` | IPC communication exception |

### Environment variables

| Variable | Description |
|---|---|
| `SHOTIUM_ENDPOINT` | Overrides default IPC socket address for clients and daemons in this process |
| `SHOTIUM_DAEMON_LOG` | Default log file path for daemon processes spawned via `daemon.connect()` |

## Type definitions

The published `dist/index.d.ts` is generated from `src/types.ts`, which ships in the package. Key interface shapes:

```ts
// ==================== 1. Capture and Viewport Options ====================

export interface Viewport {
  /** Layout viewport width in CSS pixels, default 1280 */
  width?: number;
  /** Layout viewport height in CSS pixels, default 720 */
  height?: number;
}

export interface Clip {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface PageGotoParams {
  /** Load timeout in milliseconds, default 30000 */
  timeout?: number;
  /**
   * Readiness condition:
   * - 'load': wait for document parse completion and load event
   * - 'networkidle': wait for a 500ms window with no pending network requests
   */
  waitUntil?: 'load' | 'networkidle';
}

/** HTTP cache mode following standard Fetch semantics */
export type CacheMode = 'default' | 'reload' | 'no-store' | 'only-if-cached';

export interface ScreenshotOptions {
  /** Target http/https/file URL or local filesystem HTML path */
  file: string;
  /** Output image encoding format, default 'png' */
  type?: 'png' | 'jpeg' | 'webp';
  /** Capture entire document scroll height rather than the viewport */
  fullPage?: boolean;
  /** Capture bounding box of the first matching CSS selector */
  selector?: string;
  /** Compression quality 1-100 (jpeg and webp only), default 90 */
  quality?: number;
  /** Device scale factor DPR (0.01-8), default 1 */
  scale?: number;
  /** Retain alpha transparency instead of white backdrop (unsupported for jpeg) */
  omitBackground?: boolean;
  /** Stream directly to destination file path (returns image: null) */
  path?: string;
  /** Navigation and wait options */
  pageGotoParams?: PageGotoParams;
  /** Explicit rectangular crop region in CSS pixels */
  clip?: Clip;
  /** Layout viewport dimensions */
  viewport?: Viewport;
  /** Allow document to load local file: subresources, default false */
  allowFileAccess?: boolean;
  /** HTTP caching strategy, default 'default' */
  cache?: CacheMode;
  /** Custom request headers (sent to main document and same-origin subresources) */
  headers?: Record<string, string>;
}

export interface TileOptions {
  /** Maximum CSS pixel height covered per tile (up to 32000) */
  height: number;
}

export interface ScreenshotTilesOptions extends ScreenshotOptions {
  /** Tiling configuration */
  tile: TileOptions;
}

export interface ScreenshotTile {
  /** Encoded image Buffer, or null when path is specified */
  image: Buffer | null;
  /** Absolute coordinates within document in CSS pixels */
  x: number;
  y: number;
  width: number;
  height: number;
  /** Target file path written when path is configured */
  path?: string;
}

export interface ScreenshotResult {
  image: Buffer | null;
  stats: CaptureStats;
}

export interface ScreenshotTilesResult {
  tiles: ScreenshotTile[];
  stats: CaptureStats;
}

// ==================== 2. Performance Diagnostics & Timing ====================

export interface CaptureTiming {
  /** Top-level document fetch duration (DNS/TCP/TLS on cold URLs, in ms) */
  fetch: number;
  /** Document parse, subresources, styling, and layout duration (in ms) */
  render: number;
  /** Page/frame initial setup duration (in ms) */
  setup: number;
  /** Waiting for subresources and document load completion (in ms) */
  wait: number;
  /** Selector resolution and lifecycle advancement (in ms) */
  lifecycle: number;
  /** Extracting Blink paint records (in ms) */
  paint: number;
  /** Raster surface preparation and paint-record replay (in ms) */
  raster: number;
  /** Image format encoding duration (in ms) */
  encode: number;
  /** Total wall clock duration for the entire capture (in ms) */
  total: number;
}

export interface CaptureStats {
  /** Total network requests issued during document load (including document) */
  requests: number;
  /** Requests answered from HTTP disk cache (including 304 revalidations) */
  fromCache: number;
  /** Failed subresource requests */
  failed: number;
  /** Sum of decoded body bytes */
  bytes: number;
  /** Top-level document HTTP response code (0 for file: URLs) */
  httpStatus: number;
  /** Final destination URL after following redirects */
  finalUrl: string;
  /** Detailed phase timing metrics */
  timing: CaptureTiming;
}

// ==================== 3. Engine Lifecycle Controls ====================

export interface StartOptions {
  /** Root directory for HTTP disk cache; pass null to disable caching */
  cacheDir?: string | null;
  /** Ceiling on cache directory in bytes, default 256 MB (0 for auto) */
  cacheMaxBytes?: number;
  /** Custom global User-Agent header */
  userAgent?: string;
  /** Directory containing binary resource pak files */
  resourceDir?: string;
}

export interface StartResult {
  /** Whether the engine runtime is active */
  running: boolean;
  /** Active HTTP disk cache directory path */
  cacheDir: string | null;
  /** Loaded native engine binary directory */
  enginePath: string | null;
  /** Whether cache directory is actively writable and functioning */
  cacheActive: boolean;
}

export interface ReleaseMemoryOptions {
  /** Request OS reclaim physical working set pages */
  releaseWorkingSet?: boolean;
}

// ==================== 4. Resident Daemon Controls ====================

export interface DaemonOptions extends StartOptions {
  /** Named identifier for daemon instance (overrides configuration hash) */
  name?: string;
  /** Explicit IPC socket or named pipe endpoint */
  endpoint?: string;
  /** Idle shutdown timeout in milliseconds, default 300000 (5 min), 0 to disable */
  idleTimeoutMs?: number;
  /** Render disposable test document on boot to eliminate cold-start lag, default true */
  prewarm?: boolean;
  /** Automatically spawn daemon if not running, default true */
  spawn?: boolean;
  /** Diagnostic log file path for spawned daemon */
  logFile?: string;
  /** Timeout waiting for daemon startup and handshake response (in ms) */
  startTimeoutMs?: number;
}

export type DaemonCapability = 'screenshot' | 'tiles';

export interface DaemonStatus {
  ok?: boolean;
  running?: boolean;
  spawned?: boolean;
  pid: number;
  endpoint: string;
  cacheDir: string | null;
  userAgent?: string;
  resourceDir?: string;
  /** Whether engine has completed initial prewarm capture */
  warm: boolean;
  uptimeMs: number;
  connections: number;
  inFlight: number;
  served: number;
  idleTimeoutMs: number;
  version: string;
  protocolVersion: number;
  capabilities: DaemonCapability[];
}

// ==================== 5. HTTP Disk Cache ====================

export interface CacheTarget {
  /** Cache directory: 'current' (project root), 'all', or specific directory path */
  target?: 'current' | 'all' | (string & {});
}

export interface CacheEntry {
  /** Cached resource URL */
  url: string;
  /** Last access timestamp (Unix ms) */
  lastUsedMs: number;
  /** Resource byte length */
  bytes: number;
  /** Containing cache directory */
  dir: string;
}

export interface CacheClearOptions extends CacheTarget {
  /** URL glob matching patterns (supports *, **, ?, {a,b}) */
  glob?: string[];
  /** Evict entries unaccessed for longer than specified seconds (0 for no limit) */
  maxAge?: number;
  /** LRU evict entries until directory size is under bytes (0 for no limit) */
  maxSize?: number;
}

export interface CacheClearResult {
  /** Count of removed entries (-1 if entire directory was dropped) */
  removed: number;
  bytesBefore: number;
  bytesAfter: number;
  dir: string;
}
```

## Limits and boundaries

- **No JavaScript execution**: V8 is completely omitted from the build; `<script>` tags are ignored. Inputs must be fully prepared HTML (e.g., SSR output, template engine output, or static markup)
- **No `data:` URL as the main document**: Main documents must be supplied as local file paths, stdin streams, or HTTP(S) URLs
- **Single engine instance per process**: Blink maintains process-wide global state and cannot be re-initialised. Captures run sequentially through an internal worker queue. For multi-core scaling, deploy multiple processes or use independently named daemons
- **Raster coordinate limits**: Blink paints up to 32,767 CSS pixels per viewport window; taller documents are handled via automatic internal scroll-paging (abstracted by `fullPage` and `screenshotTiles()`). Single image output is bounded by codec formats: 65,535 pixels per dimension for PNG and JPEG, and 16,383 pixels for WebP

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](https://github.com/sj817/shotium/blob/main/LICENSE)

