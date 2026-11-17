# docs

English · [简体中文](./README.zh.md)

The engine's reference documents, the images the READMEs embed, and the benchmark archive. The maintainer's working guide is [CLAUDE.md](../../CLAUDE.md) at the repository root; the documents here are what it points to.

## Design documents

| Document | Scope and Topics |
|---|---|
| [agent-reference.md](agent-reference.md) | Runtime and API contracts, repository tooling conventions, build and CI guidelines, and testing methodologies |
| [upstream-sync.md](upstream-sync.md) | Chromium upstream synchronization: baseline pinning, reasons against git-merge, semantic replay workflows, deliberate divergence points, and post-sync verification |
| [upstream-sync-tool.md](upstream-sync-tool.md) | Architecture and usage guide for `pnpm upstream:sync` scoped 3-way merger tool |
| [performance.md](performance.md) | A/B benchmarking methodology: environment isolation, adaptive sampling stopping rules, and metric interpretation |

Only documents that describe the current tree are kept here. The reasoning and measurements behind a change are in its commit message and pull request; one-off work logs (cutting rounds, audits, sync acceptance records) were removed and remain in `git log`.

## Assets

`assets/` holds the images the root README embeds: `hero.svg` and `hero.zh.svg`, the two comparison cards at the top, drawn by `pnpm docs:hero` from the newest archive under `benchmarks/`; and `card.webp`, `example-node.webp` and `example-cli.webp`, generated from [apps/demo-card](../demo-card/README.md) by `pnpm docs:assets`. `demo.gif` is the terminal recording `pnpm docs:demo` makes from the same card; the README no longer embeds it. None of them is edited by hand.

## Benchmarks

`benchmarks/` is the archive of every published benchmark run, one immutable directory per run under `v<version>/`, with `index.json` and `LATEST.md` at the top. [benchmarks/README.md](benchmarks/README.md) describes the layout; [apps/benchmark](../benchmark/README.md) produces it and [apps/benchmark-site](../benchmark-site/README.md) publishes it.

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)

