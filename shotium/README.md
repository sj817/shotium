# @shotkit/shotium

> High-performance static HTML/CSS screenshot engine powered by a stripped Chromium Blink core.

[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://github.com/sj817/shotium/blob/main/LICENSE)
[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg)](https://www.npmjs.com/package/@shotkit/shotium)

---

## Overview

`@shotkit/shotium` provides Node.js / TypeScript bindings for **shotium**, a stripped-down Chromium engine built specifically for fast static page rendering. By completely removing V8 and browser chrome overhead, Shotium delivers cold starts under 350 ms, single-shot captures in ~47 ms, and an idle memory footprint of ~58 MB.

The engine is loaded into your own process as a Node-API addon over a C ABI. Nothing is spawned, no image crosses a process boundary, and `screenshot()` returns the bytes Blink just encoded.

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

## Installation

```bash
npm install @shotkit/shotium
```

Prebuilt platform binaries are installed automatically via npm optional dependencies — six of them, covering Windows, macOS and Linux on x64 and arm64. There is no build step and no postinstall download.

The package is ESM. `import` works on Node 18 and up; `require()` of it needs Node 22.12 or 20.19, and anything older should use `await import('@shotkit/shotium')`.

---

## Usage

### 1. In-Process Engine

The engine runs inside your Node.js process, loaded through Node-API from the C ABI in [`shot/shot_api.h`](https://github.com/sj817/shotium/blob/main/shot/shot_api.h). `screenshot()` returns the bytes Blink just encoded: nothing is spawned, and no image is copied across a process boundary (~31 ms per shot).

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

Recommended for CLI tools, ephemeral CI tasks, or serverless workers where startup latency is critical.

The daemon is the same engine in a process of its own, behind a local socket (Named Pipe on Windows, Unix domain socket on POSIX). It renders a blank page on start, so it is warm before the first real request arrives.

```ts
import { daemon } from '@shotkit/shotium';

// Connect to an existing daemon (automatically starts one if none is running)
const client = await daemon.connect();

const { image, stats } = await client.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

client.close();

// Check status or stop the daemon
const status = await daemon.status(); // { running: true, pid: 12345, warm: true, ... }
await daemon.stop();
```

One connection may have several requests outstanding, because every message carries an `id`. That is a convenience for the client rather than concurrency: the daemon holds one renderer too, and answers in the order it finished them. Two at once means two daemons, told apart by `name`.

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
  fromCache: number;    // answered from the HTTP cache, no network touched
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

## License

BSD-3-Clause. See [LICENSE](https://github.com/sj817/shotium/blob/main/LICENSE) for details.
