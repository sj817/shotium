# 砍除进度

> 波次 3 结束（2026-08-16）。策略：一刀切到位，中途不编译；
> 反馈回路是 `gn gen`（约 25 秒）加闭包 diff，不是 ninja。

## 1. 总量

| | 基线 `f79cc2b74310` | 现在 | 变化 |
|---|---:|---:|---:|
| GN 闭包（`gn desc //shot:shot deps --all`） | 9,747 | **3,414** | −6,333（−65.0%） |
| git 跟踪文件删除 | — | 210,589 | — |
| git 跟踪行删除 | — | 31,905,411 | — |

三次提交：

```
bbe6e127218a  146,168 files changed,  746 insertions, 21,571,815 deletions
3dde8ebd6c58   52,903 files changed,  325 insertions,  7,981,080 deletions
波次 3          11,518 files changed,  795 insertions,  2,352,516 deletions
```

波次 3 的 git 数字比前两次小得多，但磁盘上删得最多（约 4 GiB）——因为
`perfetto`、`dawn`、`devtools-frontend`、`jetstream`、`speedometer` 这些
都是 DEPS 拉下来的，不在主仓版本控制里。**这正是必须同时改 DEPS 的原因**
（见第 4 节，波次 3 已兑现）。

## 2. 已删除

### 波次 1 — `bbe6e127218a`（闭包 9747 → 7730）

| 类别 | 内容 |
|---|---|
| 产品目录 | `chrome` `headless` `extensions` `ash` `chromeos` `ios` `android_webview` `chromecast` `fuchsia_web` `remoting` `content/shell` `content/web_test` `apps` `tools/perf` `ui/views_content_client` |
| WebUI | `ui/webui` 全套 313 targets，枢纽是 `content/browser/webui/content_web_ui_configs.cc`；连带 lit / polymer / d3 / material_web_components / cros-components / lottie 整条 TypeScript 工具链 |
| DevTools 前端 | 1,405 targets，唯一那条 `content/browser/devtools → blink_generate_devtools_grd → third_party/devtools-frontend` 边 |
| 产品标识搬家 | `chrome/VERSION` → `shot/VERSION`；BRANDING、`chrome_version.rc.version` → `shot/`；`chrome/version.gni`、`process_version_rc_template.gni` → `build/util/` |

### 波次 2 — `3dde8ebd6c58`（闭包 7730 → 4708）

| 类别 | 内容 |
|---|---|
| Blink modules | 全部 120 个子目录 + `bindings/modules` + `bindings/extensions` + `generated_in_modules.gni` / `idl_in_modules.gni` |
| content/browser | 109 个子目录 → 27 |
| WebRTC | `third_party/webrtc` + `webrtc_overrides` + `libsrtp` + `openh264` + `crc32c` + `media/webrtc` + `components/webrtc` + blink platform 的 peerconnection / mediastream / p2p / video_capture，171.8 MiB |
| ML 栈 | xnnpack、tflite、litert、mediapipe、coremltools、tensorflow_models、ruy、fp16、pthreadpool、cpuinfo、eigen3、gemmlowp、farmhash、`services/webnn`、`services/on_device_model`、`components/language_detection`，662.1 MiB |
| components | 412 → 41（其中 296 个本就不在闭包里）；`components/BUILD.gn` 清空 |
| services | 24 → 11 |
| device / media | `device/{bluetooth,fido,gamepad,vr}`、`media/{cast,remoting,midi,cdm,muxers,device_monitors}`、`media/capture`、`content/services/isolated_xr_device`、`third_party/{widevine,openxr}` |

**Blink modules 的边界比预想干净。** `core/` 里 `#include "third_party/blink/renderer/modules/..."`
的次数是 **0**，`controller/` 只有 5 处。`CoreInitializer` 的 22 个纯虚函数是 core 伸进
modules 的全部通道，`ModulesInitializer` 现在是它们的空实现。

**但它单独只减了 294 个 target。** WebRTC、ML、media 这些后端都被 `//content` 独立依赖，
必须从 Content 侧一起切——和 `closure-measurements.md` 3.5 节 `content/utility` 的结论一致。

### 波次 3（闭包 4708 → 3414，`gn gen` 通过）

三个子代理并行删除，主线程串行收敛构建图——它们都不跑 `gn`、不碰 `git`，
否则会撞 `environment.x64` 和 `index.lock`。

| 类别 | 内容 |
|---|---|
| V8 组 | `v8` 147.4 MiB、`gin`、`blink/renderer/bindings` 4.9 MiB / 635 文件、`pdf`、`third_party/pdfium`、`tools/v8_context_snapshot`、`tools/code_cache_generator` |
| tracing | `third_party/perfetto` 188.1 MiB、`base/tracing` 63.8 MiB、`services/tracing`、`base/trace_event`、`components/tracing`、`base/test/tracing` |
| Dawn | `third_party/dawn` 1167.4 MiB、`webgpu-cts` 61.5 MiB、ANGLE 的 `libANGLE/renderer/wgpu`；`use_dawn` / `skia_use_dawn` / `angle_enable_wgpu` 三个 arg 源码级删除 |
| 媒体管线 | `media/{mojo,audio,filters,formats,parsers,renderers,video,gpu,capabilities,test,ffmpeg}`、`ui/accessibility`、`services/device` |
| 编解码器 | ffmpeg、dav1d、libaom、libvpx、libgav1、opus、flac、iamf_tools、libwebm、crabbyavif，266 MiB |
| 大件 third_party | `devtools-frontend` 862.4 MiB、`jetstream` 721.4 MiB、`speedometer` 405.5 MiB、`webpagereplay` 212.4 MiB、`screen-ai` 225.3 MiB，加另外 70 个无引用目录 |
| DEPS | 5,163 → 4,548 行，删掉对应 deps / hooks / recursedeps；`ast.parse` 校验通过 |

`webgl` 和 `catapult` **没删**：前者被 `content/test` 的 telemetry 目标当 data 依赖，
后者有 27 处活引用散在 `build/android`、`testing/`、`mojo/public/tools`、`tools/grit`。

## 3. 剩余闭包构成

3,414 个 target 按顶层目录：

```
third_party 1169    services  626    ui        416    components 373
gpu          121    content   118    build      94    base        81
mojo          75    skia       62    net        57    url         53
cc            53    ipc        25    sandbox    24    printing    20
```

`third_party` 里最大的几块：

| | targets | 挂点 |
|---|---:|---|
| blink | 522 | 本体 |
| abseil-cpp | 181 | `base` 的传递依赖 |
| rust + rust-toolchain | 221 | 工具链 |
| angle | 42 | GL 后端 |
| swiftshader | 28 | 软件 Vulkan |

**blink 只从 544 降到 522。** 砍掉的是它外围的 modules 和 bindings，
core 本体就是渲染管线本身——这个数字不下降才是符合预期的。

## 4. 欠下的债（波次 3 后）

前一版这一节的四条里，DEPS、devtools-frontend、dawn 三条已在波次 3 兑现。
剩下的是**已知会在首次编译时爆的东西**，都是砍的必然结果，不是意外。

### 4.1 V8 的真实代价不在 `//v8` 目录

```
blink/renderer/bindings/           4.9 MiB / 635 文件   IDL 生成的胶水，已删
blink/renderer/platform/bindings/  0.7 MiB / 116 文件   V8 抽象层，还在
   └─ blink core 有 852 个文件、1157 处 include 它
```

`ScriptState` / `ScriptWrappable` / `V8PerIsolateData` 住在 `platform/bindings`，
**不在**已删的 `bindings/` 里。全树 strip 摘掉了 1,283 个文件里的 `#include "v8/..."`
共 200,853 字节，但**代码体里的符号用法原样保留**。真正的 V8ectomy 是重写这 852 个文件，
删目录只是把债务显性化。

### 4.2 `base/trace_event` 是这次最大的单笔债

子代理在 tracing 任务里连 `base/trace_event`、`base/tracing`、`base/test/tracing`
一起删了（超出了指派的四个目录）。理由技术上成立，但残留面是：

```
仍在 #include "base/trace_event/..."     1,006 个文件
仍在调用 TRACE_EVENT* 宏               4,891 处 / 879 个文件
   ui 1064   content 770   gpu 665   third_party 624
   cc 543    components 507   net 287   base 216
```

**比 V8 那 852 个文件还大**，而且性质更糟：`ScriptState` 集中在 blink 一侧，
`TRACE_EVENT` 是撒遍全树的宏，`cc/`、`gpu/`、`ui/` 这些**要留下的渲染核心**里就有 2,272 处。

**已解决——恢复了，见第 7 节。** 当时我的倾向是「只删 perfetto 后端、留下
`base/trace_event`」，理由是 Chromium 支持 `enable_base_tracing=false` 让
`TRACE_EVENT` 编译成空。**这个前提在这个版本上是错的**：upstream 提交
`4c826fa5fd52 [tracing] Remove enable_base_tracing` 已经把那条配置删掉了，
`base/trace_event` 里有 57 处 perfetto include 且没有 stub 路径。所以 perfetto 也一并回来。

### 4.3 blink 事件代码生成链缺一环

`core_event_interfaces` 靠 `renderer/bindings/scripts/generate_event_interface_names.py`
读 Web IDL 数据库产出 `event_interface_names.json5`，脚本和数据库都随 bindings 没了。

**GN 会检查「声明为输入的生成文件有没有生成者」**（不同于 `sources`，那个只有 ninja 才查），
所以这个洞必须在 `gn gen` 阶段就补。做法是把最后一次生成的产物固化进源码树：

```
out/ShotCheck/gen/.../event_interface_names.json5  (6,599 字节 / 220 行)
  → third_party/blink/renderer/core/events/event_interface_names.json5
```

内容是一张静态的事件接口名表，签入是正确形态，不是权宜之计。

### 4.4 devtools 协议只切了一半

`blink/public/devtools_protocol` 和 `core/inspector` 里对 `v8_inspector_js_protocol`
的引用已删（协议只从 `browser_protocol.pdl` 生成）。但 `core/inspector` 本身
——blink core 的 probe instrumentation 层，探针撒遍整个 core——还在，
它是 V8ectomy 的一部分，不能在收敛循环里顺手做。

### 4.5 从未编译

自 `f79cc2b74310` 起没有跑过一次 ninja。除上面几条外，C++ 侧还有大量指向已删子系统的
悬空调用点。镜像体积收益目前**全部是未知数**——`size-attribution.md` 的数据是基线二进制的。

镜像体积的收益目前**全部是未知数**——`size-attribution.md` 的数据是基线二进制的，
新的归属要等下一次成功链接。

## 4.6 工具自己造成的三处损伤

工具让机械编辑可复现，但它们不懂 C++，也不懂预处理器。这一轮有三次是**工具的缺陷**
而不是判断错误，记下来是因为它们的症状都指向别处：

1. **`cpp_drop_decls.py` 的回溯会吃掉前一个声明。** 遇到注释块时不停，于是从目标声明
   一路退过空行、退过**前一个声明的 `override;` 续行**、退进它的注释。
   `renderer_blink_platform_impl.h` 三处、`browser_main_loop.h` 一处。
   **症状出现在邻居身上**，而且被破坏的声明往往仍能编译成别的东西。
   已修：回溯遇到 `//` 开头的行就停。

2. **`restore_includes.py` 不理解 `#if`。** 它锚定「原文件里的前一条 include」，
   那条恰好在 `#if BUILDFLAG(IS_CHROMEOS)` 里，于是恢复的 include 也进了条件块。
   报错是 `use of undeclared identifier 'proxy_resolver'`，指向 include 下方 37 行处，
   而那条 include 明明就在文件里。

3. **`mojom_dangling_imports.py` 第一版把生成的 mojom 报成悬空。** blink 从 feature 列表
   生成 `runtime_feature.mojom` 和 `origin_trial_feature.mojom`，它们解析到 `<out>/gen`。
   我在文档里断言「不存在这个误报类别」，那句话是错的，害我去 git 历史里找一个从未提交过的
   文件。已修：同时检查 gen 目录。

配套的 `check_conditional_includes.py` 第一版也报了 131 条，其中 126 条误报——
它把头文件的 include guard 也算成条件块，于是 .h 里每条 include 都是「depth 1」。
**一个噪声比信号多 25 倍的检查器等于没有检查器。**

4. **v8ectomy 的清扫把 cppgc 的 include 一起删了。** 清扫按 `#include "v8/..."` 匹配，
   而 cppgc 是从 V8 里 vendored 出来的，blink 恰恰按这个前缀包含它。blink 里 49 条
   cppgc include 被删掉 44 条；活下来的 5 条全都带尾注释（`// IWYU pragma: export`、
   `// nogncheck`），正则锚在行尾，越过了它们。

   这个损伤**在头文件路径修好之前完全看不见**：在那之前每条 cppgc include 都是
   "file not found"，删没删都一样。路径修好之后症状才浮出来，而且换了个面孔——
   9023 条 `no template named 'MakeGarbageCollected' in namespace 'cppgc'`。
   namespace `cppgc` 存在（唯一幸存的 `type-traits.h` 把它带进来了），但里面什么都没有。
   连带 7201 条 `unknown template name 'CustomSpace'`、4942 条 `HeapConsistency`
   不完整类型、2261 条 `AdditionalBytes`——全是同一处损伤的不同投影。

   已按上游原样（`v8/include/cppgc/...`）恢复，见 §7 恢复记录。
   `thread_state.h` 有意排除：它是手写重构过的，改用独立的 `cppgc::Heap` 而不是
   `v8::CppHeap`，include 列表对它现在做的事是正确的。

   教训：**清扫脚本的匹配面必须按「这个前缀下有没有我要留的东西」来划，而不是按目录名。**
   `//v8` 目录被删了，但 `v8/include/cppgc` 这个*包含路径*还活着。

## 5. 工具

全部在 `tools/shot/`，都是这两波实际用过的：

| 工具 | 作用 |
|---|---|
| `size_attrib.py` | 用 PDB section contribution 把镜像字节归属到 GN 目录 |
| `size_cut_groups.py` | 按路径前缀称量候选删除组 |
| `gn_drop.py` | 按精确匹配删除 GN 文件里的列表条目 |
| `gn_drop_target.py` | 花括号配对，删除整个 target 块 |
| `gn_drop_prefix.py` | 按前缀删除已删目录的源文件条目 |
| `gn_autofix.py` | 循环 `gn gen`，删掉 GN 钉出的悬空依赖行；遇到 `import` 停下交人判断 |
| `strip_component.py` | 全树抹除一个已删组件：import、deps、`if (guard) {}`、`#include`、`#if BUILDFLAG()` |
| `gn_dangling_imports.py` | 全树找 `import("//...")` 指向不存在的文件，一次删完 |
| `gn_undefined_vars.py` | 全树找「被读但从未赋值」的 GN 变量 |
| `gn_drop_if.py` | 删掉条件引用已失效变量的 `if` 块 |
| `deps_drop.py` | 按 checkout 路径删除 `DEPS` 条目 |
| `find_unreferenced.py` | 找没有任何外部 GN 引用的目录（**候选**，不是删除清单） |
| `gn_missing_sources.py` | 找 `sources`/`inputs`/`traits_sources` 里已不存在的文件 |
| `blink_core_coupling.py` | 按「引用落在渲染管线里的次数」给 blink core 子目录排序 |
| `blink_cut_core_dirs.py` | 删 blink core 子目录并同时从 `core/BUILD.gn` 摘除两处列表 |
| `vendor_cppgc.py` | 从 V8 checkout 里取出 cppgc，生成 `sources.gni` |
| `restore_includes.py` | 把 `strip_component.py` 从存活文件里删掉的 `#include` 按原位置插回 |

