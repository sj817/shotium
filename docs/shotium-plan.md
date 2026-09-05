# shotium — 现阶段任务计划

裁剪阶段已经收尾(见 `docs/cut-progress.md` §17–19):从零构建通过,渲染逐字节可复现,
分发集 44.81 MB / 压缩后 14.08 MB。**这一阶段的目标从"砍小"变成"能用"。**

> **状态:第 0–3 组全部完成**(落地记录见 `docs/cut-progress.md` §20)。
> 分发集 **57.71 MB / 压缩后 16.69 MB**,`render_corpus.html` 的 SHA-256 仍是
> `b42f1efe5f7f433c…` —— 全程渲染零回归。76 条自动检查全绿:
> `tools/shot/serve_check.py`(42)、`net_check.py`(17)、`node_check.cjs`(17)。
> 下一件事是回头砍体积,目标压缩后 15 MB,差 1.69 MB,候选清单在 §20.7。

体积问题**暂时搁置**。接 HTTP 会把 `/OPT:REF` 之前丢掉的网络栈拉回来,体积一定涨,
这是知情的取舍。等能用之后再回头砍,那时 `tools/shot/size_report.py` 和 §17 的方法还在,
重新量一次即可。

---

## 0. 已定的决策

| 项 | 决定 |
|---|---|
| **名字** | **shotium**(shot + Chromium 的 `-ium`)。对应 WebKit 版的 `shotkit`,同构。C 符号前缀 `shotium_*` |
| **Node 层形态** | **TypeScript,tsdown 打包成纯 ESM**,类型由源码生成而不是手写第二份 |
| **进程模型** | **渲染在调用方进程内**,走 `.node` addon 和它背后的 C ABI。曾经是进程池派 `shotium.exe`,2026-08-27 砍掉 —— 见下 |
| **JS 引擎** | 永远没有。V8 已删且不恢复。靠脚本渲染的页面出来是空的 —— 这是产品边界,不是待办 |
| **字体** | 用宿主系统字体。同一份 HTML 在不同机器上像素不同,这是接受的行为 |
| **公网威胁模型** | **不在本项目范围**。shotium 只提供底层能力,SSRF / 本地文件访问 / 资源上限由调用方决定。对应地,`file://` 访问做成**显式选项**而不是硬编码 —— 那是接口设计,不是安全策略 |
| **体积** | 本阶段不管 |

### 为什么渲染必须出进程

Blink 是**进程级单例**,代码里已确认三处:

- `blink::Platform::InitializeBlink()` 建 WTF / Partitions —— 一次
- `cppgc::InitializeProcess()` 在 `base::NoDestructor` 静态里 —— 一次
- `blink::Initialize()` + `WebThreadScheduler` 绑定主线程 —— 一次

所以**一个进程只能有一个 Blink**。于是:

- **并发** ⇒ 只能靠多进程
- **崩溃隔离** ⇒ 渲染必须出进程,否则一次崩溃拖垮整个 Node 进程

两条需求指向同一个架构。而这个架构顺带**消掉了原本 `.node` 方案的三个障碍**:
不用关 `use_partition_alloc_as_malloc`(worker 是独立进程,PartitionAlloc 随便用),
不用担心和 Node 的 V8 撞 cppgc 符号,不用把 Blink 调用 marshal 到专用线程。

代价只有进程启动(池化摊掉)和 IPC 传 PNG(几百 KB 走管道)。Puppeteer 也是这个模型。

---

## 1. 架构

```
shotium (npm, TypeScript)     生命周期 start/stop · 串行队列 · 选项校验
     │
     │  shotium.node (Node-API):JSON 进,字节出
     ▼
libshotium (shot_c)           八个 shot_* 函数,一条引擎线程
     │
     ▼
Blink   DOM → CSS → Layout → Paint → Raster → 编码
```

一个进程一个 Blink,而且**是一辈子一个,不是一次一个** —— blink 初始化写的是
进程级静态量,没有回退路径,所以 `stop()` 之后 `start()` 不回来。要并发就要多进程。

