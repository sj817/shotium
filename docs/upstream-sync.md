# 与上游 Chromium 同步

> 这棵树不是 Chromium 的一个分支,是它的一份切片。所以同步不是 merge,
> 而是「按记录的基线重放上游的差异」。这份文档是那个记录,以及那套动作。

## 1. 基线

同步的一切都从这四行开始。**每次同步完成后改这里,这是唯一的记录点。**

```
UPSTREAM_BASE    c0bba1026178fe2a8b441fead7928b697a801c1e
UPSTREAM_POS     refs/heads/main@{#1680169}
UPSTREAM_DATE    2026-08-15
CHROME_VERSION   153.0.8010.0
```

这四行不是随手抄的,树里有两处独立的佐证,对不上就说明记错了:

- `build/util/LASTCHANGE` —— gclient 写的,内容就是
  `c0bba1026178...-refs/heads/main@{#1680169}`
- `git merge-base codex/shot-engine-round1 upstream/main` —— 那条分支是唯一
  还带着上游血缘的,它的 merge-base 正是 `c0bba102`

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
| `HttpStreamFactory::JobController` / `URLRequestContextBuilder` / net proxy resolution | 建连入口直接选择 DIRECT；移除代理解析服务、异步解析状态、PAC/WPAD/系统监听及 context/session 持有关系 | Shot 的对外接口没有代理配置；先前 CreateDirect 仍带入整套解析服务和平台依赖。普通 DNS 网络变化通知、TLS、HTTP2 和缓存继续保留 |
| Blink AnimationWorklet / NativePaint / ClipPaintPropertyNode | 删除无实现注册的Worklet控制器与背景色、box-shadow、clip-path生成器和状态；裁剪矩形收窄为当前CPU布局范围 | 无JS和合成器线程；主线程CSS动画、普通阴影、背景色、SVG和shape裁剪继续保留。第八批Windows EXE/DLL与完整运行检查通过，181张像素一致；动态clip专项输出与静态中点一致。六平台实际编译待完成 |
| CSS Paint API / cc Worklet派发 | 移除CSS paint()解析/样式缓存/跨线程值、Canvas记录器尾巴、cc AnimationWorklet任务与时间事件、PaintWorklet异步派发和调度等待 | 无JS注册和具体Worklet创建方；无效CSS声明fallback和正常CPU动画/绘制保留。自定义/原生Worklet属性动画、tracker及专用帧状态同步删除；PaintWorkletInput/DeferredPaintRecord、图片记录映射/provider、绘制和序列化支路均删除；普通CPU PaintRecord与解码/动画/HDR保留，本批Windows完整验证通过，181/181像素一致；普通CPU绘制记录不可误删 |
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

第 3 条之后还有一步:六个平台的引擎构建各自导出构建图(`pnpm graph:export`,
CI 产物 `graph-<os>-<arch>`),然后 `pnpm trim-tree plan --graph <dir>...` 算出
上游新带进来的、任何构建都不打开的文件,`pnpm trim-tree apply` 删掉,
`pnpm prune-deps --inputs <plan>/untracked-inputs.txt` 同步剪 DEPS / hooks /
`.gitmodules` / gitlink。规则全在 `scripts/trim-tree.ts` 里,不靠人记。
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
`pnpm missing-inputs`(`scripts/missing-inputs.ts`)。

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