后三个是波次 3 为了打破「GN 一次只报一个错」而写的：每轮 `gn gen` 25 秒，
一个一个撞下去要几十轮。它们把同一类问题一次性扫完。

已知问题：

1. **`strip_component.py` 会误删构建机制。** 它假设「组件消失 ⇒ 引用它的都是死代码」，
   对定义 GN 模板的目录不成立。`components/vector_icons`（提供 `aggregate_vector_icons()`）
   和 `components/proto_extras`（生成 proto 辅助代码）被误删，已回滚，还得手工补回 4 处
   `import()` 和 protobuf 的 visibility 条目。**从引用本身看不出区别。**
2. **`gn gen` 不检查 `sources` 里的文件是否存在**，只有 ninja 在构建时检查。所以删目录后
   残留的源文件条目在首次构建前是不可见的，`gn_drop_prefix.py` 就是为此存在。
   但它**会**检查「声明为输入的生成文件有没有生成者」，见 4.3。
3. **`gn_drop_if.py` 对 `||` 的初版处理是错的**，已修。它原本假设「变量失效 ⇒ 整个条件为假」，
   对 `if (A || X || B)` 不成立。ANGLE 的 `angle_abseil` 因此被内联了错误的 `else` 分支，
   连到 abseil 的 per-container target 上（visibility 只对 abseil 自己开放）而报
   `Dependency not allowed`。现在含 `||` 时只删那一项、保留块。
4. **`gn_undefined_vars.py` 的形态覆盖是逐次补出来的**，最终四种：`"$var"` 字符串展开、
   `if (var)` / `assert(var)`、`x = var` 裸标识符赋值、`[ var ]` 列表里的裸标识符。
   前三版各漏一种，每漏一种就多撞一轮 `gn gen`。它对作用域一无所知，
   所以**只会漏报不会误报**——报出来的一定是真断点。

## 6. 宿主环境

- `gn gen` 会并行为多个 Windows toolchain 变体跑 `setup_toolchain.py`，它们抢写同一个
  `environment.x86` / `environment.x64`，间歇性 `PermissionError`。`gn_autofix.py` 已内置重试。
- `git` 抢 `index.lock` 的真凶是外部的 `git status --porcelain` 轮询。
  `git fsmonitor--daemon` **不持锁，不要杀**。判据见 memory `chromium-repo-git-index-lock`。

## 7. 恢复记录

砍除不是单向的。「能用」优先于「砍得干净」，每一次把删掉的东西放回来都记在这里，
包括当初为什么删、后来为什么必须回来。

| 恢复内容 | 提交 | 为什么必须回来 |
|---|---|---|
| `base/trace_event`、`base/tracing`、`base/test/tracing`、`third_party/perfetto`（187.8 MiB，约 1600 个 target） | `18f71ab7d01e` | `base/check.h` → `base/location.h` → `base/trace_event/base_tracing_forward.h`。全 Chromium 每个翻译单元都包含 `base/check.h`，所以 tracing 在 `CHECK`/`DCHECK` 的依赖链上，不是可选埋点。原以为可以用 `enable_base_tracing=false` 走官方 stub 路径，但 upstream `4c826fa5fd52` 已删除该配置。 |
| `services/cert_verifier`、`components/network_time` | `d7a8cb049bde` | 删除证书验证不该是体积清理的副作用。cert_verifier 依赖 network_time 做有效期检查。等进程模型重做时它会自然消失。 |
| `third_party/protobuf` 的 perfetto 豁免与 visibility 条目 | `18f71ab7d01e` | 见下。 |
| `components/performance_manager/scenario_api`（7 个文件） | 编译期 | `net/disk_cache/sql` 的 `IsBrowserIdle()` 用它判断是否该跑缓存淘汰。`public_deps` 只有 `//base`，恢复成本为零——比猜「没有场景观察者时 `CurrentScenariosMatch` 该返回什么」诚实得多。 |
| `components/url_pattern`（4 个文件） | 编译期 | `services/network` 的 `connection_allowlist` 直接用 `SimpleUrlPatternMatcher`，而 connection_allowlist 被 content/browser 的 56 个文件引用。切掉比恢复贵一个数量级。只牵 `//third_party/liburlpattern` + `//third_party/re2`，两者都在。 |
| `components/services/storage/public/cpp/filesystem` + 同名 mojom（9 个文件） | 编译期 | `third_party/leveldatabase/env_chromium.cc` 的所有文件操作都走 `storage::FilesystemProxy`。`//storage/browser` 被 `content/public/browser` 依赖，所以 leveldb 在图里。闭包只有 base + mojo + 一个 mojom。 |
| blink 里 44 条 `#include "v8/include/cppgc/..."` | 编译期 | 被 v8ectomy 的清扫正则误伤，详见 §4.6 第 4 条。cppgc 就是 Oilpan，`blink::GarbageCollected<T>` 是 `cppgc::GarbageCollected<T>` 的别名——没有它 blink 一个对象都建不起来。恢复用上游原样拼写，配合 `third_party/cppgc/BUILD.gn` 把 vendor 根放进搜索路径，这样这批文件和上游逐字一致，日后 rebase 不用手工对账。 |

| `platform/heap/weak_cell.h` 的实现（原 `gin/weak_cell.h`） | 编译期 | blink 的 `WeakCell`/`WeakCellFactory` 只是 `gin::` 版本的两行别名，而 `gin/weak_cell.h` 除了一句文档注释里的 `v8::Isolate::GetCurrent()->GetCppHeap()`,**通篇没有 V8**——它是 `cppgc::GarbageCollected` 上的一个 `WeakMember` 单元。所以没有恢复 `//gin`,而是把实现搬进 blink 自己的头文件,顺手把两层合成一层(blink 继承它本来就只为了替它找 allocation handle)。 |
| `platform/bindings/frozen_array.h`（原 `bindings/core/v8/frozen_array.h` + `platform/bindings/frozen_array_base.h`） | 编译期 | 上游拆成两个文件只为了隔离 V8：`FrozenArrayBase` 实现 `Wrap()`/`AssociateWithWrapper()`,把内部向量变成一个**冻结的** JS 数组并按 world 缓存 wrapper;模板子类只提供 `MakeV8ArrayToBeFrozen()`。core 里 31 个调用方要的是另一半——一个可 trace 的不可变向量(Element 的 assignedSlot 列表、ResizeObserverEntry 的 box sizes、CSSContainerRule 的 conditions)。所以只留另一半,合成一个头文件。局限已写在文件里:参数从 IDL 类型换成了 blink 类型,`FrozenArray<IDLString>` 会编译失败而不是悄悄做错。 |
| `core/inspector/console_message.{h,cc}` | 编译期 | 它随 inspector 目录一起被删,但它不是 DevTools——它是 blink 各处报告问题时构造的那个数据对象(CSP 违规、解析错误、deprecation、子资源失败),core 里约 180 个调用点构造它,而它**一个 V8 类型都没有**(source、level、字符串、SourceLocation、frame、node id)。四个构造函数回来两个:`WorkerThread` 那个(workers 已删)和 `WebConsoleMessage` 那个(嵌入方 API 已删)没有回来,`DocumentLoader` 那个回来了但丢掉了 request identifier,因为 `IdentifiersFactory` 铸造的是 DevTools 协议 id。恢复它同时是**验收第 3 条的需要**:解释像素差异要有证据,console 消息就是证据。 |

### 7.1 `strip_component.py` 的固有缺陷（第三次踩到）

它假设「凡是提到已删组件的地方，都是对该组件的依赖」。这个假设有三类反例，
每一类都栽过一次：

1. **定义 GN 模板的目录**（波次 2）：`components/vector_icons` 提供
   `aggregate_vector_icons()`，`components/proto_extras` 生成 proto 辅助代码。
2. **策略声明**（本次）：`third_party/protobuf/proto_library.gni` 的豁免名单里
   `"//third_party/perfetto/*"` 的含义是「**不要**对 perfetto 做 import_dirs 检查」；
   `BUILD.gn` 的 visibility 里 `"//third_party/perfetto/gn:protobuf_full"`
   的含义是「**授权** perfetto 访问」。两者都不是依赖。
3. **跨组件的 `#include`**（本次）：347 处 include 被从 281 个存活文件里删掉。
   这一类最难查，因为**报错点和原因离得很远**——
   `base/task/sequence_manager/task_queue.h` 丢了
   `base/tracing/protos/chrome_track_event.pbzero.h` 之后，报的错是
   「`perfetto` 命名空间里没有 `protos`」，指向 perfetto 而不是那个消失的头文件。
   `restore_includes.py` 从删除前的版本按原位置插回。

**从引用本身看不出区别**，这一点三次都成立。

## 8. V8ectomy

### 8.1 度量：V8 在 blink 里的真实分布

`core/` + `controller/` 里 671 个文件包含 `platform/bindings/` 的头文件，共 858 条
include。按头文件排序后，头两名占了 55%：

| include 数 | 头文件 |
|---|---|
| 287 | `exception_state.h` |
| 185 | `script_wrappable.h` |
| 92 | `script_state.h` |
| 35 | `dom_wrapper_world.h` |
| ≤31 | 其余 30 个头文件 |

按符号计：`ExceptionState` 出现在 500 个文件 2251 行，`ScriptState` 529 文件 1798 行，
`ScriptWrappable` 272 文件 306 行，`v8::Local` 159 文件 597 行。

单看这些数字，V8ectomy 是一次 800+ 文件的改写。但再往下测一层，结论就反过来了：

**`ExceptionState` 全树只有 10 个方法被调用过**，且 `ClearException()` 一次都没有：

| 方法 | 调用点 |
|---|---|
| `HadException` | 478 |
| `ThrowDOMException` | 419 |
| `ThrowTypeError` | 265 |
| `ThrowSecurityError` | 44 |
| `GetContext` / `ThrowRangeError` / `Message` / `ThrowSyntaxError` / `GetIsolate` / `Code` | 39（合计） |

**`DEFINE_WRAPPERTYPEINFO()` 在 core 里出现 482 次**，而真正调用
`GetWrapperTypeInfo()` 的只有 11 个文件。

### 8.2 决策：保 API、掏空实现，而不是改调用方

据此定下这次 V8ectomy 的主路线——**`platform/bindings` 里被删的类型，凡是公开 API 被
大量引用的，都保留名字和签名，只把内部的 V8 实现换掉**：

- `ExceptionState` 变成一个纯记账器：记 `code_` + `message_`，不再构造 V8 异常对象。
  公开方法一个不少（`ThrowDOMException`/`ThrowTypeError`/…/`HadException()`/
  `NonThrowableExceptionState`/`DummyExceptionStateForTesting`/`IGNORE_EXCEPTION`）。
  原来只在 `swallow_all_exceptions_` 时才记录的分支改成无条件记录——因为除了 V8
  没人消费「已抛出」的那个值，那条路径本来就是死的。
- `ScriptWrappable` 变成 `GarbageCollected<ScriptWrappable>` + `NameClient`，
  别的什么都不是。wrapper 缓存、`WrapperTypeInfo`、`ToV8`/`Wrap`/
  `AssociateWithWrapper` 全部删除，**但 `DEFINE_WRAPPERTYPEINFO()` 保留为
  `static_assert(true)` 的空宏**。
- `ExceptionContext` 自带 `enum class ExceptionContextType` 替换 `v8::ExceptionContext`，
  成员名逐个对齐。

这一个决定把 core 的改动面从「500 个文件用 ExceptionState + 482 处宏」压到
**13 个真正需要动的文件**：2 处直接用 `ExceptionState(v8::Isolate*)` 构造
（`css/css_style_declaration.cc`、`dom/document_test.cc`），8 处直接引用
`v8::ExceptionContext::k*`（`dom/observable.cc`、`dom/dom_exception.{h,cc}`），
11 个文件直接调 `GetWrapperTypeInfo()`/`GetStaticWrapperTypeInfo()`。

留空壳而不是留桩：这些类**真的在工作**——`ExceptionState` 真的记录错误码和消息，
`HadException()` 真的反映是否出过错，调用方的错误分支行为不变。变的只是「错误不再
变成一个 JS 异常对象」，而这个引擎里本来就没有 JS 去接。

### 8.3 `SourceLocation` 丢掉的东西（有意的）

`SourceLocation` 保留 url/function/line/column/script_id，删掉
`v8_inspector::V8StackTrace` 的捕获与 `StackTrace()`/`TakeStackTrace()`/
`HasStackTrace()`/`BuildInspectorObject()`。`CaptureWithFullStackTrace()` 保留原名，
但现在只构造一个 unknown location——**没有 JS 在跑，就没有 JS 栈可抓**，
这不是省事，是那个概念在这个引擎里不存在。

### 8.4 `core/script` 不能删，只能变成无操作

`HTMLParserScriptRunner` 是 HTML 解析器直接调用的，删掉解析器就散架。所以
ES module / classic script 的处理方式是**让脚本执行成为一个明确的 no-op**，
而不是把目录删掉。

实际落地的形状（这段决定了「有 `<script>` 的页面还能不能截出图」）：

- `ScriptLoader::PrepareScript()` **一步没少**：type / `nomodule` / `for`-`event`
  检查、`already started`、`src` 校验与 error 事件、render-blocking 登记、
  `GetScriptSchedulingTypePerSpec()` 全部照跑。
- **内联 classic 脚本**：完全不变，准备阶段取源码，立即 ready。
- **外部 `<script src>`：不再发请求。** 字节取回来也没有引擎去编译执行，取了就是下载完
  丢掉。`PendingScript` 直接以 ready 状态创建，所以一个 parser-blocking 的
  `<script src>` 在它阻塞解析器的同一个任务里就把解析器放开，而不是去等一个永远不会到的响应。
- **`GetSource()` 返回空源码，不是 null。** 这个区别是关键：在规范路径里 null 的含义是
  「出错了」，`ExecuteScriptBlockInternal` 会因此给页面上**每一个**脚本发 `error` 事件。
  什么都没失败，所以元素拿到的是 `load`。
