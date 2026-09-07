# 截图无用代码清理执行记录（2026-09-07）

本轮按 `screenshot-unused-code-audit-2026-09-07.md` 的根目录清单实施。审计报告是删除前快照；本文件记录实际删除、提交和验证状态。删除授权覆盖耦合残留，采用少量大批次提交，不创建 PR。

## 第一批：SQL、脚本编译缓存和已失去消费者的构建依赖

已实际删除 `sql/`、`components/persistent_cache/`、`components/sqlite_vfs/`、`components/sqlite_proto/`、`third_party/sqlite/`、`third_party/pffft/`。SQLite 的 DEPS 条目和 gitlink、`.gitmodules` 条目一并删除。

同时删除 Blink 的 CodeCacheHost、后台代理、持久缓存代理、脚本缓存元数据处理器、代码缓存 fetcher、WorkerMainScriptLoader 及其 Mojo 协议；清除 DocumentLoader、FrameLoader、ResourceFetcher、ResourceRequestSender、URLLoader 和 ShotURLLoader 的相应接口、成员及调用。普通资源加载、HTTP Simple 缓存和加载冻结/重定向仍保留。

清除 SQL 磁盘缓存开关、工厂分支、实验参数及构建目标。Perfetto 的离线 SQL trace processor 使用它自身的独立开关关闭，并增加可重复应用到 DEPS checkout 的补丁，使 GN 不再求值该目标；本地构建入口与三个平台的 CI 源码准备步骤均应用此补丁。运行时 tracing 并未在这一批删除。

删除 `ui/gl` 的占位 EGL/GLES DLL 目标和 dummy 源码，移除 Vulkan loader 的 data dependency。**Vulkan loader checkout、GPU/GL 主体尚不属于这一批的已删除成果**，继续随后续消费者清理。

### 验证

| 项目 | 实测结果 |
|---|---|
| Windows GN | 8,392 个目标 / 991 个输入构建文件；删除前为 9,164 / 1,214 |
| 缺失输入 | `shot` + `shot_c` 的 7,472 个源码树输入全部存在 |
| Windows 编译 | EXE、DLL 均成功；0 FAILED、0 编译诊断 |
| serve / net | 全部通过，含实际 HTTPS、跨 worker Simple 缓存、HTTP 与 file 图片相同 |
| demos | 62 精确匹配、1 在原有容差内、21 smoke；84 项全通过 |
| 删除前后图片 | 169 张 demos 输出、4 张 render cases、1 张验收 corpus 全部逐字节相同 |
| Node / daemon / daemon protocol | 用本轮重建的 addon 和 DLL 验证，全部通过 |
| Bilibili 离线长文 | 两篇长文、分片与全图、照片和底部二维码全部通过 |
| acceptance | 运行完成；相对既有 Chrome oracle 有 1.5245% 不同像素，但本轮 corpus 与删除前完全相同，差异非本次引入 |
| Linux probe | 构建图解析通过；0 缺少的 BUILD.gn、0 主仓库缺失文件。Windows 宿主工具后缀和未安装的 Linux toolchain/DEPS 不算 Linux 实编译 |
| macOS / 六平台实编译 | 尚待最终批次提交后验证；本机通过不代表六平台通过 |

验证二进制：EXE SHA256 `b7ba1bc0eaed9072765846321ffc3ee56057d315dce6b7616de391024c1713f5`；DLL SHA256 `bf66377ef21dd41500e9bbb1db2bcfac2bcd9f03b4e2f7b82920e3ed5fddbbbb`。

中间曾遇到临时源码复制保留旧时间戳，导致 Ninja 漏编加载器对象、运行时新旧接口混用。已刷新所有修改过的 C++ 文件和头文件时间戳、重新构建，并通过上述运行检查。后续临时区改动使用重新写入内容的方式落盘，不沿用临时文件时间戳。

第一批提交：`71ebd6e65771`。

## 第二批：硬件认证、脚本缓存传输和失效构建骨架

删除设备绑定会话的整个网络实现、协议、traits、URLRequest/HTTPJob/Blink 调用链，以及 `components/unexportable_keys`、TPM Rust 解析器、不可导出密钥、用户验证密钥和专用 Apple Keychain 包装。正常 HTTPS、系统证书、TLS 私钥适配和基本密码算法保留。WebFeature 中三个历史编号仅为枚举身份，不再对应实现。

继续清除脚本编译缓存的传输尾巴：URLLoaderClient 的缓存载荷参数、各级回调/转发、Resource 的缓存元数据方法和统计、JS 源码哈希开关均删除。字体响应处理仍处理字体内容本身。

实际删除根目录 `content/`、`google_apis/`、`extensions/`；删除 components 下 15 组只有构建骨架的浏览器设施，以及 ui 下 aura、compositor、menus、webui 等空壳、cert_verifier/test 服务空壳。对应 import、deps、构建开关和 WebUI 资源条件一起清理。

