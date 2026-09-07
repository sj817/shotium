# 静态截图裁剪任务：完整目标与接续清单

## 目标与授权

用户要求完成根目录审计报告中的全部状态：无用实现、仍有引用但截图不用的耦合残留，以及待进一步确认的候选，都必须有实际处理结果。不能只把构建开关关闭、删掉一层调用或把“仍被引用”写成永久保留理由。

依据：`docs/screenshot-unused-code-audit-2026-09-07.md` 是删除前完整快照（根目录及子目录附录）；`docs/screenshot-cut-execution-2026-09-07.md` 记录完成批次和验证证据。本文件是当前任务的接续入口，不替代这两份报告。

2026-09-07 用户确认：Canvas 删除绘图系统，保留无 JS 下影响截图的最小 HTML 兼容；继续全部剩余工作，分批 commit、尽量少量大提交、不创建 PR。用户随后要求完整记录目标，并先完成整个大批次再集中编译，避免频繁构建。本任务不包含发布新版本。

## 不可丢失的产品边界

- 保留 HTML/CSS/DOM、布局、SVG、MathML、文本塑形和字体、图片解码、CPU 栅格化、PNG/JPEG/WebP 输出、文件与 HTTP(S) 加载。
- 保留当前公共 API、CLI、C ABI、Node addon、worker/daemon 协议、缓存、分片截图、加载等待和 CaptureStats。
- V8、content、GPU 进程、DevTools 不恢复；不改变字体栅格化、不启用 `-Oz`。
- Canvas 的绘图上下文、GPU 资源、脚本绘图入口删除；标签备用内容、属性宽高比、已有静态布局行为保留。Skia 的 SkCanvas 是整页 CPU 绘制设施，不属于 Web Canvas 删除范围。
- CSS 动画取值、滚动布局、表单默认外观、lazy loading、XML/XSLT 等不能因“无 JS”一概删除；逐调用点确定实际声明式行为。
- 保护共享工作区的既有修改：Skia、Perfetto、Vulkan loader checkout 中已有局部改动；逐文件核对，不能全仓 stage、reset 或 clean。只暂存本任务明确拥有的路径。

## 完成定义

每一个 A/B 候选必须删除完整功能闭包，或提供新的实际截图调用证据并明确收窄后保留的最小部分。C 候选必须完成调查并落到“删除”或“保留哪些行为、为什么”。不得以“耦合复杂”“以后再处理”作为完成状态。

删除的闭包包括：公开入口、调用者、成员/生命周期、实现、协议/traits、生成器输入、GN target/import/deps/data_deps、DEPS/gitlink/.gitmodules、同步 hook 和磁盘文件。第三方包不能只删本地目录，否则 gclient 会重新检出。

保留目录必须说明其实际职责与剩余子目录，而非只说“底层”。最终按根目录回填状态，并附全仓残留搜索、构建闭包与验证证据。构建图通过、编译通过、真实运行检查通过严格分开；没有完成的六平台编译不能标绿。

## 构建与提交节奏

1. 先完成一个完整的大批次：调用方、接口、实现、构建配置、实体删除及静态残留检查集中处理。
2. 批内不为每个文件/目录反复启动完整构建。到批次边界集中执行 GN、缺失输入检查、必要语法检查、EXE/DLL 编译与运行验证。
3. 出现编译错误，先收集同批完整错误集合，再按失败 TU 做 syntax-only 修复；只有前端错误解决后才继续完整构建。
4. Windows 构建只能 `pnpm build:engine`、只能 `out/Shot`。用户最新指令：清理后台内存后，后续构建临时采用 jobs 20。此前已实试 jobs 31，但 LLVM OOM 后回退 8，当前批次 EXE/DLL 已通过。下一批用 20，监测内存并在实际 OOM 时降低并发；不修改项目永久默认并发。
5. 每批完成后用显式拥有路径清单提交；尽量合并相邻低风险链，避免一个目录一个 commit。
6. 每次任务接续先读本文件、执行记录、git 状态和当前批次清单；不要重新应用旧 scratch 覆盖已修复源码。

## 已完成的提交

| 批次 | 提交 | 完成内容 |
|---|---|---|
| 1 | `71ebd6e65771` | SQL/SQLite/VFS/persistent_cache、脚本缓存主链、PFFFT、GL dummy 和 Vulkan data dependency |
| 2 | `f48e852f6cb8` | 设备绑定认证/不可导出密钥、脚本缓存传输尾巴、content/google_apis/extensions、Mojo JS/TS 工具和浏览器空壳 |
| 3 | `7bdc3cfe9b56` | media/device/printing/chrome、blockfile、cc 微基准、媒体公共接口、版本/plist 和 Closure 残留 |
| 4 | `329c2433b200` | Canvas 绘图链、sandbox/storage 服务空壳、栈/堆采样 profiler；176 张像素一致 |
| 5 | `7817855215f4` | PAC/WPAD/系统代理解析服务及生命周期、prefs；116 文件删除，20 并发 EXE/DLL 与完整运行验证通过 |

前三批 Windows EXE/DLL、84 demos、serve/net、Node/daemon/协议、Bilibili 和像素基线检查已完成，详见执行记录；六平台实际编译未完成。没有创建 PR 或发布。

## 剩余大批次清单

### 4. Canvas、服务空壳、采样 profiler（已提交 329c2433b200）

- [x] Canvas 专用 layout/painter、context host/factory、font/performance cache、资源 provider、GPU bitmap/drawing buffer/shared context 完整删除。
- [x] 保留最小 HTMLCanvasElement，移除仅脚本可用的 bitmap 尺寸状态、paint 请求队列等残留；保留属性宽高比和实际静态行为。ImageElementBase 的普通图片/SVG 职责保留或迁出 canvas 路径。
- [x] sandbox/storage 根目录、proxy_resolver/proxy_resolver_win、components/services/storage/quarantine 的调用/协议/GN 和实体清理。
- [x] 删除栈采样、heap sampling、线程池 profiler 钩子、企业管理/默认应用工具；保留线程池调度、allocator 基础设施和真实诊断消费者需要的 module cache。
- [x] 集中构建、测试、像素对照、更新记录、提交。

### 5. 浏览器嵌入层、交互、合成器和 GPU 完整闭包