- **`<script type=module>`** 归类为 defer，不求值，因此 `load` 事件和 defer 顺序仍然符合规范。
- `IgnoreDestructiveWriteCountIncrementer`、`Push/PopCurrentScript`、
  `DocumentParserTiming` 记账、render-blocking 释放全部保留，只有 `script->RunScript(...)`
  那一行换成注释。

**`DOMContentLoaded` 和 `load` 正常触发，解析器不会停在 `<script>` 上。**

### 8.6 会改变渲染结果的取舍（供像素比对时对账）

这几条是有意的行为变化，将来像素比对出现差异时，先查这里：

| 变化 | 影响 |
|---|---|
| 外部 `<script src>` 不再发起网络请求 | 网络行为变化；页面若靠脚本注入内容，那部分内容不会出现（本来也不会——没有引擎执行它） |
| XML 美化树视图删除 | `TransformDocumentToXMLTreeView` 整个实现在 `documentxmltreeviewer.js` 里、跑在隔离世界。**直接打开一个无样式的 `.xml` URL，渲染结果会和原版不同**：现在走默认 XML 渲染规则（`SetIsViewSource(true)` 仍然生效） |
| MHTML 的声明式 Shadow DOM 重建删除 | 它同样是靠执行一段内联 JS 实现的 |
| `<input pattern>` 改用 ICU 正则 | ICU 不支持 `v` 标志的集合运算记法，用到集合运算的 `pattern=` 现在编译失败因而不施加约束——和原版遇到编译失败时的行为一致 |
| 字体外部内存记账删除 | `SimpleFontData` / `FontCustomPlatformData` 原本把 FreeType/SkData 的分配量报给 V8 驱动堆增长启发式。cppgc 没有对应钩子，**如实删掉而不是伪造一个数字** |
| `performance.mark()` 丢掉 `detail` 字段 | 时间线条目本身不受影响 |
| 任务归因（task attribution）失效 | 装载它的是 `V8PerIsolateData`。`From()` 如实返回 null，所有调用方本来就判空 |

### 8.7 `EventLoop` 的微任务队列是真的重写了，不是摘掉

`scheduler::EventLoop` 原来把微任务塞进每个 EventLoop 自己的 `v8::MicrotaskQueue`。
现在它自己持有一个 `Deque<OnceClosure>` 加重入保护，
`PerformMicrotaskCheckpoint()` 会把队列抽干**并包含抽干过程中新入队的微任务**，
然后跑 checkpoint 结束任务。

这一条不能糊弄：自定义元素、MutationObserver、图片加载和动画都是 `EnqueueMicrotask`
的重度用户，微任务不跑，这些东西就静悄悄地不工作，而症状会是「图差一点」而不是崩溃。

`ArrayBufferContents` 同理，改为通过 Blink 自己的 `ArrayBufferBackingStore`
（`shared_ptr` 后面一块 PartitionAlloc 内存）持有字节，而不是 `v8::BackingStore`。

### 8.5 blink 侧的 WebGPU / WebXR 残留

`//gpu/webgpu`、`//third_party/dawn`、`gpu/command_buffer/client/webgpu_interface.h`
在波次 3 就删了，但 blink **客户端**整套留着：
`platform/graphics/gpu/{dawn_*,webgpu_*}` 22 个文件，加上 WebXR 的
`xr_frame_transport*`（它存在的唯一目的就是把画好的帧经 WebGL 或 WebGPU 交给 XR 设备）。
一并删除，共 27 个文件 3600 余行，并摘掉 `platform/BUILD.gn` 的 `//gpu/webgpu:common`。

`CanvasRenderingAPI::kWebgpu` 这个枚举值**故意留着**：它只可能由
`canvas.getContext("webgpu")` 产生，而能应答这个调用的工厂在 `modules/webgpu`，
那个目录已经不存在了。留着枚举值比改动 `canvas_rendering_context.h` 的
`kMaxValue` 和所有 switch 便宜，且不可达。

## 8.8 恢复还是切除：这一轮沉淀出的判据

进入编译阶段之后，绝大多数错误都归结为同一个选择：**把被删的东西恢复回来，还是把引用它的
代码也切掉。** 反复摇摆几次之后，判据稳定成两条：

**第一条：上游是否已经提供了「这个东西不存在」的表达方式。**
有就用它，不要动代码。

| 情况 | 用的机制 |
|---|---|
| `third_party/webrtc` 不在 checkout 里 | `is_p2p_enabled = false`——上游为 iOS 加的开关，P2P 代码本来就全在 `BUILDFLAG(IS_P2P_ENABLED)` 后面 |
| ffmpeg / libvpx 已删 | `media_use_ffmpeg` / `media_use_libvpx` / `media_use_symphonia = false` |
| 企业缓存加密特性关闭 | 恢复 `components/enterprise/buildflags`（2 个文件），让 flag 存在且为 false |
| DevTools 协议基线不匹配 | `check_protocol_compatibility.py --expected_errors`，逐条列出 |

这比挖掉 `#if` 块正确，因为 `#if BUILDFLAG(...)` **就是**上游为「这个特性关掉的构建」准备的
机制。挖掉它反而丢掉了信息。

**第二条：这个组件是「被误伤的基础设施」，还是「本来就该走的功能」。**

| 恢复的（基础设施） | 为什么 |
|---|---|
| `components/web_package` | Web Bundle 被 network 和 content 的 13+ 个文件引用 |
| `components/os_crypt/async` | `network_context` 的 cookie 加密路径 |
| `services/proxy_resolver/public`（**只有接口层**） | `ProxyLookupClient` 传 `net::ProxyInfo`——那是代理**查询结果**，和执行 PAC 脚本无关。服务实现（真正跑 JS 的部分）保持删除 |
| `components/{cookie_config,domain_reliability,certificate_transparency,url_matcher}` | network / cert_verifier 直接 include |
| `components/payments/mojom`、`digital_goods/mojom`、`media_session/public` | 一度恢复，见下 |

| 切掉的（功能） | 为什么 |
|---|---|
| PaymentRequest、navigator.share、navigator.contacts、getInstalledRelatedApps、手写识别 | 实现都在已删的 `modules/` 里，且只能从 JS 调用 |
| `services/network/p2p` | WebRTC |
| DevTools 录屏 / 媒体编码服务 | 需要 `media/mojo` |

`payments/mojom` 这一组值得单独说：我**先恢复后切除**，因为顺着导入链走到尽头才看见代价——
`payments/secure_payment_confirmation_service.mojom` 导入 `webauthn/authenticator.mojom`，
而后者的 typemap 需要 `device::CredentialType`，那会拖回整个 `//device/fido`。
**为了几个连实现都不存在的 API 恢复一条这么长的链，代价和收益完全不成比例。**
判据本身没变，是我一开始没走到链条尽头就下了判断。

### 8.9 这个 fork 只维护 Windows 配置

`#if BUILDFLAG(IS_MAC)` / `IS_ANDROID` / `IS_CHROMEOS` / `IS_IOS` 里残留的、指向已删组件的
引用**不会被逐一修正**。例如 `render_widget_host_view.h` 在 Mac 分支里 include
`webshare.mojom.h`，而那个 mojom 已经不在任何目标的 sources 里了。

这是明说而不是默认：那些分支在这个构建里不求值，逐一修正它们要动几百处，
而且没有任何东西能验证结果。同理，`services/proxy_resolver` 的 is_mac typemap
被删掉了而 is_win 的保留——**留一条指向不存在目录的路径，是只会在别人日后尝试 Mac 构建时
才炸的潜伏陷阱**，而删掉它至少让失败发生在 GN 阶段。

## 9. 拿不回来的东西，和它们的诚实替代

不是每个缺口都能靠恢复解决。这一节记的是「原件取不到，于是换了个真的能工作的做法」，
以及为什么那不是桩。

### 9.1 `third_party/crc32c` —— 用 leveldb 自带的可移植实现

`third_party/leveldatabase/port/port_chromium.cc` 的 `AcceleratedCRC32C()` 转发到
`//third_party/crc32c`，那是个按 SSE4.2 / ARMv8 CRC32 指令在运行时选实现的库。
它是 **gclient 管理的子模块**（tree 里是 `160000 commit` 的 gitlink），本地没有
`.git/modules`，`git checkout` 只能恢复 chromium 侧那四个文件，恢复不了源码。

改成返回 0。**这不是桩，是 leveldb 自己定义的信号**：
`src/util/crc32c.cc` 的 `CanAccelerateCRC32C()` 拿一个已知缓冲区调它、和期望校验和比对，
不相等就走同一文件里的查表实现。注释原文就是
「port::AcceleretedCRC32C returns zero when unable to accelerate」。
**算出来的校验和完全相同**，差别只在吞吐。

### 9.2 `D3DImageBacking::GetPendingWaitFences()` —— 重建，未经上游对照

这个函数原来的签名带一个 `wgpu::Device` 参数，在 Dawn 清理里被整个删掉，
但两个调用点都活着。子代理照存活的栅栏簿记（`write_fences_`、`read_fences_`、
`d3d11_signaled_fence_map_`、`use_cross_device_fence_synchronization()`）重建了一个
双参版本。语义是站得住的：

- 上一次写永远要等；
- 写访问额外等所有未完成的读；
- 跨设备时只等**别的**设备签的栅栏——同设备的工作在即时上下文里本来就是串行的。

**但它没有和上游源码逐行对照过**，这里记下来是为了可回溯：如果将来 GPU 光栅路径
出现撕裂或读到半张图，这是第一个该看的地方。另外它返回 `std::optional` 却从不返回
`nullopt`，两个调用点的判空因此是死代码——原版大概在 Dawn 设备栅栏查找失败时返回过
`nullopt`。

### 9.3 只有死调用方的函数，随调用方一起删

不是所有「找不到头文件」都需要恢复。`ui/base/webui` 里三个函数
（`GetI18nTemplateHtml`、`GetWebUiCssTextDefaults`、`AppendWebUiCssTextDefaults`）
需要已删的 `ui/webui/resources` grit 头，但**在整棵存活树里零调用方**——
它们给 chrome:// 页面注入 `loadTimeData` 脚本和默认文字样式，而这个引擎从不加载
chrome:// 页面，也没有脚本引擎去跑注进去的东西。删函数，不恢复资源树。
真有调用方的 `SetLoadTimeDataDefaults()` 和 `AppendJsonJS()` 原样保留。

同理 `ui/` 里整条 headless 屏幕路径（`screen_win_headless`、
`desktop_screen_win_headless`、`hwnd_message_handler_headless`，6 个文件）：
`headless::HeadlessScreenInfo` 定义在已删的 `//headless` 里，而这条路径存在的唯一目的
就是驱动 //headless 声明的虚拟显示器。shot 自带 `ShotScreen`，
`display::Screen::Get()->IsHeadless()` 恒为 false，三个分支点各留一句注释说明
原来那一支做什么、为什么现在不需要。

## 10. IDL 绑定：不是恢复，也不是砍掉，而是重新生成

这是 v8ectomy 里最大的一次判断，值得单列。

### 10.1 问题：blink core 用生成类当自己的类型词汇

blink 的 IDL 枚举、联合、字典**不是绑定层的内部细节**，它们就是 core 自己的类型：

```cpp
if (play_state == V8AnimationPlayState::Enum::kFinished)   // core/animation
timing.fill_mode = V8FillMode(V8FillMode::Enum::kBackwards);
FocusOptions* options = FocusOptions::Create();            // core/dom
const RangeBoundary* rangeStart();                         // = V8UnionStringOrTimelineRangeOffset
```

`third_party/blink/renderer/bindings/` 随 V8 一起被删，于是 core 失去了 262 个类型。
错误面是这样的（第 47 轮）：

```
1138  unknown type name 'V8TrustedType'
 542  use of undeclared identifier 'V8CompositeOperation'
 237  incomplete type 'blink::FocusOptions'
```

### 10.2 为什么不能砍

「砍掉用到它们的功能」意味着掏空 core animation、canvas、typed OM、DOM options bag，
而这些恰恰是截图必须算对的东西。成本远高于收益。

### 10.3 为什么可以重新生成

**`.idl` 文件还在**——光 `renderer/core` 下就有 690 个，它们正是被删掉的生成器读的输入。
所以可以用同一份输入，生成同一批类，只去掉「和 JavaScript 引擎说话」的部分：

| 去掉 | 留下 |
|---|---|
| `Create(v8::Isolate*, v8::Local<v8::Value>, ExceptionState&)` | `Enum` / `ContentType`、字符串表、构造、`AsEnum()` |
| `ToV8()` / `NativeValueTraits` / `ToV8Traits` | `Is*()` / `GetAs*()` / `Set()` |
| `DictionaryBase` 的 `FillValues` 三件套 | `has<X>()` / `<x>()` / `set<X>()`、`.idl` 里写的默认值 |

三个生成器都在 `tools/shot/`：`gen_idl_enums.py`（68 个枚举）、
`gen_idl_unions.py`（67 个联合 + 24 个 typedef 别名）、
`gen_idl_dictionaries.py`（130 个字典）。
`gen_idl_build_gni.py` 扫描输出目录写出 GN 源文件列表。

### 10.4 命名规则是**对着调用点验出来的**，不是猜的

这是这一节最重要的一句。三个生成器都有 `--check`：

- 枚举：收集 core/platform 里出现的每一个 `V8Foo::Enum::kBar`，断言生成器产出它。
  **171 个全中。** 这条检查直接推翻了我一开始的假设——规则不是「缩写大写」，
  调用点写的是 `kRgbaFloat16`、`kRec2100Hlg`、`kSharedStorageSelectUrl`，
  只有 `srgb` 保持大写（`kSRGBLinear`）。
- 联合：上游生成器本身在基线提交里还能取到
  （`git show c0bba1026178:third_party/blink/renderer/bindings/scripts/…`），
  规则从那里读出来，再和调用点核对。生成的 64 个文件名里 **62 个与上游逐字符相同**，
  0 个冲突。另外校验了 `ContentType::k…` 和 `GetAs…()` 在 159 个使用文件里全部命中。

**一个没有对照过调用点的生成器，产出的是「看起来对」的代码。**

### 10.5 头文件循环：为什么生成 `.h` + `.cc` 对

第一版把成员类型的头文件直接 include 进联合/字典的头里，结果是：

```
css_numeric_value.h -> v8_typedefs.h -> v8_union_cssnumericvalue_double.h -> css_numeric_value.h
```

include guard 赢了这场比赛，`CSSNumericValue` 在联合内部是不完整类型，
报错是 `use of undeclared identifier 'CSSNumericValue'` —— 指向生成代码，
看起来像生成器 bug。

上游的做法（对照 `css_numeric_value.h` 确实 include `v8_typedefs.h` 得到印证）是：
**头文件前向声明，`Trace()` 出行到 `.cc`**，因为只有 `visitor->Trace(Member<T>)` 需要 T 完整。
生成的 197 个 `.cc` 归进 core 组件本身（`blink_core_sources("bindings_core_v8")`），
不是独立 target——它们 include core 的头，core 又 include 它们，任何别的安排都是依赖环。

