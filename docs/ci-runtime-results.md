# CI runtime optimization results

Complete: all six platforms passed the final code candidate `db6f03a6ccfbb48c26d607e25debf199457a8fe8`, with their existing checks and nonempty, unexpired standalone-binary and npm-package artifacts. All six build caches persisted and all four toolchain caches exist. v0.5.0 is already published and is unaffected; no new version was released.

Observed warm build/shard runner time fell from 240.20 to 42.62 minutes, about 82%. The longest individual build job fell from 26.55 to 10.52 minutes, about 60%. These runs were deliberately staggered to reuse cache fills, so this is not a measurement of one simultaneous six-platform dispatch-to-release wall clock. Including final metadata jobs gives 44.30 runner minutes; runner billing multipliers are not applied.

## Verified final-candidate measurements

Times below are build-job wall minutes, including setup, original platform checks, packaging and cache save. They exclude queueing and the metadata job. Single samples include runner/network variation; these are not statistical speedup guarantees.

| Platform | Earlier warm build job | Final warm build job | Final run |
| --- | ---: | ---: | --- |
| Windows arm64 | 19.92 min | 8.65 min | [34298127374](https://github.com/sj817/shotium/actions/runs/34298127374) |
| Linux x64 | 8.30 min, successful retry | 4.75 min | [34298131161](https://github.com/sj817/shotium/actions/runs/34298131161) |
| Linux arm64 | 15.83 min | 4.12 min | [34298135058](https://github.com/sj817/shotium/actions/runs/34298135058) |
| macOS arm64 | 11.55 min | 6.93 min | [34298145170](https://github.com/sj817/shotium/actions/runs/34298145170) |
| Windows x64 | 25.65 min | 7.65 min | [34302276662](https://github.com/sj817/shotium/actions/runs/34302276662) |
| macOS x64 | 26.55 min | 10.52 min | [34301499213](https://github.com/sj817/shotium/actions/runs/34301499213) |

The earlier baseline used multiple compilation runners in addition to the final build runner; its cumulative runner cost is recorded in `ci-runtime-task.md`. Final warm runs use one build runner. The Linux x64 baseline had an upstream HTTP 503 retry: do not count recovery time as an optimization gain. Windows/Linux arm64 retain their existing cross-compilation verification limits; a green build does not imply native ARM runtime checks were added.

All six completed final runs restored the exact compiler-toolchain cache and saved a new build-directory cache visible through GitHub's API. The new save wrapper's primary path passed; a real reservation failure has not been injected to test its retry path.

Including the metadata job, summed job minutes were Windows arm64 8.98, Linux x64 4.97, Linux arm64 4.35 and macOS arm64 7.30. Time from dispatch to build-job start was respectively 28, 19, 22 and 39 seconds; this includes metadata/dependency scheduling and is not pure runner queue time. Windows arm64 ninja executed exactly two link edges in 15 seconds; its npm platform-package step took 129 seconds, illustrating why eliminating more C++ files would not remove all remaining CI time.

macOS x64 used 10.73 summed job minutes and started its build 22 seconds after dispatch. The preceding Windows x64 and macOS x64 cache-fill jobs took 68.77 and 63.30 minutes respectively. Those cold/migration costs are real expenditure and are excluded from warm savings. This optimization does not turn a cold compiler run into a ten-minute build.

Windows x64 used 7.97 summed job minutes and started its build 27 seconds after dispatch. Its ninja log also contains exactly two link edges. All six final builds started within 19–39 seconds of dispatch; there was no large queue delay in this sample. Changed-source/cold parallel compilation was not retuned or benchmarked against a matched new baseline, so no cold/source-change speedup is claimed.

## Changes and evidence

- Warm documentation/version changes select one runner when a successful compatible object cache exists. Source changes and cold builds retain the existing parallel strategy. A live auto-selection run, [34292386470](https://github.com/sj817/shotium/actions/runs/34292386470), selected one runner and performed only two link edges.
- Downloaded inputs keep timestamps when their SHA-256 fingerprints are unchanged. Previously, editing a Python hook in DEPS changed compiler timestamps and triggered hundreds/thousands of compile edges despite unchanged compiler bytes. The first migration can still rebuild when only an old cache exists.
- macOS retains spare Xcodes when disk space is sufficient. The baseline spent 5–7 minutes deleting them despite 109 GiB free.
- Removed eager installation of the common vpython environment; on-demand vpython remains available. This previously cost about 123 seconds on Windows and 287 seconds on macOS x64.
- Exact-key clang/Rust environment caches preserve gclient's download receipts. Linux fill/hit comparison: sync 95.826 s versus 29.110 s plus 8.416 s restore, approximately 58 s net setup saving. This measures preparation, not an assumed whole-job improvement.
- Object-cache saves now verify API visibility and retry a failed reservation with a fresh key. Cleanup is limited to the current branch. A previous successful macOS build failed to save its cache. Separately, I deleted the validation-branch Windows x64 cache without confirming a main copy survived; that caused an avoidable cold rebuild. Neither cold run is counted as a warm result.

## Community Actions and environment choices

| Option | Decision | Reason |
| --- | --- | --- |
| [actions/cache](https://github.com/actions/cache) | Use for downloaded toolchains and objects | Existing integration, measured Linux preparation benefit, exact keys and gclient receipts preserve normal verification. |
| [actions/github-script](https://github.com/actions/github-script) | Use to verify saved cache entries | Reuses authenticated GitHub API client instead of adding another custom service. |
| [mozilla-actions/sccache-action](https://github.com/mozilla-actions/sccache-action) | Defer | Current GHA support exists; old backend errors are not a reason to reject it. Warm ninja has no compiler invocations to accelerate. Changed-source benefit needs separate measurements and extra cache capacity. |
| [hendrikmuhs/ccache-action](https://github.com/hendrikmuhs/ccache-action) | Defer | Also caches compiler results; does not remove repeated toolchain downloads. Duplicates storage for this warm-build bottleneck. |
| Prepared containers or persistent runners | Defer | Additional image maintenance, platform differences and infrastructure costs; no paid infrastructure was provisioned. |
| Skip Linux dependency installation | Keep current behavior | Current installer costs about 47 s, but includes fonts/locales. A package-only quick check cannot prove the same rendering environment. |

After the final Windows x64 save, the live inventory contained 30 entries totaling 10,015,507,769 bytes (9.33 GiB): toolchains 3,692,232,823 bytes, object caches 5,660,386,892 bytes, and the rest package-manager caches. All six platforms have two main-branch object entries and all four host-toolchain entries exist. Capacity is tight: new branches or toolchain revisions can cause eviction, so retention is not guaranteed. GitHub documents [cache scope and eviction](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching). Storage settings were not increased and no paid service was purchased. Avoid clearing the last known good cache while builds are running.

Raw run metadata, artifact inventories, cache inventory and logs are kept locally under `out/ci-runtime/`; the linked GitHub runs provide public provenance. This bounded optimization and its six-platform verification are complete. Remaining opportunities are separate work: package-manager/bootstrap variability, native npm package build time, and a matched changed-source compiler-cache experiment. They are not unfinished release gates. No engine behavior or fonts were changed by this CI batch.