- [ ] Blink WebView/WebFrameWidget/WebPagePopup、ChromeClientImpl/LocalFrameClientImpl 等浏览器嵌入层；Shot 直接 Page/Frame/EmptyClients，保留布局所需最小接口。
- [ ] platform/widget、components/input、输入预测/OneEuroFilter、手势/fling/overscroll、IME、clipboard、dragdrop、鼠标锁定、popup/fullscreen、editing 交互命令/撤销。
- [ ] 保留表单文本/default value 绘制、CSS 焦点/状态、布局选择计算中确实需要的机制。
- [ ] cc LayerTreeHost/Impl、代理、scheduler、tile 调度和 GPU raster provider、帧率/scroll-jank 统计；保留 paint record、CPU 图片与必要 property tree 数据。
- [ ] Viz client/提交、GPU IPC/command buffer/SharedImage、GL context、GPU transfer cache；必要颜色/像素格式/几何类型拆出，不保留整个运行目标。
- [ ] GPU/GL 解耦后同步清理 ANGLE、Vulkan loader/headers、SPIR-V、libdrm/libsync/khronos/DX headers 的无用部分、DEPS、.gn、overrides、gpu_lists_version hook。
- [ ] 六平台窗口/屏幕/Ozone/headless 系统适配逐项收窄，不能破坏字体/颜色/scale。
- [ ] 批次收尾构建、运行验证、像素对照、提交。

### 6. 网络公共层、旧 IPC、浏览器服务与存储接口

- [ ] services/network/public/cpp 拆资源加载实际类型，去掉浏览器 policy/helper 的无用消费者；prefs 48 文件及 GN 依赖已在第五批删除并验证。
- [ ] NetworkContext 全服务、代理/设备/存储等未连接协议与冗余 bindings、service_manager public 服务管理契约、独立 cert_verifier 服务残留。
- [x] net PAC/WPAD/DHCP/系统代理：解析服务及初始化删除，HTTP 建连直接选择 DIRECT；Windows EXE/DLL 和全部运行/176 张像素回归通过。
- [ ] Network Quality Estimator、NetLog 导出、持久 cookie/字典/报告存储空壳逐个确认并收窄。
- [ ] memory cache、First-Party Sets/隔离键/权限等 C 项逐项决策；保留已有缓存语义、安全检查、证书/TLS、HTTP2/HPACK。
- [ ] ipc 老 Channel/Pickle traits 等沿实际消费者拆除；与 Shot 自有 stdio/daemon 协议区分。
- [ ] FileReader/文件选择/Blob 服务通道、Worker/Worklet 线程与接口清理；保留 file URL、普通资源线程池。
- [ ] 批次验证并提交。

### 7. 浏览器诊断、遥测和剩余 Blink 扩展

- [ ] UKM/metrics 服务、metrics_proto、生成器、browser frame 指标和 performance_manager/scenario_api。
- [ ] base profiler 剩余 module cache/trace_event/tracing 与 Perfetto 记录/会话/导出后端，按消费者处理；不能删 CaptureStats、加载判断/FCP、CHECK 或实际日志。
- [ ] crashpad/crash key 接入、device_event_log、skia tracing/benchmark wrapper/跨进程图像适配收窄；保留需要的崩溃诊断。
- [ ] DevTools emulator/probe/异步跟踪残留；保留 Shot 输出的 ConsoleMessage。
- [ ] accessibility 协议/事件链；保留 HTML 属性与 CSS 状态。
- [ ] PerformanceObserver/User Timing、resize/intersection observer 区分脚本回调与内部 lazy loading/生命周期。
- [ ] route_matching/url_pattern/view_transition/origin_trials、XSLT/sanitizer/Trusted Types 明确静态用途；逐项删或保留最小行为，不按目录名判死。
- [ ] 批次验证并提交。

### 8. 第三方、资源、构建工具与根目录最终收口

- [ ] 按报告附录逐项回查 anonymous_tokens、leveldatabase、snappy、libpfm4、jsoncpp、win_virtual_display、inspector_protocol 等骨架。
- [ ] libyuv、re2、liburlpattern、ipcz 按剩余实际调用决策；普通图片转换/URL/Mojo 管道不能误删。
- [ ] 无消费者的上游测试 target/template、google_benchmark/fuzztest/ocmock 等，保留 Shot 测试资产和真实生成工具。
- [ ] Lottie、桌面图标/语言/AX resources、GRD/XTB 和未使用工具 hooks 同步收窄。
- [ ] base 电源事件/提权/系统工具、ui gfx/native/屏幕扩展逐项完成 C 类审查。
- [ ] shot 实验参数/加载 fallback 回调逐项确认；保留公开能力和有用途诊断。
- [ ] 根 .gn、BUILD.gn、DEPS、.gitmodules、prune-deps/trim-tree 白名单与同步行为检查；删除后不再检出同一无用包。
- [ ] 精确删除确认无文件的空目录；不能递归清理共享 checkout 或旧 out 实验目录。
- [ ] 全部根目录及附录逐项回填最终状态，列清保留基础设施/工具/测试，修正历史“全删完”表述。
- [ ] 最终 Windows EXE/DLL 和完整运行检查、Linux/macOS/六平台实际构建边界记录；必须有真实证据，不把 probe 当编译。

## 当前接续信息与验证资产

- 当前分支原为 `release`，最近完成提交 `7817855215f4`。继续前以实时 git 状态为准。
- `out/cut-stage6/manifest.json`、`extra-edits.json`、`tail-edits.json` 记录 Canvas 和本批追加 owned paths；源码已应用，原始 hash 不能用于重放。Canvas 删除清单已扩大到 118 个文件并已实体删除，包括绘图值类型/IDL/生成绑定。`ImageElementBase` 已迁到 core/html 公共位置，canvas 目录仅留标签三文件。
- 暂停时 editing_utilities 的 Canvas image URL 分支已删除。恢复后集中构建仅先发现 thread_pool_impl 缺 FilePath 的直接 include；已修复，失败 TU syntax-only 1/1 通过。
- `out/cut-stage7/manifest.json` 和 `out/cut-stage8/manifest.json` 的服务/采样 profiler 修改已核对 hash 并应用，后续另有收尾修复，不能重新覆盖。临时文件直接位于对应 stage 路径下，没有 scratch 子目录。
- 当前整批已实体删除 225 个普通文件，证明在 `out/cut-batch4/deletion-proof.json`；已去掉 34 个核对过的空目录，sandbox/storage 等根目录消失。GN 通过、7,459 个输入全部存在，IDL 131 个枚举引用/43 个 union 名称引用均无缺失。
- 本批 Windows EXE/DLL 与完整运行回归已通过，176/176 像素对照一致，已提交 329c2433b200。用户要求 jobs 31 后已实试，但多个 Blink animation TU 出现 LLVM OOM，记录 `out/Shot/cut-batch4-j31-oom.log`；已停掉明确属于本任务的 31 并发进程树并确认无遗留编译子进程，自动回退 jobs 8。当前日志 `out/Shot/cut-batch4-build.log`，不要与 OOM 日志混淆。
- 当前第五批已应用：out/cut-stage9/manifest.json 为 owned edits（含修复后的实际源码，scratch 不可重放），deletions.json / deletion-proof.json 记录 116 个实体删除。PAC/WPAD/系统配置/解析服务及 URLRequestContext/HttpNetworkSession 持有关系已删除，HTTP JobController 直接选直连，prefs 48 文件与 GN 依赖删除；已完成 Windows EXE/DLL（jobs 20）、完整运行检查和 176 张像素回归，第五批已提交 7817855215f4。后续继续 jobs 20。
- 旧 IPC 的网络 ParamTraits 仍被 Mojo [Native] ConnectionInfo / EffectiveConnectionType / URLRequestRedirectInfo 真正序列化使用，不能只删除 include；后续需迁为明确 Mojom 字段/枚举及验证 traits，再拆旧 IPC。相关调查位于 out/cut-stage9。
- 原始裁剪基线位于 `out/cut-baseline`，是本次任务从已验证 out/Shot 保存的对照资产，不是新编译验证对象。新产物一律用 `out/Shot`。
- Canvas 八组研究样本位于 `out/canvas-assessment`，保存真实 canvas 与普通元素差异。候选必须与 canvas.png 比，不能与 generic.png 比。
- 完成批次对照：169 demos + 4 render cases + corpus，174 张基线；补上 Canvas 专项 fixture。已有 Chrome oracle 差异 1.5245%，本次不得新增未解释差异。
- 不允许新起子代理（当前没有用户或项目授权）；按当前任务本地推进。

