# 截图链路性能审计与裁剪（2026-09-14）

对 shotium 截图链路做的一轮「删无用工作、删无用对象、删无用数据」的审计和实施记录。每一项候选都给出处置结论，
每一处收益都标明证据等级：

1. **源码推断**：只读了代码，没有构建或测量。
2. **构建图通过**：`gn gen` / `ninja -n` 通过，没有编译。
3. **编译通过**：受影响目标从当前源码成功构建。
4. **功能与像素检查通过**：具名检查对着本轮构建的产物通过。
5. **性能收益经过测量**：同一台机器、同一批夹具、基线与候选二进制交替测量。

## 1. 基线

| 项 | 值 |
|---|---|
| 提交 | `edea59e8ec957e03e9809e30c9893da75361bac0`（`main`，`bench(results): v0.8.0 gh34755743048-a1`） |
| 工作区 | 仅 `.gitignore` 有一行既有改动（`/temp`），本轮保留未动 |
| 主机 | Windows 11 Pro for Workstations 10.0.26300，i9-14900KF（8P+16E，32 线程），64 GB |
| 构建配置 | `build/args/shot.gn`（`is_official_build`、ThinLTO、jumbo 64、`symbol_level = 0`），`out/Shot`，`pnpm build:engine --jobs 16` |
| 基线产物 | 从 `edea59e8` 全量重编（4,214 步，约 25 分钟）；`shotium.exe` SHA-256 `166d76c0…`，`shotium.node` `6c712bc2…`；副本放在 `out/ShotBaseline/`（CLI）和一个模拟 checkout 布局的临时目录（Node 包，`dist/` 与 `package.json` 复制自 `apps/typescript`） |
| 候选产物 | 本轮源码最终构建：`shotium.exe` SHA-256 `527f4def…`、`shotium.node` `9adf16e0…`、`shotium.dll` `62821275…`；`shotium.exe` 比基线小 91,648 字节。§3.3 的完整矩阵跑在前一次构建（`fa92615c…` / `cc3135bf…`）上，两者之间只删了 `shot_renderer.cc` 一个未用的 `#include`，其余检查在最终构建上重跑 |
| 测量条件 | 系统文件缓存**温**（每个二进制先跑一张丢弃）；不是「全冷」。所有资源来自本地磁盘；`verify:net` 的 https 用例除外 |

复现命令（仓库根目录）：

```sh
# 阶段拆分（需要 --verbose 或 SHOT_VERBOSE=1）
SHOT_PROFILE=1 out/Shot/shotium.exe apps/demo-card/card.html --width 800 --height 450 -o card.png --verbose
# 常驻引擎：N 次截图的 p50/p95（脚本见附录）
node bench.mjs apps/typescript apps/demo-card/card.html 100
# 完整矩阵：基线包 vs 候选包，校准 + 像素校验
pnpm perf:compare <baseline-package> apps/typescript result.json --calibrate
pnpm perf:report result.json --platform win32-x64 --output report.md
```

`bench.mjs` 是本轮写的 40 行驱动脚本：`require()` 包、`start({cacheDir: null})`、5 张预热、N 张计时，读回
`stats.timing` 的各阶段，打印 p50/p95。它没有进入仓库工具集（`perf:compare` 已经覆盖同一件事），附录给出全文。

### 1.1 基线的阶段拆分

常驻引擎（Node 插件，`cacheDir: null`），每个夹具 30–100 张，p50，单位 ms。`raster` 含图片解码，`wait` 含
style/layout/paint 各轮与消息泵。

| 夹具（视口） | total | setup | wait | raster | encode |
|---|---:|---:|---:|---:|---:|
| demo card 800×450 | 10.50 | 1.09 | 0.87 | 5.39 | 2.69 |
| simple 1280×720 | 5.84 | 0.52 | 1.38 | 0.30 | 3.44 |
| text 1280×720 | 7.18 | 0.51 | 2.62 | 0.25 | 3.64 |
| fonts 1280×720 | 8.80 | 0.58 | 4.22 | 0.34 | 3.42 |
| css-heavy 1280×720 | 19.12 | 0.69 | 7.88 | 5.24 | 5.05 |
| images 1280×720 | 11.04 | 0.61 | 2.38 | 1.50 | 6.31 |
| gradient 1280×720 | 12.81 | 0.62 | 1.74 | 1.83 | 8.16 |
| filter 1280×720 | 20.82 | 0.65 | 2.58 | 8.55 | 7.83 |
| bilibili 1788403871415 视口 1440×900 | 265.5 | 22.7 | 133.9 | 80.7 | 13.9 |

三个结论决定了后面的顺序：

- **PNG 编码是多数页面的最大单项**（simple 59%、gradient 64%、images 57%），而它在主线程串行，光栅线程全部空转。
- **`wait` 里有整轮被丢弃的工作**：css-heavy 在外链样式表到达前做了一整轮 style+layout+prepaint+paint（3.8 ms），
  样式表落地后全部重做；fonts 每张都重新解码同一个 woff2（2.7 ms）。
- 候选清单里的启动项（A3）确实大：引擎启动 20 ms 里 12 ms 是网络栈，其中 10 ms 是 `NetworkChangeNotifier`。

启动拆分（CLI，`SHOT_PROFILE=1`，温缓存）：

