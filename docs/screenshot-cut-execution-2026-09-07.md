stage28 当前接续：stage27 已提交 df70ff97787b。本批删除 Skia 内置 Vulkan SDK 51文件及其rewrite_includes例外，清理独立DEPS的5个Vulkan条目；另删除21个GPU SkSL生成器/验证器及2份GN/Bazel源清单中的对应声明，覆盖GLSL/HLSL/Metal/SPIRV/WGSL/PipelineStage。共76源码路径、72文件删除。CPU core/core_module源列表逐字不变，4个RasterPipeline文件保留，SkSLGLSL.h的CPU编译器共享类型仍需保留。备份哈希、已删文件名引用、GN/Bazel/Python语法和diff检查通过；未生成当前图、编译/运行/像素验证。证据 out/cut-stage28-vulkan-headers/、out/cut-stage28-sksl/。独立Skia MODULE/Bazel/GN工具和Dawn/ANGLE/SPIRV依赖元数据仍有残留，后续需成组清理；原始根目录清单和最终统一验收仍未完成。

stage27 当前接续：stage26 已提交 e0e0551ed5e4。本批移除 SkMesh 及 Canvas/Device、record/capture/NWay/SVG/XPS 绘制转发和 GN/Bazel 声明；SkBitmapDevice 的该入口原本只有 TODO，实际 SkVertices CPU 绘制保留。同时清理 Skia/cc/Blink 的 texture-backed 查询、GPU 图像类型和加速图像 getter，保留 CPU 解码、缓存、lazy image、gainmap/tonemap、颜色与采样。去重后44源码路径，删除3文件。相关符号跟踪源码搜索无残留，GN语法1项、Bazel兼容语法2项及diff检查通过；未生成当前图、编译或运行/像素验证。证据 out/cut-stage27-mesh/、out/cut-stage27-texture-query/。其他根目录/第三方、生成器、tracing/AX/Worker等原始清单及最终统一验收继续待办。

stage26 当前接续：stage25 已提交 51aed1185794。GPU Slug 的SkCanvas/Device转换与绘制、SkPicture/SkRecord记录回放、序列化数组/标签/回调及capture/NWay转发已移除，删除Slug.h和SlugFromBuffer.cpp并同步GN/Bazel（26路径）；普通TextBlob/字形绘制保留，旧DRAW_SLUG位于枚举末端，删除未重排之前操作号。编码器JPEG/PNG/RustPNG/WebP的GPU context签名和调用方同步收口（15路径），8实现除参数和前置声明外文本一致，质量/颜色/压缩逻辑不变。合计41源码路径，静态调用/残留、GN/Bazel语法及diff检查通过；无C++编译/运行/像素验证。清单和原文 out/cut-stage26-slug/、out/cut-stage26-encoders/。仍需texture-backed跨cc/Blink类型链、SkMesh、生成器/第三方/其他原始根目录清单及最终统一验收。

stage25 当前接续：stage24 已提交 cd12b9220ca8。本批收掉Surface/Canvas/Device的空GPU recorder/context查询、纹理替换、等待/characterization、GPU类型枚举与前置声明（父侧8文件）；SkImage readPixels/getROPixels/onAsLegacyBitmap/makeRasterImage 去掉GPU context参数，CPU子类与缓存/缩放/编码/SVG等调用同步（子代理24文件），移除makeNonTextureImage及仅返回false的Lazy读像素GPU探针。保留正常CPU绘制与实际失败路径。当前仍未编译/运行，diff与调用残留检查为源码证据。清单 out/cut-stage25-surface/ 与 out/cut-stage25-skimage/。下一批必须处理GPU Slug记录/回放/序列化闭包（slug-references.txt），以及texture-backed跨cc/Blink查询和encoder外部GPU参数；不得当作GPU全部收尾完成。

stage24 当前接续：stage23 已提交 110e3281210f。Skia GPU/Ganesh/Graphite 的 src/gpu、include/gpu、include/private/gpu、src/text/gpu 1336 路径，加GPU独占私有头/两份GN源清单，共1346文件已删除；配套GN/Bazel和CPU桥接修改合计1362路径，314,084行删除。阴影源码与旧非Ganesh分支文本2/2一致，SkImageAndroid保留RasterFromBitmapNoCopy；SK_GRAPHITE_USE_LEGACY_RRECT_CLIP_SHADER原本也影响CPU SkRRect，已直接保留原启用公式，未改圆角判断。GN parse4/4、Bazel兼容语法7/7、diff与已删头引用检查通过，未生成当前图/编译/运行。原文备份与精确清单 out/cut-stage24-skia-gpu/、out/cut-stage24-gpu-cpu-companions/。SkSurface等头中无效GPU接口声明/空虚函数、其他shader/toolchain生成器及独立构建配置仍待继续收口，不能宣称全部GPU耦合已完成。