`shotium.exe` 还在构建,还在 release 的 `.7z` 里,但**不进 npm 包**:它和
`libshotium` 各自静态链了一份完整的 `shot_core`(GN 的 `source_set` 会把 .obj
链进每个依赖方),两个文件差 22 KB,exe 里没有任何对 dll 的引用。同时发就是在
40 MB 的下载里塞 19.75 MB 的重复引擎。砍掉之后 win32-x64 从 40,494,470 字节
降到 20,350,362。

---

## 2. 网络栈选型

**用 `//net` 的 `URLRequestContextBuilder`,不碰 `//services/network`。**

`//services/network` 不是"更完整"的那个 —— 它是把 `URLRequest` 包成 mojo 服务给多进程
Chrome 用的壳。缓存、重定向、HTTP/2、TLS 全部在 `//net` 里。worker 自己就是一个进程,
不需要那层壳。

| 组件 | 决定 | 说明 |
|---|---|---|
| HTTPS / TLS | **开** | BoringSSL 已在二进制里 |
| HTTP/2 | **开** | `net/spdy/spdy_session.cc` 在,白送 |
| **HTTP/3 (QUIC)** | **关** | quiche 体积大,截图场景收益小。随时可开 |
| 重定向 | **开**,设上限 | `URLRequest` 自带 |
| **缓存** | **磁盘 · Simple 后端** | 见下 |
| 代理 | **不支持**,`ProxyResolutionService::CreateDirect()` | 系统代理要拉 PAC/WPAD |
| DNS | **系统解析器** | 内置异步 DNS 更大,收益不明显 |
| Cookie | **内存 `CookieMonster`,不落盘** | 没有 JS 之后用处小,但重定向链里 set-cookie 再带回去很常见(登录墙 / CDN / 防爬)。内存版几乎不要钱,当保险 |

### 缓存为什么选磁盘 Simple 后端

内存缓存会随着页面和字体增多产生持续的内存压力,而 worker 是常驻的。磁盘缓存把这部分
移出内存。

`net/disk_cache/` 有四个后端:`memory`、`simple`、`blockfile`、`sql`。
`enable_disk_cache_sql_backend = false` 已经把 `sql` 那套连同 `//third_party/sqlite` 一起
砍掉了(§19.2),而 **`net/disk_cache/simple/` 零处引用 sqlite** —— 它是一个 entry 一个
文件,不是数据库。所以选 Simple 后端**不会把 sqlite 拉回来**。

注意这不是净省:sqlite 本来就已经砍掉了,Simple 后端自身还要多约 20 个源文件的代码。
换来的是内存不涨。

多 worker 各用各的缓存目录,避免抢锁。

---

## 3. 任务清单

### 第 0 组 · 协议与骨架 — ✅ 完成

| # | 内容 | 落点 |
|---|---|---|
| 0.1 | `--serve` 常驻模式,长度前缀帧 | `shot/shot_server.{h,cc}` |
| 0.2 | 请求结构体对齐 `ScreenshotOptions` 全字段 | `shot/shot_request.{h,cc}` |
| 0.3 | **渲染可复用**:每次渲染建/拆 Page | `ShotRenderer::TearDown()` + `ScopedClosureRunner` |
| 0.4 | 崩溃/超时协议:worker 死 = 管道断 | Windows 上管道断是**读失败** `ERROR_BROKEN_PIPE`,不是零长度读 |

### 第 1 组 · 网络 + 等待语义 — ✅ 完成

| # | 内容 | 落点 |
|---|---|---|
| 1.1 | 接 `//net` `URLRequest`,按 §2 配置 | `shot/shot_network.{h,cc}`、`shot/shot_fetch.{h,cc}` |
| 1.2 | `waitUntil`:`load` / `networkidle`(500ms 静默窗口) | `ShotRenderer::WaitForLoad()` |
| 1.3 | `file://` 访问变显式选项 | `ScreenshotRequest::allow_file_access` |

