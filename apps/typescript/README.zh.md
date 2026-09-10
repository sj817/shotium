# @shotkit/shotium

[English](./README.md) · 简体中文

基于 Chromium 深度裁剪的静态网页渲染引擎：保留 Blink 排版、Skia 绘制与网络栈，剥离 V8 引擎与浏览器外壳，专为高性能服务端截图设计

[![npm version](https://img.shields.io/npm/v/@shotkit/shotium.svg?label=npm)](https://www.npmjs.com/package/@shotkit/shotium) [![Chromium baseline](https://img.shields.io/badge/chromium-155.0.8048.0-4285F4?logo=googlechrome&logoColor=white)](https://chromium.googlesource.com/chromium/src/+/refs/tags/155.0.8048.0) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://github.com/sj817/shotium/blob/main/LICENSE)

`@shotkit/shotium` 是 [shotium](https://github.com/sj817/shotium) 专为 Node.js 提供的官方 SDK；shotium 从 Chromium 源码中深度剥离出核心静态渲染能力：保留负责 DOM/CSS 排版与绘制的 Blink、负责 CPU 栅格化与图像编码的 Skia 以及网络栈 `//net`，彻底移除了 V8 引擎、浏览器外壳、GPU 进程及 DevTools；页面直接在当前 Node.js 宿主进程内的专用引擎线程上完成排版与绘制，并返回 PNG、JPEG 或 WebP 字节流——无需拉起庞大的无头浏览器，无 WebSocket 协议损耗，亦无子进程僵尸泄漏风险

本包自身具备**零运行时依赖（0 dependencies）**特性；原生引擎通过 6 个平台架构包分发，`npm install` 时通过 `optionalDependencies` 自动按需拉取：涵盖 Windows、macOS 与 Linux 的 x64 及 arm64 架构

## 目录

| 导航分类 | 核心内容与快速跳转 |
|:---|:---|
| **起步与控制** | [快速安装](#安装) · [极简上手](#快速上手) · [直接截图](#直接截图) · [Web 服务生命周期](#长期运行服务里的生命周期管理) · [CLI 守护进程](#短生命周期进程的常驻守护进程) |
| **实战场景** | [内存 HTML 渲染](#内存中的-html-渲染) · [本地资源授权](#本地子资源访问) · [流式写盘](#返回-buffer-与直接写文件) · [超长页面分片](#超长页面分片渲染) · [磁盘缓存](#http-磁盘缓存管理) · [错误排查](#错误排查与诊断) |
| **API 参考** | [screenshot()](#screenshotoptions) · [screenshotTiles()](#screenshottilesoptions) · [生命周期控制器](#startoptionsstatusstopreleasememoryoptions) · [daemon](#daemon) · [cache](#cache) · [配置参数](#screenshotoptions-1) |
| **技术规范** | [TypeScript 类型定义](#类型定义) · [限制与边界](#限制与边界) · [开源协议](#许可证) |

## 安装

```bash
npm install @shotkit/shotium
# 或：pnpm add @shotkit/shotium · yarn add @shotkit/shotium · bun add @shotkit/shotium
```

运行环境要求 Node.js 18 或更高版本，本包提供原生 ESM 输出并附带完整 TypeScript 类型声明：

- 在支持 ESM 的 Node.js 环境中直接通过 `import { screenshot } from '@shotkit/shotium'` 导入
- Node.js 20.19 与 22.12 及更高版本原生支持 `require('@shotkit/shotium')` 同步加载 ES 模块
- 早期 CommonJS 环境可使用动态导入 `const { screenshot } = await import('@shotkit/shotium')`

导入本包不会触发原生引擎初始化，在首次调用 `screenshot()` 或显式执行 `start()` 之前不会执行任何原生 Native 逻辑

## 用法

### 快速上手

```ts
import { writeFileSync } from 'node:fs';
import { screenshot } from '@shotkit/shotium';

// 仅需传入 URL 或文件路径，其余参数均有默认值
const { image } = await screenshot({ file: 'https://example.com' });
writeFileSync('example.png', image!);
```

### 直接截图

在简单脚本或一次性任务中，直接调用 `screenshot()` 或 `screenshotTiles()` 即可；引擎在首次调用时以默认配置自动启动（包括启用位于 `~/.shotium/cache` 的持久化 HTTP 缓存与默认 User-Agent）：

```ts
import { screenshot } from '@shotkit/shotium';

const { image } = await screenshot({ file: 'https://example.com', fullPage: true, type: 'webp', quality: 85 });
```

### 长期运行服务里的生命周期管理

在常驻 Web/API 服务启动时显式初始化引擎，避免首个请求承受冷启动延迟；在业务低谷期主动释放内存；在服务下线时优雅关闭：

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

**单进程单例约束**：底层 Blink 依赖进程级全局状态，不支持二次初始化；当前进程内的所有 `screenshot()` 调用均共享同一引擎实例，并通过内部渲染工作队列串行执行；若需多核并发渲染，请启动多个独立的 Node.js 进程（例如使用 Cluster 模式或 PM2）

### 短生命周期进程的常驻守护进程

对于频繁短生命周期运行的 CLI 工具或 CI 任务，若每次进程执行都重新初始化 Blink 将带来显著冷启动开销；`daemon` 模块提供了常驻后台守护进程支持，在 Windows 平台通过命名管道（Named Pipe）、类 Unix 平台通过 Unix Domain Socket 进行 IPC 通信，首次调用时可自动拉起守护进程并按需复用：

```ts
import { daemon } from '@shotkit/shotium';

// 单次调用：自动连接（或拉起守护进程）、完成截图并断开
const { image } = await daemon.screenshot({
  file: 'https://example.com',
  viewport: { width: 1280, height: 720 },
  daemon: { name: 'cli-pool', idleTimeoutMs: 300_000 },
});

// 多次批量调用：复用长连接
const client = await daemon.connect({ name: 'batch' });
try {
  for (const url of urls) {
    const { image } = await client.screenshot({ file: url });
  }
} finally {
  client.close(); // 断开当前客户端连接，守护进程保持后台运行直至 idleTimeoutMs 超时
}

// 检查状态或显式停止
const status = await daemon.status({ name: 'batch' });
await daemon.stop({ name: 'batch' });
```

若启动配置 `StartOptions` 不同，将会对应不同的守护进程实例；若未显式指定 `name`，IPC 通信地址将根据 `cacheDir`、`userAgent`、`resourceDir` 自动计算哈希生成，以防客户端误连到配置不匹配的守护进程

## 常见实践

### 内存中的 HTML 渲染

引擎不支持将 `data:` 协议 URL 作为主文档；若需渲染动态生成的 HTML 字符串，可将其写入系统临时目录：

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

### 本地子资源访问

出于安全考虑，渲染本地 HTML 文件时默认禁止读取文件系统的其他子资源；若页面引用了相对路径的样式表、图片或本地字体，请设置 `allowFileAccess: true`，否则未授权的子资源请求将失败并记录至 `stats.failed`

```ts
const { stats } = await screenshot({ file: './templates/report.html', allowFileAccess: true });
if (stats.failed > 0) console.warn(`${stats.failed} subresource(s) failed`);
```

### 返回 Buffer 与直接写文件

未指定 `path` 参数时，编码后的图像数据将以 `image: Buffer` 形式直接返回；若指定了 `path` 参数，引擎将在内部通过原子重命名机制直接落盘保存文件，此时返回的 `image` 为 `null`

```ts
await screenshot({ file: 'https://example.com', path: './output.png' });
```

### 超长页面分片渲染

单张图像受编码格式限制（PNG 与 JPEG 单边最大 65,535 像素，WebP 为 16,383 像素）；`screenshotTiles()` 在单次文档解析与排版的基础上，将目标区域切分为高度不超过 `tile.height`（CSS 像素）的多个水平分片；若 `path` 包含 `{n}` 占位符，各分片将在编码完成后立即流式写入磁盘，峰值图像内存占用始终维持在单个分片量级：

```ts
import { screenshotTiles } from '@shotkit/shotium';

const { tiles } = await screenshotTiles({
  file: './long-article.html',
  allowFileAccess: true,
  fullPage: true,
  tile: { height: 8000 },
  path: 'article-{n}.png', // 输出 article-1.png、article-2.png ……
});
for (const tile of tiles) console.log(tile.path, tile.y, tile.height);
```

若未指定 `path`，则每个分片对象中各自携带独立的 `image: Buffer`

### HTTP 磁盘缓存管理

引擎集成 Chromium 的底层磁盘缓存，默认位于 `~/.shotium/cache/<project-hash>`，使用同一缓存目录的所有进程可共享缓存并在 `stop()` 后持续生效：

```ts
import { cache } from '@shotkit/shotium';

cache.getDir();                                  // 当前项目的缓存目录
cache.getDirs({ target: 'all' });                // ~/.shotium/cache 下的所有缓存目录
const entries = await cache.getFiles();          // 获取缓存条目列表 [{ url, lastUsedMs, bytes, dir }]
await cache.clear({ glob: ['https://cdn.example.com/**'] });
await cache.clear({ maxAge: 7 * 86400, maxSize: 100 * 1024 * 1024 });
```

单次截图可以通过 `cache: 'reload' | 'no-store' | 'only-if-cached'` 灵活覆盖缓存策略，同时对主文档及子资源生效

### 错误排查与诊断

截图失败时返回的 Promise 将会被拒绝（reject），错误对象中附带 `error.stats` 属性，记录截至失败发生时的详细诊断数据（例如 HTTP 状态码、已发出请求数、失败子资源数及各阶段耗时）：

```ts
try {
  await screenshot({ file: 'https://example.com/slow', pageGotoParams: { timeout: 5000 } });
} catch (error) {
  const { stats } = error as Error & { stats?: CaptureStats };
  if (stats) console.error(stats.httpStatus, stats.requests, stats.failed, stats.timing);
}
```

## API

### 模块导出

```ts
import shotium, {
  screenshot, screenshotTiles,          // 通过共享引擎截图
  start, status, stop, releaseMemory,   // 共享引擎的生命周期
  runtime, Runtime,                     // 共享实例及其类
  daemon,                               // 常驻引擎
  cache, Cache,                         // HTTP 缓存
} from '@shotkit/shotium';
```

默认导出是同一组函数组成的一个对象，外加一个实时的 `running` getter；下文所有类型同样导出：`ScreenshotOptions`、`ScreenshotTilesOptions`、`ScreenshotResult`、`ScreenshotTilesResult`、`ScreenshotTile`、`TileOptions`、`Viewport`、`Clip`、`PageGotoParams`、`CacheMode`、`CaptureStats`、`CaptureTiming`、`StartOptions`、`StartResult`、`ReleaseMemoryOptions`、`DaemonOptions`、`DaemonStatus`、`DaemonCapability`、`DaemonClient`、`CacheTarget`、`CacheEntry`、`CacheClearOptions`、`CacheClearResult`

### screenshot(options)

`screenshot(options: ScreenshotOptions): Promise<ScreenshotResult>`

使用共享引擎实例渲染截图；若引擎尚未启动则自动触发初始化；返回 Promise 解析为 `{ image, stats }`；若指定了 `path` 参数则 `image` 为 `null`；若传入选项校验失败，将在调用原生代码前抛出 `TypeError`；若渲染执行失败，将抛出包含 `stats` 诊断属性的 `Error`

### screenshotTiles(options)

`screenshotTiles(options: ScreenshotTilesOptions): Promise<ScreenshotTilesResult>`

将指定区域（支持 `fullPage`、`selector`、`clip` 或默认视口）自上而下切分为高度不超过 `tile.height` CSS 像素的多个水平分片进行渲染；返回 Promise 解析为 `{ tiles, stats }`；若指定了 `path` 参数，必须包含 `{n}` 占位符，其将被替换为从 1 开始递增的分片序号

### start(options)、status()、stop()、releaseMemory(options)

| 函数 | 返回值 | 说明 |
|---|---|---|
| `start(options?: StartOptions)` | `StartResult` | 初始化引擎实例；若引擎已在运行则安全复用；若传入选项与已初始化引擎配置冲突将抛出异常 |
| `status()` | `StartResult` | 查询当前引擎实例的运行状态及配置参数 |
| `stop()` | `Promise<void>` | 排空当前渲染队列，释放临时缓存内存并停止接收新请求；Blink 底层仍保持初始化状态，磁盘缓存予以保留；后续调用可直接复用该引擎实例 |
| `releaseMemory(options?: ReleaseMemoryOptions)` | `void` | 主动触发 Blink 垃圾回收、清理 Skia 绘制缓存及分配器空闲列表；指定 `releaseWorkingSet: true` 时将请求操作系统收缩物理工作集（不影响磁盘数据） |

默认导出对象上的 `shotium.running` 属性（或 `status().running`）指示当前引擎是否处于就绪运行状态

### Runtime

`new Runtime()` 允许调用方创建独立的生命周期控制器，其提供与模块顶级完全一致的方法及属性：`start`、`status`、`stop`、`releaseMemory`、`screenshot`、`screenshotTiles`、`running` getter 以及 `cache` 实例；需要说明的是，`Runtime` 负责的是生命周期状态管理而非新建引擎：进程内无论实例化多少个 Runtime，均共同管理并复用唯一的底层原生引擎；runtime 是本模块默认导出的共享实例

### daemon

| 函数 | 返回值 | 说明 |
|---|---|---|
| `daemon.connect(options?)` | `Promise<DaemonClient>` | 连接到守护进程；若无守护进程处于监听状态则自动拉起新实例（除非指定 `spawn: false`） |
| `daemon.screenshot(options)` | `Promise<ScreenshotResult>` | 通过守护进程渲染单张截图；`options` 包含常规 `ScreenshotOptions` 及可选的 `daemon: DaemonOptions` |
| `daemon.screenshotTiles(options)` | `Promise<ScreenshotTilesResult>` | 通过守护进程执行分片截图 |
| `daemon.start(options?)` | `Promise<DaemonStatus & { spawned: boolean }>` | 显式启动守护进程并返回其当前状态 |
| `daemon.status(options?)` | `Promise<Partial<DaemonStatus> & { running: boolean; endpoint: string }>` | 查询守护进程运行状态（若未运行不会自动拉起） |
| `daemon.stop(options?)` | `Promise<{ stopped: boolean; endpoint: string }>` | 请求守护进程安全退出 |

`connect()` 返回的 `DaemonClient` 实例成员：

| 成员 | 返回值 | 说明 |
|---|---|---|
| `screenshot(options)` | `Promise<ScreenshotResult>` | 通过当前长连接执行截图，返回结构与进程内调用一致 |
| `screenshotTiles(options)` | `Promise<ScreenshotTilesResult>` | 通过当前长连接执行分片截图 |
| `status()` | `Promise<DaemonStatus>` | 查询当前连接的守护进程状态 |
| `shutdown()` | `Promise<{ ok: boolean }>` | 请求远程守护进程退出 |
| `close()` | `void` | 关闭当前客户端连接，守护进程保持后台运行 |
| `endpoint`、`closed` | `string`、`boolean` | 当前连接的 IPC 地址及连接关闭状态 |

守护进程共享全局缓存，其缓存目录可通过 `daemon.status()` 获取，并调用 `cache.clear({ target: <dir> })` 进行清理

### cache

| 函数 | 返回值 | 说明 |
|---|---|---|
| `cache.getDir(options?: CacheTarget)` | `string` | 获取目标对应的 HTTP 磁盘缓存目录路径 |
| `cache.getDirs(options?: CacheTarget)` | `string[]` | 获取匹配的所有缓存目录列表 |
| `cache.getFiles(options?: CacheTarget)` | `Promise<CacheEntry[]>` | 遍历缓存条目列表，按资源 URL 及其元数据呈现 |
| `cache.clear(options?: CacheClearOptions)` | `Promise<CacheClearResult[]>` | 按 URL glob 通配符、最大年龄或容量限制清理缓存条目 |

缓存管理 API 独立于引擎生命周期，无需引擎处于运行状态即可直接执行读写与清理

### ScreenshotOptions

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `file` | `string` | 必填 | `http:` / `https:` / `file:` URL，或本地 HTML 文件绝对/相对路径 |
| `viewport` | `{ width?, height? }` | `1280 × 720` | 排版视口尺寸（CSS 像素） |
| `type` | `'png' \| 'jpeg' \| 'webp'` | `'png'` | 输出图像编码格式 |
| `quality` | `number` (1-100) | `90` | 图像编码质量（仅适用于 `jpeg` 与 `webp`） |
| `scale` | `number` (0.01-8) | `1` | 设备像素比（Device Scale Factor / DPR） |
| `fullPage` | `boolean` | `false` | 是否渲染整个文档完整内容而非仅视口区域 |
| `selector` | `string` | 无 | 截取匹配指定 CSS 选择器的首个元素包围盒（通过内部 DOM 解析，不注入任何脚本） |
| `clip` | `{ x, y, width, height }` | 无 | 指定裁切矩形区域（CSS 像素） |
| `omitBackground` | `boolean` | `false` | 是否保留透明背景通道（设置为 `true` 时不绘制默认白色底色；`jpeg` 格式不支持） |
| `path` | `string` | 无 | 输出文件路径；若指定则直接落盘写入，返回的 `image` 为 `null` |
| `pageGotoParams.timeout` | `number` (毫秒) | `30000` | 页面资源加载与排版等待超时时间 |
| `pageGotoParams.waitUntil` | `'load' \| 'networkidle'` | `'load'` | 页面就绪判断策略：`networkidle` 会在网络静默 500ms 后再确认完成 |
| `allowFileAccess` | `boolean` | `false` | 是否允许 HTML 加载本地 `file:` 协议子资源（样式、图片、字体） |
| `cache` | `CacheMode` | `'default'` | HTTP 缓存策略：`default`（标准缓存）、`reload`（强制刷新）、`no-store`（不使用缓存）、`only-if-cached`（仅读取缓存） |
| `headers` | `Record<string, string>` | 无 | 随主文档及同源子资源发送的自定义 HTTP 请求头（不会泄露给第三方跨域资源） |

`fullPage`、`selector`、`clip` 三者互斥；传入未知选项字段将被严格拦截校验，而非静默忽略

`ScreenshotTilesOptions` 继承自 `ScreenshotOptions`，新增 `tile: { height: number }` 配置（单个分片高度最大 32,000 CSS 像素）

### StartOptions

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `cacheDir` | `string \| null` | `~/.shotium/cache/<project-hash>` | HTTP 磁盘缓存根路径；传入 `null` 完全禁用磁盘缓存 |
| `cacheMaxBytes` | `number` | 256 MB | 缓存目录容量上限；传入 `0` 表示根据磁盘剩余空间自动估算 |
| `userAgent` | `string` | 内置默认值 | 自定义全局 HTTP `User-Agent` 请求头 |
| `resourceDir` | `string` | 引擎二进制所在目录 | 指定 `shotium_data.pak` 与 `shotium_strings.pak` 所在的资源目录 |

### DaemonOptions

`DaemonOptions` 继承自 `StartOptions`，新增守护进程专用配置：

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `name` | `string` | 无 | 指定守护进程唯一名称标识（替代基于配置哈希的寻址） |
| `endpoint` | `string` | 自动推导 | 显式指定 IPC 管道或 Unix Socket 路径 |
| `idleTimeoutMs` | `number` | `300000` | 空闲退出超时时间（毫秒）；无连接且无任务持续达此时间后自动退出；`0` 表示永不退出 |
| `prewarm` | `boolean` | `true` | 是否在启动后立即渲染一次测试文档，避免首个真实请求承受引擎冷启动损耗 |
| `spawn` | `boolean` | `true` | 当守护进程未运行时是否自动拉起新进程；为 `false` 时若未运行则直接报错 |
| `logFile` | `string` | `$SHOTIUM_DAEMON_LOG` | 守护进程后台运行日志输出文件路径 |
| `startTimeoutMs` | `number` | `20000` | 等待守护进程启动及 IPC 握手应答的最大超时时间（毫秒） |

### 结果与统计

- `ScreenshotResult` 结构为 `{ image: Buffer | null, stats: CaptureStats }`
- `ScreenshotTilesResult` 结构为 `{ tiles: ScreenshotTile[], stats: CaptureStats }`，其中每个分片为 `{ image: Buffer | null, x, y, width, height, path? }`（坐标单位为 CSS 像素）


`CaptureStats` 诊断统计字段：

| 字段 | 说明 |
|---|---|
| `requests` | 当前文档加载过程中发起的请求总数（包含主文档自身） |
| `fromCache` | 命中 HTTP 磁盘缓存的请求数（包含经 304 校验命中的资源） |
| `failed` | 加载失败的子资源请求数 |
| `bytes` | 解码后的响应正文字节总和 |
| `httpStatus` | 主文档 HTTP 状态码（本地 `file:` 协议为 `0`） |
| `finalUrl` | 最终重定向后的实际文档 URL |
| `timing.fetch` | 获取主文档网络耗时（毫秒，冷启动下包含 DNS 解析、TCP 握手与 TLS 协商） |
| `timing.render` | 文档解析、样式计算、排版及绘制总耗时（毫秒） |
| `timing.setup`、`wait`、`lifecycle`、`paint`、`raster`、`encode` | 渲染流程内部各细分阶段精确耗时（毫秒） |
| `timing.total` | 截图任务从接收到完成的完整墙钟耗时（毫秒） |

`StartResult` 字段：

| 字段 | 说明 |
|---|---|
| `running` | 当前生命周期控制器是否已就绪 |
| `cacheDir` | 当前生效的磁盘缓存目录绝对路径（禁用时为 `null`） |
| `enginePath` | 当前加载的原生引擎所在目录路径 |
| `cacheActive` | 磁盘缓存是否处于正常可用写入状态 |

`DaemonStatus` 提供守护进程的运行状态详情，包括 `pid`、`endpoint`、`cacheDir`、`warm`（是否已完成预热）、`uptimeMs`、`connections`（当前连接数）、`inFlight`（进行中任务数）、`served`（已服务任务累计数）、`idleTimeoutMs`、`version` 及 `capabilities`（支持的能力列表）

### 错误处理

| 触发情形 | 抛出类型 | 说明 |
|---|---|---|
| 选项参数校验失败（如缺少 `file`、未知字段、参数冲突等） | `TypeError` | 在进入原生代码前直接于 JS 层抛出 |
| 渲染执行失败（如文档不可达、CSS 选择器未命中、超时等） | `Error` | 错误对象包含 `error.stats` 属性，记录详细诊断数据 |
| `start()` 选项与当前运行中的引擎实例配置冲突 | `Error` | 单进程内无法以不同配置二次初始化已运行的引擎 |
| 守护进程连接中断或协议握手失败 | `Error` | IPC 通信异常 |

### 环境变量

| 环境变量 | 作用说明 |
|---|---|
| `SHOTIUM_ENDPOINT` | 覆盖当前进程内客户端连接守护进程的默认 IPC 地址 |
| `SHOTIUM_DAEMON_LOG` | 自动拉起守护进程时的默认日志文件路径 |

## 类型定义

发布的 `dist/index.d.ts` 由随包分发的 `src/types.ts` 生成，核心接口结构如下：

```ts
// ==================== 1. 截图与视口配置 ====================

export interface Viewport {
  /** 视口宽度（CSS 像素），默认 1280 */
  width?: number;
  /** 视口高度（CSS 像素），默认 720 */
  height?: number;
}

export interface Clip {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface PageGotoParams {
  /** 加载超时时间（毫秒），默认 30000 */
  timeout?: number;
  /**
   * 页面就绪策略：
   * - 'load': 等待 DOM 解析完成与 load 事件触发
   * - 'networkidle': 额外等待 500ms 内部网络空闲
   */
  waitUntil?: 'load' | 'networkidle';
}

/** HTTP 缓存策略，遵循 Fetch 语义 */
export type CacheMode = 'default' | 'reload' | 'no-store' | 'only-if-cached';

export interface ScreenshotOptions {
  /** 目标 URL（http/https/file）或本地 HTML 文件路径 */
  file: string;
  /** 输出图像格式，默认 'png' */
  type?: 'png' | 'jpeg' | 'webp';
  /** 是否截取整个文档内容 */
  fullPage?: boolean;
  /** 截取匹配指定 CSS 选择器的首个元素（内部 DOM 解析，无 JS 注入） */
  selector?: string;
  /** 图像质量（1-100，仅 jpeg 与 webp），默认 90 */
  quality?: number;
  /** 设备像素比 DPR（0.01-8），默认 1 */
  scale?: number;
  /** 是否保留透明背景（不绘制默认白色背景，jpeg 不支持） */
  omitBackground?: boolean;
  /** 直接写入磁盘文件路径（指定后 image 返回 null） */
  path?: string;
  /** 页面导航与等待选项 */
  pageGotoParams?: PageGotoParams;
  /** 指定矩形裁切区域（CSS 像素） */
  clip?: Clip;
  /** 页面排版视口尺寸 */
  viewport?: Viewport;
  /** 是否允许加载本地 file: 协议子资源，默认 false */
  allowFileAccess?: boolean;
  /** HTTP 缓存控制策略，默认 'default' */
  cache?: CacheMode;
  /** 附加 HTTP 请求头（仅随主文档及同源子资源发送） */
  headers?: Record<string, string>;
}

export interface TileOptions {
  /** 单个分片最大高度（CSS 像素，上限 32000） */
  height: number;
}

export interface ScreenshotTilesOptions extends ScreenshotOptions {
  /** 分片切割配置 */
  tile: TileOptions;
}

export interface ScreenshotTile {
  /** 编码后图像 Buffer；若指定 path 落盘则为 null */
  image: Buffer | null;
  /** 分片在文档中的绝对坐标（CSS 像素） */
  x: number;
  y: number;
  width: number;
  height: number;
  /** 写入的目标文件路径（若指定 path 参数） */
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

// ==================== 2. 性能诊断与指标统计 ====================

export interface CaptureTiming {
  /** 主文档获取耗时（冷启动包含 DNS/TCP/TLS，毫秒） */
  fetch: number;
  /** DOM 解析、样式计算与排版总耗时（毫秒） */
  render: number;
  /** 页面/框架创建与初始环境准备耗时（毫秒） */
  setup: number;
  /** 等待子资源加载完成耗时（毫秒） */
  wait: number;
  /** 选择器匹配与生命周期状态推进耗时（毫秒） */
  lifecycle: number;
  /** 提取 Blink 绘制记录耗时（毫秒） */
  paint: number;
  /** 栅格化回放与像素光栅化耗时（毫秒） */
  raster: number;
  /** 图像格式编码耗时（毫秒） */
  encode: number;
  /** 单次截图全流程总墙钟耗时（毫秒） */
  total: number;
}

export interface CaptureStats {
  /** 本次渲染发起的全部网络请求总数（含主文档） */
  requests: number;
  /** 从 HTTP 磁盘缓存命中的请求数（含 304 校验命中） */
  fromCache: number;
  /** 失败的子资源请求总数 */
  failed: number;
  /** 解码后响应正文字节总和（非传输压缩体积） */
  bytes: number;
  /** 主文档 HTTP 状态码（本地 file: 协议为 0） */
  httpStatus: number;
  /** 最终重定向后的实际文档 URL */
  finalUrl: string;
  /** 细分阶段耗时统计 */
  timing: CaptureTiming;
}

// ==================== 3. 引擎生命周期管理 ====================

export interface StartOptions {
  /** HTTP 磁盘缓存目录；传入 null 禁用缓存 */
  cacheDir?: string | null;
  /** 缓存目录容量上限（字节），默认 256 MB，0 表示自动估算 */
  cacheMaxBytes?: number;
  /** 自定义全局 User-Agent 请求头 */
  userAgent?: string;
  /** 原生资源包 (shotium_data.pak) 所在目录 */
  resourceDir?: string;
}

export interface StartResult {
  /** 当前引擎生命周期是否已启动 */
  running: boolean;
  /** 生效中的磁盘缓存目录路径 */
  cacheDir: string | null;
  /** 当前加载的原生引擎所在路径 */
  enginePath: string | null;
  /** 磁盘缓存是否可正常写入生效 */
  cacheActive: boolean;
}

export interface ReleaseMemoryOptions {
  /** 是否同时请求操作系统收缩物理工作集（释放物理内存） */
  releaseWorkingSet?: boolean;
}

// ==================== 4. 常驻守护进程 (Daemon) ====================

export interface DaemonOptions extends StartOptions {
  /** 守护进程命名标识（替代基于配置哈希的寻址） */
  name?: string;
  /** 显式指定 IPC 管道或 Socket 路径 */
  endpoint?: string;
  /** 空闲退出超时时间（毫秒），默认 300000（5分钟），0 表示永不退出 */
  idleTimeoutMs?: number;
  /** 启动后是否预热渲染一次测试页面以消除冷启动延迟，默认 true */
  prewarm?: boolean;
  /** 未运行时是否自动拉起新守护进程，默认 true */
  spawn?: boolean;
  /** 后台守护进程诊断日志文件路径 */
  logFile?: string;
  /** 等待守护进程启动绑定的最大超时时间（毫秒） */
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
  /** 引擎是否已完成至少一次渲染预热 */
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

// ==================== 5. HTTP 磁盘缓存 ====================

export interface CacheTarget {
  /** 目标目录：'current'（当前项目）、'all'（全部）或指定目录路径 */
  target?: 'current' | 'all' | (string & {});
}

export interface CacheEntry {
  /** 缓存资源原始 URL */
  url: string;
  /** 最近访问时间戳（Unix 毫秒） */
  lastUsedMs: number;
  /** 资源大小（字节） */
  bytes: number;
  /** 所属缓存目录 */
  dir: string;
}

export interface CacheClearOptions extends CacheTarget {
  /** 匹配 URL 的 glob 通配符（支持 *、**、?、{a,b}） */
  glob?: string[];
  /** 清理超过指定秒数未被访问的条目，0 表示不限 */
  maxAge?: number;
  /** 按 LRU 淘汰条目直至目录小于指定字节数，0 表示不限 */
  maxSize?: number;
}

export interface CacheClearResult {
  /** 清理的条目数（-1 表示整目录秒删） */
  removed: number;
  bytesBefore: number;
  bytesAfter: number;
  dir: string;
}
```

## 限制与边界

- **无 JavaScript 执行**：构建剔除 V8 引擎，不执行 `<script>` 脚本，输入须为已排版就绪的静态或 SSR HTML 内容
- **不支持 `data:` 协议主文档**：主文档须以文件路径、stdin 管道或 HTTP(S) URL 传入
- **单进程单一引擎实例**：底层 Blink 依赖进程级状态，不支持进程内并发重复初始化；高并发场景请采用多进程或多守护进程架构
- **排版绘制上限**：Blink 单视口最大绘制坐标为 32,767 CSS 像素，超长页面通过内部滚动分段机制绘制（`fullPage` 与 `screenshotTiles()` 已自动封装）；单图编码受格式规格限制（PNG/JPEG 单边最大 65,535 像素，WebP 为 16,383 像素）

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](https://github.com/sj817/shotium/blob/main/LICENSE)

