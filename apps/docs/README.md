# docs

English · [简体中文](./README.zh.md)

The design records of the engine, the images the READMEs embed, and the benchmark archive. The maintainer's working guide is [CLAUDE.md](../../CLAUDE.md) at the repository root; the documents here are what it points to.

## Design documents

| Document | Scope and Topics |
|---|---|
| [agent-reference.md](agent-reference.md) | Runtime and API contracts, repository tooling conventions, build and CI guidelines, and testing methodologies |
| [shotium-plan.md](shotium-plan.md) | Core architecture and engineering decisions: project scope, component layers (Section 1), network stack selection (Section 2), public API contracts (Section 4), and non-goals (Section 5) |
| [cut-progress.md](cut-progress.md) | Historical record of Chromium component trimming: V8 decoupling (Section 8), restore-or-cut criteria (Section 11), driving Blink without `//content` (Section 14), binary size breakdown (Section 17), usability milestones (Section 20), CI pipelines (Section 21), and tree trimming based on GN build graphs (Section 22) |
| [upstream-sync.md](upstream-sync.md) | Chromium upstream synchronization: baseline pinning, reasons against git-merge, semantic replay workflows, deliberate divergence points, and post-sync verification |
| [upstream-sync-tool.md](upstream-sync-tool.md) | Architecture and usage guide for `pnpm upstream:sync` scoped 3-way merger tool |
| [upstream-sync-0.6.0-validation.md](upstream-sync-0.6.0-validation.md) | Validation and acceptance log for the Chromium 155.0.8048.0 sync, accompanied by machine-readable state and decision records |
| [performance.md](performance.md) | A/B benchmarking methodology: environment isolation, adaptive sampling stopping rules, and metric interpretation |
| [scripts-to-typescript.md](scripts-to-typescript.md) | Repository automation migration history: replacing ad-hoc scripts with typed tooling and standard libraries |

Most design documents are written in Chinese; the code comments and CLAUDE.md are in English.

## Assets

`assets/` holds the images the root README embeds: `hero.svg` and `hero.zh.svg`, the two comparison cards at the top, drawn by `pnpm docs:hero` from the newest archive under `benchmarks/`; and `card.webp`, `example-node.webp` and `example-cli.webp`, generated from [apps/demo-card](../demo-card/README.md) by `pnpm docs:assets`. `demo.gif` is the terminal recording `pnpm docs:demo` makes from the same card; the README no longer embeds it. None of them is edited by hand.

## Benchmarks

`benchmarks/` is the archive of every published benchmark run, one immutable directory per run under `v<version>/`, with `index.json` and `LATEST.md` at the top. [benchmarks/README.md](benchmarks/README.md) describes the layout; [apps/benchmark](../benchmark/README.md) produces it and [apps/benchmark-site](../benchmark-site/README.md) publishes it.

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)

