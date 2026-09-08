stage41 当前接续：stage40 已提交 ecb62e916c0d。本批解除discardable memory、PaintController、Partitions三处组件CrashKey消费者：删除仅用于附加崩溃信息的计数回调/快照及mapped-size格式化，保留原内存计数/释放/锁、OOM分支、错误日志和NOTREACHED。同步service/WTF GN；删除无人消费的crash_keys与shared_memory_user_stream共7文件及common/utils/旧平台测试目标。当前组件CrashKey外部源码调用为零，但受保护core/BUILD.gn:1214仍依赖crash_key，库本身和Crashpad尚未删除。另删services/metrics孤立根BUILD，清理public/cpp两个无消费者测试目标；UKM真实Document调用与生成器继续待解耦。合计16源码路径、8文件实际删除；备份SHA、删除存在性、已删头引用、4份GN语法和diff检查通过，未生成当前图/编译/运行/像素验收。证据out/cut-stage41-crash-consumers/、out/cut-stage41-metrics/。原始全量清理仍待完成，三组拒绝范围未触碰。另发现build/private_code_test/private_code_test.gni保留已无components/metrics等路径例外，待独立构建骨架清理。

stage40 当前接续：stage39 已提交 841e5c753419。本批实际删除gfx桌面菜单/窗口控制、屏幕更新抑制、提权图标、物理屏幕尺寸探测、GPU子窗口管理、热键观察及native view helper与CrashIdHelper共19文件，3配套移除WindowImpl调试ID/CrashId调用和gfx crash依赖；通知窗口、DPI/字体/颜色及错误检查保持。ui/base删除identifier/metadata/unowned骨架、无消费者时间/字节格式化、startup ResourceBundle helper、collator及4个Mojo协议共18文件，3配套仅收口GN和无用include；保留实际ResourceBundle/l10n字体/attributed-string/menu/windowstate等。合计43源码路径、37文件实际删除。备份SHA、已删头/协议引用、3份GN语法及diff检查通过，关键保留实现与HEAD逐字一致；未生成当前图、编译或运行/像素验收。证据out/cut-stage40-gfx/、out/cut-stage40-base/、out/cut-stage40-base-audit/。Crashpad仍有内存管理/paint/allocator三个活跃诊断消费者与受保护coreGN，UKM及其他原始全量审计和统一验收仍待办；三组拒绝范围未触碰。

stage39 当前接续：stage38 已提交 1cfffcd291bb。本批实际删除无人使用的委托墨迹数据/Point/Renderer Mojo及序列化，移除ChromeClient空SetDelegatedInkMetadata接口；删除favicon/桌面monogram/nine-image/spinner/扩展集合/旧插值及顺序ID辅助。桌面Animation调度/container/runner/subclass及duration scale共24文件退出，Windows字体通知中仅删除更新无人读取动画偏好缓存的调用，字体参数和缓存失效逻辑不变；保留Tween/keyframe及Blink CSS prefers-reduced-motion设置链。另删14个原生Mac/Win/XKB键码转换与旧事件feature文件，保留实际KeycodeConverter、DOM扫描码表、KeyboardCode枚举和时间戳/latency。合计71源码路径、63文件实际删除，8配套含5个无消费者switch/feature收口。备份SHA、删除存在性、4份GN语法、diff及已删头引用检查通过，关键保留文件与HEAD逐字一致；尚未生成当前图/编译/运行/像素验证。证据out/cut-stage39-gfx/、out/cut-stage39-events/、out/cut-stage39-events-audit/。原始全量清理和统一验收仍待完成，三组拒绝范围未触碰。

stage38 当前接续：stage37 已提交 84b604ec07dc。本批实际删除 display 配置/颜色管理 Mojo、native delegate、独立显示旋转及辅助代码40文件，3份GN配套；移除其唯一消费者退出后的gfx overlay转换和Mojo5文件、2份GN配套及头清单。另删Khronos全14文件及Blink无人消费的graphics_types_3d.h，平台GN和头清单同步收口。合计67源码路径、60文件实际删除。Screen/DPI/字体主题/ScreenInfo/DisplayColorSpaces及CPU渲染保持；备份SHA、删除存在性、6份GN语法、diff与已删头引用检查通过。尚无当前构建图、编译或运行/像素验收；原始全量审计、UKM/Crashpad/其余第三方与最终统一验收仍待处理。证据out/cut-stage38-display-audit/、out/cut-stage38-display/、out/cut-stage38-display-deletions/、out/cut-stage38-khronos/。此前三组拒绝范围未触碰。

stage37 当前接续：stage36 已提交 2e30e70cfe3f。本批实际删除原生Event/dispatch/平台桥/devices/Ozone及无人使用event.mojom/serializer共105文件，4份GN解除过宽类型映射和依赖；真实Blink事件枚举、键码/时间戳/latency类型及range-check保留。另删GpuExtraInfo/ANGLE诊断数据及Mojo、VSync基类/export骨架8文件，收掉Screen空GPU信息接口、NativeEvent旧别名/前置声明、12个无消费者GPU/合成switch（7配套）；libsync源码checkout原本缺失，本批删4个wrapper/metadata文件并移除DEPS和.gitmodules同步入口，避免再拉回。合计130源码路径、117文件实际删除；6份GN语法、DEPS/.gitmodules语法、原文SHA/删除存在性/精确残留和diff检查通过，未生成当前图/编译/运行/像素验证。证据out/cut-stage37-events-audit/、out/cut-stage37-events/、out/cut-stage37-events-deletions/、out/cut-stage37-gpu-diagnostics/、out/cut-stage37-libsync/。保留NativeView/Window/Cursor、字体和主题；原始全量审计、UKM/Crashpad/其余平台/第三方及最终统一验收仍待处理，三组拒绝路径未触碰。

stage36 当前接续：stage35 已提交 464aa447afbd。本批实际删除ui/events/blink、gesture_detection、gestures、velocity_tracker及滚动曲线76文件，保留真实Event/键码/时间戳/滚动类型；2处配套解除过宽sources/deps和event.h无用include，Windows WebMouseEventBuilder→ScreenWin调用已随实现移除。另删除gfx GPU Fence/NativePixmap/GPU buffer/IOSurface/CALayer/Swap/Presentation及Mojo序列化66文件，4配套收口memory_buffer/native_handle目标及typemap；buffer_types.h、overlay_transform.h和转换实现4文件与HEAD逐字一致。合计148源码路径、142文件实际删除。备份SHA、删除存在性、已删头/协议引用、3份GN语法及diff检查通过，无当前图/编译/运行/像素验收。证据 out/cut-stage36-events-audit/、out/cut-stage36-events/、out/cut-stage36-events-deletions/、out/cut-stage36-gfx-gpu/。仍需Event serializer/native平台链、Screen GPU附加信息与VSync基类、UKM/Crashpad及原始其他根目录项和最终统一验收；受拒绝三组路径保持原状，不宣称全部已完成。

stage35 当前接续：stage34 已提交 25fe94e6fd39。本批实际删除31个无外部消费者的桌面UI helper、CursorFactory、pointer探测/TouchUiController、device_form_factor/view_prop文件，3配套收口；pointer_device.h仅保留原PointerType/HoverType两enum，枚举体逐字不变。另删除mac DisplayLink/VSync及独占screen_utils共14文件，移除2个开关、Metal链接和Jumbo例外（3配套）；删除浏览器崩溃处理/JS错误上报/WER/旧discardable客户端4个孤立BUILD。合计55源码路径、49文件实际删除。三个GN语法、备份SHA、已删头/符号/目标残留和diff检查通过；未生成当前图或编译/运行/像素验收。保留Blink真实Clipboard/Cursor/Dragdrop类型、Resource/l10n/原生主题、Shot实际共享内存管理器和现有crash annotation调用；UKM及Crashpad的活跃调用仍是待解耦任务。证据 out/cut-stage35-ui-audit/、out/cut-stage35-ui/、out/cut-stage35-ui-deletions/、out/cut-stage35-display-link/、out/cut-stage35-orphan-targets/。受拒绝三组路径未触碰，原始全量审计与最终统一验收仍未完成。

stage34 当前接续：stage33 已提交 d71c109d386a。本批删除 Windows/Cocoa 桌面窗口、光标、OLE、菜单、远程 layer 和无消费者 IME 控制器/候选窗等111文件，4处 GN 配套；保留 mac defaults_utils 的原生主题调用及 Blink 使用的 IME span/text/mojom 类型，blink_headers 依赖收窄至 :ime_types。另删除 PCI/语音播报独立加载器和元数据5文件，清理16处生成器、DEPS include规则、安装依赖及sysroot包清单。合计136源码路径、116文件实际删除。备份SHA、删除存在性、已删头/加载器引用、6份GN语法、Python/DEPS语法和diff检查通过；未生成当前图或编译/运行/像素验证。证据 out/cut-stage34-ui-audit/、out/cut-stage34-ui/、out/cut-stage34-ui-deletions/、out/cut-stage34-linux-loaders/。Route92、Perfetto2039、ICU16待授权范围未触碰；完整原始任务继续待办。

stage33 当前接续：stage32 已提交 757ba692c76e。本批实际删除ui/base桌面accelerators、interaction、models（保留menu_separator_types.h）、base_window、user_activity、WebUI/template及独占cocoa menu/xdg shortcut/themed icon/export配套、空idle骨架和ui/linux status icon，共95文件；ui/base/BUILD.gn与ui/linux/BUILD.gn同步收口并清空无操作分支，合计97源码路径。原生主题实际消费menu_separator_types.h，ResourceBundle/l10n/字体/表单和Linux平台支持保留。备份SHA、删除存在性、已删头/类型及GN标签残留、GN语法和diff检查通过；无生成当前图/编译/运行/像素验证。证据 out/cut-stage33-ui-deletions/、out/cut-stage33-ui/和out/cut-stage33-ui-audit/。与Route92、Perfetto2039、ICU16拒绝清单交集为零，这些待授权范围保持原状；完整原始任务仍有待办。

stage32 当前接续（部分完成）：stage31 已提交 7de589d51fb7。独立处理root build_overrides的Mesa/Meson/PDFium无消费者配置3文件，移除.gn内PDFium/DevTools失效参数及Siso对Dawn的7项例外，共5路径。ICU仅应用BUILD.gn/sources.gni两处配套（normalization_test、Wasm分支、msvcres头清单），保留实际cast→icu-repack→shot数据及native汇编嵌入链。ICU16个独立元数据/测试/Wasm文件删除被自动审批以blocked by policy拒绝，仍全部存在，未重试；与此前Perfetto2039文件分别属于待具体授权清单。不能把这两组写成物理已删。root配置语法/diff、ICU GN语法及备份检查为静态证据，无当前图/编译/运行验证。证据 out/cut-stage32-build-overrides/、out/cut-stage32-icu/；此前Perfetto精确清单 out/cut-stage31-trace-processor/approval-review.md。完整任务仍未完成。

stage31 当前接续（部分完成）：stage30 已提交 8b32de799b8c。已删除Perfetto独立Android/Bazel/Meson入口及生成/检查器18文件、UI构建/发布骨架2文件，移除旧pre-push安装；base/cppgc的TraceProcessor JSON导出分支和对应base buildflag/参数收口8文件。子代理已应用23个GN/普通protozero测试配套编辑，解除libperfetto无条件memory_graph依赖并移动普通测试描述符生成位置。拟删除的2039个processor/离线工具文件被自动审批以blocked by policy拒绝，实时核对2039/2039仍存在，未重试/拆分；绝不能记录为已删。具体清单、原文、已应用补丁及拒绝记录 out/cut-stage31-trace-processor/approval-review.md。其他证据 out/cut-stage31-perfetto-build/、out/cut-stage31-perfetto-ui/、out/cut-stage31-base-json/。当前仅静态GN/Python/diff检查，无图/编译/运行验证；Python/测试/站点及配置残留与原始全量任务继续待办。

stage30 当前接续：stage29 已提交 c9f2c8726ae1。本批删除独立工具/platform_tools/experimental工具109文件，以及bentleyottmann、skplaintexteditor、skparagraph、sksg、jsonreader、skresources、modules/svg、skshaper、skunicode九模块276文件，共385源码文件删除。真实截图SVG仍走Blink SVGImage绘制记录，字体走Blink HarfBuzz/Skia字体端口；保留modules/skcms、pathops源清单、src/svg writer，以及Blink/SkCodec实际调用的experimental/rust_ico五文件。精确调用审计未发现代码/GN外部消费者，余下引用仅历史RELEASE_NOTES。385备份哈希和删除存在性、16GNI语法及diff检查通过；未生成图/编译/运行/像素验收。证据 out/cut-stage30-tools/、out/cut-stage30-modules/、out/cut-stage30-modules-audit/。其他第三方/根目录清单及最终统一验收仍待完成，不据此宣称全部裁剪结束。

stage29 当前接续：stage28 已提交 bc0e517d2f8c。本批整体退役 Skia 独立 Bazel/GN 配置、工具链、构建/生成脚本、全部散布Bazel定义、上游CI/PRESUBMIT及测试、旧vendor roll和Go/Gerrit辅助工具；独立DEPS及ANGLE/Dawn/D3D12/SPIRV/window包装器也已删除。共451源码路径，其中438删除、13份GNI仅更新直接维护说明。主仓三个Skia入口实际导入的16份GNI完整保留，13个注释修改文件的非注释文本逐字一致；16份GN语法解析和diff检查通过。备份/清单 out/cut-stage29-standalone/、out/cut-stage29-gpu-wrappers/，调用审计 out/cut-stage29-standalone-audit/。未生成当前图、编译或运行；Skia其余工具/未用模块和原始其他根目录清单、最终六平台与像素验收仍待完成。

