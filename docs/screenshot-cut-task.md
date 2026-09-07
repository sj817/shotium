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

前三批 Windows EXE/DLL、84 demos、serve/net、Node/daemon/协议、Bilibili 和像素基线检查已完成，详见执行记录；六平台实际编译未完成。没有创建 PR 或发布。

## 剩余大批次清单

### 4. Canvas、服务空壳、采样 profiler（验证完成，待提交）

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

- [ ] services/network/public/cpp 拆资源加载实际类型，去掉 prefs/浏览器 policy/helper 的无用消费者。
- [ ] NetworkContext 全服务、代理/设备/存储等未连接协议与冗余 bindings、service_manager public 服务管理契约、独立 cert_verifier 服务残留。
- [ ] net PAC/WPAD/DHCP/系统代理：Shot 明确直连，切断后台服务/初始化；保留真正直连和 URLRequestContext 接口。
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

- 当前分支原为 `release`，最近完成提交 `7bdc3cfe9b56`。继续前以实时 git 状态为准。
- `out/cut-stage6/manifest.json`、`extra-edits.json`、`tail-edits.json` 记录 Canvas 和本批追加 owned paths；源码已应用，原始 hash 不能用于重放。Canvas 删除清单已扩大到 118 个文件并已实体删除，包括绘图值类型/IDL/生成绑定。`ImageElementBase` 已迁到 core/html 公共位置，canvas 目录仅留标签三文件。
- 暂停时 editing_utilities 的 Canvas image URL 分支已删除。恢复后集中构建仅先发现 thread_pool_impl 缺 FilePath 的直接 include；已修复，失败 TU syntax-only 1/1 通过。
- `out/cut-stage7/manifest.json` 和 `out/cut-stage8/manifest.json` 的服务/采样 profiler 修改已核对 hash 并应用，后续另有收尾修复，不能重新覆盖。临时文件直接位于对应 stage 路径下，没有 scratch 子目录。
- 当前整批已实体删除 225 个普通文件，证明在 `out/cut-batch4/deletion-proof.json`；已去掉 34 个核对过的空目录，sandbox/storage 等根目录消失。GN 通过、7,459 个输入全部存在，IDL 131 个枚举引用/43 个 union 名称引用均无缺失。
- 本批 Windows EXE/DLL 与完整运行回归已通过，176/176 像素对照一致，待提交。用户要求 jobs 31 后已实试，但多个 Blink animation TU 出现 LLVM OOM，记录 `out/Shot/cut-batch4-j31-oom.log`；已停掉明确属于本任务的 31 并发进程树并确认无遗留编译子进程，自动回退 jobs 8。当前日志 `out/Shot/cut-batch4-build.log`，不要与 OOM 日志混淆。
- `out/cut-stage9` 目前只有下一批只读调查：prefs 没有外部 C++ include，GN 路径 shot_core → network/public/cpp → prefs；旧 IPC 经网络 traits/url IPC 等被带入。尚未应用该批源码修改。
- 原始裁剪基线位于 `out/cut-baseline`，是本次任务从已验证 out/Shot 保存的对照资产，不是新编译验证对象。新产物一律用 `out/Shot`。
- Canvas 八组研究样本位于 `out/canvas-assessment`，保存真实 canvas 与普通元素差异。候选必须与 canvas.png 比，不能与 generic.png 比。
- 完成批次对照：169 demos + 4 render cases + corpus，174 张基线；补上 Canvas 专项 fixture。已有 Chrome oracle 差异 1.5245%，本次不得新增未解释差异。
- 不允许新起子代理（当前没有用户或项目授权）；按当前任务本地推进。

本文件各待办只有完成实际处理和对应验证后才能勾选。预算、运行时间或上下文切换不构成完成理由。

- 下一批统一收尾 Canvas 公共 property tree 子树状态、孤立 canvas_snapshot_info.h 和 ActiveScriptWrappableCreationKey 的旧 Canvas friend 声明；不保留无调用方的绘图 metadata。