```
network: change_notifier=10.3 context=1.9 total=12.2
runtime: allocator=0.05 icu=0.08 resources=0.8 thread_pool=0.28 mojo=0.03 blink_static=0.26
         scheduler=0.16 discardable=0.02 skia=0.01 dwrite=1.5 system_fonts=1.6 blink=3.2 network=12.3 total=20.2
main:    runtime=20.2 capture=26.9（冷进程首张） total=48.2
```

进程壁钟约 83–96 ms，即 main() 之外（加载 43 MB 映像、静态初始化、退出时的析构）占了 35–45 ms。

## 2. 实施的改动

按依赖链整体处理；每项写明删掉的调用链、为什么不影响功能、证据等级。

### 2.1 网络栈按需建立（候选 A3）

`shot/shot_network.{h,cc}`、`shot/shot_capture.cc`、`shot/shot_fetch.cc`。

- 之前：`ShotRuntime::Create` → `ShotNetwork::Create` 无条件 `NetworkChangeNotifier::CreateIfNeeded()`（Windows 上同步
  `RecomputeCurrentConnectionType`，再起 `SystemDnsConfigChangeNotifier`）→ `URLRequestContextBuilder::Build()` → 有缓存目录时
  `OpenCacheEagerly`。
- 之后：`Create()` 只记录配置。配置了缓存目录时立刻建 context 并打开缓存（这是 `start()` 能回答 `cacheActive` 的前提，契约不变）；
  没有缓存目录时什么都不建。`EnsureUp()` 在第一次 http(s) 请求（顶层文档或子资源）时补齐 context，并在这时才创建
  change notifier。`Get()` 仍是「有没有」的问题，不触发建立。
- 为什么安全：`NetworkChangeNotifier` 的观察者列表是进程级静态对象，`HostResolverManager` 在 Build 时注册进去，之后创建的
  notifier 照样通知它；notifier 缺席时连接类型读作 `CONNECTION_UNKNOWN`，`//net` 自己的 `kDeferConnectionTypeAtStartup`
  把这个值定义为「已连接、类型未定」。`FetchDocument` 与 `ShotFetch::StartNow` 改经 `EnsureUp()`，建失败的错误照原样报告。
- 证据：等级 5。CLI `runtime total` 20.2 → 6.1 ms；Node `start()` 22–26 → 11.1 ms（`cacheDir: null`）。`verify:net` 全部
  19 项通过（含 http 与 file 字节一致、跨进程磁盘缓存、networkidle、https 真实服务器）。

### 2.2 `--stdin` 不再经过临时文件（候选 A4）

`shot/shot_options.{h,cc}`、`shot/shot_request.h`、`shot/shot_capture.cc`、`shot/main.cc`。

- 之前：读 stdin → `CreateUniqueTempDir` → 写 `stdin.html` → `FilePathToFileURL` → 渲染时再 `ReadFileToString`。
- 之后：`PreparedShot::document` 携带字节；`ScreenshotRequest::document`（不在线协议里，两个解析器都不设置）把它交给
  `WithDocument`，直接构造 `RenderInput`，并按资源计数。逻辑 URL 仍是 `file:///<temp>/shot-stdin-<uuid>/stdin.html`
  这个形状，只是目录不再创建：file: 源、绝对路径本地资源可达、相对引用解析到一个不存在的目录——与之前临时目录里只有
  这一个文件时的语义完全相同。
- 证据：等级 4。`cat card.html | shotium.exe --stdin` 与 `shotium.exe card.html` 输出字节相同。

### 2.3 子资源体直接交付，不再经 mojo 数据管道（候选 B5、B2）

`shot/shot_url_loader.{h,cc}`、`third_party/blink/renderer/platform/loader/fetch/{resource_loader.*,url_loader/url_loader_client.h}`。

- 之前：每个子资源 `mojo::CreateDataPipe`（64 KB 共享缓冲）→ `DataPipeProducer` 在线程池序列上分块写入 → 主线程
  `SimpleWatcher` 每块唤醒一次 → `DataPipeBytesConsumer` 逐块复制进 `SegmentedBuffer`；本体在 loader 里保留到写完，
  每张在途图片驻留两份。
- 之后：`DidReceiveResponse(response, 空句柄)` → `DidReceiveData(整段 span)` → `DidFinishLoading`，全部在主线程一次任务内
  完成；blink 的一份拷贝是唯一的一份，且是一个分配而不是 64 KB 一段。`URLLoaderClient::DidReceiveDataForTesting` 改名为
  `DidReceiveData`（它就是 `ResourceLoader` 流式路径的入口；上游只有测试到达它），与 `ResponseBodyLoaderClient::DidReceiveData`
  合并为一个实现。
- 为什么安全：`ImageResource::AppendData` 只拒绝 `SegmentedBuffer` 变体（后台响应处理器路径），span 变体正是数据管道路径
  每块调用的那个；`DidFinishLoading` 在没有 body loader 时同步完成。`DidReceiveResponse` 可能取消加载（MIME 拒绝、CORS），
  用 WeakPtr 观察自身是否已随 `ResourceLoader` 销毁。
- 证据：等级 4。168 个夹具解码像素与基线逐字节相同；`verify:net`、`verify:bilibili`（两篇整页、每张图、二维码）通过。

### 2.4 MIME 扩展名查询进程内完成，浏览器接口代理删除（候选 B2）