stage28 当前接续：stage27 已提交 df70ff97787b。本批删除 Skia 内置 Vulkan SDK 51文件及其rewrite_includes例外，清理独立DEPS的5个Vulkan条目；另删除21个GPU SkSL生成器/验证器及2份GN/Bazel源清单中的对应声明，覆盖GLSL/HLSL/Metal/SPIRV/WGSL/PipelineStage。共76源码路径、72文件删除。CPU core/core_module源列表逐字不变，4个RasterPipeline文件保留，SkSLGLSL.h的CPU编译器共享类型仍需保留。备份哈希、已删文件名引用、GN/Bazel/Python语法和diff检查通过；未生成当前图、编译/运行/像素验证。证据 out/cut-stage28-vulkan-headers/、out/cut-stage28-sksl/。独立Skia MODULE/Bazel/GN工具和Dawn/ANGLE/SPIRV依赖元数据仍有残留，后续需成组清理；原始根目录清单和最终统一验收仍未完成。

stage27 当前接续：stage26 已提交 e0e0551ed5e4。本批移除 SkMesh 及 Canvas/Device、record/capture/NWay/SVG/XPS 绘制转发和 GN/Bazel 声明；SkBitmapDevice 的该入口原本只有 TODO，实际 SkVertices CPU 绘制保留。同时清理 Skia/cc/Blink 的 texture-backed 查询、GPU 图像类型和加速图像 getter，保留 CPU 解码、缓存、lazy image、gainmap/tonemap、颜色与采样。去重后44源码路径，删除3文件。相关符号跟踪源码搜索无残留，GN语法1项、Bazel兼容语法2项及diff检查通过；未生成当前图、编译或运行/像素验证。证据 out/cut-stage27-mesh/、out/cut-stage27-texture-query/。其他根目录/第三方、生成器、tracing/AX/Worker等原始清单及最终统一验收继续待办。

stage26 当前接续：stage25 已提交 51aed1185794。GPU Slug 的SkCanvas/Device转换与绘制、SkPicture/SkRecord记录回放、序列化数组/标签/回调及capture/NWay转发已移除，删除Slug.h和SlugFromBuffer.cpp并同步GN/Bazel（26路径）；普通TextBlob/字形绘制保留，旧DRAW_SLUG位于枚举末端，删除未重排之前操作号。编码器JPEG/PNG/RustPNG/WebP的GPU context签名和调用方同步收口（15路径），8实现除参数和前置声明外文本一致，质量/颜色/压缩逻辑不变。合计41源码路径，静态调用/残留、GN/Bazel语法及diff检查通过；无C++编译/运行/像素验证。清单和原文 out/cut-stage26-slug/、out/cut-stage26-encoders/。仍需texture-backed跨cc/Blink类型链、SkMesh、生成器/第三方/其他原始根目录清单及最终统一验收。

stage25 当前接续：stage24 已提交 cd12b9220ca8。本批收掉Surface/Canvas/Device的空GPU recorder/context查询、纹理替换、等待/characterization、GPU类型枚举与前置声明（父侧8文件）；SkImage readPixels/getROPixels/onAsLegacyBitmap/makeRasterImage 去掉GPU context参数，CPU子类与缓存/缩放/编码/SVG等调用同步（子代理24文件），移除makeNonTextureImage及仅返回false的Lazy读像素GPU探针。保留正常CPU绘制与实际失败路径。当前仍未编译/运行，diff与调用残留检查为源码证据。清单 out/cut-stage25-surface/ 与 out/cut-stage25-skimage/。下一批必须处理GPU Slug记录/回放/序列化闭包（slug-references.txt），以及texture-backed跨cc/Blink查询和encoder外部GPU参数；不得当作GPU全部收尾完成。

stage24 当前接续：stage23 已提交 110e3281210f。Skia GPU/Ganesh/Graphite 的 src/gpu、include/gpu、include/private/gpu、src/text/gpu 1336 路径，加GPU独占私有头/两份GN源清单，共1346文件已删除；配套GN/Bazel和CPU桥接修改合计1362路径，314,084行删除。阴影源码与旧非Ganesh分支文本2/2一致，SkImageAndroid保留RasterFromBitmapNoCopy；SK_GRAPHITE_USE_LEGACY_RRECT_CLIP_SHADER原本也影响CPU SkRRect，已直接保留原启用公式，未改圆角判断。GN parse4/4、Bazel兼容语法7/7、diff与已删头引用检查通过，未生成当前图/编译/运行。原文备份与精确清单 out/cut-stage24-skia-gpu/、out/cut-stage24-gpu-cpu-companions/。SkSurface等头中无效GPU接口声明/空虚函数、其他shader/toolchain生成器及独立构建配置仍待继续收口，不能宣称全部GPU耦合已完成。

stage23 当前接续：stage22 已提交 e1417567d3f3。本批删除 Skia PDF 后端48文件、2个PDF公开头及pdf.gni，移除 //skia 关闭分支/空PDF实现和SetNodeIdOp、节点ID版文字绘制重载、Blink打印页眉页脚PDF标记；普通文字栅格调用、字体形状和DOM节点获取保留。71个源码/构建路径，51个文件删除，12,615行删除；GN解析3/3、Bazel兼容语法5/5、diff检查通过，当前C++仍未编译。清单/原文/证据 out/cut-stage23-pdf/。PDF特有绘制调用已无残余，其他printing/AX/DOM元数据及未使用独立GN配置仍待清理，不能据此宣称所有打印耦合已处理。子代理完成GPU只读审计：1336个候选及Android CPU边界在 out/cut-stage23-skia-gpu-audit/，尚未删除。

stage22 当前接续：stage21 已提交 4ace701b670c。CanvasKit（Emscripten JS/WASM）和 Skottie（Lottie）278 个文件及配套独立 GN、Bazel、presubmit 引用已移除；Skia 独立根 BUILD.gn/.gn、WASM/iOS示例入口和无用 fuzz.gni 同批删除。295 个源码/构建路径，约 74,699 行删除，原文 SHA 与备份在 out/cut-stage22-skia-{modules,standalone}/；保留主仓 //skia 的构建、原生 SkCanvas、SVG/字体/CPU绘制及共享实现。GN parse、限定 diff、Python/Bazel兼容语法检查通过，无当前图/编译/运行验证。历史 RELEASE_NOTES 中的模块说明保留为历史。GPU/Graphite、剩余独立构建元数据、其他无用模块及整个原始清单仍待继续，不能把本批当作 Skia 内部裁剪完成。

stage21 当前接续：stage20 已提交 d1e909e8c184，以下旧的“未提交”记录是历史。新增清理 base/net/DNS/services 的 FuzzTest 元数据、net_fuzztests/coep_fuzztest/base fuzztest_support，WTF/controller/CBOR 配套以及 WebP 8 个、rapidhash 1 个 fuzzing 目标；删除 libFuzzer proto/renderer 两个无实现 BUILD.gn。12 个源码路径，10 个现存 GN 文件语法解析通过，限定 diff 检查通过；尚未生成当前图、编译或运行。WebP 子仓仍含实际 fuzzing 源文件，通用模板和 Route core 依赖未闭合，均继续待办。证据在 out/cut-stage21-{network-fuzztests,library-fuzztests,proto-renderer}/。

stage20：并行完成 net/DNS/Mojo 9 个 fuzzing 支持目标、base fuzzing_buildflags 和 Linux fuzzing 覆盖率分支、Blink/Skia 49 个目标及 1 个模板清理。14 个源码路径，793 行删除；本批 GN 文件语法解析和限定 diff 检查通过，未生成当前构建图或编译。通用 FuzzTest/libFuzzer 和受 Route 范围约束的 core 包装器仍待闭合。

# 静态截图无用代码与残留依赖审计

stage20 回填：net/DNS/Mojo 的 9 个旧 fuzzing 支持/协议目标与两个专用辅助源文件已移除，普通生产库保留。精确清单见 out/cut-stage20-fuzzer-support/manifest.json。仅语法解析和残留核查，当前未提交/编译，通用测试支持链仍在清理。

## stage19 回填：ICU 直接源码维护

三库均已完成管理方式迁移：ICU、Skia、Perfetto 是普通主仓源码，原独立检出入口、4 个 patch 和应用逻辑已移除。保留源码与原文件一致，历史图覆盖不是最终六平台验证；第三方内部无用实现和最终根目录裁剪仍有待办。具体完成/未完成边界见 screenshot-cut-stage19-report.md。

third_party/icu 已由 gitlink 转为主仓普通源码，保留 common/i18n/stubdata、GN/必要生成脚本、数据与许可证，996 个原文件哈希不变。原子仓与未纳入文件完整保存于 out/cut-stage19-direct-source/vendor-backup/icu；ICU patch/检出入口已移除。Skia/Perfetto 仍待迁移。这里只证明源码转换与备份，未编译验收、未提交。

## 最新回填：MojoLPM 生成链

最新追加：Breakpad fuzzing 生成/复制/日志分支已删；base/net/DNS/Mojo/URL 等 9 个 GN 文件已删 83 个旧 fuzzer 入口及导入，具体见 out/cut-stage18-breakpad-fuzzer/、out/cut-stage18-engine-fuzzers/。仅 GN 语法解析和 diff 检查通过，未编译。其独占支持库、proto、buildflags，以及 Blink/skia/通用 runner 和 vendor 测试源尚未完成，后续按这些闭包继续，当前不标完成。

后续回填：额外 fuzzable 协议生成模板已删除，metrics_proto 仅生成普通协议；13 个库 BUILD.gn 内的旧 fuzzer_test 入口及导入已移除。精确路径见 out/cut-stage18-fuzzable-proto/manifest.json 与 out/cut-stage18-library-fuzzers/manifest.json。生产 TLS、字体、图片、正则库保留；BoringSSL/RE2 检出中的 vendor 测试源、base/net/Blink/skia/Breakpad 和通用 runner 闭包仍待完成。当前仅语法解析，未编译、未提交。

mojo/public/tools/fuzzers 的两份无实现 GN 骨架及 MojoLPM Python 生成器已删除；mojom 模板、预编译、typemap、protobuf 可见性和 Siso 对象引用一并收尾。保留普通 Mojo 类型生成。精确清单见 out/cut-stage18-mojolpm/manifest.json；仅语法解析和跟踪源码残留核查通过，未构建或运行，当前未提交。testing/libfuzzer、third_party/fuzztest、third_party/libprotobuf-mutator 的通用闭包尚未完成。下方历史根目录快照不能覆盖此接续状态。

**本批本地源码提交：62 删除、65 修改。** 精确路径及提交号记录在 out/cut-stage17/commit.json。仅保存已经应用的父任务改动；Route 92 路径补丁未获具体确认、未应用。部分对应调用方仍待该补丁收尾，本批未生成、编译或运行，不是可构建/验收通过的版本。不 push，不创建 PR。


## 恢复后的源码进度（2026-09-08）

用户已明确继续。当前新阶段以 64f61d2ef182 为基准，out/cut-stage17/progress.json 跟踪接续。父任务已落实网络 connection allowlist / SafeUrlPattern / liburlpattern、无实现的 Network Service GN 骨架、FileReader 与脚本 Object URL 尾巴、JsonCpp 空骨架，并移除不用的内存 HTTP 缓存后端，共删除 62 个普通跟踪文件；Route/CSS/URLPattern 完整提案被自动审批拒绝，等待具体确认，子代理已停止。保留 CSP/混合内容/TLS 等真实加载检查、普通 File/FileList/输入框外观、实际 MemoryCache 和 Shot 缓存 API。这里尚未生成、编译或运行验证；只读 GN path 暴露的一处旧 runtime action 无用赋值已修复。此段优先于下方暂停交接历史。


> 2026-09-08 用户已明确恢复任务（“算了 你继续吧。claude卡死了”）。从 64f61d2ef182 接续剩余裁剪；下面暂停交接记录为历史。本轮先完成 Route/URLPattern 与网络 allowlist 闭包，再集中验证，最多一名已获授权子代理。


日期：2026-09-07。对象：`D:/Github/chromium` 当前源码、根目录、第三方检出目录及本机 Windows 构建依赖图。

本报告保留删除前的审计快照。随后获授权执行的实际删除与验证，见 [执行记录](screenshot-cut-execution-2026-09-07.md)。

## 当前执行状态：本批源码完成，暂停交接

**用户最新要求：本批完成后暂停并交接。此指令优先于下面所有历史 active 目标和继续执行计划；未经用户再次要求，不再裁剪、构建或启动子代理。**

本批七组已明确授权并全部应用：Service Manager、ANGLE/SPIR-V/Vulkan、Blink 拖放、UKM、NQE/FileNetLog、Sanitizer/Skeleton、Origin Trial。加上本轮前半部分 GPU/Viz、旧 IPC、原生 UI、Skottie、第三方骨架、XSLT 脚本 API 与输入预测清理，相对最后验证提交 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb 共删除 1,404 个普通跟踪文件、5 个 gitlink，新增 3 个 Mojo traits 头文件。源码与文档合为一个本地提交，提交号及精确路径见 out/cut-stage16-final/commit.json；不创建 PR、不 push、不发布。

七组源码完成为 7/7；整个裁剪目标尚未完成。已删除 gpu/、ipc/、services/service_manager、ui/gl 及 ANGLE/SPIR-V/Vulkan checkout；五个第三方完整仓库和既有修改保存在 out/cut-stage16-angle-authorized/vendor-backup。仍保留截图所需 CPU 绘制、CSS/DOM/布局、SVG、图片、字体、表单主题、Canvas 备用布局、原生 XSLT、普通 Mojo 和实际网络加载。

