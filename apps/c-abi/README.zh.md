# shotium C ABI

[English](./README.md) · 简体中文

基于 Chromium 静态渲染架构的跨平台原生 C 动态库，统一通过标准 C 头文件 `shot_api.h` 提供底层调用支持

[![C ABI version](https://img.shields.io/badge/C%20ABI-v3-blue.svg?logo=c&logoColor=white)](../../shot/shot_api.h) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

shotium 为所有编程语言提供跨平台的原生 C 动态库：`shotium.dll`（Windows）、`libshotium.so`（Linux）或 `libshotium.dylib`（macOS），统一通过标准 C 头文件 `shot_api.h` 进行调用

本文档随各平台 C ABI 发布包（作为 `C_ABI.zh.md`）及各语言示例包分发，假设读者已解压当前运行环境对应的发布文件；内容涵盖发布包目录结构、生命周期约束、内存所有权隔离原则、状态码定义及 JSON 协议规范；接口签名及数据结构请以 `shot_api.h` 头文件为最终准则

## 目录

| 导航分类 | 核心内容与快速跳转 |
|:---|:---|
| **工程架构** | [压缩包布局](#压缩包布局) · [生命周期约束](#生命周期) · [缓冲区与所有权](#缓冲区与所有权) · [状态码](#状态码) |
| **接口与配置** | [C API 声明](#c-api-核心声明) · [引擎选项](#引擎选项) · [截图请求](#截图请求) · [截图统计](#截图统计) · [分片渲染](#分片渲染) |
| **系统控制** | [状态与内存管理](#状态与内存管理) · [HTTP 缓存管理](#http-缓存管理) · [最小 C 程序](#最小-c-程序) · [多语言示例](#多语言示例) · [许可证](#许可证) |

## 压缩包布局

每个版本按平台发布独立压缩包 `shotium-c-abi-<os>-<arch>.7z`（`<os>` 为 `windows`、`linux` 或 `macos`；`<arch>` 为 `amd64` 或 `arm64`），选择时请务必对应**运行宿主进程的 CPU 架构**，而非仅看操作系统架构；解压后包含以下文件：

| 文件 | 说明 |
|---|---|
| `libshotium.so` / `libshotium.dylib` / `shotium.dll` | C ABI 原生动态库 |
| `shotium.dll.lib` | （仅 Windows）供编译期链接动态库使用；动态加载方案无需此文件 |
| `shot_api.h` | C 接口头文件 |
| `shotium_data.pak`、`shotium_strings.pak` | 引擎初始化所需的资源包 |
| `C_ABI.md`、`C_ABI.zh.md` | C ABI 使用规范说明文档 |
| `LICENSE` | BSD-3-Clause 开源协议文件 |

动态库与两个 `.pak` 资源文件必须保持在同一目录下且版本严格匹配；创建引擎时需将该所在目录作为 `resourceDir` 参数传入；动态库自身无法可靠隐式定位资源包（例如在 Linux 上动态模块解析可能回退至主可执行文件路径），因此必须由调用方显式指定

Linux 版本依赖 glibc 运行时，不支持 musl libc；页面文字渲染依赖系统字体，在极简容器镜像中请确保安装基础字体包（如 Fontconfig 与常用 TrueType 字体）

包内顶层目录为 `shotium-c-abi-<平台>/`（解压后可直接置于工程 `native/` 目录下）：

```bash
# Linux / macOS（以 linux-amd64 为例，需要 7z 或 7zz）
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS

# 校验并解压至 native/ 目录
sha256sum --check --ignore-missing SHA256SUMS
7z x shotium-c-abi-linux-amd64.7z -onative
```

```powershell
# Windows PowerShell（以 windows-amd64 为例）
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS

# 校验并解压至 native/ 目录
$expected = (Get-Content SHA256SUMS | Select-String "shotium-c-abi-windows-amd64.7z").Line.Split(" ")[0]
if ((Get-FileHash shotium-c-abi-windows-amd64.7z -Algorithm SHA256).Hash.ToLower() -ne $expected) { throw "SHA256 校验不匹配" }
7z x shotium-c-abi-windows-amd64.7z -onative
```


## 生命周期

1. 加载原生动态库，并在宿主进程生命周期内持续保持映射；在引擎对象或工作线程尚未完全释放前切勿卸载动态库
2. 调用 `shot_abi_version()`，核对当前头文件定义的 `SHOT_ABI_VERSION`（当前版本为 **3**）；若不匹配应立即终止执行
3. 调用 `shot_engine_create(options_json, &engine, &error)` 初始化引擎，该函数会同步阻塞直至 Blink 准备就绪
4. 在需要时调用 `shot_engine_capture()` 或 `shot_engine_capture_tiles()`，调用将同步阻塞直至图像编码完成
5. 当进程退出或不再需要截图时，调用 `shot_engine_destroy(engine)`，等待进行中的任务完成并安全释放引擎工作线程

由于底层 Blink 依赖进程级全局状态，调用方必须遵循以下架构规则：

- **单进程单一引擎实例**：整个进程生命周期中仅能创建一次引擎实例，一旦销毁即无法再次创建；若尝试发起第二次 `shot_engine_create()` 调用，无论前一实例是否已被销毁，均返回 `SHOT_ERR_STATE` 错误；进程内所有截图任务共享同一引擎实例，若需并发多核渲染，请在系统层部署多进程架构
- **线程安全与调用串行化**：`shot_engine_capture()` 支持跨线程并发调用，所有截图任务均由内部工作队列排队串行分发给引擎线程执行；对于具有主事件循环的宿主语言运行时，建议在独立工作线程中发起调用以避免阻塞事件循环
- **静态渲染与协议约束**：渲染文档不执行任何 JavaScript；主文档不支持 `data:` 协议，请将其保存为本地文件或通过 HTTP(S) URL 访问

## 缓冲区与所有权

C ABI 动态库返回的所有动态数据均封装为 `shot_buffer*` 结构（包含图像字节流、统计 JSON、错误信息文本及状态配置）；调用方通过 `shot_buffer_data()` 读取数据指针，通过 `shot_buffer_size()` 获取有效字节长度，并在使用完毕后调用 `shot_buffer_free()` 进行释放

严禁将缓冲区指针传递给宿主环境的 `free()`、GC 垃圾回收器或宿主语言运行时的分配器：两者使用相互独立的内存堆空间，跨边界释放会导致严重的内存损坏或崩溃问题；调用 `shot_buffer_free(NULL)` 是安全的空操作

- 错误信息与 JSON 字符串缓冲区末尾保证以 `\0`（NUL）结尾，可作为标准 C 字符串读取
- 图像数据缓冲区为纯二进制数据，**不**以 NUL 结尾，必须通过 `shot_buffer_size()` 获取准确长度
- 所有 ABI 函数在执行前均会自动清空所有传出指针参数，因此外部变量可在多次调用间复用而无需提前手动清零
- `shot_engine_capture()` 的 `*out_image` 与 `*out_error` 保证严格互斥：成功时返回图像数据，失败时返回错误信息指针；若不需要统计数据，`out_stats` 参数可传入 `NULL`
- 若请求 JSON 中指定了 `path` 输出文件，引擎将在内部直接写盘，返回的图片缓冲区为空，但仍需调用 `shot_buffer_free()` 释放
- 无论渲染成功还是失败，均会返回诊断统计对象，调用方应及时读取并调用 `shot_buffer_free()` 释放

## 状态码

| 数值 | 常量名 | 说明 |
|---|---|---|
| `0` | `SHOT_OK` | 截图完成或引擎初始化成功 |
| `1` | `SHOT_ERR_USAGE` | 请求 JSON 格式错误、类型不符或缺少必填字段（错误信息包含具体字段名） |
| `2` | `SHOT_ERR_CAPTURE` | 请求参数有效但渲染执行失败（如文档无法加载、CSS 选择器未匹配、编码失败或超时） |
| `3` | `SHOT_ERR_STATE` | 当前调用时机不允许（如试图在单进程内创建第二个引擎实例，或在未成功初始化的引擎上调用截图） |

在读取或处理图片数据前，请务必先校验返回的状态码

## C API 核心声明

`shot_api.h` 导出的核心接口类型与函数原型声明如下：

```c
#define SHOT_ABI_VERSION 3

// 不透明句柄类型
typedef struct shot_engine shot_engine;
typedef struct shot_buffer shot_buffer;
typedef struct shot_tile_list shot_tile_list;

// 状态码枚举
typedef enum shot_status {
  SHOT_OK = 0,
  SHOT_ERR_USAGE = 1,
  SHOT_ERR_CAPTURE = 2,
  SHOT_ERR_STATE = 3,
} shot_status;

// 核心生命周期与截图
int32_t shot_abi_version(void);
shot_status shot_engine_create(const char* options_json, shot_engine** out_engine, shot_buffer** out_error);
shot_status shot_engine_capture(shot_engine* engine, const char* request_json, shot_buffer** out_image, shot_buffer** out_stats, shot_buffer** out_error);
shot_status shot_engine_capture_tiles(shot_engine* engine, const char* request_json, shot_tile_list** out_tiles, shot_buffer** out_stats, shot_buffer** out_error);
void shot_engine_destroy(shot_engine* engine);

// 缓冲区读取与释放
const void* shot_buffer_data(const shot_buffer* buffer);
size_t shot_buffer_size(const shot_buffer* buffer);
void shot_buffer_free(shot_buffer* buffer);

// 状态查询与内存回收
shot_status shot_engine_status(shot_engine* engine, shot_buffer** out_json, shot_buffer** out_error);
void shot_engine_purge(shot_engine* engine, int release_working_set);

// 超长页面分片列表操作
size_t shot_tile_list_count(const shot_tile_list* tiles);
void shot_tile_list_region(const shot_tile_list* tiles, size_t index, int* x, int* y, int* w, int* h);
const char* shot_tile_list_path(const shot_tile_list* tiles, size_t index);
shot_buffer* shot_tile_list_take_image(shot_tile_list* tiles, size_t index);
void shot_tile_list_free(shot_tile_list* tiles);

// HTTP 磁盘缓存管理
shot_status shot_cache_list(shot_engine* engine, shot_buffer** out_json, shot_buffer** out_error);
shot_status shot_cache_clear(shot_engine* engine, const char* clear_json, shot_buffer** out_json, shot_buffer** out_error);
```

## 引擎选项

`shot_engine_create()` 接收一个配置 JSON 对象，所有字段均为可选：

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `resourceDir` | string | 可执行文件目录 | `shotium_data.pak` 与 `shotium_strings.pak` 所在路径，请传入解压目录 |
| `cacheDir` | string | 无 | HTTP 磁盘缓存目录；省略时禁用缓存（渲染网络 URL 每次需重复进行网络协商） |
| `cacheMaxBytes` | integer | `0` | 缓存容量上限字节数；`0` 表示由存储后端根据可用磁盘空间动态决定 |
| `userAgent` | string | 内置默认值 | 自定义全局 HTTP `User-Agent` 请求头 |
| `allowFileAccess` | boolean | `false` | 全局是否默认允许加载本地 `file:` 子资源（单次请求可覆盖此配置） |

```json
{"resourceDir": "/opt/shotium/native/shotium-c-abi-linux-amd64", "cacheDir": "/var/cache/shotium", "cacheMaxBytes": 268435456}
```

## 截图请求

`shot_engine_capture()` 接收一个请求 JSON 对象；字段命名与 Node.js SDK 的 `ScreenshotOptions` 保持一致，唯一区别在于视口参数采用扁平化设计（`width` 与 `height` 直接作为顶层字段）：

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `file` | string | 必填 | `http:` / `https:` / `file:` URL，或本地文件绝对/相对路径 |
| `width`、`height` | integer | `1280`、`720` | 排版视口尺寸（CSS 像素） |
| `type` | `"png"` / `"jpeg"` / `"webp"` | `"png"` | 输出图像编码格式 |
| `quality` | integer (1-100) | `90` | 图像编码质量（仅适用于 `jpeg` 与 `webp`） |
| `scale` | number (0.01-8) | `1` | 设备像素比（DPR） |
| `fullPage` | boolean | `false` | 是否截取整个文档完整内容而非仅当前视口 |
| `selector` | string | 无 | 截取匹配指定 CSS 选择器的首个元素包围盒 |
| `clip` | `{x, y, width, height}` | 无 | 指定裁切矩形区域（CSS 像素） |
| `omitBackground` | boolean | `false` | 是否保留透明背景（`jpeg` 格式不支持） |
| `path` | string | 无 | 直接写出文件路径（指定时内存图片返回为空） |
| `timeout` | integer (毫秒) | `30000` | 页面资源加载与排版等待超时时间 |
| `waitUntil` | `"load"` / `"networkidle"` | `"load"` | 就绪判定策略（`networkidle` 额外等待 500ms 网络静默） |
| `cache` | `"default"` / `"reload"` / `"no-store"` / `"only-if-cached"` | `"default"` | HTTP 缓存控制模式 |
| `headers` | 字符串对象 | 无 | 随主文档及同源子资源发送的自定义 HTTP 请求头 |
| `allowFileAccess` | boolean | 引擎默认值 | 是否允许加载本地 `file:` 协议子资源 |
| `tile` | `{height}` | 无 | 分片高度配置（仅用于分片截图） |

`fullPage`、`selector`、`clip` 三者互斥；`file` 中的相对路径按宿主进程当前工作目录解析，建议传入绝对路径

```json
{"file": "/abs/card.html", "allowFileAccess": true, "width": 720, "height": 380, "type": "png"}
```

## 截图统计

`out_stats` 输出一个 JSON 对象，各时间字段单位为毫秒（浮点数）：

| 字段 | 说明 |
|---|---|
| `requests` | 文档加载过程中发起的请求总数（包含主文档自身） |
| `fromCache` | 命中 HTTP 磁盘缓存的请求数（包含 304 再验证） |
| `failed` | 失败的子资源请求数 |
| `bytes` | 解码后的响应正文字节总和 |
| `httpStatus` | 主文档 HTTP 状态码（本地 `file:` 协议为 `0`） |
| `finalUrl` | 最终重定向后的实际文档 URL |
| `timing.fetch` | 获取主文档网络耗时（冷启动下包含 DNS 解析、TCP 握手及 TLS 协商） |
| `timing.render` | 文档解析、样式计算、排版及绘制总耗时 |
| `timing.setup`、`wait`、`lifecycle`、`paint`、`raster`、`encode` | 渲染流程内部各细分阶段精确耗时 |
| `timing.total` | 截图任务从接收到完成的完整墙钟耗时 |

## 分片渲染

`shot_engine_capture_tiles()` 接收相同的请求参数并要求提供 `tile` 配置，例如 `{"file": "...", "fullPage": true, "tile": {"height": 8000}}`；目标区域将在单次文档加载与排版的基础上，被自上而下切分为高度不超过 `tile.height` CSS 像素的多个水平分片进行独立渲染与编码；单片最大高度为 32,000 CSS 像素

当页面总高度超过单个图像格式规范上限时（PNG 与 JPEG 单边最大 65,535 像素，WebP 为 16,383 像素），可通过此分片接口流式获取长图数据

返回结果封装在 `shot_tile_list*` 句柄中：

| 函数 | 说明 |
|---|---|
| `shot_tile_list_count(tiles)` | 获取总分片数量 |
| `shot_tile_list_region(tiles, i, &x, &y, &w, &h)` | 获取第 `i` 个分片在文档中的相对坐标与尺寸（CSS 像素） |
| `shot_tile_list_path(tiles, i)` | 获取指定 `path` 时第 `i` 个分片写盘的文件路径（未指定时返回 `NULL`） |
| `shot_tile_list_take_image(tiles, i)` | 从列表中提取第 `i` 个分片的图像缓冲区（提取后由调用方负责释放） |
| `shot_tile_list_free(tiles)` | 释放分片列表及所有未被提取的图像缓冲区 |

若使用 `path` 参数写盘，字符串必须包含 `{n}` 占位符，其将被替换为从 1 开始的分片序号；`*out_tiles` 与 `*out_error` 严格互斥

## 状态与内存管理

- `shot_engine_status(engine, &json, &error)` 查询引擎当前状态，返回类似 `{"cacheDir": "..." | null, "cacheActive": true}`；若配置了缓存目录但 `cacheActive` 为 `false`，表明指定目录无法写入或打开失败，此时引擎将自动降级为无缓存模式继续运行（支持多进程共享同一缓存目录）
- `shot_engine_purge(engine, release_working_set)` 主动释放 Blink 堆内存、清理 Skia 绘制缓存及分配器空闲列表；当 `release_working_set` 为非零时，将请求操作系统回收进程物理工作集内存；建议在一批密集渲染任务完成后调用，避免在单次任务间频繁触发

## HTTP 缓存管理

`shot_cache_list()` 与 `shot_cache_clear()` 提供独立的缓存管理能力；若进程内已创建引擎，必须传入该引擎句柄；仅当进程内从未初始化过引擎时，方可传入 `NULL`

清理参数 JSON：

| 字段 | 适用操作 | 说明 |
|---|---|---|
| `cacheDir` | 全部 | 目标缓存目录路径（必填） |
| `urls` | clear | 需要精确匹配删除的资源 URL 列表 |
| `unusedSinceMs` | clear | 清理指定 Unix 毫秒时间戳之后未被使用的缓存条目 |
| `maxBytes` | clear | 按 LRU 策略淘汰旧条目，直至目录总容量低于该阈值 |

上述清理条件支持组合使用；若均未提供，将清空整个缓存目录；C ABI 本身不提供通配符过滤逻辑，调用方可通过 `shot_cache_list()` 遍历条目，在应用层使用正则或字符串匹配后将目标 URL 列表传入清理

返回值说明：`shot_cache_list()` 返回 `{url, lastUsedMs, bytes}` 数组；`shot_cache_clear()` 返回 `{removed, bytesBefore, bytesAfter}`（整目录删除时 `removed` 记为 `-1`）

## 最小 C 程序

```c
#include <stdio.h>
#include "shot_api.h"

int main(void) {
  if (shot_abi_version() != SHOT_ABI_VERSION) return 1;
  shot_engine* engine = NULL;
  shot_buffer* error = NULL;
  if (shot_engine_create("{\"resourceDir\":\"/opt/shotium\"}", &engine, &error) != SHOT_OK) {
    fprintf(stderr, "%s\n", (const char*)shot_buffer_data(error));
    shot_buffer_free(error);
    return 1;
  }
  shot_buffer* png = NULL;
  shot_buffer* stats = NULL;
  shot_status status = shot_engine_capture(
      engine, "{\"file\":\"https://example.com\",\"width\":720,\"height\":380}", &png, &stats, &error);
  if (status == SHOT_OK) {
    FILE* out = fopen("example.png", "wb");
    fwrite(shot_buffer_data(png), 1, shot_buffer_size(png), out);
    fclose(out);
  } else {
    fprintf(stderr, "capture failed (%d): %s\n", status, (const char*)shot_buffer_data(error));
  }
  if (stats) printf("%s\n", (const char*)shot_buffer_data(stats));
  shot_buffer_free(png);
  shot_buffer_free(stats);
  shot_buffer_free(error);
  shot_engine_destroy(engine);
  return status == SHOT_OK ? 0 : 1;
}
```

## 多语言示例

五个可运行示例分别通过各语言的通用 FFI 加载原生动态库，无需额外中间封装包：[Go](https://github.com/sj817/shotium/blob/main/apps/go/README.zh.md)（purego）、[Python](https://github.com/sj817/shotium/blob/main/apps/python/README.zh.md)（ctypes）、[Rust](https://github.com/sj817/shotium/blob/main/apps/rust/README.zh.md)（libloading）、[C#](https://github.com/sj817/shotium/blob/main/apps/csharp/README.zh.md)（P/Invoke）、[Java](https://github.com/sj817/shotium/blob/main/apps/java/README.zh.md)（JNA）；每个示例在发布页各提供独立 7z 压缩包，均以 720×380 规格渲染同一个 `card.html`，输出与 CLI 命令行工具逐字节一致

示例附件统一命名为 `shotium-example-<语言>.7z`（`go`、`python`、`rust`、`csharp`、`java`），包含源码及版本、提交、文件哈希清单，不内置原生库；另下载同一 Release 的 C ABI 包，并使用统一的 `SHA256SUMS` 校验

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)
