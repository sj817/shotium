# @shotkit/shotium

> High-performance, lightweight static HTML/CSS screenshot engine powered by a stripped Chromium Blink core.

[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg?label=npm)](https://www.npmjs.com/package/@shotkit/shotium)
[![Chromium Baseline](https://img.shields.io/badge/chromium-155.0.8048.0-4285F4?logo=googlechrome&logoColor=white)](https://chromium.googlesource.com/chromium/src/+/refs/tags/155.0.8048.0)
[![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases)
[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://github.com/sj817/shotium/blob/main/LICENSE)

`@shotkit/shotium` provides official Node.js and TypeScript bindings for **shotium**, a stripped-down Chromium engine built specifically for fast, deterministic, static document screenshots.

By stripping out the V8 JavaScript engine, the browser shell (`//content`), the GPU process, and DevTools, Shotium eliminates browser boot latency, IPC serialization overhead, and orphaned processes. It renders HTML/CSS via Blink and Skia directly inside your process (or in a pre-warmed resident daemon), returning PNG, JPEG, or WebP buffers in tens of milliseconds.

<details>
<summary><b>🇨🇳 点击展开查看简体中文文档 / Click to expand Chinese documentation</b></summary>

<br>

> **基于精简 Chromium Blink 内核的高性能、轻量级静态 HTML/CSS 截图引擎。**

### 核心优势

- **Chromium 155 内核基准**：保留的 Blink DOM/CSS 排版引擎、Skia 图形库与 `//net` 网络栈深度同步至 Chromium `155.0.8048.0`。全面支持现代 CSS Grid、Flexbox、容器查询 (Container Queries)、`@font-face`、SVG、CSS 变量与渐变。
- **零外部运行时依赖**：自动通过可选依赖安装 6 大平台架构的预编译原生二进制（Windows / macOS / Linux，x64 与 arm64），无需本地编译器或单独下载 Chrome。
- **进程内 Node-API 调用**：直接使用原生异步服务（GN 构建的 `shotium.node`），无子进程拉起开销、无 WebSocket 通信、无僵尸进程。
- **确定性输出**：固定伽马曲线的灰度抗锯齿排版，确保跨平台像素逐字节一致。
- **极致内存控制**：引擎内核仅约 15 MB 私有常驻内存，活跃渲染工作集仅约 50~70 MB。

---

### 安装与模块兼容

```bash
# pnpm
pnpm add @shotkit/shotium

# npm
npm install @shotkit/shotium

# yarn / bun
yarn add @shotkit/shotium
bun add @shotkit/shotium
```

- **ESM (`import`)**：Node.js 18+ 原生支持。
- **CommonJS (`require`)**：Node.js 22.12+ 与 20.19+ 支持原生同步 `require('@shotkit/shotium')`。
- **较早 Node.js 版本**：可使用动态导入 `const { screenshot } = await import('@shotkit/shotium')`。

---

### 快速上手

无需手动配置或初始化引擎，直接调用 `screenshot()` 即可：

```ts
import { writeFileSync } from 'node:fs';
import { screenshot } from '@shotkit/shotium';

// 首次调用会自动以默认配置拉起引擎，无需手动 start()
const { image, stats } = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'png',
});

console.log(`渲染耗时: ${stats.timing.render.toFixed(1)}ms, 总耗时: ${stats.timing.total.toFixed(1)}ms`);
writeFileSync('example.png', image!);
```

---

### 三种运行范式

| 运行范式 | 适用场景 | 特点说明 |
|---|---|---|
| **直接调用** | 简单脚本、快速验证 | 零配置；首次调用时按默认配置自动启动引擎。 |
| **常驻 Web 服务管理** | Express、Fastify、NestJS 服务端 | 服务启动时预热并配置缓存，高峰后主动回收内存，停机时优雅关闭。 |
| **常驻守护进程 (Daemon)** | CLI 命令行工具、CI 流水线、Serverless | 后台常驻预热进程，客户端建立连接仅需约 **2 ms**，彻底消除冷启动。 |

#### 1. 直接调用（零配置）
直接调用 `screenshot()` 或 `screenshotTiles()`，引擎会在当前进程按默认规则（项目持久化缓存位于 `~/.shotium/cache`）自动初始化。

#### 2. 常驻 Web 服务生命周期管理
在长期运行的 Web API 服务中，建议在服务启动时显式调用 `start()` 固化配置并预热：

```ts
import express from 'express';
import shotium, { screenshot } from '@shotkit/shotium';

const app = express();

// 1. 服务启动时初始化引擎（设置持久化磁盘缓存上限）
const { cacheDir, cacheActive } = shotium.start({
  cacheMaxBytes: 512 * 1024 * 1024, // 512 MB 缓存
});

app.get('/render', async (req, res, next) => {
  try {
    const { image } = await screenshot({
      file: req.query.url as string,
      viewport: { width: 1280, height: 720 },
      type: 'png',
    });
    res.setHeader('Content-Type', 'image/png');
    res.send(image);
  } catch (err) {
    next(err);
  }
});

// 2. 流量高峰后主动释放非必要缓存，并通知 OS 回收物理工作集
setInterval(() => {
  shotium.releaseMemory({ releaseWorkingSet: true });
}, 10 * 60 * 1000);

// 3. 停机时安全清理
process.on('SIGTERM', async () => {
  await shotium.stop();
  process.exit(0);
});
```

> **进程单例与排队机制**：Blink 依赖不可逆的进程级全局状态，一个进程仅存在一个引擎单例。所有 `screenshot()` 调用在底层按先进先出（FIFO）队列串行处理。如需多核并行渲染，请使用 Node.js `worker_threads` 或多工作进程。

#### 3. 常驻守护进程 (Resident Daemon)

- **方式 A：一键单行调用（免手动管理连接）**
  ```ts
  import { daemon } from '@shotkit/shotium';

  const { image } = await daemon.screenshot({
    file: 'https://example.com',
    viewport: { width: 1280, height: 720 },
    daemon: { name: 'cli-pool', idleTimeoutMs: 300000 },
  });
  ```
- **方式 B：长连接复用（批量截图）**
  ```ts
  import { daemon } from '@shotkit/shotium';

  const client = await daemon.connect({ name: 'batch-worker' });
  try {
    for (const url of ['https://example.com/1', 'https://example.com/2']) {
      const { image } = await client.screenshot({ file: url });
    }
  } finally {
    client.close(); // 仅断开当前连接，后台守护进程继续常驻
  }
  ```
- **方式 C：守护进程状态与停止**
  ```ts
  import { daemon } from '@shotkit/shotium';

  const status = await daemon.status({ name: 'batch-worker' });
  await daemon.stop({ name: 'batch-worker' });
  ```

---

### 实战技巧与避坑指南

#### 1. 渲染内存中的动态 HTML 字符串
Shotium 移除了 V8 且不接受 `data:` URL。如需渲染动态拼接或模板引擎输出的 HTML，推荐写入临时文件后渲染：

```ts
import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { screenshot } from '@shotkit/shotium';

export async function renderHtml(html: string, options = {}) {
  const tempDir = mkdtempSync(join(tmpdir(), 'shotium-'));
  const htmlPath = join(tempDir, 'index.html');
  try {
    writeFileSync(htmlPath, html, 'utf8');
    return await screenshot({
      file: htmlPath,
      allowFileAccess: true, // 允许加载本地子资源
      ...options,
    });
  } finally {
    rmSync(tempDir, { recursive: true, force: true });
  }
}
```

#### 2. 本地文件与子资源权限 (`allowFileAccess: true`)
渲染本地 HTML 时，若页面内引用了相对路径的图片、CSS 样式表或字体（如 `<img src="./avatar.png">` 或 `@font-face`），**必须设置 `allowFileAccess: true`**。出于安全性考虑，Chromium 默认会阻止对本地文件的跨路径访问。

#### 3. 内存 Buffer 与磁盘原子落盘 (`path`)
- **不配置 `path`**：返回 `image: Buffer`，便于直接在内存中处理、通过 HTTP 响应发送或上传 OSS/S3。
- **配置 `path`**：C++ 引擎直接采用临时暂存文件完成原子落盘写入，返回的 `image` 为 `null`，避免了原生堆到 JS 堆的内存冗余拷贝。

#### 4. 超长页面切片 `screenshotTiles`
- **超长尺寸限制**：PNG/JPEG 最大限制 65,535 像素，WebP 最大限制 16,383 像素；Blink 单锚点绘制上限为 32,767 像素。
- **内存受控流式切片**：在 `path` 中使用 `{n}` 占位符（如 `path: 'page-{n}.png'`），每片编码完成后立即写入磁盘，无论页面多长，活跃内存始终维持在单片级别。
- **内存切片**：不配置 `path` 则返回 `tiles: ScreenshotTile[]` 数组，每个元素携带各自的 `image: Buffer`。

#### 5. 缓存管理模块 (`cache`)
```ts
import { cache } from '@shotkit/shotium';

// 1. 获取当前项目或全部缓存目录
const dir = cache.getDir();
const all = cache.getDirs({ target: 'all' });

// 2. 查看已缓存条目
const entries = await cache.getFiles();

// 3. 精准淘汰：按 URL Glob 清理，或限制条目过期时长与整体容量
await cache.clear({ glob: ['https://cdn.example.com/**'] });
await cache.clear({ maxAge: 7 * 86400, maxSize: 100 * 1024 * 1024 });
```

#### 6. 错误排查与指标诊断 (`error.stats`)
发生超时或页面加载异常时，抛出的错误对象上会挂载 `error.stats`，可精准定位问题卡在哪个阶段：

```ts
try {
  await screenshot({ file: 'https://example.com', pageGotoParams: { timeout: 5000 } });
} catch (err: any) {
  console.error('截图失败:', err.message);
  if (err.stats) {
    console.error('阶段耗时:', err.stats.timing);
    console.error(`请求总数: ${err.stats.requests}, 失败子资源数: ${err.stats.failed}`);
  }
}
```

---

</details>

---

## Table of Contents

- [Key Highlights](#key-highlights)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Execution Paradigms](#execution-paradigms)
  - [1. Direct Capture (Zero Configuration)](#1-direct-capture-zero-configuration)
  - [2. Managed Lifecycle for Long-Running Web Services](#2-managed-lifecycle-for-long-running-web-services)
  - [3. Resident Daemon for CLI & Serverless](#3-resident-daemon-for-cli--serverless)
- [Cookbook & Practical Recipes](#cookbook--practical-recipes)
  - [Rendering Dynamic In-Memory HTML Strings](#rendering-dynamic-in-memory-html-strings)
  - [Local Subresources & `allowFileAccess`](#local-subresources--allowfileaccess)
  - [Buffer vs. Atomic Direct Disk Write (`path`)](#buffer-vs-atomic-direct-disk-write-path)
  - [Very Tall Pages & `screenshotTiles`](#very-tall-pages--screenshottiles)
  - [Managing the Persistent HTTP Cache (`cache`)](#managing-the-persistent-http-cache-cache)
  - [Error Handling & Diagnostics (`error.stats`)](#error-handling--diagnostics-errorstats)
- [API Reference](#api-reference)
  - [Module Exports](#module-exports)
  - [`screenshot(options)`](#screenshotoptions)
  - [`screenshotTiles(options)`](#screenshottilesoptions)
  - [`start(options)` & `stop()`](#startoptions--stop)
  - [`releaseMemory(options)`](#releasememoryoptions)
  - [`daemon` Namespace](#daemon-namespace)
  - [`cache` Namespace](#cache-namespace)
  - [Type Definitions](#type-definitions)
- [License](#license)

---

## Key Highlights

- **Chromium 155 Engine Baseline**: Retained Blink DOM/CSS layout, Skia rasterization, and `//net` stack are synchronized against Chromium `155.0.8048.0`. Full support for modern CSS Grid, Flexbox, Container Queries, `@font-face`, SVG, CSS variables, and gradients.
- **Zero External Runtime Dependencies**: Native prebuilt binaries for 6 platform targets (Windows, macOS, Linux on `x64` and `arm64`) are bundled via optional dependencies. No local compiler or Chromium download required.
- **In-Process Node-API Call**: Direct native asynchronous service (`shotium.node`, built by GN). No child browser process, no WebSocket connection, and no zombie process leaks.
- **Deterministic Output**: Grayscale antialiasing with a fixed gamma curve ensures byte-identical rendering across operating systems.
- **Ultra-Low Memory Footprint**: Core engine occupies ~15 MB idle private memory; renders within ~50–70 MB working set.

---

## Installation

```bash
# pnpm
pnpm add @shotkit/shotium

# npm
npm install @shotkit/shotium

# yarn / bun
yarn add @shotkit/shotium
bun add @shotkit/shotium
```

### Module Formats & Compatibility

The package is published as native ESM with full TypeScript declarations:

- **ESM (`import`)**: Supported natively on Node.js 18+.
- **CommonJS (`require`)**: Synchronous `createRequire` or native `require('@shotkit/shotium')` is supported on Node.js 22.12+ and 20.19+.
- **Earlier CommonJS**: Use dynamic `const { screenshot } = await import('@shotkit/shotium')`.

---

## Quick Start

You don't need to manually initialize or manage an engine to take a screenshot. Just call `screenshot()`:

```ts
import { writeFileSync } from 'node:fs';
import { screenshot } from '@shotkit/shotium';

// Capture an online page directly to an in-memory buffer
const { image, stats } = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'png',
});

console.log(`Rendered in ${stats.timing.render.toFixed(1)}ms, total: ${stats.timing.total.toFixed(1)}ms`);
writeFileSync('example.png', image!);
```

---

## Execution Paradigms

Depending on your application architecture, choose the paradigm that best matches your lifecycle:

| Paradigm | Best For | Characteristics |
|---|---|---|
| **[Direct Capture](#1-direct-capture-zero-configuration)** | Scripts, one-off tasks, prototyping | Zero setup; engine boots automatically on first call. |
| **[Managed Web Service](#2-managed-lifecycle-for-long-running-web-services)** | Express, Fastify, NestJS, Koa | Warm start at boot, shared disk cache, burst memory reclamation, clean shutdown. |
| **[Resident Daemon](#3-resident-daemon-for-cli--serverless)** | CLI tools, CI steps, Serverless functions | Pre-warmed background process; connect in **~2 ms** without cold engine start. |

---

### 1. Direct Capture (Zero Configuration)

Calling `screenshot()` or `screenshotTiles()` without calling `start()` automatically spins up the shared process singleton using sensible defaults (a persistent project cache in `~/.shotium/cache`).

```ts
import { screenshot } from '@shotkit/shotium';

const { image } = await screenshot({
  file: 'https://example.com',
  fullPage: true,
  type: 'webp',
  quality: 85,
});
```

---

### 2. Managed Lifecycle for Long-Running Web Services

In persistent backend applications (e.g., Express or Fastify), initialize the engine during server startup, reuse it across requests, proactively trim memory after bursts, and shut down cleanly on `SIGTERM`.

```ts
import express from 'express';
import shotium, { screenshot } from '@shotkit/shotium';

const app = express();

// 1. Initialize engine at server boot (idempotent; sets immutable process config)
const { cacheDir, cacheActive } = shotium.start({
  cacheMaxBytes: 512 * 1024 * 1024, // 512 MB HTTP disk cache
});
console.log(`Shotium ready. Cache dir: ${cacheDir} (active: ${cacheActive})`);

// 2. Render route
app.get('/render', async (req, res, next) => {
  try {
    const targetUrl = req.query.url as string;

    const { image, stats } = await screenshot({
      file: targetUrl,
      viewport: { width: 1280, height: 720 },
      type: 'png',
    });

    res.setHeader('Content-Type', 'image/png');
    res.setHeader('X-Render-Time-Ms', stats.timing.render.toFixed(1));
    res.send(image);
  } catch (err) {
    next(err);
  }
});

// 3. Periodic or post-burst memory reclamation
//    Frees Blink heap, Skia font/image caches, and PartitionAlloc free lists.
//    releaseWorkingSet: true requests OS-level physical memory trimming.
setInterval(() => {
  shotium.releaseMemory({ releaseWorkingSet: true });
}, 10 * 60 * 1000);

// 4. Graceful shutdown
process.on('SIGTERM', async () => {
  console.log('Draining requests and stopping Shotium...');
  await shotium.stop();
  process.exit(0);
});
```

> [!NOTE]
> **Process Singleton & Queueing**:
> Blink's C++ state is a process-wide singleton that cannot be un-initialized. All `screenshot()` calls within a process share this instance and are processed sequentially through an internal FIFO queue. For parallel rendering across multiple CPU cores, scale horizontally using Node.js `worker_threads` or cluster worker processes.

---

### 3. Resident Daemon for CLI & Serverless

For short-lived processes (such as CLI tools, CI actions, or serverless workers), spinning up Blink on every execution introduces tens of milliseconds of process startup cost. The **resident daemon** runs the engine in a background process, pre-warms it with an empty page, and listens on a local IPC socket (Named Pipe on Windows, Unix domain socket on POSIX).

#### Option A: One-Liner (Auto-Connect & Auto-Spawn)

Use `daemon.screenshot()` or `daemon.screenshotTiles()` directly. It connects to an existing daemon (or starts one in the background if absent), executes the capture, and cleanly closes the connection:

```ts
import { daemon } from '@shotkit/shotium';

const { image, stats } = await daemon.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  daemon: {
    name: 'cli-pool',       // Optional: isolates daemon instances by name
    idleTimeoutMs: 300000, // Exits automatically after 5 minutes of inactivity
  },
});
```

#### Option B: Persistent Client Connection (Batch Requests)

When executing multiple captures in a loop or pipeline, keep the client connection open:

```ts
import { daemon } from '@shotkit/shotium';

// Connect or spawn
const client = await daemon.connect({
  name: 'batch-worker',
  idleTimeoutMs: 60000,
});

try {
  for (const url of ['https://example.com/1', 'https://example.com/2']) {
    const { image } = await client.screenshot({ file: url });
    // Process image...
  }
} finally {
  client.close(); // Close IPC connection; daemon stays alive in background
}
```

#### Option C: Daemon Inspection & Teardown

```ts
import { daemon } from '@shotkit/shotium';

// Inspect background daemon status
const status = await daemon.status({ name: 'batch-worker' });
if (status.running) {
  console.log(`Daemon PID: ${status.pid}, Served requests: ${status.served}, Uptime: ${status.uptimeMs}ms`);
}

// Stop daemon process explicitly
await daemon.stop({ name: 'batch-worker' });
```

---

## Cookbook & Practical Recipes

### Rendering Dynamic In-Memory HTML Strings

Shotium intentionally does not embed V8 and does not support `data:` URLs as the main document. To render dynamically generated HTML (e.g. from React/Vue SSR, Mustache, or template literals), write it to a temporary file:

```ts
import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { screenshot } from '@shotkit/shotium';

export async function renderHtml(html: string, options = {}) {
  // 1. Create a secure temporary directory
  const tempDir = mkdtempSync(join(tmpdir(), 'shotium-render-'));
  const htmlFile = join(tempDir, 'index.html');

  try {
    // 2. Write HTML markup to disk
    writeFileSync(htmlFile, html, 'utf8');

    // 3. Capture local file (allowFileAccess: true if referencing local images/fonts)
    return await screenshot({
      file: htmlFile,
      allowFileAccess: true,
      ...options,
    });
  } finally {
    // 4. Clean up temporary directory
    rmSync(tempDir, { recursive: true, force: true });
  }
}

// Usage:
const { image } = await renderHtml(`
  <!DOCTYPE html>
  <html>
    <head>
      <style>
        body { font-family: system-ui, sans-serif; padding: 40px; background: #f0f4f8; }
        .card { background: white; padding: 24px; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.08); }
        h1 { color: #1a365d; margin-top: 0; }
      </style>
    </head>
    <body>
      <div class="card">
        <h1>Invoice #2048</h1>
        <p>Billed to: Acme Corporation</p>
      </div>
    </body>
  </html>
`, { viewport: { width: 600, height: 400 } });
```

---

### Local Subresources & `allowFileAccess`

> [!IMPORTANT]
> **Enabling Local Resource Access**:
> For security, Chromium disables access to local files by default. If your HTML file references local subresources such as `<link rel="stylesheet" href="./style.css">`, `<img src="./avatar.png">`, or `@font-face { src: url("./font.woff2"); }`, you **must** set `allowFileAccess: true`. Otherwise, subresource requests will be blocked and will show as `failed` in `stats`.

```ts
import { screenshot } from '@shotkit/shotium';

const { image, stats } = await screenshot({
  file: './templates/report.html',
  allowFileAccess: true, // Required to load local style.css and images
  viewport: { width: 1024, height: 768 },
});

if (stats.failed > 0) {
  console.warn(`Warning: ${stats.failed} subresource(s) failed to load.`);
}
```

---

### Buffer vs. Atomic Direct Disk Write (`path`)

Shotium offers two output strategies:

1. **In-Memory `Buffer` (Omit `path`)**:
   The encoded image is returned as `image: Buffer`. Ideal when serving HTTP responses, transforming with Sharp, or piping to cloud storage.
   ```ts
   const { image } = await screenshot({ file: 'https://example.com' });
   // image is Buffer
   ```

2. **Zero-Copy Atomic Disk Write (Specify `path`)**:
   When `path` is specified, the C++ engine writes the encoded bytes directly to disk using a staging file and atomic rename (preserving file permissions). `image` returns `null` to eliminate redundant memory copying between native and JS heaps.
   ```ts
   const { image } = await screenshot({
     file: 'https://example.com',
     path: './output.png', // image is null; file written directly
   });
   ```

---

### Very Tall Pages & `screenshotTiles`

While `fullPage: true` captures an entire document into a single image, image formats have hard dimension limits:
- **PNG & JPEG**: Maximum 65,535 pixels per dimension.
- **WebP**: Maximum 16,383 pixels per dimension.
- **Blink internal paint limit**: Blink stops painting past 32,767 CSS pixels from a single scroll anchor.

`screenshotTiles()` renders a long document in a single load and cut pass, slicing it into horizontal strips of at most `tile.height` CSS pixels (up to 32,000 px).

#### Bounded-Memory Mode with `{n}` Pattern:
Specifying `{n}` in `path` streams each tile directly to disk as soon as it is encoded, keeping active bitmap memory bounded to a single tile regardless of how long the page is:

```ts
import { screenshotTiles } from '@shotkit/shotium';

const { tiles, stats } = await screenshotTiles({
  file: './long-article.html',
  fullPage: true,
  tile: { height: 8000 },
  path: 'article-tile-{n}.png', // Writes article-tile-1.png, article-tile-2.png, ...
});

for (const tile of tiles) {
  console.log(`Saved tile at y=${tile.y}, height=${tile.height} to ${tile.path}`);
}
```

#### In-Memory Tiles Mode:
Omitting `path` returns an array of `ScreenshotTile` objects, each containing its own `image: Buffer`:

```ts
const { tiles } = await screenshotTiles({
  file: 'https://example.com/feed',
  fullPage: true,
  tile: { height: 5000 },
});

for (const { image, y, height } of tiles) {
  console.log(`Tile at y=${y} (${height}px), buffer size: ${image!.length} bytes`);
}
```

---

### Managing the Persistent HTTP Cache (`cache`)

Shotium integrates Chromium's `//net` disk cache, located by default under `~/.shotium/cache/<project-hash>`. The cache is shared across processes and survives engine restarts.

```ts
import { cache } from '@shotkit/shotium';

// 1. Get cache directory path
const currentCacheDir = cache.getDir();
const allCacheDirs = cache.getDirs({ target: 'all' });

// 2. Query cached resource metadata
const entries = await cache.getFiles();
for (const entry of entries.slice(0, 5)) {
  console.log(`${entry.url} (${(entry.bytes / 1024).toFixed(1)} KB, last used: ${new Date(entry.lastUsedMs).toISOString()})`);
}

// 3. Granular eviction:
// Evict entries matching a URL glob pattern
await cache.clear({
  glob: ['https://cdn.example.com/**'],
});

// Evict entries unused for over 7 days, or trim total directory size to 100 MB via LRU
await cache.clear({
  maxAge: 7 * 24 * 3600,       // In seconds
  maxSize: 100 * 1024 * 1024,  // In bytes
});
```

---

### Error Handling & Diagnostics (`error.stats`)

When a screenshot fails (due to a navigation timeout, invalid selector, or network error), Shotium rejects the Promise with an `Error`. Crucially, **partial telemetry is attached to `error.stats`**, allowing you to inspect which phase stalled or which subresources failed:

```ts
import { screenshot } from '@shotkit/shotium';

try {
  await screenshot({
    file: 'https://example.com/slow-page',
    selector: '#target-content',
    pageGotoParams: { timeout: 5000 },
  });
} catch (err: any) {
  console.error('Screenshot failed:', err.message);

  if (err.stats) {
    const { timing, requests, failed, httpStatus } = err.stats;
    console.error(`HTTP Status: ${httpStatus}`);
    console.error(`Requests: ${requests}, Failed: ${failed}`);
    console.error(`Fetch: ${timing.fetch}ms, Wait: ${timing.wait}ms, Total: ${timing.total}ms`);
  }
}
```

---

## API Reference

### Module Exports

```ts
import shotium, {
  screenshot,
  screenshotTiles,
  start,
  stop,
  status,
  releaseMemory,
  daemon,
  cache,
  runtime,
  Runtime,
} from '@shotkit/shotium';
```

---

### `screenshot(options)`

Captures a single screenshot.

- **`options`**: [`ScreenshotOptions`](#screenshotoptions-1)
- **Returns**: `Promise<`[`ScreenshotResult`](#screenshotresult)`>`

```ts
const { image, stats } = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'png',
});
```

---

### `screenshotTiles(options)`

Renders and slices a document into horizontal strips.

- **`options`**: [`ScreenshotTilesOptions`](#screenshottilesoptions-1)
- **Returns**: `Promise<`[`ScreenshotTilesResult`](#screenshottilesresult)`>`

---

### `start(options)` & `stop()`

Explicit lifecycle controls for the in-process engine.

- **`start(options?: StartOptions): StartResult`**: Starts the engine or adopts the running one. Throws if invoked with options that conflict with the running engine.
- **`status(): StartResult`**: Reports the current status, active cache directory, and native engine path.
- **`stop(): Promise<void>`**: Drains pending captures, releases Blink memory, and stands down the engine.

---

### `releaseMemory(options)`

- **`releaseMemory(options?: ReleaseMemoryOptions): void`**: Hands back non-essential memory (Blink Oilpan heap, Skia caches, PartitionAlloc free lists). When `releaseWorkingSet: true` is passed, also instructs the OS to reclaim physical memory pages.

---

### `daemon` Namespace

Client for the resident daemon:

- **`daemon.screenshot(options: ScreenshotOptions & { daemon?: DaemonOptions }): Promise<ScreenshotResult>`**: One-shot capture via daemon.
- **`daemon.screenshotTiles(options: ScreenshotTilesOptions & { daemon?: DaemonOptions }): Promise<ScreenshotTilesResult>`**: One-shot tiles capture via daemon.
- **`daemon.connect(options?: DaemonOptions): Promise<DaemonClient>`**: Opens a multiplexed IPC connection.
- **`daemon.start(options?: DaemonOptions): Promise<DaemonStatus & { spawned: boolean }>`**: Starts a background daemon if not already running.
- **`daemon.status(options?: DaemonOptions): Promise<Partial<DaemonStatus> & { running: boolean, endpoint: string }>`**: Queries daemon health and metrics.
- **`daemon.stop(options?: DaemonOptions): Promise<{ stopped: boolean, endpoint: string }>`**: Shuts down the daemon.

---

### `cache` Namespace

HTTP disk cache management:

- **`cache.getDir(options?: CacheTarget): string`**: Returns the absolute cache directory path.
- **`cache.getDirs(options?: CacheTarget): string[]`**: Returns all matching cache directory paths.
- **`cache.getFiles(options?: CacheTarget): Promise<CacheEntry[]>`**: Lists metadata for cached URLs.
- **`cache.clear(options?: CacheClearOptions): Promise<CacheClearResult[]>`**: Evicts cache entries by glob, age, or size.

---

### Type Definitions

#### `ScreenshotOptions`

```ts
export interface ScreenshotOptions {
  /** Target URL (http/https/file protocol) or local file path. Dynamic HTML must be saved to a file first. */
  file: string;

  /** Image encoder format. Default 'png'. */
  type?: 'png' | 'jpeg' | 'webp';

  /** Output compression quality (1-100; for 'jpeg' and 'webp' only). Default 90. */
  quality?: number;

  /** Layout viewport dimensions in CSS pixels. Default 1280x720. */
  viewport?: { width?: number; height?: number };

  /** Capture entire scrollable document height. Mutually exclusive with selector and clip. */
  fullPage?: boolean;

  /** Capture bounding box of element matching CSS selector. Mutually exclusive with fullPage and clip. */
  selector?: string;

  /** Crop rectangle in CSS pixels. Mutually exclusive with fullPage and selector. */
  clip?: { x: number; y: number; width: number; height: number };

  /** Device pixel ratio (DPR, 0.01 - 8.0). Default 1. */
  scale?: number;

  /** Preserve transparent background instead of painting white canvas (png and webp only). Default false. */
  omitBackground?: boolean;

  /** Output file destination. When set, writes directly to disk and returned image is null. */
  path?: string;

  /** Page navigation and wait conditions. */
  pageGotoParams?: {
    /** Load timeout in milliseconds. Default 30000. */
    timeout?: number;
    /**
     * 'load': waits for DOM parsed, load event, and subresources complete (default).
     * 'networkidle': additionally waits for a 500ms window with 0 in-flight requests.
     */
    waitUntil?: 'load' | 'networkidle';
  };

  /** Allow document to read file:// subresources (local images, fonts, styles). Default false. */
  allowFileAccess?: boolean;

  /** HTTP disk cache policy. Default 'default'. */
  cache?: 'default' | 'reload' | 'no-store' | 'only-if-cached';

  /** Extra request headers sent strictly to same-origin targets. */
  headers?: Record<string, string>;
}
```

#### `ScreenshotResult`

```ts
export interface ScreenshotResult {
  /** Encoded image buffer, or null if path was specified. */
  image: Buffer | null;
  /** Detailed timing and network telemetry. */
  stats: CaptureStats;
}
```

#### `ScreenshotTilesOptions`

```ts
export interface ScreenshotTilesOptions extends ScreenshotOptions {
  /** Tile slicing configuration. */
  tile: {
    /** Maximum CSS pixels per tile (at most 32000). */
    height: number;
  };
}
```

#### `ScreenshotTilesResult`

```ts
export interface ScreenshotTilesResult {
  /** Sliced tiles ordered top-to-bottom. */
  tiles: Array<{
    image: Buffer | null;
    x: number;
    y: number;
    width: number;
    height: number;
    path?: string;
  }>;
  stats: CaptureStats;
}
```

#### `CaptureStats` & `CaptureTiming`

```ts
export interface CaptureStats {
  requests: number;   // Total network requests (main document + subresources)
  fromCache: number;  // Subresources served from HTTP disk cache
  failed: number;     // Failed subresource requests
  bytes: number;      // Total decoded body bytes
  httpStatus: number; // Main document HTTP status code (0 for local files)
  finalUrl: string;   // Final URL after redirects
  timing: CaptureTiming;
}

export interface CaptureTiming {
  fetch: number;      // Main document retrieval (DNS, TCP, TLS, round-trip)
  render: number;     // HTML parse, subresources, style, layout, paint
  setup: number;      // Page/Frame creation & synchronous install
  wait: number;       // Wait for parsing and subresources
  lifecycle: number;  // Capture selection, style/layout advancement
  paint: number;      // PaintRecord extraction
  raster: number;     // Skia rasterization replay
  encode: number;     // PNG/JPEG/WebP encoding
  total: number;      // Wall-clock duration
}
```

#### `StartOptions` & `StartResult`

```ts
export interface StartOptions {
  /** Root directory for HTTP disk cache. Pass null to disable caching. */
  cacheDir?: string | null;
  /** Maximum cache size in bytes. Default 256 MB. */
  cacheMaxBytes?: number;
  /** Custom User-Agent header. */
  userAgent?: string;
  /** Path to shotium_data.pak and shotium_strings.pak (source checkouts only). */
  resourceDir?: string;
}

export interface StartResult {
  running: boolean;
  cacheDir: string | null;
  enginePath: string | null;
  cacheActive: boolean;
}
```

#### `DaemonOptions` & `DaemonStatus`

```ts
export interface DaemonOptions extends StartOptions {
  /** Instance name identifier for isolation. */
  name?: string;
  /** Inactivity timeout in ms before daemon exits automatically (default 300000; 0 = never). */
  idleTimeoutMs?: number;
  /** Render blank page on startup to prewarm Blink and font caches. Default true. */
  prewarm?: boolean;
  /** Diagnostic log file destination for spawned daemon. */
  logFile?: string;
  /** Timeout in ms to wait for daemon to bind. Default 20000. */
  startTimeoutMs?: number;
}

export interface DaemonStatus {
  pid: number;
  endpoint: string;
  cacheDir: string | null;
  warm: boolean;
  uptimeMs: number;
  connections: number;
  inFlight: number;
  served: number;
  idleTimeoutMs: number;
  version: string;
  protocolVersion: number;
  capabilities: ('screenshot' | 'tiles')[];
}
```

#### `CacheClearOptions` & `CacheClearResult`

```ts
export interface CacheClearOptions {
  /** 'current' (default), 'all', or specific project hash / directory path. */
  target?: 'current' | 'all' | (string & {});
  /** Glob patterns matched against entry URLs (e.g. ['https://example.com/**']). */
  glob?: string[];
  /** Evict entries unused for this many seconds (0 = no limit). */
  maxAge?: number;
  /** Evict LRU entries until directory size is at or below this byte count (0 = no limit). */
  maxSize?: number;
}

export interface CacheClearResult {
  removed: number;      // Number of entries removed (-1 if entire dir removed)
  bytesBefore: number;
  bytesAfter: number;
  dir: string;
}
```

---

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](https://github.com/sj817/shotium/blob/main/LICENSE).

### Node native entry

The Node-API addon is built by GN with the engine core: `pnpm build:engine
--target shot_node` produces `out/Shot/shotium.node`. Screenshot requests and
statistics cross the Node boundary as objects. Rendering stays on the engine
thread and completes Promises through Node-API; captures do not occupy libuv
workers while waiting for rendering. Public `stop()`/`start()` behavior is unchanged.

The npm platform packages contain the self-contained `.node`, CLI and resource
packs. The independent C ABI library remains available in GitHub Release
archives; Node does not load it. Building the addon requires the full source
checkout and the pinned SDK prepared by `scripts/node-sdk.ts`, not node-gyp.