静态证据 out/cut-stage16/static-proof-combined.json：15,905 个代码/构建输入、285 个改动 C/C++ 预处理配对、653 份新阶段原始 SHA 备份；扫描发现的一个 drag_state.h 旧 include 已补删，当前记录 0 问题。早期父阶段和 Skottie 备份另有原始证据。

**本批尚未 GN 生成、编译/链接、重建 addon、运行或像素验收，不能宣称可构建或截图无回归。** out/Shot 仍是第十一批旧二进制：183 张已验证像素，第 184 张 Unbounded 只有旧二进制基线；六平台实际编译仍未完成。以后由用户决定是否验证，建议用已成功的 jobs 8（20 已多次实际 OOM），不改永久默认值。

剩余网络/公共协议/FileReader/Blob/Worker、诊断/Perfetto/Crashpad/AX、UI/display/拖放辅助、Route/URLPattern、Skia 实际 checkout 及其他第三方/生成器/同步尾巴，连同接手顺序与回退位置，全部见 [交接报告](screenshot-cut-handoff-2026-09-08.md)。逐根目录状态见 [根目录报告](screenshot-root-status-2026-09-08.md)。子代理已停止。

## 第十一批执行状态（Windows 验证通过）

第十一批已实体删除 576 文件，修改 225 个源码/构建输入及四份任务/同步文档。CC 合成器运行链、Canvas 绘图属性、元素/区域捕获与追踪的 Blink→CC/Viz→Mojo 链、浏览器 ImageReplacement、过渡 CSSOM/私有 descriptor/UA 资源已连同调用方和 GN 移除。普通 CPU 绘制、SVG、图片/表单、Canvas 备用布局、过渡相关普通 CSS 属性与 @supports、真实导航初始化保留。GN 6839/852、7409 个输入及 IDL 检查通过。Windows EXE/DLL 均以 jobs 8 编译链接成功，0 失败 edge；新 addon 已重建，旁置 DLL/资源与 out/Shot 哈希一致。serve、net、84 demos（62 exact / 1 fuzzy / 21 smoke）、Node、daemon、协议、Bilibili 离线长页与 accept 全部通过。183/183 张 PNG 解码像素完全一致，动态 clip-path 与等价静态中点完全一致；Chrome oracle 差异仍为 1.524%。EXE 45,542,912 字节，较第十批减少 790,016 字节；DLL 45,540,864 字节。 Linux probe 为 0 缺 BUILD、0 主仓库缺输入，仍有 3 个 Linux DEPS 检出项和 1 个宿主工具链输入缺失；jumbo 静态扫描列出 39 个候选，部分平台/生成输入未扫描。这是图与静态证据，六平台实际编译仍未完成。其余 Viz/GPU、协议、网络/诊断、第三方和根目录总复核继续处理，整个目标 active。完整接续见 screenshot-cut-task.md；本报告后文保持删除前审计快照。

## 结论

当前仓库没有完成“只留下静态截图所需代码”的裁剪。已经移除浏览器功能入口，但仍存在三层残留：

1. **无用实现仍参与构建**：例如 SQL、GPU 命令客户端、合成器、音频处理、输入路由、硬件密钥服务。
2. **功能实现已删除，接口和构建骨架仍存在**：例如 content、Google APIs、sandbox、printing、extensions。
3. **源码本体不需要，仍因构建读取或依赖同步而保留整包**：例如 ANGLE 的头文件依赖牵住完整检出，SPIR-V 构建配置，以及工具链附带内容。

“仍有 include / GN 依赖”只能解释为什么现在删不掉，不能证明它对截图有用。本报告把这类耦合一并列入裁剪范围，不以“还在引用”为结束理由。

也不能把整个 `ui`、`cc`、`crypto`、`mojo` 目录一起判为无用：它们包含真实的截图基础设施。正确边界是保留所需类型与机制，移除不使用的功能实现、入口和构建依赖。

## 审计口径与证据

### 截图实际需要什么

以当前实现为准：HTML/CSS/DOM、布局与绘制、SVG、文本塑形和字体、图片解码、CPU 栅格化、PNG/JPEG/WebP 编码，以及本地文件和 HTTP(S) 资源加载。保留当前已有的缓存、常驻 worker、C ABI、Node addon、daemon、分片截图和性能统计能力。

关键源码锚点：

- [shot_renderer.cc](/D:/Github/chromium/shot/shot_renderer.cc:1455)：明确禁用脚本和插件；通过空 ChromeClient / FrameClient 驱动文档生命周期。
- [shot_image_stream.cc](/D:/Github/chromium/shot/shot_image_stream.cc:1570)：CPU 像素表面上的绘制，不创建 GPU 渲染设备。
- [shot_network.cc](/D:/Github/chromium/shot/shot_network.cc:140)：直连代理服务；显式选择 Simple 磁盘缓存后端；HTTP/3 实现已移除。
- [shot_platform.cc](/D:/Github/chromium/shot/shot_platform.cc:46)：接口代理只实现 MimeRegistry，其余接口请求关闭连接。
- [modules_initializer.cc](/D:/Github/chromium/third_party/blink/renderer/modules/modules_initializer.cc:13)：媒体、Canvas 上下文、WebGL/WebGPU、worklet 等模块初始化已经移除。

本报告中的“无用”不是“某个简单网页没走到”。例如网络、SVG、CSS 动画状态、表单默认外观、字体回退不能因为单次截图未用就删除。反过来，没有服务端、没有创建入口、功能明确禁用的实现，即使被整个 target 带进编译，也属于审计对象。

### 状态标记

| 标记 | 意义 | 是否可直接删除整个路径 |
|---|---|---|
| **A 无用功能实现** | 当前截图产品不提供该功能；已有实现或配置证据 | 不等于可以直接 rm，仍需切断调用和类型耦合 |
| **B 耦合残留** | 无用功能的协议、公共类型、实现被其他目标带入 | 必须拆类型、接口或 target；不能用耦合为其永久保留辩护 |
| **C 待收窄** | 已找到候选，但可能影响声明式页面、系统适配或现有诊断行为 | 需进一步逐调用点/像素验证，不计入已证明无用的文件总数 |
| **K 截图需要** | 当前渲染、资源加载或对外产品能力使用 | 保留；不代表目录内所有文件都必要 |
| **T 构建/维护** | 不参与截图运行，但用于编译、验证、发布、许可或开发 | 和运行代码区分，不冒充“无用功能”删除 |

### 验证边界

完成了全部根目录的清点、相关 GN/source list/入口实现核查、当前本机 GN 目标导出与依赖闭包分析，并对 `third_party` 的全部一级目录作磁盘清点。详细计数和依赖链见后面的自动生成附录。

这是**根目录全覆盖、功能级别的静态审计**，不是对每个函数作过运行时覆盖证明的删除清单。未改引擎源码、未删除源文件、未重新编译或跑六平台验证，也没有测量可减少的二进制字节数。单个平台不用于宣判其他平台文件无用。

GN JSON 的 `deps` 合并了普通依赖和 `data_deps`；附录中的“可达”表示构建可达，不表示链接或执行。对 Vulkan loader、ANGLE 占位 DLL 等已回查 `BUILD.gn` 并明确标出 `data_deps`。头文件 target、生成器 target 也计入可达数，因此该数不是编译对象数。

## 一、按根目录逐项排查

审计基线：`60a0c6abe6497d2ebff29917046a8112e4ccb78f`。开始时工作区干净；下表计数为创建报告前的主仓库普通文件，**不包含子模块内部文件**。全部 41 个根目录已列出。主仓库普通文件 27130 个，子模块入口 38 个。

GN 导出 9164 个目标，其中从 shot/shot_c 构建可达 2907 个。无可达 target 仍可能作为 import、配置或生成器输入被读取。

| 根目录 | 普通文件 | C/C++/ObjC/Rust含头文件 | GN/GNI | 构建可达目标（含data） | 判断 |
|---|---:|---:|---:|---:|---|
| `.agents/` | 0 | 0 | 0 | 0 | T 技能链接 |
| `.claude/` | 5 | 0 | 0 | 0 | T 开发技能 |
| `.git/` | 0 | 0 | 0 | 0 | T 版本库 |
| `.github/` | 12 | 0 | 0 | 0 | T CI/发布 |
| `apps/` | 132 | 0 | 0 | 0 | T 基准工具 |
| `base/` | 1842 | 1764 | 31 | 78 | K + B/C 基础库与监控残留 |
| `benchmark-results/` | 200 | 0 | 0 | 0 | T 基准数据 |
| `build/` | 748 | 123 | 272 | 91 | T + B 构建及失效加载链 |
| `build_overrides/` | 23 | 0 | 22 | 0 | T/B 第三方配置 |
| `buildtools/` | 76 | 18 | 6 | 3 | T 工具链 |
| `cc/` | 724 | 697 | 9 | 53 | K + A/B 合成器/输入/GPU残留 |
| `chrome/` | 2 | 0 | 0 | 0 | B/T 版本/plist路径 |
| `components/` | 510 | 429 | 73 | 135 | K + A/B/C 浏览器服务与后端残留 |
| `content/` | 30 | 0 | 30 | 0 | B 只有GN/GNI |
| `crypto/` | 91 | 89 | 2 | 5 | K + A/B 硬件密钥 |
| `device/` | 5 | 2 | 3 | 1 | B 同步工具/统计耦合 |
| `docs/` | 13 | 0 | 0 | 0 | T 文档 |
| `extensions/` | 1 | 0 | 1 | 0 | B 构建开关 |
| `google_apis/` | 6 | 0 | 5 | 0 | B GN/配置空壳 |
| `gpu/` | 289 | 248 | 13 | 109 | A/B 命令客户端及类型 |
| `ipc/` | 46 | 42 | 2 | 25 | A/B 旧IPC |
| `media/` | 360 | 353 | 7 | 7 | A/B 音视频基础实现 |
| `mojo/` | 611 | 405 | 42 | 46 | K + B/C 消息/传输 |
| `net/` | 1425 | 1378 | 18 | 47 | K + A/B/C 网络额外能力 |
| `out/` | 0 | 0 | 0 | 0 | T 本地构建产物 |
| `patches/` | 3 | 0 | 0 | 0 | T 依赖补丁 |
| `printing/` | 2 | 0 | 1 | 14 | A/B 打印协议 |
| `sandbox/` | 11 | 0 | 9 | 15 | B 服务协议/构建 |
| `scripts/` | 74 | 0 | 0 | 0 | T 维护工具 |
| `services/` | 597 | 390 | 38 | 524 | K + A/B/C 服务契约 |
| `shot/` | 642 | 31 | 1 | 6 | K/T 产品/夹具 |
| `shotium/` | 21 | 1 | 0 | 0 | K/T API/addon/daemon |
| `skia/` | 92 | 76 | 4 | 58 | K + B/C 绘图封装 |
| `sql/` | 36 | 34 | 2 | 2 | A/B SQL依赖遗漏 |
| `storage/` | 12 | 9 | 3 | 1 | B 浏览器存储类型 |
| `testing/` | 19 | 1 | 13 | 0 | T/B 测试/构建 |
| `tests/` | 8 | 0 | 0 | 0 | T 回归验证 |
| `third_party/` | 15925 | 12483 | 335 | 1232 | K/T + A/B/C；见逐包附录 |
| `tools/` | 305 | 0 | 13 | 2 | T/B 生成器/失效工具 |
| `ui/` | 2155 | 1572 | 87 | 404 | K + A/B/C 绘图与桌面混合 |
| `url/` | 61 | 52 | 4 | 49 | K URL基础库 |


### .agents/

**T**：本地技能链接，属于开发工具。没有截图运行代码，不是 Chromium 功能复活。可以不放进分发包，不能和待裁剪引擎代码混算。

### .claude/

**T**：构建、验证、裁剪、性能、发布技能。与截图运行无关，用于维护工程。无引擎功能裁剪项。

### .git/

**T**：版本库元数据。不属于引擎、构建输入或源码裁剪范围；本次未遍历内部对象数据库。

### .github/

**T**：CI、跨平台构建、发布及相关自动化。只应清理失效的工作流引用，不能按“截图不会执行 YAML”删除发布设施。

### apps/

**T**：性能比较工具与基准站点。截图运行时不用，但属于项目维护能力。需精简源码分发时可单独排除，不是引擎残留依赖。

### base/

**K**：内存、容器、字符串、任务、线程、锁、文件、时间、平台适配是实际基础设施。`shot_core → base` 直接依赖，不能整体删除。

| 候选路径/功能 | 判断 | 留存原因与处理边界 |
|---|---|---|
| `base/profiler/` 中栈采样与持续采样设施 | C | `base/BUILD.gn` 直接列入 `stack_sampling_profiler.*`。截图没有采样产品接口，但仍需检查调度器/诊断初始化；不能连线程和栈基础类型一起删。 |
| `base/sampling_heap_profiler/` | C | 采样剖析不同于 Shot 已有的内存统计；需把采样器和 allocator 统计分开。 |
| `base/trace_event/`、`base/tracing/` 的 tracing 后端 | B/C | `base → perfetto:libperfetto` 仍可达。只关闭 `use_perfetto_trace_processor` 没有删除 tracing。先收窄记录/会话/导出功能，保留必要宏接口；不得顺手删除 `CaptureStats` 或 DCHECK/CHECK。 |
| `base/metrics/` 的直方图收集/登记 | C | Blink/net 到处调用计量宏；截图不需要浏览器遥测上传，但宏周边可能承载真实状态计算。须逐块分离，不能整段按 telemetry 标签删。 |
| `base/power_monitor/` 平台电源事件源 | C | 源列表包含平台实现。电源/挂起恢复事件可能影响网络和线程池，未证明所有行为可移除；仅把浏览器特有的监控扩展列为候选。 |
| `base/enterprise_util*`、`base/win/default_apps_util*`、部分提权/系统集成工具 | C | `base` 大目标保留了企业管理、默认应用等 OS 工具；非页面栅格化本身需要，需检查平台调用者后收窄。 |