### 10.6 三个只有脚本能调用的入口，随脚本一起删

- `LocalDOMWindow::requestAnimationFrame(V8FrameRequestCallback*)` 和 `V8FrameCallback`
  适配器。`FrameCallback` 和整个 collection 留着——blink 自己也往里排动画帧任务。
- `UniversalGlobalScope::queueMicrotask(VoidFunction)`。微任务队列非常活跃，
  但这个入口的存在意义是排入一个脚本函数。
- `ReportingObserver` 的回调。类本身留着（`ReportingContext` 持有观察者列表，
  deprecation / intervention / CSP 都往里塞报告），但 `ReportToCallback()` 现在
  只是把队列清空——**这是诚实的，不是桩**：没有回调可调，报告确实无处可去。

## 11. 「不能恢复」和「不该恢复」是两件事

第 8.8 节的判据在这一轮被反复用到，沉淀出一条更清楚的分界。碰到一个引用了已删东西的
文件，先问的不是「这个功能要不要」，而是**「它到底依赖 V8 的什么」**：

| 依赖的是 | 结论 | 例子 |
|---|---|---|
| 只是**住在**被删的目录里 | 恢复，一行不改 | `platform/bindings/transform_view.h`（零 V8 引用）、`core/inspector/dev_tools_emulator`、`core/route_matching`、`platform/mhtml` |
| 生成器产出的类，但**输入还在** | 重新生成 | IDL 枚举 / 联合 / 字典，见第 10 节 |
| V8 的**运行时服务**，但有等价物 | 换实现，写清楚差异 | `core/url_pattern` 的 `ScriptRegexp` → ICU 的 `URLPatternRegexp`；差异是 `v` 标志的集合记法不支持，走原有的「自定义正则无效」路径 |
| V8 的**数据结构本身** | 不能恢复，只能切 | `AdScriptIdentifier` = `v8_inspector::V8DebuggerId` + v8 script id；`ScriptValue` / `record<USVString, any>` |
| 只有**脚本**能触发的入口 | 切入口，留机制 | `requestAnimationFrame(callback)` 切了，`FrameCallback` 和整个 collection 留着 |

**`ActiveScriptWrappable` 是这条分界最容易搞错的一个。** 名字里有 Script，住在
`bindings/core/v8/`，看起来是 V8 的东西——实际上它零 V8 引用，纯 cppgc：
「这个对象有未完成的活动，GC 时别回收它」这个问题，有没有脚本引擎都成立，
core 里 37 个类继承它。同理 `ActiveScriptWrappableManager`。

反过来，`core/sanitizer/` 看起来非切不可（它在 HTML 解析器里，`html_construction_site.cc`
有 11 处引用），实际 `sanitizer.h` 和 `sanitizer_api.h` 里**一个 `v8::` 都没有**，
只有 `ExceptionState`（还在）和一堆 `V8Union*`（生成器能产出）。之前两次跳过它是因为
我数 grep 命中数时把 `V8Union` 和 `ExceptionState` 一起算进了「V8 引用」——
**数字段名比数 grep 命中可靠。**

### 11.1 `&& false` 是不诚实的替代

早先某一轮为了中和「跨 script world」检查，把条件改成 `... && false`。这不是
「最小实现」，是把死代码留在原地：`-Wunreachable-code-return` 直接报错，而且下一个人
读到的是一个看起来还在工作的检查。正确写法是把整段删掉，留一句话说明
「这里比较请求方和缓存方的 DOMWrapperWorld，而这里只有一个世界的空无」。

### 11.2 遥测一律切，且要说清留下的是什么

`DocumentResourceCoordinator` / `RendererResourceCoordinator` 是 blink 向浏览器
performance manager 汇报的通道。切掉时有三处需要判断而不是删除：

- `OriginTrialContext` 的 use counter 留着，只有「别冻结这个 browsing context group」
  的汇报没了——那句话对**会冻结文档的浏览器**才有意义。
- `PaintTiming::OnFirstContentfulPaint()` 留着，那是 blink 自己的记账，
  **截图就是等它**；走掉的只是 IPC。
- `MainThreadMetricsHelper` 的 `main_thread_task_load_state_` 留着，因为下面的直方图按它分桶。

这三处如果连同删掉，会分别丢掉一个计数器、一个截图时序信号、一个直方图——
**「这是遥测」不等于「整段可以删」。**

## 12. 悬空的「埋点族」：报错点离原因很远的第二种形状

§7.1 记的是「跨组件 include 被删,报错指向别处」。这一轮出现的是它的近亲:
**一整族宏或数据写入函数的定义随目录消失,而调用点散落在 core 各处**。

三族,全部来自 `core/inspector` 的 DevTools timeline 埋点:

| 族 | 调用点 | 定义原处 |
|---|---:|---|
| `TRACE_SCHEDULE_STYLE_INVALIDATION` / `TRACE_STYLE_INVALIDATOR_INVALIDATION*` | 26 处,4 个文件 | `core/inspector/inspector_trace_events.h` |
| `DEVTOOLS_TIMELINE_TRACE_EVENT*`（4 个变体） | 30 处,16 个文件 | 同上 |
| `inspector_*_event::Data/BeginData/EndData` | 12 处,8 个文件 | `core/inspector/inspector_trace_events.cc` |

处理方式**三族并不相同**,分别是:

1. **宏调用整体删掉**。没有把宏 `#define` 成空——空宏会让每个调用点读起来
   仍像在埋点,而且参数里的类型可能自己也已经被删。
2. **两个本地 `_IF_ENABLED` 包装宏**(`invalidation_set.cc`、`style_invalidator.cc`)
   删掉整个 `#define`。它们的宏体是 `if (...) [[unlikely]] <被删的宏>`,
   只删宏体会留下一个没有语句的 `if`。
3. **`TRACE_EVENT` 本身保留,只摘掉 payload**。`TRACE_EVENT_BEGIN("devtools.timeline",
   "Layout", "beginData", lambda)` 里,事件是 perfetto 的、活着的、有用的,
   丢失的只有 lambda 里那个写 DevTools 字段的函数。所以变成
   `TRACE_EVENT_BEGIN("devtools.timeline", "Layout")`。
   `paint_layer_painter.cc` 尤其如此:`PaintTimelineReporter` 存在的意义就是维护
   begin/end 配对,删掉 begin 会留下一个不配对的 end。

工具:`tools/shot/cpp_drop_macro_calls.py`。它按括号配对走完整个调用,
而不是按行匹配——这些是跨 3–8 行的语句宏,删第一行会把参数留在原地,
错误出现在三行之后。

### 12.1 一个生成器 bug,和它为什么躲过了 `--check`

`gen_idl_enums.py` 把 IDL 里的空字符串枚举值拼成 `kEmptyString`。看起来对。
上游的拼法是**裸的 `k`**——证据是基线上游自己的
`core/html/track/vtt/vtt_region.h`:

```cpp
V8ScrollSetting::Enum scroll_ = V8ScrollSetting::Enum::k;
```

`html_media_element.cc` 的 `V8CanPlayTypeResult` 和 `vtt_cue.cc` 的
`V8DirectionSetting` 拼法一致,全树 `Enum::kEmptyString` 出现 **0 次**。
规则其实很朴素:生成器把值切成词、驼峰化、前面加 `k`,空值一个词都没有。

`--check` 本该抓住它,却没有——它的正则是
`V8[A-Za-z0-9_]+::Enum::k[A-Za-z0-9_]+`,**要求 `k` 后面至少一个字符**,
于是 `::Enum::k;` 根本不在被检查的集合里。已改成 `*`。

这是记忆里那条的第二个实例:*生成器必须对着调用点验命名,推断出来的规则会产出
「看起来对」的代码*。第一次是枚举里的缩写(`kSRGBLinear` 只有 srgb 大写),
这一次是空值。两次都是「合理推断」输给了「实际拼写」。

## 13. 自研 IDL 生成器的五个 bug,和它们共同的形状

§10 记的是「为什么要重新生成」。这一节记的是**生成器本身错在哪里**。一轮之内抓到
五个,形状高度一致:**生成器做了一个合理的推断,产出了看起来对的代码,而错误出现在
离原因很远的地方**。

| # | bug | 症状出现在哪 | 真正的原因 |
|---|---|---|---|
| 1 | 空字符串枚举值拼成 `kEmptyString` | `vtt_region.h`:`no member named 'k' in 'V8ScrollSetting::Enum'` | 上游拼作裸的 `k`。`--check` 的正则 `k[A-Za-z0-9_]+` 要求 `k` 后至少一个字符,于是 `::Enum::k;` 根本不在被检查集合里 |
| 2 | `partial interface` 抢注了接口名 | `html_all_collection.cc`:union 的 `kElement` 分支参数类型是 `ElementComputedStyleMap*` | `[ImplementedAs=ElementComputedStyleMap] partial interface Element` 排序在 `core/dom/element.idl` 前面。**partial 只贡献成员,不定义这个名字是什么**,它的 `ImplementedAs` 说的是那些成员的实现类 |
| 3 | `Member<T>` 用前向声明 | libc++ `is_base_of.h`:`incomplete type 'V8UnionCSSNumericValueOrString' used in type trait expression`,离真正的成员五层 note | 裸 `Member<T>` 可以,`HeapVector<Member<T>>` 不行:实例化 vector 会实例化 `Member<T>::kAffinity`,其初值 `ThreadingTrait<T>::kAffinity` 要在偏特化之间选择,而那个约束是 `std::derived_from<T, blink::Node>` |
| 4 | 修 #3 时把环绕回来了 | 1452 份 `use of undeclared identifier 'V8SanitizerAttribute'`,而报错的头文件**明明 include 了定义它的头** | 让 union include 生成的字典后,链路成了 `v8_typedefs.h → union → 字典 → v8_typedefs.h`(第二次进被 include guard 挡住),于是用到别名时它还没定义。解法:**生成的文件一律不引 `v8_typedefs.h`,直接写 union 的真名和真头文件** |
| 5 | 字典体用正则取,`= {}` 把它截断 | `fragment_parser.cc`:`no member named 'runScripts' in 'SetHTMLUnsafeOptions'` | `\{(.*?)\}\s*;` 在 `sanitizer = {};` 的 `};` 处就收尾了,`runScripts` 被静默丢掉。**而这个生成器的模块注释恰好承诺过「不会悄悄少一个字段」** |

外加一个不是 bug 但是缺口:上游给每个字典成员都生成 `getFooOr(fallback)`,
调用点在「缺省 == 显式给了这个值」时会用它
(`shadow_root_init_dict->getSerializableOr(false)`)。少生成它,
报错读起来像「字典少了个成员」,而不是「访问器少了一个」。

### 13.1 教训

三条,按可操作性排:

1. **对着调用点验,不要对着规范验。** #1 和 #2 都是「合理推断」输给「实际拼写」,
   而两次的证据都在基线上游自己的源码里,`git show c0bba1026178:<file>` 就能拿到。
   `--check` 是这套生成器里最有价值的部分——但它只在**正则覆盖到**的地方有价值,
   #1 就是被自己的正则漏掉的。
2. **生成器之间的 include 图必须是无环的,而且这件事要能一眼看出来。** #3→#4 是
   我在一小时内自己制造的:先说「只有 Trace 需要完整类型,所以前向声明」,
   再说「HeapVector 需要完整类型,所以 include」,两条都对,合起来成环。
   现在的不变式写得下来:**生成的头文件只 include 生成的头文件和 platform 的基础
   设施,永远不 include `v8_typedefs.h`,永远不 include 非生成的 blink 实现头。**
   前两条保证无环,第三条保证不和 core 的头文件互相纠缠。
3. **正则解析 IDL 的地方要挨个问「这个符号会不会出现在字面量里」。** #5 是括号,
   之前还栽过一次注释(`"http://..."` 里的 `//` 被当成注释,吃掉了默认值和下一个
   成员声明,报错点在真正的损坏上方三行)。两次都是同一类。

## 14. 换掉 `main()`:不用发明,树里已经有一个没有 //content 的 Page

这是通往 50 MB 的最大一步,而它的风险比看上去小得多——**Blink 里已经存在一条
完全不经过 //content 的渲染路径,而且一直在生产里跑**:`SVGImage`。

一张 SVG 图片是一份完整的文档,要走 DOM → CSS → 布局 → 绘制,但它不能有自己的
渲染进程、合成器或窗口。所以 Blink 给它建了一个孤立的 Page:

`core/svg/graphics/isolated_svg_document_host.cc`:

```cpp
page = Page::CreateNonOrdinary(chrome_client, agent_group_scheduler,
                               inherited_color_maps);
page->GetSettings().SetScriptEnabled(false);          // 本来就不执行脚本

frame_client_ = MakeGarbageCollected<LocalFrameClient>(this);  // : EmptyLocalFrameClient
frame = MakeGarbageCollected<LocalFrame>(
    frame_client_, *page, nullptr, nullptr, nullptr,
    FrameInsertType::kInsertInConstructor, LocalFrameToken(), nullptr,
    /*InterfaceRegistry=*/nullptr, mojo::NullRemote());
frame->SetView(MakeGarbageCollected<LocalFrameView>(*frame));
frame->Init(/*opener=*/nullptr, DocumentToken(),
            /*policy_container=*/nullptr, StorageKey(),
            ukm::kInvalidSourceId, /*creator_base_url=*/NullUrl());

frame->ForceSynchronousDocumentInstall(AtomicString("image/svg+xml"),
                                       *data, base_url);
```

然后 `core/svg/graphics/svg_image.cc` 出像素,就两行:

```cpp
view->UpdateAllLifecyclePhases(DocumentUpdateReason::kSVGImage);
return view->GetPaintRecord(cull_rect);
```

注意这里面**没有的东西**:没有 ContentMain,没有 RenderProcess,没有
RenderFrameHost,没有导航,没有 aura WindowTreeHost,没有 ui::Compositor,
没有 viz,没有 cc 合成器,没有 GPU,没有沙箱。InterfaceRegistry 传 `nullptr`,
BrowserInterfaceBroker 传 `mojo::NullRemote()`。

### 14.1 shot 要写的东西

| 需要 | 从哪来 | 状态 |
|---|---|---|
| `ChromeClient` | `EmptyChromeClient`(`core/loader/empty_clients.h:92`) | 树里有,上游自己的最小实现 |
| `LocalFrameClient` | `EmptyLocalFrameClient`(同文件 :304) | 同上 |
| `AgentGroupScheduler` | `MainThreadScheduler::CreateAgentGroupScheduler()` | 树里有 |
| `Page` + `LocalFrame` + `Document` | 上面那段,逐字照抄 | — |
| 装载 HTML | `LocalFrame::ForceSynchronousDocumentInstall("text/html", data, url)`(`local_frame.h:593`) | 树里有,**同步、不经网络栈** |
| 布局+绘制 | `LocalFrameView::UpdateAllLifecyclePhases()` + `GetPaintRecord()`(`local_frame_view.h:648`) | 树里有 |
| 光栅 | `SkSurface::MakeRaster` → `canvas->drawPicture(record)` → `SkImage` → PNG | Skia,CPU |

