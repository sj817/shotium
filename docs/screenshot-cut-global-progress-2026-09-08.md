# 静态截图引擎全局裁剪进度（stage50）

本轮已完成上层 BidirectionalStream 和 AcceptCHFrameObserver 闭包：27 个源码路径、9 个文件删除、18 个配套编辑，源码增加 33 行、删除 1781 行。普通 HTTP 和底层 HTTP/2 CONNECT 保留。备份、引用、2 份 GN 语法及 diff 检查通过；本轮没有执行完整图生成、编译或运行验收。

## 全局百分比与口径

- **源码裁剪与依赖收口：约 80%（估计范围 70%–85%）。** 大体积独立实现已删除；剩余多为跨 DOM、CSS、网络、遥测和构建目标的耦合收尾，不能按文件体积换算工作量。
- **当前累计改动的最终验收：0% 完成。** 表示当前源码尚无一套最终通过的构建和验收结果，并非之前没有测试。上一轮集中图生成仍失败，当前 C++/链接/运行/像素/六平台门均不能宣称通过。
- **全任务交付进度：约 65%，合理误差约 ±10 个百分点。** 为便于跟踪，暂按源码处理占 80%、最终构建修复和验收占 20% 估算，80%×80%≈64%，取整约 65%。这是工作量估算，不是自动统计完成率，也不代表有 65% 的当前产物已通过验收。
- 不能用 stage50/50 或已删文件数除旧 27k 文件数作为完成率。stage 是历史执行记录编号；第三方转为直接维护后文件分母也已变化。

## 已完成的主要源码工作

| 范围 | 当前已完成 | 仍需区分 |
|---|---|---|
| 根目录大组件 | gpu、sql、sandbox、device、google_apis 的现存跟踪文件已清空；prefs 和多个独立浏览器服务已清理 | 空目录实体清理与源码清空是两件事 |
| Canvas/浏览器嵌入层 | Canvas 绘图 API/资源桥接、WebView/WebWidget/Popup 大实现、系统剪贴板和多组编辑命令已移除 | 保留 canvas 标签静态 fallback、宽高比；部分交互类型仍需收口 |
| GPU/合成器/Skia | 大部分合成器执行端、GPU/GL/Vulkan/Skia GPU/PDF/Skottie 和桌面图形桥接已删除 | CPU paint、原生 SkCanvas、图片/字体/SVG/MathML 保留 |
| 网络 | PAC/WPAD/系统代理、备用磁盘/内存后端、旧服务协议、content_settings、WebSocket、上层双向流和多组观察者已清理 | 实际 HTTP/TLS/HTTP2/缓存及安全类型保留；WebBundle 等后续闭包待做 |
| 遥测/第三方/维护 | 栈堆采样、大量 UKM 调用点、若干 CrashKey 调用及无用测试/工具已清理；ICU/Skia/Perfetto 已改直接维护源码 | UKM/Crashpad 库和运行 tracing 尚未整链完成；三组受阻清单未删除 |

此前最后一套真正编译和运行通过的基线是第十一批 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb：Windows EXE/DLL/addon、serve/net、84 demos、Node/daemon/协议、Bilibili 与 183/183 像素一致。这不能证明 stage16–50 的累计改动已通过。六平台当前实际编译未完成。

## 全局待办：按后续大批次组织

以下是剩余工作的完整分组，不是允许整目录删除的名单。每组最终须裁掉无用闭包，或写出具体静态截图用途和最小保留范围。

