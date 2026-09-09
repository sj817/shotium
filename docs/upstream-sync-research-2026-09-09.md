# Shotium 上游同步调研（2026-09-09）

## 结论

**值得同步，但建议“每个版本审查、按需同步渲染修复”，不要每个 Chromium 版本整体升级，更不要直接 merge upstream/main。**

当前优先收益来自现有功能的正确性与健壮性：XSLT 初始化、双向文本换行、CSS 颜色计算；其次是文本排版和 Grid 内存优化。没有证据表明必须立即整包升级 Blink、Skia 或整个工具链。新 CSS 特性可以按真实页面需求引入。

本次只调研：读取本仓、更新只读 upstream/main 引用、检查上游提交及部分补丁；未修改引擎、DEPS、字体设置，没有编译或触发 CI。下列“值得同步”是候选评估，不是已移植或已复现结论。

## 1. 我们到底落后多少

| 项目 | 本次核实结果 |
| --- | --- |
| 本仓审查提交 | `4035292e387b52ed721de672d0084f25c4061896` |
| 文档记录的 Chromium 基线 | `c0bba1026178fe2a8b441fead7928b697a801c1e`，2026-08-15，153.0.8010.0 |
| 本次抓取的上游 main | `c099bd180a2db0fa6a313d43653529ba02665c84`，2026-09-08 |
| main 的 chrome/VERSION | **155.0.8048.0** |
| 基线到上述 main 的提交数 | **17,000**，整个 Chromium 的历史范围，不是我们必须接收的数量 |
| 本仓当前跟踪路径数 | **31,201**，按 HEAD 的递归树统计，包含 gitlink |
| Skia 独立来源基线 | `653397c6be15b87fe8f89a4492582fbb825f6da8` |
| ICU 独立来源基线 | `8cc91d9b6ab9991802fd208ee03a69714fd0251c` |

