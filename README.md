# shotium

> High-performance static HTML/CSS screenshot engine built on a stripped-down Chromium core.

[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](LICENSE)
[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg)](https://www.npmjs.com/package/@shotkit/shotium)
[![Download](https://img.shields.io/badge/npm%20download-19.4%20MB%20(win32--x64)-green.svg)](https://www.npmjs.com/package/@shotkit/shotium)

**English** · [简体中文](README.zh.md)

---

## Overview

**shotium** is a lightweight, high-performance rendering engine designed specifically for capturing static screenshots from HTML and CSS. It is built by stripping down Chromium to its core rendering pipeline: Blink layout, Skia CPU rasterization, typography, image decoders, and the Chromium `//net` network stack.

**V8 and the browser layer are completely removed.** There is no `//content`, no GPU process, no compositor, and no DevTools protocol. What remains is about 41 MB of engine that starts in under 350 ms, captures a viewport in ~47 ms, and consumes significantly less memory than Headless Chrome or Puppeteer.

It ships two ways. For Node.js it is a Node-API addon over a shared library, loaded into the calling process: `npm install` pulls 19.4 MB for win32-x64 and nothing is spawned. For everything else it is a standalone executable on the [releases page](https://github.com/sj817/shotium/releases), 15.3 MB compressed.

```ts
import shotium from '@shotkit/shotium';

shotium.start();

const { image, stats } = await shotium.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  fullPage: true,
});

console.log(stats.timing.total, 'ms', stats.fromCache, 'of', stats.requests, 'cached');

await shotium.stop();
```

---

## Technical Scope

shotium is intended for server-side HTML/CSS rendering tasks, such as generating social preview cards, invoices, receipts, banners, PDF/PNG reports, and SSR page snapshots.

### Supported
- **Blink Layout Engine**: Full CSS Flexbox, Grid, custom web fonts, SVG, pseudo-elements, and CSS variables.
- **Image Formats**: Direct encoding to PNG, JPEG, and WebP with configurable quality and alpha channel support.
- **Chromium Networking**: Direct integration with Chromium's `//net` stack (HTTPS, HTTP/2, Brotli, redirects, disk cache, in-memory cookies).
- **In-Process Rendering**: The engine is a Node-API addon over a C ABI, not a spawned browser. No IPC, no temporary files, no port, and no image copied across a process boundary. A resident daemon is provided for callers that would rather not pay the start.

### Deliberate Omissions
- **No JavaScript Execution**: V8 is absent. `<script>` tags are ignored, and client-side framework hydration (e.g., pure CSR React/Vue apps) will not execute. Target documents must be server-rendered or static HTML.
- **No Browser Sandbox**: Security controls (such as SSRF prevention and URL allowlists) must be enforced by the caller. Local `file://` subresource loading is disabled by default (`allowFileAccess: false`).
- **Font Rendering Consistency**: Rasterization uses grayscale antialiasing with a fixed gamma curve and ignores host ClearType settings, ensuring byte-identical output across processes on the same OS.

---

## Installation

```bash
npm install @shotkit/shotium
```

Prebuilt binaries are published as optional dependencies for supported platforms (Windows, macOS, Linux on x64 and arm64). npm installs the matching package automatically without requiring build tools or postinstall download scripts.

---

## Usage

### 1. In-Process Engine

The engine runs inside your Node.js process, loaded through Node-API from the C ABI in [`shot/shot_api.h`](shot/shot_api.h). `screenshot()` returns the bytes Blink just encoded: nothing is spawned, and no image is copied across a process boundary (~31 ms per shot).

```ts
import shotium, { screenshot } from '@shotkit/shotium';

const { cacheDir, cacheActive } = shotium.start();

const { image, stats } = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'webp',
  quality: 85,
});

// Hand memory back between batches without giving up the engine
shotium.releaseMemory({ releaseWorkingSet: true });

await shotium.stop();
```

**One engine per process -- but `start()` and `stop()` are not rationed.** Starting Blink writes process-wide statics it has no path to undo, so a process gets one engine and a second `Runtime` adopts it rather than building another. That is a fact about how many engines there are, not about how many times you may ask for one: `stop()` stands the engine down (queue drained, memory returned, `running: false`) and `start()` picks the same one back up, warm cache and all, as often as you like. What `start()` will refuse is a *different* configuration -- the options are fixed when the engine is built, so naming one that disagrees with what is running throws instead of quietly rendering with the other value. Concurrent callers are queued and served one at a time, because there is one renderer, so parallelism is more processes.

`start()` reports what it came up as. `cacheActive: false` with a `cacheDir` set means the directory could not be opened and this engine is running without a cache -- correctly, silently, and a round trip slower on everything.

> **Upgrading from 0.2.** The lifecycle moved from `shotium.runtime.*` to the module itself, `purge()` became `releaseMemory()`, `screenshot()` now resolves to `{ image, stats }` rather than to the buffer alone, and `stop()` is no longer final -- a stopped engine starts again. `shotium.runtime` and the `Runtime` class are still exported for callers who own their own lifecycle.

---

### 2. Resident Daemon (`daemon`)

Recommended for CLI commands, CI pipelines, and ephemeral tasks where process startup overhead must be minimized.

The daemon is that same engine in a process of its own, behind a local domain socket or named pipe. It renders a blank page on start, so it is warm before the first real request arrives, and a later process connects with ~2.3 ms latency.

```ts
import { daemon } from '@shotkit/shotium';

// Connect to existing daemon, or automatically launch one if not running
const client = await daemon.connect();

const { image, stats } = await client.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

client.close();

// Optional management
const status = await daemon.status(); // { running: true, pid: 12345, warm: true, ... }
await daemon.stop();
```

One connection may have several requests outstanding, because every message carries an `id`. That is a convenience for the client rather than concurrency: the daemon holds one renderer too, and answers in the order it finished them. Two at once means two daemons, told apart by `name`.

---

### 3. Standalone CLI

Standalone executables are available from [GitHub Releases](https://github.com/sj817/shotium/releases) for environments without Node.js.

```bash
# Capture URL to file
shotium https://example.com --width 1280 --height 720 -o output.png

# Capture full-page local document as WebP
shotium --file page.html --full-page --type webp --quality 85 -o output.webp

# Stay resident and answer length-prefixed JSON requests on stdin
shotium --serve --cache-dir /var/tmp/shotium-cache
```

---

## API Reference

### `ScreenshotOptions`

```ts
interface ScreenshotOptions {
  /** Target URL (http/https/file) or local file path */
  file: string;

  /** Output image format (default: 'png') */
  type?: 'png' | 'jpeg' | 'webp';

  /** Viewport dimensions (default: 1280x720) */
  viewport?: { width?: number; height?: number };

  /** Capture entire scrollable document */
  fullPage?: boolean;

  /** Capture bounding box of matching CSS selector */
  selector?: string;

  /** Capture specific rectangular region */
  clip?: { x: number; y: number; width: number; height: number };

  /** Image compression quality: 1-100 (jpeg and webp only, default: 90) */
  quality?: number;

  /** Device scale factor: 0.01 - 8.0 (default: 1.0) */
  scale?: number;

  /** Preserve transparent background (png and webp only) */
  omitBackground?: boolean;

  /** Output file path. If provided, screenshot writes directly to disk and returns null */
  path?: string;

  /** Page navigation options */
  pageGotoParams?: {
    timeout?: number;
    waitUntil?: 'load' | 'networkidle';
  };

  /** Allow document to read local file:// subresources (default: false) */
  allowFileAccess?: boolean;

  /** What this capture may do with the HTTP cache (default: 'default') */
  cache?: 'default' | 'reload' | 'no-store' | 'only-if-cached';

  /** Extra request headers, sent to same-origin URLs only */
  headers?: Record<string, string>;
}
```

> **Note**: `fullPage`, `selector`, and `clip` are mutually exclusive. Specifying more than one will throw a validation error.

`cache` is spelled the way `fetch` spells it and means the same things: `reload` reads nothing and writes everything, `no-store` neither reads nor writes, and `only-if-cached` refuses to touch the network and fails on a miss. It applies to subresources as well as to the document.

`headers` stops at the origin boundary. A caller passing `Authorization` or `Cookie` means it for the site being photographed, so it is not forwarded to a stylesheet or font on another origin.

---

### `StartOptions`

```ts
interface StartOptions {
  /**
   * HTTP disk cache directory. Defaults to a per-project directory under
   * ~/.shotium/cache; null disables caching.
   */
  cacheDir?: string | null;

  /** Ceiling on that directory, in bytes (default: 256 MB) */
  cacheMaxBytes?: number;

  /** Override the built-in User-Agent */
  userAgent?: string;

  /** Where shotium_data.pak and shotium_strings.pak are */
  resourceDir?: string;
}
```

`start()` and `status()` return `{ running, cacheDir, cacheActive }`. `running` is about this lifecycle, not about the process: after `stop()` it is `false` while `cacheDir` still names the directory the stood-down engine holds.

---

### `CaptureStats`

Every capture reports what it cost. The numbers were always known inside the engine; before 0.3 none of them left it.

```ts
interface CaptureStats {
  requests: number;     // every resource the document asked for, itself included
  fromCache: number;    // body came from the HTTP cache (see below)
  failed: number;
  bytes: number;        // decoded body bytes, not transfer size
  httpStatus: number;   // the document's own status; 0 for a file: URL
  finalUrl: string;     // after redirects
  timing: {
    fetch: number;      // fetching the document: DNS, TCP, TLS, round trip
    render: number;     // parse, subresources, style, layout, paint
    encode: number;
    total: number;
  };
}
```

`timing.fetch` is the one that surprises people. For a cold `https:` URL it is routinely an order of magnitude larger than the render, which makes it the answer to most "why was this slow" questions:

```
local file            fetch   0.2 ms   render  20 ms   total   25 ms
https, cold           fetch 321.1 ms   render  16 ms   total  350 ms
https, cache hit      fetch   0.7 ms   render  18 ms   total   31 ms
```

`fromCache` says the body came from disk, which is not the same as "no network was touched". A stale entry that can be revalidated costs a conditional request and a 304 -- what the cache saved there is the download, not the round trip -- so `fromCache: 1` alongside a `timing.fetch` of 88 ms is an ordinary result and not a contradiction.

Statistics are attached to failures too, as `error.stats` -- a capture that timed out after fetching forty subresources has already explained itself.

---

### `cache`

The HTTP cache, which outlives any one engine: the directory is on disk whether or not anything is running, so these work before `start()` and after `stop()` -- and `stop()` does not empty it. A cache whose point is the next run has to survive the end of this one.

```ts
import { cache } from '@shotkit/shotium';

cache.getDir();                     // this project's directory, absolute, forward slashes
cache.getDirs({ target: 'all' });   // every shotium cache directory on this machine

await cache.getFiles();             // [{ url, lastUsedMs, bytes, dir }, ...]

await cache.clear();                                       // everything
await cache.clear({ glob: ['https://example.com/**'] });    // by URL pattern
await cache.clear({ maxAge: 86400 });                       // unused for a day
await cache.clear({ maxSize: 64 * 1024 * 1024 });           // evict LRU down to 64 MB
```

Cache directories live under `~/.shotium/cache`, one per project, named by a hash of the project root. Not the temporary directory, which is defined by not surviving -- `/tmp` is emptied on reboot and swept of anything untouched for ten days, and a cache whose whole value is the next run cannot live there.

`target` selects which directory: `'current'` (the default) is this project's, `'all'` is every one under the shared root, and a string is either an absolute path or a project hash.

Two things are worth knowing about how this is implemented, because both are places a reasonable guess is wrong:

- **The files are not named after URLs.** A cache directory holds files called `5349fbae98c6d9a1_0` -- the name is a hash of the entry key -- plus an `index`. So `getFiles()` returns URLs rather than filenames, and `glob` matches URLs. A pattern written against filenames would match nothing.
- **Removal goes through the cache backend, never through the filesystem.** Deleting entry files by hand leaves the index naming things that are gone, and the next process to open the directory rebuilds or discards it. The one exception is clearing a whole directory in a process that has no engine at all, which removes the directory outright -- safe precisely because nothing survives to disagree.

Several processes may share one cache directory and all of them will cache; the backend takes no cross-process lock. Within a single process a directory has one backend, which is why these calls borrow the running engine's.

---

## Performance Benchmarks

Measured on a 32-core Windows workstation across 10 static local HTML test documents at 1280x720 viewport, PNG output, `waitUntil: 'load'`, with 7 runs per test case (median reported). For detailed methodology and raw logs, see [`bench/cross/RESULTS.md`](bench/cross/RESULTS.md).

### 1. Throughput & Cold Start

| Engine | Cold Start (1 shot) | Marginal / Shot | 10 Pages, 4 in Flight | Working Set Memory (Private) |
|---|--:|--:|--:|--:|
| **shotium** | **352 ms** | **47 ms** | **237 ms (42 shots/s)** | **256 MiB (73 MiB private)** |
| Puppeteer (`chrome-headless-shell`) | 946 ms | 133 ms | 905 ms (11 shots/s) | 647 MiB (180 MiB private) |
| Puppeteer (`headless Chrome`) | 1,559 ms | 132 ms | 1,890 ms (5 shots/s) | 1,287 MiB (379 MiB private) |
| Playwright (`chrome-headless-shell`) | 962 ms | 150 ms | 1,171 ms (9 shots/s) | 652 MiB (215 MiB private) |
| Playwright (`headless Chrome`) | 1,385 ms | 146 ms | 1,276 ms (8 shots/s) | 789 MiB (282 MiB private) |

> The four-in-flight column was measured with four shotium **processes**, against four pages in one browser on the other side. One process holds one renderer, so shotium scales by process; one engine serialises its callers however many are waiting.

### 2. Client Connection to Pre-warmed Engine

| Engine | Client End-to-End | Connect Latency | Screenshot Time | Idle Memory (Total) | Idle Memory (Engine Only) |
|---|--:|--:|--:|--:|--:|
| **shotium daemon** | **250 ms** | **2.3 ms** | **57 ms** | **58 MiB** | **2.8 MiB** |
| Puppeteer (`chrome-headless-shell`) | 512 ms | 17.0 ms | 170 ms | 355 MiB | 287 MiB |
| Puppeteer (`headless Chrome`) | 588 ms | 19.0 ms | 204 ms | 587 MiB | 519 MiB |
| Playwright (`chrome-headless-shell`) | 764 ms | 38.0 ms | 189 ms | 272 MiB | 154 MiB |
| Playwright (`headless Chrome`) | 680 ms | 34.0 ms | 228 ms | 400 MiB | 299 MiB |

---

## Architecture

```
┌────────────────────────────────────────────────────────┐
│  shotium (TypeScript / Node.js API)                    │
│  Lifecycle · Serialized queue · Option validation      │
└──────────────────────────┬─────────────────────────────┘
                           │ Node-API: JSON in, bytes out
┌──────────────────────────▼─────────────────────────────┐
│  shotium.node  ──►  libshotium (C ABI, shot_api.h)     │
│                     same process, no IPC               │
│                                                        │
│  Blink Pipeline:                                       │
│  DOM ──► Layout / Style ──► PaintRecord ──► Skia Raster│
│                                                 │      │
│  Image Encoder (PNG / JPEG / WebP) ◄────────────┘      │
│                                                        │
│  Network: Chromium //net (HTTP/2, HTTPS, Disk Cache)   │
└────────────────────────────────────────────────────────┘
```

1. **Direct Blink Lifecycle**: Instead of dispatching IPC calls through `//content` and compositor layers, Shotium instantiates `PageNonOrdinary` and triggers `LocalFrameView::UpdateAllLifecyclePhases()` synchronously.
2. **Skia CPU Raster**: Draws the generated `cc::PaintRecord` directly to an in-memory `SkSurface`, passing raw pixels to Skia image codecs.
3. **Embedded Network Stack**: Links directly against Chromium's `//net` subsystem (`URLRequestContext`, BoringSSL, SpdySession).
4. **One Engine, Two Front Ends**: The npm platform package carries the shared library and the addon; the standalone executable is a separate download. Both call the same `shot::Capture`, and `tools/shot/node_check.cjs` asserts that they produce byte-identical images.

---

## Environment

| Variable | Effect |
|---|---|
| `SHOTIUM_ENDPOINT` | Overrides the daemon's socket path or named pipe. |
| `SHOTIUM_DAEMON_LOG` | Where a daemon started by `daemon.connect()` writes its diagnostics. |

The addon is loaded from the installed platform package, or from `shotium/native/build/Release/` in a source checkout. Resource packs are looked for beside it, which is where an install keeps them; a local build keeps them in its build directory instead, so point `resourceDir` at that:

```ts
shotium.start({ resourceDir: '/path/to/out/Shot' });
```

---

## C ABI / FFI Integration

For integration with languages other than JavaScript (C++, Rust, Go, Python), shotium exports a C interface in [`shot/shot_api.h`](shot/shot_api.h):

```c
#include "shot_api.h"

shot_engine* engine = NULL;
shot_buffer* error = NULL;
shot_engine_create("{}", &engine, &error);

shot_buffer* png = NULL;
shot_buffer* stats = NULL;  /* optional; pass NULL for none */
shot_engine_capture(engine, "{\"file\":\"https://example.com\"}",
                    &png, &stats, &error);

const uint8_t* data = shot_buffer_data(png);
size_t size = shot_buffer_size(png);

shot_buffer_free(png);
shot_buffer_free(stats);
shot_engine_destroy(engine);
```

The ABI version is **2** as of 0.3: `shot_engine_capture` grew the `out_stats`
parameter, and `shot_engine_status`, `shot_cache_list` and `shot_cache_clear`
were added. Check `shot_abi_version()` against `SHOT_ABI_VERSION` before
calling anything -- a prebuilt addon and a prebuilt engine are separate files,
and nothing stops them being separate versions.

---

## Building from Source

### Prerequisites

- [depot_tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up) on your `PATH`
- ~40 GB free disk space
- Toolchains:
  - **Windows**: Visual Studio 2022 + Windows SDK (10.0.26100.0 or 10.0.28000)
  - **macOS**: Xcode
  - **Linux**: Build dependencies (`./build/install-build-deps.sh --no-prompt --no-nacl`)

### Steps

```bash
mkdir shotium-build && cd shotium-build

cat > .gclient <<'EOF'
solutions = [{
  "name": "src",
  "url": "https://github.com/sj817/shotium.git",
  "managed": False,
  "custom_deps": {},
  "custom_vars": {"checkout_configuration": "small"},
}]
target_os = ["win"] # or ["mac"], ["linux"]
EOF

gclient sync --nohooks --no-history
gclient runhooks

cd src

# Generate ICU tables
python3 tools/shot/icu_repack.py \
  third_party/icu/cast/icudtl.dat \
  third_party/icu/shot/icudtl.dat --preset shot

mkdir -p out/Shot
echo 'import("//build/args/shot.gn")' > out/Shot/args.gn
# macOS: echo 'import("//build/args/shot-mac.gn")' > out/Shot/args.gn
# Linux: echo 'import("//build/args/shot-linux.gn")' > out/Shot/args.gn

gn gen out/Shot
ninja -C out/Shot shot
```

### Test Suites

```bash
python tools/shot/serve_check.py   out/Shot/shotium.exe  # Protocol and codecs
python tools/shot/net_check.py     out/Shot/shotium.exe  # HTTP, SSL, redirect, cache
node   tools/shot/node_check.cjs   out/Shot/shotium.exe  # Addon, queue, lifecycle
node   tools/shot/daemon_check.cjs out/Shot/shotium.exe  # Daemon socket and concurrency
python tools/shot/demo_check.py    out/Shot/shotium.exe  # Visual reftests (84 tests)
```

The two node suites load the addon, not the executable, so build it against the library this checkout just produced -- an addon checked against a different build is checking nothing. The executable argument is what they compare their output against.

```bash
export SHOT_INCLUDE_DIR=$PWD/shot SHOT_LIB_DIR=$PWD/out/Shot
npx node-gyp@13 rebuild -C shotium/native
cp out/Shot/libshotium.so out/Shot/*.pak shotium/native/build/Release/
npm --prefix shotium install && npm --prefix shotium run build
```

---

## License

BSD-3-Clause (Chromium upstream license). See [LICENSE](LICENSE).