`third_party/blink/renderer/platform/network/mime/mime_type_registry.cc`、`shot/shot_platform.{h,cc}`。

- 之前：`MIMETypeRegistry::GetMIMETypeForExtension` → `[Sync]` mojo 调用 → `ShotPlatform` 的接口代理把 receiver 放到线程池
  序列（放在同线程会自等）→ `net::GetMimeTypeFromExtension`。每个 file: 外链样式表触发一次（`CSSStyleSheetResource::CanUseSheet`）。
- 之后：直接调用 `net::GetMimeTypeFromExtension`；沙箱不存在，注册表可读。`ShotBrowserInterfaceBroker`、`MimeRegistryImpl`
  和它们的线程池序列一并删除，`Platform::GetBrowserInterfaceBroker()` 回到 blink 的默认实现（丢弃每个接口请求）。
- 证据：等级 4。所有外链 CSS 的 demo 和新增的 `local-stylesheet` reftest 通过。

### 2.5 `PolicyContainer::CreateEmpty` 不再造管道（候选 B2）

`third_party/blink/renderer/core/frame/policy_container.cc`。空容器之前绑定一个「专用 receiver」的 `AssociatedRemote`
（一条消息管道），`UpdateReferrerPolicy` / `AddContentSecurityPolicies` 向它序列化消息。现在空容器的 remote 未绑定，
两个更新在 `is_bound()` 为假时只更新本地策略。CSP 与 Referrer Policy 的本地计算不变。证据：等级 4（demo 与 net 检查）。

`LocalFrameMojoHandler` 每帧仍建 2 条关联端点 + 1 条普通管道（经 `EmptyLocalFrameClient` 的 `LocalProvider`），
43 处 `GetLocalFrameHostRemote()` 调用向无人接收的端点序列化消息。**未处理**：帧创建整段实测 0.03 ms（见 2.8），
剩余管道的成本落在测量分辨率以下，不值得改 `LocalFrame`。这是候选 B2 里「不要把删掉两个接口说成整个 Mojo 已移除」的那部分：
`mojo::core::Init()` 保留（0.02 ms），依赖根仍在 `LocalFrameMojoHandler`、`AssociatedInterfaceProvider` 与
`DocumentLoader::content_security_notifier_`。

### 2.6 Skia 执行器不再投递空任务、等待不再自旋（候选 B4）

`shot/shot_runtime.cc`。之前 `add()` 每个工作项投递一个线程池任务，`borrow()` 偷走工作后那个任务只检查到空队列；
`SkTaskGroup::wait()` 在队列空、别的线程还在跑时自旋调用 `borrow()`。现在：投递的任务把队列排空，投递数不超过工作项数
也不超过线程池 3 个前台线程；`Borrow()` 在队列空且有工作在跑时等条件变量，工作完成时广播。使用方只有 `SkBlurEngine`
的两趟（每趟 4 个任务）。证据：等级 4（filter demo 像素相同）；**收益未单独测量**——filter 夹具的 raster 在本轮同时被
块编码改变，无法归因，按「删掉的是确认存在的空调度与自旋」记为源码级改动。

### 2.7 一次性 CLI 退出前不做常驻整理（候选 B6）

`shot/main.cc`、`shot/shot_renderer.{h,cc}`。`ShotRenderer::SetOneShot(true)` 后：渲染后的 `MemoryReclaimer::ReclaimAll()`
不做，页面不再排队到空闲回合去 detach。图片写完后 `fflush` 两个标准流并 `TerminateCurrentProcessImmediately`，跳过引擎析构
（帧 detach、堆回收、调度器关停、线程池 join、43 MB 映像解除映射）。**配置了 `--cache-dir` 的运行仍走完整退出**：
simple 后端在关停时写索引，条目由线程池写出，硬退出会让下一个进程重建索引。光栅前的那次 `ReclaimAll`（大截图降峰值）保留。
证据：等级 5。CLI 壁钟 min 87–91 → 68–70 ms（与 2.1 合计；单独归因见 §3.1）。

### 2.8 页面创建：共享 AgentGroupScheduler、缓存颜色表、经 `WillBeDestroyed` 拆页（候选 A6、A1）

`shot/shot_renderer.{h,cc}`。

- `Page::CreateNonOrdinary` 之前每张截图新建一个 `AgentGroupScheduler`（两条主线程任务队列）并传 `nullptr` 让 `Page`
  自己 `ColorProviderColorMaps::CreateDefault()`（三个 `ColorProvider` 混色再转成表，`Page` 再从表建三个）。现在 AGS 由
  渲染器持有并复用（`ReleaseRetained()` 时放手），颜色表进程内算一次。`page=` 0.13 → 0.04 ms。
- `TearDown()` 之前只 `frame_->Detach()`，从不调用 `Page::WillBeDestroyed()`：`Page` 留在 `AllPages()`（弱引用，但）
  它的 `PageSchedulerImpl` 一直挂在 `MainThreadSchedulerImpl::page_schedulers_` 上直到下一次 GC，而 GC 在小截图之间是被
  推迟的，每次策略更新遍历一串死页面。现在按 `IsolatedSVGDocumentHost::Shutdown()` 的做法调用 `WillBeDestroyed()`，
  同步释放页面调度器。