本文件各待办只有完成实际处理和对应验证后才能勾选。预算、运行时间或上下文切换不构成完成理由。

- 下一批统一收尾 Canvas 公共 property tree 子树状态、孤立 canvas_snapshot_info.h 和 ActiveScriptWrappableCreationKey 的旧 Canvas friend 声明；不保留无调用方的绘图 metadata。

- 第五批所有 exec session 已结束，无运行中的构建。下一整批优先拆 Blink 浏览器嵌入层/WidgetBase 与 GPU/合成器创建入口，out/cut-stage6/embedder-* 是早期只读定位，必须以当前源码为准；旧 IPC 的 5 个 Native 类型证据在 out/cut-stage9/native-mojom-refs.txt。不需要重编已验证第五批。

- 第六批正在实施，尚未构建或提交：out/cut-stage10/manifest.json 记录 17 个已应用调用方文件，deletions/deletion-proof.json 记录 widget_creation_observer.h 的删除。已拆无窗口媒体查询分支、widget 创建 observer、scheduler→widget 的任务完成通知（保留真正调度与队列统计）、FCP 的 widget 回调（当时保留 ChromeClient 通知；其并非实际截图等待入口，见后文核实更正）、DOM/HTML unbounded native window 分支与部分 DevTools/cursor 回调。当时尚未删除的主实现已在下述后续步骤删除；WidgetBase/FrameWidget 接口与下游调用仍待清理。需要把整个大批次完成后才集中编译，不能把当前状态当第六批已完成。
- out/cut-stage10/{creation-sites,external-refs,widget-callers,widget-lifecycle-refs}.txt 为当前批次切入证据，已修改调用方后内容是修改前快照；后续搜索当前源码刷新。不得重新应用 scratch 覆盖随后修复。

- 第六批继续：现已实体删除 37 文件（含前述 observer），移除 8 组 WebView/WebFrameWidget/WebPagePopup/WebLocalFrame/WebRemoteFrame/ChromeClientImpl/FrameClientImpl 主体及仅由它们创建的 DevToolsEmulator、ScreenMetricsEmulator、FullscreenController、WebSettingsImpl、FindInPage/TextFinder/FindTaskController、WebInputMethodController 和观察者。六个 GN source list 已同步。当前仍有下游旧引用，源码暂时不构建；必须继续完成依赖闭包后集中验证再提交。删除源码快照保存在 out/cut-stage10/deleted-source，便于迁出确实属于静态机制的实现；不要因为编译报错恢复整个浏览器层。当前 owned manifest 为 22 个路径，其中 WebFrameWidgetImpl.cc 后续已删除；使用去重路径并集提交。

- 下一接续直接处理 out/cut-stage10/remaining-browser-refs.txt 对应的当前下游调用。优先：删除 browser exported WebFrame 包装/序列化/public observer 尾巴、core Frame::Swap（仅 browser WebFrame 调用，保留 Frame::Detach）；拆 LocalFrame::GetWidgetForLocalRoot 剩余调用及 FrameWidget/WidgetBase；LocalFrameMojoHandler 中 WebFrame 查询和输入方法需连同未连接的 Mojom 方法删除或迁出实际核心机制，不能创建返回 nullptr 的替代 WebFrame。FCP 在 core/frame/local_frame.cc 的 ChromeClient::OnFirstContentfulPaint 保留。当前无运行中的构建/测试，不要基于旧二进制宣称第六批通过。

- 第六批后续进度：当前实体删除 62 文件，owned manifest 75 路径（另有本任务清单文档），累计 diff 约 3.34 万行删除；仍未编译/提交。已删除 Frame::Swap/SwapImpl、LocalFrame::SwapIn 和仅浏览器调用的页面交换辅助函数，保留 DetachDocument/XSLT 文档交接及正常 frame 树逻辑；移除 LocalFrameView 同文档导航屏幕呈现回调。
- 已删除 WebFrame 包装实现、HTML/MHTML writer FrameSerializer/WebFrameSerializerImpl 及 public API、WebFrameContentDumper、WebScopedPagePauser、InteractionEffectsMonitor/外部监视器（含 SoftNavigation 订阅、通知、Trace），未删除 MHTML 读取路径。FrameFetchContext/BaseFetchContext 无调用方 WebSocket 握手及其专属混合内容检查删除，普通 fetch 混合内容检查保留。
- 已删除 DOMViewport/viewport.idl 和 window.viewport 脚本属性（保留真实 VisualViewport/CSS 媒体查询），删除 Unbounded native bounds 缓存（HTMLElement/NodeRareData）和 PrePaint 发往原生窗口的分支；保留 active 状态及 CPU paint property 更新。删除 selection 的 Widget 输入作用域、image/text/layout-shift HUD 回调/接口；PaintTimingDetector 的无窗口坐标转换辅助已消除，调用者保留原始矩形语义。
- PointerLockController、DOM/TreeScope 查询、request/exit/事件 IDL、PointerLockOptions 及生成值类型、Page 持有关系、插件鼠标锁 listener/callback/API、输入事件锁定目标分支已删除；WidgetBase/FrameWidget 的 PointerLock Mojom 接口仍待与整层一起拆。删除列表及 hash 证明已同步 out/cut-stage10，均确认文件不存在，旧源码备份在 deleted-source。git diff --check 无空白错误，仅已有 CRLF 提示。
- 当前剩余引用快照：out/cut-stage10/widget-callers-current.txt、deleted-header-residuals.txt（清掉其中 8 个已无用 include 后需刷新）、remaining-browser-refs.txt（部分已过时）、closed-feature-residuals.txt；serialization-refs/pointer-lock-refs/dom-wrapper-refs 是修改前调查，不可作为当前状态。不要重放任何 scratch 或 manifest 原始内容。
- 下一步先处理 LocalFrameMojoHandler 未连接浏览器 RPC（UpdateOpener/WebFrame guard、DIP 坐标输入、浏览器控件等）、RemoteFrame/RemoteFrameOwner/RemoteFrameView 浏览器远端嵌入残留、LocalFrame::GetWidgetForLocalRoot/PrescientNetworking/SaveImageAt；然后 popup/plugin/context-menu/input 与 FrameWidget/WidgetBase 整层，最终 cc/Viz/GPU 创建链。当前 GetWidgetForLocalRoot 还在约 14 个文件出现，不能构建前只删 include 或新加空返回 WebFrame shim。
- PaintTiming 待处理调查：MarkPaintTimingInternal 在无 Widget 且无测试 callback_manager 时，先 Take 三类 image/text 回调并清 pending_paint_events，再直接返回；之后约 200 行是屏幕呈现和 Web Performance 队列，无 Shot 执行。后续需要连生产者/队列一起拆，不能只删 Widget guard 让原来不执行的回调运行。SetFirstContentfulPaint/LocalFrameView/LocalFrame 的绘制里程碑暂时保留；原称其为截图等待链不准确，见后文核实更正。RegisterNotifyPresentationTime 还与 FMP/BFCache 相连，需集中处理；本次尚未改它。
- 本次没有启动构建或测试进程。最新已验证二进制仍是第五批；后续完成第六批整个闭包后集中用 jobs 20 进行 GN/输入/EXE/DLL 和完整运行像素回归。

