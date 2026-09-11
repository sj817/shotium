# 六平台基准测试

[English](README.md)

本工程是 Shotium 的标准化跨平台性能与韧性评测套件。套件评估 Shotium 以及在当前测试宿主上具备原生二进制支持的各主流浏览器自动化方案。对于目标平台无原生二进制支持的竞品记录为 `n/a`；而在受支持平台上发生的安装或启动异常则严格归类为失败。

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

如需仅运行一个场景分片，可追加 `--shard startup`、`--shard throughput`、`--shard parallel`、`--shard resident` 或 `--shard resilience`。省略该选项（或传入 `--shard all`）时，仍按本机单任务方式运行全部场景：

```bash
pnpm run benchmark -- --shotium-version 0.3.2 --profile full --shard throughput --output ./out --seed local-check
```

分片边界固定为：`startup` 包含冷启动、冷启动稳定后首张截图和生命周期；`throughput` 包含暖机和批量；`parallel` 单独包含并发场景；`resident` 包含常驻客户端和页面复用；`resilience` 包含故障及浸泡测试。

CI 会展开为 30 个 `平台 x 场景分片` 矩阵任务。每个分片仍在同一台原生 runner 上以平衡顺序测试所有可用引擎，因此同一场景内的比较仍是同机比较。五个分片会先合并为一个平台结果，再聚合六个平台；runner 信息保留在各分片中，不会汇总不同分片或不同平台的原始耗时。需要跨分片汇总时，只对同一 runner 内测得的同测试项相对比率做几何聚合。

如需在同一台机器上直接比较源码构建的可执行文件，可运行：

```bash
pnpm run benchmark:native -- --baseline-executable /path/to/headless_shell --baseline-engine headless-shell --shot-executable /path/to/shotium --iterations 5 --warmup-iterations 1 --output ./out-native
```

JSON/CSV 报告包含原始样本、经过校验的 PNG 元数据、可执行文件 SHA-256/版本，以及同机 `基线 p50 / Shot p50` 比率。被测目标包含五个引擎配置变体：Shotium 原生引擎，以及由 Puppeteer 与 Playwright 分别驱动的完整 Chrome 和 headless shell。

每个质量通过的平台还会单独生成几何平均综合排名。只有 Shotium 与对比引擎在同一场景、同一并发度下均为“通过”且允许排名的测试项才会参与；归一化相对耗时越低越好。报告会列出覆盖数和单项胜出次数，并且绝不跨平台混排。只有覆盖本平台全部可比项的引擎才会获得正式名次；部分覆盖仍展示成绩，但会明确标记为不授予名次。失败、波动、缺分片或缺证据的平台保留诊断数据，但不生成正式名次或首位排名。

测试设计专注于一个明确收窄的问题域：各开箱即用引擎变体在使用其标准浏览器二进制时，执行静态 HTML/CSS 页面截图任务的综合表现。测试不代表通用的 JavaScript 交互或动态自动化能力。所有引擎均接受完全相同的并发请求、视口大小、缓存配置、渲染语料、PNG 输出格式与超时控制；测试如实反映各实现底层的进程拓扑与资源消耗差异。

如需仅重新生成某次归档的 Markdown/CSV 展示层（包括旧的四分片归档），可运行：

```bash
pnpm run render-report -- --result-directory ../../apps/docs/benchmarks/v0.3.2/<归档目录>
```