顺带:主线程消息泵换成 `MessagePumpType::IO`(`//net` 靠 `base::CurrentIOThread` 看 socket),
并撞出两个既有缺陷 —— `font-family: system-ui` 段错误、`file:` 外部样式表被静默丢弃。
详见 §20.2 / §20.3。

### 第 2 组 · 渲染能力 — ✅ 完成

| # | TS 字段 | 做法 |
|---|---|---|
| 2.1 | `scale` | `Page::SetInspectorDeviceScaleFactorOverride` + `ChromeClient::GetScreenInfo`,**不动** `SetPageScaleFactor`(那是捏合缩放,会改布局) |
| 2.2 | `fullPage` | 撑大视口最多三轮再画(视口变大会让 `vh` 内容变大,两者会互相追) |
| 2.3 | `clip` | cull rect + canvas 平移 |
| 2.4 | `selector` | `Document::QuerySelector` + `AbsoluteBoundingBoxRect`;默认构造的 `ExceptionState`,非法选择器是错误消息不是崩溃 |
| 2.5 | `type` + `quality` | `gfx::JPEGCodec` / `gfx::WebpCodec`,未指定时质量 90 |
| 2.6 | `omitBackground` | `LocalFrameView::SetBaseBackgroundColor(kTransparent)` |
| 2.7 | `path` | worker 直接写盘 |

不能同时满足的组合直接拒绝(`quality`+png、`omitBackground`+jpeg、
`selector`/`clip`/`fullPage` 三选一)——静默忽略一个字段等于交出一张悄悄没照做的图。

### 第 3 组 · Node 层 — ✅ 完成

`shotium/`:`index.js`、`lib/{protocol,worker,pool}.js`、`index.d.ts`、`README.md`。
`runtime.start()/stop()`、`on()` 五个事件、进程池 + 队列、`retry`、类型定义,全部落地。

### 第 4 组 · 常驻守护进程 — ✅ 完成

进程池的寿命等于持有它的 Node 进程。命令行、CI 步骤、队列 worker、`node -e`
都是「起 4 个 worker,渲一张,全扔掉」——对一次性进程来说,启动就是这张图的
大头。第 4 组把同一个池放到一个 socket 后面。

| # | 内容 | 落点 |
|---|---|---|
| 4.1 | 守护进程:池 + 监听 + 空闲退出 | `shotium/src/lib/daemon.ts`、`src/daemon_main.ts` |
| 4.2 | 端点按配置取哈希(Windows 命名管道 / POSIX unix socket) | `shotium/src/lib/endpoint.ts` |
| 4.3 | 客户端:连不上就拉起,一条连接多请求并发(带 `id`) | `shotium/src/lib/client.ts` |
| 4.4 | ~~命令行 `shotium` / `shotium daemon start\|status\|stop`~~ 已从 npm 包里拿掉,改由 releases 页发二进制。npm 包只服务 node | — |
| 4.5 | 检查:跨进程复用、并发、失败不致命、两种退出 | `tools/shot/daemon_check.cjs` |

三个决定值得记下来:

- **端点是配置的哈希,不是固定名字。** 连上「碰巧在跑的那个」意味着用别人的
  二进制、别人的 flag 渲图。两份配置就是两个守护进程;要按名字找就传 `name`。
- **预热是启动的一部分。** 每个 worker 先渲一张丢掉的空文档,否则第一个真实
  请求要替所有 worker 付懒初始化的钱。
- **worker 用 `detached` 起。** Windows 上这对应 `DETACHED_PROCESS`:不分配
  控制台,也就没有每个 worker 一个的 `conhost.exe`。实测四个 worker 少 40 MB
  工作集。名字听着像会活过 supervisor,其实不会——stdin 一断 worker 就退出,
  而 supervisor 死掉 stdin 就断了。

### 第 5 组 · 横向基准 — ✅ 完成

