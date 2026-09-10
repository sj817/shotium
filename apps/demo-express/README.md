# demo-express

English · [简体中文](./README.zh.md)

Node.js server integration example: Express serves the [character profile card](../demo-genshin-card/README.md) over HTTP, `@shotkit/shotium` captures the target DOM element, and both perform a graceful shutdown.

## Run

From the repository root, ensuring the local engine has already been built:

```bash
pnpm build:engine --target shot_node                 # out/Shot/shotium.node
pnpm -C apps/typescript install --no-lockfile
pnpm -C apps/typescript run build                    # apps/typescript/dist
pnpm -C apps/demo-express install --frozen-lockfile
pnpm -C apps/demo-express start
```

`index.js` binds Express to an ephemeral port, initializes the engine via `shotium.start({ cacheDir: null })`, captures the `#card` element into `apps/demo-express/faruzan.png`, logs render and total latency metrics, and cleanly tears down both the shotium runtime and HTTP server.

The demo directly imports `../typescript/dist/index.js` to validate in-tree package builds. For external projects, install `@shotkit/shotium` from npm and import by package name.

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)