- A1（初始空文档）：`frame_->Init()` 仍先提交一个空 `DocumentLoader`/`Document`，`ForceSynchronousDocumentInstall` 再
  `Shutdown()` 它。拆分计时后：`frame=0.03 initial_document=0.18 viewport=0.02`（ms，温）。空文档是 `LocalDOMWindow` 与
  `DocumentLoader`（真实文档的 `ResourceFetcher` 由 `FrameFetchContext::CreateFetcherForCommittedDocument(*loader, doc)`
  建立）的载体，绕开它需要重写 `FrameLoader::Init`/`DocumentLoader::CommitNavigation` 的提交流程。0.18 ms 是一张卡片的
  1.7%，**未处理**，计时留在 `SHOT_PROFILE` 里。
- A5（`ApplyChromeWebPreferences`）：实测 `settings=` 0.01–0.02 ms，**不值得做**。
- 证据：等级 4/5。`verify:node`（stop/start、restart）、`verify:daemon` 通过；`page=` 计时来自 `SHOT_PROFILE`。

### 2.9 PNG：进程内写容器，deflate 流由光栅线程并行产生

`shot/shot_image_stream.cc`、`shot/BUILD.gn`（新增 `//third_party/zlib`）。

- 之前：`SkPngRustEncoder`（Rust `png` 0.18 + `flate2`/miniz_oxide，level 1，Up 滤波）在主线程逐条带压缩；光栅线程做完就闲。
- 之后：`PngBlockEncoder` 自己写 signature/IHDR/IDAT/IEND。`RowEncoder` 新增块接口：`EncodeBlock(first_row, rows)`
  任意线程调用，产出一段以 sync flush 结尾的 raw deflate（level 1，Up 滤波；块首行用 Sub，所以块只读自己的行），
  `AppendBlock` 在拥有输出的线程按行序写 IDAT，Adler-32 用 `adler32_combine` 合并；`Finish` 补最终空块 `01 00 00 FF FF`
  和校验和。多条带截图由光栅线程在光栅完自己的条带后就地编码；单条带截图（≤ 500k 像素）把行分成至多 4 块，线程池
  3 个前台线程 + 调用线程并行压缩。
- 为什么不影响功能：无损；168 个夹具（含 `omitBackground` 的非预乘 RGBA 路径）解码像素与基线相同；PNG 结构由
  独立的 Python 解析器（CRC、zlib 流、原始长度、滤波还原）和 `verify:serve` 的 pngjs 解码双重校验。JPEG、WebP 路径未动，
  输出字节相同。
- 文件尺寸：Chromium zlib 的 level 1 比 miniz 的小，168 个夹具合计 5,384,067 → 4,698,870 字节（−12.7%）。
- 证据：等级 5。encode（主线程等待编码的时间）：simple 3.44 → 0.05 ms，gradient 8.16 → 0.31，card 2.69 → 1.04（单条带
  分 4 块）。多条带页面的压缩时间计入 `raster`（在光栅线程上），所以 `raster` 数字上升、总时间下降。

### 2.10 等待循环：中间轮只做 style+layout，首轮在只有本地资源在途时推迟（候选 A2）

`shot/shot_renderer.cc`、`shot/shot_capture_context.h`、`shot/shot_fetch.cc`。

- 之前：每轮 `UpdateAllLifecyclePhases`，包括外链样式表未到时的首轮——那轮的 style/layout/prepaint/paint 在样式表落地后
  全部重做（css-heavy：3.8 ms）。首轮的理由是让内联 `@font-face` 的字体与外链样式表并行请求，省一个网络往返。
- 之后：有请求在途的轮次只 `UpdateLifecycleToLayoutClean`（layout 才发现字体，style 才发现背景图，prepaint/paint 什么
  都不发现）；判定「已加载」的那一轮必须是完整生命周期，这样截图的 paint 就是检查过的那棵树，后生命周期步骤
  （`loading=lazy` 的 IntersectionObserver 派发）发出的请求也在判定之前计入。首轮：本轮截图还没发过任何网络请求
  （`CaptureContext::network_requested()`，由 `ShotFetch::Start` 置位）且有资源在途时，不做——本地文件在下一个任务就到，
  提前布局只是布局两遍。`last_lifecycle` 从安装时刻起算，否则首轮的 50 ms 间隔规则永远为真（这是本轮改动时发现并修
  的一个小逻辑错）。
- networkidle 语义不变；http 页面的首轮布局保留（并行请求的价值仍在）。
- 证据：等级 5。css-heavy wait 7.88 → 3.98 ms，text 2.62 → 1.28，simple 1.38 → 0.74。新增 reftest `local-stylesheet`
  （本地外链样式表 + 内联 `@font-face` 指向本地 ahem.ttf）和 `img-lazy`（视口内 `loading=lazy` 图片）通过；`verify:net`
  的「networkidle 等到 load 等不到的样式表」通过。

### 2.11 解码后的 Web 字体按内容缓存

`third_party/blink/renderer/platform/fonts/font_custom_platform_data.{h,cc}`、`shot/shot_runtime.cc`。

- 现象：`FreshnessLifetime()` 对桌面平台的 file: 响应返回 0（改过的文件要被看见），所以每张截图都重新取、重新解码同一个
  字体：woff2 解压 + OTS 清洗 + `SkTypeface` 创建，63 KB 的 Roboto 2.7 ms，bilibili 页面 403 个子集。
