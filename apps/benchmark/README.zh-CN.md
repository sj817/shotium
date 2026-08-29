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
`--shard resident` 或 `--shard resilience`。省略该选项（或传入 `--shard all`）时，
仍按本机单任务方式运行全部场景：

```bash
npm run benchmark -- --shotium-version 0.3.2 --profile full --shard throughput --output ./out --seed local-check
```

CI 会展开为 24 个 `平台 x 场景分片` 矩阵任务。每个分片仍在同一台原生 runner
上以平衡顺序测试所有可用引擎，因此同一场景内的比较仍是同机比较。四个分片会先
合并为一个平台结果，再聚合六个平台；runner 信息保留在各分片中，不会把不同分片
或不同平台的耗时混在同一个排名里。

如需在同一台机器上直接比较源码构建的可执行文件，可运行：

```bash
npm run benchmark:native -- --baseline-executable /path/to/headless_shell --baseline-engine headless-shell --shot-executable /path/to/shotium --iterations 5 --warmup-iterations 1 --output ./out-native
```

JSON/CSV 报告包含原始样本、经过校验的 PNG 元数据、可执行文件 SHA-256/版本，
以及同机 `基线 p50 / Shot p50` 比率。

`full` 配置包含七次冷启动重复、1/2/4 并发、20 次生命周期循环，以及连续 1000 次
请求或十分钟的浸泡测试。每个测试单元都会等待主机稳定；非冷启动单元还会等待引擎
达到实测就绪状态。测试允许重试一次有噪声的稳定过程，记录全部样本，并且只终止由
自身启动的 PID 进程树。仓库仅保存
精简的 `permanent` 输出；PNG、日志和进程时间线作为 CI 工件保留 90 天。

使用 `Six-platform benchmark` GitHub Actions workflow 测试已发布的精确语义版本
或 npm dist-tag。GitHub Release 创建后，发布流程会以精确发布版本触发同一基准测试。

在同一 runner 系列积累至少五次可比的 full 结果之前，基准仅记录数据，不设置武断的
性能回归阈值；后续阈值策略需单独制定和评审。
