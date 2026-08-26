# shotium

> 基于精简 Chromium Blink 内核的高性能 HTML/CSS 静态截图引擎。

[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](LICENSE)
[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg)](https://www.npmjs.com/package/@shotkit/shotium)
[![Release](https://img.shields.io/badge/binary-~12MB%20compressed-green.svg)](https://github.com/sj817/shotium/releases)

[English](README.md) · **简体中文**

---

## 概述

**shotium** 是一款专为 HTML/CSS 静态渲染截图设计的高性能渲染引擎。它基于 Chromium 进行深度精简，保留了排版与渲染所需的完整管线（Blink 排版、Skia CPU 光栅化、字体处理、图片解码及 Chromium `//net` 网络栈）。

**项目中完全移除了 V8 及多进程浏览器外壳**（无 `//content`、GPU 进程、合成器和 DevTools 调试协议）。最终编译产物为一个独立的单文件可执行程序（未压缩约 41 MB，**压缩后约 12.8 MB**），冷启动耗时低于 350 ms，单张截图渲染耗时约 47 ms，内存占用显著低于标准的 Headless Chrome / Puppeteer。

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

## 技术范围与设计边界

shotium 适用于各类服务端 HTML/CSS 渲染场景（如自动生成社交分享卡片、电子发票/小票、营销海报、数据报表导出及 SSR 页面快照）。

### 支持的能力
- **Blink 排版引擎**：完整支持现代 CSS 规范（Flexbox、Grid、Web 字体、SVG、伪元素、CSS 变量及复杂选择器）。
- **图片格式编码**：原生输出 PNG、JPEG、WebP 格式（支持质量参数与透明通道）。
- **Chromium 网络栈**：直连 Chromium `//net`（支持 HTTPS、HTTP/2、Brotli 压缩、重定向、磁盘缓存与内存 Cookie）。
- **多进程调度与隔离**：内置进程池管理、任务队列调度、超时检测与崩溃自动重试。

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

### 1. 多进程池模式 (`runtime`)

推荐用于常规后端 API 服务与后台队列任务。由主进程管理独立的 Worker 进程池，提供并发分发与崩溃隔离能力。

```ts
import { runtime, screenshot } from '@shotkit/shotium';

// 可选：监听运行时生命周期事件
runtime.on('crash', ({ worker }) => console.warn(`Worker ${worker} 崩溃并已自动重启`));
runtime.on('timeout', ({ worker, timeout }) => console.warn(`Worker ${worker} 超时 (${timeout}ms)`));

// 启动进程池
runtime.start({
  workers: 4,                        // 默认：CPU 核心数的一半
  cacheDir: '/var/tmp/shotium-cache' // 可选：HTTP 磁盘缓存路径
});

// 执行截图
const buffer = await screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  type: 'webp',
  quality: 85,
});

await runtime.stop();
```

---

### 2. 常驻守护进程模式 (`daemon`)

推荐用于命令行工具（CLI）、CI 流水线及短生命周期的 Serverless 任务。

守护进程在本地套接字（Windows 命名管道 / POSIX Unix Domain Socket）后台常驻预热好的 Worker 进程池，后续进程建立连接耗时仅约 **2.3 ms**。

```ts
import { daemon } from '@shotkit/shotium';

// 连接现有守护进程（若未运行则自动拉起）
const client = await daemon.connect({ workers: 4 });

const png = await client.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

client.close();

// 可选：查看状态或手动停止
const status = await daemon.status(); // { running: true, pid: 12345, workers: 4, ... }
await daemon.stop();
```

---

### 3. 进程内原生引擎模式 (`@shotkit/shotium/native`)

推荐用于单线程连续批处理任务，追求极限低延迟（单张截图渲染仅需 **~31 ms**）。

通过 Node-API 与 C ABI（[`shot/shot_api.h`](shot/shot_api.h)）直接将精简版 Blink 内核加载到当前 Node.js 进程内运行。该模式下请求在进程内串行执行。

```ts
import { native } from '@shotkit/shotium/native';

const png = await native.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
});

// 批处理结束后主动释放物理内存与缓存
native.purge({ releaseWorkingSet: true });
await native.stop();
```

---

### 4. 独立命令行工具 (CLI)

无 Node.js 环境时，可直接从 [GitHub Releases](https://github.com/sj817/shotium/releases) 下载独立可执行程序：

```bash
# 截取远程 URL 并输出至文件
shotium https://example.com --width 1280 --height 720 -o output.png

# 截取本地 HTML 全页面并输出为 WebP
shotium --file page.html --full-page --type webp --quality 85 -o output.webp

# 作为常驻 Worker 服务启动
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

  /** 失败或超时后的自动重试次数 (默认: 0) */
  retry?: number;
}
```

> **注意**：`fullPage`、`selector` 与 `clip` 为互斥参数，同时传入多个将触发参数校验错误。

---

## 性能实测基准

测试环境：32 核 Windows 台式机，测试语料为 10 个本地静态 HTML 文档（1280×720 视口，PNG 输出，`waitUntil: 'load'`，每项重复 7 次取中位数）。详细测试方案与原始日志参见 [`bench/cross/RESULTS.md`](bench/cross/RESULTS.md)。

### 1. 冷启动与并发吞吐量

| 引擎 | 冷启动耗时 (单图) | 稳态增量耗时 / 图 | 10 页 4 并发总耗时 | 工作集内存 (私有工作集) |
|---|--:|--:|--:|--:|
| **shotium** | **352 ms** | **47 ms** | **237 ms (42 张/秒)** | **256 MiB (私有 73 MiB)** |
| Puppeteer (`chrome-headless-shell`) | 946 ms | 133 ms | 905 ms (11 张/秒) | 647 MiB (私有 180 MiB) |
| Puppeteer (`headless Chrome`) | 1,559 ms | 132 ms | 1,890 ms (5 张/秒) | 1,287 MiB (私有 379 MiB) |
| Playwright (`chrome-headless-shell`) | 962 ms | 150 ms | 1,171 ms (9 张/秒) | 652 MiB (私有 215 MiB) |
| Playwright (`headless Chrome`) | 1,385 ms | 146 ms | 1,276 ms (8 张/秒) | 789 MiB (私有 282 MiB) |

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
│  进程池管理 · 任务队列调度 · 统一事件总线 · 自动重试机制    │
└──────────────────────────┬─────────────────────────────┘
                           │ 长度前缀 JSON / 二进制 IPC
┌──────────────────────────▼─────────────────────────────┐
│  shotium.exe --serve (C++ Worker 渲染进程)              │
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

---

## 自定义二进制路径

如需使用本地编译的产物或自定义路径，可通过环境变量设置：

```bash
export SHOTIUM_BINARY=/custom/path/to/shotium.exe
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
shot_engine_capture(engine, "{\"file\":\"https://example.com\"}", &png, &error);

const uint8_t* data = shot_buffer_data(png);
size_t size = shot_buffer_size(png);

shot_buffer_free(png);
shot_engine_destroy(engine);
```

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
node   tools/shot/node_check.cjs   out/Shot/shotium.exe  # 进程池生命周期与重试校验
node   tools/shot/daemon_check.cjs out/Shot/shotium.exe  # 守护进程与并发校验
python tools/shot/demo_check.py    out/Shot/shotium.exe  # 视觉排版参考测试 (84 项 reftest)
```

---

## 许可证

基于 Chromium 原生开源许可证：BSD-3-Clause。详见 [LICENSE](LICENSE)。
