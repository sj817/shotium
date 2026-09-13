# 与上游 Chromium 同步

## 当前标准入口

使用 `pnpm upstream:sync plan/apply`，具体参数、保护和输出见
[批量同步工具](upstream-sync-tool.md)。本轮目标与验收状态以
[upstream-sync-state.json](upstream-sync-state.json) 为准；`completedBaseline`
只记录已经完成验收的基线，不能把正在合并的 target 当成已同步版本。

2026-09-09 开始将保留引擎同步到固定的 Chromium `c099bd180a2d`
（155.0.8048.0），目标发布版本为用户批准的 0.6.0。主计划 220 个文本冲突
已解决，Skia、ICU、Perfetto 已按独立基线同步源码。Windows EXE/DLL、GN、
源码输入、网络、Node/daemon、84 个渲染用例和离线 Bilibili 检查通过。
basic/layout/paint/text 四组与升级前二进制的像素及哈希完全一致。
旧编码输入行为保持原状（同步文档装载仍按 UTF-8）。六平台 CI 已在
`07ac95e4a85dad12bc7fd8029c5ca229e3e38d9f` 全部通过，0.6.0 已发布，
七个 npm 包及六个 GitHub 二进制附件已核对。不恢复被裁剪的浏览器功能。
各平台执行了其工作流支持的检查；交叉编译不等于原生设备运行验收。

发布证据与 CI 分片问题记录见 [0.6.0 验收记录](upstream-sync-0.6.0-validation.md)。

具体源码决策见 [155 同步记录](upstream-sync-decisions-155.json)。这份记录包括
固定上游提交、保留产品差异的原因以及新增/退役路径，不是补丁重放队列。

下方旧统计与 `out/ShotWip` 命令属于历史记录；执行时以当前 CLAUDE.md、
build-engine/verify-engine 流程为准，Windows 构建入口为
`pnpm build:engine`，输出为 `out/Shot`。

ICU、Skia、Perfetto 现在由本仓直接维护。更新它们时按各目录 README.shotium.md
记录的上游版本审查差异，直接修改源码；不恢复它们的 DEPS/gitlink 或补丁重放。
原 ICU 功能裁剪、Skia 并行模糊/逐行解码限制、Perfetto trace processor 开关均已
保留在对应源码中。外部依赖的其他 DEPS 条目仍按本文同步。

> 这棵树不是 Chromium 的一个分支,是它的一份切片。所以同步不是 merge,
> 而是「按记录的基线重放上游的差异」。这份文档是那个记录,以及那套动作。

## 1. 基线

同步的一切都从这四行开始。**每次同步完成后改这里,这是唯一的记录点。**

```
UPSTREAM_BASE    c099bd180a2db0fa6a313d43653529ba02665c84
UPSTREAM_POS     refs/heads/main@{#1694285}
UPSTREAM_DATE    2026-09-09
CHROME_VERSION   155.0.8048.0
```

基线来自固定上游提交及其 `Cr-Commit-Position`，并由同步决策记录与六平台
验收共同确认。`build/util/LASTCHANGE` 可能记录本仓提交，旧分支的 merge-base
也不会随切片同步推进；它们不能作为本轮已同步版本的证据。

以下为 2026-08-26 的历史调研，统计不代表当前仓库：

截至 2026-08-26,上游比这个基线**领先 9,130 个提交**。

## 2. 为什么不能 merge

发布历史的根提交是 `ac613e9ccf87`,一个压扁的初始提交。它和
`upstream/main` **没有共同祖先**,所以 `git merge upstream/main` 得到的不是
一次同步,是一场把 50 万个文件全部当成冲突的灾难。

规模决定了做法。上游 `upstream/main` 有 504,751 个文件,我们跟踪 64,425 个 —— 12.8%。这
64,425 个按「同步时该拿它怎么办」分,只有四类(数字实测于
`NEW = 0289ec3b`,2026-08-26):