证据：[base/BUILD.gn](/D:/Github/chromium/base/BUILD.gn:615)。这里的候选是实现/子目标边界，不是对 1,842 个文件的整体删除结论。

### benchmark-results/

**T**：历史基准数据。运行截图用不到，但属于可追溯性能证据。可以按数据保留策略另行归档，不计入源码裁剪收益。

### build/

**T + B**：编译系统必需；同时是旧功能/目标被继续加载的重要来源。

- **B**：对已经删除的 content、Google APIs、桌面 UI、测试目标的 import/deps/config 链。即使目标不构建，GN 读取其 `BUILD.gn` 也会让裁剪器把文件留下。
- **C/T**：Android、iOS、Fuchsia、ChromeOS 等不发布平台的配置、模板和测试支持应逐引用收窄。仅以目录名字判无用不可靠：通用模板经常 import 某个平台的 `.gni` 来取得默认值。
- **T**：Windows/Linux/macOS 工具链、资源打包、Rust/IDL/Mojom 生成器、版本和平台头必须继续可冷构建。
- **流程缺口**：`trim-tree.ts` 的规则保留整个 `build/` 再排除一部分外平台路径，属于保守构建闭包，不是“截图功能最小集合”。
- **文档错误**：[shot.gn](/D:/Github/chromium/build/args/shot.gn:236) 仍写“无磁盘缓存、只加载 file URL 后退出”，与现有 HTTP(S)、Simple 缓存和常驻模式不一致。不能据此裁剪网络。

### build_overrides/

**T/B**：第三方库的 Chromium 集成开关。ANGLE/Vulkan 等无用后端仍会读取对应 override；删除后端时要同步清理。保留仍在用的 Skia、Rust、BoringSSL 等配置。

### buildtools/

**T**：GN、格式工具和 libc++ 构建适配。不是运行功能；没有因为“不参与像素生成”而整体删除的依据。可清理不再发布的平台工具包，但需覆盖六平台冷构建。

### cc/

**这是主要裁剪对象之一，不能再笼统说合成器已经完全删掉。** `Blink core → //cc:cc` 仍可达，单个大目标把 700 余个文件中的大量实现捆在一起。

| 候选 | 判断 | 为什么截图不用；需要一起处理什么 |
|---|---|---|
| `cc/benchmarks/`，尤其 `unittest_only_benchmark*`、`rasterize_and_record_benchmark*` | A | 截图 API 不执行 Chromium 的合成器微基准。这些文件直接列在正常 `cc` 目标中，不只是测试目录。删除注册和 controller 链。 |
| `cc/trees/layer_tree_host*`、`layer_tree_host_impl*` 等合成器执行端 | A/B | Shot 获取 paint record 后自己 CPU 回放，不运行浏览器的 compositor frame 提交链。需保留 Blink paint conversion 使用的 property tree 等数据结构，拆出实现。 |
| `cc/scheduler/` | A/B | 合成器 begin-frame、提交、激活调度不是截图等待网络/布局的任务调度。两者不可混称 scheduler。 |
| `cc/raster/gpu_raster_buffer_provider*`、`one_copy_raster_buffer_provider*`、`zero_copy_raster_buffer_provider*` | A/B | GPU/共享表面栅格上传路径；当前 CPU SkSurface 不调用这类 provider。需切断 tile/compositor 引用。 |
| `cc/tiles/gpu_image_decode_cache*`、tile 调度/优先级/淘汰器 | A/B | 合成器 tile 管线；Shot 的截图 tiles/strip 是另一套代码，不能因同名保留。CPU 图片解码、mipmap 工具需单独检查。 |
| `cc/input/` 的 fling、overscroll、浏览器工具栏移动 | A/B | 当前没有用户输入/交互式滚动；保留布局滚动偏移、scroll tree 等视觉数据。 |
| `cc/metrics/` 的 frame sequence、scroll jank、compositor frame reporting | A/B | 衡量交互帧率和卡顿，不是截图的加载/渲染耗时。沿着调用者一起删，不能只去掉统计类。 |
| `cc/layers/` 中视频、texture、surface 等合成器 layer | B | 无视频/GPU提交，但 Blink/WebView/paint 类型仍引用。按 layer 种类拆，不能整包删除。 |
| `cc/paint/skottie_*` 与 GPU transfer-cache 序列化 | B | Skottie/Lottie 播放、跨 GPU 进程传输不属于当前截图输入。需保留通用 PaintOp、图片和文字绘制。 |

**K**：`cc/paint` 的 PaintRecord/PaintOp、滤镜、文字、路径、图片；`cc::ImageProvider` 和 CPU 回放；Blink 需要的变换/裁剪/效果树。CSS transform、opacity、filter 不等于必须启动合成器。

证据：[cc/BUILD.gn](/D:/Github/chromium/cc/BUILD.gn:37)、[shot_image_stream.cc](/D:/Github/chromium/shot/shot_image_stream.cc:1205)。

### chrome/

当前仅 `VERSION` 和 `app/app-Info.plist`，**没有浏览器界面实现**。

**B/T**：可以作为路径清理对象，但先迁移读取方。`base/BUILD.gn` 的 Apple 分支仍读取 plist，Apple 构建脚本仍命名 `chrome/VERSION`。迁移后才可能让这个根目录消失。

### components/

| 候选 | 判断 | 当前耦合/裁剪边界 |
|---|---|---|
| `persistent_cache`、`sqlite_vfs` | A/B，明确遗漏 | V8 字节码缓存后端已换占位实现，但 `persistent_cache/BUILD.gn:42` 仍无条件依赖 `sqlite_vfs`，进而带入 SQL/SQLite。三条本机 SQL 路径最终都汇入这条链。需检查 pending-file-set 和 Mojo traits 的独立依赖。 |
| `unexportable_keys/` | A/B，明确遗漏 | 设备绑定会话关闭后，`net_features` 仍无条件带入密钥服务；URLRequestContextBuilder 的服务头 include 和 setter 签名也还在，只有函数内部赋值被开关保护。截图不使用设备绑定会话、TPM 排队任务/密钥持久化。与 crypto 的基础密码算法拆开。 |
| `input/` | A/B | 输入路由、鼠标滚轮队列、touch emulator、fling、render-widget 输入派发仍被 Blink platform 带入。保留布局/样式使用的少量输入枚举，移除运行实现。 |
| `viz/client/`、`viz/common/` 的提交、GPU、命中测试/视频资源部分 | A/B | Viz service 实现已大幅删除，客户端及协议仍牵住 GPU。保留共享像素格式、颜色、paint 所需类型。不能把整个 common 永久视作必要。 |
| `prefs/` | B | `services/network/public/cpp` 将偏好系统带入；Shot 配置走自己的 JSON/C++ options，不是浏览器 profile prefs。需要逐个拆开依赖 prefs 的 network helper。 |
| `content_settings/core/common/` 的浏览器站点设置系统 | B/C | 模式匹配、设置元数据、cookie 设置基类、Mojo traits 被 public headers 拉入。保留当前资源加载实际使用的安全/权限语义，不把所有 policy 一起删除。 |
| `crash/core/common/` 的 Crashpad 适配 | B/C | `cc/paint → crash_key → Crashpad client`。保留崩溃信息/日志可以用更小接口；当前不能声称只有几个无成本枚举。 |
| `performance_manager/scenario_api/` | B/C | 浏览器负载/输入场景共享内存与观察者。不负责 Shot 的网络空闲判断；需检查调度器默认值和观察链后拆除。 |
| `headless/display_util`、`headless/screen_info` | B/C | Linux Ozone headless 带入的屏幕模拟设施。不是“只要 headless 都必需”，需与字体和基础 display 类型分开。 |
| `device_event_log/` | C | 浏览器设备事件日志，不是 Shot 运行日志；需确认剩余系统适配调用。 |
| 其余只有 `BUILD.gn` 的 browser/process/enterprise/webrtc 等骨架 | B | 已无实现的目标声明；停止加载/引用后可清理。见目录附录。 |

**K**：discardable_memory 的实际分配器、资源加载需要的 link header/url 处理、CBOR 等仍有真实用途的工具。`network_time/time_tracker` 只是小型时钟跟踪工具，不能因名字含 network_time 就当作 Google 联网授时服务删除。

证据：[persistent_cache/BUILD.gn](/D:/Github/chromium/components/persistent_cache/BUILD.gn:38)、[sqlite_vfs/BUILD.gn](/D:/Github/chromium/components/sqlite_vfs/BUILD.gn:48)、[net/BUILD.gn](/D:/Github/chromium/net/BUILD.gn:127)。

### content/

**B，构建空壳**：30 个文件全部为 `.gn/.gni`；当前 Windows 引擎构建闭包没有 `//content` 目标。browser、renderer、gpu、utility、devtools 等目录名仍在，但没有对应 C++ 实现。

需要清理这 30 个构建文件及其反向 GN 引用。它们继续被 GN 加载、继续命名已删除的源码，是“目录反复还在”的具体原因之一。不是仍运行完整 content。

### crypto/

**K**：HTTPS、证书、哈希、HTTP 认证等需要密码基础库；`shot → net → crypto` 是合理依赖。

**A/B 候选**：`tpm.rs`、`tpm_parser.*`、`unexportable_key*`、`user_verifying_key*` 及它们的 Windows/Apple 适配、用户验证和硬件密钥统计。当前 `crypto` 大目标无条件依赖 `:tpm`；截图产品未提供硬件认证/生物识别/设备绑定会话能力。

拆掉 `components/unexportable_keys` 不会自动让这些文件离开 `crypto`。需要再拆 crypto 内部 target。`apple/keychain*` 中可能涉及正常 TLS 系统证书/密钥访问的部分应逐调用者区分，不能因为含 keychain 全删。

证据：[crypto/BUILD.gn](/D:/Github/chromium/crypto/BUILD.gn:21)、[crypto/BUILD.gn](/D:/Github/chromium/crypto/BUILD.gn:117)。

### device/

**B**：只剩 `one_writer_seqlock.cc/.h` 和 3 个构建文件。当前链为 `Blink core → cc → device/base/synchronization`，直接使用点是 `cc/metrics/shared_metrics_buffer.h`。

这里没有蓝牙/摄像头/传感器实现。序列锁本身是通用同步工具，但此处调用来自待裁剪的合成器统计：应随统计调用链一起解除，而不是以“底层同步工具”永久保留整个 device 路径。`udev_linux/BUILD.gn` 是另一个平台构建残留。

### docs/

**T**：不参与截图运行。需要纠正与实际实现不符的“完全移除”“已排除 SQL/Vulkan”等历史结论，但历史日志应保留为历史，不当作当前验证结果。

### extensions/

**B**：只有 `extensions/buildflags/buildflags.gni`，不是扩展系统实现。将仍需要的布尔值收敛到本工程配置、消除 import 后可删除目录。

### google_apis/

**B**：6 个文件：根/子目录 `BUILD.gn`、`config.gni`、`build/check_internal.py`。没有 Google 登录、Drive、Calendar 的业务源码。当前 Windows 引擎闭包没有该目录 target，但 `build.ninja.d` 仍记录读取全部 6 个文件。

应清理 GN 加载链和官方密钥配置探测，再删除这些空壳；不是恢复了 Google API 功能。

### gpu/

**A/B，仍有大量真实实现，不能解释成“只剩类型”。**

| 候选 | 为什么不用 | 仍在的原因 |
|---|---|---|
| `command_buffer/client/gles2_implementation*`、`raster_implementation*`、cmd helper、transfer/ring/mapped buffer、query/program/VAO tracker | 当前不创建 GL 上下文，也不向 GPU 服务发送命令 | Blink platform/viz common/cc 的公开依赖捆绑进来 |
| `ipc/client/gpu_channel_host*`、`command_buffer_proxy_impl*`、共享图像 IPC proxy | 没有 GPU 进程或 GPU channel 服务端 | `cc → gpu/ipc/client` 仍存在 |
| `client/internal/mappable_buffer_dxgi*`、IOSurface/native pixmap 路径 | GPU/平台图像共享；不是当前 CPU 图片解码入口 | 共享图像客户端目标带入，需保留可能仍被其他 CPU shared-memory 类型引用的基础接口 |
| `config/` 中驱动控制列表、GPU workaround 决策、设备性能数据 | 软件截图不需选择真实 GPU 驱动 | 仍被 Blink 的 GPU feature/type 定义和共享代码引用；拆数据接口与收集/选择逻辑 |
| `ipc/common/*.mojom`、GLES/WebGPU 命令及枚举 | 当前没有对应功能 | 生成协议/traits 和公共头的兼容残留；随消费者收窄 |

`GpuFeatureInfo`、mailbox、SyncToken、SharedImage 等类型当前仍被多处公共接口引用。它们属于**待解耦**，不是自动证明功能必需。删除 GPU 运行实现时，应决定哪些类型需要小型独立目标、哪些随消费者一起消失。

证据：[Blink platform/BUILD.gn](/D:/Github/chromium/third_party/blink/renderer/platform/BUILD.gn:205)、[viz/common/BUILD.gn](/D:/Github/chromium/components/viz/common/BUILD.gn:276)。

### ipc/

**A/B**：老式 Chromium IPC 的 channel/proxy、message pipe reader、跨进程句柄附件、bootstrap、ParamTraits 与 Mojo 适配。当前路径 `shot → services/network/public/cpp → ipc`。