- **FCP 证据更正（以后接续按此项，不按早期摘要）：** 当前 shot/shot_renderer.cc:120 的 ShotRenderer::ChromeClient 只覆盖屏幕 scale 和 ConsoleMessage，并没有 OnFirstContentfulPaint/NotifyPresentationTime override。WaitForLoad:1113 的真正 loaded 判据是跑过生命周期、HasFinishedParsing、IsLoadCompleted、ActiveRequestCount()==0；FrameClient::DispatchDidHandleOnloadEvents:222 唤醒 CaptureContext。不得把此前摘要声称的“FCP 浏览器通知是截图等待入口”当事实。真实绘制里程碑与调度/字体/图片流程仍需按实际调用保留，不能误删 capture 等待、onload 唤醒、资源判据和 CaptureStats。
- 第六批最新进度：删除 79 文件、owned manifest 100 路径，约 3.91 万行删除，尚未构建或提交。LocalFrameMojoHandler 已移除全部入站 LocalFrame/LocalMainFrame/视频全屏/DevicePosture 接收器、registry 注册、ActiveURLMessageFilter 和约千行浏览器命令处理。frame.mojom 删除 LocalFrame、LocalMainFrame、无人调用的 LocalMainFrameHost 接口；device_posture_provider.mojom 仅保留 CSS 使用的姿态 enum，MediaValues 直接回答原有 kContinuous；Mac TextInputHost/FullscreenVideoElementHandler 协议文件删除。Handler 当前仅持有四个出站 host/reporting/BFCache remotes，后续仍需拆其调用方，并非最终保留结论。
- WebPrescientNetworking（一直因没有 WebLocalFrame 返回 null）工厂/字段、预连接/DNS 提示处理、HTML preload scanner 的 preconnect 请求种类、PreloadHelper 调用、hover DNS 提示已删除；保留真实图片/样式/字体 preload 及其过滤/安全检查。SavableResources 和测试导出接口、对应 Mojo 存储结构也已删除。LocalFrame 的 Save/CopyImageAt、媒体浏览器操作/视频帧 IPC 拷贝、Mac GetCharacterIndexAtPoint 和相应 MediaPlayerAction Mojom 已删除。
- AnchorElementMetrics/发送器/viewport tracker、navigation_predictor.mojom、Document/anchor/scroll/FCP 的遥测挂钩已整链删除；anchor 正常渲染与 render-blocking 处理保留。Mac SubstringUtil 无调用方后删除，HandleShadowDOMInSubstringUtil 运行开关删除。
- PaintTiming 的 MarkPaintTimingInternal 屏幕呈现/Web Performance 回调分支删除，改为 DiscardPresentationCallbacks，只保留原先无 Widget 路径的三个 TakePaintTimingCallback 和清 pending_paint_events 的真实收尾；删除 MarkPaintTiming/last_rendering_update_end_time_、测试 CallbackManager。FMP/BFCache 的 RegisterNotifyPresentationTime 仍留着；image/text/LCP/element timing 生产者的深层遥测裁剪仍待整链处理，不能把这个过渡状态当全裁完。
- 当前 GetWidgetForLocalRoot 仅剩 3 个实际调用：remote_frame_view.cc:286（OOPIF 合成缩放，LocalFrameView:948 会遍历调用 UpdateCompositingScaleFactor）、keyboard_event_manager.cc:274（PWA 对键盘 JS 分发例外）、view_transition_style_tracker.cc:2036（虚拟键盘 resize height）。LocalFrame 的 getter/声明还保留，不能引入返回 nullptr 的新替代实现；接着拆这三处及 RemoteFrame/RemoteFrameOwner/FrameWidget/WidgetBase 全层。element_timing_utils.cc 的 WebLocalFrameImpl 转换并不经过这个 getter，仍需同处理。
- out/cut-stage10/receiver-preconnect-residuals.txt、latest-closed-feature-residuals.txt、removed-receiver-type-refs.txt、remote-frame-view-refs.txt 是本轮调查；早期快照含已删除内容。before-receiver-cut 保存 handler 重写前源码用于迁出真正需要机制，不要整体恢复。预处理条件配对对 80 个当前 owned C++/头文件检查通过，git diff --check 通过；这些不等于语法/编译通过。所有构建/测试仍未启动，第五批产物仍是最近验证版本。下一轮完成大批次后统一 jobs 20。