| 类 | 数量 | 动作 |
|---|---|---|
| 我们动过、上游也有 | **1,193** | **手工合,唯一要人看的** |
| 没动过、上游也有 | 61,620 | 直接取上游新版 |
| 没动过、上游已经删了 | 1,554 | 跟着删,逐个判断 |
| 我们自己的文件 | 58 | 不动 |

`1193 + 61620 + 1554 + 58 = 64425`,对得上。**同步的全部难度就在第一行那
1,193 个里面**,剩下 98% 是机械操作。

### 2.1 静态截图裁剪的同步边界

不要因上游重新出现 include 或 GN 源项就恢复已删除的浏览器模块。当前逐批删除与验证清单在 docs/screenshot-cut-execution-2026-09-07.md，完整接续边界在 docs/screenshot-cut-task.md。

- 滚动动画保留 ScrollAnimationState/ScrollAnimator 的 CPU 曲线、RTL 换算、完成/取消和锚定状态，合成器提交与接管已删除。第六批已通过 Windows 与177张像素验证。
- 第七批已删除 components/input、Blink IME/EditContext、浏览器公共宿主接口、Widget输入Mojo、CSS selector watcher、Autofill专用缓存/事件、execCommand、拼写检查/文字建议与SystemClipboard。GN/IDL/Mojo同步删除，Windows与179张像素验证通过。
- 表单关联、校验、默认值、真实焦点和plaintext-only样式必须保留。其他GPU/拖放/诊断残留仍按完整清单继续处理，不以未完成项作为恢复已删模块的理由。

## 3. 远程仓库

```
origin     https://github.com/sj817/shotium.git      ← 推这里
upstream   https://github.com/chromium/chromium.git  ← 只读,push URL 已禁用
```

`upstream` 是 `blob:none` 的部分克隆(`remote.upstream.promisor=true`),所以
它有完整的提交和树对象,blob 按需拉。`git ls-tree -r upstream/main` 能跑,
`git log` 能跑,只有真去读文件内容时才会走网络。

`push.default=upstream` + `remote.pushDefault=origin`:工作分支叫 `release`
而远程分支叫 `main`,名字不一样,靠这两个配置让裸 `git push` 正确落到
`origin/main`,不必每次写全。

> 本地还有一个孤儿分支 `main`(`5b0f17663ada Initial commit`),不是 `release`
> 的祖先,任何远程都没有它。留着只会让 `git branch` 看着糊涂,可以
> `git branch -D main`。

## 4. 同步动作

设新基线为 `$NEW`,旧基线为 `$OLD`(即上面的 `UPSTREAM_BASE`)。

### 4.1 取上游

```bash
git fetch upstream main
NEW=$(git rev-parse upstream/main)
OLD=c0bba1026178fe2a8b441fead7928b697a801c1e
ROOT=ac613e9ccf87132210fc4b8d603f14582443786a
```

### 4.2 分桶

```bash
# 我们动过、且还留着的 —— 唯一需要人看的那 1,193 个
git diff --name-only --diff-filter=d $ROOT HEAD | sort > /tmp/ours.txt

# 我们跟踪的全部
git ls-files | sort > /tmp/tracked.txt

# 上游新基线里存在的全部 —— 用来剔除「我们自己的文件」和「上游已删的文件」
git ls-tree -r --name-only $NEW | sort > /tmp/theirs.txt

# 原样桶:我们跟踪 ∧ 我们没动过 ∧ 上游还有
comm -23 /tmp/tracked.txt /tmp/ours.txt | comm -12 - /tmp/theirs.txt > /tmp/pristine.txt
```

第三步的交集不能省:
`shot/`、`bench/`、`bootstrap/`、`tests/` 里有 58 个文件是根提交带来的、
之后又没改过的**我们自己的**文件,不剔掉就会被当成上游文件处理,而上游根本
没有它们。

跑完对一下数,四类加起来应该等于 `git ls-files | wc -l`:

```bash
# 1,193 和 61,620
wc -l /tmp/ours.txt /tmp/pristine.txt

# 1,612,即上游已删的 1,554 加我们自己的 58
comm -23 /tmp/tracked.txt /tmp/ours.txt | comm -23 - /tmp/theirs.txt > /tmp/gone.txt
wc -l /tmp/gone.txt
```

