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

### First candidate observations (2026-09-09 07:06 China time)

Five platforms succeeded; macOS x64 remains queued. Successful build job times: Windows x64 9.6 min, Windows arm64 14.4 min, Linux x64 7.1 min, Linux arm64 7.0 min, macOS arm64 7.7 min. Each now uses one build runner. The differences include download/test variability, so do not attribute every saved minute to the code change. In particular, Windows x64 checks took 2.0 min versus approximately 7 min in the baseline. API snapshots are `out/ci-runtime/run-<id>.json`.

### Second candidate in flight

Commit `b9dd0d07ff499b834d253d99540004ad166a7848`, checks 34288235418 passed. Launched with `shards=1`, all runtime checks enabled:

- Windows x64: 34289154087
- Windows arm64: 34289159125
- Linux x64: 34289163175
- Linux arm64: 34289167429
- macOS arm64: 34289172394
- macOS x64: **not dispatched yet**, to avoid duplicating its queued first-candidate run. Dispatch after 34287922393 finishes, then collect its exact second-candidate SHA result.

Do not declare optimization complete until this candidate succeeds on all six platforms and the final timing report is committed. A live cache lookup for the final SHA should also confirm that auto selects one runner after those caches are saved. No extra release/tag is needed.

### Cache invalidation discovered during the second candidate

The second candidate's setup takes approximately four minutes, but ninja then recompiles extensively. `ci-stamp-mtimes.ts` assigned **every non-Git downloaded file the last DEPS commit timestamp**. Removing one Python preinstall hook therefore changes timestamps of unchanged clang/Rust inputs and invalidates downstream compilation. This is a source-level explanation pending confirmation from completed run logs; this run is not comparable to a warm version-only build.

A follow-up changes downloaded-input timestamp handling to compare SHA-256 fingerprints saved inside `out/Shot/ci-input-mtimes.json` alongside the build cache. Identical bytes retain their old mtime across DEPS edits; changed bytes receive a time newer than the cached ninja log, including when checking out an older commit. First use of an old cache retains the legacy DEPS-time fallback. All source actions restore the cache before stamping. Focused tests cover unchanged content, changed content with an older commit, new inputs, repeat use and consistent shard timestamps; TypeScript checks pass.

Wait for the running second-candidate builds to finish; do not cancel their rebuild work or start duplicate builds. Then validate this final cache fix on all six platforms with their newly populated caches. macOS x64 second-candidate dispatch can be replaced with the final candidate (which includes the hook removal), after its first candidate completes. Preserve the second-candidate timings as an invalidation case rather than claiming a warm speedup from them.