- 做法：`FontCustomPlatformData::Create(SharedBuffer*)` 先算字节的 SHA-256 + 长度，命中则复用 `sk_sp<SkTypeface>`；
  未命中解码后插入。以解码后大小为界（64 MB，最旧先出），`ShotRuntime::PurgeMemory()` 经
  `FontCustomPlatformData::ClearDecodedFontCache()` 清空。字节仍然每次重读（改过的文件是 miss），只有解码结果按内容共享——
  与 `FontCache` 共享系统字体 `SkTypeface` 是同一件事。
- 证据：等级 5。fonts 8.80 → 3.53 ms；bilibili 视口 265 → 174 ms（wait 134 → 51）。像素相同；`verify:bilibili` 通过。
  内存：fonts 30 张后 peak private 71.8 → 57.3 MB，purged 36.3 → 31.7 MB（缓存清空后更低，因为不再有 30 份等待 GC 的
  `FontResource` 数据）。

### 2.12 诊断：`SHOT_PROFILE` 覆盖启动与退出

新增 `shot/shot_profile.h`（`ProfileEnabled()`、`ProfileStages`），`ShotRuntime::Create`、`ShotNetwork::BuildContext`、
`main()` 打印分步耗时和 `process_to_main`；`SHOT_DUMP_OPS` 递归进 `DrawRecordOp`；每个条带的 raster/encode 分开打印；
`SHOT_STRIP_RASTER_PIXELS` 可以把单条带阈值往下调以做实验（§6）。全部在 `SHOT_PROFILE` 之后，正式测量不开。

## 3. 结果

### 3.1 首次出图（新进程，温文件缓存）

| 指标 | 基线 | 候选 | 证据 |
|---|---:|---:|---|
| 引擎启动 `ShotRuntime::Create`（CLI，无缓存目录） | 19.1–20.2 ms | 6.1–6.2 ms | `SHOT_PROFILE`，3 次 |
| 其中网络栈 | 12.0–12.4 ms | 0.002 ms（延迟到首次 http） | 同上 |
| Node `Runtime.start({cacheDir: null})` | 21.8–25.9 ms | 11.1–11.5 ms | bench.mjs，3 次交替 |
| CLI 进程壁钟，card 800×450，`Measure-Command` 15 次交替 | p50 93.8、min 87.3 ms | p50 79.9、min 69.7 ms | 噪声 ±10 ms；min 更稳 |
| 进程创建到 `main()` | — | 约 30 ms（`process_to_main`） | 加载器与静态初始化，本轮未动 |

配置了缓存目录时，context 与缓存仍在启动时建立（`cacheActive` 契约），只有 change notifier（10 ms）延后。

### 3.2 常驻引擎（p50 / p95，ms，Node 插件，每个夹具 30–100 张）

| 夹具 | 基线 total | 候选 total | 变化 | 基线 p95 | 候选 p95 |
|---|---:|---:|---:|---:|---:|
| demo card 800×450 | 10.50 | 8.67 | −17% | 11.53 | 9.74 |
| simple 1280×720 | 5.84 | 3.00 | −49% | 6.39 | 3.26 |
| text 1280×720 | 7.18 | 3.39 | −53% | 11.79 | 4.06 |
| fonts 1280×720 | 8.80 | 3.53 | −60% | 9.72 | 3.98 |
| css-heavy 1280×720 | 19.12 | 13.73 | −28% | 24.64 | 20.03 |
| images 1280×720 | 11.04 | 6.85 | −38% | 11.82 | 7.36 |
| gradient 1280×720 | 12.81 | 9.38 | −27% | 13.35 | 15.25 |
| filter 1280×720 | 20.82 | 17.73 | −15% | 25.52 | 19.18 |
| bilibili 1788403871415 视口 1440×900 | 265.5 | 173.8 | −35% | 267.8 | 181.8 |

全部为最终二进制；gradient 的 p95 抖动来自光栅线程与压缩共用核心（§2.9），`perf:compare` 对同一夹具给出
13.34 → 8.99 ms（§3.3）。完整矩阵见 §3.3。

### 3.3 `perf:compare` 完整矩阵（win32-x64，基线包 vs 候选包，`--calibrate`）

运行：2026-09-14，`pnpm perf:compare <baseline> apps/typescript result.json --calibrate`，候选插件 `cc3135bf…`（与最终产物
`9adf16e0…` 的差别只是随后删掉的一个未用 `#include`），基线插件 `6c712bc2…`。校准（候选对自身，6 个用例）给出等价带：
中位/均值 +2.36%，尾部 +13.25%。108 个用例全部完成采样；`pnpm perf:images`：108/108 配对图片像素校验通过。

判定：**91 项更快，15 项等价，2 项方向未定，0 项更慢**。`improvement` 验收要求每个引擎用例都「更快」，本轮**未通过**：
等价的 15 项是 JPEG/WebP 编码占死的用例（`card-webp`、`standard-*-webp`、`scale-8-jpeg`、`daemon-jpeg/webp-*`、`startup-jpeg/webp`）
和两个外部等待用例（`http-slow` 只要求不更慢，已满足）；方向未定的两项是 `output-file-jpeg`（2.94 → 2.97）和
`many-resources-default-webp`（49.2 → 48.2）。JPEG 与 WebP 编码器本轮没有动，它们的输出字节与基线相同。