stage23 当前接续：stage22 已提交 e1417567d3f3。本批删除 Skia PDF 后端48文件、2个PDF公开头及pdf.gni，移除 //skia 关闭分支/空PDF实现和SetNodeIdOp、节点ID版文字绘制重载、Blink打印页眉页脚PDF标记；普通文字栅格调用、字体形状和DOM节点获取保留。71个源码/构建路径，51个文件删除，12,615行删除；GN解析3/3、Bazel兼容语法5/5、diff检查通过，当前C++仍未编译。清单/原文/证据 out/cut-stage23-pdf/。PDF特有绘制调用已无残余，其他printing/AX/DOM元数据及未使用独立GN配置仍待清理，不能据此宣称所有打印耦合已处理。子代理完成GPU只读审计：1336个候选及Android CPU边界在 out/cut-stage23-skia-gpu-audit/，尚未删除。

stage22 当前接续：stage21 已提交 4ace701b670c。CanvasKit（Emscripten JS/WASM）和 Skottie（Lottie）278 个文件及配套独立 GN、Bazel、presubmit 引用已移除；Skia 独立根 BUILD.gn/.gn、WASM/iOS示例入口和无用 fuzz.gni 同批删除。295 个源码/构建路径，约 74,699 行删除，原文 SHA 与备份在 out/cut-stage22-skia-{modules,standalone}/；保留主仓 //skia 的构建、原生 SkCanvas、SVG/字体/CPU绘制及共享实现。GN parse、限定 diff、Python/Bazel兼容语法检查通过，无当前图/编译/运行验证。历史 RELEASE_NOTES 中的模块说明保留为历史。GPU/Graphite、剩余独立构建元数据、其他无用模块及整个原始清单仍待继续，不能把本批当作 Skia 内部裁剪完成。

stage21 当前接续：stage20 已提交 d1e909e8c184，以下旧的“未提交”记录是历史。新增清理 base/net/DNS/services 的 FuzzTest 元数据、net_fuzztests/coep_fuzztest/base fuzztest_support，WTF/controller/CBOR 配套以及 WebP 8 个、rapidhash 1 个 fuzzing 目标；删除 libFuzzer proto/renderer 两个无实现 BUILD.gn。12 个源码路径，10 个现存 GN 文件语法解析通过，限定 diff 检查通过；尚未生成当前图、编译或运行。WebP 子仓仍含实际 fuzzing 源文件，通用模板和 Route core 依赖未闭合，均继续待办。证据在 out/cut-stage21-{network-fuzztests,library-fuzztests,proto-renderer}/。

stage20：并行完成 net/DNS/Mojo 9 个 fuzzing 支持目标、base fuzzing_buildflags 和 Linux fuzzing 覆盖率分支、Blink/Skia 49 个目标及 1 个模板清理。14 个源码路径，793 行删除；本批 GN 文件语法解析和限定 diff 检查通过，未生成当前构建图或编译。通用 FuzzTest/libFuzzer 和受 Route 范围约束的 core 包装器仍待闭合。

# 截图无用代码清理执行记录（2026-09-07）

## stage20：net/DNS/Mojo fuzzing 支持链

移除 9 个支持/协议目标及 Mojo 两个独占辅助源文件，生产实现保留。清单/原文备份在 out/cut-stage20-fuzzer-support/；3 个 GN 文件语法解析及目标/符号残留搜索通过，未生成、未编译、未提交。通用 fuzzing 闭包尚未完成。

## stage19：取消 patch，ICU 已直接纳入主仓

最新收尾：三库均已纳入主仓，4/4 补丁移除，检出与重放入口清除；补齐符号链接目标并保留原执行模式。脚本 TypeScript 检查、DEPS 语法检查通过，未构建引擎。stage18 已提交 d21776f19892（35 文件，删除 2672 行），stage19 独立提交；本轮报告见 screenshot-cut-stage19-report.md。下方 ICU 单库状态是中间记录。

