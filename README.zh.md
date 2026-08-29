# shotium

> 基于精简 Chromium Blink 内核的高性能 HTML/CSS 静态截图引擎。

[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](LICENSE)
[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg)](https://www.npmjs.com/package/@shotkit/shotium)
[![Download](https://img.shields.io/badge/npm%20download-19.4%20MB%20(win32--x64)-green.svg)](https://www.npmjs.com/package/@shotkit/shotium)

[English](README.md) · **简体中文**

---

## 快速上手

### 1. 安装

```bash
# npm
npm install @shotkit/shotium

# pnpm
pnpm add @shotkit/shotium

# yarn
yarn add @shotkit/shotium

# bun
bun add @shotkit/shotium
```

预编译二进制已打包为可选依赖分发（覆盖 Windows、macOS、Linux 的 x64 与 arm64 架构）。包管理器会自动匹配下载对应平台包，无需本地编译环境。

### 2. 基础示例

```ts
import shotium, { screenshot } from '@shotkit/shotium';

// 1. 启动引擎
shotium.start();

// 2. 渲染远程 URL、本地 HTML 或内联 HTML 字符串 (data:text/html)
const { image, stats } = await screenshot({
  file: 'data:text/html,<h1 style="color: #0969da; font-family: sans-serif;">Hello Shotium</h1>',
  viewport: { width: 800, height: 400 },
});

console.log(`渲染耗时: ${stats.timing.render}ms, 总耗时: ${stats.timing.total}ms`);

// 3. 停止引擎
await shotium.stop();
```

---

## 目录

- [概述与核心特性](#概述与核心特性)
- [运行模式选型建议](#运行模式选型建议)
- [技术范围与设计边界](#技术范围与设计边界)
- [运行模式详解](#运行模式详解)
  - [1. 进程内引擎 (In-Process)](#1-进程内引擎-in-process-engine)
  - [2. 常驻守护进程模式 (Daemon)](#2-常驻守护进程模式-daemon)
  - [3. 独立命令行工具 (CLI)](#3-独立命令行工具-cli)
- [API 参考](#api-参考)
  - [`ScreenshotOptions` & `ScreenshotResult`](#screenshotoptions)
  - [`StartOptions` & `StartResult`](#startoptions)
  - [`CaptureStats` 统计与耗时分析](#capturestats)
  - [`daemon` 模块与状态监控](#daemon-模块)
  - [`cache` 缓存管理模块](#cache-模块)
- [性能基准测试](#性能基准测试)
- [技术架构](#技术架构)
- [环境变量](#环境变量)
- [C ABI / FFI 跨语言集成](#c-abi--ffi-跨语言集成)
- [源码构建指南](#源码构建指南)
- [许可证](#许可证)

---

## 概述与核心特性

**shotium** 是一款专为 HTML/CSS 静态渲染截图设计的高性能轻量级引擎。

通过深度剥离 Chromium 的多进程外壳与冗余组件，仅保留核心排版与渲染管线：
- **Blink** 排版引擎
- **Skia** CPU 内存光栅化
- 字体排印与图像编解码器
- Chromium **`//net`** 网络子系统

### 核心优势

- **彻底移除 V8 与浏览器外壳**：无 `//content` 框架、无 GPU 进程、无合成器、无 DevTools 调试协议。
- **极致轻量与极速响应**：引擎核心体积约 41 MB，冷启动 < 350 ms，单图视口渲染 ~47 ms，内存开销显著低于 Headless Chrome。
- **零 IPC 进程内渲染**：基于 Node-API 直接在宿主进程内完成渲染与编码，单张耗时低至 **31 ms**。
- **两种交付形态**：
  - **Node.js 原生扩展**：通过 Node-API 在当前进程内直接加载，零子进程开销（win32-x64 仅 19.4 MB）。
  - **独立可执行文件**：单文件 CLI 工具（压缩包仅 15.3 MB），可前往 [Releases 页面](https://github.com/sj817/shotium/releases) 下载。

---

## 运行模式选型建议

| 场景需求 | 推荐模式 | 核心优势 |
|---|---|---|
| **常驻 Web / API 服务**（如 Express、Fastify、NestJS） | **进程内引擎 (In-Process)** | 零 IPC 通信开销、零进程拉起成本，延迟最低（~31 ms/张）。 |
| **CLI 命令行工具 / CI 流水线 / Serverless 任务** | **守护进程模式 (Daemon)** | 跨命令复用预热引擎，客户端连接仅需 **2.3 ms**，避免冷启动开销。 |
| **非 Node.js 环境 / Shell 脚本自动化** | **独立命令行工具 (CLI)** | 单文件可执行程序，支持 `--stdin` 管道输入，无任何外部依赖。 |

---

## 技术范围与设计边界

shotium 专为服务端静态 HTML/CSS 渲染设计（如社交分享卡片、电子发票/小票、营销海报、数据报表导出及 SSR 页面快照）。

### 支持特性

- **现代 CSS 规范完整支持**：基于 Blink 核心，支持 Flexbox、CSS Grid、Web 字体、SVG、CSS 变量及复杂选择器。
- **多种输入源形式**：支持远程 URL（`http://`, `https://`）、本地文件路径（绝对/相对路径及 `file://`）以及**内联 HTML 字符串**（`data:text/html,...`）。
- **原生多图像格式编码**：原生支持输出 PNG、JPEG、WebP 格式，支持画质压缩比与 Alpha 透明通道。
- **内置 Chromium 网络栈**：直连 Chromium `//net`（支持 HTTPS、HTTP/2、Brotli、重定向、磁盘缓存及 Cookie 管理）。
- **同进程高性能渲染**：基于 Node-API 与 C ABI 在当前进程内直接渲染，无跨进程内存拷贝。

### 明确的设计边界与非目标

- **不执行任何 JavaScript**：完全移除 V8 引擎，忽略 `<script>` 标签与客户端 Hydration。仅支持服务端渲染（SSR）或静态 HTML。
- **不包含多进程沙箱**：无浏览器安全沙箱，SSRF 防范与目标 URL 校验须由上层业务处理；默认禁止加载本地 `file://` 子资源（可配置 `allowFileAccess: true` 开启）。
- **确定性灰度抗锯齿**：字体光栅化采用固定伽马曲线灰度抗锯齿，不读取宿主系统的 ClearType 次像素设置，保证跨系统与跨进程输出像素级一致。

---

## 运行模式详解

### 1. 进程内引擎 (In-Process Engine)

引擎通过 Node-API 加载 [`shot/shot_api.h`](shot/shot_api.h) 定义的 C ABI，直接运行在当前 Node.js 进程中。`screenshot()` 返回由 Blink 编码完成的图像 Buffer，单张耗时约 **31 ms**。

```ts
import shotium, { screenshot } from '@shotkit/shotium';

// 启动引擎并获取缓存状态
const { cacheDir, cacheActive } = shotium.start();

// 1. 渲染远程 URL
const res1 = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'webp',
  quality: 85,
});

// 2. 渲染动态拼装的内联 HTML 字符串（无需落盘临时文件）
const html = `<div style="padding: 24px; background: #f6f8fa;"><h2>Invoice #1024</h2></div>`;
const res2 = await screenshot({
  file: `data:text/html;charset=utf-8,${encodeURIComponent(html)}`,
  viewport: { width: 600, height: 300 },
});

// 3. 内存释放策略：
// - releaseMemory()：清理 Blink 堆、Skia 缓存与 PartitionAlloc 空闲链表（轻量无感）
// - releaseWorkingSet: true：进一步请求 OS 回收物理工作集内存（用于长时间闲置前）
shotium.releaseMemory({ releaseWorkingSet: true });

// 4. 停止引擎
await shotium.stop();
```

#### 生命周期与运行机制

- **单例模型**：
  - Blink 内核使用全局静态状态，因此单个 Node.js 进程只初始化一个引擎实例。
  - 多次调用 `new Runtime()` 或使用顶层 API 将自动复用已创建的引擎。
- **启停机制**：
  - `stop()` 会排空执行队列、交还工作集内存并将状态置为 `running: false`。
  - 后续再次调用 `start()` 可无缝重新激活引擎，并保留热缓存。
- **配置一致性校验**：
  - 引擎配置参数在首次初始化时固定。
  - 若后续调用 `start()` 传入了与当前实例冲突的配置，将抛出异常以避免配置静默失效。
- **排队与并行化**：
  - 单个引擎实例内部为串行渲染。
  - 如需高并发吞吐，请通过多进程架构（如 Cluster / Worker 进程）横向扩展。
- **状态感知**：
  - `start()` 与 `status()` 返回 `{ running, cacheDir, cacheActive }`。
  - 若 `cacheDir` 无法打开，`cacheActive` 将为 `false`，此时引擎在无缓存模式下正常运作。

---

### 2. 常驻守护进程模式 (Daemon)

适用于 CLI 工具、CI/CD 构建流水线或对单次调用冷启动延迟敏感的短生命周期任务。

守护进程在独立后台进程中常驻运行，并通过本地 IPC（Windows 命名管道 / POSIX Unix 域套接字）提供服务。启动时自动预渲染空白页完成预热，后续客户端建连握手耗时仅约 **2.3 ms**。

```ts
import { daemon } from '@shotkit/shotium';

// 连接现有守护进程（若未启动则自动在后台拉起）
const client = await daemon.connect();

// 截图请求
const { image, stats } = await client.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

// 在客户端连接上管理守护进程状态或释放内存
const clientStatus = await client.status();
await client.releaseMemory({ releaseWorkingSet: false });

client.close();

// 管理操作（可选）
const daemonInfo = await daemon.status();
console.log(`Daemon PID: ${daemonInfo.pid}, Uptime: ${daemonInfo.uptimeMs}ms, Served: ${daemonInfo.served}`);

// 停止守护进程
await daemon.stop();
```

- **请求多路复用**：单条 IPC 连接支持同时提交多个请求，每条消息包含唯一 `id`，服务端按完成顺序回传结果。
- **多实例隔离**：单个守护进程内部同样为串行渲染；若需多任务并行处理，可通过指定不同 `name` 启动多个守护进程实例。

---

### 3. 独立命令行工具 (CLI)

在无 Node.js 运行时的主机环境中，可直接运行从 [GitHub Releases](https://github.com/sj817/shotium/releases) 下载的独立二进制文件：

```bash
# 1. 截取远程 URL 视口并输出为 PNG 文件
shotium https://example.com --width 1280 --height 720 -o output.png

# 2. 截取本地 HTML 全长页面并输出为 WebP
shotium --file page.html --full-page --type webp --quality 85 -o output.webp

# 3. 标准输入管道截图 (Pipeline / Stdin)
cat template.html | shotium --stdin --width 800 --height 600 -o banner.png

# 4. 常驻服务模式：通过 stdin 接收长度前缀的 JSON 请求并输出结果
shotium --serve --cache-dir /var/tmp/shotium-cache
```

---

## API 参考

### `ScreenshotOptions`

```ts
interface ScreenshotOptions {
  /** 目标 URL (http/https/file/data) 或本地文件路径 */
  file: string;

  /** 输出图像格式 (默认: 'png') */
  type?: 'png' | 'jpeg' | 'webp';

  /** 视口尺寸 (默认: 1280x720) */
  viewport?: { width?: number; height?: number };

  /** 是否截取完整长图（滚动整个文档） */
  fullPage?: boolean;

  /** 截取指定 CSS 选择器匹配元素的包围盒 (通过 Document::querySelector 解析) */
  selector?: string;

  /** 截取指定矩形坐标区域 */
  clip?: { x: number; y: number; width: number; height: number };

  /** 压缩质量: 1-100 (仅适用于 jpeg 与 webp，默认: 90) */
  quality?: number;

  /** 设备像素比 (Device Scale Factor): 0.01 - 8.0 (默认: 1.0) */
  scale?: number;

  /** 是否保留背景透明度 (仅适用于 png 与 webp；jpeg 不支持透明通道) */
  omitBackground?: boolean;

  /** 输出文件路径；若指定则引擎在 C++ 层直接落盘，screenshot 返回的 image 为 null */
  path?: string;

  /** 页面导航与加载控制 */
  pageGotoParams?: {
    /** 超时时间 (毫秒)，默认: 30000 */
    timeout?: number;
    /**
     * load: 等待 DOM 解析及基础资源加载完毕 (默认)
     * networkidle: 额外等待 500ms 内无任何在途网络请求 (适用于后置加载 WebFont/CSS 的页面)
     */
    waitUntil?: 'load' | 'networkidle';
  };

  /** 是否允许加载本地 file:// 子资源 (默认: false) */
  allowFileAccess?: boolean;

  /** HTTP 缓存策略 (默认: 'default') */
  cache?: 'default' | 'reload' | 'no-store' | 'only-if-cached';

  /** 附加请求头，仅向主文档同源地址发送 */
  headers?: Record<string, string>;
}
```

> **注意**：`fullPage`、`selector` 与 `clip` 为互斥参数，同时指定多个将抛出参数校验异常。

#### `ScreenshotResult` 返回结构

```ts
interface ScreenshotResult {
  /** 图像二进制 Buffer；若调用时传了 path 参数则为 null (已直接落盘) */
  image: Buffer | null;
  /** 详细耗时与网络请求统计 */
  stats: CaptureStats;
}
```

#### 参数补充说明

- **`file` 输入支持**：
  - 远程地址：`https://example.com`
  - 本地路径：`./template.html` 或 `/absolute/path/index.html` 或 `file:///...`
  - 内联 HTML 字符串：`data:text/html;charset=utf-8,<h1>Hello</h1>`
- **`cache` 策略选项**（与 Web Fetch API 一致，对主文档及子资源均生效）：
  - `default`：标准 HTTP 缓存策略。
  - `reload`：忽略现有缓存，向服务端请求最新资源并更新缓存。
  - `no-store`：不读取也不写入缓存。
  - `only-if-cached`：仅从本地缓存读取；若未命中则直接报错，不发起网络请求。
- **`headers` 请求头作用域**：
  - 严格遵循同源策略（Same-Origin）。
  - 传入的 `Authorization`、`Cookie` 等认证标头仅发送至目标站点，不会转发至跨域的外部样式表或字体等静态资源。

---

### `StartOptions`

```ts
interface StartOptions {
  /**
   * HTTP 磁盘缓存目录。默认使用 ~/.shotium/cache 下的项目隔离目录；
   * 传入 null 则禁用磁盘缓存。
   */
  cacheDir?: string | null;

  /** 缓存目录容量上限，单位字节 (默认: 256 MB) */
  cacheMaxBytes?: number;

  /** 自定义 User-Agent 字符串 */
  userAgent?: string;

  /** 资源包（shotium_data.pak、shotium_strings.pak）所在目录 */
  resourceDir?: string;
}
```

#### `StartResult` 返回结构

```ts
interface StartResult {
  /** 当前 Runtime 实例是否处于运行状态 */
  running: boolean;
  /** 当前正在使用的缓存目录路径 (禁用缓存时为 null) */
  cacheDir: string | null;
  /** 缓存目录是否成功打开并生效 */
  cacheActive: boolean;
}
```

---

### `CaptureStats`

每次截图调用均返回详细的执行耗时与网络请求指标：

```ts
interface CaptureStats {
  requests: number;     // 页面请求的资源总数（包含主文档）
  fromCache: number;    // 由 HTTP 磁盘缓存响应的资源数量
  failed: number;       // 请求失败的资源数量
  bytes: number;        // 解码后的正文字节总数（非传输体积）
  httpStatus: number;   // 主文档 HTTP 状态码（本地 file: / data: 协议为 0）
  finalUrl: string;     // 经历重定向后的最终 URL
  timing: {
    fetch: number;      // 主文档获取耗时（DNS 解析、TCP 握手、TLS 协商及网络往返）
    render: number;     // 渲染耗时（HTML 解析、子资源加载、样式计算、布局及绘制）
    encode: number;     // 图像编码耗时
    total: number;      // 总执行耗时
  };
}
```

#### 耗时与指标分析

- **网络耗时构成**：对于冷请求 `https:` 地址，`timing.fetch` 通常占据总耗时的大部分；缓存命中后主文档获取耗时可降至毫秒级：

| 场景 | `fetch` 获取耗时 | `render` 渲染耗时 | `total` 总耗时 |
|---|---|---|---|
| **本地文件 / 内联 HTML** (`file:` / `data:`) | 0.2 ms | 20 ms | 25 ms |
| **HTTPS（首次冷请求）** | 321.1 ms | 16 ms | 350 ms |
| **HTTPS（缓存命中）** | 0.7 ms | 18 ms | 31 ms |

- **`fromCache` 说明**：表示正文内容直接从磁盘缓存中读取。若缓存条目过期并触发协商缓存验证（304 Not Modified），仍包含一次网络往返耗时（节省传输体积而非往返延迟）。
- **异常诊断 (`error.stats`)**：截图失败或超时时，异常对象附带 `error.stats` 属性，便于排查子资源加载或网络异常。

---

### `daemon` 模块

管理常驻后台的守护进程实例与 IPC 连接：

```ts
import { daemon } from '@shotkit/shotium';

// 1. 建立 IPC 连接
const client = await daemon.connect({
  name: 'custom-pool',      // 可选：守护进程命名隔离
  idleTimeoutMs: 300000,    // 无连接时空闲退出时间 (默认 5 分钟；0 为永不退出)
  prewarm: true,            // 启动时自动渲染空白页完成预热 (默认 true)
});

// 2. Client 实例可用方法
const res = await client.screenshot({ file: 'https://example.com' });
const status = await client.status();
await client.releaseMemory({ releaseWorkingSet: false });
client.close();

// 3. 守护进程全局管理
const info: DaemonStatus = await daemon.status();
await daemon.stop();
```

#### `DaemonStatus` 结构说明

```ts
interface DaemonStatus {
  pid: number;              // 守护进程 OS 进程 ID
  endpoint: string;         // IPC 套接字路径 / 命名管道名称
  cacheDir: string | null;  // 使用的缓存目录
  warm: boolean;            // 引擎是否已完成初始化预热
  uptimeMs: number;         // 进程运行时长 (毫秒)
  connections: number;      // 当前活跃客户端连接数
  inFlight: number;         // 正在处理中的截图请求数
  served: number;           // 累计已完成的截图请求总数
  idleTimeoutMs: number;    // 空闲退出超时设置
  version: string;          // 引擎版本
}
```

---

### `cache` 模块

管理跨进程与跨生命周期持久化的 HTTP 磁盘缓存：

```ts
import { cache } from '@shotkit/shotium';

// 1. 目录查询
cache.getDir();                     // 获取当前项目的缓存目录绝对路径
cache.getDirs({ target: 'all' });   // 获取当前主机上所有的 shotium 缓存目录

// 2. 列出缓存条目元数据
const files = await cache.getFiles(); // [{ url, lastUsedMs, bytes, dir }, ...]

// 3. 清理缓存并获取结果
const result: CacheClearResult = await cache.clear({
  glob: ['https://example.com/**'], // 按 URL Glob 模式匹配清理
  maxAge: 86400,                    // 清理超过 1 天未访问的条目 (秒)
  maxSize: 64 * 1024 * 1024,        // 按 LRU 淘汰至 64 MB 以内
});

console.log(`清理条目: ${result.removed}, 清理前体积: ${result.bytesBefore}, 清理后体积: ${result.bytesAfter}`);
```

#### 缓存机制与规范

- **存储路径**：缓存目录位于 `~/.shotium/cache/<project-hash>`，避免系统临时目录（如 `/tmp`）定期清理导致缓存失效。
- **索引与安全清理**：缓存条目以 URL 键哈希命名，元数据由索引文件维护。必须通过 `cache` API 进行清理，避免直接在文件系统中手动删除文件造成索引损坏。
- **多进程共享**：多个进程可安全共享同一个缓存目录，内部具备无锁只读与安全写入保障。

---

## 性能基准测试

当前正式基准会在六种原生 GitHub runner 平台执行，逐样本结果统一归档于 [`benchmark-results/`](benchmark-results/LATEST.md)。下表是早期单机 Windows 数据，仅作为 [legacy 证据](benchmark-results/legacy/2026-08-25-windows-local/RESULTS.md) 保留，不能与 CI 系列直接比较。

### 1. 冷启动与吞吐量

| 引擎 | 冷启动耗时 (首图) | 稳态增量耗时 / 图 | 10 页 4 并发总耗时 (4 进程) | 工作集内存 (私有工作集) |
|---|--:|--:|--:|--:|
| **shotium** | **352 ms** | **47 ms** | **237 ms (42 张/秒)** | **256 MiB (私有 73 MiB)** |
| Puppeteer (`chrome-headless-shell`) | 946 ms | 133 ms | 905 ms (11 张/秒) | 647 MiB (私有 180 MiB) |
| Puppeteer (`headless Chrome`) | 1,559 ms | 132 ms | 1,890 ms (5 张/秒) | 1,287 MiB (私有 379 MiB) |
| Playwright (`chrome-headless-shell`) | 962 ms | 150 ms | 1,171 ms (9 张/秒) | 652 MiB (私有 215 MiB) |
| Playwright (`headless Chrome`) | 1,385 ms | 146 ms | 1,276 ms (8 张/秒) | 789 MiB (私有 282 MiB) |

> 注：4 并发测试中，shotium 使用 4 个独立进程；对比引擎使用单个浏览器实例内的 4 个并发页面。shotium 单个进程内渲染器为串行模型，横向扩展依赖多进程。

### 2. 预热引擎连接性能

| 引擎 | 客户端端到端耗时 | 连接握手耗时 | 截图执行耗时 | 空闲总内存 | 空闲仅引擎内存 |
|---|--:|--:|--:|--:|--:|
| **shotium daemon** | **250 ms** | **2.3 ms** | **57 ms** | **58 MiB** | **2.8 MiB** |
| Puppeteer (`chrome-headless-shell`) | 512 ms | 17.0 ms | 170 ms | 355 MiB | 287 MiB |
| Puppeteer (`headless Chrome`) | 588 ms | 19.0 ms | 204 ms | 587 MiB | 519 MiB |
| Playwright (`chrome-headless-shell`) | 764 ms | 38.0 ms | 189 ms | 272 MiB | 154 MiB |
| Playwright (`headless Chrome`) | 680 ms | 34.0 ms | 228 ms | 400 MiB | 299 MiB |

---

## 技术架构

```
┌────────────────────────────────────────────────────────┐
│  shotium (TypeScript / Node.js API 层)                  │
│  生命周期管理 · 串行请求队列 · 参数校验                      │
└──────────────────────────┬─────────────────────────────┘
                           │ Node-API：JSON 输入，二进制图像输出
┌──────────────────────────▼─────────────────────────────┐
│  shotium.node  ──►  libshotium (C ABI, shot_api.h)      │
│                     同进程加载，无跨进程 IPC                │
│                                                        │
│  Blink 渲染管线:                                        │
│  DOM ──► 样式与布局计算 ──► 绘制记录 (cc::PaintRecord)  │
│                                      │                 │
│  Skia CPU 光栅化 ◄───────────────────┘                 │
│         │                                              │
│         ▼                                              │
│  图像编码器 (PNG / JPEG / WebP)                         │
│                                                        │
│  网络子系统: Chromium //net (HTTP/2, HTTPS, 磁盘缓存)    │
└────────────────────────────────────────────────────────┘
```

1. **直接调用 Blink 核心**：绕过 `//content` 多进程框架与合成器层，直接创建 `PageNonOrdinary` 并同步调用 `LocalFrameView::UpdateAllLifecyclePhases()` 执行布局排版。
2. **Skia CPU 内存光栅化**：排版阶段生成的 `cc::PaintRecord` 直接光栅化至内存 `SkSurface`，随后经由 Skia 原生 Codec 压缩为目标图像格式。
3. **内嵌网络子系统**：直接链接 Chromium `//net` 核心库（`URLRequestContext`、BoringSSL、SpdySession），无冗余 IPC 中转。
4. **一致性交付**：npm 平台依赖包与独立 CLI 执行文件均调用相同的底座 `shot::Capture` 接口，测试套件保障两者渲染结果逐字节一致。

---

## 环境变量

| 变量名 | 说明 |
|---|---|
| `SHOTIUM_ENDPOINT` | 覆盖守护进程的 IPC 通信地址（Unix Socket 路径或 Windows 命名管道）。 |
| `SHOTIUM_DAEMON_LOG` | 指定 `daemon.connect()` 自动启动的守护进程诊断日志输出路径。 |

平台预编译二进制默认随 npm 平台包分发；源码构建时可指定 `resourceDir` 指向编译产物目录：

```ts
shotium.start({ resourceDir: '/path/to/out/Shot' });
```

---

## C ABI / FFI 跨语言集成

针对 C++、Rust、Go、Python 等环境，shotium 在 [`shot/shot_api.h`](shot/shot_api.h) 中提供了纯 C 导出接口：

```c
#include "shot_api.h"

shot_engine* engine = NULL;
shot_buffer* error = NULL;
shot_engine_create("{}", &engine, &error);

shot_buffer* png = NULL;
shot_buffer* stats = NULL;  /* 可选参数；不需要统计可传 NULL */
shot_engine_capture(engine, "{\"file\":\"https://example.com\"}",
                    &png, &stats, &error);

const uint8_t* data = shot_buffer_data(png);
size_t size = shot_buffer_size(png);

shot_buffer_free(png);
shot_buffer_free(stats);
shot_engine_destroy(engine);
```

> **ABI 版本**：当前 ABI 版本为 **2**（自 0.3 起新增 `out_stats` 参数及 `shot_engine_status`、`shot_cache_list`、`shot_cache_clear` 接口）。调用前可通过 `shot_abi_version()` 校验与头文件 `SHOT_ABI_VERSION` 的兼容性。

---

## 源码构建指南

### 前置依赖

- 已配置至 `PATH` 的 [depot_tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up)
- 约 40 GB 可用磁盘空间
- 平台编译工具链：
  - **Windows**：Visual Studio 2022 + Windows SDK (10.0.26100.0 或 10.0.28000)
  - **macOS**：Xcode
  - **Linux**：系统基础依赖（`./build/install-build-deps.sh --no-prompt --no-nacl`）

### 构建步骤

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
target_os = ["win"] # 或 ["mac"], ["linux"]
EOF

gclient sync --nohooks --no-history
gclient runhooks

cd src

# 生成裁剪后的 ICU 数据表
python3 tools/shot/icu_repack.py \
  third_party/icu/cast/icudtl.dat \
  third_party/icu/shot/icudtl.dat --preset shot

mkdir -p out/Shot
echo 'import("//build/args/shot.gn")' > out/Shot/args.gn
# macOS 环境: echo 'import("//build/args/shot-mac.gn")' > out/Shot/args.gn
# Linux 环境: echo 'import("//build/args/shot-linux.gn")' > out/Shot/args.gn

gn gen out/Shot
ninja -C out/Shot shot
```

### 测试套件

```bash
python tools/shot/serve_check.py   out/Shot/shotium.exe  # 协议格式与图像编码校验
python tools/shot/net_check.py     out/Shot/shotium.exe  # HTTP、SSL、重定向与缓存校验
node   tools/shot/node_check.cjs   out/Shot/shotium.exe  # Addon 绑定、请求队列与生命周期校验
node   tools/shot/daemon_check.cjs out/Shot/shotium.exe  # 守护进程 IPC 与并发校验
python tools/shot/demo_check.py    out/Shot/shotium.exe  # 视觉渲染一致性参考测试 (84 项 reftest)
```

本地编译 Node.js 原生扩展并运行测试：

```bash
export SHOT_INCLUDE_DIR=$PWD/shot SHOT_LIB_DIR=$PWD/out/Shot
npx node-gyp@13 rebuild -C shotium/native
cp out/Shot/libshotium.so out/Shot/*.pak shotium/native/build/Release/
npm --prefix shotium install && npm --prefix shotium run build
```

---

## 许可证

基于 Chromium 原生开源许可证：BSD-3-Clause。详见 [LICENSE](LICENSE)。
