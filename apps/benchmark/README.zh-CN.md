# 六平台基准测试

[English](README.md)

这个 TypeScript 应用是 Shotium 标准的性能与韧性基准工具。它会测试
Shotium，以及所有在当前运行器上具备原生可执行文件的竞品浏览器。不支持的
竞品架构会记为 `n/a`；受支持架构上的安装或启动失败不会被隐藏成 `n/a`。

应用采用标准、简洁的目录结构：

```text
apps/benchmark/
├─ src/       TypeScript CLI、引擎、生命周期与聚合逻辑
├─ test/      通过 tsx 执行的 TypeScript 单元测试
├─ schema/    永久结果的 JSON Schema
└─ fixtures/  共用的静态渲染语料与资源
```

```bash
pnpm install --frozen-lockfile
pnpm run benchmark -- --shotium-version 0.3.2 --profile smoke --output ./out --seed local-check
```

如需仅运行一个场景分片，可追加 `--shard startup`、`--shard throughput`、
`--shard parallel`、`--shard resident` 或 `--shard resilience`。省略该选项
（或传入 `--shard all`）时，仍按本机单任务方式运行全部场景：

```bash
pnpm run benchmark -- --shotium-version 0.3.2 --profile full --shard throughput --output ./out --seed local-check
```

分片边界固定为：`startup` 包含冷启动、冷启动稳定后首张截图和生命周期；
`throughput` 包含暖机和批量；`parallel` 单独包含并发场景；`resident` 包含常驻
客户端和页面复用；`resilience` 包含故障及浸泡测试。

CI 会展开为 30 个 `平台 x 场景分片` 矩阵任务。每个分片仍在同一台原生 runner
上以平衡顺序测试所有可用引擎，因此同一场景内的比较仍是同机比较。五个分片会先
合并为一个平台结果，再聚合六个平台；runner 信息保留在各分片中，不会汇总不同分片
或不同平台的原始耗时。需要跨分片汇总时，只对同一 runner 内测得的同测试项相对比率
做几何聚合。

如需在同一台机器上直接比较源码构建的可执行文件，可运行：

```bash
pnpm run benchmark:native -- --baseline-executable /path/to/headless_shell --baseline-engine headless-shell --shot-executable /path/to/shotium --iterations 5 --warmup-iterations 1 --output ./out-native
```

JSON/CSV 报告包含原始样本、经过校验的 PNG 元数据、可执行文件 SHA-256/版本，
以及同机 `基线 p50 / Shot p50` 比率。
五个测试对象是引擎变体：Shotium，以及 Puppeteer、Playwright 分别驱动完整 Chrome
和 headless shell；它们并不是五个互相独立的软件包。每个质量通过的平台还会单独
生成几何平均综合排名。只有 Shotium 与对比引擎在同一场景、
同一并发度下均为“通过”且允许排名的测试项才会参与；归一化相对耗时越低越好。
报告会列出覆盖数和单项冠军次数，并且绝不跨平台混排。只有覆盖本平台全部可比项的
引擎才会获得正式名次；部分覆盖仍展示成绩，但会明确标记为不授予名次。失败、波动、
缺分片或缺证据的平台保留诊断数据，但不生成正式名次或第一名。

这里的公平性只对应一个刻意收窄的问题：每个锁定版本的开箱即用引擎变体，使用它
通常自带的浏览器二进制，完成同一套静态 HTML/CSS 截图任务时表现如何。它不是对
Puppeteer 或 Playwright 整个软件包能力的总评，也不能隔离驱动层开销：两者使用各自
锁定的 Chromium 修订版，而 Shotium 必须使用自己的裁剪 Chromium。Puppeteer 与
Playwright 可以另设「相同浏览器」赛道回答纯驱动开销问题，但 Shotium 本身就是浏览器
引擎、并非用于控制原版 Chrome 的驱动，因此无法做三方同浏览器比较。语料也只覆盖
三者共有的静态渲染面，不代表 JavaScript 或通用浏览器自动化。三者得到相同的外部
并发度、视口、缓存策略、语料、PNG 格式和操作超时；结果仍会如实包含各产品不同的
内部进程与内存拓扑，不会假设它们消耗完全相同的资源。

如需只重新生成某次归档的 Markdown/CSV 展示层（包括旧的四分片归档），运行：

```bash
pnpm run render-report -- --result-directory ../../benchmark-results/v0.3.2/<归档目录>
```

