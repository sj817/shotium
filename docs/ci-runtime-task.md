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
2. **Dependency preparation and remaining critical path**: inspect sync/download and cache logs across platforms; remove or reuse demonstrably duplicated work without stale toolchains, deleting live dependencies or hiding failed checks. Decide from measurements whether another cache/preparation stage actually saves time. Still pending.
3. **Measure and finish**: obtain successful six-platform candidate runs and compare wall time plus summed runner minutes against comparable warm runs. Report cold-build limitations honestly, record run URLs and remaining costs. Do not describe a forecast as measured savings. Still pending.

Keep the `shotium-ci` heartbeat active until these batches are complete. On unchanged CI state, remain quiet. Notify on a failure, required action, or verified meaningful outcome. Commit related changes in batches; no PR and no further release.