Shot 的 worker stdio 协议、Node daemon socket 并不使用这套 Chromium IPC；它们是项目自己的协议。需将实际使用的 ResourceRequest/网络数据结构从 IPC traits 与 channel 实现拆开。不能只删 `ipc/`，也不能拿“产品有进程间通信”作为保留老 IPC 的理由。

### media/

**A/B，功能已删但基础目标仍过大。** 当前 360 个文件中 358 个在 `media/base`，`Blink platform → media → media/base` 仍存在。

- **A**：`audio_bus`、audio converter/fifo/shifter、重采样/声道处理、音频缓冲调度等播放处理。
- **A**：CDM promise/context/factory、DRM/session/decrypt 等加密媒体设施。
- **A/B**：视频帧/GPU buffer、媒体 pipeline/renderer、媒体时钟、解码器配置与 capability 查询的运行实现。
- **B**：枚举、颜色空间、视频元素占位/尺寸等公共类型需要拆出最小集合。不能删除 `<video>` 元素的布局、poster 图片加载，或让图片解码丢失颜色信息。

禁用 FFmpeg/VPX/dav1d 等后端不等于删除 `media/base`。外部 codec 包已经没了，音视频基础代码仍因 roll-up target 编译。

证据：[media/BUILD.gn](/D:/Github/chromium/media/BUILD.gn:121)、[media/base/BUILD.gn](/D:/Github/chromium/media/base/BUILD.gn:15)。

### mojo/

**K + B**：不能整体删除。Shot 明确初始化 Mojo，并实现同进程跨线程的同步 MimeRegistry；删除后外部 file CSS 的 MIME 判断会出问题。

**B/C 候选**：多进程 invitation/channel/broker、跨进程平台句柄运输、旧 Mojo 与 ipcz 的冗余后端、未使用 bindings 生成模式，以及为 GPU/media/browser 服务生成的大量协议。

先从上层 `.mojom` 依赖拆起，再收窄 transport；不能按“单进程”直接移除所有消息管道。保留当前同步调用、任务线程、消息序列化、句柄生命周期需要的部分。

### net/

**K**：URLRequest、HTTP(S)、HTTP/2、DNS、TLS、证书、重定向、压缩响应、当前 Cookie 语义和 Simple 缓存都是网页截图的真实输入路径。

| 候选 | 判断 | 依据和边界 |
|---|---|---|
| `net_features → components/unexportable_keys` | A/B | 设备绑定会话关闭后仍残留的无条件依赖；与基础 crypto 分开修。 |
| `proxy_resolution` 的 PAC 下载/解释调度、DHCP/WPAD、系统代理监控、平台 system proxy resolver | A/B | Shot 显式 `ConfiguredProxyResolutionService::CreateDirect()`，不读取系统代理配置。保留直连服务、接口和 URLRequestContext 初始化所需部分。 |
| `disk_cache/blockfile/` | A/B | Shot 显式选 `DISK_SIMPLE`，不是默认磁盘后端；Blockfile 实现仍在源码集合中。需去掉工厂其他分支，保留 cache_util/simple 共用代码。 |
| `disk_cache/memory/` | C | 当前配置不开 MEMORY 后端，但内存/失败路径是否仍能选入必须查工厂；与缓存元数据驻内存不是同一概念。 |
| `net/extras` 的持久 Cookie/共享字典/报告存储 | B | 当前 root 中已基本删掉相应源码，BUILD 仍描述目标；不应因声明残留误称产品运行该功能。 |
| Network Quality Estimator、NetLog 导出、浏览器级网络指标 | C | 需要确认 URLRequestContextBuilder 默认初始化和诊断用途；不能将所有统计计算整段切掉。 |
| First-Party Sets / 存储分区/隐私分区扩展 | C | 无浏览器 profile，但 Cookie、隔离键、网络缓存语义可能仍使用公共值，未证明可整体删。 |

**特别保留**：`net/third_party/quiche` 不能因 QUIC 已删就整个删除；剩余 SPDY/HTTP2/HPACK/common 仍服务 HTTP/2。报告不建议关闭网络安全检查来换体积。

### out/

**T，本地构建产物**：不是功能源码。`Shot` 是当前构建目录，其他 probe/shard/旧实验/graph/perf 目录不表示相关功能还在运行。

历史 dep log 与旧 jumbo 文件曾经误导裁剪；要从当前可达对象过滤。清理旧输出属于磁盘维护，应先确认没有正在使用的构建与基准，本次不执行。

### patches/

**T/K**：为 DEPS 检出的 ICU/Skia 等依赖提供修改；虽然补丁文件不运行，其效果会进入截图。不能作为无用文件删。后端裁剪必须同步 DEPS 与补丁适用范围。

### printing/

**A/B**：仅 `mojom/BUILD.gn` 和 `print.mojom`，定义打印机颜色模式、双面、质量等协议。Shot 只输出图像，没有打印作业入口。

当前 `Blink public:blink_headers → printing/mojom` 仍生成绑定；应从 public headers/print 参数声明中切掉打印协议。CSS `@media print` / 分页布局属于另一类样式能力，不能拿删除打印机协议当作关闭 CSS 支持的授权。

### sandbox/

**B**：9 个 GN/GNI 文件加 `policy/mojom/context.mojom`、`sandbox.mojom`。没有 OS 沙箱实现。

协议仍被网络 Cookie 等接口以及 proxy resolver 的 ServiceSandbox 元数据引用。删功能服务协议、简化剩余接口，再移除枚举与构建 import。不能因为存在 `sandbox.mojom` 就宣称程序真的启用了沙箱。

### scripts/

**T**：构建、验证、裁剪、发布及分析工具。运行截图不需要，但当前维护工作需要。

本轮发现应修正的裁剪机制：不能只按 GN 读取集合保留文件；需要先做功能目标拆分，再收集构建闭包。`prune-deps.ts` 保留的 hook 还包含 `gpu_lists_version`，当前会产生 `gpu/config/gpu_lists_version.h`，这是一个真实的重新生成来源，但只解释该头文件，不能解释整个 GPU 树。

### services/

| 子目录 | 判断 | 无用部分与必须区分的保留部分 |
|---|---|---|
| `network/public/cpp` | B/C | Shot 直接用 ResourceRequest 等结构，但大目标还带入 IPC、prefs、cookie encryption provider、众多浏览器 policy/helper。拆出资源请求/响应最小目标；保留真实加载路径的 URL、安全策略与头解析。 |
| `network/public/mojom` | B | NetworkContext 全服务、浏览器管理、设备绑定、代理等不实际连接的接口仍统一生成。Shot 不通过 NetworkService 发 HTTP；删未服务的接口组，保留真实数据和 MimeRegistry 间接需要的基础 bindings。 |
| `viz/public/mojom` | B | 合成器帧、GPU/共享图像、surface、layer tree 协议；没有真实 Viz 服务。仍拉入 `ui/gl` 等目标。 |
| `metrics/public/cpp` 与 `public/mojom` | A/B | UKM builders、metrics 管道和 metrics_proto 用于浏览器指标，而不是 Shot CaptureStats。Blink core 仍依赖 ukm_builders，需连调用一起删。 |
| `service_manager/public` | A/B | Service/Connector/Identity/接口 filter 等浏览器服务管理契约；服务管理主实现已不在可达闭包中，但 public/cpp 与协议还在。 |
| `proxy_resolver`、`proxy_resolver_win` | A/B | 直连模式不用独立代理解析服务；其 Mojom/公共声明继续被其他协议引用。 |
| `cert_verifier` | B/C | 独立证书验证服务协议可收窄；net 内部证书验证必须保留，不能一起关掉。 |
| `services/test` | B/T | 残余测试协议/构建文件，不是截图功能。脱离 GN 加载链后清理。 |

证据：[shot_url_loader.cc](/D:/Github/chromium/shot/shot_url_loader.cc:21)、[services/network/public/cpp/BUILD.gn](/D:/Github/chromium/services/network/public/cpp/BUILD.gn:68)、[shot_platform.cc](/D:/Github/chromium/shot/shot_platform.cc:46)。

### shot/

**K**：实际截图入口、渲染、图像流、网络、worker 协议、缓存、C ABI，不能把其中“不是 CSS 算法”的代码都当成冗余。

- **C**：`DispatchDidFinishLoad` / `DidStopLoading` 等当前同步文档安装路径不触发的 fallback 回调。源码已有说明，可评估收窄，但不是本轮主要残留。
- **C**：实验环境变量分支需逐项确认仍是否提供被使用的性能/诊断能力，不能凭名称统一删。
- **T**：testdata、图片、Bilibili 离线夹具和 reftest 是验证资产，不参与引擎运行；占多数文件数不表示引擎冗余。

### shotium/

**K/T**：对外 TypeScript API、Node addon、daemon/client、类型与打包配置。不是 HTML/CSS 实现，但属于产品承诺。`dist`、`native/build` 等本地产物不应混计为 Chromium 功能残留。

### skia/

这是 Chromium 对第三方 Skia 的封装目录，真正 Skia 源在 `third_party/skia`。

**K**：CPU bitmap、颜色转换、字体与栅格属性、图片编解码集成。

**B/C 候选**：`ext/benchmarking_canvas.*` 的绘图基准封装、`ext/event_tracer_impl.*` 的 tracing 接入、`public/mojom` 的跨进程图像序列化，以及 `ext/rgba_to_yuva.*` 等需要按消费者检查的图像格式适配。随遥测、Viz、媒体消费者拆分；保留软件渲染使用的 SkImage/SkBitmap/ICC 数据结构。`skia_memory_dump_provider` 等内存诊断需和 Shot 现有统计区分，不能一并删除。不能把 Skia 的渐变/滤镜运行机制或 CPU 所需 SkSL 误认为全是 GPU shader 后端。

### sql/

**A/B，最高优先级明确遗漏**：当前截图不选择 SQL 缓存，不提供 WebSQL 或 V8 字节码缓存；实际保留原因是 persistent_cache 的无条件 VFS 依赖。

```text
shot_core
  → Blink platform/loader
  → components/persistent_cache
  → components/sqlite_vfs
  → sql
  → third_party/sqlite
```

`persistent_cache/BUILD.gn` 同时写着“占位实现排除 SQL”并保留反向拉入 SQL 的依赖。8 月 21 日提交 `589d8246efcd` 已存在此矛盾，后续裁剪继续按构建闭包把 SQL 留下。

需要一起处理：VFS 依赖、pending_file_set、持久缓存与 VFS 的 Mojo traits、`sql/BUILD.gn`、`third_party/sqlite` 及 DEPS/gitlink。只把一行 `//sql` 移进条件分支不足以证明完整移除。

### storage/

**B**：保留的是数据库 origin 标识、浏览器 FileSystem 元数据/URL 工具、quota padding key，以及 BUILD 骨架。当前 `Blink core → storage/common`。

截图文件读取通过 ShotURLLoader/net，不是浏览器 FileSystem/配额服务。需要把 drag/file/blob/public 接口对这些类型的引用拆开；`file://` 读取能力与此目录不是同义词。若某 origin 辅助函数仍被资源加载实际使用，应保留或提取工具，不保留整个存储功能。

### testing/

**T/B**：测试框架 GN 模板、libfuzzer、Rust 测试、脚本公共工具。当前 Windows 引擎目标闭包没有 testing 目标，但 GN 仍 import 测试模板。

可以清掉已失效测试声明及其无用模板；保留真实构建脚本间接导入的 Python 工具。`testing/scripts/common.py` 曾因被构建脚本动态导入而不能直接删；“不是测试运行所以全删 testing”会破坏冷构建。

### tests/

**T**：渲染回归测试与说明。运行截图不需要，但裁剪后证明像素不变需要。不能为了让目录少而删掉验证能力。

### third_party/

该目录既有真正需要的引擎依赖，也有配置残留、整包检出、工具链和本地空目录。不能用主仓库文件数代表实际检出大小：37 个子模块入口在主仓库索引只各占一项。

#### 无用或过度耦合的依赖

