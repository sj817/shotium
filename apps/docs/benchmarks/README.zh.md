# 基准测试结果归档

[English](./README.md) · 简体中文

每次不可变的 CI 运行位于 `v<精确版本>/<YYYYMMDDTHHmmssZ>-gh<run-id>-a<attempt>/`。一次完整的运行包含六个按 npm 平台命名的目录，外加 `manifest.json`、`report.md`、`report.zh-CN.md` 与 `summary.csv`。`report.md` 是英文，`report.zh-CN.md` 是简体中文，`LATEST.md` 同时链接两个版本。平台目录里是 `summary.json`、`samples.jsonl`、`quality.json` 与 `failures.json`。

[VitePress 基准站点](https://sj817.github.io/shotium/)以同一份归档为数据源，提供中文优先的标签、平台内的正式排名、覆盖范围排除项、场景筛选与失败证据。

性能指标严格基于同一 runner 硬件规格采集与比对。`n/a` 表示对应竞品未提供该架构的原生浏览器构建；已支持但因安装或启动异常中断的用例统一按失败统计。生成图片、标准输出/错误日志及高频进程采样记录作为 GitHub Actions 工件留存 90 天，其工件名称与内容 SHA-256 哈希均记录在 `manifest.json` 中。

`legacy/` 目录存放早期本地基准测试历史数据，脱离于当前的标准化评测序列，仅用于追溯历史文档引用的基准数据。

当前阶段基准数据仅用于持续记录与观察，暂未配置硬性性能门禁。在同一规格 runner 集群上累计至少五次完整可比的基准记录前，不设置性能阻断阈值；后续任何门禁阈值调整需通过专门评审确定。