删除 Mojo JS/TS/Fuzzilli 生成器、目标、调用选项、预编译模板、测试目标的失效生成依赖，以及 `mojo/public/js/`、`tools/typescript/`。C++、Rust 和仍有调用的生成工具保留。

| 项目 | 第二批实测结果 |
|---|---|
| Windows GN | 6,988 个目标 / 874 个输入构建文件 |
| 缺失输入 | 7,466 个源码树输入全部存在 |
| Windows 编译 | EXE / DLL 均通过，0 FAILED、0 编译诊断 |
| serve / net / demos | 全通过；84 个 demos 全通过，含实际 HTTPS |
| 删除前后图片 | 169 张 demos、4 张 render cases、1 张 corpus 共 174 张全部逐字节相同 |
| Node / daemon / daemon protocol / Bilibili | 用本轮重建的 addon 和 DLL，全部通过 |
| Linux probe | 0 缺少的 BUILD.gn、0 主仓库缺失文件；宿主后缀及 3 个未安装的 Linux DEPS 仍只是本机探测限制 |
| 六平台实编译 | 待最终批次验证，不把本地测试提升为跨平台通过 |

第二批日志前缀：`out/Shot/cut-batch2-combined-`；图片证据：`out/cut-batch2-combined/pixel-comparison.json`。

第二批提交：`f48e852f6cb8`。

## 第三批：媒体、旧缓存、设备/打印与版本构建残留

已实际删除根目录 `device/`、`printing/`、`chrome/`、`media/`，以及 `cc/benchmarks/`、`net/disk_cache/blockfile/`、`third_party/closure_compiler/`。Chrome 版本读取改为现有 `shot/VERSION`，macOS plist 移至 `shot/app-Info.plist`。打印预设协议删除；页面缩放仍用 Blink 自己的同值枚举，不改变 CSS 打印布局。

删除微基准控制器的成员与调用、无消费者的 dropped-frame 共享内存和视频粗糙度统计，删除媒体播放/音频设备/解密/捕获接口及其导出包装、线程接口、无播放器的绘制分支。保留 video 的海报图片与 HTML 布局，保留普通 MIME 分类。Simple 和内存缓存仍在，旧 blockfile 工厂、枚举、测试等待入口及实现一并清理。Protobuf JS/TS 的 GN 与 Python 调用链同步删除。

删除实体文件前，先编译修改后的消费者，确认候选未被其他工作修改，再核对 EXE/DLL 的 Ninja 输入和 GN 重生成输入均不读取候选。第三批的两组实体删除清单分别为 35 和 432 个文件。

| 项目 | 第三批实测结果 |
|---|---|
| Windows GN | 6,945 个目标 / 867 个输入构建文件 |
| 删除后的缺失输入检查 | 7,464 个源码树输入全部存在 |
| Windows EXE / DLL | 均通过；最后一次构建 0 FAILED / 0 编译诊断 |
| serve / net / demos | 全通过；84 项 demos，含实际 HTTPS |
| 像素比较 | 169 张 demos + 4 张 render + 1 张 corpus，共 174 张逐字节相同 |
| Node / daemon / 协议 / Bilibili | 本轮 DLL 与重建 addon，全部通过 |
| acceptance | 与原有 Chrome oracle 的 1.5245% 差异未扩大；与本轮删除前 corpus 逐字节相同 |
| IDL 枚举检查 | 155 个引用值，0 个未生成 |
| Linux probe | 0 缺 BUILD.gn / 0 主仓库缺失输入；本机宿主及未安装 Linux DEPS 限制同前 |
| 六平台实编译 | 尚未完成，不把本机结果当跨平台证明 |

第一次高并发构建出现 LLVM 内存耗尽，降低并发后通过；随后媒体拆除暴露的残留调用与间接 include 已修复并重编译。没有通过重新引入 media 或 blockfile 消除错误。

日志前缀：`out/Shot/cut-media-`；删除证明与像素证据：`out/cut-stage4/deletion-proof.json`、`out/cut-stage5/deletion-proof.json`、`out/cut-stage5/pixel-comparison.json`。

## 第四批：Canvas 绘图、服务空壳和采样 profiler

删除 Web Canvas 绘图上下文、context host/factory、字体与性能缓存、专用 layout/painter、ImageData/TextMetrics/ElementImage 等脚本值类型及生成绑定，以及资源 provider、drawing buffer、GPU bitmap、Offscreen 占位和绘制事件队列。`core/html/canvas` 只剩 HTML 标签的 `.cc/.h/.idl` 三文件。普通图片与 SVG 继续使用的 `ImageElementBase` 迁到 `core/html`；SVG 序列化直接读取图片响应 URL。

`<canvas>` 保留备用内容、属性宽高比、已有静态布局行为。直接改成普通元素的研究样本出现差异，本批保留最小兼容后两套专项样本均与原实现逐像素相同。Skia CPU SkCanvas、SVG、图片与字体绘制保留。