### 4.3 原样桶直接取上游

```bash
xargs -a /tmp/pristine.txt git checkout $NEW --
```

### 4.4 剩下那 1,193 个手工过

```bash
# 上游在这段区间里对这些文件做了什么
xargs -a /tmp/ours.txt git log --oneline $OLD..$NEW --
```

没有捷径,但量是可控的,而且绝大多数是 `BUILD.gn` 和被砍过的头文件。

其中有几个不是「砍」也不是「改 BUILD.gn」,而是**行为上的分歧**:上游那一行
是对的,只是对浏览器是对的。这类改动没法靠 diff 认出来 —— 它们看起来就是一行
普通改动 —— 所以逐条记在这里,同步时按语义重放,不要按行合。

| 文件 | 分歧 | 为什么不能用别的办法 |
|---|---|---|
| legacy IPC / Mojo [Native] | 五个 Native 类型换为明确字段和枚举，旧 Channel/ParamTraits/native serializer 与紧急消息调度链完整删除 | 普通 Mojo 仍用于现有接口；net::HttpConnectionInfo 缓存数值、ECT 值域、RedirectInfo 字段、默认任务优先级不变。Shot 产品协议与 base::Pickle 缓存序列化保留；源码未集中编译 |
| Linux Shot libc 与依赖闭包 | Shot 可选择 musl ABI；独立 musl 目标工具链切换 Clang/Rust triple 与 Alpine sysroot，host 工具保持 glibc；内置 Expat、libunwind 并关闭 GLib/NSS，Node 插件恢复 `--as-needed` | Linux 发布包要求除 libc 外不依赖非系统 DSO；公共 HTTPS 只使用编译进二进制的 Chrome Root Store，不读取宿主或企业证书库；glibc 与 musl 产物、缓存和 npm 包必须隔离 |
| Linux Chrome Root Store | 当 Linux 关闭 NSS 时，`CreateSslSystemTrustStoreChromeRoot` 返回仅含 Chrome Root Store 的 trust store | 上游 Linux 默认组合总是 NSS，关闭后缺少工厂实现；Shot 没有宿主 CA 注入入口 |
| XSLTProcessor | 保留 PI 驱动的原生 XSLT；删除脚本导入、参数、transformToDocument/Fragment、包装类和独占 helper/构造器 | XML/XSL 文件不需要 JS，仍须转换成截图。保留 stylesheet 参数默认值、同源读取、禁止写文件/网络、排序/编码和文档替换；空外部参数原本无效果。源码未集中编译 |
| Origin Trial / RuntimeEnabledFeatures | 删除 token 与试验上下文链；生成器保留普通 feature、依赖/implied 和有效 context override | 无 token policy 注册者；XSLT 默认开启。内部 TestFeatureDependent/TestFeatureImplied 的 context override 传播与旧实现不同，见 out/cut-stage16-origin-generator/review.md；本批未编译 |
| Sanitizer / Skeleton | 删除脚本净化 API 和默认关闭 Skeleton；普通 HTML parser 走原 sanitizer-null 路径 | 保留正常 DOM 插入、template patchfor 和声明式 Shadow DOM，不因上游 parser 改动恢复脚本净化链；本批未编译 |
| Memory HTTP cache | 删除 net/disk_cache/memory 和独占工厂/构建分支；内部 HttpCacheParams 默认改为 DISK_SIMPLE | Shot 只有 DisableHttpCache 或显式 DISK_SIMPLE，失败只返回错误并无缓存运行，不切换内存后端。保留 Simple 索引、Blink MemoryCache、Cookie 与 Shot 缓存 API；本批未编译 |
| NQE / FileNetLog / Service Manager | 删除估计器、文件导出器、旧服务管理实现及专用接口 | 保留 ECT 值类型、原空估计器超时回退、HTTP2 PING/TCP RTT 与普通类型化 Mojo/ipcz；本批未编译 |
| CC / Canvas 绘图属性 / 元素捕获 / View transition | 删除合成器运行链、Canvas 绘图变换与隐私标记、元素/区域捕获和追踪元数据、浏览器 ImageReplacement；删除过渡运行时、专用 UA 资源与 @view-transition 规则对象，该 at-rule 由通用未知规则路径跳过 | 无 Shot 创建或消费方。保留 CPU PaintRecord/SkCanvas、SVG 滤镜与真实来源安全检查、Canvas 备用布局、图片/表单/iframe 正常处理；view-transition-name/group/class/scope 普通属性和选择器解析仍影响静态分组及 @supports，不能一起删。导航初始化保持原调用时点。第十一批待集中编译及 183 张原始像素验证，不能当作已通过 |
| `HttpStreamFactory::JobController` / `URLRequestContextBuilder` / net proxy resolution | 建连入口直接选择 DIRECT；移除代理解析服务、异步解析状态、PAC/WPAD/系统监听及 context/session 持有关系 | Shot 的对外接口没有代理配置；先前 CreateDirect 仍带入整套解析服务和平台依赖。普通 DNS 网络变化通知、TLS、HTTP2 和缓存继续保留 |
| Blink AnimationWorklet / NativePaint / ClipPaintPropertyNode | 删除无实现注册的Worklet控制器与背景色、box-shadow、clip-path生成器和状态；裁剪矩形收窄为当前CPU布局范围 | 无JS和合成器线程；主线程CSS动画、普通阴影、背景色、SVG和shape裁剪继续保留。第八批Windows EXE/DLL与完整运行检查通过，181张像素一致；动态clip专项输出与静态中点一致。六平台实际编译待完成 |
| CSS Paint API / cc Worklet派发 | 移除CSS paint()解析/样式缓存/跨线程值、Canvas记录器尾巴、cc AnimationWorklet任务与时间事件、PaintWorklet异步派发和调度等待 | 无JS注册和具体Worklet创建方；无效CSS声明fallback和正常CPU动画/绘制保留。自定义/原生Worklet属性动画、tracker及专用帧状态同步删除；PaintWorkletInput/DeferredPaintRecord、图片记录映射/provider、绘制和序列化支路均删除；普通CPU PaintRecord与解码/动画/HDR保留，本批Windows完整验证通过，181/181像素一致；普通CPU绘制记录不可误删 |
| Worker/Worklet公共协议与线程调度 | 删除无创建方的Worklet/ShadowRealm/V8/WebNN token、Mojom映射、Worklet请求类别、脚本FileReader入口、WorkerScheduler代理与限流、合成线程和V8任务队列 | 保留字体/HTML预扫描的后台线程、GC、任务清理及实际资源调度；FileReader共享读取器仍被DataObject使用。第九批Windows EXE/DLL和完整运行验证通过，181张原始像素一致；枚举保留编号同步映射完整性表，Linux仅完成图检查 |
| `HTMLCanvasElement` / `ImageElementBase` / Blink graphics | 删除 Web Canvas 绘图上下文、资源 provider、GPU bitmap 和脚本绘图值类型；普通图片基类迁到 `core/html`，标签保留备用内容与属性宽高比 | 没有 JS 或原生绘图调用方，但标签静态布局影响截图；176 张删除前后像素对照一致，不能把标签改成普通元素或恢复整套绘图系统 |
| `preload_helper.cc` / `document_init.cc` / Blink MIME registry | 删除无播放器的 audio/video preload 与媒体文档探测；普通非图片 MIME 分类仍保留原容器集合 | 引擎没有解复用、解码或播放入口，不应为播放能力保留整套 media 类型、线程与缓存依赖；视频 poster 仍走图片加载与绘制 |
| `third_party/blink/renderer/platform/graphics/parkable_image.cc` | `kDelayParkingImages` 默认关(上游开) | 这个二进制不注册 FeatureList,`IsEnabled` 一律回落到编译期默认值;`FeatureList::SetInstance` 又 CHECK「之前没有任何 feature 被读过」,而引擎起来之前 //base、//net、mojo 都已经读过自己的了。默认值就是唯一的开关。见 `shot/shot_renderer.h` 的 `ParkImagesEnabled` |
| `cc/paint/draw_looper.h` / `.cc` | 加了 `DrawLooper::MaxOutset()` | 纯新增,上游没有对应物;条带光栅要知道 looper 画出多远,而 `SkPaint` 里没有 looper,`computeFastBounds` 问不出来 |
| `third_party/blink/renderer/platform/graphics/compositing/paint_chunks_to_cc_layer.{h,cc}` | `ConvertInto` 多两个可选参数(cull rect、chunk 过滤器) | 都是纯新增的可选形参,上游调用点行为不变;超长文档要分多次滚动重画,过滤器是「贴视口的东西只画一遍」的落点 |