该命令读取已归档的 manifest 和各平台 summary，只替换 `report.md`、
`report.zh-CN.md`、`summary.csv`，以及已有索引对应的 `LATEST.md`；不会修改原始样本、
质量记录、失败证据或 manifest。报告顶部会链接到
[VitePress 基准站点](https://sj817.github.io/shotium/)。

`full` 配置包含七次冷启动重复、1/2/4 并发、20 次生命周期循环，以及连续 1000 次
请求或十分钟的浸泡测试。每个测试单元都会等待主机稳定；非冷启动单元固定预热三次。预热延迟 CV 和进程树 RSS 漂移继续作为引擎诊断数据记录，但不再决定单元是否有资格参与比较：Chrome 在每次新页面截图时创建和回收 renderer，本来就会造成进程树 RSS 波动，把它当作共享 runner 噪声会系统性地排斥 Puppeteer/Playwright。主机稳定门槛按分片校准：第一个单元开始前先采样 5 秒空载 CPU，采样时同时开着每个单元都会开的两个进程监控，这部分开销因此算进基线而不是算进噪声。门槛取 `max(25%, 空载 p95 + 10 个百分点)`，不设上限——GitHub 的 Windows 和 macOS runner 空载 CPU 本身就高，上限低于主机自身的空载水位时，这道门槛没有任何单元能过。门槛超过 80% 时结果里会记 `cpu_limit_exceeds_ceiling`，避免在一台吵闹的机器上声称主机是安静的。每个采样器占用不超过单核的 20%：一次进程表查询在 Linux 上是几十毫秒，在 Windows 上约 700 毫秒，不加节流的循环会把一整个核用来枚举进程，而这份负载正是门槛随后测到的「主机负载」。节流后的实际采样分辨率记在遥测的 `observed_mean_period_ms` 里。开测前的稳定定义是连续三个一秒采样都低于门槛且空闲内存平稳；固定预热后，在引擎仍存活时再次检查同一 CPU 门槛，此时空闲内存变化包含被测引擎自身行为，只作诊断、不作拦截。首次预检六秒内等不到安静主机的单元标为噪声并重试一次，重试最多等待 15 秒；持续繁忙的 runner 会如实留下 noisy，而不会在每个未采样单元上空耗 45 秒。分片用光配置里的时间预算后就不再排新单元，结果和证据会照常写出，不会被 job 超时一起带走。对照引擎失败——平台上没有对应的浏览器、同一张静态页两次渲染不一致、浸泡测试中途出空图——仍记录在 `summary.json` 和 `failures.json` 里，并先上传证据；六个平台与全部分片、证据和 Shotium/测试框架本身可信时可发布，带噪声或失败的对照单元会保留标签并从配对排名中排除。三套引擎的导航与截图操作统一使用 30 秒上限。到达 `load` 后，Puppeteer/Playwright 适配器会等待字体完成和两个动画帧再截图，相关时间全部计入实测操作，这与 Shotium 内部强制达到 paint-clean 生命周期的要求对齐。除此以外浏览器包保留默认启动和截图行为（只有 Linux CI 不可用的 sandbox 被关闭）；部分完成或超时的合成帧会如实记为包/浏览器结果，不用 benchmark 专属 Chrome 参数掩盖。每一处 RGBA 精确差异仍保留在原始样本中供审计，正确性判定则使用 Pixelmatch 的标准 `0.1` 感知阈值：肉眼不可见的 GPU/滤镜一级舍入可以通过，缺失或重复的合成 tile 仍会失败。冷启动计时统一把软件包导入放在计时内；常驻场景每个引擎只启动并稳定一个宿主，再对该宿主测量七个新客户端，而不是重复七次启动和预热。顺序批量和并发场景同样保留七轮及全部逐用例样本，但每个引擎/并发度只复用一个已稳定实例；冷启动与生命周期场景仍保持独立启动。测试记录全部样本，并且只终止由自身启动的 PID 进程树。仓库仅保存精简的 `permanent` 输出；PNG、日志和进程时间线作为 CI 工件保留 90 天。

使用 `benchmark` GitHub Actions workflow 测试已发布的精确语义版本
或 npm dist-tag。GitHub Release 创建后，发布流程会以精确发布版本触发同一基准测试。

手动定向诊断时，可以把 `platform_filter` 设为一个原生平台、把 `shard_filter`
设为一个场景分片，或同时指定两者。两个输入均保持 `all` 时仍执行完整的 30 任务。
任何带筛选的运行都会上传对应分片的数值结果和 Actions 详细证据，但会明确跳过平台
合并、仓库聚合和结果提交，绝不会把局部诊断伪装成完整归档。发布流程不传这两个可选
筛选项，因此仍保持六平台全量测试。

在同一 runner 系列积累至少五次可比的 full 结果之前，基准仅记录数据，不设置武断的
性能回归阈值；后续阈值策略需单独制定和评审。