| 依赖 | 判断 | 证据/需要处理的边界 |
|---|---|---|
| `sqlite` | A/B | persistent_cache/VFS 依赖遗漏；与根目录 sql 同一项，不重复计算收益。 |
| `angle` 的编译器、后端、测试与完整检出 | A/B | 当前图仍需 `angle:includes`。这不等于需要 libANGLE/translator 整包；DEPS 仍检出全部内容。收敛头文件和 GN 配置后清理源包。 |
| `vulkan-loader` | A | 当前 Windows 图真实到达 `libvulkan`，且包含 loader C 源。来源是 `ui/gl` 的 **data_deps**，不是截图链接/执行 Vulkan。此前“删完 Vulkan loader”的结论遗漏了该路径。 |
| `vulkan-headers` | B | GL/GPU 公共类型与 loader 引用。移除 loader 后仍需清类型依赖，不能保证随之自动消失。 |
| `spirv-tools`、`spirv-headers` | B | 本机引擎闭包没有 SPIR-V 目标，但 GN 仍加载它们的配置，DEPS 与 gitlinks 仍在。需六平台确认和移除加载链，不宣称已经重新启用 shader 编译。 |
| `crashpad` | B/C | `cc/paint → crash_key_lib → crashpad/client`，再带 util/compat。没有独立上传/崩溃服务不等于这包已消失。可评估使用较小 crash key 实现；要保留现有诊断需求。 |
| `perfetto` | B/C | `base → libperfetto` 仍在；关闭 trace_processor 不会关闭整体 tracing。移除记录后端需遍历 tracing 宏/初始化与生成器，不破坏 Shot 指标。 |
| `metrics_proto` | A/B | UKM/浏览器指标协议链。与 services/metrics、tools/metrics 的生成器一起收窄。 |
| `pffft` | A/B | Blink platform 直接依赖 FFT 库，当前音频模块已删除。需核查所有 FFT 使用者后拆目标；不能因为 target 仍在就把音频处理算作截图功能。 |
| `one_euro_filter` | A/B | Blink platform/UI prediction 的输入轨迹滤波依赖；当前无用户输入。随预测器拆除。 |
| `libyuv` | B/C | media、图像格式转换/共享图像相关依赖。视频 YUV 路径可裁，普通位图转换是否使用必须分别查，不能整个库直接判无用。 |
| `liburlpattern`、`components/url_pattern` | B/C | URLPattern/路由等脚本和导航能力残留。与真正 URL 解析、CSS URL 和网络地址处理分开。 |
| `re2` | B/C | 当前 `Blink core → re2` 可达；使用点包含媒体片段 URI、第三方脚本探测、URLPattern、headless 屏幕参数解析。应在移除这些消费者后继续核查剩余匹配用途，不直接整体删除正则库。 |
| `libxslt` | C | XSLT 可由 XML 声明触发，不需要 JavaScript；当前范围以 HTML/CSS 为主，但必须确认 XML 输入语义，不能只因“无 JS”就判死代码。 |
| `ipcz` | B/C | Mojo transport 后端；当前仍有同进程管道。要验证可替代/收窄的 transport，不能直接拔掉。 |
| `libdrm`、`libsync`、`khronos`、`microsoft_dxheaders` | B/C | GPU/平台图像类型和头文件链。需与 Ozone/SharedImage/GL 一起处理，保留平台基础类型实际需要部分。 |
| `inspector_protocol` | B/T | 现仅许可、说明、GN 模板，没有 DevTools 完整实现；清理模板 import 后才能去掉。 |
| `anonymous_tokens`、`leveldatabase`、`snappy`、`libpfm4`、`jsoncpp`、`win_virtual_display` 等 | B/T | 当前主仓库多为 BUILD/README/LICENSE 骨架。逐项状态见附录；不是对应功能已经全恢复。 |
| `googletest`、`fuzztest`、`google_benchmark`、`libprotobuf-mutator`、`ocmock`、测试字体等 | T/B | 未使用的 Chromium 测试 target/模板可裁。需要区分 Shot 验证资产、构建生成器测试约束与失效上游测试。 |
| `catapult`、`node`、`typescript`、`closure_compiler` 等开发/生成工具 | T/C | 不是运行代码。检查实际 hook/资源生成调用后精简；不能因为非 C++ 就统一保留或删除。 |

**K**：Skia CPU 绘图；cppgc/Oilpan；ICU；HarfBuzz/Fontations、FreeType、fontconfig、OTS、WOFF2；PNG/JPEG/WebP 等已支持图片格式；BoringSSL；HTTP 内容压缩；通用容器/字符串依赖；libxml 的 SVG/XML 解析。Rust 依赖需按实际 `features` 和调用者拆，不能以“V8 已删除”误删 cppgc，也不能误删字体的 Rust 路径。

**T**：clang、rust-toolchain、GN、ninja、Python、libc++/libunwind、sysroot、nasm 等。protobuf 有宿主生成器与运行库两种身份；不能把生成器不在最终二进制里等价为源码无用。

#### third_party/blink：必须深入的内部耦合

`modules/` 当前只剩 5 个初始化/导出骨架文件，但 `core` 和 `platform` 仍携带与已删模块相关的行为与接口。

| 子系统/路径 | 判断 | 应裁内容与保留边界 |
|---|---|---|
| `core/script`、`platform/loader` 的 code cache、cached metadata 协议 | A/B | 脚本执行、排队、编译缓存、CodeCacheHost 不存在；保留 HTML parser 对 script 标签/阻塞语义的必要处理和共用 fetch settings。 |
| `core/clipboard`、`core/input`、editing 的命令/撤销/交互子系统 | A/B | 不提供剪贴板、拖放、输入、编辑 API。文字选择范围、可见位置计算、文本控件默认值绘制可能仍用于布局，不能删整个 editing。 |
| `core/fullscreen`、`page` 的 popup、鼠标锁定、交互焦点/拖动 | A/B | EmptyChromeClient 不开窗口或交互服务；保留影响默认样式、焦点伪类或表单外观的最小状态。 |
| `core/fileapi`、`platform/blob` 的 FileReader、文件选择/Blob 服务通道 | A/B | 无脚本 FileReader/Blob 创建及浏览器服务；本地 URL 文件读取通过另一条路径，不能和它一起删除。保留表单文件控件的静态外观。 |
| `platform/graphics` 的 accelerated bitmap、GPU/shared-image、Canvas resource provider、WebGL drawing buffer | A/B | 没有 GPU context，Canvas 工厂已不注册。保留 BitmapImage、CPU image、色彩管理和共享的 paint record 工具。 |
| `platform/graphics` 的 worklet mutator/dispatcher、begin_frame provider | A/B | 无 animation/paint worklet、无 compositor 帧源；不是 CSS 动画取值和生命周期更新本身。 |
| `platform/widget`、`core/exported` 的 WebView/WebWidget、输入路由/合成器桥接 | B | Shot 直接创建 Page/Frame，不走完整浏览器 widget；拆平台导出和必要初始化，不要删 FrameView 的布局职责。 |
| `platform/scheduler/worker`、compositor thread、worker inspector | A/B | 无 Web Worker/Worklet 执行入口；保留主线程 scheduler 和网络/字体使用的通用线程池。 |
| `core/timing` 的 PerformanceObserver、用户计时、浏览器 UKM/LCP 上报 | B/C | 无脚本观测接口/浏览器指标服务；但加载完成判断、first-contentful-paint、layout 生命周期与 Shot 等待有关，不能整包删。 |
| `core/inspector` 的 DevTools emulator、样式溯源映射 | A/B | 无 DevTools 会话。ConsoleMessage 仍由 Shot 输出资源加载错误，应保留；目录名含 inspector 不构成全删理由。 |
| `core/probe`、`platform/instrumentation` | B/C | 已删 agent 的探针、异步任务跟踪可拆；计时/状态相关调用要保留语义。 |
| `core/accessibility` 和 `ui/accessibility` 的 AX 意图、协议枚举 | B | AX cache 模块已经不注册；保留 HTML 属性解析与影响 CSS 的状态，裁无消费者的辅助访问事件/协议链。 |
| `core/resize_observer`、`intersection_observer` | B/C | 脚本观察回调不用；IntersectionObserver 可能参与内部 lazy loading，ResizeObserver 周边可能参与生命周期，必须区分内部用途。 |
| `core/view_transition`、`origin_trials`、`route_matching`、`url_pattern` | B/C | 无脚本/浏览器导航服务的分支可裁；声明式样式或 feature 默认值可能影响截图，不按目录整删。 |
| `core/xml` 的 XSLT、`sanitizer`、Trusted Types | C | 各有 HTML/XML 解析或字符串工具共用点。不能凭“脚本能力”标签判断整个目录无用。 |
| `public/mojom` 的 media/GPU/worker/storage/printing/service 等接口组 | B | 正常截图只连接极少接口，整体 public deps 却生成多套 C++/Blink bindings。按服务能力拆 import/deps/traits；不是仅删生成输出。 |
| `bindings` 中名为 V8 的枚举/union/dictionary、ActiveScriptWrappable | K/B | 当前有非 V8 的已生成类型、cppgc 生命周期机制。删除真正脚本入口，保留渲染调用所需类型；不按名字删。 |

**明确保留**：CSS/style/layout/paint/DOM/HTML parser、SVG、MathML、字体/text、图片解码、heap/cppgc、WTF、必要 loader、滤镜/渐变/裁剪/变换。CSS animation、scroll、display locking、懒加载和表单布局不能整体宣布“静态截图不需要”。

### tools/

**T/B**：grit、clang/rust 工具、protoc wrapper、资源/IDL/字体数据生成仍供构建使用。

候选包括：随着 UKM/UMA 裁剪可取消的 `tools/metrics` 部分生成器与 XML 输入、失效 ipc_fuzzer/上游测试模板、已经没有输出消费者的 TypeScript/Inspector 工具配置。不能删 `grit` 或动态 Python import 需要的文件来换取目录消失。

### ui/

`ui` 同时容纳渲染基础设施与桌面界面设施，CSS 引擎在 Blink。按功能分开如下。

| 子目录/功能 | 判断 | 具体范围 |
|---|---|---|
| `gfx/geometry`、字体、颜色、codec、Skia 转换 | K | Shot 和 Blink 直接使用。不能为了消除 ui 根目录复制/改名后宣称功能已裁剪。 |
| `base/resource`、必要字符串与资源包 | K | UA stylesheet、字体/控件资源；资源包缺失会改变截图。 |
| `native_theme`、`color` 中网页控件与颜色选择 | K/C | 网页 checkbox/radio/button/default appearance 需要；桌面 UI 专用 palette/mixer 可以另行收窄。 |
| `gl/` 中 context/surface、DirectComposition、overlay、EGL/GL 绑定与窗口集成 | A/B | CPU 截图无真实 GL/显示提交；cc/media/viz 的依赖继续拉入。保留尚未拆除的格式/枚举时使用独立目标。 |
| `gl:dummy_libEGL`、`dummy_libGLESv2` | A | 为上游基础设施生成的占位 DLL；由 data_deps 带入，不是截图功能。还要查打包脚本是否误带入。 |
| `gl → vulkan-loader:libvulkan` | A | data_deps 仍会构建 loader，普通“非 data 路径”查询会漏掉；源码用 `angle_shared_libvulkan` 条件控制。 |
| `events` 的设备监听、手势识别、keyboard hook、velocity tracker | A/B | 无物理输入；保留 DOM/UI 使用的枚举和必要事件值，不保留所有 OS 设备实现。 |
| `base/ime`、clipboard、dragdrop、data_exchange、interaction、cursor | A/B | 输入法、系统剪贴板、拖放与桌面交互；控件文本布局与静态鼠标样式是不同层。 |
| `base/prediction`、`latency` | A/B | 输入预测与交互延迟统计；不负责截图耗时。与 OneEuroFilter、components/input、cc/metrics 一起处理。 |
| `ozone`、`platform_window`、`linux` 的窗口/surface/headless screen 管理 | B/C | Linux 当前配置确实选择 Ozone headless，但 Shot 不因此需要完整虚拟窗口管理。拆系统平台/字体/屏幕接口，保留兼容性必需部分。 |
| `display` 的物理屏幕枚举、GPU 信息工具与桌面监控 | B/C | Shot 自行构造 ScreenInfo/scale；相关几何/scale 类型必须保留。系统字体/颜色设置可能仍需平台数据，不整体删除。 |
| `lottie`、`resources` 的 Lottie 资源 | A/B | Chromium 桌面动画播放器，不是 CSS animation；当前资源目标仍统一生成相关 pak。 |
| `resources`、`strings` 中未使用的语言/桌面图标/AX annotation 资源 | B/C | Shot 使用 en-US 包，但 grit 读过其他 `.xtb`/资源即被保留。先收窄输出与 `.grd`，不能直接删翻译文件导致生成器失败。 |
| `aura`、`compositor`、`webui`、`wm` 及其他只剩 BUILD 的目录 | B | 无桌面实现，仅遗留 GN 骨架/配置/说明。移除反向加载链后清理。 |
| `accessibility` 中 AX 协议/事件意图 | B | 无 AX cache 服务；与 Blink public/types 解耦，不影响正常 DOM/CSS 属性。 |
| `gfx/animation`、`gfx/image`、`gfx/native_*` 的桌面辅助扩展 | C | 部分有普通图像/颜色/动画值用途，应在清理 UI 调用者后继续拆，不把整个 gfx 当作必需包。 |

证据：[ui/gl/BUILD.gn](/D:/Github/chromium/ui/gl/BUILD.gn:69)、[ui/gl/BUILD.gn](/D:/Github/chromium/ui/gl/BUILD.gn:356)、[ui/resources/BUILD.gn](/D:/Github/chromium/ui/resources/BUILD.gn:8)、[theme_painter_default.cc](/D:/Github/chromium/third_party/blink/renderer/core/paint/theme_painter_default.cc:330)。

### url/

**K**：URL 解析、规范化、origin、scheme 处理，是资源加载与 CSS URL 的基础。不等于导航 UI 或脚本 URL API。

**B/T 候选**：供跨进程调用而生成的冗余 Mojo 绑定/traits、失效测试模板。应随上层协议收窄，不能删 URL 规范化或安全语义。

## 二、跨目录一起处理的裁剪单元

不能按根目录顺序机械 rm；报告按根目录展示，实际修改要按依赖闭包实施。

### 根目录控制文件也要一起处理

- `.gn` 仍直接 import `third_party/angle/dotfile_settings.gni`，并设置 ANGLE/Vulkan、Crashpad、PDF/Fuchsia 等默认参数。ANGLE 不只是被某个 C++ include 牵住，GN 启动本身就要求它存在。
- 根 `BUILD.gn` 已只把 `//shot` 放进 `gn_all`，所以不是根 target 仍主动构建整个浏览器；残留主要在子目录 BUILD 中被读取的其他目标、依赖和模板。
- `DEPS` 和 `.gitmodules` 仍保留 ANGLE、SPIR-V、Vulkan、SQLite 等入口。确定包的消费者移除后，三者必须与磁盘目录一起处理，不能只改 `.gitmodules`。
- `DEPS` 中 `gpu_lists_version` hook 与 `scripts/prune-deps.ts` 的保留白名单需要同步更新；否则删了生成头还会回来。
- `.gitignore` 隐藏的缓存/旧检出、`root_extra_deps` 和父目录 `.gclient` 是本地状态排查点；本次未改父目录配置，不能从一个本机快照断言历史每次恢复都由同一过程导致。