### 4.5 上游新增的文件

上面三个桶都不包含「上游新加、我们目录里本该有」的文件:

```bash
git diff --diff-filter=A --name-only $OLD $NEW | grep -E '^(base|build|cc|net|third_party/blink|ui)/'
```

一个一个看要不要。默认答案是**不要** —— 这棵树的默认动作是删,不是加。
见 `docs/cut-progress.md`。

### 4.6 上游已经删掉的

`/tmp/gone.txt` 里减去我们自己那 58 个,剩下 1,554 个是上游在这段区间里删掉、
我们还留着的。绝大多数来自 `third_party/` 的 roll(比如 abseil 换版本时删掉
的文件),跟着删就对了。但**不要整批 `rm`** —— 我们砍过 `BUILD.gn`,某个文件
在上游是「随组件一起没了」,在我们这里可能是「唯一还在引用它的地方是我们改过
的那份构建文件」。删完跑第 5 节的第 2 条。

### 4.7 DEPS 和 .gitmodules 必须一起改

`gclient` 只读 `DEPS`,`git` 只读 `.gitmodules`。两边不一致不会报错,只会
**静默地不拉取**某个目录,然后在几小时后的 `gn gen` 或 ninja 里变成一条看不懂
的错。任何一次动 DEPS 的同步,都要同时动 `.gitmodules`,并且核对两边的条目
数对得上。