- 第六批最新接续：已实体删除 183 文件，owned manifest 135 路径，累计 diff 约 6.59 万行删除；尚未编译或提交。本轮删除 RemoteFrame/RemoteFrameView/RemoteDOMWindow/RemoteFrameOwner/RemoteFrameClient/RemoteSecurityContext 和 ChildFrameCompositingHelper/Compositor 共 14 文件，移除 frame token 的远端查找、跨进程 focus/zoom/owner 属性复制、LocalFrameView 的远端遍历及远端合成提交。Frame::ResolveFrame 对非本地 token 返回未找到；真正 LocalFrame 布局、focus、普通 frame owner 与 iframe 标签保留。
- 已删除 GetWidgetForLocalRoot 的最后调用及接口本身，没有新增空返回适配器。删除 platform/widget 全部 60 个 tracked 文件，含 FrameWidget/WidgetBase、LayerTreeView、输入队列/预测/overscroll、Mojo 输入处理与 compositor 桥接；GN source/jumbo exclusion/旧测试和 Android WebView 目标条目一并清除。8 个外部旧 include、Mojo direct_receiver 的 widget friend 与 main_thread 的 worker-pool-delegate friend 已去掉。目录下若还有空目录，最终根目录收尾精确删除，不能递归碰其他路径。
- 外部/内部下拉弹窗、颜色/日期选择器 UI、ChooserResourceLoader、ColorPagePopupController 共 17 文件删除。随后删除 PagePopup/Client/Controller 和 ValidationMessageClient/Impl/OverlayDelegate 共 11 文件；Page 持有、生命周期 layout/prepaint/paint/animation 和 Document 焦点通知已断开。ListedElement 保留真正 validity 缓存与 CSS :valid/:invalid/:user-valid/:user-invalid 更新；ShowValidationMessage 改名 FocusValidationAnchor，保留原有滚动与焦点机制，删除提示气泡和其后台更新队列。普通控件的样式、默认值、校验计算保留。
- LocalFrame 的 PagePopupOwner setter/member/Trace/clipboard 借用和 StyleEngine 的 popup 字体分支、DocumentLoader 的 popup 专用测试 origin/agent-cluster 分支已删除；普通 CSSFontSelector 和正常 owner document 安全来源路径保留。PagePopup/PagePopupCopyPaste 运行开关已删除。动画帧监视器 AnimationFrameTimingMonitor 随 Widget 创建入口消失，现删除 cc/h 和 core_probes.json5 observer 注册；PerformanceMonitor 仍待后续审查。
- IME 的 Widget 输入标志保存/恢复、EditContext 通知 Widget 取消输入法、Unbounded 原生窗口跨 frame 命中测试结构/函数/两处调用删除。LoaderFactoryForFrame 的 WebFrame URLLoaderThrottleProvider 查找和包装 CreateThrottles 方法删除，给真正 URLLoader 构造传空 throttle vector，与原有 EmptyLocalFrameClient 路径一致；URLLoader/真实资源加载仍保留。
- 当前证据：out/cut-stage10/{remote-after-cut,widget-subtree-external,widget-input-external,widget-other-symbols,widget-after-cut,embedder-current,public-embedder-includes,page-popup-refs,validation-client-external,paint-compositor-external,remote-protocol-refs}.txt。带 external/refs 的多为修改前快照，请以当前源码刷新；widget-after-cut 与 popup-validation-after 均无实际残余结果。183 个删除文件均确认不存在，备份 hash 与 proof 一致；112 个 owned C++/头文件 #if 配对无错误，git diff --check 通过，仅已有 CRLF 提示。这些不等于编译通过。gn path out/Shot //shot:shot_core //third_party/blink/renderer/platform:platform 确认 public 依赖直达；没有启动新 GN 或构建。
- 下一步优先 PaintArtifactCompositor / LocalFrameView::PushPaintArtifactToCompositor 及约 33 处外部调用，连到 cc LayerTreeHost/Layer/合成器创建链。当前唯一 PaintArtifactCompositor 创建位于 LocalFrameView:2980，PushPaintArtifactToCompositor 先检查 acceleratedCompositingEnabled；settings.json5:198 默认 false，全仓现存只有 serializer 显式设置 false，没有 Shot 设为 true。必须保留 shot/shot_renderer.cc 使用的 paint_chunks_to_cc_layer（名字含 cc，但承担实际 paint record 转换）。不要盲删整个 graphics/compositing 或 cc/paint。另需继续插件/ContextMenu/FileChooser/ElementTiming 的 WebLocalFrameImpl 残留，以及 public WebFrame/WebView/Widget 与 remote_frame.mojom；这批未闭合前仍不启动大编译。jobs 20，最近已验证产物仍第五批，所有 exec session 已结束。