| 顺序 | 裁剪单元 | 必须联动的目录 | 主要目标 |
|---|---|---|---|
| 1 | SQL/字节码缓存遗漏 | Blink loader/public mojom、components/persistent_cache、sqlite_vfs、sql、third_party/sqlite、DEPS | 断开确实没用的后端链，不能止于关闭旧开关 |
| 2 | 设备绑定/硬件密钥遗漏 | net_features、components/unexportable_keys、crypto/TPM/用户验证 | 去掉无条件带入和大目标内未拆部分 |
| 3 | GL 附带构建产物 | ui/gl、Vulkan loader、ANGLE stubs、打包脚本 | 先消除无用 data_deps，验证不会生成/分发无关 DLL |
| 4 | 旧构建骨架 | content、google_apis、chrome 路径、extensions、sandbox/printing 的配置部分 | 清掉加载者再删文件，使根目录真正消失或只剩明确接口 |
| 5 | 合成器/GPU/输入 | Blink graphics/widget、cc、gpu、viz、ui/gl/events/ime、device、OneEuroFilter | 分离 CPU paint 与 compositor/交互/GPU 执行体系 |
| 6 | 音视频/Canvas/worklet 残留 | Blink platform/core、media/base、GPU image、pffft、libyuv | 保留静态元素外观与图片数据，去掉功能实现 |
| 7 | 浏览器 IPC/服务契约 | services、ipc、mojo、Blink public/mojom、storage、sandbox、printing | 从业务协议减起，再减少基础传输依赖 |
| 8 | tracing/metrics/crashpad | base、cc metrics、services/metrics、tools/metrics、perfetto、metrics_proto、crashpad | 区分 Shot 诊断与浏览器遥测；保留必要错误信息 |
| 9 | 构建与第三方检出体积 | build、testing、tools、DEPS、.gitmodules、第三方模板 | 移除无消费者工具/配置、最小化头文件包，而非反复只删工作区文件 |

这不是可减少体积的排名；本次没有二进制 section/符号测量。源文件量、编译步骤、链接输入、最终保留字节和冷启动内存是不同指标。

## 三、为什么此前清理了仍存在

1. **禁用功能只断一部分边**：SQL、硬件密钥是现存实例。
2. **大目标把实现和基础类型打包**：cc、media、ui、crypto、services/network/public/cpp 是主要实例。
3. **生成协议继续牵住整个组件**：无服务端的 GPU/media/storage 等 Mojom 仍会拉 traits 和对应库。
4. **GN 读取不是引擎使用**：`build.ninja.d` 收录被加载的 BUILD/import/exec_script；当前裁剪器将它们全部视作保留依据。
5. **data_deps 被忽略**：Vulkan loader/ANGLE 占位库不需要链接进 Shot，却仍被要求构建。
6. **DEPS 是检出单位，不是头文件单位**：只用 ANGLE 的一小部分头也会拉整个源包；删除本地包但不修改 DEPS 会恢复。
7. **生成 hook 单独存在**：GPU lists version hook 确实会重建特定头文件，但不能用它解释整个 GPU 代码树。
8. **历史构建记录污染过裁剪集合**：9 月 6 日提交 `358e52d892d1` 修正旧 deps/jumbo 的误保留；这是历史问题，不代表今天所有残留都由旧缓存造成。
9. **空目录留在磁盘**：本机 third_party 还有大量无业务文件的目录，详见磁盘附录。它们没有参与主仓库源码裁剪，但会继续在资源管理器里显示。

## 四、实施时的完成标准

每一项应同时记录：功能入口、当前依赖链、需要保留的公共类型、修改过的消费者、被删除的实现/目标/协议、DEPS/hook 同步处理，以及验证结果。不能只报告“grep 不到了”或“这次能编译”。

修改顺序是拆目标/接口 → 冷生成并检查缺输入 → 实际编译 → 六平台必要验证 → 页面像素/网络/worker/addon/daemon 检查 → 再裁文件和第三方检出。所有涉及视觉语义的修改应保留 CSS、SVG、字体和控件回归证据。

对于 **C 待收窄** 项，应先完成内部调用者确认；如果影响当前声明式截图能力，就从无用清单撤回该部分，不能为了完成清单关闭能力。

## 附录 A：关键构建依赖链

这些是本次 GN 导出里计算出的具体可达路径。`→` 只表示构建依赖；不推导函数执行，也不推导最终链接大小。

- **`//sql:sql`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/platform:platform` → `//third_party/blink/renderer/platform/loader:loader` → `//components/persistent_cache:persistent_cache` → `//components/sqlite_vfs:sqlite_vfs` → `//sql:sql`。
- **`//components/unexportable_keys:unexportable_keys`**：`//shot:shot` → `//shot:shot_core` → `//net:net` → `//net:net_features` → `//components/unexportable_keys:unexportable_keys`。
- **`//crypto:tpm`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/platform:platform` → `//crypto:crypto` → `//crypto:tpm`。
- **`//gpu/ipc/client:client`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/core:core` → `//cc:cc` → `//gpu/ipc/client:client`。
- **`//gpu/command_buffer/client:gles2_implementation`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/platform:platform` → `//components/viz/common:common` → `//gpu/command_buffer/client:gles2_implementation`。
- **`//cc:cc`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/core:core` → `//cc:cc`。
- **`//media/base:base`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/platform:platform` → `//media:media` → `//media/base:base`。
- **`//components/input:input`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/platform:platform` → `//components/input:input`。
- **`//components/prefs:prefs`**：`//shot:shot` → `//shot:shot_core` → `//services/network/public/cpp:cpp` → `//components/prefs:prefs`。
- **`//ipc:ipc`**：`//shot:shot` → `//shot:shot_core` → `//services/network/public/cpp:cpp` → `//ipc:ipc`。
- **`//storage/common:common`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/core:core` → `//storage/common:common`。
- **`//services/metrics/public/cpp:metrics_cpp`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/core:core` → `//services/metrics/public/cpp:ukm_builders` → `//services/metrics/public/cpp:metrics_cpp`。
- **`//services/service_manager/public/cpp:cpp`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/core:core` → `//services/service_manager/public/cpp:cpp`。
- **`//third_party/crashpad/crashpad/client:client`**：`//shot:shot` → `//shot:shot_core` → `//cc/paint:paint` → `//components/crash/core/common:crash_key` → `//components/crash/core/common:crash_key_lib` → `//third_party/crashpad/crashpad/client:client`。
- **`//third_party/perfetto:libperfetto`**：`//shot:shot` → `//shot:shot_core` → `//base:base` → `//third_party/perfetto:libperfetto`。
- **`//third_party/pffft:pffft`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/platform:platform` → `//third_party/pffft:pffft`。
- **`//third_party/one_euro_filter:one_euro_filter`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/platform:platform` → `//third_party/one_euro_filter:one_euro_filter`。
- **`//third_party/angle:includes`**：`//shot:shot` → `//shot:shot_core` → `//ui/display:display` → `//components/viz/common/resources:shared_image_format` → `//third_party/angle:includes`。
- **`//third_party/vulkan-loader/src:libvulkan`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/core:core` → `//cc:cc` → `//ui/gl:gl` → `//third_party/vulkan-loader/src:libvulkan`。其中 ui/gl 引入该产物的边是 data_deps，不代表链接进 Shot。
- **`//ui/gl:dummy_libEGL`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/core:core` → `//cc:cc` → `//ui/gl:gl` → `//ui/gl:generate_angle_stubs` → `//ui/gl:dummy_libEGL`。其中 ui/gl 引入该产物的边是 data_deps，不代表链接进 Shot。
- **`//ui/gl:dummy_libGLESv2`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/renderer/core:core` → `//cc:cc` → `//ui/gl:gl` → `//ui/gl:generate_angle_stubs` → `//ui/gl:dummy_libGLESv2`。其中 ui/gl 引入该产物的边是 data_deps，不代表链接进 Shot。
- **`//ui/lottie:lottie`**：`//shot:shot` → `//shot:shot_core` → `//ui/base:base` → `//ui/lottie:lottie`。
- **`//printing/mojom:mojom`**：`//shot:shot` → `//shot:shot_core` → `//third_party/blink/public:blink` → `//third_party/blink/public:blink_headers` → `//printing/mojom:mojom`。

## 附录 B：third_party 全部一级目录

逐包清点覆盖磁盘上的 145 个一级目录。文件数统计排除 `.git`、`node_modules`、`__pycache__`，不跟随符号链接；对疑似空目录又包含缓存作了复查。子模块内部代码计入“磁盘文件”，不计入“主库文件”。GN 可达数是 Windows 单平台；0 不是六平台删除证明。

