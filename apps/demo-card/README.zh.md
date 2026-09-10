# demo-card

[English](./README.md) · 简体中文

提供 shotium 各语言示例统一使用的登机牌卡片模板：包含单文件自包含 HTML、Node.js 截图调用脚本，以及主 README 示例动图的录制源文件。

## 文件

| 文件 | 作用 |
|---|---|
| `card.html` | 登机牌页面。采用内联 CSS 设计且无外部子资源依赖，标准视口尺寸为 720×380 CSS 像素。本文件为权威源，`apps/{go,python,rust,csharp,java}` 中的同名卡片均由此文件同步生成 |
| `card.mjs` | 根目录 README 演示的 Node.js 脚本：演示 `start()` 启动引擎、执行截图及 `stop()` 资源释放流程 |
| `cli-session.txt` | 命令行交互终端录制文本，用于生成 `apps/docs/assets/example-cli.webp` |
| `demo.tape` | 用于生成 `apps/docs/assets/demo.gif` 的 [VHS](https://github.com/charmbracelet/vhs) 录屏脚本 |

## 用法

在更新 `card.html` 后，将变更同步至各个语言示例工程：

```bash
pnpm demo:sync           # 把 card.html 复制到 apps/{go,python,rust,csharp,java}/
pnpm demo:sync --check   # CI 一致性检查：若任一语言示例目录的副本与源文件不一致则报错
```

当卡片样式或演示脚本变更后，可重新构建文档媒体资产：

```bash
pnpm docs:assets   # 生成 card.webp、example-node.webp、example-cli.webp（需安装 freeze 与 ffmpeg）
pnpm docs:demo     # 生成 demo.gif（需安装 vhs、ttyd、ffmpeg 与 bash）
```

上述构建命令会在当前目录创建临时测试项目 `.demo-run/` 并安装发布版 npm 包，以确保文档录屏反映真实端到端的使用体验。该目录已被 `.gitignore` 忽略。

通过 CLI 本地手动渲染卡片：

```bash
shotium card.html --width 720 --height 380 --scale 2 -o card.png
```

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)

