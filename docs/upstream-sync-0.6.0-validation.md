# 0.6.0 上游同步验收

2026-09-09 发布完成。发布源码与全部引擎产物均为
`07ac95e4a85dad12bc7fd8029c5ca229e3e38d9f`，标签 `v0.6.0`。
上游保留切片基线为 Chromium `c099bd180a2db0fa6a313d43653529ba02665c84`
（155.0.8048.0）。源码决策见 `upstream-sync-decisions-155.json`。

## 发布证据

| 平台 | 成功运行 |
|---|---|
| Windows x64 | https://github.com/sj817/shotium/actions/runs/34325268543 |
| Windows ARM64 | https://github.com/sj817/shotium/actions/runs/34325275963 |
| Linux x64 | https://github.com/sj817/shotium/actions/runs/34325281648 |
| Linux ARM64 | https://github.com/sj817/shotium/actions/runs/34325286199 |
| macOS x64 | https://github.com/sj817/shotium/actions/runs/34325292783 |
| macOS ARM64 | https://github.com/sj817/shotium/actions/runs/34325297400 |

发布流水线：https://github.com/sj817/shotium/actions/runs/34339033452

GitHub Release：https://github.com/sj817/shotium/releases/tag/v0.6.0

七个 npm 包（主包及六个平台包）均通过 registry 的 0.6.0 版本与 integrity
核对；GitHub 六个 `.7z` 附件均处于 uploaded 状态，Release 非草稿。
各平台执行其工作流支持的检查；Windows/Linux ARM64 的交叉编译不代表在
原生 ARM64 设备执行了全部运行测试。

本地集成提交 `1e25a9bf414e2f6b45c77a0ea8ffc1fe6f420bed` 的 EXE、DLL、
Node/daemon、网络、84 demos、Bilibili 验收通过；basic/layout/paint/text
四组升级前后像素与哈希一致。最终提交修复 SDK 门禁和 Jumbo 头文件检查，
并以以上六平台 CI 完成最终验收。旧编码输入仍按现有 UTF-8 装载行为处理。
这些检查不代表全网站兼容性或性能无回退。

## CI 分片路径修复

`actions/download-artifact@v8` 在匹配单个 artifact 时直接下载到目标目录，
即 `shards/shard.tar`；多个 artifact 则位于 `shards/<name>/shard.tar`。
原工作流只寻找第二种路径。macOS x64 默认总计两个分片，恰好只下载一个
外部分片，导致合并循环为空；其最终日志没有 `merged into` 输出，随后
又执行 1056 个构建任务，09:14:22–09:59:39 UTC 约 45 分钟（包含必要链接，
不能把整个阶段当成可节省时间）。

发布后修复三个平台的路径查找，同时支持单层和嵌套布局。Bash 与 PowerShell
均通过 single=1、multiple=2、empty=0 的文件发现夹具验证。该工作流修复
不包含在 v0.6.0 标签内，尚未通过后续实际构建量化提速；无需为它重建已经
验收的发布二进制。本收尾提交不修改引擎源码。
