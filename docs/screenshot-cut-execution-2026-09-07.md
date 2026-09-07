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

## 后续批次

继续处理网络公共层、输入/合成器/GPU、诊断后端等剩余闭包，完整接续清单见 `screenshot-cut-task.md`。不把待处理或已关闭开关标为彻底删除。
