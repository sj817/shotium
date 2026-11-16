# 冷启动与高并发：空闲后的回暖、start() 和光栅流水线（2026-11-16）

[English summary](#english-summary)

约束和前几轮一样：性能优先，任何改动不许让渲染变慢；体积和内存在这个前提下再省。
所有数字是同一台 Windows 工作站（32 核）上的本地构建，用同一套脚本、同一组夹具测的。

## 先量清楚问题在哪

**冷启动**拆成三段看（Node addon 路径，`SHOT_PROFILE=1 SHOT_VERBOSE=1`）：

| 段 | 耗时 | 说明 |
|---|---:|---|
| Node 进程本身 | 约 77 ms | 这台机器建进程就慢：一个空的 `hello.exe` 进入 `main` 也要 20 ms，是杀毒和进程创建回调，不在引擎里 |
| `require` addon | 约 12 ms | 映射 36 MB 镜像 + 静态导入 DirectWrite（约 5 ms）；user32 Node 自己已经导入 |
| 引擎初始化 | 约 6.5 ms | Linux（WSL）3 ms |
| 预热 + 首张 | 11 + 5 ms | 不预热时首张约 16 ms，一样多 |

可执行文件的 `process_to_main` 25 ms 里只有约 5 ms 是镜像本身；DirectWrite 和 user32 的
加载（共约 12.5 ms）并行化无效——加载器锁让两个线程上的 `LoadLibrary` 串行，顺序或并行
总时间一样。所以冷启动里引擎能动的只剩下面第 2 条。

**真正影响用户的「冷」是空闲之后。** 引擎空闲 2 秒做一次回收、10 秒归还工作集。机器人类负载
的请求间隔经常超过这个时间，于是几乎每张都在付回暖的钱：

| 页面 | 连续请求 | 空闲 3 秒后 | 空闲 13 秒后 |
|---|---:|---:|---:|
| B 站模板（60 个字体子集） | 179 ms | 246 ms | 260 ms |
| fonts.html | 3.4 ms | 7.7 ms | 12.0 ms |

**并发下的内存**不随排队长度增长：进程内 512 个请求同时提交，引擎只多约 26 MB，其余都是调用方
自己攥着的结果 Buffer；守护进程 64 个客户端同时请求，峰值工作集 158 → 194 MB。问题在吞吐：
一个进程只有一个 Blink 主线程，64 张 B 站模板串行要 11.7 秒，想更快只能多开进程，每个进程
预热后常驻 70 MB 以上。

## 1. 空闲回收保留字体（`PurgeScope::kIdle`）

把 `PurgeMemory()` 的五步逐个关掉测「空闲后变慢多少、空闲时省多少」（临时环境变量开关，测完删掉）：

| 跳过的一步 | B 站模板空闲后 | fonts.html 空闲后 | 空闲私有内存 |
|---|---:|---:|---:|
| 都不跳（原行为） | +68 ms | +4.3 ms | 49.6 MB |
| Blink 各缓存（memory consumers） | +70 | +4.7 | 49.5 |
| Skia 字形缓存 | +72 | +3.8 | 49.7 |
| 已解码网络字体 | **−9** | +2.2 | 47.9 |
| 字形缓存 + 已解码字体 | −15 | **+1.5** | 48.1 |
| PartitionAlloc 回收 | +67 | — | 50.9 |

68 ms 全部来自重新解压 60 个 WOFF2 子集；保留它们空闲内存反而没变大（解码出来的字体只有
几 MB，重新解码时的临时分配比它还多）。于是空闲回收保留**已解码网络字体**和 **Skia 字形缓存**，
两者都有上限（24 MB，Skia 字形缓存上限）；资源缓存、垃圾、PartitionAlloc 空闲页照旧还。
显式 `releaseMemory()` / `shot_engine_purge()` 仍全部清空——调用方主动要内存，说明下一张
不会马上来。字体缓存同时改成按最近使用淘汰（原来按插入顺序，命中不刷新）。

改后：B 站模板空闲 3 秒后 166 ms、13 秒后 172 ms（原 246 / 260）；fonts.html 4.9 ms（原 7.7）；
空闲私有内存 47.9 MB（原 49.6）。

13 秒后的归还工作集（`ReleaseWorkingSet`）另测过：让下一张多 3–7 ms，但工作集从 85 MB 降到
6.5 MB，内存压力下这是真收益，保留不动。

## 2. `start()` 不再等预热

`Status()` 往引擎线程投一个同步任务去读缓存目录和 `cacheActive`，而这两个值在引擎线程发出
就绪信号前就已确定、之后不会再变；任务却只能排在预热后面，于是 `start()` 白等 11 ms。
改为就绪前存快照，`Status()` 直接返回。Node 里 `start()` 27.7 → 12.5 ms（Linux 19.8 → 约 8）。
启动后马上截图的总耗时不变（预热仍在队首），启动后还有别的初始化要做时，预热就和它重叠了。

## 3. 光栅流水线（`ShotRenderer::RenderPipelined`）

单张图里主线程（解析、样式、布局、绘制）和光栅加编码大约各占一半：B 站模板 92 / 90 ms，
css-heavy 7.6 / 10.2 ms。`RenderDocument` 本来就是先把所有显示列表录完再光栅，大图路径甚至
在光栅前就拆掉了页面——显示列表自带光栅需要的一切。所以把录制之后的部分拆成
`RasterPrepared()`，只读任务对象（显示列表、几何、输出），不碰 Blink 对象，也不读
`CaptureContext::Current()`（那是个普通全局变量，属于主线程上正在排版的下一张）。

- 单张、单个绘制窗口、不超过 `ImageStream::IsSmall`（400 万像素）的捕获，录完就交给渲染器
  的光栅通道（一条专用线程），主线程接着处理下一个请求。分块、超长滚动分段、超大图、一次性
  CLI 仍在本线程同步完成。
- **只在后面已有请求排队时才交接**（`SetOverlapProbe`，计数来自 `EngineThread::PostCapture`）。
  第一版无条件交接，性能矩阵里逐个 await 的小图慢了 1–3%（线程切换的延迟，没有可重叠的
  工作）；加上这个判断后顺序调用与旧版持平（4 轮交替 47.0/47.7、51.6/48.5、48.1/47.7、
  47.8/48.3 ms），并发收益不变。
- 通道上同一时刻只有一个捕获：交接时先等上一个光栅完，所以最多一张已录制的列表在等像素。
  可复用位图、`RasterPool`、光栅读的缓存，凡是通道外要用的地方先 `DrainRaster()`。
- `CaptureAndDeliverPipelined` 让两半各自报到，后到的一方合并统计（光栅、编码加到主线程测的
  各阶段上，`total` 量到图片就绪）、补写 `path`、回调。
- 完成顺序必须仍是提交顺序。在主线程上直接作答的请求（排版前就失败、选择器找不到、分块）
  比通道上前一张的光栅快得多，会插队：`verify:node-entry` 的 FIFO 用例约一半次数拿到
  `[2, 1, 3]`。所以这些作答点都先 `DrainRaster()`；新增用例「重光栅后面紧跟立即失败的请求」
  在修复前每次都失败，修复后 10/10 通过。
- TS 包不再在 JS 里排队，每个请求立刻交给引擎；`tail` 只用来让 `stop()` / `dispose()` 等
  在途请求结束。

满载吞吐（同一页面 16 张同时提交，每张平均耗时）：

| 页面 | 旧 | 新 | 倍数 |
|---|---:|---:|---:|
| simple | 3.8 ms | 1.83 ms | 2.1× |
| images | 7.4 | 4.8 | 1.55× |
| css-heavy | 13.6 | 10.9 | 1.25×（光栅本身是瓶颈） |
| filter | 18.7 | 15.1 | 1.24× |
| B 站模板 #1 | 249 | 153 | 1.63× |
| B 站模板 #2 | 278 | 239 | 1.16×（主线程占 82%） |

代价：满载时峰值私有内存 B 站模板 #1 140.7 → 159.0 MB（+18 MB），#2 192 → 176，images 不变。
多开一个进程至少 70 MB。

正确性：12 种页面在旧版顺序、旧版并发、新版顺序、新版并发四种模式下哈希全部相同；混合压力
（视口图、整页、分块、出错请求、写文件、请求高峰中途 `releaseMemory()`）300 张与单独渲染
逐张比对 0 处不一致；`verify:node` 新增「同时提交与逐个提交逐字节相同」一项。

## 冷启动的计时口径

`start()` 不再等预热以后，预热跑在 `start()` 返回之后、第一张截图之前。马上截图的调用方，
第一张会多等预热剩下的部分：`perf:compare` 原来把首张 `wall` 单独判定，于是 `startup-*`
三项判成 slower（4.4 → 14.4 ms），同时 `startMs` 24.2 → 11.8 ms 判成 faster。调用方真正
等待的是从 `require` 到第一张图，同一进程里不经 IPC 直接量（每边 4 组 × 15 个新进程，
对 0.12.4）：

| p50 | import | start() | 首张 | import → 首图 |
|---|---:|---:|---:|---:|
| 0.12.4 | 7.3–7.8 | 24.1–27.0 | 8.7–9.2 | 40.6–43.8 ms |
| 本次 | 6.3–6.4 | 11.4–12.6 | 20.0–20.3 | 38.1–39.0 ms |

所以冷启动现在按 `firstImageMs`（import + start + 首张）和 `importMs` 判定，`startMs` 与首张
`wall` 照常记录、不判定——一次性工作放在 `start()` 里还是首张前面，只在两者之间挪时间。

## 量过、没做的

- **DirectWrite / user32 并行加载**：加载器锁串行化，0 收益。
- **不加载 user32**（从注册表读菜单字体）：只对可执行文件省约 8 ms，Node 本来就加载了 user32；
  注册表值在默认配置下可能不存在，`system-ui` 的结果可能和 `SystemParametersInfo` 不一致。
- **跳过预热**：冷路径上预热 11 ms + 首张 5 ms 和不预热的首张 16 ms 一样多。
- **释放后仍常驻的 16 MB**：在 malloc 分区里（base、net、Skia、ICU、HarfBuzz 的长期对象），
  `pa=` 统计只覆盖 Blink 自己的分区，所以对不上。要再压得按分配点逐项查。
- **TS 进程池**：吞吐随进程数线性增长，但每个进程 70 MB 以上；用户选了先做进程内流水线。

## English summary

Measured on one Windows workstation against the same fixtures. (1) The idle
purge now keeps decoded web fonts and Skia's glyph cache (both bounded): the
first capture after a 3 s pause went from 246 to 166 ms on a template with
sixty WOFF2 subsets, with idle private memory unchanged; an explicit
`releaseMemory()` still clears everything. (2) `start()` no longer waits
behind the warm-up render (27.7 → 12.5 ms in Node), because engine status is
a snapshot taken before ready. (3) Raster is pipelined: a small single-window
capture hands its display list to a raster lane once painted, and the engine
thread lays out the next request meanwhile. Saturated throughput rises
1.16–2.1× depending on the page's raster share, for at most about 18 MB more
peak memory; output is byte-identical.
