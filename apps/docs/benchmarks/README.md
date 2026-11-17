# Benchmark result archive

Each immutable CI run lives at `v<exact-version>/<YYYYMMDDTHHmmssZ>-gh<run-id>-a<attempt>/`. A complete run has six npm-standard platform directories plus `manifest.json`, `report.md`, `report.zh-CN.md`, and `summary.csv`. `report.md` is English and `report.zh-CN.md` is Simplified Chinese; `LATEST.md` links both versions. Platform directories contain `summary.json`, `quality.json`, and `failures.json`. The raw per-cell samples (`samples.jsonl`) are validated and hashed into `manifest.json` but not committed: they were most of the archive's size. They are kept for 90 days in the run's `benchmark-samples-<run-id>-a<attempt>` Actions artifact. Runs archived before this change still carry their `samples.jsonl`.

The [VitePress benchmark explorer](https://sj817.github.io/shotium/) presents the same archive with Chinese-first labels, within-platform formal rankings, coverage exclusions, scenario filters, and failure evidence.

Ratios are strictly computed against runs on identical runner hardware specifications. An `n/a` entry indicates the competitor does not provide a native browser build for that target architecture; engines that are nominally supported but fail to install or initialize are counted as failures. Rendered PNGs, process stdout/stderr logs, and fine-grained process timelines are retained as 90-day Actions artifacts, with names and SHA-256 hashes recorded in `manifest.json`.

`experiments/` holds the one local measurement the root README cites as evidence: the [2026-10-11 CPU CSS restoration](experiments/20261011T030349Z-cpu-css-effects/report.zh-CN.md), with paired Windows size and memory measurements. It is outside the published CI series and is not added to its index or rankings.

The initial benchmark series is observational. Regression gates remain disabled until at least five comparable full runs have been collected across the same runner family; any future gating threshold will be introduced through separate review.