ICU 996 个原文件加忽略规则和来源说明已转换为主仓普通文件；原子仓完整保存在 out/cut-stage19-direct-source/vendor-backup/icu。逐文件哈希一致，移除 ICU patch、gitlink、DEPS/.gitmodules 和 Windows 补丁步骤，三平台 repack 时间戳读取主仓历史。Skia/Perfetto 尚待迁移，本轮未编译、未提交，ICU 文件及 .gitmodules 已暂存。精确证据见 out/cut-stage19-direct-source/icu-migration-proof.json。

## stage18：MojoLPM 已移除，通用 fuzzing 待收尾

再追加：Breakpad minidump_fuzzer 的生成、复制和专用日志配置闭包已删；base/net/DNS/Mojo/URL 等 9 个 GN 文件再删 83 个 fuzzer_test 声明和导入。对应源输入字面路径均不存在，生产实现未删。清单见 out/cut-stage18-breakpad-fuzzer/ 和 out/cut-stage18-engine-fuzzers/，10 个 GN 文件语法解析、diff --check 通过，未生成/编译/运行/提交。net/DNS 支持库和 proto、Mojo 支持 group、base buildflags、Blink/skia、通用 runner 和 vendor 测试源仍待收尾。

追加两组源码：out/cut-stage18-fuzzable-proto/ 删除额外 fuzzable 协议生成模板及无实现自测，保留 metrics_proto 的原普通协议生成；out/cut-stage18-library-fuzzers/ 删除 13 个库 BUILD.gn 内的 17 处 fuzzer_test 声明、import 和独占配置，BoringSSL 循环目标也移除。15 个 GN 文件语法解析通过；无编译、无运行、未提交。BoringSSL/RE2 保留 gitlink 中的测试源尚未裁剪，通用 fuzzing 完整闭包仍未完成。

以 f4d07d2944a0 为基准，移除三个 Mojo fuzzing 文件并修改六个调用/构建文件，清单及原文备份见 out/cut-stage18-mojolpm/manifest.json。普通 Mojo 生成保留，删除原本 enable_mojom_fuzzer=false 的分支及其实现；生成器默认语言不再包含已删除的 mojolpm。三个 GN 文件、两个 Python 文件语法解析通过，跟踪源码中的 mojolpm/enable_mojom_fuzzer/fuzzers 路径残留为零（文档、Rust vendor 历史材料排除）。未生成、未编译、未运行，也未提交；后续合并测试骨架批次。通用 libFuzzer、FuzzTest、libprotobuf-mutator 仍在清理范围，Route 被拒补丁仍未应用。

**本批本地源码提交：62 删除、65 修改。** 精确路径及提交号记录在 out/cut-stage17/commit.json。仅保存已经应用的父任务改动；Route 92 路径补丁未获具体确认、未应用。部分对应调用方仍待该补丁收尾，本批未生成、编译或运行，不是可构建/验收通过的版本。不 push，不创建 PR。


## 恢复后的源码进度（2026-09-08）

用户已明确继续。当前新阶段以 64f61d2ef182 为基准，out/cut-stage17/progress.json 跟踪接续。父任务已落实网络 connection allowlist / SafeUrlPattern / liburlpattern、无实现的 Network Service GN 骨架、FileReader 与脚本 Object URL 尾巴、JsonCpp 空骨架，并移除不用的内存 HTTP 缓存后端，共删除 62 个普通跟踪文件；Route/CSS/URLPattern 完整提案被自动审批拒绝，等待具体确认，子代理已停止。保留 CSP/混合内容/TLS 等真实加载检查、普通 File/FileList/输入框外观、实际 MemoryCache 和 Shot 缓存 API。这里尚未生成、编译或运行验证；只读 GN path 暴露的一处旧 runtime action 无用赋值已修复。此段优先于下方暂停交接历史。


> 2026-09-08 用户已明确恢复任务（“算了 你继续吧。claude卡死了”）。从 64f61d2ef182 接续剩余裁剪；下面暂停交接记录为历史。本轮先完成 Route/URLPattern 与网络 allowlist 闭包，再集中验证，最多一名已获授权子代理。


> 2026-09-08 最新指令：本批源码收尾后暂停，等待用户接手。下文旧继续计划仅为历史记录；当前状态以 [交接报告](screenshot-cut-handoff-2026-09-08.md) 为准。

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

