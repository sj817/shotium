# Benchmark result archive

Each immutable CI run lives at
`v<exact-version>/<YYYYMMDDTHHmmssZ>-gh<run-id>-a<attempt>/`. A complete run has
six npm-standard platform directories plus `manifest.json`, `report.md` and
`report.zh-CN.md` and `summary.csv`. `report.md` is English and
`report.zh-CN.md` is Simplified Chinese; `LATEST.md` links both versions.
Platform directories contain `summary.json`, `samples.jsonl`, `quality.json`
and `failures.json`.

The [VitePress benchmark explorer](https://sj817.github.io/shotium/) presents
the same archive with Chinese-first labels, within-platform formal rankings,
coverage exclusions, scenario filters and failure evidence.

Only same-runner ratios are reported. `n/a` means the competitor had no native
browser for that architecture; a supported engine that failed to install or
start remains a failure. PNGs, stdout/stderr and high-frequency process
timelines are retained as 90-day Actions artifacts whose name and content hash
are recorded in the manifest.

`legacy/` is outside the canonical series. It contains the retired local
benchmark solely so old README numbers remain auditable.

The initial series is record-only. No performance regression gate is enabled
until at least five comparable full runs exist on the same runner family; any
future threshold policy must be introduced and reviewed separately.