- 第六批最新接续（合成层调用收窄中）：owned manifest 155 路径、删除 187 文件，当前整批 diff 约 6.75 万行删除；仍未构建/提交。已删除 LinkHighlight/LinkHighlightImpl 4 文件及 Page 持有、所有 prepaint/paint/导航/合成 host 钩子，SetTapHighlight 在当前树没有外部创建调用；同时删除无调用方 Page::DidInitializeCompositing/WillStopCompositing。PrePaint/PropertyTreeBuilder 的点击高亮额外 fragment 条件消除，普通 inline fragment 和 CSS highlight/selection 绘制保留。
- 已清除 PaintPropertyTreeBuilder 的 DirectlyUpdateCcTransform/Opacity 和所有调用、滚动偏移/裁剪矩形直推合成树、VisualViewport 的直推 scale/scroll、FrameCaret 的直推 opacity、CaretDisplayItemClient/Scrollbar/DisplayLock 的合成更新通知。保留原有 CPU property node Update、OnUpdateTransform/Effect/ScrollTranslation、CullRectUpdater、IntersectionObservation、paint invalidation/重绘标志和 caret PaintCaret。DisplayLockContext::MarkNeedsRepaintAndPaintArtifactCompositorUpdate 改名 MarkNeedsRepaint，其真实重绘逻辑未删。Canvas transform/property-tree 子树标志尚待下一轮整链删除，不能认为本轮已闭合。
- 已删除 VisualViewport 中不创建的 cc scroll/scrollbar layers、创建/刷新/颜色更新/foreign layer Paint、相应 effect nodes/Trace/LayerFor* getter；LocalFrameView 的 viewport foreign-layer Paint 调用、Page 初始化滚动条层通知和 LayoutView 的层颜色通知同步删除。视觉视口的缩放/滚动偏移/真实 transform 与 scroll property nodes、TransformNodeForViewportScrollbars 和 UsedColorSchemeChanged 保留。RootFrameViewport/ScrollableArea 的 LayerFor* 与 HasLayerFor* 旧接口删除；ReplacedPainter 直接保留原本会执行的 CPU resizer 绘制，普通 ScrollableArea 重绘状态与 CPU ScrollableAreaPainter 保留。
- DropCompositorScrollDeltaNextCommit 全部 Blink 接口/调用已删除（三个 ScrollableArea 子类、基类、ScrollAnimatorCompositorCoordinator 中断回调、ViewTransition counter-scroll 提交通知）；保留现有 counter-scroll 本身。ScrollingCoordinator::UpdateCompositorScrollOffset 的所有调用/方法删除，但整个 ScrollingCoordinator 和 PaintArtifactCompositor 创建仍留着，后续须一起删除。LocalFrameView 的无外部调用 RunPaintBenchmark/MainThreadScrollingReasonsAsText/SetTracksRasterInvalidations 及 LocalFrame 的 GetLayerTreeAsTextForTesting/CompositedLayersAsJSON 调试入口删除；PaintController 删除两个对应不再调用的强制合成 benchmark enum，CaptureStats 与 Shot 自有性能测量不动。
- 最新 getter 残留在 out/cut-stage10/pac-current-callers.txt：除 LocalFrameView 自身，GetPaintArtifactCompositor 仅 PaintLayerScrollableArea::ShouldScrollOnMainThread 和 UsesCompositedScrolling 两处。SetPaintArtifactCompositorNeedsUpdate 仅 LocalFrameView::UpdatePaintDebugInfoEnabled 内部调用。但不能据此直接删整个 PAC：out/cut-stage10/paint-compositor-types-current.txt 记录 Animation/KeyframeEffect/CompositorAnimations/AnimationTimeline/Trigger/DocumentAnimations/PendingAnimations 的直接 PAC 参数传递，尚未清理。
- 下一具体步骤：先把 ScrollAnimator/ProgrammaticScrollAnimator/ScrollAnimatorCompositorCoordinator 的合成线程状态迁出/删除，仅保留真实主线程插值、offset/position（含 RTL）转换、pending/run/cleanup 状态、取消完成回调和 ScheduleAnimation。证据 out/cut-stage10/scroll-animator-coupling.txt。不能把 SendAnimationToCompositor/ShouldScrollOnMainThread 改成假返回常量；删发送分支及无实际路径的状态，再拆接口。ScrollAnimatorBase 会在禁用平滑滚动时直接实例化，它的即时滚动必须保留；cc::ScrollOffsetAnimationCurveFactory/Curve 用于真实主线程插值，不能随整个 cc/animation 删除。
- 然后收完 DocumentAnimations 的 PAC 传递与 PendingAnimations 主线程就绪。特别注意 Animation::PreCommit 的 Outdated + Playing + CurrentTime + PaintClean + ScriptForbidden 提前 defer 判据在原本无合成器的路径也可能执行；CSS 动画 pending/NotifyReady、滚动 timeline deferred start 和 ready 状态不能随着 GPU 创建盲删。LocalFrameView::RunPaintLifecyclePhase 在无 PAC 时 needed_full_update 原本恒 true，更新 CSS animations/Timeline 的 CPU 行为必须继续。最终移除 PAC/PendingLayer/PropertyTreeManager 的 cc layer 管线时保留 shot_renderer 使用的 PaintChunksToCcLayer CPU record 转换。
- 本轮 static 核验：187 个删除路径均不存在且备份 hash 对应；131 个 owned C++/头文件预处理 #if 配对通过；git diff --check 无空白错误，仅已有 CRLF 提示。这些不是语法/编译/运行通过。没有启动新构建，全部 exec session 已结束，后续完整批次 jobs 20，最近验证二进制仍第五批。

- 第六批最新接续（滚动 CPU 与合成器实删）：owned manifest 195 路径，实体删除 203 文件；整批 tracked diff 约 7.62 万行删除，另有新建 scroll_animation_state.cc/h。未构建、未提交；最近验证产物仍为第五批。新建路径在 manifest 的 sha 为 null，提交时应显式加入这两个新文件，不要遗漏，也不要重放旧 scratch。
- ScrollAnimatorCompositorCoordinator cc/h 已删除，真正 CPU 生命周期迁为 ScrollAnimationState cc/h，保留 Idle/WaitingToStart/RunningOnMainThread/PostAnimationCleanup、Detach 取消、curve、RTL/vertical-rl 的 offset-position 换算、clamp 和 ScrollOffsetChanged。ScrollAnimator/ProgrammaticScrollAnimator 删除 compositor animation 创建、ID/group/附着/提交/接管/中断回调与 impl-only 调整队列；保留同一 cc::ScrollOffsetAnimationCurve/Factory、时钟、曲线调整、立即滚动、开始/完成/取消回调和调度失败 fallback。UpdateCompositorScrollAnimations/UpdateCompositorAnimations 改为 UpdateScrollAnimationState/UpdateAnimationState，所有基类和 LocalFrameView/RootFrameViewport 调用已同步。调度失败后 ProgrammaticScrollAnimator 立即滚到目标并 return，避免原先 Reset 清空 curve 后又设为待启动；行为分歧已记录 docs/upstream-sync.md，尚未运行验证。
- ScrollableArea/VisualViewport/PaintLayerScrollableArea 的 compositor host/timeline getters、ShouldScrollOnMainThread、UsesCompositedScrolling 已删；滚动条保留 CPU 可见性、fade 定时器与 paint invalidation。MayCompositeScrollbar 及 ScrollbarDisplayItem 的 CPU Paint 路径仍存在（其 metadata/图层创建支路待后续拆除）；没有用返回 false 的新空壳冒充删除。
- LocalFrameView 的 PaintArtifactCompositor 持有/Trace/创建/Push/RootCcLayer/getter/更新通知、ScrollingCoordinator 全类两文件和 Page getter/所有权/销毁已删除。DidCompositorScroll 的 core 调用链、按 compositor ID 查找滚动区、仅用于提交的 scrollable_areas_with_scroll_node_ 注册集合已删除。真正 scrollable_areas_、锚定、sticky、CPU property nodes/重绘、PaintTree、NotifyPaintFinished 和 CSS 动画生命周期保留。ChromeClient/EmptyChromeClient 的 AttachRootLayer/GetCompositorAnimationHost/GetScrollAnimationTimeline、Document 的 timeline 挂接/解挂、Animation 的 timeline 挂接/解挂方法和调用已删除，没有补 null adapter。
- PageAnimator 向 cc host 上报 Canvas/inline style/SMIL/RAF/view-transition 统计的字段、setter、ReportFrameAnimations 及 Element/CSSOM/ViewTransition 调用已删除。SMIL ServiceSmilOnAnimationFrame 仍执行；next_frame_has_pending_raf_ 与 PostAnimate 的动画时钟控制保留。DocumentAnimations::GetAnimationsCount 只计 cc host 动画，已删除。
- AnimationTrigger/TimelineTrigger 的 cc trigger 持有、delegate、创建/销毁、异步激活/暂停时间同步、TimelineTriggerRange::ComputeCcBoundaries、Animation::StartTriggeredAnimationOnCompositor/NotifyAnimationStartedAsync/PausedAsync 已整链删除。TimelineTriggerRange 的 Idle/Primary/Inverse enum 改为自身 CPU 状态类型；CSS trigger 范围、play/pause/reverse 行为、pending ready 和真实 UpdateState/ComputeState 保留。DocumentAnimations 的仅用于 cc 的 trigger registry/更新/解挂删除；CompositorEventTrigger/CompositorTimelineTrigger flags 和 PlayInternal 的触发器 cc cancel 例外同步删净。
- PaintArtifactCompositor 和 PropertyTreeManager 四文件已实体删除；此前所有实例都不存在，CompositorPropertyAnimationsHaveNoEffect 的 null-PAC 路径原来返回 false，现删除这段不可执行的图层分析和所有 PAC 参数/前置声明。保留 missing_style_or_layout 判断和 pending 失效状态收尾。DocumentAnimations/AnimationTimeline/Animation/KeyframeEffect/CompositorAnimations/PendingAnimations 以及 SVGImage、ClipPathClipper 调用已同步。随后删除其无人创建的 PendingLayer/ContentLayerClientImpl/LayersAsJSON/AdjustMaskLayerGeometry 八文件和 GN/test source 项。合成目录现在仅留 chunk_to_layer_mapper 与 paint_chunks_to_cc_layer 四文件，它们仍有真实 CPU 使用；不能一并删掉。
- 证据：out/cut-stage10/{scroll-closure-all,scroll-closure-head,scrolling-coordinator-final-refs,animation-trigger-gpu-refs,trigger-callback-closure,trigger-async-callback-refs,page-animation-report-refs,pac-before-physical-cut,after-pac-dependent-refs,layer-client-head-refs}.txt 多为修改前快照；scroll-trigger-host-after/compositing-physical-after 为空表示对应旧入口/include 搜索无残余，不表示编译通过。gn refs out/Shot 对 PAC 文件返回 no matches，这是旧缓存图查询，不能当重新生成或实编译。203 个删除文件不存在且备份 hash 通过；166 个 owned C++/头文件 #if 配对无错，git diff --check 通过，仅已知 CRLF 警告。所有 exec session 已结束，没有构建进程。
- 下一步：动画内部仍保留 GPU PreCommit/group/pending/CompositorAnimation/NativePaintWorklet/KeyframeModel、Eligibility 和 PaintWorklet 子链，必须继续拆，不能把删除 PAC 等同全删完动画 GPU。PreCommit 的 Outdated + PaintClean + ScriptForbidden 延迟和 PendingAnimations 的 NotifyReady/scroll timeline deferred/start/postcommit 必须保留实际 CPU 语义，SVGImage 用 LayoutClean + false 的路径也要保留。Canvas property tree 标志、ScrollbarDisplayItem::CreateOrReuseLayer、foreign layer 的插件/view-transition/embedded/paint_layer 支路和 cc 主机/raster GPU 待处理。浏览器旧 include 的实时清单是 browser-headers-current.txt：插件、FileChooser、ContextMenu、ElementTiming、BlinkLeakDetector 等仍需闭包；LocalFrame/PreloadHelper/HTMLResourcePreloader/CreateWindow 还有早期旧 include 需按真实使用清掉。整批闭合后统一 jobs 20 构建和完整运行/176 像素验证，禁止复用第五批二进制冒充本批通过。


