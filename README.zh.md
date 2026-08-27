# shotium

> 基于精简 Chromium Blink 内核的高性能 HTML/CSS 静态截图引擎。

[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](LICENSE)
[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg)](https://www.npmjs.com/package/@shotkit/shotium)
[![Download](https://img.shields.io/badge/npm%20download-19.4%20MB%20(win32--x64)-green.svg)](https://www.npmjs.com/package/@shotkit/shotium)

[English](README.md) · **简体中文**

---

## 概述

**shotium** 是一款专为 HTML/CSS 静态渲染截图设计的高性能渲染引擎。它基于 Chromium 进行深度精简，保留了排版与渲染所需的完整管线（Blink 排版、Skia CPU 光栅化、字体处理、图片解码及 Chromium `//net` 网络栈）。

**项目中完全移除了 V8 及多进程浏览器外壳**（无 `//content`、GPU 进程、合成器和 DevTools 调试协议）。最终产物约 41 MB，冷启动耗时低于 350 ms，单张截图渲染耗时约 47 ms，内存占用显著低于标准的 Headless Chrome / Puppeteer。

它有两种交付形态。给 Node.js 的是一个 Node-API addon 加一个共享库，直接加载进调用方进程：win32-x64 平台包 `npm install` 下载 19.4 MB，不拉起任何子进程。其余场景是 [Releases 页](https://github.com/sj817/shotium/releases) 上的独立可执行程序，压缩后 15.3 MB。

```ts
import shotium from '@shotkit/shotium';

shotium.start();

const { image, stats } = await shotium.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  fullPage: true,
});

console.log(stats.timing.total, 'ms，', stats.requests, '个请求命中缓存', stats.fromCache, '个');

await shotium.stop();
```

---

## 技术范围与设计边界

shotium 适用于各类服务端 HTML/CSS 渲染场景（如自动生成社交分享卡片、电子发票/小票、营销海报、数据报表导出及 SSR 页面快照）。

### 支持的能力
- **Blink 排版引擎**：完整支持现代 CSS 规范（Flexbox、Grid、Web 字体、SVG、伪元素、CSS 变量及复杂选择器）。
- **图片格式编码**：原生输出 PNG、JPEG、WebP 格式（支持质量参数与透明通道）。
- **Chromium 网络栈**：直连 Chromium `//net`（支持 HTTPS、HTTP/2、Brotli 压缩、重定向、磁盘缓存与内存 Cookie）。
- **进程内渲染**：引擎是一个架在 C ABI 上的 Node-API addon，而不是被拉起的浏览器。没有 IPC、没有临时文件、没有端口，图片也不跨进程边界拷贝。不想付启动开销的调用方可以用常驻守护进程。

### 明确不包含的功能
- **无 JavaScript 执行环境**：完全移除 V8 引擎，不执行任何 `<script>` 标签与客户端 JS Hydration 逻辑（纯客户端渲染的 SPA 应用将无法直接渲染，需配合 SSR 或服务端生成的静态 HTML 使用）。
- **无浏览器沙箱机制**：无 Chromium 多进程沙箱，SSRF 防御与 URL 白名单校验需由业务层处理。本地 `file://` 子资源加载默认关闭（`allowFileAccess: false`）。
- **字体光栅化一致性**：采用固定伽马曲线的灰度抗锯齿，不读取宿主系统的 ClearType 次像素设置，保证同系统跨进程渲染结果逐字节一致。

---

## 安装

```bash
npm install @shotkit/shotium
```

预编译二进制已打包为 npm 可选依赖（覆盖 Windows、macOS、Linux 的 x64 与 arm64 架构），npm 安装时会自动匹配并下载对应平台的二进制文件，无需本地编译或后置下载脚本。

---

## 使用方式

### 1. 进程内引擎

引擎跑在你自己的 Node.js 进程里，通过 Node-API 加载 [`shot/shot_api.h`](shot/shot_api.h) 提供的 C ABI。`screenshot()` 返回的就是 Blink 刚编码出来的字节：不拉子进程，图片也不跨进程边界拷贝（单张约 **31 ms**）。

```ts
import shotium, { screenshot } from '@shotkit/shotium';

const { cacheDir, cacheActive } = shotium.start();

const { image, stats } = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'webp',
  quality: 85,
});

// 批次之间交还内存，但不交出引擎
shotium.releaseMemory({ releaseWorkingSet: true });

await shotium.stop();
```

**一个进程只有一个引擎 —— 但 `start()` 和 `stop()` 不限次数。** 启动 Blink 会写下进程级静态量，而它没有回退路径，所以一个进程只能有一个引擎，`new Runtime()` 会接管已有的那个而不是再造一个。但这说的是引擎有几个，不是你能要几次：`stop()` 只是让引擎停下来（队列排空、内存交还、`running: false`），`start()` 会把同一个引擎重新拿起来，缓存还是热的，想调多少次都行。`start()` 唯一会拒绝的是**不同的配置** —— 引擎的选项在创建时就定死了，所以指定一个跟当前引擎不一致的值会抛错，而不是默默用另一个值去渲染。并发调用会排队逐个执行，因为渲染器只有一个 —— 所以要并行就要多开进程。

`start()` 会返回引擎起来之后的状态。`cacheDir` 有值而 `cacheActive` 为 `false`，意思是那个目录没能打开，这个进程在裸跑 —— 渲染结果一样正确，只是每张图都多付一次网络往返，而且不会有任何报错。

> **从 0.2 升级。** 生命周期方法从 `shotium.runtime.*` 移到了模块本身，`purge()` 改名为 `releaseMemory()`，`screenshot()` 的返回值从 Buffer 变成了 `{ image, stats }`，`stop()` 不再是终局 —— 停了还能再 `start()`。`shotium.runtime` 和 `Runtime` 类仍然导出，供需要自己持有生命周期的调用方使用。

---

### 2. 常驻守护进程模式 (`daemon`)

推荐用于命令行工具（CLI）、CI 流水线及短生命周期的 Serverless 任务。

守护进程就是上面那个引擎，只不过跑在它自己的进程里，藏在本地套接字（Windows 命名管道 / POSIX Unix Domain Socket）后面。它启动时先渲一张空白页预热，所以第一个真实请求到达时它已经是热的；后续进程建立连接耗时仅约 **2.3 ms**。

```ts
import { daemon } from '@shotkit/shotium';

// 连接现有守护进程（若未运行则自动拉起）
const client = await daemon.connect();

const { image, stats } = await client.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

client.close();

// 可选：查看状态或手动停止
const status = await daemon.status(); // { running: true, pid: 12345, warm: true, ... }
await daemon.stop();
```

一条连接上可以同时挂多个未完成的请求 —— 每条消息都带 `id`。那是给客户端的便利，不是并发：守护进程这边也只有一个渲染器，按渲完的顺序回。想真并发就开两个守护进程，用 `name` 区分。

---

### 3. 独立命令行工具 (CLI)

无 Node.js 环境时，可直接从 [GitHub Releases](https://github.com/sj817/shotium/releases) 下载独立可执行程序：

```bash
# 截取远程 URL 并输出至文件
shotium https://example.com --width 1280 --height 720 -o output.png

# 截取本地 HTML 全页面并输出为 WebP
shotium --file page.html --full-page --type webp --quality 85 -o output.webp

# 常驻，从 stdin 读取长度前缀 JSON 请求并作答
shotium --serve --cache-dir /var/tmp/shotium-cache
```

---

## API 参考

### `ScreenshotOptions`

```ts
interface ScreenshotOptions {
  /** 目标 URL (http/https/file) 或本地文件路径 */
  file: string;

  /** 输出图片格式 (默认: 'png') */
  type?: 'png' | 'jpeg' | 'webp';

  /** 视口尺寸 (默认: 1280x720) */
  viewport?: { width?: number; height?: number };

  /** 是否截取完整长图（滚动全文档） */
  fullPage?: boolean;

  /** 仅截取指定 CSS 选择器所匹配元素的包围盒 */
  selector?: string;

  /** 截取指定矩形坐标区域 */
  clip?: { x: number; y: number; width: number; height: number };

  /** 压缩质量: 1-100 (仅限 jpeg 与 webp，默认: 90) */
  quality?: number;

  /** 渲染缩放比例（设备像素比）: 0.01 - 8.0 (默认: 1.0) */
  scale?: number;

  /** 是否保留透明背景 (仅限 png 与 webp) */
  omitBackground?: boolean;

  /** 直接写入文件的路径（若设置则 screenshot 返回 null） */
  path?: string;

  /** 页面导航与加载控制 */
  pageGotoParams?: {
    timeout?: number;
    waitUntil?: 'load' | 'networkidle';
  };

  /** 是否允许文档访问本地 file:// 子资源 (默认: false) */
  allowFileAccess?: boolean;

  /** 这次截图可以怎么用 HTTP 缓存 (默认: 'default') */
  cache?: 'default' | 'reload' | 'no-store' | 'only-if-cached';

  /** 附加请求头，只发给与文档同源的地址 */
  headers?: Record<string, string>;
}
```

> **注意**：`fullPage`、`selector` 与 `clip` 为互斥参数，同时传入多个将触发参数校验错误。

`cache` 的四个取值照抄 `fetch` 的拼写，含义也一致：`reload` 不读缓存但写缓存，`no-store` 不读也不写，`only-if-cached` 不碰网络、未命中即报错。它对子资源和主文档同时生效 —— 只刷新 HTML 而复用昨天的样式表，不会是有人真正想要的行为。

`headers` 到同源边界为止。调用方传 `Authorization` 或 `Cookie` 是冲着被截的站点去的，所以它不会跟着发给另一个源上的样式表或字体。

---

### `StartOptions`

```ts
interface StartOptions {
  /**
   * HTTP 磁盘缓存目录。默认是 ~/.shotium/cache 下按项目区分的一个目录；
   * 传 null 关闭缓存。
   */
  cacheDir?: string | null;

  /** 该目录的容量上限，单位字节 (默认: 256 MB) */
  cacheMaxBytes?: number;

  /** 覆盖内置的 User-Agent */
  userAgent?: string;

  /** shotium_data.pak 与 shotium_strings.pak 所在目录 */
  resourceDir?: string;
}
```

`start()` 和 `status()` 返回 `{ running, cacheDir, cacheActive }`。`running` 说的是这个生命周期，不是这个进程：`stop()` 之后它是 `false`，而 `cacheDir` 仍然是那个停下来的引擎持有的目录。

**0.3 起缓存默认开启**，这是相对 0.2 的反转。理由是量出来的：不开缓存时，每次截一个 `https:` URL 都要付 DNS、TLS 和一次往返，对一个小页面来说这是整个调用耗时的大头，也是调用方完全没预期要花的时间。至于「短命进程会留下一个目录」这个顾虑，答案是把目录按项目区分、加上容量上限、并放在 `~/.shotium` 这一个用户找得到也删得掉的地方，而不是干脆不缓存。

---

### `CaptureStats`

每次截图都会报告它花了多少。这些数字引擎内部一直知道，只是在 0.3 之前一个都没往外传。

```ts
interface CaptureStats {
  requests: number;     // 文档请求过的所有资源，含它自己
  fromCache: number;    // 正文由 HTTP 缓存作答的数量（含义见下）
  failed: number;
  bytes: number;        // 解码后的正文字节数，不是传输体积
  httpStatus: number;   // 主文档自身的状态码；file: URL 为 0
  finalUrl: string;     // 跟随重定向之后的地址
  timing: {
    fetch: number;      // 取主文档：DNS、TCP、TLS、一次往返
    render: number;     // 解析、子资源、样式、布局、绘制
    encode: number;
    total: number;
  };
}
```

`timing.fetch` 是最容易出乎意料的一项。对一个冷的 `https:` URL，它通常比渲染本身大一个数量级 —— 这也就是大部分「为什么这次特别慢」的答案：

```
本地文件          fetch   0.2 ms   render  20 ms   total   25 ms
https，冷连接      fetch 321.1 ms   render  16 ms   total  350 ms
https，命中缓存    fetch   0.7 ms   render  18 ms   total   31 ms
```

`fromCache` 的含义是「正文来自磁盘」，不等于「没碰网络」。一个已过期但可以重新验证的条目，仍然要付一次条件请求和一个 304 —— 缓存在那种情况下省下的是下载量，不是往返 —— 所以 `fromCache: 1` 同时 `timing.fetch` 有 88 ms，是正常结果而不是自相矛盾。

失败时统计同样会给出来，挂在 `error.stats` 上 —— 一次取了四十个子资源之后超时的截图，它自己已经把原因说清楚了。

---

### `cache`

HTTP 缓存。它比任何一个引擎都活得久：目录就在磁盘上，跟有没有引擎在跑无关，所以这组方法在 `start()` 之前和 `stop()` 之后都能用 —— 而且 `stop()` 不会清掉它。缓存的意义在于下一次运行，那它就必须活过这一次。

```ts
import { cache } from '@shotkit/shotium';

cache.getDir();                     // 本项目的缓存目录，绝对路径，统一用 /
cache.getDirs({ target: 'all' });   // 本机上 shotium 的所有缓存目录

await cache.getFiles();             // [{ url, lastUsedMs, bytes, dir }, ...]

await cache.clear();                                       // 全清
await cache.clear({ glob: ['https://example.com/**'] });    // 按 URL 模式清
await cache.clear({ maxAge: 86400 });                       // 清掉一天没用过的
await cache.clear({ maxSize: 64 * 1024 * 1024 });           // 按 LRU 清到 64 MB 以内
```

缓存目录在 `~/.shotium/cache` 下，按项目分，目录名是项目根路径的哈希。不放系统临时目录 —— 那个目录的定义就是「不用活到明天」：`/tmp` 重启即清，十天没碰过的文件也会被清掉，而一个全部价值都在下一次运行的缓存不能住在那里。

`target` 决定操作哪个目录：`'current'`（默认）是本项目的，`'all'` 是共享根目录下的全部，传字符串则是绝对路径或项目哈希。

有两件事值得知道，因为它们都是「凭直觉猜会猜错」的地方：

- **缓存文件不是按 URL 命名的。** 一个缓存目录里躺的是 `5349fbae98c6d9a1_0` 这样的文件（文件名是 entry key 的哈希），外加一个 `index`。所以 `getFiles()` 返回的是 URL 而不是文件名，`glob` 匹配的也是 URL；照着文件名写的模式什么都匹配不到。
- **删除一律走缓存后端，绝不走文件系统。** 手工删 entry 文件会让 index 里留下已经不存在的条目，下一个打开这个目录的进程要么重建索引，要么发现不一致后整个丢弃。唯一的例外是「进程里压根没有引擎时清空整个目录」—— 那种情况会把目录整个删掉，而它之所以安全，恰恰是因为不会有东西残留下来跟 index 对不上。

多个进程可以共用同一个缓存目录，而且都能缓存 —— 后端不做跨进程加锁。但在单个进程内，一个目录只有一个后端，所以这些调用会借用正在跑的引擎的那一个。

---

## 性能实测基准

测试环境：32 核 Windows 台式机，测试语料为 10 个本地静态 HTML 文档（1280×720 视口，PNG 输出，`waitUntil: 'load'`，每项重复 7 次取中位数）。详细测试方案与原始日志参见 [`bench/cross/RESULTS.md`](bench/cross/RESULTS.md)。

### 1. 冷启动与并发吞吐量

| 引擎 | 冷启动耗时 (单图) | 稳态增量耗时 / 图 | 10 页 4 并发总耗时 (4 进程) | 工作集内存 (私有工作集) |
|---|--:|--:|--:|--:|
| **shotium** | **352 ms** | **47 ms** | **237 ms (42 张/秒)** | **256 MiB (私有 73 MiB)** |
| Puppeteer (`chrome-headless-shell`) | 946 ms | 133 ms | 905 ms (11 张/秒) | 647 MiB (私有 180 MiB) |
| Puppeteer (`headless Chrome`) | 1,559 ms | 132 ms | 1,890 ms (5 张/秒) | 1,287 MiB (私有 379 MiB) |
| Playwright (`chrome-headless-shell`) | 962 ms | 150 ms | 1,171 ms (9 张/秒) | 652 MiB (私有 215 MiB) |
| Playwright (`headless Chrome`) | 1,385 ms | 146 ms | 1,276 ms (8 张/秒) | 789 MiB (私有 282 MiB) |

> 「4 并发」那一列是四个 shotium **进程** 对上对面浏览器里的四个页面测出来的。一个进程只有一个渲染器，所以 shotium 靠进程扩展；单个 `runtime` 无论排着多少调用，都是串行的。

### 2. 客户端连接常驻引擎性能

| 引擎 | 客户端端到端耗时 | 连接握手开销 | 截图执行耗时 | 空闲总内存 | 空闲仅引擎内存 |
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
│  生命周期 · 串行队列 · 参数校验                             │
└──────────────────────────┬─────────────────────────────┘
                           │ Node-API：JSON 进，字节出
┌──────────────────────────▼─────────────────────────────┐
│  shotium.node  ──►  libshotium (C ABI, shot_api.h)      │
│                     同一进程，无 IPC                    │
│                                                        │
│  Blink 渲染管线:                                        │
│  DOM ──► 样式与布局计算 ──► 绘制记录 (cc::PaintRecord)  │
│                                      │                 │
│  Skia CPU 内存光栅化 ◄────────────────┘                 │
│         │                                              │
│         ▼                                              │
│  图片编码 (PNG / JPEG / WebP)                           │
│                                                        │
│  网络子系统: Chromium //net (HTTP/2, HTTPS, 磁盘缓存)    │
└────────────────────────────────────────────────────────┘
```

1. **直接调用 Blink 核心**：绕过 `//content` 多进程与合成器层，直接创建 `PageNonOrdinary` 并同步调用 `LocalFrameView::UpdateAllLifecyclePhases()` 执行布局排版。
2. **Skia CPU 内存光栅化**：将排版阶段生成的 `cc::PaintRecord` 直接绘制到内存 `SkSurface`，随后经由 Skia 原生 Codec 压缩为指定格式。
3. **直连网络栈**：直接链接 Chromium `//net`（`URLRequestContext`、BoringSSL、SpdySession），无多余 IPC 转发。
4. **一个引擎，两个前端**：npm 平台包里装的是共享库和 addon，独立可执行程序是 Releases 上另一个下载项。两者调用的是同一个 `shot::Capture`，`tools/shot/node_check.cjs` 会断言它们产出的图片逐字节相同。

---

## 环境变量

| 变量 | 作用 |
|---|---|
| `SHOTIUM_ENDPOINT` | 覆盖守护进程的套接字路径 / 命名管道。 |
| `SHOTIUM_DAEMON_LOG` | `daemon.connect()` 拉起的守护进程把诊断信息写到哪里。 |

addon 从已安装的平台包里加载，源码检出时则从 `shotium/native/build/Release/` 加载。资源包（`.pak`、ICU）默认在 addon 旁边找 —— 安装出来就是这么放的；本地构建则把它们留在构建目录里，所以要把 `resourceDir` 指过去：

```ts
shotium.start({ resourceDir: '/path/to/out/Shot' });
```

---

## C ABI / FFI 跨语言集成

对于 C++、Rust、Go、Python 等环境，shotium 在 [`shot/shot_api.h`](shot/shot_api.h) 中提供了纯 C 接口：

```c
#include "shot_api.h"

shot_engine* engine = NULL;
shot_buffer* error = NULL;
shot_engine_create("{}", &engine, &error);

shot_buffer* png = NULL;
shot_buffer* stats = NULL;  /* 可选，不需要就传 NULL */
shot_engine_capture(engine, "{\"file\":\"https://example.com\"}",
                    &png, &stats, &error);

const uint8_t* data = shot_buffer_data(png);
size_t size = shot_buffer_size(png);

shot_buffer_free(png);
shot_buffer_free(stats);
shot_engine_destroy(engine);
```

0.3 起 ABI 版本是 **2**：`shot_engine_capture` 多了 `out_stats` 参数，并新增了
`shot_engine_status`、`shot_cache_list` 和 `shot_cache_clear`。调用任何东西之前，
先拿 `shot_abi_version()` 和头文件里的 `SHOT_ABI_VERSION` 比一下 —— 预编译的
addon 和预编译的引擎是两个文件，没有任何机制阻止它们是两个版本。

---

## 源码构建指南

### 前置依赖

- 已加入 `PATH` 的 [depot_tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up)
- 约 40 GB 磁盘空间
- 编译工具链：
  - **Windows**：Visual Studio 2022 + Windows SDK (10.0.26100.0 或 10.0.28000)
  - **macOS**：Xcode
  - **Linux**：系统构建依赖（`./build/install-build-deps.sh --no-prompt --no-nacl`）

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

# 重新生成 ICU 数据表
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
python tools/shot/serve_check.py   out/Shot/shotium.exe  # 协议与编码器校验
python tools/shot/net_check.py     out/Shot/shotium.exe  # HTTP、SSL、重定向与缓存校验
node   tools/shot/node_check.cjs   out/Shot/shotium.exe  # addon、串行队列与生命周期校验
node   tools/shot/daemon_check.cjs out/Shot/shotium.exe  # 守护进程与并发校验
python tools/shot/demo_check.py    out/Shot/shotium.exe  # 视觉排版参考测试 (84 项 reftest)
```

两个 node 套件加载的是 addon 而不是可执行文件，所以要先拿这次检出刚产出的那个库把 addon 编出来 —— 拿别的构建校出来的 addon 什么也没校。命令行上那个可执行文件参数，是用来比对输出的基准。

```bash
export SHOT_INCLUDE_DIR=$PWD/shot SHOT_LIB_DIR=$PWD/out/Shot
npx node-gyp@13 rebuild -C shotium/native
cp out/Shot/libshotium.so out/Shot/*.pak shotium/native/build/Release/
npm --prefix shotium install && npm --prefix shotium run build
```

---

## 许可证

基于 Chromium 原生开源许可证：BSD-3-Clause。详见 [LICENSE](LICENSE)。