`bench/cross/`:同一份语料、同一套测量模型下,shotium 对 puppeteer 和
playwright(各两档:`chrome-headless-shell` 与完整 headless Chrome)。六个场景
——冷启动、冷启动一秒后、稳态、十张顺序、十张四并发、以及**空进程连常驻引擎**
——每格 7 次,一次一棵全新进程树。内存是整棵树工作集的采样峰值,并在峰值那一
刻按映像名拆开,因为四个引擎都是 node 驱动的,Node 堆不是被比较的东西。

方法和口径写在 `bench/cross/README.md`,结果表在 README 的「Numbers」一节。

### 第 6 组 · C ABI 与 node addon — ✅ 完成

第 6 组把 blink 放进调用方自己的进程:`shared_library("shot_c")` 导出一套 C
接口,一个 Node-API addon 用它。当时这是进程池旁边的一条旁路;2026-08-27 之后
它是唯一一条,`shotium/src/lib/engine.ts` 就是它的 JS 外壳。

| # | 内容 | 落点 |
|---|---|---|
| 6.1 | C 接口:八个函数,不透明指针,JSON 进字节出 | `shot/shot_api.h`、`shot_api.cc` |
| 6.2 | 引擎线程:blink 一条线程,任何线程调用都排到它上面 | `shot/shot_api.cc` |
| 6.3 | 导出裁剪(版本脚本 / exported_symbols_list) | `shot/shot_api.map`、`shot_api.exports` |
| 6.4 | addon 与 JS 外壳,队列深度 1 | `shotium/native/binding.cc`、`shotium/src/lib/engine.ts` |
| 6.5 | 检查:与可执行文件逐字节一致、并发串行化、只有一个引擎 | `tools/shot/node_check.cjs` |

四个决定值得记下来:

- **接缝是 C,不是 C++。** 引擎用 clang、自带的 libc++ 和自带的分配器编译,这些
  都过不了另一套工具链。`std::string` 放在接缝上,布局就成了两边各自标准库的
  属性。所以:不透明指针、UTF-8、返回码,以及没有任何内存的所有权跨过去。
- **请求走 JSON,不走结构体。** `ScreenshotOptions` 已经有一份线上格式,
  `shotium/src/lib/request.ts` 生产它、`shot_request.cc` 消费它。再定义一个 C 结构
  体等于给同一件事写第三种拼法,而第三种拼法总是先漂移的那一个;顺带也免掉了
  结构体布局的版本兼容问题。
- **导出必须裁到只剩 `shot_*`。** 这个库链进了 PartitionAlloc 的 allocator
  shim,它定义 `malloc`、`free` 和 operator new 一族并且**故意**标成可见——在可
  执行文件里这是重点,在被别人加载的共享库里就是替换掉宿主整个进程的分配器。
  Windows 上不导出就不导出;Linux 用版本脚本,macOS 用 exported_symbols_list。
- **资源目录要显式传。** 可执行文件靠 `DIR_MODULE` 在自己旁边找 `.pak`,共享库
  找不到:Linux 上那条路径走 `/proc/self/exe`,指向的是宿主的二进制(node)。
  所以 `resourceDir` 是引擎选项的一部分,由 addon 用 `__dirname` 填。

代价写在 `shot_api.h` 和两份 README 里,不藏着:进程内只有一个渲染器(blink 是
进程级单例,`worker_threads` 也共享进程),而且渲染器崩溃会带走宿主。这条路是给
「一次一张、不想跑池子」的程序用的,服务仍然用池子或守护进程。

### 第 7 组 · 发版形态

npm 上 `shotium` 这个名字拿不到,所以包名带上了 scope:`@shotkit/shotium`。产物
的名字跟着一起对齐了,构建出来的可执行文件从 `shot.exe` 改成 `shotium.exe`,
共享库、两个 `.pak`、压缩包同理 —— 解开一个归档里只有一个名字,不是两个。

| # | 内容 | 落点 |
|---|---|---|
| 7.1 | GN 输出名(`output_name`,两个 repack 的 `output`) | `shot/BUILD.gn` |
| 7.2 | 六个平台包的装配 | `tools/shot/make_platform_package.cjs` |
| 7.3 | 平台包的解析 | `shotium/src/lib/platform.ts` |
| 7.4 | CI 产出 `.tgz` 并挂到 draft release | `.github/workflows/engine-*.yml` |
| 7.5 | 从 release 资产发布七个包 | `.github/workflows/npm-publish.yml` |