- 第六批最新接续（文件选择/菜单/插件/旧 WebFrame 头文件闭合）：owned manifest 260 路径、实体删除 249 文件；整批 tracked diff 在文档更新前为 500 文件、+331/-84517 行，另有新建 scroll_animation_state.cc/h。仍未提交、未有第六批新二进制。源码达到一次集中验证的批次边界，开始统一 GN/输入/编译；GPU 动画内部、输入、ContainerTiming/Performance 与 public WebFrame 等后续裁剪并未宣称完成。旧条目中“这批闭合前不构建”的切入工作已推进至全部已删头文件无直接残余；不必为后续全部 GPU/遥测项目继续无限扩大本次未验证 diff。
- FileChooser cc/h 与 file_chooser.mojom 三文件、ChromeClient/Empty 打开窗口接口、FileInputType 的选择/取消/目录遍历/Mojo 文件转换/拖入文件路径与 InputType/HTMLInputElement 包装已删除。FilePickerEventsFix、FileChooserOpened probe 和 GN 源项一并删除。保留文件输入的 shadow button/text、required/disabled/multiple/style/value/validity 和现有 FileList 状态；CanReceiveDroppedFiles 仍跟随 DragController 的悬停样式支路，后续输入/拖放整链继续拆，不是永久保留。
- ContextMenuController/Provider、ContextMenuAllowedScope、菜单数据/traits/builder/public structs/协议和无人使用 WebPopupMenuInfo 共 16 文件删除。Node 右键菜单默认处理、Page 创建/持有/Trace、frame.mojom 出站 ShowContextMenu 与 public WebLocalFrame/Client 方法删除；selection/IME/gesture 的 allowed scope 去掉。普通选择/焦点状态保留。ui/base 中 OmniboxContextMenuController 的旧 friend 是后续 ui 清理项，本轮没有宣称 ui 全删。
- WebPluginContainerImpl/public WebPlugin/Container/Document/Params、PluginDocument、PluginData/PluginRegistry、仅插件 sandbox 使用的 SinkDocument 共 14 文件删除；插件专用 BeforeUnloadEventListener 再删两文件，WebPluginScriptForbiddenScope wrapper 再删两文件。插件创建/焦点/输入/打印/合成层/Find/持久化/延迟释放/注册表 MIME 查询/外部处理、Document/Page/LocalFrame/View/Node/LayoutEmbeddedContent 持有及调用已整链拆除，GN/public 列表与 Mojo 同步。原先 Shot 和 SVG/serializer 显式 PluginsEnabled=false，默认也为 false；现在删除开关及 Empty CreatePlugin 空实现，没有添加假 null adapter。
- HTMLPlugInElement 仍是 object/embed 共享的布局/资源加载实现，不能按名称一起删除。保留图片加载、尺寸/属性样式、CSP/混合内容检查、真实子文档加载和 LayoutEmbeddedObject/备用内容、load-event delay 增减；RequestObject 对不支持内容直接失败，HTMLObjectElement 仍走原来的 RenderFallbackContent。删除 PluginParameters、JS 强制布局、插件实例持久化、Flash override 和远端 FrameView 恢复支路。嵌入内容可用状态字段改名 embedded_content_is_available_；仍承担实际 focus/CSP 拒绝状态。NeedsPluginUpdate/UpdatePlugins 等旧命名还承载 object/embed 布局后的加载，后续应按真实机制决定命名，不能误删其完成加载计数。
- 嵌入绘制中插件/远端 frame 的 SVG filter override 和对应 display item/hit-test enum 删除。原先 LocalFrame 路径仅统计并返回空 properties，现删对应计数接口和额外父节点遍历；真正 PaintPropertyTreeBuilder 的本地跨来源 SVG filter 安全处理保持。ViewTransition snapshot foreign layer 仍待后续闭包，不要把这次删插件等同全删 ForeignLayerDisplayItem。
- BlinkLeakDetector cc/h/Mojom 三文件及绑定注册、专用开关/PrepareForLeakDetection wrappers、内部测试 settings supplement、Worklet GC 空接口删除。正常 ResourceFetcher StopFetching、MemoryCache、CSSDefaultStyleSheets 初始化/Reset、真实 cppgc 不动。LocalFrame/HTMLResourcePreloader/PreloadHelper/CreateWindow 中已无用的旧 WebFrame/WebView includes 删除。
- ImageElementTiming/TextElementTiming/ElementTimingUtils 六文件删除，PaintTiming 的创建/持有/Trace/Get/废弃呈现 callback 生产者和 PaintTimingDetector image/background/remove 通知同步去掉。TextRecord 删除 element timing 专属 rect/bit/ctor 参数，TextPaintTimingDetector 删除相应 eligibility 和坐标转换；保留当前 LCP/SoftNavigation 检测、FCP/真实 painting/loading 逻辑。LCP 仍需要截断 data URL 到 100 字符，常量已迁入 largest_contentful_paint_calculator.cc。ContainerTiming/PerformanceElementTiming 和 Element/Node/layout/prepaint 标记仍在，下一个遥测子链须整链继续清理；调查 out/cut-stage10/container-timing-refs.txt。
- 当前静态证据：deleted-header-residuals-latest.json 遍历所有 tracked C++/h/mm 对 249 个已删路径的直接 include，结果为空；browser-closure-after.txt 对旧 WebFrame/WebView/ChromeClientImpl/ElementTiming/LeakDetector 入口检索为空。220 个 owned C++/h 的预处理配对通过，249 个删除文件不存在且备份 hash 通过，git diff --check 无空白错误。以上都不是编译通过。若 inline 修改出现 UNKNOWN 文件写入错误，应先核对哪些已经落盘再继续；本轮 page.h 一次写入错误已精确检查并完成，不能重放 *-edits-once.json 覆盖后续修改。
- 新增基准：out/cut-plugin/fixture.html、baseline.png（1200x1060，16 个 object/embed/PNG/fallback/noembed/file/select/date/color 情况）。使用经过 SHA256 核验的第五批 out/Shot/shotium.exe 生成，provenance.json 实际文件名为 baseline-provenance.json；仅旧版参考，不是第六批回归通过。PNG object/embed 能绘制，file 默认标签文字在旧二进制这张图中已经为空；noembed 是 raw-text。后续候选须对比这张基准，原来 176 张加本图共 177 张。原有 Canvas 应与 out/canvas-assessment/canvas.png 对比，不要用 generic.png。
- 2026-09-07 本轮已开始 pnpm build:engine --gen-only --jobs 20 --log out/Shot/cut-batch6-gen.log（统一验证，不是小修改频繁编译）。执行前 Get-Process ninja/clang-cl/rustc/lld-link 为空，CIM 在沙箱内拒绝访问，未杀任何进程。GN 完成后顺序 missing-inputs、EXE/DLL jobs 20；失败按 build:errors 分组、check:syntax 批量修复后续跑，不恢复整个浏览器层。执行状态以最新工具及本文件后续追加为准。已有 vendor Skia/Perfetto/Vulkan 改动仍须保护，不能 stage gitlink 或整个目录。第六批验证后再继续 GPU 动画/输入/public WebFrame/ContainerTiming 等剩余所有清单项，不能把本次验证当最终任务完成。

