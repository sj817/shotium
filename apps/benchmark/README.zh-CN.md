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
npm ci
npm run benchmark -- --shotium-version 0.3.2 --profile smoke --output ./out --seed local-check
```

如需仅运行一个场景分片，可追加 `--shard startup`、`--shard throughput`、
`--shard parallel`、`--shard resident` 或 `--shard resilience`。省略该选项
（或传入 `--shard all`）时，仍按本机单任务方式运行全部场景：

```bash
npm run benchmark -- --shotium-version 0.3.2 --profile full --shard throughput --output ./out --seed local-check
```

分片边界固定为：`startup` 包含冷启动、冷启动稳定后首张截图和生命周期；
`throughput` 包含暖机和批量；`parallel` 单独包含并发场景；`resident` 包含常驻
客户端和页面复用；`resilience` 包含故障及浸泡测试。

CI 会展开为 30 个 `平台 x 场景分片` 矩阵任务。每个分片仍在同一台原生 runner
上以平衡顺序测试所有可用引擎，因此同一场景内的比较仍是同机比较。五个分片会先
合并为一个平台结果，再聚合六个平台；runner 信息保留在各分片中，不会把不同分片
或不同平台的耗时混在同一个排名里。

如需在同一台机器上直接比较源码构建的可执行文件，可运行：

```bash
npm run benchmark:native -- --baseline-executable /path/to/headless_shell --baseline-engine headless-shell --shot-executable /path/to/shotium --iterations 5 --warmup-iterations 1 --output ./out-native
```

JSON/CSV 报告包含原始样本、经过校验的 PNG 元数据、可执行文件 SHA-256/版本，
以及同机 `基线 p50 / Shot p50` 比率。
每个平台还会单独生成几何平均综合排名。只有 Shotium 与对比引擎在同一场景、
同一并发度下均为“通过”且允许排名的测试项才会参与；归一化相对耗时越低越好。
报告会列出覆盖数和单项冠军次数，并且绝不跨平台混排。只有覆盖本平台全部可比项的
引擎才会获得正式名次；部分覆盖仍展示成绩，但会明确标记为不授予名次。

如需只重新生成某次归档的 Markdown/CSV 展示层（包括旧的四分片归档），运行：

```bash
npm run render-report -- --result-directory ../../benchmark-results/v0.3.2/<归档目录>
```

该命令读取已归档的 manifest 和各平台 summary，只替换 `report.md`、
`report.zh-CN.md`、`summary.csv`，以及已有索引对应的 `LATEST.md`；不会修改原始样本、
质量记录、失败证据或 manifest。报告顶部会链接到
[VitePress 基准站点](https://sj817.github.io/shotium/)。

`full` 配置包含七次冷启动重复、1/2/4 并发、20 次生命周期循环，以及连续 1000 次
请求或十分钟的浸泡测试。每个测试单元都会等待主机稳定；非冷启动单元还会等待引擎
达到实测就绪状态。主机稳定门槛按分片校准：第一个单元开始前先采样 5 秒空载 CPU，
门槛取 `max(25%, 空载 p95 + 10 个百分点)`，上限 80%——GitHub 的 Windows 和 macOS
runner 空载就有 28–41% CPU，固定 25% 的门槛在那里永远过不去。稳定的定义是连续
三个一秒采样都低于门槛且空闲内存平稳；六秒内达不到的单元标为噪声并重试一次。
测试记录全部样本，并且只终止由自身启动的 PID 进程树。仓库仅保存
精简的 `permanent` 输出；PNG、日志和进程时间线作为 CI 工件保留 90 天。

使用 `Six-platform benchmark` GitHub Actions workflow 测试已发布的精确语义版本
或 npm dist-tag。GitHub Release 创建后，发布流程会以精确发布版本触发同一基准测试。

手动定向诊断时，可以把 `platform_filter` 设为一个原生平台、把 `shard_filter`
设为一个场景分片，或同时指定两者。两个输入均保持 `all` 时仍执行完整的 30 任务。
任何带筛选的运行都会上传对应分片的数值结果和 Actions 详细证据，但会明确跳过平台
合并、仓库聚合和结果提交，绝不会把局部诊断伪装成完整归档。发布流程不传这两个可选
筛选项，因此仍保持六平台全量测试。

在同一 runner 系列积累至少五次可比的 full 结果之前，基准仅记录数据，不设置武断的
性能回归阈值；后续阈值策略需单独制定和评审。
