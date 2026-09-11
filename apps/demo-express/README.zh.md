# demo-express

[English](./README.md) · 简体中文

Node.js 服务端集成示例：演示 Express 通过 HTTP 托管[角色资料卡片](../demo-genshin-card/README.zh.md)页面，使用 `@pixel.js/shotium` 截取目标 DOM 元素，并完成进程优雅退出。

## 运行

在仓库根目录下执行，前提是本地引擎产物已构建完成：

```bash
pnpm build:engine --target shot_node                 # out/Shot/shotium.node
pnpm -C apps/typescript install --no-lockfile
pnpm -C apps/typescript run build                    # apps/typescript/dist
pnpm -C apps/demo-express install --frozen-lockfile
pnpm -C apps/demo-express start
```

`index.js` 在随机空闲端口启动 Express 服务，调用 `shotium.start({ cacheDir: null })` 初始化引擎，将页面中的 `#card` 元素渲染截图并保存为 `apps/demo-express/faruzan.png`。脚本会打印渲染耗时及总耗时统计，并在完成后依次关闭 shotium 运行时与 HTTP 服务。

本示例直接引用本地构建产物 `../typescript/dist/index.js` 以测试当前仓库代码。在实际应用项目中，直接从 npm 安装 `@pixel.js/shotium` 依赖即可。

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)