合并删除 `sandbox/`、`storage/`、代理解析服务协议和 storage/quarantine 服务骨架；删除栈采样、堆采样和线程池 profiler 钩子、企业管理与默认应用工具。真实 tracing 消费者仍使用的五个 module cache 文件留到诊断后端批次处理。网络 cookie 值类型和受限接口保留，删除没有服务实现的特权 CookieManager 接口。

实际删除 225 个普通文件、34 个核对过的空目录，路径及删除前 hash 位于 `out/cut-batch4/deletion-proof.json`。这些路径没有独立 DEPS/gitlink 入口。公共 paint/property tree 的 Canvas 子树状态和浏览器 compositor 的共享 GPU 接口尚未全部拆除，已列入后续整批；不把本批结果称作整个 GPU 或合成器已删除。

| 验证 | 结果 |
|---|---|
| Windows GN / 输入 | 6,871 targets、858 files；shot + shot_c 的 7,459 个源码输入全部存在 |
| IDL | 131 个枚举值引用、43 个 union 名称引用均无缺失 |
| EXE / DLL | 两者编译通过，0 FAILED edge；失败 TU 的间接 include、SVG URL 调用和 Canvas 输入尾巴已修复，syntax-only 通过 |
| serve / net | 全部通过，包括真实 HTTPS 和网络/文件像素一致性 |
| demos | 84 组通过：62 exact、1 fuzzy、21 smoke |
| Node / daemon / 协议 | 使用重新构建的 addon 和同 hash 新 DLL，全部通过 |
| Bilibili | 两篇离线长文、分片、照片和二维码全部通过 |
| 原始像素对照 | 169 demos + 4 render cases + corpus + 2 Canvas 专项，共 176/176 逐像素一致 |
| acceptance | 完成；既有 Chrome oracle 差异约 1.5245%，本次 corpus 与裁剪前完全一致 |
| Linux probe | 0 缺 BUILD、0 主仓库缺失输入；宿主工具后缀及未装 Linux DEPS/toolchain 不代表 Linux 实编译 |
| 六平台实际构建 | 尚未完成，后续最终提交验证 |

EXE SHA256：`2d13e25a508001c0959415e09dc180353846bc0c02aa55fe71c654ae6b9b0288`；DLL SHA256：`3cb9c9088092cfe50f05c510fa9e9dc2ab1b0336404ae12bd9cf16c07af52028`。日志、二进制证据和像素清单位于 `out/cut-batch4`。

并发记录：用户授权临时 31 并发，实试出现 LLVM OOM 后回退 8 完成本批。用户随后清理后台并指定后续用 20 并发，下一批采用 20；没有修改项目永久默认并发。

## 第五批：直连网络路径和 prefs

实际删除 116 个文件：68 个代理解析/配置服务文件和 48 个 prefs 文件。删除 PAC 下载、WPAD/DHCP、系统代理配置与监听、解析线程、平台 resolver、ProxyResolutionService/Request 接口，以及 URLRequestContext/HttpNetworkSession 的持有、关闭和成功通知链。HTTP JobController 直接选择 DIRECT，删除为异步代理解析和切换代理而设的状态机。同步清除所有平台 GN source 项、prefs 依赖、WinHTTP/DHCP 库链接和仅 Linux 代理配置需要的 GLib 依赖。

正常 DNS 网络变化通知、TLS/系统证书、HTTP2、重定向、缓存、资源加载和对外 worker/daemon/API 保留。剩余 11 个 proxy_resolution 文件是 HTTP/认证匹配或网络 Mojom 还使用的配置、列表、结果及重试值类型；这些协议与代理 HTTP socket 耦合仍在任务清单中，不冒充整个代理相关代码已经清完。

| 验证 | 结果 |
|---|---|
| GN / 输入 | 6,868 targets、857 files；7,458 个 shot + shot_c 源码输入全部存在 |
| Windows EXE / DLL | 均用用户指定 20 并发通过，无 OOM；一处注解显式转换错误修复后 syntax-only 1/1 通过，再完成增量构建 |
| serve / net | 全部通过，包括拒绝请求、重定向/并发限制、跨 worker 缓存和真实 HTTPS |
| demos | 84 组通过：62 exact、1 fuzzy、21 smoke |
| Node / daemon / 协议 | 新 addon 与新 DLL，全部通过 |
| Bilibili / acceptance | 离线长文全部通过；原有 Chrome oracle 差异约 1.5245%，本批没有扩大 |
| 原始像素对照 | 176/176 逐像素相同，包括两套 Canvas 样本 |
| Linux probe | 图解析通过、0 缺 BUILD、0 主仓库缺失输入；3 个未检出 Linux DEPS 和 1 个宿主工具链文件不计作实编译通过 |
| 六平台实编译 | 尚未完成 |

EXE SHA256：`0f4267166f8bf876b8e18e3f7df6f36359d86635cf525cf4733c37ddc18d8cf4`；DLL SHA256：`a8a6f4655e910b7cf95771c1c87b8b432c4954e430e0db626565d17429bd7ad4`。删除与构建图证据在 `out/cut-stage9`，运行及像素证据在 `out/cut-batch5`，编译日志为 `out/Shot/cut-batch5-*.log`。