## 最新接续状态：第十二批 Viz/GPU 闭包进行中

当前基准为 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb，上一批 stage15 冻结；新批次使用 out/cut-stage16。已删除无调用的 BeginFrameProvider/Params/Client、Viz bundle ID 生成器、StaticBitmapImageTransform 及专用格式/读回 API（7 文件）；随后删除 embedded/unbounded surface、CompositorFrameSink/FrameSinkBundle 协议及 bundle ID/traits（7 文件），同步 GN/源清单及无调用的浏览器请求方法。普通 StaticBitmapImage 创建、CPU 绘制、EXIF 朝向保留；UnboundedElement 的死状态/绘制链尚需继续拆。证据来自完整 git grep 与 shot_core 到 platform/Viz/mojom 的 GN 路径，当前仅静态检查，尚未构建。按用户要求先完成整个 Viz/GPU 大批次再集中验证，jobs 8（最新实试 20 OOM）。拖放拒绝提案仍未应用，全部根目录目标继续 active，无 PR/推送/发布。

## 第十一批已提交（当前验证基线）

本段优先于后面的历史进度。基准提交 cd916149eb10080a0c176826e9308f96ef5dd82d；第十一批使用 out/cut-stage15，共 229 个 owned 路径（225 源码/构建输入、4 文档）和 576 个实体删除，总计 805 个变更路径。提交 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb，全部 Windows 验证通过；stage15 清单冻结。

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


## 2026-09-08：第十二批源码继续，改为剩余全范围最后统一验证

当前基准为 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb；stage15 冻结，当前源码整轮使用 out/cut-stage16。累计 115 个 owned 路径（含三份文档）、97 个实体删除，尚未编译、未提交，不计为新二进制验证通过。前段已处理 provider/图片转换、浮层协议、Unbounded 死状态、RemoteFrame/导航 keepalive、CC 帧元数据与 offset tag、Viz client/GPU context。随后删除浏览器可见计时 reporter/request/Mojo、显示帧回调与 FMP 专用检测/布局计数器/上报入口、WidgetScheduler 全工厂和生命周期/输入转发/idle 通知、GPU 图片复制器、GL 扩展/SharedImage provider/Canvas GPU timer/WebGL CPU 转换头、VisualProperties 与 FrameVisualProperties 传输字段/Mojo、GPU 查询接口及 Blink 主目标的 GPU/Viz 直接依赖；CC TextureBacking 工厂/存储/读回/绑定链也已删除。保留同步 CPU FCP、绘制结束 bookkeeping、实际网络 idle 回调、普通 scheduler 队列、iframe 几何稳定性阈值与图片 CPU 解码。新建第 184 张 Unbounded 旧二进制基准，尚未跑新二进制对照。

最新用户要求剩余所有范围先裁剪一遍，最后统一编译验收，优先于历史“每个大批次构建”的要求。下一步从 cc/paint 绘制序列化/SharedImage/transfer cache 接口继续 GPU/Viz 闭包，再连续处理网络/IPC、诊断、第三方、资源/构建/同步和根目录复核，不在 Viz/GPU 类别结束时提前启动完整构建。六平台实际编译仍欠缺；拖放拒绝提案未应用。目标继续 active，无 PR/推送/发布。

本段仅记录源码进度。逐文件备份、SHA256 删除证据及 owned 清单位于 out/cut-stage16；旧二进制仍是第十一批已验证版本。没有为上述每条闭包启动完整构建。


## 2026-09-08：整组删除 GPU/Viz 与 Skia GPU 构建入口，源码未编译

当前基准仍为 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb。父任务清单 out/cut-stage16 为 177 个修改路径、1,034 个实际删除文件；独立 Skottie/Lottie 清单 out/cut-stage16-skottie 为 26 个修改、51 个删除。合并去重后为 189 个修改路径、1,085 个删除文件。尚未编译、尚未提交，不能标为新二进制验证通过。

GPU/Viz 运行闭包、CC PaintOp 序列化/transfer cache、Skia GPU 编译入口等前序源码修改保留。gpu/ 已从磁盘消失；Viz 当前保留颜色/像素格式实际使用的 12 个文件。第三方 Skia checkout 本身的精简尚未完成，已有局部改动保留。