**唯一要新写的是子资源加载**:`ForceSynchronousDocumentInstall` 只装主文档,
`<img>`/`@font-face`/`<link rel=stylesheet>` 仍然走 `ResourceFetcher` → `URLLoader`。
shot 需要一个只认 `file:` 和 `data:` 的最小 loader factory。这是**真实实现**
(它真的去读文件),不是桩。

### 14.2 为什么这同时让验收更容易

验收合同第 4 条要求「PNG 必须来自 Blink 真实的 DOM→CSS→Layout→Paint→Raster」。
现在这条路径上,从 `ForceSynchronousDocumentInstall` 到 `GetPaintRecord` 之间
**没有任何东西可以作弊**——中间没有第二个渲染器、没有截图 API、没有窗口抓屏。
`PaintRecord` 就是 Blink 绘制阶段的输出本身,`drawPicture` 只是把它回放到
一块 CPU 位图上,和合成器回放到 tile 上是同一份数据。

Chromium 自己的打印和 paint preview 走的也是 `PaintRecordBuilder` +
`LocalFrameView::PaintOutsideOfLifecycle()`,是同一条路的另一个入口。

## 15. //content 拔掉之后:实测的依赖闭包,和下一批体积目标

§14 记的是设计。这一节记的是**做完之后量到了什么**。

### 15.1 构建图

