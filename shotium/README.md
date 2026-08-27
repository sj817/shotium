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

shotium.runtime.start();

const png = await shotium.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  fullPage: true,
});

await shotium.runtime.stop();
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

### 1. In-Process Engine (`runtime`)

```ts
import { runtime, screenshot } from '@shotkit/shotium';

runtime.start({
  cacheDir: '/var/tmp/shotium-cache' // Optional HTTP disk cache. Default: null (off)
});

// Returns a Buffer, or null when `path` was given and the engine wrote the file
const buffer = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'webp',
  quality: 85,
});

// Hand memory back between batches without giving up the engine
runtime.purge({ releaseWorkingSet: true });

await runtime.stop();
```

**One engine per process, ever, and not one at a time.** Starting Blink writes process-wide statics it has no path to undo, so `stop()` is final: a `start()` after it throws, and so does a second `Runtime`. Concurrent callers are queued and served one at a time, because there is one renderer. Parallelism is therefore more processes, and a program that will want another screenshot later should stay started and call `purge()` rather than stopping.

#### `StartOptions`

```ts
interface StartOptions {
  /** Root of the HTTP disk cache. null (the default) disables caching. */
  cacheDir?: string | null;

  /** User-Agent sent with every request. */
  userAgent?: string;

  /** Where the engine looks for its resource packs. Defaults to the
   *  directory the addon was loaded from, which is right for an install. */
  resourceDir?: string;
}
```

---

### 2. Resident Daemon (`daemon`)

Recommended for CLI tools, ephemeral CI tasks, or serverless workers where startup latency is critical.

The daemon is the same engine in a process of its own, behind a local socket (Named Pipe on Windows, Unix domain socket on POSIX). It renders a blank page on start, so it is warm before the first real request arrives.

```ts
import { daemon } from '@shotkit/shotium';

// Connect to an existing daemon (automatically starts one if none is running)
const client = await daemon.connect();

const png = await client.screenshot({
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

  /** Output format (default: 'png') */
  type?: 'png' | 'jpeg' | 'webp';

  /** Viewport dimensions (default: 1280x720) */
  viewport?: { width?: number; height?: number };

  /** Capture full scrollable document */
  fullPage?: boolean;

  /** Capture element bounding box matching selector */
  selector?: string;

  /** Capture specific rectangular crop */
  clip?: { x: number; y: number; width: number; height: number };

  /** Image compression quality: 1-100 (jpeg and webp only, default: 90) */
  quality?: number;

  /** Device scale factor: 0.01 - 8.0 (default: 1.0) */
  scale?: number;

  /** Preserve transparent background (png/webp only) */
  omitBackground?: boolean;

  /** Output file destination path (returns null if specified) */
  path?: string;

  /** Navigation & wait options */
  pageGotoParams?: {
    timeout?: number;
    waitUntil?: 'load' | 'networkidle';
  };

  /** Allow document to access local file:// resources (default: false) */
  allowFileAccess?: boolean;
}
```

An option this interface does not list is a typo, and a typo that was quietly dropped is a screenshot that ignored what you asked for — so an unknown key is a `TypeError` rather than a silent no-op. `fullPage`, `selector` and `clip` are mutually exclusive.

---

## License

BSD-3-Clause. See [LICENSE](https://github.com/sj817/shotium/blob/main/LICENSE) for details.