新增原生 UI 闭包删除 364 个文件：GL（暂留 features.gni）、Ozone、平台窗口、WM、原生 keyboard hook、OS 剪贴板/拖放/data-transfer-policy。Blink token 仍使用的剪贴板常量与 sequence token 两个头文件保留为 clipboard_types，解除整套 OS 剪贴板实现的传递依赖。此处不包含被审批拒绝的 Blink 14 文件拖放提案。ui/gfx 的几何、颜色、字体与表单默认外观仍是实际截图基础设施。

Skottie/Lottie 已移除 PaintOp/绘制 API、包装器、动画、ResourceBundle 解析与缓存、29 个 JSON 资源、GRD/配额和 Grit 专用生成分支。Skia 的 Skottie、jsonreader、skresources、sksg、primitive skshaper 编译来源移除；249 个当前 Skia 源文件中不存在这些模块。CPU SkSL、普通图片、SVG、文字和实际被滤镜使用的 Tween 保留。没有改第三方 Skia checkout。

scenario_api 的 8 个文件及调度器对应观察接口、注册/注销、空回调、3 个无调用辅助方法、V8 专用 feature 和每任务通知已删除。外部没有 ScopedScenarioObserverList 或共享场景内存创建者，回调原本已无实际内容；真实任务队列、RAIL/加载调度和 FCP 保留。scenario 和 skottie trace 分类同步移除。

合并静态证据见 out/cut-stage16/static-proof-combined.json：15,320 个现存代码/构建文件检查、119 个 C/C++/头文件预处理配对、全部备份 SHA256，0 问题。这不代替编译、运行和像素验证。当前最后通过验证的仍是第十一批 EXE/DLL 与 183 张像素，新增第 184 张 Unbounded 基线等待新二进制对照；六平台实际构建仍未完成。

用户最新并行限制：最多 1 个子代理。原网络与诊断代理已中断；唯一保留的代理完成 Skottie 后核验 HTMLParserMetrics / LocalFrameUkmAggregator 两条 UKM 记录闭包；源码写入被自动审批拒绝，目前只准备 out/cut-stage16-ukm 审阅材料，尚未执行 23 个修改和 6 个删除。它拥有对应调用方和 core/html/build.gni、core/frame/build.gni、必要的 page/build.gni；core/BUILD.gn、platform/BUILD.gn 由父代理整合。保留 IntersectionObserver 的独立内部标志、懒加载、display-lock 与真实生命周期。

自动审批尚未放行四组精确提案：Service Manager（53 文件删除、9 文件修改）、ANGLE/SPIR-V/Vulkan（10 文件修改、12 普通文件删除、5 个第三方 checkout/gitlink 与同步条目）、Blink 拖放（11 修改、3 删除）、UKM 记录链（23 修改、6 删除）。四组均未执行；总审阅清单为 out/cut-stage16/approval-review.md。Service Manager 完整 patch 在 service-manager-proposal-review 子目录；ANGLE 精确范围及第三方改动保留方案在 angle-proposal-review/scope.json；拖放仍用 out/cut-stage11/drag-proposal-review/scope.json；UKM 的 proposal.patch、review.md 和清单位于 out/cut-stage16-ukm。ANGLE 与 Vulkan loader 的既有局部改动必须完整备份，不直接清除。旧拖放 patch 与并发变更必须按当前文件重新核对。未获得明确放行前不重试或绕过拒绝。

继续网络/旧 IPC、其余诊断、第三方/资源/生成器/同步和根目录全量复核。严格遵守最新节奏：剩余源码全部处理一遍后，集中 GN/缺失输入/编译修复/EXE+DLL/运行及像素验收。20 并发已实测再次 OOM，最终构建使用已通过的 8 并发，不更改仓库永久默认值。目标保持 active，不创建 PR、不 push、不发布。


## 2026-09-08：旧 IPC、第三方骨架和 XSLT 脚本尾巴，源码未编译

当前基准仍为 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb。剩余源码整轮裁剪累计已有 1,187 个跟踪文件实际删除，新增 3 个明确的 Mojo traits 头文件；各组修改路径与备份分别在 out/cut-stage16、cut-stage16-skottie、cut-stage16-ipc、cut-stage16-ipc-generator、cut-stage16-scaffolds、cut-stage16-xslt-api、cut-stage16-gfx-tail。源码尚未集中编译、运行或提交；最后已验证二进制仍为第十一批，不能把当前源文件删除算成新版本验收。

