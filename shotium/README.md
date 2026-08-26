# @shotkit/shotium

> High-performance static HTML/CSS screenshot engine powered by a stripped Chromium Blink core.

[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://github.com/sj817/shotium/blob/main/LICENSE)
[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg)](https://www.npmjs.com/package/@shotkit/shotium)

---

## Overview

`@shotkit/shotium` provides Node.js / TypeScript bindings for **shotium**, a stripped-down Chromium engine built specifically for fast static page rendering. By completely removing V8 and browser chrome overhead, Shotium delivers cold starts under 350 ms, single-shot captures in ~47 ms, and an idle memory footprint of ~58 MB.

```ts
import shotium from '@shotkit/shotium';

shotium.runtime.start({ workers: 4 });

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

Prebuilt platform binaries are installed automatically via npm optional dependencies.

---

## Usage

### 1. Multi-Process Pool (`runtime`)

Recommended for standard backend servers and continuous job queues.

```ts
import { runtime, screenshot } from '@shotkit/shotium';

// Optional: listen to runtime lifecycle events
runtime.on('crash', ({ worker }) => console.warn(`Worker ${worker} recovered from crash`));
runtime.on('timeout', ({ worker, timeout }) => console.warn(`Worker ${worker} timed out (${timeout}ms)`));

// Start pool
runtime.start({
  workers: 4,                        // Default: Math.max(1, Math.floor(cpuCount / 2))
  cacheDir: '/var/tmp/shotium-cache' // Optional HTTP disk cache
});

// Take screenshot (returns Buffer or writes to disk if 'path' is specified)
const buffer = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'webp',
  quality: 85,
});

await runtime.stop();
```

---

### 2. Resident Daemon (`daemon`)

Recommended for CLI tools, ephemeral CI tasks, or serverless workers where startup latency is critical.

`daemon` keeps a pre-warmed worker pool listening behind a local socket (Named Pipe on Windows, Unix domain socket on POSIX).

```ts
import { daemon } from '@shotkit/shotium';

// Connect to existing daemon (automatically starts one if none is running)
const client = await daemon.connect({ workers: 4 });

const png = await client.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

client.close();

// Check status or stop daemon
const status = await daemon.status();
await daemon.stop();
```

---

### 3. In-Process Native Engine (`@shotkit/shotium/native`)

Recommended for single-process, single-threaded batch rendering with minimum overhead (~31 ms per shot).

```ts
import { native } from '@shotkit/shotium/native';

const png = await native.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

// Purge cache and release working set after batch
native.purge({ releaseWorkingSet: true });
await native.stop();
```

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

  /** Auto retry count on failure (default: 0) */
  retry?: number;
}
```

---

## License

BSD-3-Clause. See [LICENSE](https://github.com/sj817/shotium/blob/main/LICENSE) for details.