### 4.8 同步 checkout

```bash
gclient sync -D --no-history
```

## 5. 同步之后必须过的检查

按代价从低到高,**顺序不要换** —— 前一条挂了,后一条的失败信息只会更难读。

| # | 命令 | 它证明什么 | 代价 |
|---|---|---|---|
| 1 | `gn gen out/ShotWip` | GN 图能生成 | ~25 秒 |
| 2 | `pnpm missing-inputs` | 图里每条边背后都真有文件 | ~10 秒 |
| 3 | `ninja -C out/ShotWip shot shot_c` | 真能编出来 | 小时级 |
| 4 | `pnpm verify:serve` 等 | 引擎还对 | 分钟级 |

第 2 条是这次同步加进来的,因为第 1 条**证明不了**它。

第 3 条之后还有一步:八个平台的引擎构建各自导出构建图(`pnpm graph:export`,
CI 产物 `graph-<os>-<arch>`),然后 `pnpm trim-tree plan --graph <dir>...` 算出
上游新带进来的、任何构建都不打开的文件,`pnpm trim-tree apply` 删掉,
`pnpm prune-deps --inputs <plan>/untracked-inputs.txt` 同步剪 DEPS / hooks /
`.gitmodules` / gitlink。规则全在 `scripts/tree/trim-tree.ts` 里,不靠人记。
删完对每个热构建目录跑一次 `pnpm depfiles:prune`:ninja 会拒绝在一个
depfile 指向已删文件的目录里开工。

## 6. 陷阱

### 6.1 热构建目录会把「图里引用、树里没有」藏起来

GN 会把一条边的 inputs 原样写进 `build.ninja`,**不管那个文件在不在**。
ninja 只有轮到构建那条边时才去看。所以只要缓存里那条边的产物还在,一个已经
被删掉的文件可以在图里躺很久没人发现 —— 然后第一次冷构建十二秒就炸:

