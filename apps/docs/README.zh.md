# docs

[English](./README.md) · 简体中文

引擎的参考文档、README 嵌入的图片，以及基准测试归档。维护者的工作指南是仓库根目录的 [CLAUDE.md](../../CLAUDE.md)，这里的文档是它所指向的内容。

## 设计文档

| 文档 | 适用场景 / 核心主题 |
|---|---|
| [agent-reference.md](agent-reference.md) | 深入了解运行时与 API 约束、脚本约定、构建与 CI 注意事项，或测试方法 |
| [upstream-sync.md](upstream-sync.md) | 上游 Chromium 同步指南：基线版本记录、分歧点设计原理、基于语义的重放流程以及同步后的完整性验证 |
| [upstream-sync-tool.md](upstream-sync-tool.md) | `pnpm upstream:sync` 定制化三方合并工具的设计与操作说明 |
| [performance.md](performance.md) | 基准性能对比测试方法论：测量环境控制、采样收敛条件与判定准则 |

这里只保留描述当前代码树的文档。每项改动的理由和实测数据写在它的提交信息和 PR 里；一次性的工作记录（裁剪流水账、审计、同步验收记录）已删除，需要时查 `git log`。

## 素材

`assets/` 存放根 README 嵌入的图片：开头的两张对比卡片 `hero.svg`、`hero.zh.svg`，由 `pnpm docs:hero` 从 `benchmarks/` 下最新一份归档画出；`card.webp`、`example-node.webp`、`example-cli.webp` 由 `pnpm docs:assets` 从 [apps/demo-card](../demo-card/README.zh.md) 生成。`demo.gif` 是 `pnpm docs:demo` 用同一张卡片录的终端演示，README 不再嵌入它。这些文件都不手工编辑。

## 基准测试

`benchmarks/` 是每次已发布基准运行的归档，`v<version>/` 下每次运行一个不可变目录，顶层有 `index.json` 与 `LATEST.md`。[benchmarks/README.zh.md](benchmarks/README.zh.md) 描述其布局；[apps/benchmark](../benchmark/README.zh.md) 产出它，[apps/benchmark-site](../benchmark-site/README.zh.md) 发布它。

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)