GPU/Viz、CC PaintOp 序列化/transfer cache、Skia GPU 编译入口、364 个原生 UI 文件、51 个 Skottie/Lottie 文件和 8 个 scenario_api 文件的删除已经落盘。gpu/、ipc/、UI 下 Ozone/平台窗口/WM/Lottie 及 UI/URL 的旧 IPC 目录已消失。ui/gl 暂留 features.gni，随待确认 ANGLE 组处理。Viz 的 12 个 CPU 颜色/像素格式文件、SkCanvas、SVG/图片/字体/表单主题及真实调度保留；第三方 Skia checkout 尚未完整收窄，原有局部修改保留。

新增旧 IPC 闭包删除 77 文件：五个 [Native] 类型改为显式 Mojo enum/struct，保留 ConnectionInfo 0–42、ECT 0–5、事件类型值和 RedirectInfo 原有 10 字段。删除 Channel/Proxy/附件/旧 ParamTraits、native 序列化桥、fuzzer 名称探针和调度器紧急消息回调/计数/提权分支；默认任务仍为 normal priority。普通 Mojo/ipcz、Shot stdio/daemon 协议、net 缓存 Pickle 不变。生成器及属性检查拒绝重新引入 legacy [Native]，普通 C++ typemap 保留。当前 GN 2,494 个依赖中已无旧 IPC；这只说明依赖图，不是编译通过。

新增八个第三方骨架组与失效工具共删除 24 跟踪文件：anonymous_tokens、leveldatabase、snappy、libpfm4、ocmock、win_virtual_display、inspector_protocol、google_benchmark。同步去掉测试/GN 引用、BoringSSL 可见性、Snappy unbundle/sysroot 项、IndexedDB Snappy 空 feature、LevelDB dump 名称、失效 DevTools PDL 更新工具和 DEPS include allowlist；八个目录均已从磁盘清除，没有保留这些包的 DEPS/gitlink 同步项。inspector_protocol 的一个孤立 pyc 已备份后删除。

XSLT 原生声明式转换保留，脚本专用 importStylesheet、transformToDocument/Fragment、参数 API/参数表、包装类型、无消费者的 fragment helper 和脚本 stylesheet 构造器已删；xslt_processor.idl 删除。PI→样式表加载→原生事件→libxslt→新文档的链、xsl:param 默认值、同源加载、禁止写文件/网络的检查、编码、排序和 CAP 行为保留。libxslt 原有 xsltQuoteUserParams(ctx, nullptr) 直接返回 0，因此去掉空外部参数转换不删除 XSL 文件内的参数求值。GFX 同步删除无人调用的 AsGLColorSpace 和旧 media 友元；CPU 颜色转换与普通 Mojo traits 保留。

空目录使用经过绝对路径校验的非递归 rmdir，逐个确认无文件后删除；本轮另外清掉 358 个空目录，含先前留下的 Ozone 与 Android/UI 子目录。没有递归删除共享 checkout、已有第三方修改或 out。证据为 out/cut-stage16/empty-directory-proof.json。

合并静态检查见 out/cut-stage16/static-proof-combined.json：16,049 个现存代码/构建输入、157 个已修改 C/C++/头文件预处理配对、当前新阶段 188 份原始 SHA 备份，0 问题；保留早期父任务和 Skottie 的备份证据。没有运行生成器测试、引擎构建或像素验证。第十一批 183 张已验证像素、新增第 184 张 Unbounded 旧二进制基线及六平台编译边界保持原记录。

最多 1 个子代理；其他两名已停止。唯一子代理完成 Mojo 生成器和声明式研究后，正将被拒的 Sanitizer/Skeleton 改动写成 out/ 投影审阅提案，真实源码未改。声明式结论见 out/cut-stage16-declarative-review.md：XSLT 有真实用途；view-transition-name 有静态 3D 分组/backdrop 作用；Origin Trial 无策略注册者，Route/URLPattern 的声明式入口未启用，后两组尚未实施，不能按“有引用”永久保留。

自动审批拒绝且未应用六组：Service Manager、ANGLE/SPIR-V/Vulkan、Blink 拖放、UKM、NQE/NetLog、Sanitizer/Skeleton。具体文件、理由、完整补丁或提案完成边界见 out/cut-stage16/approval-review.md 及各子目录。NQE、Service Manager 等旧提案必须按当前独立改动重新核对；不得重放旧 patch、改用工具或拆分绕过拒绝。第三方既有局部改动先完整保存。