Chrome 153 的官方稳定版发布日期是 9 月 8 日。因此我们是 **153 的早期快照加本地裁剪与修改**，不是已经完整同步 153 稳定分支，更不是简单“落后两个正式版本”。本次 main 候选可能来自 154/155 开发周期，未逐条核实是否回合到 153 稳定分支，不能宣传成 153 已发布功能。[Chrome 153 官方说明](https://developer.chrome.com/release-notes/153)

Chrome 从 153 开始转为两周里程碑节奏。对我们更合适的是每两周筛选一次，而非两周一次重做裁剪。[官方周期说明](https://developer.chrome.com/blog/chrome-two-week-start)

### 当前差异规模

用三个 Git 树的对象 ID 比较：旧 Chromium 基线、本仓 HEAD、新 upstream/main。没有使用“本地根提交以后是否改过”来替代真实基线比较。

| 当前保留路径的状态 | 数量 | 含义 |
| --- | ---: | --- |
| 旧、新上游都存在，上游内容没变 | 16,774 | 本轮不需要为了追上游而更新 |
| 上游变了，本地仍等于旧基线 | 2,038 | 可优先评估，但不代表可以整文件覆盖 |
| 上游和本地都变了 | 587 | 需要逐项语义审查，不等于 587 个实际冲突 |
| 旧上游存在，新上游已删除 | 325 | 要确认本地依赖，不能直接跟删 |
| 旧、新 Chromium 树均无同路径 | 11,477 | 包括直接内置的第三方源码及项目文件，不能全部称作原创代码 |
| 合计 | **31,201** | 未将上游新增但本地不存在的路径算入分母 |

其中现存 Blink 路径的变动包括 CSS 154、layout 164、paint 49、SVG 29、platform/fonts 40 个文件。它们仍混有重构、接口和实验功能，不能按目录全部接收。原始逐路径数据位于本地 `out/upstream-research/retained-files.csv`，用于复核，不是删除清单。

## 2. 现在具体值得同步什么

优先级：P0 表示应先做安全/健壮性适用性核查；P1 表示优先小批移植；P2 表示有价值但需要专项需求或更大验证；P3 暂缓。难度是源码审查估计，不包含实际跨平台构建排队时间。

| 优先级 | 上游提交及内容 | 对 Shotium 的价值 | 本仓证据、成本与限制 |
| --- | --- | --- | --- |
| **P0 核查** | [`7d8b7fc6c403`](https://chromium.googlesource.com/chromium/src/+/7d8b7fc6c403d128ae37b5ed0d25fd3c95ff4e33)：初始化时注销 libxslt 非标准多文档输出扩展 | 我们保留原生 XML/XSLT；上游指出扩展在样式表编译时就绑定，早于逐次转换配置，不能只看转换时的安全设置 | 本地 `xslt_extensions.cc/.h` 仍等于旧基线，未包含新的初始化函数。上游改 3 个生产文件，预计低到中难度；须核对本地 PI 转换调用链与初始化线程，测试扩展回退及禁止外部写入。**不是已确认本产品存在可利用漏洞** |
| **P1** | [`4ec4e4e42a1d`](https://chromium.googlesource.com/chromium/src/+/4ec4e4e42a1dd3e29f795eee3afa49b8ce51e091)：进入 unicode-bidi 范围时补换行机会 | 阿拉伯语、希伯来语、中英文混排，连字符后紧接 `<bdi>` 等边界时，避免漏掉合法换行 | 本地 line_breaker.cc 仍是旧实现；上游生产变动主要为一个函数和 `LineBreakBidiControlEnter` stable 开关。低难度，需窄栏、RTL、连字符像素用例 |
| **P1** | [`f4df34e60889`](https://chromium.googlesource.com/chromium/src/+/f4df34e60889cef0b48ca1f886b42fef9c7ecc23)：color-mix()/palette-mix() 权重和为零时正确返回透明结果 | 纯 CSS 页面即可触发；影响颜色、透明度和彩色字体调色板 | 本地仍有 `p1 == 0 && p2 == 0` 返回失败逻辑。约 6 个生产文件，解析、归一化、调用者及序列化必须一起移植，不能只改返回值。中难度，测试字面 0%、calc() 得到 0、透明背景和字体调色板 |
| **P1 性能候选** | [`c69cb2a9cbff`](https://chromium.googlesource.com/chromium/src/+/c69cb2a9cbff5dd68d0f2a22c1ea5776463d762c)：HarfBuzz 字形宽度转换去间接调用并利于向量化 | 纯文本排版路径，无需 JS；长文章、表格、混排均可能受益 | 本地仍有循环中的 `round_if_subpixel` 函数指针；上游仅改 skia_text_metrics.cc/.h，低到中难度。**不能把上游 Speedometer 数字当作 Shotium 收益**；保持舍入、AA/gamma、字体选择不变，像素与性能都要验证 |
| **P2** | [`25c4d384b8c7`](https://chromium.googlesource.com/chromium/src/+/25c4d384b8c78e26796de3c1fc277a06e7cca8bd)：Grid track sizing 完成后释放中间集合 | 大 Grid 页面可能降低内存占用 | 约 6 个生产文件，关联 grid_lanes；本地 track collection 尚未更新。上游 large-grid 用例约释放 417 KB，仅是该用例。中难度，需重复布局、子网格、分片/多栏测试 |
| **P2** | [`937ddee6b39a`](https://chromium.googlesource.com/chromium/src/+/937ddee6b39a0a2af5df2bdd641619e509d1a4ef)：sticky inline 里的 sticky box 偏移修复 | 静态页面也可能包含 sticky；Shot 的长图分段/滚动绘制使这类问题有实际意义 | 约 5 个生产文件，触及本地重度裁剪的 LayoutObject 等；中到高难度。比较整图与分段、嵌套 sticky/RTL，不需要恢复浏览器输入链 |
| **P2** | [`c6c0a98dd0ac`](https://chromium.googlesource.com/chromium/src/+/c6c0a98dd0acd039e57591dce7e2ccecad0ca91b)：SVG 空属性值按无效值处理 | 修复尺寸、角度、数字、preserveAspectRatio 默认值；真实 SVG 图片会受影响 | 新的 feature gate 与多种 SVG 属性类联动，中难度。应先固定预期再接受像素变化，不批量开启全部实验 SVG 特性 |
| **P2/后批** | [`49ab5f77a032`](https://chromium.googlesource.com/chromium/src/+/49ab5f77a032) + [`1f237d50988b`](https://chromium.googlesource.com/chromium/src/+/1f237d50988b67fcd6e4fcd14373d98ae64fe550)：line-clamp 内浮动元素裁剪 | 摘要卡片、浮动图片与截断文本可能受益 | 后一个提交明确依赖前置 float 标记，还新增 clip node，触及我们改动过的绘制属性树。高难度；要先核对 line-clamp 功能开关与实际适用语法，再做单独批次 |
| **P3** | [`3e5d0b609aab`](https://chromium.googlesource.com/chromium/src/+/3e5d0b609aab74d7b0540ce98221cb7ae77ae230)：var() fallback 前导空白的断言位置修复 | 有利于调试构建与边界输入健壮性 | 本地仍保留旧 DCHECK，但这主要是断言修复，不能包装成已确认的发行版渲染错误；可顺手加入 CSS 批，不值得单独跑六平台 |
| **P3** | [`47fc06ba6c38`](https://chromium.googlesource.com/chromium/src/+/47fc06ba6c38379fcb788927a14c4ea6dd9a6a73)：legend 不接收 fieldset break token | 清理内部不变量 | 上游说明预期不改变行为。虽然只有一行，也不应为了提交数量当作高收益更新 |

这些提交是从真实 Git 范围筛出的代表候选，不是全部 17,000 个提交的逐条安全审计。最终移植前还需查目标提交之后的修复、回滚及依赖；“能无冲突应用”不代表正确。

## 3. 哪些新版本功能不值得跟，哪些已存在

Chrome 153 发布说明不等于我们缺失的功能清单。核对本仓后：[官方说明](https://developer.chrome.com/release-notes/153)

- **Single-axis scroll containers**：本仓已经有实现和 experimental 开关，上游 153 说明也明确标为非稳定渠道可用。不要当作一个全新缺失功能直接启用；它改变 overflow/sticky 几何，应等明确需求和专项用例。
- **Rust XML parser**：本仓已有 Rust parser 与 `XMLRustForNonXslt`，后者当前为 experimental，不能称为“完全没有”或“已经完整启用”。153 的 SVG 适用路径值得单独核查，但不能为此破坏保留的原生 XSLT。分块 XML/样式表恢复修复 [`7e9243ad45b8`](https://chromium.googlesource.com/chromium/src/+/7e9243ad45b8a8a98f4c35ee7c37d2b6cccd931d) 也需先证明当前实际进入 Rust 解析路径。
- **scroll-axis-lock**：主要控制用户滚动手势，对无交互截图收益很低。
- **transitionrun / media-query change 事件派发时间**：JS 事件收益不适用；若提交同时改变原生 CSS 动画采样，需要单独拆开审查，不能按标题整批接收。
- **camera/microphone、WebAudio、WebGPU、JavaScript Iterator、脚本性能标记**：不进入本产品同步范围。
- **HTML-in-Canvas、PaintWorklet、DevTools、浏览器 UI、V8 优化**：继续排除。普通 SkCanvas CPU 绘制与 Web Canvas API 是不同范围，不能混同。
- **XSLT 弃用横幅/移除计划**：与本产品保留 XML/XSLT 的选择冲突，不照搬。如上游 [`0c473f917938`](https://chromium.googlesource.com/chromium/src/+/0c473f917938) 的弃用警告不应污染截图结果。

## 4. 同步边界不能只写“CSS”

截图结果依赖一条完整链路：HTML/XML/SVG 解析 → CSS 解析与计算 → 布局与文字塑形 → CPU paint/Skia → 图片编码。外部样式、字体、图片还依赖网络、TLS、压缩和图片解码器。

建议长期跟踪以下范围，而不是只跟 `core/css/`：

| 范围 | 跟踪策略 |
| --- | --- |
| Blink CSS/style/layout/paint/SVG/HTML、字体与图片解码 | 每个里程碑筛选兼容性修复；小补丁优先 |
| Skia CPU raster、HarfBuzz、FreeType、ICU、图片库、libxml/libxslt | 单独记录来源基线，优先安全和确定性修复；不跟入 GPU/工具/测试整树 |
| net、TLS、压缩库、基础内存组件 | 筛选仍可达路径的修复；无 JS 不代表不处理不可信字节 |
| GN/编译器/Rust/CIPD 工具链 | 有兼容需求、安全需求或必要前置时成组升级，不随每个 Blink 小修复强制升级 |
| V8/content、GPU 进程、媒体、交互、诊断上报等已移除功能 | 默认不接收，不为解决 include/链接错误直接恢复组件 |

例如 9 月 1 日官方安全公告列有 Skia 信息泄漏修复。只能据此把 Skia 放进适用性审查队列；公告名称不足以证明本地 CPU 路径受影响，仍要核对修复提交、Skia 版本和触发条件。WebGL/V8 等已删除范围也不能与保留的 Skia 混为一谈。[官方安全公告](https://chromereleases.googleblog.com/2026/09/stable-channel-update-for-desktop.html)

## 5. 现有同步文档必须先纠正的地方

`docs/upstream-sync.md` 可以作为历史记录，但不能照着旧命令直接执行：

1. **64,425 个路径、1,193 个手工文件、98% 机械操作是旧数据。** 当前树已再次大幅裁剪，需重新生成差异账本。
2. **根提交“以后没改过”不代表等于上游基线。** 根提交本身就可能包含裁剪。应比较 OLD/HERE/NEW 的内容，内置第三方另用各自来源基线。
3. **原样文件也不能默认整份覆盖。** 上游新版可能引入被删除模块的调用、新生成接口或 ABI 依赖；按语义提交及依赖闭包接收。
4. **LASTCHANGE 不再是独立上游证据。** 本次本地文件实际写的是本仓根 `ac613e9...`，不是文档所称的 c0bba...；现有 CI 会处理时间戳/版本戳。不要用它认定同步完成。
5. **旧 `out/ShotWip` + 手写 gn/ninja 流程已与 CLAUDE.md 冲突。** 后续使用当前 build-engine/verify-engine 流程，Windows 构建走 `pnpm build:engine` 和 `out/Shot`；跨平台仍用现有 CI。
6. **局部挑选修复后不能把 UPSTREAM_BASE 直接改成 main 最新 SHA。** 否则下次会把尚未审查的中间提交误认为已同步。只有完整定义范围的基线迁移通过后才前移基线。
7. **内置 Skia/ICU/Perfetto 不恢复成 DEPS checkout。** 外部依赖仅在适用时同步 DEPS/.gitmodules/gitlink，不能机械把三者所有条目数量要求相同。

## 6. 推荐的长期工作方式

### 两种同步分开管理

**日常：选择性修复。** 每个 Chrome 里程碑审查，先以稳定分支为优先来源；main 只挑明确修复且可独立移植的提交。安全/崩溃类不必等下一个里程碑。一次汇总若干相关修复，集中构建和验收。

**周期性：渲染基线迁移。** 建议先以 2–3 个里程碑为评估窗口，而非硬性期限。遇到真实页面缺新 CSS、补丁前置过长、旧接口维护成本明显上升时，再把整个“保留的渲染范围”迁移到一个固定上游基线。这仍不等于整仓合并。

### 每次同步的账本

建议新增机器可读清单，记录：上游 repo/SHA、关联提交、对应本地提交、接受/跳过/已包含/待核查、理由、触及路径、新增依赖、功能开关、验证用例、验证 SHA。报告是它的摘要，而不是唯一数据源。

保留“已删除范围”的明确约束，但不能只用路径黑名单：像 `ui/gfx`、`cc/paint`、Skia 的 CPU 部分仍可能有真实消费者。新文件须沿调用与构建关系审查；旧六平台图用于发现范围，不足以证明新文件不需要。

### 验证方法

1. 把每个选中修复对应的最小 HTML/CSS/SVG 输入移入我们自己的用例；WPT 的 JS 测试驱动不能原样运行，可提取静态输入与参考输出。
2. 保留同步前二进制作对照。正确性修复允许目标用例出现**预期**像素差异，不允许把所有新结果直接更新成基线；非目标用例应保持。
3. 补丁成批完成后做图/缺失输入检查，再集中构建、运行现有检查。改字体转换时额外检查舍入、不同字体与六平台差异；不调整现有 AA/gamma/像素几何设置来掩盖差异。
4. 性能候选用本地固定输入、相同参数对比旧/新引擎，分别看耗时与内存；不把浏览器上游的性能样本套用到本产品。
5. 最终同一代码 SHA 的六平台二进制/原有检查完成后记录验收状态，再发布。同步本身不自动授权新版本发布。

## 7. 建议的第一轮工作量

**第一批：XSLT 加固 + 双向文本换行 + CSS 零权重颜色修复。** 三组先做调用与用例核查，再一起移植，必要时顺带 var() 断言修复。粗估 1–2 个工程日，主要成本在本地语义差异和用例，不在改动行数；未做试移植，估算置信度中低。

**第二批：字形转换性能 + Grid 中间内存。** 有明确优化方向但收益需实测。粗估 1–2 个工程日；任何像素回归先解决，不能用“上游就是这样”跳过。

**第三批按需求：sticky、SVG 空属性、line-clamp 浮动裁剪。** 前两者可独立选；line-clamp 需要较多绘制树审查，建议单独安排，不为了凑版本一起带入。粗估 2–4 个工程日，风险高于前两批。

上述为方案估算，不是已开始的实施目标，也不是保证交付时间。**如果下一步只选一批，我建议先第一批；目前没有理由马上做一次整版 Chromium 同步。**