该命令读取已归档的 manifest 和各平台 summary，只替换 `report.md`、`report.zh-CN.md`、`summary.csv`，以及已有索引对应的 `LATEST.md`；不会修改原始样本、质量记录、失败证据或 manifest。报告顶部会链接到 [VitePress 基准站点](https://sj817.github.io/shotium/)。

### 详细度量规则与环境隔离

- **测试规模**：`full` 配置包含 7 次冷启动重复、1/2/4 并发阶梯、20 次生命周期循环，以及 1000 次请求连续浸泡（或 10 分钟上限）
- **预热与采样**：非冷启动单元固定预热 3 次；预热延迟变异系数（CV）与进程树 RSS 漂移作为引擎诊断数据记录；主机稳定性检测在每个分片起始阶段采样 5 秒空载 CPU（同时运行进程采样器），动态门限设定为 `max(25%, 空载 p95 + 10%)`；采样器 CPU 占用严格限制在单核 20% 以内，实测采样间隔记入 `observed_mean_period_ms`
- **重试与限时**：单测试单元若在 6 秒内未等到静默主机则标记为 `noisy` 并重试一次（最多等待 15 秒）；分片执行若耗尽分配的时间预算即停止调度后续单元，并保留已生成的测试结果与现场证据
- **图像判定**：导航与截图统一配置 30 秒超时上限；Puppeteer/Playwright 适配器在 `load` 后等待网络和两帧渲染后触发截图，以对齐 Shotium 内部严格的 paint-clean 生命周期；渲染正确性校验基于 Pixelmatch 算法（感知阈值 `0.1`），过滤不可见的 GPU 舍入误差，同时精准捕获图块缺失或合成异常
- **页面模型**：每次截图新建页面、结束后关闭，页面复用是独立场景；Puppeteer 的页面按独立窗口创建。Chrome 自带的 headless 模式会把新开的页面放成后台标签页，而后台标签页不出帧：`visibilityState` 为 `hidden`，`requestAnimationFrame` 不回调，合成表面为空或过期，并发截图因此会拿到空图、与同一用例首图不一致的帧，以及一路等到超时上限的就绪等待。Playwright 的页面和 Chrome 的 headless shell 本来就是可见的，这条只是消除适配器之间的不对称；除下文 Playwright 的窗口尺寸外，启动参数仍保持各软件包默认。此改动之前归档中的 Puppeteer Chrome 并发数据，同时也测进了这些隐藏页面等待的标签页激活。Playwright 的页面视口来自窗口本身，而不是设备仿真。给 context 设了视口后，Playwright 会把每个新页面的 headless 窗口设成视口大小，当作窗口没有浏览器界面，但 Chrome 自带的 headless 模式仍保留标签条和工具栏（macOS 87 px；Windows 连边框 16 × 95 px），标签页容器因此比视口小。设备仿真照样把页面的 widget 撑到视口大小，可在 macOS 上，只要有同级标签页关闭、页面被重新挂回容器，widget 就缩回容器尺寸，下一次 `Page.captureScreenshot` 只能一边把 widget 撑大一边拷贝；拷贝偶尔落在改尺寸之前的那一帧，Chromium 会把它平铺成视口大小（第 633–719 行重复第 0–86 行）。macOS 上反复出现的 playwright-chrome 证据失败全是这种帧，且只在两个及以上页面时发生。用 CDP 逐页放大窗口能消掉它，但 Playwright 每开一页都会再把窗口缩回去，每张图多两次真实的窗口 resize，Intel macOS runner 上 playwright-chrome 延迟涨了 70–85 %。因此适配器启动 Playwright 时传 `--window-size`（视口加实测边距）和 `--force-device-scale-factor=1`，context 用 `viewport: null`：widget 一出生就是视口大小，没有仿真也没有 resize，fixture 的输出与仿真视口逐字节相同。每个分片在第一个 cell 之前实测一次边距，记录在 `engines[].window_inset`；边距量不出来的 Playwright 引擎直接跳过，而不是按错误尺寸截图
- **数据留存**：常驻场景复用稳定引擎宿主测量多个客户端请求；顺序批量与并发场景在稳定实例上收集 7 轮样本；代码库仅提交精简的汇总结果，渲染 PNG、控制台日志与细粒度时间线在 CI 中保留 90 天

使用 `benchmark` GitHub Actions workflow 测试已发布的精确语义版本或 npm dist-tag。GitHub Release 创建后，发布流程会以精确发布版本触发同一基准测试。

手动定向诊断时，可以把 `platform_filter` 设为一个原生平台、把 `shard_filter` 设为一个场景分片，或同时指定两者。两个输入均保持 `all` 时仍执行完整的 30 任务。任何带筛选的运行都会上传对应分片的数值结果和 Actions 详细证据，但会明确跳过平台合并、仓库聚合和结果提交，绝不会把局部诊断伪装成完整归档。

在同一 runner 系列积累至少五次可比的 full 结果之前，基准仅记录数据，不设置武断的性能回归阈值；后续阈值策略需单独制定和评审。