接续先处理已被拒组的明确授权与其余网络/协议、诊断/Perfetto/Crashpad/AX、OriginTrial/Route/URLPattern、第三方/资源/同步闭包，再统一 GN、缺失输入、语法修复、EXE/DLL、运行和像素门。20 并发已再次实测 OOM，最终使用已成功的 8 并发。目标继续 active；不创建 PR、不 push、不发布。当前逐根目录进展见 docs/screenshot-root-status-2026-09-08.md。

## 2026-09-08 最终交接：七组已应用，源码提交未编译

**用户最新要求：本批完成后暂停并交接。此指令优先于下面所有历史 active 目标和继续执行计划；未经用户再次要求，不再裁剪、构建或启动子代理。**

本批七组已明确授权并全部应用：Service Manager、ANGLE/SPIR-V/Vulkan、Blink 拖放、UKM、NQE/FileNetLog、Sanitizer/Skeleton、Origin Trial。加上本轮前半部分 GPU/Viz、旧 IPC、原生 UI、Skottie、第三方骨架、XSLT 脚本 API 与输入预测清理，相对最后验证提交 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb 共删除 1,404 个普通跟踪文件、5 个 gitlink，新增 3 个 Mojo traits 头文件。源码与文档合为一个本地提交，提交号及精确路径见 out/cut-stage16-final/commit.json；不创建 PR、不 push、不发布。

七组源码完成为 7/7；整个裁剪目标尚未完成。已删除 gpu/、ipc/、services/service_manager、ui/gl 及 ANGLE/SPIR-V/Vulkan checkout；五个第三方完整仓库和既有修改保存在 out/cut-stage16-angle-authorized/vendor-backup。仍保留截图所需 CPU 绘制、CSS/DOM/布局、SVG、图片、字体、表单主题、Canvas 备用布局、原生 XSLT、普通 Mojo 和实际网络加载。

静态证据 out/cut-stage16/static-proof-combined.json：15,905 个代码/构建输入、285 个改动 C/C++ 预处理配对、653 份新阶段原始 SHA 备份；扫描发现的一个 drag_state.h 旧 include 已补删，当前记录 0 问题。早期父阶段和 Skottie 备份另有原始证据。

**本批尚未 GN 生成、编译/链接、重建 addon、运行或像素验收，不能宣称可构建或截图无回归。** out/Shot 仍是第十一批旧二进制：183 张已验证像素，第 184 张 Unbounded 只有旧二进制基线；六平台实际编译仍未完成。以后由用户决定是否验证，建议用已成功的 jobs 8（20 已多次实际 OOM），不改永久默认值。

剩余网络/公共协议/FileReader/Blob/Worker、诊断/Perfetto/Crashpad/AX、UI/display/拖放辅助、Route/URLPattern、Skia 实际 checkout 及其他第三方/生成器/同步尾巴，连同接手顺序与回退位置，全部见 [交接报告](screenshot-cut-handoff-2026-09-08.md)。逐根目录状态见 [根目录报告](screenshot-root-status-2026-09-08.md)。子代理已停止。

### 当前具体阻断

Route/CSS/URLPattern 的 92 路径补丁（44 修改 / 48 删除）被自动审批拒绝，真实文件 SHA 92/92 未变，尚未应用。原因、完整补丁和精确清单见 out/cut-stage17/approval-review.md；等待用户具体确认后继续本组。父任务 58 文件删除仍在工作区，尚未提交/生成/编译，本轮不能标为可构建。不得重放或换工具绕过拒绝。

### 独立完成：内存 HTTP 缓存后端

新增删除 net/disk_cache/memory 四文件，以及 InMemory 工厂、builder 选项与两个统计条件的专用分支。Shot 只有禁用缓存或显式 DISK_SIMPLE，OnBackendCreated 失败不选择其他后端；缓存失败继续无缓存运行。内部 HttpCacheParams 默认改为 DISK_SIMPLE，保留既有后续枚举数值；Simple 索引、Blink MemoryCache、Cookie 与 Shot 缓存 API 不变。证据/备份在 out/cut-stage17-memory-http-cache。当前父任务累计 62 删除、66 唯一修改；备份校验通过，未生成/编译/运行。Route 92 路径仍未应用，不能把整个源码批次标完成。
