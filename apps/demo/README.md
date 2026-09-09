# Node.js demo

The complete `@shotkit/shotium` npm package now lives in [shotium/](shotium/README.md), including TypeScript sources, Node-API sources, package metadata and build configuration. It is still published under the same npm name; this directory is not a pnpm workspace.

完整 npm 源码现位于 `apps/demo/shotium/`，发布名称仍为 `@shotkit/shotium`。仓库脚本、GN 和 CI 均使用新路径，不再使用根目录 `shotium/`。

Run from the repository root after building the local addon:

```bash
pnpm build:engine --target shot_node
pnpm -C apps/demo/shotium install --no-lockfile
pnpm -C apps/demo/shotium run build
pnpm -C apps/demo install --frozen-lockfile
pnpm -C apps/demo start
```

The Express example serves the local Faruzan card, writes `apps/demo/faruzan.png`, then stops the runtime and HTTP server. It imports the sibling package's built output so the demo exercises the migrated source. To use the published package in your own application, install `@shotkit/shotium` and import it by that name.

示例启动本地 Express 静态服务器，截图写入 `apps/demo/faruzan.png`，随后停止 Runtime 与 HTTP 服务。这里直接导入同目录的 npm 构建产物，确保测试的是迁移后的源码；业务项目请正常安装并导入 npm 包。