| 目录 | 主库文件 | 子模块入口 | 磁盘文件 | Windows可达target | 当前状态/处理范围 |
|---|---:|---:|---:|---:|---|
| `abseil-cpp/` | 448 | 0 | 448 | 181 | K 运行基础/绘图/字体/网络；不整体删 |
| `android_deps/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `androidx/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `androidx_javascriptengine/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `angle/` | 0 | 1 | 13404 | 1 | B GPU类型/配置/整包检出；见正文 |
| `anonymous_tokens/` | 2 | 0 | 2 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `apache-portable-runtime/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `apple_apsl/` | 3 | 0 | 3 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `aria-practices/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `blink/` | 8977 | 0 | 9004 | 459 | K + A/B/C；正文逐子系统审计 |
| `boringssl/` | 2 | 1 | 8638 | 4 | K 运行基础/绘图/字体/网络；不整体删 |
| `breakpad/` | 3 | 0 | 3 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `brotli/` | 110 | 0 | 110 | 5 | K 运行基础/绘图/字体/网络；不整体删 |
| `cast_core/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `catapult/` | 0 | 1 | 8217 | 0 | T/C 开发/生成依赖；核查hook消费 |
| `ced/` | 3 | 1 | 31 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `clang-format/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `closure_compiler/` | 2 | 0 | 2 | 0 | T/C 开发/生成依赖；核查hook消费 |
| `colorama/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `compiler-rt/` | 2 | 1 | 4881 | 1 | T 构建/平台工具；非运行功能 |
| `cppgc/` | 307 | 0 | 307 | 5 | K 运行基础/绘图/字体/网络；不整体删 |
| `cpu_features/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `cpython3/` | 0 | 0 | 4280 | 0 | T 构建/平台工具；非运行功能 |
| `crashpad/` | 347 | 0 | 347 | 9 | B/C 需要分离实现/公共用途；见正文 |
| `depot_tools/` | 0 | 1 | 7875 | 0 | T 构建/平台工具；非运行功能 |
| `domato/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `emoji-segmenter/` | 3 | 1 | 10 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `expat/` | 3 | 1 | 220 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `fadec/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `fast_float/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `fdlibm/` | 6 | 0 | 6 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `federated_compute/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `flatbuffers/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `fontconfig/` | 16 | 1 | 16 | 0 | K 运行基础/绘图/字体/网络；不整体删 |
| `freetype/` | 5 | 1 | 762 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `fuzztest/` | 2 | 0 | 2 | 0 | T/B 测试依赖；核查真实构建消费 |
| `glib/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `glslang/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `google-closure-library/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `google-truth/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `google_benchmark/` | 2 | 0 | 2 | 0 | T/B 测试依赖；核查真实构建消费 |
| `googletest/` | 3 | 1 | 254 | 0 | T/B 测试依赖；核查真实构建消费 |
| `gperf/` | 0 | 1 | 186 | 0 | T 构建/平台工具；非运行功能 |
| `grpc/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `gsettings-desktop-schemas/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `gvdb/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `harfbuzz/` | 3 | 1 | 3796 | 4 | K 运行基础/绘图/字体/网络；不整体删 |
| `highway/` | 3 | 1 | 281 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `hyphenation-patterns/` | 1 | 0 | 1 | 0 | B/T 只剩BUILD，Blink测试目标引用；不等于可关闭文字断词 |
| `iaccessible2/` | 3 | 0 | 3 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `icu/` | 0 | 1 | 7119 | 9 | K 运行基础/绘图/字体/网络；不整体删 |
| `ink/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `inspector_protocol/` | 3 | 0 | 3 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `ipcz/` | 93 | 0 | 93 | 5 | B/C 需要分离实现/公共用途；见正文 |
| `isimpledom/` | 3 | 0 | 3 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `jinja2/` | 29 | 0 | 29 | 0 | K/T 生成器或语言/通用依赖；需按消费者拆 |
| `jni_zero/` | 4 | 0 | 4 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `jsoncpp/` | 4 | 0 | 4 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `junit/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `khronos/` | 14 | 0 | 14 | 0 | B GPU类型/配置/整包检出；见正文 |
| `leveldatabase/` | 2 | 0 | 2 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `libFuzzer/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `libc++/` | 0 | 1 | 12408 | 0 | T 构建/平台工具；非运行功能 |
| `libc++abi/` | 0 | 1 | 163 | 0 | T 构建/平台工具；非运行功能 |
| `libdisplay-info/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `libdrm/` | 2 | 1 | 2 | 0 | B GPU类型/配置/整包检出；见正文 |
| `libei/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `libgudev/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `libinput/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `libjpeg_turbo/` | 0 | 1 | 308 | 7 | K 运行基础/绘图/字体/网络；不整体删 |
| `libpfm4/` | 2 | 0 | 2 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `libphonenumber/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `libpng/` | 4 | 0 | 4 | 0 | K 运行基础/绘图/字体/网络；不整体删 |
| `libprotobuf-mutator/` | 3 | 0 | 3 | 0 | T/B 测试依赖；核查真实构建消费 |
| `libsync/` | 4 | 0 | 4 | 0 | B GPU类型/配置/整包检出；见正文 |
| `libunwind/` | 0 | 1 | 81 | 0 | T 构建/平台工具；非运行功能 |
| `liburlpattern/` | 16 | 0 | 16 | 1 | B/C 需要分离实现/公共用途；见正文 |
| `libwebp/` | 3 | 1 | 352 | 17 | K 运行基础/绘图/字体/网络；不整体删 |
| `libxml/` | 90 | 0 | 90 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `libxslt/` | 48 | 0 | 48 | 1 | B/C 需要分离实现/公共用途；见正文 |
| `libyuv/` | 0 | 1 | 191 | 2 | B/C 需要分离实现/公共用途；见正文 |
| `llvm-build/` | 0 | 0 | 338 | 0 | T 构建/平台工具；非运行功能 |
| `llvm-libc/` | 2 | 1 | 8183 | 2 | T 构建/平台工具；非运行功能 |
| `llvm-libclang/` | 0 | 0 | 21 | 0 | T/C 本地工具包；无当前直接DEPS入口 |
| `lss/` | 0 | 1 | 0 | 0 | T 构建/平台工具；非运行功能 |
| `mako/` | 0 | 0 | 0 | 0 | 本地缓存残留：17 个文件；无业务源码 |
| `markupsafe/` | 8 | 0 | 8 | 0 | K/T 生成器或语言/通用依赖；需按消费者拆 |
| `material_color_utilities/` | 3 | 1 | 389 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `metrics_proto/` | 40 | 0 | 40 | 3 | A/B 无用功能链，先解耦后删 |
| `microsoft_dxheaders/` | 3 | 1 | 84 | 0 | B GPU类型/配置/整包检出；见正文 |
| `microsoft_webauthn/` | 3 | 0 | 3 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `minigbm/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `modp_b64/` | 6 | 0 | 6 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `mutter/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `nasm/` | 0 | 1 | 1356 | 1 | T 构建/平台工具；非运行功能 |
| `nearby/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `ninja/` | 0 | 0 | 3 | 0 | T 构建/平台工具；非运行功能 |
| `node/` | 4 | 0 | 2744 | 0 | T/C 开发/生成依赖；核查hook消费 |
| `oak/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `ocmock/` | 3 | 0 | 3 | 0 | T/B 测试依赖；核查真实构建消费 |
| `one_euro_filter/` | 5 | 0 | 5 | 1 | A/B 无用功能链，先解耦后删 |
| `ots/` | 3 | 1 | 522 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `perfetto/` | 0 | 1 | 12459 | 274 | B/C 需要分离实现/公共用途；见正文 |
| `pffft/` | 5 | 0 | 5 | 1 | A/B 无用功能链，先解耦后删 |
| `ply/` | 7 | 0 | 7 | 0 | K/T 生成器或语言/通用依赖；需按消费者拆 |
| `protobuf/` | 296 | 0 | 296 | 15 | K/T 生成器或语言/通用依赖；需按消费者拆 |
| `pyjson5/` | 10 | 0 | 10 | 0 | K/T 生成器或语言/通用依赖；需按消费者拆 |
| `pywebsocket3/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `quic_trace/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `rapidhash/` | 4 | 0 | 4 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `re2/` | 3 | 1 | 141 | 1 | B/C 媒体片段/脚本探测/URLPattern/屏幕解析，按消费者拆 |
| `requests/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `rust/` | 4827 | 0 | 4827 | 151 | K/T 生成器或语言/通用依赖；需按消费者拆 |
| `rust-toolchain/` | 0 | 0 | 7070 | 41 | T 构建/平台工具；非运行功能 |
| `securemessage/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `simdutf/` | 5 | 0 | 5 | 2 | K 运行基础/绘图/字体/网络；不整体删 |
| `siso/` | 0 | 0 | 2 | 0 | T/C 本地工具包；无当前直接DEPS入口 |
| `skia/` | 0 | 1 | 12292 | 0 | K 运行基础/绘图/字体/网络；不整体删 |
| `smhasher/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `snappy/` | 2 | 0 | 2 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `speech-dispatcher/` | 3 | 0 | 3 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `spirv-cross/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `spirv-headers/` | 0 | 1 | 121 | 0 | B GPU类型/配置/整包检出；见正文 |
| `spirv-tools/` | 0 | 1 | 1752 | 0 | B GPU类型/配置/整包检出；见正文 |
| `sqlite/` | 10 | 1 | 2228 | 2 | A/B 无用功能链，先解耦后删 |
| `test_fonts/` | 5 | 0 | 92 | 0 | T/B 测试依赖；核查真实构建消费 |
| `typescript/` | 3 | 0 | 238 | 0 | T/C 开发/生成依赖；核查hook消费 |
| `ukey2/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `vulkan-headers/` | 0 | 1 | 89 | 1 | B GPU类型/配置/整包检出；见正文 |
| `vulkan-loader/` | 0 | 1 | 288 | 2 | A/B 无用功能链，先解耦后删 |
| `vulkan-tools/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `vulkan-utility-libraries/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `vulkan-validation-layers/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `wayland/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `wayland-protocols/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `webgl/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `weston/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `win_virtual_display/` | 5 | 0 | 5 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `woff2/` | 18 | 0 | 18 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `wpt_tools/` | 0 | 0 | 0 | 0 | 本地残留空目录；无主库/子模块/直接DEPS入口 |
| `wtl/` | 3 | 0 | 23 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `wuffs/` | 3 | 1 | 17 | 1 | K 运行基础/绘图/字体/网络；不整体删 |
| `x11proto/` | 3 | 0 | 3 | 0 | B/T 主要为GN/元数据骨架；检查加载链 |
| `zlib/` | 47 | 0 | 47 | 8 | K 运行基础/绘图/字体/网络；不整体删 |
| `zstd/` | 2 | 0 | 2 | 1 | K 运行基础/绘图/字体/网络；不整体删 |

复查结果：**48 个目录没有普通文件**；`mako` 只剩 17 个 Python 字节码缓存。它们与 SQL/GPU/media 仍参与构建的问题不同。

## 附录 C：主要混合目录的子目录规模

以下是定位表，不是删除计数。每行文件包括源码、头文件、配置和资源；“该功能无用”不代表该目录内每个公共工具都无用。主要候选和保留边界均在正文。

### cc

| 子目录 | 主库文件 |
|---|---:|
| `animation/` | 38 |
| `base/` | 40 |
| `benchmarks/` | 20 |
| `debug/` | 12 |
| `input/` | 46 |
| `layers/` | 89 |
| `metrics/` | 66 |
| `mojo_embedder/` | 6 |
| `mojom/` | 37 |
| `paint/` | 121 |
| `raster/` | 36 |
| `resources/` | 13 |
| `scheduler/` | 17 |
| `slim/` | 1 |
| `tiles/` | 59 |
| `trees/` | 118 |
| `view_transition/` | 2 |

### components

| 子目录 | 主库文件 |
|---|---:|
| `base32/` | 1 |
| `cbor/` | 5 |
| `content_settings/` | 29 |
| `crash/` | 17 |
| `device_event_log/` | 6 |
| `discardable_memory/` | 10 |
| `embedder_support/` | 2 |
| `enterprise/` | 2 |
| `file_access/` | 1 |
| `headless/` | 6 |
| `input/` | 80 |
| `link_header_util/` | 3 |
| `memory_pressure/` | 1 |
| `metal_util/` | 1 |
| `network_session_configurator/` | 1 |
| `network_time/` | 3 |
| `performance_manager/` | 8 |
| `persistent_cache/` | 23 |
| `power_monitor/` | 1 |
| `prefs/` | 48 |
| `proto_extras/` | 5 |
| `services/` | 13 |
| `sqlite_proto/` | 1 |
| `sqlite_vfs/` | 26 |
| `stability_report/` | 1 |
| `unexportable_keys/` | 27 |
| `url_formatter/` | 3 |
| `url_pattern/` | 5 |
| `version_info/` | 1 |
| `viz/` | 176 |
| `web_package/` | 3 |
| `webrtc/` | 1 |

### ui

| 子目录 | 主库文件 |
|---|---:|
| `accessibility/` | 9 |
| `actions/` | 1 |
| `aura/` | 1 |
| `aura_extra/` | 1 |
| `base/` | 442 |
| `color/` | 62 |
| `compositor/` | 1 |
| `compositor_extra/` | 1 |
| `display/` | 141 |
| `events/` | 241 |
| `gfx/` | 496 |
| `gl/` | 164 |
| `latency/` | 7 |
| `linux/` | 13 |
| `lottie/` | 4 |
| `menus/` | 1 |
| `native_theme/` | 32 |
| `native_window_tracker/` | 1 |
| `ozone/` | 92 |
| `platform_window/` | 20 |
| `resources/` | 120 |
| `strings/` | 300 |
| `webui/` | 2 |
| `wm/` | 3 |

### third_party/blink/renderer/core

| 子目录 | 主库文件 |
|---|---:|
| `accessibility/` | 6 |
| `animation/` | 347 |
| `clipboard/` | 22 |
| `css/` | 969 |
| `display_lock/` | 11 |
| `dom/` | 364 |
| `editing/` | 389 |
| `events/` | 142 |
| `execution_context/` | 23 |
| `exported/` | 53 |
| `fetch/` | 8 |
| `fileapi/` | 35 |
| `frame/` | 278 |
| `fullscreen/` | 14 |
| `geolocation/` | 1 |
| `geometry/` | 31 |
| `highlight/` | 13 |
| `html/` | 782 |
| `image_replacement/` | 5 |
| `input/` | 40 |
| `inspector/` | 13 |
| `intersection_observer/` | 17 |
| `layout/` | 649 |
| `loader/` | 110 |
| `mathml/` | 29 |
| `navigation_api/` | 1 |
| `origin_trials/` | 3 |
| `overscroll/` | 7 |
| `page/` | 93 |
| `paint/` | 234 |
| `permissions_policy/` | 11 |
| `preferences/` | 3 |
| `probe/` | 7 |
| `resize_observer/` | 18 |
| `route_matching/` | 7 |
| `sanitizer/` | 14 |
| `scheduler/` | 20 |
| `script/` | 29 |
| `script_tools/` | 1 |
| `scroll/` | 43 |
| `skeleton/` | 8 |
| `style/` | 154 |
| `svg/` | 464 |
| `timing/` | 153 |
| `trustedtypes/` | 20 |
| `typed_arrays/` | 10 |
| `url/` | 15 |
| `url_pattern/` | 18 |
| `view_transition/` | 33 |
| `xml/` | 35 |

### third_party/blink/renderer/platform

| 子目录 | 主库文件 |
|---|---:|
| `animation/` | 9 |
| `bindings/` | 30 |
| `blob/` | 11 |
| `exported/` | 42 |
| `fonts/` | 220 |
| `geometry/` | 42 |
| `graphics/` | 374 |
| `heap/` | 41 |
| `image-decoders/` | 41 |
| `image-encoders/` | 4 |
| `instrumentation/` | 26 |
| `json/` | 4 |
| `loader/` | 172 |
| `mac/` | 2 |
| `mhtml/` | 9 |
| `mojo/` | 19 |
| `network/` | 43 |
| `scheduler/` | 158 |
| `storage/` | 4 |
| `text/` | 76 |
| `theme/` | 8 |
| `transforms/` | 23 |
| `weborigin/` | 18 |
| `widget/` | 60 |
| `wtf/` | 175 |

## 附录 D：原始证据与复查方式

- [GN 全目标快照](/D:/Github/chromium/out/screenshot-audit-20260907/gn-targets.json)：`buildtools/win/gn.exe desc out/Shot "*" --format=json`，本次成功导出。
- [根目录与文件清单](/D:/Github/chromium/out/screenshot-audit-20260907/inventory.json)：HEAD、普通文件、gitlink、根目录计数。
- [构建可达链](/D:/Github/chromium/out/screenshot-audit-20260907/reachable-targets.json)：从 shot/shot_c 遍历 GN deps 得到，包含 data 依赖。
- [第三方磁盘清单](/D:/Github/chromium/out/screenshot-audit-20260907/third-party-inventory.json)：145 个一级目录及空目录复查。
- [PFFFT 外部源码引用检查](/D:/Github/chromium/out/screenshot-audit-20260907/pffft-external-references.txt)：主库 C++/头文件中排除 pffft 自身后无匹配；只是辅助证据，不覆盖子模块内部、其他语言或平台预处理。
- [net 硬件密钥头引用](/D:/Github/chromium/out/screenshot-audit-20260907/net-key-references.txt)：builder/URLRequestContext 仍 include 服务头；禁用功能后仍有接口耦合。

原始快照放在 gitignored 的 `out/`，主报告包含关键结论、数量与路径，不依赖快照才能阅读。后续 HEAD 或配置变更后需重导；旧 CI graph 和旧二进制不作为本次当前行为证明。

### 独立完成：内存 HTTP 缓存后端

新增删除 net/disk_cache/memory 四文件，以及 InMemory 工厂、builder 选项与两个统计条件的专用分支。Shot 只有禁用缓存或显式 DISK_SIMPLE，OnBackendCreated 失败不选择其他后端；缓存失败继续无缓存运行。内部 HttpCacheParams 默认改为 DISK_SIMPLE，保留既有后续枚举数值；Simple 索引、Blink MemoryCache、Cookie 与 Shot 缓存 API 不变。证据/备份在 out/cut-stage17-memory-http-cache。当前父任务累计 62 删除、66 唯一修改；备份校验通过，未生成/编译/运行。Route 92 路径仍未应用，不能把整个源码批次标完成。