引擎不进主包:它是一次 Chromium 构建,一个平台一个架构各 41 MB,六份。所以
`@shotkit/shotium` 只有 JS,引擎在 `@shotkit/shotium-win32-x64` 那六个包里,以
`optionalDependencies` 声明并带 `os` / `cpu`,npm 只装匹配当前机器的那一个。

没选 postinstall 下载:那条路让 lockfile 失去意义(锁住的不是最终拿到的东西),
在 registry 镜像后面会直接断掉,而且为了省下 npm 本来就在做的事,在安装期多跑
了一段任意代码。

addon 和它链接的共享库放在同一个平台包里,这不是为了整齐 —— `.node` 是动态链接
到那个库的,分开发就是发一个跑不起来的 `.node`。

一处诚实的缺口,写在这里而不是等别人踩:交叉编译出来的 arm64(win 和 linux)在
x64 runner 上没法执行,所以那两个平台的 addon **是交叉编译且从没在真机上跑过的**
—— 只过了编译器和链接器。以前这不太要紧,因为进程池能兜底;现在 addon 是平台包里
唯一能被 node 调用的东西,所以交叉编译失败已经改成**直接让 job 失败**,而不是发一个
装完就渲染不了的包。真机检查要等 arm64 runner 补上。

---

## 4. 对外接口(约定)

```ts
shotium.runtime.start()        // 含完整生命周期,支持 on()
shotium.screenshot(options)

interface ScreenshotOptions {
  file: string                                    // URL 或本地路径
  type?: 'png' | 'jpeg' | 'webp'
  fullPage?: boolean
  selector?: string
  quality?: number                                // 1-100,仅 jpeg/webp
  scale?: number                                  // 默认 1
  omitBackground?: boolean
  path?: string
  pageGotoParams?: { timeout?: number; waitUntil?: 'load' | 'networkidle' }
  clip?: { x: number; y: number; width: number; height: number }
  viewport?: { width?: number; height?: number }   // 新增,见下
  allowFileAccess?: boolean                        // 新增,见下
}
```

`retry` 曾经在这里，2026-08-27 拿掉了。它是监工的旋钮：把请求重发给一个
新 worker。监工和 worker 都没了，进程内引擎无处可重发，所以它现在是一个
被拒绝的未知字段，而不是一个收下来但不生效的承诺。

补了两个字段:

- **`viewport`** —— 原接口**没有任何地方能表达视口尺寸**。`fullPage` 和 `clip` 说的都是
  「从视口里取哪一块」,不是「视口多大」。JS 层把它摊平成协议里的 `width`/`height`。
- **`allowFileAccess`** —— 文档能不能读 `file:` 子资源。默认关:库不该替调用方决定
  一份文档可以读它所在机器的文件系统。CLI 对着自己指到的那个文件打开它。

2026-09-04 加了分片:

```ts
shotium.screenshotTiles(options)   // options = ScreenshotOptions + tile
interface ScreenshotTilesOptions extends ScreenshotOptions {
  tile: { height: number }         // 每片最多多少 CSS px,最后一片是余数;上限 32000
}
// -> { tiles: [{ image, x, y, width, height, path? }, ...], stats }
```

- 是一个新方法而不是 `screenshot()` 的一个选项:一个回一张图,一个回一组,返回形态不同
  的东西不该共用一个名字。`screenshot()` 收到 `tile` 会报错并指向 `screenshotTiles()`。
- 引擎内一次加载、一次布局,逐片光栅、逐片编码,任何时刻只有一片的位图在内存里。
  这也是超过单图上限(png/jpeg 65535、webp 16383)的页面唯一的整页出路。
- 线上协议:请求多一个 `tile` 字段;应答头是 `{ok, tiles:[{x,y,width,height,bytes,path?}], stats}`,
  后面跟 **每片一帧**,顺序与头中一致。守护进程的 op 叫 `tiles`。
