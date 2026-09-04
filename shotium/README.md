# @shotkit/shotium

> High-performance static HTML/CSS screenshot engine powered by a stripped Chromium Blink core.

[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://github.com/sj817/shotium/blob/main/LICENSE)
[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg)](https://www.npmjs.com/package/@shotkit/shotium)

---

## Quick Start

### 1. Installation

```bash
# pnpm 9.15.9
pnpm add @shotkit/shotium
```

Prebuilt platform binaries are installed automatically via optional dependencies across six architectures (Windows, macOS, and Linux on x64 and arm64). No local compiler or postinstall build script is required.

The package is published as native ESM:
- `import` is supported on Node.js 18+.
- Synchronous `require()` is supported on Node.js 22.12+ / 20.19+.
- Earlier Node.js versions can use dynamic `await import('@shotkit/shotium')`.

### 2. Basic Example

```ts
import shotium, { screenshot } from '@shotkit/shotium';

// 1. Initialize engine
shotium.start();

// 2. Render remote URLs, local HTML files, or inline HTML strings (data:text/html)
const { image, stats } = await screenshot({
  file: 'data:text/html,<h1 style="color: #0969da; font-family: sans-serif;">Hello Shotium</h1>',
  viewport: { width: 800, height: 400 },
});

console.log(`Rendered in ${stats.timing.render}ms, Total: ${stats.timing.total}ms`);

// 3. Shut down engine and release resources
await shotium.stop();
```

### Vite Demo

The [Genshin profile share-card demo](../apps/genshin-faruzan-profile-card) is a Vite-served HTML/CSS card. The sibling [`apps/demo`](../apps/demo) example starts its own HTTP server and captures the card with Shotium.

---

## Table of Contents

- [Overview & Key Highlights](#overview--key-highlights)
- [Execution Mode Selection Guide](#execution-mode-selection-guide)
- [Usage Modes](#usage-modes)
  - [1. In-Process Engine](#1-in-process-engine)
  - [2. Resident Daemon](#2-resident-daemon-daemon)
- [API Reference](#api-reference)
  - [`ScreenshotOptions` & `ScreenshotResult`](#screenshotoptions)
  - [`StartOptions` & `StartResult`](#startoptions)
  - [`CaptureStats` Metrics Breakdown](#capturestats)
  - [`daemon` Module & Status](#daemon-module)
  - [`cache` Management Module](#cache-module)
- [License](#license)

---

## Overview & Key Highlights

`@shotkit/shotium` provides Node.js / TypeScript bindings for **shotium**, a stripped-down Chromium engine built specifically for fast static page rendering.

By completely removing V8 and multi-process browser overhead, Shotium delivers:
- **Fast Cold Starts**: Under 350 ms
- **High Render Speed**: ~47 ms for single viewport capture (down to ~31 ms in-process)
- **Low Memory Footprint**: ~58 MB idle memory

The engine is loaded into the host process as a Node-API addon wrapping a shared library. With zero child process overhead, no inter-process image copying is needed, and `screenshot()` directly returns the encoded image buffer from Blink.

---

## Execution Mode Selection Guide

| Use Case | Recommended Mode | Key Benefit |
|---|---|---|
| **Long-Running Web / API Services** (Express, Fastify, NestJS) | **In-Process Engine** | Zero IPC overhead, zero process startup cost, lowest per-shot latency (~31 ms). |
| **CLI Tools / CI Pipelines / Serverless Tasks** | **Resident Daemon** | Cross-process pre-warmed engine reuse; connects in **2.3 ms**, eliminating cold starts. |

---

## Usage Modes

### 1. In-Process Engine

The engine runs directly inside your Node.js process via Node-API, bound to the C ABI in [`shot/shot_api.h`](https://github.com/sj817/shotium/blob/main/shot/shot_api.h). `screenshot()` returns the image buffer encoded directly by Blink (~**31 ms** per shot).

```ts
import shotium, { screenshot } from '@shotkit/shotium';

// Start engine and retrieve cache status
const { cacheDir, cacheActive } = shotium.start();

// 1. Capture remote URL
const res1 = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'webp',
  quality: 85,
});

// 2. Capture dynamically assembled inline HTML string (no temporary files on disk)
const html = `<div style="padding: 24px; background: #f6f8fa;"><h2>Invoice #1024</h2></div>`;
const res2 = await screenshot({
  file: `data:text/html;charset=utf-8,${encodeURIComponent(html)}`,
  viewport: { width: 600, height: 300 },
});

// 3. Memory reclamation strategies:
// - releaseMemory(): clears Blink heap, Skia caches, and PartitionAlloc free lists (instant)
// - releaseWorkingSet: true: additionally asks OS to reclaim physical working set memory
shotium.releaseMemory({ releaseWorkingSet: true });

// 4. Shut down engine
await shotium.stop();
```

#### Lifecycle & Execution Model

- **Process Singleton**:
  - Blink relies on process-wide statics that cannot be cleanly uninitialized.
  - Each process hosts a single engine instance; subsequent `new Runtime()` calls reuse the existing instance.
- **Start / Stop Semantics**:
  - Calling `stop()` drains the task queue, marks `running: false`, and releases working set memory.
  - Calling `start()` subsequent times re-activates the engine and preserves its warm disk cache.
- **Configuration Consistency**:
  - Engine options are established at initial creation.
  - Passing conflicting options in subsequent `start()` calls throws an explicit error rather than silently ignoring parameters.
- **Concurrency & Parallelism**:
  - Within a single engine instance, concurrent screenshot calls are queued and rendered sequentially.
  - To achieve parallel throughput, scale horizontally across multiple worker processes.
- **Status Reporting**:
  - `start()` and `status()` return `{ running, cacheDir, cacheActive, enginePath }`.
  - `enginePath` identifies the directory the loaded native engine came from and is `null` before the first engine load.
  - If `cacheDir` cannot be opened, `cacheActive` is set to `false`, and the engine operates safely in cacheless mode.

---

### 2. Resident Daemon (`daemon`)

Recommended for CLI tools, ephemeral CI tasks, or serverless workers where startup latency is critical.

The daemon runs the engine in a standalone background process exposed via a local IPC socket (Named Pipe on Windows, Unix domain socket on POSIX). It pre-renders a blank page upon startup to warm up all subsystems, allowing clients to connect with ~**2.3 ms** latency.

```ts
import { daemon } from '@shotkit/shotium';

// Connect to existing daemon, or automatically launch one in background
const client = await daemon.connect();

// Dispatch screenshot request
const { image, stats } = await client.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

// Manage daemon status or release memory from client connection
const clientStatus = await client.status();
await client.releaseMemory({ releaseWorkingSet: false });

client.close();

// Global daemon operations (optional)
const daemonInfo = await daemon.status();
console.log(`Daemon PID: ${daemonInfo.pid}, Uptime: ${daemonInfo.uptimeMs}ms, Served: ${daemonInfo.served}`);

// Stop daemon
await daemon.stop();
```

- **Request Multiplexing**: Multiple requests can be dispatched concurrently over a single connection; each message carries a unique `id`. The daemon processes and returns responses in completion order.
- **Multi-Instance Isolation**: Each daemon instance renders requests serially. To scale concurrent rendering, launch multiple distinct daemon instances using unique `name` identifiers.

---

## API Reference

### `ScreenshotOptions`

```ts
interface ScreenshotOptions {
  /** Target URL (http/https/file/data) or local file path */
  file: string;

  /** Output image format (default: 'png') */
  type?: 'png' | 'jpeg' | 'webp';

  /** Viewport dimensions (default: 1280x720) */
  viewport?: { width?: number; height?: number };

  /** Capture full scrollable document */
  fullPage?: boolean;

  /** Capture bounding box of matching CSS selector (resolved via Document::querySelector) */
  selector?: string;

  /** Capture specific rectangular coordinate region */
  clip?: { x: number; y: number; width: number; height: number };

  /** Compression quality: 1-100 (jpeg and webp only, default: 90) */
  quality?: number;

  /** Device scale factor: 0.01 - 8.0 (default: 1.0) */
  scale?: number;

  /** Preserve transparent background (png and webp only; jpeg has no alpha channel) */
  omitBackground?: boolean;

  /** Output file path. If specified, writes directly to disk and image returns null */
  path?: string;

  /** Page navigation options */
  pageGotoParams?: {
    /** Timeout in milliseconds (default: 30000) */
    timeout?: number;
    /**
     * load: wait until DOM is parsed and basic resources loaded (default)
     * networkidle: additionally wait for 500ms window with 0 in-flight requests (for late WebFonts/CSS)
     */
    waitUntil?: 'load' | 'networkidle';
  };

  /** Allow document to read local file:// subresources (default: false) */
  allowFileAccess?: boolean;

  /** HTTP cache strategy (default: 'default') */
  cache?: 'default' | 'reload' | 'no-store' | 'only-if-cached';

  /** Extra request headers, sent to same-origin URLs only */
  headers?: Record<string, string>;
}
```

> **Note**: `fullPage`, `selector`, and `clip` are mutually exclusive. Specifying more than one will throw a validation error.

#### `ScreenshotResult` Return Structure

```ts
interface ScreenshotResult {
  /** Encoded image buffer; null when path parameter was specified (written directly to disk) */
  image: Buffer | null;
  /** Detailed capture timing and network statistics */
  stats: CaptureStats;
}
```

### `screenshotTiles(options)`

The same capture, delivered as a stack of images instead of one. The region
that `fullPage`, `selector`, `clip` or the viewport would have produced is cut
into horizontal tiles of at most `tile.height` CSS pixels each; the document
is loaded and laid out once for all of them. Tiles are for callers that need
the page in pieces -- a chat platform's image size limit, a viewer that pages
-- not for memory: a plain `fullPage` capture already costs the same however
tall the page is (see below).

```ts
const {tiles, stats} = await shotium.screenshotTiles({
  file: 'https://example.com/a-very-long-article',
  fullPage: true,
  tile: {height: 8000},
});
for (const {image, y, height} of tiles) {
  // image: Buffer -- a PNG of this tile
  // y, height: where it sits in the document, in CSS pixels
}
```

```ts
interface ScreenshotTilesOptions extends ScreenshotOptions {
  /** Most CSS pixels per tile; the last tile is what is left. At most 32000. */
  tile: {height: number};
}

interface ScreenshotTilesResult {
  /** Top to bottom. Consecutive tiles share x and width; each starts where the previous ended. */
  tiles: Array<{
    image: Buffer | null;  // null when path was given
    x: number; y: number; width: number; height: number;
    path?: string;         // the file written, when path was given
  }>;
  stats: CaptureStats;
}
```

With `path`, every tile is written to disk and the path must contain `{n}`,
which becomes the tile's 1-based index: `page-{n}.png` writes `page-1.png`,
`page-2.png`, and so on. `daemon.screenshotTiles()` and
`client.screenshotTiles()` take the same options.

`path` is also the bounded-memory mode: the engine writes each encoded tile as
it finishes, so image-data memory stays near the current tile. Without `path`,
the promise returns every tile as a `Buffer`; completed buffers therefore stay
in memory until the promise resolves, and their total encoded size grows with
the captured height.

#### Very tall pages

`fullPage` renders a document of any height into one image, up to what the
format can hold: 65535 pixels on a side for `png` and `jpeg`, 16383 for
`webp`. A page taller than that is refused with the size and the limit named;
lower `scale`, or use `screenshotTiles()`, which has no such ceiling. There is
no engine-side height limit either way -- a document deeper than Blink paints
from one scroll position is painted in bands and joined without a seam.

#### Memory

The image is never in memory whole. It is rastered in strips of 256 rows and
encoded as each strip completes; a strip's memory is returned to the system the
moment the encoder is done with it, so a 1440 x 40000 PNG holds about 10 MB of
bitmap at any time rather than 236 MB. Images the page draws are decoded when
a strip first needs them, at the size they are drawn at, and dropped once the
last strip that draws them is encoded. Measured on a 41,000-pixel article
(peak private memory of the process, engine included): 94 MB for the viewport,
115 MB for the whole page as PNG or JPEG. What remains is the document itself
-- for a page of thirty photographs, the 72 MB of JPEG bytes it embeds -- and
the encoded output, which for a PNG of photographs can be the largest item;
`jpeg` or `webp` is the right format for those.

Lossless WebP (`type: 'webp'` with `quality: 100`) is the one exception: libwebp
needs the whole picture at once, plus working copies of it. Use lossy WebP or
tiles for a tall page.

#### Parameter Details

- **`file` Input Schemes**:
  - Remote URLs: `https://example.com`
  - Local Paths: `./template.html`, `/absolute/path/index.html`, `file:///...`
  - Inline HTML Strings: `data:text/html;charset=utf-8,<h1>Hello</h1>`
- **`cache` Strategies** (follows Web Fetch API, applies to document and subresources):
  - `default`: Standard HTTP caching behavior.
  - `reload`: Bypasses existing cache, fetches fresh resources from server, and updates cache.
  - `no-store`: Completely disables reading and writing to cache.
  - `only-if-cached`: Retrieves cached entries only; throws an immediate error if cache misses (no network request).
- **`headers` Scope**:
  - Strictly adheres to Same-Origin policy.
  - Passed headers (such as `Authorization` or `Cookie`) are sent only to the target site, and never forwarded to cross-origin external stylesheets or fonts.

---

### `StartOptions`

```ts
interface StartOptions {
  /**
   * HTTP disk cache directory. Defaults to a project-specific directory
   * under ~/.shotium/cache; set to null to disable disk caching.
   */
  cacheDir?: string | null;

  /** Maximum size ceiling for cache directory in bytes (default: 256 MB) */
  cacheMaxBytes?: number;

  /** Custom User-Agent string */
  userAgent?: string;

  /** Directory containing shotium_data.pak and shotium_strings.pak */
  resourceDir?: string;
}
```

#### `StartResult` Return Structure

```ts
interface StartResult {
  /** Whether the runtime instance is currently started */
  running: boolean;
  /** Active cache directory path (null when caching is disabled) */
  cacheDir: string | null;
  /** Directory the loaded native engine came from (null before the first load) */
  enginePath: string | null;
  /** Whether cache directory was opened successfully and is active */
  cacheActive: boolean;
}
```

---

### `CaptureStats`

Every capture operation returns detailed timing breakdown and network statistics:

```ts
interface CaptureStats {
  requests: number;     // Total resources requested by document (including itself)
  fromCache: number;    // Number of resource bodies served from HTTP disk cache
  failed: number;       // Number of failed subresource requests
  bytes: number;        // Total decoded body bytes (not transfer size)
  httpStatus: number;   // Main document HTTP status code (0 for file: / data: URLs)
  finalUrl: string;     // Final URL after resolving redirects
  timing: {
    fetch: number;      // Document retrieval: DNS, TCP, TLS, and round-trip latency
    render: number;     // Rendering: parsing, subresources, styles, layout, paint
    setup: number;      // Page/frame creation and document installation
    wait: number;       // Parsing, load completion and subresource wait
    lifecycle: number;  // Capture selection, style, layout and lifecycle
    paint: number;      // PaintRecord extraction
    raster: number;     // Surface preparation and PaintRecord replay
    encode: number;     // Image encoding duration
    total: number;      // Total wall-clock duration
  };
}
```

#### Metrics & Timing Breakdown

- **Network Latency Breakdown**: For cold `https:` requests, `timing.fetch` represents the majority of total latency. Cache hits reduce fetch latency to sub-millisecond levels:

| Scenario | `fetch` Latency | `render` Latency | `total` Latency |
|---|---|---|---|
| **Local file / Inline HTML** (`file:` / `data:`) | 0.2 ms | 20 ms | 25 ms |
| **HTTPS (Cold request)** | 321.1 ms | 16 ms | 350 ms |
| **HTTPS (Cache hit)** | 0.7 ms | 18 ms | 31 ms |

- **`fromCache` Semantics**: Indicates that the response body was served from disk. If an entry is revalidated via conditional request (304 Not Modified), network round-trip latency is still incurred while saving body payload transfer.
- **Failure Diagnostics (`error.stats`)**: When a capture fails or times out, the error object includes `error.stats` containing network metrics prior to the error.

---

### `daemon` Module

Manages resident daemon instances and IPC connections:

```ts
import { daemon } from '@shotkit/shotium';

// 1. Establish IPC connection
const client = await daemon.connect({
  name: 'custom-pool',      // Optional: daemon naming isolation
  idleTimeoutMs: 300000,    // Idle exit timeout when no connections active (default 5 min; 0 = never)
  prewarm: true,            // Pre-renders a blank page upon startup to warm up engine (default true)
});

// 2. Client instance methods
const res = await client.screenshot({ file: 'https://example.com' });
const status = await client.status();
await client.releaseMemory({ releaseWorkingSet: false });
client.close();

// 3. Global daemon management
const info: DaemonStatus = await daemon.status();
await daemon.stop();
```

#### `DaemonStatus` Structure

```ts
interface DaemonStatus {
  pid: number;              // Daemon OS process ID
  endpoint: string;         // IPC socket path / named pipe
  cacheDir: string | null;  // Active disk cache directory
  warm: boolean;            // Whether engine pre-warm has completed
  uptimeMs: number;         // Uptime in milliseconds
  connections: number;      // Current active client connections
  inFlight: number;         // Requests currently being rendered
  served: number;           // Total completed requests
  idleTimeoutMs: number;    // Configured idle timeout
  version: string;          // Engine version
  protocolVersion: number;  // Local daemon wire protocol generation
  capabilities: ('screenshot' | 'tiles')[]; // Supported operations
}
```

---

### `cache` Module

Manages persistent HTTP disk caching across processes and engine lifecycles:

```ts
import { cache } from '@shotkit/shotium';

// 1. Directory query
cache.getDir();                     // Current project's cache directory (absolute path)
cache.getDirs({ target: 'all' });   // List all shotium cache directories on the system

// 2. List cached file metadata
const files = await cache.getFiles(); // [{ url, lastUsedMs, bytes, dir }, ...]

// 3. Evict cache and inspect result
const result: CacheClearResult = await cache.clear({
  glob: ['https://example.com/**'], // Evict by URL glob pattern
  maxAge: 86400,                    // Evict entries unused for > 24h (seconds)
  maxSize: 64 * 1024 * 1024,        // Evict via LRU to under 64 MB
});

console.log(`Removed: ${result.removed}, Bytes before: ${result.bytesBefore}, Bytes after: ${result.bytesAfter}`);
```

#### Cache Design & Guidelines

- **Directory Structure**: Stored under `~/.shotium/cache/<project-hash>`, avoiding ephemeral `/tmp` directories that are automatically purged on reboot.
- **Index Integrity**: Cache files are stored using URL-key hashes with an internal index file. Cache eviction must be performed via the `cache` API rather than manual file deletion to preserve index consistency.
- **Cross-Process Sharing**: Multiple processes may concurrently access the same cache directory safely.

---

## License

BSD-3-Clause. See [LICENSE](https://github.com/sj817/shotium/blob/main/LICENSE) for details.