下一轮旧 IPC 调查已确认五处 Mojo `[Native]` 声明，其中网络 RedirectInfo 和连接类型通过旧 ParamTraits 序列化。拆旧 IPC 前必须迁移这些协议字段/枚举及验证 traits，不能只删 include 或禁用校验。

## 第六批：浏览器宿主、弹窗、插件和合成器入口

已完成源码裁剪与 Windows 运行验证，实体删除 249 个文件；同时删除对应 GN/Mojo/IDL 源项和调用，清除 10 个空目录，包括整个 Blink platform/widget。此批不涉及新的 DEPS 检出包。

| 范围 | 实际处理与保留边界 |
|---|---|
| 浏览器嵌入层 | 删除 WebViewImpl、WebFrameWidgetImpl、WebPagePopupImpl、Local/RemoteFrameImpl、ChromeClientImpl 和浏览器 FrameClient 实现，以及 DevTools 屏幕模拟、Find、序列化等失去入口的实现。Shot 继续直接创建 Page/Frame，保留真实生命周期和最小 EmptyClients；public WebFrame/WebView 接口尾巴仍待后续处理。 |
| 弹窗、选择器与插件 | 删除平台 widget、上下文菜单、日期/颜色/选项弹窗、文件选择、插件容器/文档/注册表、外部插件处理和协议。保留表单外观/属性/有效性、object/embed 的图片或子文档加载、安全检查和备用内容；HTMLPlugInElement 的这部分仍有截图用途。 |
| 合成器与滚动 | 删除 PaintArtifactCompositor、PropertyTreeManager、PendingLayer、ContentLayerClientImpl、ScrollingCoordinator、LinkHighlight、VisualViewport 的 cc 图层，以及主合成宿主/触发器挂接和提交回调。主线程滚动状态迁入 ScrollAnimationState，保留曲线、坐标换算、clamp、完成/取消、调度失败即时滚动。CPU paint chunks 转换与真实 property tree 保留。 |
| 浏览器诊断 | 删除 BlinkLeakDetector 与专用清理入口、ImageElementTiming/TextElementTiming/ElementTimingUtils，以及失去消费者的呈现回调生产端。保留当前 LCP/FCP 记录和真实加载完成条件；ContainerTiming/Performance 等后续仍要继续拆。 |

普通 HTML/CSS、SVG/SMIL、图片、字体、MathML、CPU 绘制和公开 Shot API 保持。没有恢复 V8 或创建浏览器/GPU 空适配层。文件夹残留要按实际内容判断：platform/widget 已实际消失，graphics/compositing 仍保留 Shot 使用的 paint_chunks_to_cc_layer 与 chunk_to_layer_mapper；整个 gpu/cc/ui 的最终收口尚未完成。

| 验证 | 结果 |
|---|---|
| Windows GN / 输入 | 6868 targets、857 build files；7448 个源码输入全部存在。修正了 shot_sources.gni 对已删 fullscreen_video_element.mojom 的遗漏。 |
| Windows EXE / DLL | 均通过。按用户要求实试 20 并发时 LLVM OOM，并收集到普通 C++ 错误；批量修复后 26/26 失败 TU 的 syntax-only 通过，改用 8 并发完成剩余构建，无 OOM。 |
| serve / net / demos | 全部通过；84 demos 为 62 exact、1 fuzzy、21 smoke，真实 HTTPS 与缓存/重定向检查通过。 |
| Node / daemon / 协议 | 新 addon、新 DLL，全部通过。 |
| Bilibili / acceptance | 离线长页及图片/分片检查通过；原有 Chrome oracle 差异约 1.5245%，本批没有增加。 |
| 原始像素对照 | 177/177 解码后逐像素相同；原有 176 张加 16 个 object/embed/表单情况的专用页面。 |
| Linux probe | 图解析通过；0 缺 BUILD、0 主仓库缺失输入；3 个 Linux DEPS 检出和 1 个宿主工具链文件未具备，此项不算实编译。 |
| 六平台实编译 | 尚未完成，不能从 Windows 与 Linux probe 外推。 |

EXE 为 46,838,784 字节（较第五批减少 1,134,592 字节），SHA256：`9989fced172bb90ea3f24eaeb0cea9da865e24e7d7d5b7148a324b4fa5ef089f`。DLL 为 46,836,736 字节，SHA256：`b882dc9c7c42e00a4bc884db0a2eca7b39de6084258fb49d8f1a979e7ad0a2fa`。源码/删除证据在 out/cut-stage10；构建日志在 out/Shot/cut-batch6-*.log；运行、177 张对照和二进制证据在 out/cut-batch6。文件行数删除不等于同等二进制收益，以上大小来自真实产物。

## 第七批：输入、编辑命令、Autofill 与系统剪贴板