| 顺序 | 根目录/范围 | 必须完成的工作 | 估计大批次 |
|---|---|---|---|
| 1 | Blink core / Route / URLPattern | 先解除 core/BUILD.gn 的失效 services/network:test_support 图阻点；处理 Route/CSS/URLPattern 原 92 路径提案，重查调用与当前源码，不能重放旧补丁覆盖后续修改 | 1–2 |
| 2 | third_party/perfetto、icu | 落实离线 trace processor 的 2039 个待删文件、ICU 的 16 个待删文件；检查独立外围工具。保留真实 Unicode、字体与数据生成 | 1 |
| 3 | Blink、services/metrics、components/crash、third_party/crashpad | 去掉 Document/DocumentLoader 的 6 个 UKM builder 调用及库/协议/生成器依赖；关闭已无外部调用的组件 CrashKey GN 链并清理 Crashpad，保留实际错误诊断 | 1–2 |
| 4 | base/trace_event、base/tracing、Perfetto 运行后端 | 处理记录、会话、导出和宏调用的实际依赖；不能与已做的离线 processor 混为一组，不能误删 CaptureStats/FCP/CHECK/真实日志 | 1–2 |
| 5 | net、services/network、Blink loader | WebBundle token/handle 和 Fetcher 的完整无生产者闭包；回查浏览器 policy、持久状态、剩余协议/traits、NetLog 导出与旧 IPC 等审计项，按实际调用给最终结论 | 1 |
| 6 | third_party/blink 交互与扩展 | editing、DataTransfer/拖放、fullscreen、fileapi/blob、AX、PerformanceObserver/User Timing、probe，以及剩余脚本关联类型；同时确认内部 observer、通用线程与表单/CSS 状态的最小保留；XSLT 等已有保留结论不重新按名字砍 | 1–2 |
| 7 | ui、base、build、third_party、根配置 | GRD/语言/桌面资源、latency/AX、系统 helper、测试模板与生成工具，re2/libyuv/ipcz 等实际依赖；同步 DEPS/.gn/BUILD/.gitmodules/trim-tree/prune-deps，核对空目录和全部 A/B/C 附录，避免同步后回流 | 1–2 |
| 8 | 全局验证与修复 | 最终 GN 图、缺失输入、生成类型/语法/jumbo、Windows EXE/DLL/addon；按错误集合批量修复，避免每个小修改完整构建 | 1–2 |
| 9 | 运行/像素/六平台 | serve/net/demos/Node/daemon/协议/Bilibili、完整像素和 Canvas 专项；六平台真实编译，交付保留/删除总表和最终证据 | 1–2 |

**合计预计约 10–16 个大批次**（上表端点相加约 9–16，按约 10–16 对外规划）。可并入同一依赖闭包的工作合并提交，批次不等于必须一批一个 commit。前提是受阻范围解除，且构建不暴露新的大规模依赖问题；出现额外耦合或平台问题需上调估计。这是剩余工作量预测，不承诺固定耗时。

## 当前阻点

1. 当前构建图失败在 third_party/blink/renderer/core/BUILD.gn:1283 的 //services/network:test_support，目标目录 BUILD.gn 已不存在。精确单行修复见 out/cut-stage49-graph/approval-review.md，尚未应用。
2. Route 92 路径、Perfetto 2039 个物理删除、ICU 16 个物理删除此前被自动审批拒绝；相关路径仍保留，不能把 GN 改过算作实体已删。该限制同时影响核心文件内的 UKM/CrashKey 等收口。
3. patches 和 mojo/public/tools/fuzzers 的空目录删除此前也被拒绝；不影响它们已删源码的事实，但最终目录验收仍要明确处理。
4. 自动审批只返回 blocked by policy，没有更详细原因。当前记录保留原拒绝范围，不通过换工具、补空 target 或改其他位置绕过。

## 证据与后续接续

- 本轮 manifest、备份核对、静态结果：out/cut-stage50-combined/；网络子代理交接：out/cut-stage50-bidirectional/report.md。
- 根目录复核基础：out/cut-stage45-root-review/report.md；其中 network/content_settings/WebSocket 等项目以 stage46–50 的实际删除为准，不能重复列为未完成。
- 最新根目录现存跟踪文件统计和受阻清单存在性：out/cut-stage50-combined/global-snapshot.json。目录文件数只用来验证存在状态，不用来证明用途或百分比。
- 主任务、原始审计、执行证据仍分别位于 screenshot-cut-task.md、screenshot-unused-code-audit-2026-09-07.md、screenshot-cut-execution-2026-09-07.md。历史未勾选项包含已做源码但未验收的内容，以本报告和顶部 stage50 状态解释，不把历史 checkbox 直接计为新工作量。
