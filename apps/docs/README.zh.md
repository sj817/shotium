# docs

[English](./README.md) · 简体中文

引擎的设计记录、README 嵌入的图片，以及基准测试归档。维护者的工作指南是仓库根目录的 [CLAUDE.md](../../CLAUDE.md)，这里的文档是它所指向的内容。

## 设计文档

| 文档 | 适用场景 / 核心主题 |
|---|---|
| [agent-reference.md](agent-reference.md) | 深入了解运行时与 API 约束、脚本约定、构建与 CI 注意事项，或测试方法 |
| [shotium-plan.md](shotium-plan.md) | 了解核心架构设计与技术决策：项目定位、整体分层（第 1 节）、网络栈选型与 `//net` 集成（第 2 节）、外部接口约定（第 4 节）、明确排除的非目标（第 5 节） |
| [cut-progress.md](cut-progress.md) | 追溯 Chromium 模块裁剪历史与判定依据：移除 V8 运行时（第 8 节）、恢复还是切除的判定准则（第 11 节）、剥离 `//content` 架构直接驱动 Blink（第 14 节）、二进制体积构成分析（第 17 节）、可用性与工程化完善（第 20 节）、CI 流水线建设（第 21 节）、基于构建依赖图裁剪源码树（第 22 节） |
| [upstream-sync.md](upstream-sync.md) | 上游 Chromium 同步指南：基线版本记录、分歧点设计原理、基于语义的重放流程以及同步后的完整性验证 |
| [upstream-sync-tool.md](upstream-sync-tool.md) | `pnpm upstream:sync` 定制化三方合并工具的设计与操作说明 |
| [upstream-sync-0.6.0-validation.md](upstream-sync-0.6.0-validation.md) | Chromium 155.0.8048.0 版本同步的完整验收记录（包含机器可读的配置状态与路径决策文件） |
| [performance.md](performance.md) | 基准性能对比测试方法论：测量环境控制、采样收敛条件与判定准则 |
| [scripts-to-typescript.md](scripts-to-typescript.md) | 自动化脚本工程化演化记录：手写脚本迁移至 TypeScript 工具链的背景与架构约定 |

大部分设计文档用中文写成；代码注释与 CLAUDE.md 是英文。

## 素材

`assets/` 存放根 README 嵌入的图片：`demo.gif`、`card.webp`、`example-node.webp`、`example-cli.webp`。它们由 `pnpm docs:assets` 与 `pnpm docs:demo` 从 [apps/demo-card](../demo-card/README.zh.md) 生成，不手工编辑。

## 基准测试

`benchmarks/` 是每次已发布基准运行的归档，`v<version>/` 下每次运行一个不可变目录，顶层有 `index.json` 与 `LATEST.md`。[benchmarks/README.zh.md](benchmarks/README.zh.md) 描述其布局；[apps/benchmark](../benchmark/README.zh.md) 产出它，[apps/benchmark-site](../benchmark-site/README.zh.md) 发布它。

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)