本批实体删除 216 文件，并同步调用方、IDL、生成类型、运行开关、GN 源列表和 Mojo 类型映射。components/input 的 80 文件、Blink IME/EditContext、公共浏览器宿主头文件、Widget 输入协议、CSS selector watcher、Autofill 事件和专用表单缓存、execCommand 与格式化命令、拼写检查/文字建议、SystemClipboard 及其协议已删除。

保留影响截图的表单关联与校验、disabled fieldset/legend、radio group、shadow reference target 遍历、plaintext-only 空白处理和真实焦点/选择状态。系统剪贴板消失后 DataObjectItem 的文件令牌克隆补上直接 Mojo include；删除样式回调字段后，样式生成器的无用对齐类型一起移除。未恢复浏览器实现。

| 验证 | 结果 |
|---|---|
| GN / 输入 | 6847 targets / 856 build files；7431 个源码输入全部存在 |
| Windows EXE / DLL | jobs 8 编译链接成功；样式生成器与直接 include 问题已修复，失败 TU 1/1 syntax clean，无 OOM |
| IDL enum | dry-run 检查 54 枚举、122 引用值，0 缺失 |
| serve / net / demos | 全通过；62 exact / 1 fuzzy / 21 smoke，84 demos |
| Node / daemon / 协议 | 新 addon 与相同 SHA256 的新 DLL，全部通过 |
| Bilibili / accept | 全通过；原有 Chrome oracle 差异约 1.5245% 保持 |
| PNG 原始对照 | 179/179 解码像素完全相同；新增输入框和表单关联基准 |
| Linux probe | 0 缺 BUILD / 0 主仓库缺输入；3 Linux DEPS 和1宿主工具链缺项仍在 |
| Linux Jumbo | 扫描列出40个符号候选、部分生成源码不可用；不是编译结果，六平台实编译仍未完成 |

EXE 46,483,968 字节，较第六批减少 354,816 字节；SHA256 835d04bd053c17b507f7cae54198de70183a0d4b2ec9ae6d23665facc67a470f。DLL 46,481,920 字节，SHA256 6d4912d96a203e3b41b45da83fb4e7d5a4d1ca603d40e3e263676de3d5420486。out/cut-stage11 保存 owned/delete/hash 证明，out/cut-batch7 保存运行与179张像素结果，out/Shot/cut-batch7-*.log 保存构建日志。5个空目录已精确非递归移除，components/input 与 Blink editing/ime、spellcheck、suggestion 实际不再存在。

拖放链尚未删除：自动审批两次拒绝14文件提案，当前仍等待用户明确确认。DataTransfer/DragController 及通用输入尾巴不能列为已完成，也不能将这一批等同整个根目录清理结束。

## 第八批：AnimationWorklet、NativePaint 与延迟图片记录

实体删除74文件，调用方、成员/生命周期、运行开关和GN源项同步处理。无新DEPS包变更。删除内容包括 Blink/cc AnimationWorklet 调度与时间同步、背景色/box-shadow/clip-path原生PaintWorklet生成器和状态、CSS paint()解析/样式缓存、跨线程CSS值与派发、Canvas内存记录器尾巴，以及Host/Proxy/Scheduler的Worklet异步派发和等待。

同时删除自定义/原生Worklet属性动画与TargetPropertyId元数据、颜色曲线回调、tracker和专用帧指标、PaintWorkletInput/DeferredPaintRecord、图层/瓦片图片记录映射、PaintWorkletImageProvider及PaintImage/Shader/绘制指令/序列化延迟分支。普通CPU PaintRecord、图片解码/动画/HDR、主线程CSS动画、背景色/阴影、SVG与shape裁剪保留。裁剪矩形收窄到当前CPU布局范围，保留pixel snapping、圆角和overlay scrollbar范围；同步图片失效到栅格准备/activation的顺序门控仍保留。

| 验证 | 结果 |
|---|---|
| 静态删除闭包 | 74份备份SHA256通过；16,853 tracked源码/GN无删除路径残余；126个owned C++/头文件预处理配对通过 |
| GN / 输入 / IDL | 6847 targets/856 files；7429输入全存在；dry-run54枚举122引用0缺失 |
| Windows EXE / DLL | jobs8编译链接成功；首轮3个TU直接include/旧Worklet绑定问题批量修复，3/3 syntax clean |
| serve / net / demos | 全通过；84 demos为62 exact/1 fuzzy/21 smoke |
| Node / daemon / 协议 | 新addon和相同SHA256的新DLL，全部通过 |
| Bilibili / accept | 全通过；既有Chrome oracle差异约1.524%保持 |
| 原始像素对照 | 181/181解码RGBA完全一致：原179张加NativePaint静态和CSS paint() fallback专项 |
| 动态clip-path专项 | 旧版该fixture退出无图；新版输出成功，与等价静态中点参考0像素差 |
| Linux probe / Jumbo | 0缺BUILD/0主仓库缺输入；3项Linux DEPS和1项宿主工具链缺失；40个Jumbo候选，不计作编译通过 |
| 六平台实际编译 | 尚未完成 |