- C ABI:`shot_engine_capture_tiles()` 回一个 `shot_tile_list`,图一张张 take 出来,零拷贝进 node Buffer。
- `path` 必须含 `{n}`(1 起的片号),否则整个请求被拒——不然每一片都会覆盖上一片。

同一天修的一个底层事实:Blink 在**绘制坐标** 32767 CSS px 处停止绘制(两个轴都是),
不报错,记录到那里就没了。布局不受限——布局在 50000、transform 上移到 25000 的元素画得出;
布局在 1000、transform 下移到 51000 的画不出。所以超过这个高度的区域靠**滚动文档**分段绘制
(滚动把内容挪回原点附近),再直接光栅到目标位图对应的行上,没有拼接拷贝。
32767 以内的页面走原路径,像素逐字节不变。

2026-09-04 下午,内存模型整个换掉(用户要求任何场景不超过 200 MB):

- **空引擎只有 15 MB 私有内存**,B 站专栏页 155 MB 里 100 MB 是 Blink 堆的垃圾:60 个字体子集
  逐个回来、每回来一个整页重排一次、截图期间 GC 关着。修法在等待循环:有请求在飞时最多每 50 ms
  跑一次 lifecycle(首轮和请求清零时必跑),64 次重排变 6 次;另加一条「堆比上次翻倍就收一次」的
  兜底(`kWaitCollectFloorBytes`,`SHOT_WAIT_GC_MB` 可调)。
- **整图位图不再存在**(`shot/shot_image_stream.cc`):整图只保留地址空间(PA 的 `AllocPages`
  不可访问),按 256 行条带提交、光栅、交给按行编码器、编完立刻 decommit。PNG 走 Skia 的 Rust
  编码器 `encodeRows`;JPEG 直接用 libjpeg-turbo 按行写(Skia 的编码器固定开 `optimize_coding`,
  要整图 3 B/px 的系数缓冲,177 MB);有损 WebP 按偶数行带导入 YUV(1.5 B/px);无损 WebP 例外,
  要整图。输出字节同样是预留地址空间按需提交(`shot::Bytes`),不再有 vector 翻倍复制。
- **图片按条带解码**(`DecodedImages`,一个 `cc::ImageProvider`):画到才解码,按 mip 层解码
  (JPEG 直接 1/2、1/4 解码,其他解完整再缩),最后一个画到它的条带编码完就丢。之前的路径是 Skia
  全尺寸解码然后缓存到截图结束——34 张图的页面攒了 265 MB。绘制记录改用 `cc::DisplayItemList`
  (带 R-tree,每条带只回放碰到的 chunk);为此关掉 `RasterInducingScroll`,否则滚动容器会
  被包成需要合成器滚动表的 `DrawScrollingContentsOp`。
- 实测(峰值私有内存 / 工作集,1440 宽):bili #1 视口 155→94 / 225→136;整页 PNG 392→115 /
  472→152,耗时 2.1 s→0.9 s;整页 JPEG 559→114;bili #2(34 张图)整页 PNG 490→237——剩下的是
  72 MB 图片原始字节 + 67 MB 的 PNG 输出本身,换 JPEG 后 ~155。
- 像素:纯文字页与旧路径逐字节相同(空页 sha 一致);带图页在图片区域有重采样差异(mip 预缩放
  对 Skia 绘制时过滤),最大差在圆角边缘像素,肉眼不可见。

---

## 5. 明确不做

- JS 执行(产品边界)
- 内置字体(接受宿主有哪些字体的差异;光栅化参数已经固定下来了:灰度抗锯齿、固定伽马、不读宿主 ClearType)
- 同 revision oracle 复验(§18.3 的版本漂移问题,记录在案,本阶段不处理)
- DCHECK-off 路径的运行时行为审计(§19.4 只修了编译错误)
- 继续压体积(§19.7 列的约 5.3 MB raw / 1.5 MB 压缩后)
- 公网威胁模型(SSRF、资源上限)—— 由调用方负责