```
ninja: error: '../../.rustfmt.toml', needed by
'gen/third_party/crubit/support/rs_std/rs_alloc.h',
missing and no known rule to make it
```

`.rustfmt.toml` 就是这么丢的:`1ee6e5a` 把它当成「Chromium 的流程文件」删了,
但 `build/rust/gni_impl/cpp_api_from_rust.gni:98` 拿它当 GN action 的 input。
两个平台各烧掉一小时 CI 才看见。

`gn gen` 抓不到这个 —— GN 从没打开过那个文件,它只是把路径抄了过去。
`ninja -n` 也抓不到 —— 它走同一张图,但遇到第一个缺失就放弃,不会列全。
抓得到的是问 ninja 要输入集然后逐个 stat,也就是
`pnpm missing-inputs`(`scripts/build/missing-inputs.ts`)。

一个构建目录只回答一个平台。要回答 Linux 就对着一个 Linux 的 out/ 跑 ——
给非宿主平台 `gn gen` 在任何宿主上都能跑,分钟级,比构建便宜得多。

### 6.2 砍的基线是根提交,不是后面某个提交

判断「这个文件是不是我们自己的」要拿 `ac613e9ccf87` 比。拿后面的提交当基线
会漏掉前几波已经删掉的东西,得出「这文件一直就没有」的错误结论。

### 6.3 恢复 include 行 ≠ 恢复组件

同步时上游可能给某个文件加回一个 `#include`。加上那一行不等于那个组件回来了。
要看的是字段类型和链接期符号,不是编译期能不能过。

### 6.4 别信 `probe`

`ninja -n` 只验图。它不验平台选源是否正确,也不验缺失输入是否完整(见 6.1)。
`probe` 绿了只说明图是连通的,不说明能编出二进制。

### 普通动画与CPU几何快照裁剪（第十批，Windows已验证）

不恢复Blink CompositorAnimation包装、eligibility、回执分组、GPU时间线镜像、动画专用运行开关和IsRunning*AnimationOnCompositor样式字段。普通动画的排队、NotifyReady和PaintClean后的时序延迟保留；原资格查询中的IsCurrent/on-demand副作用在PreparePendingUpdate及UpdateEffectTimingIfNeeded中保留。

关键帧仅为transform/translate/rotate/scale保存TransformKeyframeSnapshot（TransformOperations），供CPU子像素及轴对齐判断；属性变化、neutral keyframes、zoom与viewport刷新逻辑继续生效。SVG原点分离条件位于paint_property_tree_builder.cc的CanSeparateSVGAnimationTransformOrigin，SMIL、资源祖先、zoom、vector-effect和额外SVG容器变换条件不可省略。ScrollOffsets和Timing枚举是Blink本地数据，滚动时间线16微秒/像素保持。cc平滑滚动曲线仍有实际CPU消费者，cc宿主/GPU全链尚待后续大批处理。


### GPU/Viz 与绘制传输裁剪（源码已改，尚未编译）

删除 gpu/、GPU lists hook、Dawn override、Viz 帧与 surface/quad/copy-output 协议；Viz 只保留 display/gfx 实际使用的颜色格式与 Mojo 类型。不得通过同步恢复命令客户端、GPU config、Vulkan features 或生成版本头。

CC 只在进程内重放 PaintRecord，不再读写 PaintOp 传输流或 transfer cache；滤镜和路径效果保留 CPU 行为。文字仍调用原 drawTextBlob，移除 Slug/分析缓存；PaintCanvas flush 与 GPU texture-size 参数查询删除，原 CPU 路径使用的大小值 0 保持。HDR 重采样仍用原 SkSurfaces::Raster 路径。

Skia GN 移除 Ganesh/Graphite 的 sources/defines/生成器与 workaround_list，保留 CPU SkSL、图片、字体/Fontations、原字体栅格化配置及 Apple 字体框架。vendor checkout 的精简和整轮构建验证继续，不应恢复 GPU 编译入口来掩盖尚未修复的调用点。