新增NativePaint基准含12组背景/透明/阴影/shape/SVG裁剪；CSS Paint基准含6组paint()回退、参数、@supports、mask与gradient。后者旧版本地页面与unsupported-paint()参考完全一致，不能外推所有安全上下文；移除JS注册依赖的paint()后仍按无效声明回退。旧EXE/HTML/PNG出处及SHA256在out/cut-native-paint、out/cut-css-paint的provenance.json，181张对照和动态专项结果在out/cut-batch8。

EXE46,414,336字节，较第七批减少69,632字节；SHA256：3429614bb621a7c55ba505e1c0a913561a7f34681b63e1c924281ce0f0ea6472。DLL46,411,264字节，较第七批减少70,656字节；SHA256：30804661fdf9d119061d256fa47053050edde6fd3f49ebb9cd49491f23750576。源码证据在out/cut-stage12；构建日志在out/Shot/cut-batch8-*.log；完整运行证据在out/cut-batch8/validation.json。Linux probe有效输出目录为out/CutBatch8Linux；首次嵌套目录导致脚本相对路径误报，重跑已排除。

泛用Worker/Worklet公共token、网络destination、IDL暴露声明仍待下一闭包；普通CPU PendingAnimations/PreCommit逻辑不能误删。拖放提案仍未应用。本批完成不代表cc/GPU或整个根目录清理已经结束。

## 第九批：公共脚本协议与后台调度

删除20文件：FileReader/Sync脚本入口和返回union8文件、WorkerScheduler/页面代理/脚本队列7文件、合成器线程和专用scheduler5文件。无实际创建方的Worklet/ShadowRealm/V8/WebNN token与Mojom/traits/GN映射、Worklet destination、CSSOM暴露声明，以及V8三类任务队列/runner/统计分类、Worker生命周期限流和专用队列参数同步移除。持久枚举的现有值不重编号。

字体处理和HTML预扫描实际调用NonMainThread::CreateThread，保留其默认/控制/idle任务队列、任务完成回调、GC和清理顺序。去掉原先无消费者的音频实时参数后仍使用相同默认线程优先级与message pump。共享FileReaderLoader仍被DataObject的blob读取使用，本批只删除脚本API；未改动未获批的拖放提案。Worker公共token及其网络/GPU协议仍有后续工作。

| 验证 | 结果 |
|---|---|
| 静态删除闭包 | 20份删除备份SHA256通过；16,835 tracked源码/GN无删除路径残余；42个owned C++/头文件预处理配对通过 |
| GN / 输入 / IDL | 6847 targets/856 files；7429输入全存在；dry-run枚举54个/122引用和union42引用均0缺失 |
| Windows EXE / DLL | jobs8编译链接成功；首轮2个失败TU已修复，2/2 syntax clean，无OOM |
| serve / net / demos | 全通过；62 exact/1 fuzzy/21 smoke，共84 demos |
| Node / daemon / 协议 | 新addon加载相同SHA256的新DLL，全部通过 |
| Bilibili / accept | 全通过；既有Chrome oracle差异1.524%保持 |
| 原始像素 / 动态clip-path | 181/181解码RGBA完全相同；clip-path与静态中点参考相同 |
| Linux probe / Jumbo | 0缺BUILD/0主仓库缺输入；3项Linux DEPS和1项宿主工具链缺失，40个Jumbo候选；未实编译 |
| 六平台实际编译 | 尚未完成 |

EXE46,395,392字节，较第八批减少18,944字节，SHA256 225b15334de211e7c1216bbfcbe78bf51b749d8e2b63acf164384a8317bca0d4。DLL46,393,344字节，减少17,920字节，SHA256 e4563b7c2a6fdbd4d8760c46881e7429d8a001f1515ce9017b50861a4879dd21。首轮错误为RequestDestination映射校验遗漏删除后保留的2/11编号，以及ThreadScheduler直接include；没有恢复已删除入口。源码证据在out/cut-stage13，完整运行/二进制/像素证据在out/cut-batch9，编译日志在out/Shot/cut-batch9-*.log。

下一批16组CPU动画几何与暂停时序基准已由本批新EXE生成，两次像素相同并目视检查；保存于out/cut-animation-cpu，尚未用于验证下一批。普通动画GPU状态、cc/Viz/GPU与网络公共层仍需继续拆除。

## 第十批：普通动画GPU状态与CPU变换快照

共101个变更路径，其中22个实体删除、3个CPU变换快照新文件。移除普通动画的GPU状态、分组/回执、CompositorAnimation包装/委托、eligibility及曲线桥接、合成时间线镜像与同步、非变换专用关键帧快照。七个GPU动画样式标记、属性树AnimationState参数和GPU专用变化分类、锚点GPU动画状态传播、四个运行开关和遗留测试源项一并清除。