摘录（ms，p50；完整 108 行见 `perf:report` 输出，未入库）：

| 场景 | 基线 p50 | 候选 p50 | 变化 | 判定 |
|---|---:|---:|---:|---|
| card-png | 3.61 | 2.27 | -37% | faster |
| card-jpeg | 2.27 | 2.20 | -3% | faster |
| card-webp | 13.78 | 13.69 | -1% | equivalent |
| corpus-png | 11.07 | 5.12 | -54% | faster |
| corpus-scale2-png | 31.11 | 14.95 | -52% | faster |
| features-alpha | 1.82 | 1.34 | -27% | faster |
| bili-1415-viewport | 263.59 | 170.75 | -35% | faster |
| bili-1415-clip-png | 340.59 | 195.10 | -43% | faster |
| bili-8828-clip-webp | 1281.58 | 1228.84 | -4% | faster |
| standard-simple-png | 6.51 | 3.86 | -41% | faster |
| standard-text-png | 8.04 | 3.92 | -51% | faster |
| standard-fonts-png | 9.68 | 3.96 | -59% | faster |
| standard-css-heavy-png | 19.77 | 13.16 | -33% | faster |
| standard-images-png | 11.65 | 7.42 | -36% | faster |
| standard-gradient-png | 13.34 | 8.98 | -33% | faster |
| standard-filter-png | 22.12 | 18.02 | -19% | faster |
| standard-long-page-png | 29.11 | 10.81 | -63% | faster |
| standard-filter-webp | 49.30 | 49.02 | -1% | equivalent |
| scale-8-png | 127.27 | 79.10 | -38% | faster |
| scale-8-jpeg | 66.11 | 66.08 | -0% | equivalent |
| output-file-jpeg | 2.94 | 2.97 | +1% | unproven |
| queue-16 | 33.01 | 31.46 | -5% | faster |
| processes-4 | 19.00 | 18.62 | -2% | faster |
| startup-png | 9.20 | 8.09 | -12% | faster |
| startup-jpeg | 7.91 | 7.99 | +1% | equivalent |
| soak-1000 | 2.35 | 2.29 | -3% | faster |
| daemon-png-buffer | 3.97 | 2.69 | -32% | faster |
| daemon-webp-file | 14.83 | 14.79 | -0% | equivalent |
| recovery-timeout | 16.14 | 14.49 | -10% | faster |
| http-remote-page | 15.58 | 6.72 | -57% | faster |
| http-slow | 265.47 | 264.54 | -0% | equivalent |
| http-networkidle | 518.58 | 514.72 | -1% | faster |
| cache-default | 15.56 | 5.00 | -68% | faster |
| cache-reload | 15.39 | 8.40 | -45% | faster |
| many-resources-reload-png | 57.01 | 40.51 | -29% | faster |
| many-resources-default-webp | 49.21 | 48.16 | -2% | unproven |

冷启动用例（每侧 367 个新进程）的子指标：`Runtime.start()` 22.18 → 10.99 ms，import+start 30.48 → 18.52 ms，
进程创建 220.9 → 208.8 ms（node 自身，不判定）；首张 PNG 9.20 → 8.09 ms。吞吐（均值时间的倒数）与中位同向，
`soak-1000`（1000 对）2.352 → 2.286 ms。

### 3.4 内存

引擎自己的记账（`SHOT_PROFILE` 的 `mem` 行：`GetProcessMemoryInfo` 的 private/working set 与峰值），30 张后，MB：

| 夹具 | 基线 peak private | 候选 peak private | 基线 purged private | 候选 purged private |
|---|---:|---:|---:|---:|
| card | 119.0 | 115.8 | 45.5 | 43.6 |
| fonts | 71.8 | 57.3 | 36.3 | 31.7 |
| images | 77.4 | 55.5 | 49.6 | 35.9 |
| filter | 121.9 | 119.6 | 49.0 | 47.9 |

峰值下降来自子资源体不再驻留两份、页面调度器及时释放、字体不再 30 份等 GC。字体缓存上限 64 MB 解码后大小，
清空点是显式 `releaseMemory()` / `stop()`；一个跑遍很多不同字体的常驻进程会多持有至多 64 MB，这是本轮引入的一处
有界常驻内存，文档见 2.11。工作集：purged 后 98.4 → 89.5 MB（card）。没有测量缺页与线程数。

### 3.5 像素回归与功能检查（对着最终产物 `527f4def…` / `9adf16e0…`）