- 第六批统一验证进展：GN 第一次通过后 missing-inputs 发现 shot_sources.gni 遗漏的 media/fullscreen_video_element.mojom；已删该源项并重新 GN 成功（6868 targets / 857 files），missing-inputs 7448 个源码输入全部存在。EXE 构建已实际启动 pnpm build:engine --jobs 20 --log out/Shot/cut-batch6-build.log，工具 exec session 95327。本轮编译期间不继续修改 C++ 源码；先收集这批完整失败集合，再按 build:errors / check:syntax 修复。不要另开并行构建；接续先确认此 session/明确任务进程状态。

- 第六批 j20 完整编译已结束（exit 1）：26 个失败 edge，除 8 类普通诊断外还出现多个 LLVM out of memory。完整失败日志另存 out/Shot/cut-batch6-j20-oom.log；旧 session 95327 已结束且没有遗留编译进程。按当前实际内存证据，剩余重型编译改用 jobs 8，不重复让 20 并发 OOM。第一轮 syntax-only 检查 26 个失败 TU 为 22/26 通过，又发现 OOM 掩盖的四处诊断；已修复并运行第二轮 syntax（session 72129，out/cut-batch6/syntax.log）。第六批仍未有新二进制、未提交。
- 编译修复没有恢复浏览器实现：删除 CompositedAnimationRequiresProperties 无调用函数、LayoutEmbeddedContent 仅插件光标 override、PreloadHelper 仅旧 hints 使用的 console helper。LocalFrameView ForceCommitCriteria / RequestMainFrameOnCompositorAnimation 及 ChromeClient/Empty 接口整链删除，接收端原本为空；IntersectionObserver/resize/position anchors 的实际生命周期保持。Document RandDouble、WebDocument referrer policy、FrameFetchContext callback helper、VisualViewport rect conversion 补直接 include；EditContext 暂时直接包含现存 WebLocalFrameClient 中 SyncCondition 定义，后续输入闭包删除该浏览器 selection 同步链。所有修复路径已纳入 stage10 manifest，勿重放旧 scratch。

- 第六批统一验证最终结果：EXE 和 DLL jobs 8 编译链接成功；serve/net、84 demos（62 exact/1 fuzzy/21 smoke）、新 addon 的 Node/daemon/协议、Bilibili 离线长页和 accept 全部通过；177/177 PNG 解码像素完全相同，新增 object/embed/file fixture 也相同。EXE 46,838,784 字节，SHA256 9989fced172bb90ea3f24eaeb0cea9da865e24e7d7d5b7148a324b4fa5ef089f；DLL 46,836,736 字节，SHA256 b882dc9c7c42e00a4bc884db0a2eca7b39de6084258fb49d8f1a979e7ad0a2fa。Linux probe 0 缺 BUILD、0 主仓库缺输入，3 Linux DEPS 和 1 宿主工具链仍未具备，六平台实际编译未完成。全部验证 session 已结束，无并行构建。详细记录已写执行报告第六批，结果资产 out/cut-batch6。
- 本批 249 删除文件均已复核不在磁盘，按精确父路径移除 10 个空目录（含整个 platform/widget，列表 out/cut-batch6/empty-directories.json），未递归清理 checkout。提交前仅明确暂存 stage10 manifest/deletions、两份任务/执行文档和 upstream-sync；Skia/Perfetto/Vulkan 既有改动不动。提交后接着 GPU animation / input / public WebFrame / ContainerTiming 等全部剩余清单，不能将第六批验证通过等同整个目标完成。