CPU动画排队、开始/暂停就绪、未解析滚动时间线延迟、PaintClean后时序延迟和原IsCurrent查询的on-demand副作用保留。仅为transform/translate/rotate/scale保存TransformOperations，用于CPU子像素和轴对齐；neutral keyframes、zoom和viewport刷新保持。SVG原点分离的SMIL/资源祖先/zoom/vector-effect/额外变换判定原样迁入CPU绘制调用方。ScrollOffsets、Timing枚举为Blink本地数据，滚动时间线16微秒/像素不变。cc的实际CPU平滑滚动曲线仍保留；宿主/GPU全链后续继续拆除。

| 验证 | 结果 |
|---|---|
| 静态删除闭包 | 22份删除与79份编辑备份SHA256通过；17510个源码/GN/生成输入无删除路径引用，71个owned C++/h预处理配对通过 |
| GN / 输入 / IDL | 6847 targets/856 files；7429输入全存在；54枚举122引用、union42引用均0缺失 |
| Windows EXE / DLL | jobs8编译链接通过，无OOM；首轮14个失败TU已集中修复，语法13/14+最后1/1通过 |
| serve / net / demos | 全通过；84 demos为62 exact/1 fuzzy/21 smoke |
| Node / daemon / 协议 | 新addon加载相同SHA256的新DLL，全通过 |
| Bilibili / accept | 全通过；既有Chrome oracle差异1.524%保持 |
| 原始像素 / 动态clip-path | 182/182解码RGBA完全相同；动态clip-path等于静态中点 |
| Linux probe / Jumbo | 0缺BUILD/0主仓库缺输入；3项Linux DEPS和1项宿主工具链缺失，40个候选，未实编译 |
| 六平台实际编译 | 尚未完成 |

新增动画基准含16组几何、暂停、delay/fill、zoom、SVG，旧第九批EXE生成的基准与出处保存在out/cut-animation-cpu。原181张基准SHA256未改变，本批追加一张专项后共182张。

EXE46,332,928字节，DLL46,330,880字节，分别比第九批减少62,464字节。EXE SHA256 b54ec426000f4f9ace00a2d9efc9c13c342f9308a2439e405dbc7444bcb83c31；DLL SHA256 83a1dd6731c0a9746f81a07cb1b96c82812bae37857bcbd00d4276f1a04191e5。源码证据out/cut-stage14，全部验证及像素证据out/cut-batch10，编译日志out/Shot/cut-batch10-*.log。首次诊断涉及直接声明/include、快照谓词、ScrollAxis遮蔽和三个旧参数；全部修复后未恢复被删功能。拖放提案未应用。

## 后续批次

继续处理网络公共层、输入/合成器/GPU、诊断后端等剩余闭包，完整接续清单见 `screenshot-cut-task.md`。不把待处理或已关闭开关标为彻底删除。

## 最新接续状态：第十一批验证通过，待提交

本段优先于后面的历史进度。基准提交 cd916149eb10080a0c176826e9308f96ef5dd82d；第十一批使用 out/cut-stage15，共 229 个 owned 路径（225 源码/构建输入、4 文档）和 576 个实体删除，总计 805 个变更路径。当前全部 Windows 验证通过、尚未提交；所有构建/测试会话已结束。

已移除 View transition 创建入口、Document/DocumentLoader/LocalFrameView/PageAnimator 生命周期、样式调度/显示锁、DOM 伪树、布局/绘制/属性树、跨文档协议/traits、事件/字典和 GN；ForeignLayerDisplayItem 及调用方、无调用的图层/图片缓存入口和 AddToLayerDebugInfo 等也已删除。原 EnqueuePageRevealEvent 的 RouteMap 导航初始化副作用迁为 InitializeRouteNavigationState，仍在原调用时点执行。

CC 的 LayerTreeHost/Impl、layers、scheduler、raster、GPU tiles、帧率/合成指标、动画宿主/时间线、mojo_embedder、ViewTransitionRequest 已实体删除，主目标收窄为 CPU 几何/scroll-snap/sticky 算法和当前公共值类型。动画目标仅保留两种 CPU 滚动曲线及头文件/export。Viz BindLayerContext/LayerContextSettings、layer/layer_context/tiling 及八组 CC 图层协议/traits、旧测试和 Android target 同时移除；基础/调试/资源/指标尾巴另删除 46 文件。保留 CPU paint、图片、滤镜、绘制容器和 StickyPositionConstraint::CanMerge；其余 Viz/GPU、公共帧元数据与浏览器控制参数仍需下一批收窄。

CanvasChildPaintRecord/State、CanvasDrawElement/CanvasForDrawing、ElementCanvasTransform、drawable 子树、专用合成原因及调用方全部移除。唯一隐私绘制标记 producer 已随 Canvas 绘图分支删除，继而清除 kPrivacyPreserving、tainted 属性树传播及不可达绘制分支；普通 CORS/资源来源检查、SVG 滤镜和 HTML/CSS 绘制保留。HTMLCanvasElement 仍保留标签备用内容、属性宽高比及原有布局；PaintLayerPainter 对 Canvas 子分层仍取原 feature-disabled 路径。HTMLMediaElement 空 cc::Layer 与 LayoutVideo 加速判断删除，video poster 保留。AnchorPositionScrollData 比较载荷迁为 Blink AnchorScrollSnapshot，字段、默认值与实际布局计算不变。