| | 构建 62(有 //content) | 构建 64(无 //content) |
|---|---:|---:|
| ninja edge 数 | 3227 | **2113** |
| 失败 edge | 805 | 202 |
| 其中在 //content | 383 | **0** |

`shot/` 从 24 个文件变成 7 个:删掉了 `shot_content_{client,browser_client,
renderer_client,main_delegate}`、`shot_browser_{context,main_parts}`、
`shot_controller`、`shot_main`、`shot_screen`、`shot_window_tree_host`。

```
$ gn desc out/Shot //shot:shot deps --all | wc -l
3117
$ gn desc out/Shot //shot:shot deps --all | grep -c '^//content'
0
```

### 15.2 闭包里还剩什么(按 target 数)

| 前缀 | targets | 备注 |
|---|---:|---|
| `third_party/*` | 1351 | 见下 |
| `services/*` | 552 | 其中 **network 428** —— blink 的 public mojom 大量 import 它 |
| `ui/*` | 405 | |
| `components/*` | 170 | |
| `gpu/*` | 109 | |
| `mojo/*` | 63 | |
| `net/*` | 56 | |
| `cc/*` | 53 | 其中真正要的是 `cc/paint` |

third_party 里值得点名的:

| | targets | 基线镜像占用 |
|---|---:|---:|
| perfetto | 276 | 2.31 MB(**故意保留**,在 CHECK 的依赖链上) |
| ANGLE | 42 | 13.50 MB |
| SwiftShader | 28 | 含在 GPU 栈里 |
| SPIRV-Tools | 13 | 同上 |
| Vulkan | 12 | 同上 |
| ICU | 10 | 13.44 MB(其中 icudtl 10.8 MB 是数据) |

### 15.3 下一批目标,按「字节 / 工作量」排

1. **GPU 图形栈(ANGLE + SwiftShader + Vulkan + SPIRV,基线 ~29 MB)。**
   shot 用 Skia 在 CPU 上光栅化,`GetPaintRecord()` → `SkiaPaintCanvas` →
   `SkSurface`,全程不碰 GL 或 Vulkan。它们是从 blink/platform 的 WebGL / canvas
   图形依赖进来的,而 WebGL 早就删了。这是一次**依赖图**的裁剪,不是代码裁剪。
2. **`is_official_build=true` 实测**(`build/args/shot-official.gn` 已备好)。
   见 §模型说明:336 MB 基线是 DCHECK 全开、无 LTO 的非官方构建。
3. **ICU 数据过滤**(10.8 → 2 MB 量级)。要实测:换一套预编译数据跑语料做像素 diff。
4. **`services/network` 428 个 target**。绝大多数是 mojom 定义,blink 的 public
   mojom import 它们。真正链进二进制的是一小部分,但 GN 图规模影响构建时间。

**方法论提醒**(§模型说明的重复,因为它反复被违反):删代码只为「编译不过」,
不为「体积」。体积由链接图回答。上面第 1 和第 4 条都是**改依赖边**,不是删源码。


## 16. 从「链接不了」到「像素级可对照」:一次完整的落地

§14 是设计,§15 是拆完 //content 之后的图。这一节记的是**把它真正跑起来**的全过程,
以及最后与未裁剪 Chrome 的逐像素对照结果。

### 16.1 链接:一次探针拿到全集,而不是一轮一轮撞

编译在构建 69 全部通过(8534/8535 个 edge),唯一失败的是链接。**没有**用「改一处、
重构建一次」的方式去撞链接错误,而是手工重跑了一次 lld-link,加 `/errorlimit:0`:

```
$ ninja -C out/Shot -t commands shot.exe | tail -1        # 拿到链接命令
$ ... lld-link.exe /errorlimit:0 ... @./shot.exe.rsp      # 手工重跑
```

一次拿到 **61 个未定义符号 + 3 个重复符号**,归成八类。这是本轮最重要的方法:
链接错误的集合是有限且可枚举的,不该用构建当发现工具。

| 类 | 数量 | 处置 |
|---|---:|---|
| Media Foundation(`mf_helpers` 把 `AudioDecoderConfig` 映射到 `MFAudioFormat_*` GUID,把 D3D11 纹理交给 `gpu::SharedContextState`) | 49 | **删 target 和 7 个源文件**。shot 不解码音频、没有 GPU 栈,目录外无人 include(唯一另一个调用方 `content/gpu` 不在图里)。没有加 `mfuuid.lib` —— 那会链进一个没人调用的解码器 |
| perfetto trace processor(`base::TraceLog` 把 tracing 会话转成 legacy JSON 用的 SQLite 查询引擎) | 5 | `build/args/shot.gn` 里 `use_perfetto_trace_processor = false`。代码本来就在 `BUILDFLAG(USE_PERFETTO_TRACE_PROCESSOR)` 里,这是上游提供的开关,不是改 `trace_log.cc` |
| `policy_value_mojom_traits.cc` 被编进两个 target | 3(重复) | 从 `blink/common` 的 sources 里删。它的两个邻居(UseCounterFeature、UserAgentMetadata)都只在 typemap 的 `traits_sources` 里出现一次 |
| origin_trials 的三个手写文件 | 3 | **恢复**。生成的 `origin_trials.cc` 只覆盖 `runtime_enabled_features.json5` 能表达的部分;`persistent_origin_trials.cc`、`navigation_origin_trial_features.cc`、`manual_completion_origin_trial_features.cc` 承载它表达不了的三张表,而 `trial_token.cc`、`trial_token_validator.cc`、`origin_trial_context.cc` 都还在调 |
| `active_script_wrappable.cc` | 1 | **恢复**。目录叫 bindings/core/v8,里面没有一点 V8:一个函数回答「这个 ExecutionContext 销毁了吗」,放在 .cc 里只是为了让头文件不必 include `execution_context.h` |
| `HTMLFormElement::NotifyEmailVerificationTokenFieldChanged()` | 1 | **恢复**。它让表单里每个 email input 重画验证指示器 —— 是表单渲染。被 V8 那一波误伤,六个调用点还在 |
| `TrackEvent::Trace()` | 1 | **恢复**。`track_` 是 `Member`,少了 `Trace()` 不只是少一次记账,是垃圾回收器看不见它;头文件还声明着 override,虚表槽后面是空的 |
| `SkPngEncoder::Encode` | 1 | shot 自己的 bug,见 16.3 |

### 16.2 运行期:九个缺陷,同一个形状

二进制链接出来之后一次也跑不起来。九个缺陷里有八个是**同一个缺陷**:V8 和 //content
过去承担某项进程级初始化,它们被拔掉之后没人接手。每一个都不自报家门 —— 全是空指针
解引用或者离原因很远的 CHECK。

| # | 崩溃点 | 真正的原因 |
|---|---|---|
| 1 | `ThreadState::ThreadState` | 构造函数里分配了 `ActiveScriptWrappableManager`。`MakeGarbageCollected<>` 要向 `ThreadStateStorage` 要分配句柄,而 storage 是在构造函数**返回之后**才拿到这个 ThreadState 的。分配挪到四条 `Attach*` 路径里,紧跟在注册之后 |
| 2 | `GCInfoTable::RegisterNewGCInfo` → `absl::Mutex::lock` | 没人调 `cppgc::InitializeProcess()`。它建的是全进程 GCInfoTable —— 每个 `MakeGarbageCollected<T>` 查 T 的 trace/finalize 回调的地方。有 V8 时是 `v8::V8::InitializePlatform()` 做的 |
| 3 | `VisualViewport::VisualViewport` | shot 走 `CreateMainThreadAndInitialize()`,它装的是 `SimpleMainThreadScheduler`,而它的 `CreateAgentGroupScheduler()` **直接 return nullptr**。Page 把空指针存进 `Member<AgentGroupScheduler>`,VisualViewport 在 Page 构造函数还没结束时就向它要 compositor task runner。改走渲染器真正的路径:`Platform::InitializeBlink()` → `WebThreadScheduler::CreateMainThreadScheduler(MessagePump)` → `blink::Initialize()`。顺带去掉 `SingleThreadTaskExecutor` —— 调度器自带 SequenceManager,一个线程上不能有两个 |
| 4 | `LocalFrame::Init` | `mojo::core::Init()` 没调。`LocalFrame::Init()` 建空 `PolicyContainer`,那是 `AssociatedRemote`,那是消息管道 |
| 5 | `Document::OpenForNavigation` → `DefaultLanguage` | `ShotPlatform` 没覆盖 `DefaultLocale()`。基类返回空 `WebString`,`InitializePlatformLanguage()` 把它变成 null `AtomicString`,第一个解码的文档就解引用了空 `StringImpl`。答 `en-US` 是诚实的:这个二进制只带 en-US 的 pak |
| 6 | `SkBlurMaskFilterImpl::filterRRectToNine` | `base::DiscardableMemoryAllocator` 没实例。skia 把模糊圆角矩形(CSS box-shadow)的九宫格缓存在 `SkResourceCache` 里,而它经这个分配器分配。用 `DiscardableSharedMemoryManager` —— 浏览器进程给自己用的那个真实现,只有 IPC 那半边(`Bind()`)用不到 |
| 7 | `MemoryConsumerRegistration` | 上面那个管理器要向 `base::MemoryConsumerRegistry` 登记,而唯一的生产实现在 `//content` 里(要 `MemoryConsumerGroupController`)。shot 自己补上契约的另一半:消费者登记、注销,没有协调器来要求释放 —— 这就是一个没人协调的进程里 registry 的**全部**行为 |
| 8 | `base::ThreadPool::PostTask` | 线程池没启。`DiscardableSharedMemoryManager` 在 sequenced task runner 上做记账,blink 的图片解码和字体加载也来自这里 |
| 9 | 出图但内容缺失 | `Render()` 装完文档立刻截图。子资源响应是 posted task 送达的,所以画面是文档生命第一瞬间的样子 |

第 9 条的修法值得单独说:等待循环里**必须跑生命周期**,不能只在循环后跑。`@font-face`
不是因为被解析到才去取,是因为样式解析发现有文字用到它才去取;只 pump 不跑生命周期
会永远等一个从未发出的请求。退出条件是解析完成 + load 事件完成 +
`ResourceFetcher::ActiveRequestCount() == 0`,30 秒超时则**报错**,而不是悄悄输出半张图。

### 16.3 shot 自己的四个 bug

不是缺失的初始化,是我写错了:

1. `SkPngEncoder` 是 skia 的 libpng 编码器,而 chromium 的 `skia/BUILD.gn` 编的是
   `skia_encode_rust_png_srcs`。树里那个 `SkPngEncoder` 是个后面没有实现的头文件。
   改用 `gfx::PNGCodec::EncodeBGRASkBitmap()` —— 知道 skia 到底编了哪个编码器的那层包装。
2. `ShotURLLoader` 用 URL 的 path 组件当文件名。Windows 上 `file:///D:/x/y.png` 的
   path 是 `/D:/x/y.png`(带前导斜杠、带百分号转义),于是所有子资源都指向不存在的文件,
   而主文档因为 main.cc 用的是 `net::FileURLToFilePath` 反而正常。
3. 没有给文档的 origin 授予 `CanLoadLocalResources()`,`SecurityOrigin::CanDisplay()`
   拒绝了每一个 file: 子资源。浏览器里这个标志随导航到来,来自一个判定「这个渲染器可以
   读这个文件」的浏览器进程。shot 没有浏览器进程去判定,而且已经判定了:用户给了它一个
   本地文件去拍照。
4. 把整个响应体塞进 `DidReceiveResponse` 的 `SegmentedBuffer` 参数 —— 那是**后台响应
   处理器**的路径。`ImageResource::AppendData` 明确 CHECK 自己不会被那样调用,因为图片
   没有后台处理器。改成真正的 mojo data pipe(网络服务投递响应体的方式),
   `mojo::DataPipeProducer` 自带分块和可写监视,所以超过管道容量的文件会分多次写完
   而不是被截断。

### 16.4 「看不见」本身是缺陷

前面几个 bug 我一度只能靠猜,原因有两个,两个都修了:

- `EmptyChromeClient` 吞掉所有 console 消息。blink 拒绝一个子资源时**只在这里**说。
- Windows 上 chromium 的 `LOG_DEFAULT` 是 `LOG_TO_SYSTEM_DEBUG_LOG` —— 只写
  OutputDebugString,不写 stderr。一个命令行工具不显式说明,就把自己记的一切都扔了。

现在 console 消息进 stderr,logging 显式初始化到 stderr,`--verbose` 打开逐子资源
请求日志。**一个会静默输出错误图片的截图工具是不合格的**,这不是调试脚手架。

配套的还有 `tools/shot/stack.ps1`:`symbol_level = 0` 不等于没有符号,lld 仍然写
公共符号 PDB,cdb 能给出每一帧的函数名和偏移 —— 够定位启动崩溃,而且**不需要重新构建**
(`symbol_level = 1` 要全树重编)。

### 16.5 blink 的 Settings 默认值不是浏览器的行为

`core/frame/settings.json5` 里的 `initial:` 是「嵌入方还没表态时的安全值」。浏览器
在提交文档的那一刻用 `WebPreferences` 和 `RendererPreferences` 覆盖它们。两个缺口
决定了图对不对:

```
loadsImagesAutomatically   initial: false      ← 任何 <img> 都不会发出请求
FontCache::antialiased_text_enabled_ = false   ← SkFont::Edging::kAlias
FontCache::lcd_text_enabled_ = false           ← 文字完全没有抗锯齿
```

第一个没有任何错误可看,因为**压根没尝试过**。第二个是最大的一处像素差异来源。

处置:

- `ApplyChromeWebPreferences()` 把一个默认构造的 `blink::web_pref::WebPreferences`
  (Chrome 自己的结构体,就在树里)按 `WebView::ApplyWebPreferences` 的方式应用到
  `Settings`,含字体族映射(照抄 `ApplyFontsFromMap`,含日韩脚本码转换)。值来自
  Chrome 的结构体,不是我手写的数字。
- 字体渲染参数用 `gfx::GetFontRenderParams()` **向宿主查询**(Windows 上就是 ClearType
  配置),再喂给 `WebFontRendering::SetAntialiasedTextEnabled/SetLCDTextEnabled` 和
  `skia::LegacyDisplayGlobals::SetCachedParams` —— 就是
  `WebViewImpl::UpdateFontRenderingFromRendererPrefs()` 做的事。
  **查询而不是写死是有意的**:oracle 是这台机器上的 Chrome 用这台机器的字体平滑抓的。
- `SkSurfaces::Raster()` 默认的 `SkSurfaceProps` 声明 `kUnknown_SkPixelGeometry`,
  skia 会在这种 surface 上静默降级 LCD 文本 —— 无论 SkFont 怎么配。光栅 surface 现在
  带 `skia::LegacyDisplayGlobals::GetSkSurfaceProps()`。

### 16.6 对照结果,以及每一处剩余差异的成因

1248×1320,对 Chrome 151.0.7922.138 `--headless --disable-gpu --hide-scrollbars
--force-device-scale-factor=1` 抓的 oracle:

| | 修字体前 | **现在** |
|---|---:|---:|
| 有差异的像素 | 2.998% | **0.371%** |
| 可见差异(>8/255) | 1.582% | **0.013%**(215 / 1,647,360) |
| 均值 | 2.2613 | **0.0078** |
| 最大通道差 | 225 | **85** |

`tools/shot/diff_report.py` + `shot/testdata/regions.txt` 存在,是因为一个整图百分比
分不清「到处的抗锯齿差一档」和「某个元素整个没画」,而这两者要求相反的处置。它按 corpus
的每个特性分别报告,并给出**差异像素的平均水平行程**(抗锯齿接缝是 1,真出问题是区域宽度)
和**逐通道均值**(几何差异三通道相等,颜色路径差异不相等)。

**逐字节完全一致**:标题、文本与行内流、flexbox、grid、网页字体(含 Ahem 方块)、
圆角边框盒、径向渐变、scale 变换、PNG 位图、JPEG 位图。

**剩余差异,全部量化并归因**:

| 区域 | 差异 | 可见 | 最大 | 行程 | 成因 |
|---|---:|---:|---:|---:|---|
| `s4_linear_gradient` | 26.03% | **0%** | **1** | — | 渐变抖动/舍入。四分之一的像素差 **1/255**,三通道均等(0.10/0.10/0.10)。不可见 |
| `s7_gradient_rect` | 5.49% | **0%** | **1** | — | 同上 |
| `s4_box_shadow` | 0.06% | **0%** | **1** | — | 同上 |
| `s5_rotate` | 0.14% | 0.01% | 9 | **1.00** | 斜边抗锯齿接缝。行程恰好 1 像素 = 只有边缘像素 |
| `s5_skew` | 0.16% | 0.03% | 11 | **1.00** | 同上 |
| `s7_triangle` | 4.36% | 0.32% | 41 | **1.00** | 同上(SVG polygon 的斜边) |
| `s7_circle` 描边 | 2.61% | 0.51% | 42 | 1.01 | 曲边抗锯齿接缝。同区域内的 "SVG" 文字是 **0.000%** |
| `s5_3d` 文字 | 11.03% | 8.23% | 85 | 2.72 | **唯一量级较大的残留**。透视变换子树内的文字:字形相对 oracle 平移 **-0.077px / -0.054px**,墨量少 **3.48%**。是次像素定位差异 —— 非仿射变换下字形栅格原点落在分数设备像素上,舍入方式不同 |
| `s5_3d` 几何 | 4.56% | **0%** | **1** | — | 同一个框的四边和填充:最大差 1/255。**几何完全正确**,差的只有里面的文字 |

也就是说,剩下的可见差异 215 个像素里,绝大部分来自**一个透视变换框内的两个字母**,
其余是斜边和曲边上单像素宽的抗锯齿接缝。渐变的 26% 差异是 1/255 的抖动,肉眼和阈值
都判为无差异。

### 16.7 体积:现在有真数字了

```
out\Shot\shot.exe   70,014,976 bytes   66.8 MB   (336 MB 基线的 19.9%)
```

**重申方法论**:这是 `dcheck_always_on`、无 ThinLTO 的开发配置,和 336 MB 基线同口径。
在这个数字出来之前,任何体积预测都是编的 —— 现在它由链接器给出。下一步按 §15.3 走,
但**先出 map file**:链接图里有 ANGLE 不等于二进制里有 ANGLE,`/OPT:REF` + `/OPT:ICF`
早就开着。

---

## 17. 体积构成:字节到底在谁身上

§16.7 只给出了一个总数。这一节把它拆开,因为**下一步砍什么必须由测量决定,不能由
依赖图决定**。上一节结尾写的"链接图里有 ANGLE 不等于二进制里有 ANGLE",这一节把它
从一句警告变成了一个数字 —— 而且结论正好是它警告的那样。

### 17.1 方法:不重链接,读 PDB 的段贡献表

`/MAP` 要重链一次 70 MB 的二进制。不需要:PDB 里的 **section contribution 表**是在
布局定下来**之后**写的,每一条都是最终映像里的一段地址,并标明它来自哪个 `.obj`。
`symbol_level = 0` 就够了 —— 段贡献不是调试信息。

```
python tools/shot/size_report.py out/Shot/shot.exe --depth 3
python tools/shot/size_report.py out/Shot/shot.exe --by-object blink/renderer/core
```

`tools/shot/size_report.py` 从树自带的 `llvm-pdbutil` 读 `--modules` 和
`--section-contribs`,按 `obj/<目录>/<target>/<文件>.obj → <目录>` 归并(target 段丢掉,
它是构建系统的名字,不是源码树里的位置),并且**同一地址只计一次**,避免重叠贡献重复计数。

对账:

```
on disk        70,014,976 bytes   66.77 MB
attributed     69,311,565 bytes   66.10 MB   (99.0%)
unattributed      703,411 bytes    0.67 MB   (导入表/重定位表/资源/段填充/PE 头 —— 
                                              这些是链接器造的,不属于任何 .obj)
```

99.0% 归属率,并且逐段对得上 PE 头里的 raw size。这个对账是有意打印出来的:哪边解析
错了,都会表现成对不上,而不是表现成一个看起来合理的数字。

### 17.2 构成表

| 组件 | 字节 | MB | 占二进制 | .obj 数 |
|---|---:|---:|---:|---:|
| **third_party/blink** | 30,387,637 | **28.98** | **43.4%** | 2,997 |
| **third_party/icu** | 12,167,089 | **11.60** | **17.4%** | 199 |
| **skia** | 5,630,068 | **5.37** | **8.0%** | 818 |
| base(含 partition_allocator) | 2,122,327 | 2.02 | 3.0% | 366 |
| third_party/libjxl | 2,955,160 | 2.82 | 4.2% | ~14 |
| third_party/sqlite | 954,521 | 0.91 | 1.4% | 1 |
| services/network | 640,081 | 0.61 | 0.9% | 241 |
| third_party/harfbuzz | 639,145 | 0.61 | 0.9% | 39 |
| third_party/libjpeg_turbo | 631,768 | 0.60 | 0.9% | 114 |
| third_party/zstd | 579,946 | 0.55 | 0.8% | 21 |
| third_party/libwebp | 545,583 | 0.52 | 0.8% | 87 |
| cc + cc/paint | 849,203 | 0.81 | 1.2% | 111 |
| 其余全部 | — | ~10.9 | ~16% | — |

blink 内部(depth 5):

| | MB |
|---|---:|
| `blink/renderer/core/**` | **24.16** |
| `blink/renderer/platform/**` | 4.71 |
| `blink/public` | 0.86 |
| `blink/common` | 0.27 |

### 17.3 三个结论,每一个都改变了下一步

**(一) GPU 栈已经不在二进制里了 —— `/OPT:REF` 早就干掉了。**

| | 二进制里的字节 |
|---|---:|
| ANGLE | **53,732**(0.05 MB) |
| SwiftShader | **0** |
| Dawn | **0** |
| Vulkan | 2,925 |
| SPIRV | 910 |

§15.3 里"GPU 栈约 29 MB,是最大的一块"这个判断**是错的**,它来自依赖图。真实情况是
ANGLE 仍然**被编译**(从零构建里有几百个 ANGLE 的 `.obj`),但**一个字节都没进二进制**,
因为没有任何活代码引用它。所以砍 ANGLE 省的是**构建时间,不是体积**。这正是 §16.7 结尾
警告的情形,现在有数字了。

**(二) ICU 的 11.60 MB 里,10.73 MB 是 `.rdata` —— 那是嵌进来的数据表,不是代码。**

| ICU | 字节 | MB |
|---|---:|---:|
| `.rdata`(数据) | 11,251,984 | **10.73** |
| `.text`(代码) | 881,383 | 0.84 |

来源是 `build/args/shot.gn` 里的 `icu_use_data_file = false`(为了让 exe 自包含)。
**这是单体最大的一块可裁物**,占整个二进制的 16%,而且它是数据,裁它不需要动一行 C++,
只需要用 ICU 的数据裁剪机制重新生成一份只含截图需要的部分(排序、断行、双向、时区、
转换器里绝大部分都用不上)。裁完必须用 corpus 重新像素对照 —— 因为断行和字形选择真的会
受它影响。

**(三) `blink/renderer/core` 的 24 MB 是"摊平的",没有大头可砍。**

```
最大的单个 .obj    core_generated/longhands.obj        0.60 MB
第二                computed_style_base.obj             0.38 MB
第三                longhands_custom.obj                0.31 MB
...
尾巴(其余 ~2600 个)                                  19.41 MB
```

| | MB | 占比 |
|---|---:|---:|
| 生成代码(`core_generated`) | 1.72 | **7%** |
| 手写代码 | 22.35 | **93%** |

平均每个 `.obj` 约 **8 KB**。这意味着:**core 不可能靠删几个大文件变小,只能靠一个一个
删特性变小,每个特性值约 0.1 MB**。之前隐含假设"CSS 属性表这类生成代码是大头"是错的 ——
生成代码只占 7%。

### 17.4 到 50 MB 的算术

从 66.77 MB 到 50 MB 要拿掉 **16.77 MB**。按测量出来的构成,诚实的账是:

| 手段 | 可得 | 性质 |
|---|---:|---|
| **`is_official_build` + ThinLTO** | **~28 MB**(×0.58) | **构建配置,不是裁剪** |
| ICU 数据裁剪 | ~6–8 | 重新生成数据,需像素复验 |
| libjxl(corpus 不需要 JPEG XL) | 2.82 | 删解码器 |
| sqlite / zstd / libwebp / ced / re2 / libxml | ~2.7 | 逐个查活调用方 |
| blink core 特性删除 | ~0.1 / 个 | 要删几十个才有意义 |

最大的一根杠杆是 **official build**,它一个人就能把 66.8 MB 打到 ~38.7 MB(投影,
系数 0.55–0.6 来自 Chromium 常规经验,**尚未测量**)。所以下一步的顺序是:

1. 用 `build/args/shot-official.gn` 构一次,把那个系数从**投影变成测量**;
2. ICU 数据裁剪(单体最大,且不动 C++);
3. libjxl 及其余第三方解码器;
4. 之后才轮到 blink core 的逐特性删除 —— 它是最贵的,每 0.1 MB 一刀。

**方法论记一笔**:这一节的三个结论有两个推翻了之前基于依赖图的计划(GPU 栈、生成代码)。
依赖图能回答"什么被编译",只有链接器能回答"什么在二进制里",这两个问题的答案差了 29 MB。

---

## 18. 验收:五条标准逐条对账

### 18.1 标准一 —— 从零构建,零错误

之前每一次构建都是增量的,在一个经历过很多轮配置的输出目录里。那会掩盖一整类问题:
早先配置留在磁盘上的生成头文件,可以满足一个 GN 依赖边**已经不存在**的 `#include`。
构建能过,而且只在那个目录里能过。§17 之前的两次"干净构建"其实都是续跑,不算数。

所以清空重来:

```
$ rm -rf out/ShotClean && mkdir -p out/ShotClean
$ printf 'import("//build/args/shot.gn")\n' > out/ShotClean/args.gn
$ ./buildtools/win/gn.exe gen out/ShotClean
$ ./third_party/ninja/ninja.exe -C out/ShotClean shot -j 12 -k 0
...
[15419/15420] CXX obj/shot/shot/shot_renderer.obj
[15420/15420] LINK shot.exe shot.exe.pdb

$ grep -c '^FAILED:' build-scratch.log
0
$ ninja -C out/ShotClean shot -n
ninja: no work to do.

out/ShotClean/shot.exe   69,958,144 bytes
```

15420 个动作全过,0 个 FAILED,0 条 error,复查无残留工作。这次从零构建抓到的两个
依赖边问题(`ax_enums_mojo_blink` 的窄化恢复、web bundle 的切除)记在提交
`b2dcd3f11ab2` 里 —— 那是**增量构建结构上不可能发现**的一类 bug。

### 18.2 标准三 —— 从零构建的二进制,输出逐字节相同

```
$ out/ShotClean/shot.exe --file shot/testdata/render_corpus.html \
      --width 1248 --height 1320 --output shot/testdata/out/shot_clean.png

sha256(shot.png)        b42f1efe5f7f433cebdd9ec5f362d181...   (增量构建产物)
sha256(shot_clean.png)  b42f1efe5f7f433cebdd9ec5f362d181...   (从零构建产物)
```

**相同**。所以 §16.6 那张差异表是这个二进制的属性,不是某个构建目录的属性。对 oracle:
差异 0.3708%、可见 0.0131%(215 像素)、均值 0.0078、最大 85 —— 与 §16.6 完全一致。

### 18.3 一个必须写下来的前提:oracle 的版本不对

| | 版本 | commit position |
|---|---|---|
| 这棵树 | **153.0.8010.0** | `main@{#1680169}`(2026-08-15) |
| oracle | **Chrome 151.0.7922.138**(stable) | 差两个里程碑 |

**所以 0.371% 这个数里混着版本漂移**,它不全是裁剪造成的,也不可能靠改 shot 消除:
没有任何配置能让 153 的树输出 151 的像素。要把版本变量摘掉,需要**同 revision 的
未裁剪构建** —— 官方快照桶里有 `Win_x64/1680171`(离基线 2 个 commit,2026-08-15
11:06 建,353 MB),已下载未使用。

这一步没有做,原因是记在账上比做掉更重要:**当前这张差异表的分母是脏的**,任何"追到
逐字节一致"的努力在换掉 oracle 之前都没有意义。按差异指纹,残留分三类 ——
±1 的抖动类(可见 0%,可关闭)、单像素抗锯齿接缝类(要复刻 cc 的分块光栅和矩阵连乘
顺序才能归零,收益为 0 可见像素,与砍体积的目标直接对冲)、以及 `s5_3d` 一处真差异
(需同版本 oracle 才能定责)。

---

## 19. 按测量出来的构成裁一轮:66.84 → 44.81 MB,压缩后 14.08 MB

§17 把字节拆开之后,这一节按那张表动手。**目标改成了「压缩后 ≤ 15 MB」**——分发的是压缩包,
而 raw 体积只决定装完占多少盘。全部改动一次性下去,一次构建,不再一边裁一边编。

### 19.1 结果

```
                     raw                compressed (BCJ x86 + LZMA2 -9e)
之前   66.84 MB             19.13 MB
现在   44.81 MB (-32.9%)    14.08 MB (-26.4%)      <<< 目标 15 MB
```

`out/ShotSize/shot.exe` 46,866,432 bytes = **44.70 MB = 336 MB 基线的 13.3%**。
分发集只有三个文件:`shot.exe` + `shot_data.pak` + `shot_strings.pak`。`icudtl.dat` 不需要
(数据嵌在 exe 里),构建目录里那些 `vk_swiftshader.dll` / `d3dcompiler_47.dll` / `msvcp140.dll`
全是别的 target 拷进去的,shot 的导入表里一个非系统 DLL 都没有。

**渲染输出 PNG 的 SHA-256 与裁剪前逐字节相同**(`b42f1efe5f7f433c...`)。也就是说这一轮
21 项改动**没有改变任何一个像素**,§16.6 的差异表原封不动成立。

### 19.2 做了什么

全部是 GN args 加一处 ICU 数据换档,**没有删一行 C++**:

| 改动 | 依据 |
|---|---|
| `is_official_build` + `chrome_pgo_phase=0` + `dcheck_always_on=false` | 最大杠杆。`dcheck_always_on` 之前是 **true**,每条 DCHECK 连消息字符串都在二进制里 |
| `optimize_for_size`(`-Os`) | shot 每次启动只光栅一页,时间被进程启动和布局主导,不是内层循环 |
| `enable_backup_ref_ptr_support=false` 等三项 | BRP 是给「进程内跑不可信代码」的 UAF 缓解;没有脚本引擎,威胁模型不存在 |
| `icu_data_dir_override="cast"` | 10.32 → 4.98 MB |
| `enable_jxl_decoder=false` | 实测 2.82 MB 的 Rust crate |
| `enable_device_bound_sessions` / `enable_disk_cache_sql_backend=false` | 这两条才是 net → `//sql` → sqlite 的边,1.06 MB |
| `enable_reporting` / `enable_websockets` / `enable_mdns` / `use_kerberos=false` | 静态截图碰不到 |

投影是 42.8–48.4 MB,实测 44.70,**落在正中间**——§17 那套按段拆分的算法是对的
(关键是把 ICU 的数据 blob 排除在 LTO 系数之外)。

### 19.3 ICU:裁到 1.72 MB 会让 blink 编不出来

先试的是 `flutter_desktop`(1.72 MB,比 cast 再省 3.26 MB)。**blink 自己的构建期工具挂了**:

```
character_data_generator.exe failed with exit code 2147483651   (0x80000003 = STATUS_BREAKPOINT)
```

`character_property_data_generator.cc` 的 `CheckIcuDataResources()` 调
`ulocdata_getCLDRVersion()`,而 `flutter_desktop` 的 `misc` includelist 只留了
`icustd/icuver/likelySubtags`,没有 CLDR 的 `supplementalData`。这个生成器产出的是
**要编进二进制的** `character_property_data.cc` 和 `break_iterator_data_inline_header.h`。

`cast` 是 Chromecast 用的过滤器,也就是**为「blink 渲染网页」写的过滤器**:`misc` 只排除
两项,保留完整断行规则,并且保留 `conversion_mappings` 的 WHATWG html 编码白名单——
所以 `TextCodecIcu` 仍然注册 ISO-8859-2..16 / windows-1250..1258 / KOI8,**没有任何页面
因此解不出来**。这比 flutter_desktop 少省 3.26 MB(压缩后约 0.9 MB),换来零能力损失。

**要用 flutter_desktop 的话**:那个 CHECK 是个探针,返回值直接丢掉,所以不该只把它删掉。
正确做法是把**构建期和运行期的 ICU 数据分开**——生成器是 host 工具,应该始终吃完整的
`common` 数据以保证烤进二进制的 Unicode 表正确,运行时二进制才用裁剪版。代价是
`third_party/icu/BUILD.gn` 里要再产一套 `make_data_assembly`/`icudata` 目标。

### 19.4 关掉开关掀开的三处既有缺陷

这一轮真正花时间的不是裁剪本身,是**改变链接图之后暴露出来的、原本被遮蔽的问题**:

**(一) 两个 `#if DCHECK_IS_ON()` 的 `#else` 分支从来没被编译过。**
`dcheck_always_on=true` 一直走 `#if` 那一半。关掉之后:

- `local_frame_view.cc:5105` 引用 `CoreProbeSink::kInspectorLayerTreeAgent` —— 而这棵树里
  `CoreProbeSink` **一个 `kInspector*Agent` 位都没有**,inspector 随 devtools 一起切了。
  该 disjunct 恒为假,删掉并注明。
- `frame_or_worker_scheduler.cc:97` 的 `IsMainThread()` 未声明 —— 缺
  `platform/wtf/wtf.h`,之前靠 DCHECK 路径上的头文件传递进来。

**(二) blink 一直在白嫖 //net 的依赖。**
`main_thread_scheduler_impl.cc` 直接调
`performance_scenarios::PerformanceScenarioObserverList`,但**全树只有 `net/BUILD.gn:1932`
声明了这条依赖,而且在 `if (enable_disk_cache_sql_backend)` 块里**。关掉 SQL 磁盘缓存,
这条搭便车的边断了,blink 缺的依赖才浮出来(5 个未定义符号)。修法是把依赖加在
**真正使用它的地方**,不是把开关打回去。

**(三) `enable_device_bound_sessions=false` 是个上游没测过的配置。**
`session_event.cc` 在无条件 sources 里(net/BUILD.gn:458),而它无条件使用的
`SessionError` 定义在 1317 行的条件块里。**使用者照编,定义没了,只有链接器发现。**
把 `session_error.cc/h` 挪到无条件块——它只依赖 `base/notreached.h` 和 `refresh_result.h`。

### 19.5 一处恢复

`chrome/VERSION`(4 行)。`is_official_build` 会跑 `compute_build_timestamp.py official`,
它读这个文件,而 `chrome/` 整个目录之前被删了。只恢复这一个文件,`chrome/` 下没有别的东西。

### 19.6 构建并行度:实测,别照搬

「必须 `-j 12` 否则 OOM」这条经验**当初是对的,后来是错的,再后来又对了一半**,值得写下来:

| | `-j 12` | `-j 28` | `-j 64` | `-j 20` |
|---|---:|---:|---:|---:|
| CPU | 43% | 82% | 80–92% | 76–82% |
| 编译进程总内存 | 1.85 GB | 9.6 GB | 9–11 GB(采样) | **22–25 GB** |
| 结果 | 慢一倍多 | 正常 | **9 处 OOM** | 正常 |

错误出在**采样区间**:`-j 28` 和 `-j 64` 的内存是在轻 TU 阶段测的(单进程约 300 MB),
外推到全程。真正编到 `core/animation` 时单进程要 **1.1–1.3 GB**——`scoped_css_name.h` 里
`std::ranges::equal` 对 `HeapVector<Member<...>>` 的模板实例化。64 × 1.2 = 77 GB,必炸。

雪上加霜的是 **`TaskStop` 只杀 bash 外壳,不杀进程树**:为了调并行度停了两次构建,
被遗弃的 ninja 全都活着继续编,累积到 **112 个 clang-cl**,commit 冲到 155/174 GB,
`Pages/sec` 十万级,整台机器卡死(表现为磁盘持续写入 —— 那是分页文件)。

**规矩**:(1) 按**重 TU 区间**的峰值算 j,不是全程均值;(2) `TaskStop` 之后必须显式
`Stop-Process` 清 `ninja/clang-cl/rustc/lld-link`,跑两遍;(3) 开构建前先确认没有残留 ninja,
否则并行度的观测值和实际值对不上,一切调参都是错的。

### 19.7 还能往下走的

| 手段 | 可得(raw) | 性质 |
|---|---:|---|
| ICU 再裁到 flutter_desktop | 3.26 | 需先分离构建期/运行期数据(§19.3) |
| skia Ganesh GPU 后端 | 0.82 | 删源码,shot 只用 CPU 光栅 |
| libwebp / ced / libxslt / re2 | ~1.1 | 删源码,逐个查活调用方 |
| blink core 逐特性 | ~0.1 / 个 | 最贵 |

加起来约 5.3 MB raw ≈ 1.5 MB 压缩后。**目标已达成,这些是可选的。**

---

## 20. 把「能渲染」变成「能用」:接上网络、补齐截图能力、做出进程池

这一节记的是 `docs/shotium-plan.md` 第 0–3 组的落地。和前面 19 节不同,**这一节几乎没有在删东西**
—— 它在往回加。加回来的每一样都在下面写清楚是什么、为什么。

体积因此从 **44.81 MB / 压缩后 14.08 MB** 涨到 **57.71 MB / 压缩后 16.69 MB**。这是知情的:
用户明确说过「先可用 后续再说砍的问题」。§20.6 记了这 12.9 MB 的去向,给后面那一轮当起点。

### 20.1 恢复:`//net` 整条网络栈(约 6.2 MB raw)

`//shot` 本来就依赖 `//net`(为了 `net::FilePathToFileURL` 这类工具函数),但 `/OPT:REF`
把没人调用的部分全丢了。现在 `shot_network.cc` 建了一个 `URLRequestContext`,于是
`URLRequest` / `HttpCache` / `HttpNetworkSession` / BoringSSL / quiche 的 HTTP/2 部分
全部被拉回二进制。

| 组件 | 大小 | 为什么在 |
|---|---:|---|
| `net` + `net/http` | 3.60 MB | `URLRequest`、缓存、HTTP 语义 |
| `net/third_party/quiche` | 1.15 MB | **不是 QUIC**:HTTP/2 的 framer/HPACK 在 quiche 里,`SetSpdyAndQuicEnabled(true, false)` 只关掉了 QUIC |
| `third_party/boringssl` | 1.12 MB | TLS |
| `third_party/zstd` | 0.48 MB | `Accept-Encoding` 的一员 |

**没有**接 `//services/network`。那不是「更完整的网络栈」,它是把 `URLRequest` 包成 mojo 服务
给多进程 Chrome 用的壳;worker 自己就是一个进程,那层壳是一条通向自己的管道。

配置(`shot_network.cc`,每一项都有注释说明理由):HTTP/2 开、HTTP/3 关、brotli 开、
重定向开(上限 20)、代理直连(PAC 要脚本解释器,这个二进制没有)、系统 DNS、
内存 CookieMonster、磁盘缓存用 **Simple 后端**(一个 entry 一个文件,`net/disk_cache/simple/`
零处引用 sqlite,所以不会把 sqlite 拉回来)。缓存目录不给就完全不缓存 —— 一个进程一个目录,
后端对目录是排它锁,共享目录意味着除第一个以外的 worker 全在无缓存状态下静默运行。

### 20.2 主线程换成 IO 消息泵

`WebThreadScheduler::CreateMainThreadScheduler()` 原来拿的是 `MessagePumpType::DEFAULT`
(渲染进程主线程就是这个)。`//net` 通过 `base::CurrentIOThread` 监视 socket,而那个东西只在
线程的泵是 `MessagePumpForIO` 时存在。blink 不关心泵的类型 —— 调度器在拿到的任何泵上建它的
任务队列 —— 所以 IO 泵在这里是 DEFAULT 的严格超集。

代价:零。验证:换泵之后 `render_corpus.html` 的 PNG SHA-256 仍是 `b42f1efe5f7f433c…`,
和裁剪轮之前逐字节相同。

### 20.3 三个「一直坏着、但没人问过」的缺陷

接 HTTP 的过程里撞出三个和 HTTP 无关的既有缺陷。它们的共同形状是:**语料里没有用到,
所以从来没被问过**。

**(a) `font-family: system-ui` 直接段错误。**
`FontCache::SystemFontFamily()` 返回 `MenuFontFamily()`,而后者解引用一个静态指针,
那个指针在 `SetMenuFontMetrics()` 被调用之前是 **null**。Chrome 里浏览器进程读
`NONCLIENTMETRICS`、填进 `RendererPreferences`、渲染进程在
`WebViewImpl::UpdateFontRenderingFromRendererPrefs()` 里调这三个 setter。shot 没有浏览器
进程也没有 WebView,所以从来没人调过。

修法不是加判空,是**补上那次调用**:`ShotRuntime` 用 `gfx::win::GetSystemFont()` 读同一份
`NONCLIENTMETRICS`,直接告诉 blink。`system-ui` 是真实网页上最常见的字体族之一 —— 接上 HTTP
之后第一个访问的站点就撞到了。

**(b) `file:` 下的外部样式表被静默丢弃。**
字节到了(日志明明写着 `load ok … (175 bytes, text/css)`),但
`CSSStyleSheetResource::CanUseSheet()` 对 `file:` URL 多问一句:扩展名映射出来的 MIME
是不是 `text/css`。这一问走的是 `MIMETypeRegistry::GetMIMETypeForExtension()`,而那是一次
**到浏览器进程的 mojo 同步调用**(存在的理由是沙箱里的渲染进程读不了注册表)。管道那头没人,
返回空,样式表被解析完再扔掉 —— 页面像作者没写过 CSS 一样排版,一行错误都没有。

修法:`ShotPlatform` 现在提供一个 `BrowserInterfaceBroker`,只应答一个接口
`mojom::MimeRegistry`,实现就是 `net::GetMimeTypeFromExtension` —— 和
`content::MimeRegistryImpl` 一模一样的那一行。**接收端必须放在另一个序列上**:那个方法是
`[Sync]`,调用方会阻塞 blink 线程等回复,接收端放同一个线程就是自己等自己。

这个缺陷之所以活到今天,是因为 `render_corpus.html` 用的是内联 `<style>`。现在
`features.html` 的样式已经挪进 `features.css`,每一条几何检查都顺带守着这条路径。

**(c) 不是缺陷,是一条事实,记下来省得下次再查:** `URLRequestContext::CreateRequest` 在
Windows 和 Linux 上**没有**四参数(未标注流量)的重载 —— 只有这两个平台的流量标注要审计,
所以不带标注的版本在那里被 `#if` 掉了。必须写第五个参数
`net::handles::kInvalidNetworkHandle`(意思是「走默认路由」,绑定具体网卡是 Android 的
多网络特性)。

### 20.4 截图能力(第 2 组)

`scale` / `fullPage` / `clip` / `selector` / `type` + `quality` / `omitBackground` / `path` 全部落地。
几个不显然的决定:

- **`scale` 不用 `SetPageScaleFactor`。** 那是捏合缩放,会改布局。设备像素比走
  `Page::SetInspectorDeviceScaleFactorOverride()` —— DevTools 设备模拟拉的就是这根杆 ——
  再加上 `ChromeClient::GetScreenInfo()` 报同一个数,这样 `srcset` / 分辨率媒体查询选的是 2x 资源,
  而不是把 1x 放大。只改缩放因子,`ScreenInfo` 的其它字段(尤其是屏幕矩形)保持默认:填进去会改变
  device-width 媒体查询的答案,而这条流水线根本没有一块屏幕可以报告尺寸。
- **`fullPage` 要先把视口撑大再画。** 折叠线以下的内容是被 cull 掉的,不是画了不给。撑大最多三轮:
  视口变大会让内容变大(`vh` 单位),两者会互相追。三轮足够普通情况(第二轮就稳定),
  病态情况下停在「视口偏小」而不是停在死循环。
- **`selector` 在 C++ 侧解析。** `Document::QuerySelector` + `LayoutObject::AbsoluteBoundingBoxRect`。
  没有 V8,注入脚本这条路不存在。用**默认构造的 `ExceptionState`**(它记录而不抛)而不是
  `NonThrowableExceptionState`:选择器来自进程外,非法选择器必须是一句错误消息而不是一次崩溃。
- **不能同时满足的组合直接拒。** `quality` + png、`omitBackground` + jpeg、
  `selector`/`clip`/`fullPage` 三选一。静默忽略一个字段,等于交出一张「悄悄没照做」的图。

### 20.5 出进程 + 并发(第 3 组)

`shotium/` 是纯 JS:进程池、队列、`retry`、`on()` 事件、`.d.ts`。没有原生 addon
—— 要的是对外接口长成约定的 `ScreenshotOptions`,不是必须 `.node`,于是省掉 node-gyp 和多 ABI。

一个 worker 一次只渲一张,这不是简化:blink 是绑在 worker 主线程上的进程级单例,协议允许并发
也渲不了。并发就是多进程,崩溃隔离也是同一件事的另一面。

**`ScreenshotOptions` 补了 `viewport?: {width, height}`**。之前的接口没有任何地方能表达视口
—— `fullPage` 和 `clip` 说的都是「从视口里取哪一块」,不是「视口多大」。JS 层把它摊平成协议里的
`width`/`height`。

### 20.6 这一轮的验证

三套检查,全绿:

| 脚本 | 覆盖 | 关键一条 |
|---|---|---|
| `tools/shot/serve_check.py` | 协议 + 几何 + 编码器 + 拒绝路径,42 条 | `clip` 和 `selector` 找到的是**逐字节相同**的那个盒子 |
| `tools/shot/net_check.py` | http/https、重定向、跨进程磁盘缓存、networkidle,17 条 | **同一份文档走 http 和走 file: 渲染出的字节完全相同** —— 传输不该出现在图里,这条把它从假设变成事实 |
| `tools/shot/node_check.cjs` | 池、并发、retry、崩溃隔离,17 条 | 请求飞行中 `kill` 掉 worker,重投在另一个 worker 上拿到**同样的图**,槽位被补上 |

`render_corpus.html` 的 SHA-256 全程是 `b42f1efe5f7f433c…`,和裁剪轮之前一致 ——
换消息泵、接网络栈、重写渲染器、改 ChromeClient,渲染零回归。

### 20.7 后面那一轮要砍的(实测,不是猜)

| 组件 | 大小 | 说明 |
|---|---:|---|
| `net/third_party/quiche` | 1.15 MB | 关掉 QUIC 之后剩下的是 HTTP/2 的 framer;要再小得放弃 h2 |
| `third_party/sqlite` | 0.87 MB | **不是网络栈带回来的**:`gn path` 显示是 `blink/renderer/platform/loader` → `//components/persistent_cache` |
| `third_party/zstd` | 0.48 MB | 关掉 zstd 内容编码即可 |
| §19.7 那一张表 | ~5.3 MB | 仍然有效 |

现在压缩后 16.69 MB,目标是 15 MB。差 1.69 MB,上表随便凑两项就够。

## 21. CI:一个公网 runner 到底卡在哪

第一次真的把 `engine (windows)` 跑起来,连 `gclient sync` 都没过。四个问题,每一个
都是「裁剪之后才存在」或者「本机看不见」的,记在这里。

### 21.1 `git.bat`:被自己的 `DEPOT_TOOLS_UPDATE=0` 挡住的 bootstrap

```
File "C:\depot_tools\git_cache.py", line 218, in GetCachePath
  subprocess.check_output(
FileNotFoundError: [WinError 2] The system cannot find the file specified
```

`git_cache.py` 里写死 `git_exe = "git.bat" if sys.platform.startswith("win")`。
那个 `git.bat` 不在 depot_tools 仓库里,是 `bootstrap/bootstrap.py` 生成的
(`WIN_GIT_STUB_NAMES`)。而 `gclient.bat` 只在**要自更新的时候**才会
`call update_depot_tools.bat`,bootstrap 就挂在那条路径的末尾。我为了让每次 CI
用它自己 clone 的那个 depot_tools(而不是跑到一半被上游新版本换掉)设了
`DEPOT_TOOLS_UPDATE=0`,正好把唯一一条通往 bootstrap 的路跳过去了。

要命的是 runner 上 **`git.exe` 是有的**(Git for Windows 在 PATH 里),所以看起来
「git 明明能用」。gclient 找的不是 `git`,是 `git.bat`。

修法:clone 完显式 `call bootstrap\win_tools.bat`,然后断言 `git.bat` 存在。
`win_tools.bat` 是自足的(cipd 自举),不需要先跑自更新。

### 21.2 `version_file`:条件还没判,文件已经读了

```
FileNotFoundError: 'src\chrome/build/android-arm.orderfile.txt'
```

DEPS 里四个 Android orderfile 的 CIPD 包用 `'version_file'` 指向仓库里的一个 txt。
gclient 在 `_deps_to_objects` → `CipdDependency.__init__` 里就把它读掉了 ——
**`'condition': 'checkout_android and non_git_source'` 是之后才判的**。所以哪怕
`target_os = ["win"]`、`checkout_android` 是 False,少了那个 txt 一样解析不下去。

这是「删文件」和「删依赖」不同步的典型:txt 早在第一波裁 Android 时就没了,但
DEPS 里的引用留着。修法是把那四块从 DEPS 删掉,而不是把 txt 恢复回来 —— 这棵树
永远不会构建 Android。

### 21.3 `tast_control`:一个无条件 hook,输入在 //chromeos

顺着上一个问题查的时候顺手把 57 个 hook 全过了一遍:哪些是无条件的、它们引用的
路径还在不在。只有一个真的会炸 ——

```
'name': 'tast_control',            # 没有 condition
'action': [..., '-o', 'src/chromeos/tast_control.gni',
                '-t', 'src/chromeos/tast_control.gni.template', ...]
```

脚本 `build/util/tast_control.py` 还在,输入全没了(`chromeos/` 整个删了)。
`remove_stale_files` / `remove_stale_pyc_files` 引用的路径也不存在,但那两个的
工作就是删东西,缺了正好。

**方法值得记**:与其一次跑一次 CI 撞一个,不如把 DEPS 里所有 hook 的 `'src/...'`
token 抓出来,逐个 `os.path.exists`,再按 condition 分类。一次就把剩下三个雷
(`ios_internal`、`telemetry`、`v8 builtins-pgo`)确认为「条件为假,不会跑」。

### 21.4 Windows SDK:pin 的是目录名,不是兼容性

`build/vs_toolchain.py` 里 `SDK_VERSION = '10.0.28000.0'`,注释写着
「VS 2026 18 with 10.0.28000.2270 SDK」。公网 runner 上没有,也装不了 ——
Chromium 平时用的那个打包工具链要从 Google 内部的 GCS 桶下载
(`DEPOT_TOOLS_WIN_TOOLCHAIN=1` 那条路),外面拿不到。所以 CI 只能用镜像自带的
Visual Studio,也就只能用镜像自带的 SDK。

关键判断:**那个常量是拿去拼目录名的**(`Include/<SDK_VERSION>/um/...`),
不是「低于此版本编不过」的声明。真正的能力检查在同一个文件里,叫
`SDKIncludesIDCompositionDevice4()` —— 它打开 `dcomp.h` 找一个 GUID,因为那个接口
是在一次不涨主版本号的 servicing release 里加的。既然版本号本身不承载兼容性,
就不该让它把一个装好了的 SDK 判死。

改成 `os.environ.get('CHROMIUM_WIN_SDK_VERSION', '10.0.28000.0')`,两个文件都改
(`build/vs_toolchain.py` 和 `build/toolchain/win/setup_toolchain.py`,注释里明写
这两个必须一致)。CI 里加一步扫 `Windows Kits\10\Include`,挑出**同时**有
`um\windows.h` 和 `Lib\...\um\x64\kernel32.lib` 的最新一个 —— 镜像里确实存在只有
一半的 SDK 目录。本地默认值不变。

这条改完之后,头文件比本机旧。如果有代码依赖新 SDK,它会在编译期说话;那时候读到
的报错是「这段代码需要新头文件」,不是「构建坏了」。

### 21.5 `DIR_METADATA` 的 mixin:66 个指向已删目录的引用

修完上面四个之后,`gclient sync` 过了(依赖全部拉下来,hook 跑到第 40 个),死在:

```
'python3 src/testing/generate_location_tags.py --out src/testing/location_tags.json'
failed to read "src\base\android\COMMON_METADATA": The system cannot find the path specified
```

`base/test/android/DIR_METADATA` 第一行是 `mixins: "//base/android/COMMON_METADATA"`,
而那个文件跟着 Android 一起删了。照例不是修一个,是把同类全查一遍:**67 处 dangling
mixin,分布在 66 个文件**里 —— `blink/public/mojom/*` 里一大片指向
`content/browser/*/COMMON_METADATA`,`blink/common/*` 指向
`blink/renderer/modules/*`。裁掉整个子系统时,活下来的目录还留着指向它的元数据。

删掉这些行之后有 **55 个 DIR_METADATA 变成空文件**(它们本来只有一行 mixin),
一并删掉 —— 空的元数据文件不是元数据。

顺带记两个不算错误的现象:
- `lastchange.py` 报 `Failed to get version info from git`,退回 `0.0.0`。原因是它
  找的是 `--grep=^Change-Id:`,而发布仓库是一个压扁的孤儿提交,没有 Change-Id。
- 磁盘不是瓶颈:runner 上 C: 剩 45 GB、D: 剩 219 GB,而 workspace 在 D:。
  最早那版 workflow 只打印 C:,差点把「够不够」判断在错误的盘上。

### 21.6 编译真的开始了,然后 sccache 把它弄停了

`gn gen` 过了,ninja 报出整张图 **14,907 条边**,编到第 7 条时死掉 —— 不是代码:

```
sccache: error: Server startup failed: cache storage failed to read:
  uri: https://.../\_apis/artifactcache/cache?keys=sccache/.sccache_check
  response: status: 400  <h2>Our services aren't available right now</h2>
```

`_apis/artifactcache` 是 Actions Cache 的 **v1 API**,GitHub 已经停服。sccache 启动
失败就整个退出,ninja 跟着停。

换掉,而不是修:**直接缓存构建目录本身**(`actions/cache` 存 `src/out/Shot`)。
理由是 ninja 本来就是增量的,它需要的只是自己的输出目录回来 —— 在它前面再放一层
编译器缓存,是多一层可以坏掉的东西。配套的三件事:

- `ninja` 那步给 `continue-on-error: true` 和自己的 `timeout-minutes: 280`。
  **超时不能把保存缓存那步一起带走** —— 靠 job 级超时的话,后面的步骤能不能跑
  取决于 cancel 的语义,不可靠;给这一步单独的预算就没有这个问题。
- 保存之后单独一步检查 `shot.exe` 在不在。ninja 可以没编完,job 不可以把这叫成功。
- `args.gn` 每次必须写出**逐字节相同**的内容,否则恢复回来的目标文件全部失效。

顺带确认了两件事:runner 上是 **Visual Studio 18 Enterprise**,`vs_toolchain.py`
自己找得到,所以把 `GYP_MSVS_VERSION` 拿掉了 —— 写死一个版本号只是一句会过期的
断言。SDK 检测步骤挑中 **10.0.26100.0**,clang-cl 用它的头文件编前 7 个 TU 没有
抱怨。

### 21.7 `NTDDI_WIN11_BR`:一个不存在的宏,不会报错,只会让声明消失

换掉 sccache 之后 ninja 真的开编,第 8 条边炸了:

```
fileapi.h(1081,10): error: unknown type name 'FILE_INFO_BY_HANDLE_CLASS'
winbase.h(9388,11): error: unknown type name 'FILE_INFO_BY_HANDLE_CLASS'
```

看起来像 SDK 坏了,其实是命令行里的 `-DNTDDI_VERSION=NTDDI_WIN11_BR`。
`build/config/win/BUILD.gn` 的 `config("winver")` 把它写死,而
**`NTDDI_WIN11_BR` 是 SDK 28000 才有的符号**(28000 的 `sdkddkver.h`:
`NTDDI_WIN11_GE 0x0A000010`、`NTDDI_WIN11_DT ...11`、`NTDDI_WIN11_BR ...12`),
26100 里没有。

关键在于**这不会报错**:预处理器把没见过的标识符在 `#if` 里当 0,于是
`NTDDI_VERSION` 变成 0,所有 `#if NTDDI_VERSION >= NTDDI_WIN7` 之类的守卫全部为假,
`FILE_INFO_BY_HANDLE_CLASS` 这些声明**静悄悄地消失**,几千行之后才以「未知类型名」
的形式冒出来。所以这类错误不能按字面读。

改成 gn 参数 `win_ntddi_version`,默认值仍是 `NTDDI_WIN11_BR`(本机不变);CI 在
选定 SDK 之后顺手从**同一个** `sdkddkver.h` 里把所有 `NTDDI_WIN*` 宏的值解析出来
取最大的那个,写进 `args.gn`。SDK 和 NTDDI 必须来自同一处,这是本质约束,不是巧合。
