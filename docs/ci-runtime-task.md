# CI runtime reduction after v0.5.0

User request: finish v0.5.0, then reduce CI duration and duplicate work autonomously. Keep all six platforms, existing runtime checks, binary provenance and rendering behavior. No new version release is authorized. Do not resume the old broad engine-cut task.

## Release

- v0.5.0: commit `442edbe7a9c346e66b07dc49871d1b8def3b32c2`.
- [Publish run 34286789834](https://github.com/sj817/shotium/actions/runs/34286789834) succeeded; six standalone archives attached to the public release.
- All seven npm packages observed at 0.5.0, including the delayed darwin-arm64 metadata (2026-09-09 China time).
- Clean Windows npm installation rendered successfully. The corrected local-file README example passed; the bundled README's old data URL example is unsupported. Repository README and bilingual release notes explain the correction.

## Baseline: version-only warm builds

Raw API timings and logs are under `out/release-0.5.0/` (ignored scratch).

| Platform | Run | Build job minutes | Sum of build/shard runner minutes |
| --- | --- | ---: | ---: |
| Windows x64 | 34284123397 | 26 | 55.6 |
| Windows arm64 | 34284135648 | 20 | 59.7 |
| Linux x64 | 34284141244 | 8 | 23.8 |
| Linux arm64 | 34284156206 | 16 | 25.9 |
| macOS x64 | 34284162365 | 27 | 47.0 |
| macOS arm64 | 34284176652 | 12 | 28.2 |

These are rounded job times, not billing-adjusted minutes. Linux x64 needed a retry after an upstream HTTP 503; its row is the successful jobs and is not a clean end-to-end baseline. Do not count failure recovery as an optimization gain.

Evidence: Windows x64 shards spent 6–16 minutes on setup, then ninja reported no work. The final job waited approximately nine minutes for peers. macOS x64 deleted spare Xcodes for 5–7 minutes despite 109 GiB already free. Its build cache restored successfully; dependency sync still took approximately nine minutes per runner.

## Batches and completion criteria

1. **Remove proven redundant setup**: adaptive single-runner selection for an available successful cache with only documentation/version changes; keep cold/source-change parallelism. Skip macOS Xcode deletion when at least 40 GiB is free. Correct registry verification so a 404 cannot produce a green publication gate. Implementation and focused local tests complete; six-platform CI measurement pending.
2. **Dependency preparation and remaining critical path**: removed the eager `vpython3_common` install hook and its pruning allowlist entry. Baseline hook cost: Windows x64 122.65 seconds, macOS x64 287.27 seconds. GN explicitly selects DEPS CPython; all 4,199 commands in the local Windows engine graph contain zero vpython calls. The repository vpython spec remains available on demand. Type check passes; cross-platform CI verification is pending. Other sync/cache changes require evidence that restoring another large cache is faster than downloading it.
3. **Measure and finish**: obtain successful six-platform candidate runs and compare wall time plus summed runner minutes against comparable warm runs. Report cold-build limitations honestly, record run URLs and remaining costs. Do not describe a forecast as measured savings. Still pending.

Keep the `shotium-ci` heartbeat active until these batches are complete. On unchanged CI state, remain quiet. Notify on a failure, required action, or verified meaningful outcome. Commit related changes in batches; no PR and no further release.

## First candidate in flight

Commit `0c1a945c7b44f6f880abf45a3afb093d06309569`, checks 34287898193 passed. These six runs explicitly select `shards=1` to measure the warm single-runner path; the adaptive classifier also passed a live GitHub-cache lookup locally. They do not include the subsequent vpython hook removal.

- Windows x64: 34287897157
- Windows arm64: 34287901594
- Linux x64: 34287906113
- Linux arm64: 34287917855
- macOS x64: 34287922393
- macOS arm64: 34287927557

Do not restart these while they run. Once they finish, capture timings and validate the second batch on its own exact commit. The post-release benchmark run 34287208480 is also active, with `max-parallel: 30`; at inspection four macOS benchmark jobs occupied runners and macOS x64 engine was queued. Record queue time separately from engine job time instead of attributing capacity contention to compilation. No benchmark results have been discarded.