元素/区域捕获及追踪已贯通删除：Element/NodeRareData ID 与 subrect 存储、资格判断、强制 effect、密码/iframe 坐标 producer、布局强制 box、HTML/SVG painter、PaintController/Chunker、PaintChunk 成员和 GC/比较/调试输出、CC/Viz 帧字段、CopyOutput 元数据、Mojo/traits/typemap 和 GN。无外部调用的 VideoCaptureTarget 与 CopyOutputBitmapWithMetadata 一并移除；密码控件状态/显示、iframe 正常属性与 srcdoc、实际截图输出保留。浏览器 ImageReplacement 的工厂无调用方，专用远程 iframe、图片布局分支、生命周期/事件、Mojo、开关和八个文件已移除，普通图片加载、解码、object-fit 与失败替代文本不变。

过渡 CSS 尾巴再删除六文件：CSSOM 包装/StyleRuleViewTransition、IDL 和专用 transition.css UA 资源；同步移除规则创建/存储/GC/复制、navigation/types 私有 descriptor、GN/GRD。@view-transition 通过通用未知 at-rule 跳过；普通 view-transition-name/group/class/scope 属性、选择器识别与静态分组/@supports 保留。TwoPhaseViewTransition 尚被普通导航解析使用，本批未改该导航语义。同步分歧已写 docs/upstream-sync.md。

所有实体删除均先确认工作区内普通文件，写精确备份并校验 SHA256 后逐文件删除。清单与证据在 out/cut-stage15 的 manifest.json、deletions.json、deletion-proof.json。最终静态证明：229 owned / 576 删除、16935 个输入扫描、186 个 C++/头文件预处理配对，0 问题。首次备份与基准提交无实质内容差异（11 个仅换行格式差异）。构建图 GN 6839 targets / 852 files、7409 个输入全部存在，IDL 枚举/union dry-run 无缺失。验证后按精确路径非递归删除 10 个空目录，清单 out/cut-batch11/empty-directories.json。

原第十批 182 张基准加 out/cut-view-transition/baseline.png，共 183 张不可覆盖。新增过渡基准含 @view-transition 后普通规则、九组属性/层叠/scope/@supports/变换/裁剪/SVG，来自第十批已验证 EXE（SHA256 b54ec426000f4f9ace00a2d9efc9c13c342f9308a2439e405dbc7444bcb83c31），两次渲染一致并目视检查。原始基准哈希保持不变，当前新二进制已完成全部逐像素对照。

本批已照用户最新要求实际运行 jobs 20，但出现 20 次 LLVM OOM 和 PowerShell 宿主 OOM，保存 out/Shot/cut-batch11-cpp-oom.log 后改用 8。源码错误集中修复并完成当前图 79/79 单元语法检查；续编发现的过渡滚动角残留整链删除，相关 2/2 单元通过，普通滚动条/滚动角 CPU 绘制保留。随后完整编译成功，不改变永久构建默认。

Windows EXE/DLL 均以 jobs 8 编译链接成功，0 失败 edge；新 addon 已重建，旁置 DLL/资源与 out/Shot 哈希一致。serve、net、84 demos（62 exact / 1 fuzzy / 21 smoke）、Node、daemon、协议、Bilibili 离线长页与 accept 全部通过。183/183 张 PNG 解码像素完全一致，动态 clip-path 与等价静态中点完全一致；Chrome oracle 差异仍为 1.524%。EXE 45,542,912 字节，较第十批减少 790,016 字节；DLL 45,540,864 字节。

EXE SHA256：7aee9e13c9505a25a66a99e1f1040fc924434d35146af8e6a5bca4d06667a2d3；DLL SHA256：881a850e5a2a5b8a6a18211f3d0bdf69d29f8012b2936616d05979fe4176a2b5。证据、日志和像素比较位于 out/cut-batch11；最终 EXE 日志 cut-batch11-build-final.log，DLL 日志 cut-batch11-dll.log。

Linux probe 为 0 缺 BUILD、0 主仓库缺输入，仍有 3 个 Linux DEPS 检出项和 1 个宿主工具链输入缺失；jumbo 静态扫描列出 39 个候选，部分平台/生成输入未扫描。这是图与静态证据，六平台实际编译仍未完成。其余 Viz/GPU、协议、网络/诊断、第三方和根目录总复核继续处理，整个目标 active。

拖放 14 文件提案未应用，自动审批曾拒绝且尚未获用户确认；范围在 out/cut-stage11/drag-proposal-review/scope.json。当前独立裁剪不包含该提案，不得重试或拆分。不得创建 PR、push 或发布。