| 检查 | 结果 |
|---|---|
| `pnpm verify:demos` | 86 个 demo：PASS 64、FUZZY 1（color-mix，声明容差内）、SMOKE 21，含新增 2 个 |
| `pnpm verify:serve` | 88 项全部通过（帧协议、同进程两次渲染字节一致、`allowFileAccess` 门、omitBackground 4 通道 PNG、JPEG/WebP、错误处理） |
| `pnpm verify:net` | 19 项全部通过 |
| `pnpm verify:charset` | 全部一致（状态与基线相同：ForceSynchronousDocumentInstall 固定 UTF-8） |
| `pnpm verify:node` / `verify:node-entry` / `verify:daemon` / `verify:daemon-protocol` | 全部通过（node-entry 7/7） |
| `pnpm verify:bilibili --package apps/typescript` | 两篇整页、每张瓦片、每张照片、两个二维码通过 |
| `pnpm accept --skip-build` | 语料 1248×1320 对 Chrome oracle 整图差 1.524%；用基线二进制渲染同一语料，解码像素与候选**完全相同**，差异即 cut-progress §8.6 记录的既有差异 |
| 基线 vs 候选 A/B（脚本 `abpix2.sh`） | 168 个夹具（benchmark 10、demo 86、card、features、corpus、selector×2）解码像素全部相同；JPEG、WebP 输出字节相同 |
| `pnpm render run`（apps/test/render） | 基线用 `--baseline-engine shot --baseline-executable out/ShotBaseline/shotium.exe` 生成后，4/4 用例 0 个像素变化（SHA 不同，是编码器字节差异）；这批用基线引擎生成的基线是本轮的对照物，不是 Chrome 参考，用完已删除；`cases.json` 里 `text` 用例的资源路径 `../../shot/testdata/ahem.ttf` 自 09-10 目录搬迁后就指错了，本轮改为 `../../../shot/testdata/ahem.ttf` |

## 4. 候选清单处置

| 候选 | 处置 | 说明 |
|---|---|---|
| A1 初始空文档 | 已量化，未处理 | 0.18 ms/张（§2.8）；载体是 `DocumentLoader`/`LocalDOMWindow`，绕开需重写提交流程 |
| A2 加载中的无效中间绘制 | 已处理 | §2.10；watchdog 与已完成通知在此前已由 `DispatchDidHandleOnloadEvents` 解决，本轮复核无「已完成却等 watchdog」 |
| A3 网络栈懒初始化 | 已处理 | §2.1 |
| A4 stdin 临时文件 | 已处理 | §2.2 |
| A5 重复默认值构造 | 已量化，不值得做 | `settings=` 0.01–0.02 ms |
| A6 无消费者的 Page/Frame 配套对象 | 部分处理 | AGS 复用、颜色表、`WillBeDestroyed`（§2.8）；`PerformanceMonitor`、`AutoscrollController`、`DragCaret`、`FocusController` 等仍逐页创建，帧+页创建合计 0.07 ms（含初始文档 0.25），不再细拆 |
| B1 截图专用调度路径 | 已核对，不值得做 | `FindInPageBudgetPoolController`、bfcache IPC 跟踪队列都是进程级一次性对象（scheduler 启动 0.16 ms）；每任务钩子里的 UMA 都被子采样，负载跟踪是算术 |
| B2 单进程假 IPC | 部分处理 | PolicyContainer、MIME、子资源数据管道已删（§2.3–2.5）；`LocalFrameMojoHandler` 三条管道与 `AssociatedInterfaceProvider::LocalProvider` 保留（§2.5 说明） |
| B3 可丢弃内存分配器 | 已核对，未处理 | 唯一使用者是 `SkResourceCache`（`SK_USE_DISCARDABLE_SCALEDIMAGECACHE`，模糊九宫格与 mipmap）；启动 0.02 ms，30 张 card 后 `discardable=0.1 MB`；它的 1 s `EnforceMemoryPolicy` 定时任务是唯一持续开销。替换为 malloc 池要重测峰值与回收，本轮收益无法证明 |
| B4 线程池空调度 | 已处理 | §2.6，收益未单独测量 |
| B5 复制与内存重叠 | 已处理 | §2.3（子资源体唯一一份）；顶层文档 `input.body` 仍在安装后即刻释放（既有）；`--stdin` 的 `document` 经 `const&` 请求复制一次（64 MB 上限，比原来的文件往返便宜） |
| B6 一次性 CLI 退出 | 已处理 | §2.7 |
| B7 TLS 与动态库配置 | 已核对，未处理 | `blink_heap_inside_shared_library = true` 让 `ThreadStateStorage::Current()` 在 Windows 上也走出线 getter；改成按平台需要 GN args 能看到 `target_os`，而 `import()` 顺序做不到（见 `build/args/shot.gn` 注释）。没有测出它在截图链路上的成本；不改 |
| C1 TaskAnnotator | 已核对，不值得做 | 每任务约十次指针拷贝；一张卡片十几个任务，bilibili 数百个，量级 µs |
| C2 PerformanceMonitor / CoreProbes | 已核对，不值得做 | 唯一订阅者 `WindowPerformance`（longtask，脚本驱动）不会订阅，`enabled_` 恒假；探针在无代理时是一次 `HasAgentsGlobal` + 一次 supplement 查找。删除是源码清理，无可测收益 |
| C3 UMA / 直方图 | 已核对，不值得做 | 每任务 UMA 被 1/1000 子采样；逐图 `ReportImagePixelInaccuracy`（每次图片 paint 4–6 次直方图查找）随 C4 一并量化 |
| C4 页面性能指标采集 | 已量化，未处理 | 实验构建（环境变量绕过 `ScopedPaintTimingDetectorBlockPaintHook` 与 `NotifyImagePaint`）：prepaint+paint 从 text 0.227 → 0.209、css-heavy 0.934 → 0.900、bilibili 每轮 ~2.7 → 2.4 ms，即一张截图的 0.1–0.5%。多文件手术换这个比例，不做；实验代码已还原 |
| C5 Perfetto / tracing / MemoryDump | 已核对，不值得做 | 无 tracing 会话，`TRACE_EVENT` 是一次原子读；`DiscardableSharedMemoryManager` 的 dump provider 注册在启动的 0.02 ms 里 |
| C6 CSS 调试映射与控制台历史 | 已核对，不值得做 | `InvalidationSetToSelectorMap` 的 scope 在 `IsTracking()` 为假时是静态布尔判断；`ConsoleMessageStorage` 只在有消息时增长 |
| C7 NetLog | 已核对，不值得做 | 无观察者，`AddEntry` 是原子读；source id 是一次原子自增 |
| 编码器（不在清单里） | 已处理 | §2.9，实测最大单项 |
| 字体解码（不在清单里） | 已处理 | §2.11 |

## 5. 只是源码/构建清理、没有测得运行期收益的部分

- `ShotPlatform` 删除接口代理与 `MimeRegistryImpl`（随 §2.4）；`mime_registry.mojom` 仍在 `blink/public/mojom` 目标里，
  本轮不动 mojom 列表。
- `SkPngRustEncoder` 不再被 shot 引用；Skia 内部 `SkSVGCanvas` 仍引用，目标保留。
- `shot_url_loader.h` 不再包含 `DataPipeProducer`；`mojo/public/cpp/system/data_pipe.h` 因 `ScopedDataPipeConsumerHandle`
  的空句柄仍需要。
- `shotium.exe` 小 91,648 字节，来自 Rust PNG 编码器路径与 mojo 代理的消失；不作为成果。

## 6. 未覆盖与剩余风险

- **平台**：只在 Windows x64 构建和测量。Linux 图探针（`out/ProbeLinux`，`shot-linux.gn`）的 `gn gen` 仍报 30 个
  「输入不由依赖生成」错误，全部是跨宿主的 `*.exe` 宿主工具（`cxxbridge.exe`、`brotli.exe`、`*_build_script.exe`），与本轮
  改动无关，也无法证明 Linux/macOS/musl/arm64 编译通过。新增依赖 `//third_party/zlib`（`//net` 已经依赖）和
  `//crypto`（`blink/renderer/platform` 已经依赖）不引入新的平台条件。ELF 符号表 `shot_api.map`/`shot_node.map` 未动。
- **未运行的检查**：`pnpm verify:ffi` / `verify:delivery`（需要五种语言工具链与打包）、C ABI 语言示例；`perf-gate.yml`
  六平台；`checks.yml`。
- **`process_to_main` 约 30 ms** 与 `blink::Initialize` 2.8 ms、DirectWrite 隔离工厂 1.0–1.5 ms、系统字体读取 1.0–2.1 ms
  未动。
- **单条带小图的光栅**（card 5.5 ms）是 Skia 计算：背景径向渐变 1.5 ms、`mask-image` 两个径向渐变 + 三层 SaveLayer 3.2 ms。
  用 `SHOT_STRIP_RASTER_PIXELS=100000` 把 card 分成 3 条带，像素相同、总时间 8.8 → 7.6 ms，但每条带因 100 行阴影
  边距各自光栅了 400 行中的 400 行，收益全部来自编码重叠；不改默认阈值（其它页面在条带边界的滤镜接缝没有验证）。
- **字体缓存**是本轮唯一新增的跨请求常驻状态，按内容寻址、有界、随 `releaseMemory()` 清空；命中要求字节完全相同。
- 行尾：本轮用脚本改的 15 个文件曾被写成 CRLF，已统一回 LF；`perf:compare` 结果里记录的 `sourceDiffSha256` 是归一化前的
  diff，二进制不受影响。

## 附录：bench.mjs

```js
// 常驻引擎微基准：同一文件 N 张，读回各阶段统计。用法：node bench.mjs <packageDir> <file> [n] [json-extra]
import {createRequire} from 'node:module';
import path from 'node:path';
const require = createRequire(import.meta.url);
const [pkg, file, nArg, extraArg] = process.argv.slice(2);
const n = Number(nArg || 50);
const extra = extraArg ? JSON.parse(extraArg) : {};
const shotium = require(path.resolve(pkg));
const rt = new shotium.Runtime();
const t0 = process.hrtime.bigint();
rt.start({cacheDir: null});
const startMs = Number(process.hrtime.bigint() - t0) / 1e6;
const req = {file, viewport: {width: 800, height: 450}, type: 'png', allowFileAccess: true, ...extra};
const rows = [];
for (let i = 0; i < n + 5; i++) {
  const a = process.hrtime.bigint();
  const r = await rt.screenshot(req);
  const wall = Number(process.hrtime.bigint() - a) / 1e6;
  if (i >= 5) rows.push({wall, ...r.stats.timing, bytes: r.image.length});
}
const q = (arr, p) => { const s = [...arr].sort((x, y) => x - y); return s[Math.min(s.length - 1, Math.floor(p * s.length))]; };
const keys = ['wall', 'total', 'render', 'setup', 'wait', 'lifecycle', 'paint', 'raster', 'encode'];
const out = {start: startMs.toFixed(2), n: rows.length, bytes: rows[0].bytes};
for (const k of keys) { const v = rows.map((r) => r[k]).filter((x) => typeof x === 'number'); if (v.length) out[k] = `${q(v, 0.5).toFixed(3)}/${q(v, 0.95).toFixed(3)}`; }
console.log(JSON.stringify(out));
await rt.stop();
```
