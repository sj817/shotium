---
name: perf-compare
description: >-
  Compare a locally built shotium candidate with a published npm baseline and
  report performance or memory changes. Use for regression questions, measured
  optimisation claims and the performance gate; control binary provenance,
  fixture/cache conditions and measurement overhead.
---

# Candidate vs published npm

The methodology is in `apps/docs/performance.md`; read it first. This skill
is the commands and the traps.

## Preconditions

1. **The candidate is what you think it is.** `out/Shot/shotium.node` must be newer than the
   change (`/verify-engine` steps 0 and 2). `binding.ts` loads the local addon
   *before* any platform package, so an installed `@shotkit/` package is not
   the risk -- a stale local `shotium.node` is. An incompatible binding version is rejected, but a stale matching version still loads, and you
   measure the previous engine against the published one while believing you
   measured the change.
2. **The baseline is an installed package**, not a checkout:
   `npm pack`/`npm install @pixel.js/shotium@<version>` into a scratch
   directory, and pass that directory as `BASELINE_PACKAGE`.
3. **Nothing else is running.** No build, no render checks, no other
   benchmark on the same host. The harness compares two engines on one
   machine, and a compile in the background moves both numbers by different
   amounts.
4. **The sampler is load too.** `systeminformation`'s `si.processes()` costs
   ~700 ms on Windows and is duty-cycled in the harness. Never reintroduce
   `si.powerShellStart()`; it deadlocks `si.processes()` (observed: ten
   Windows jobs, 90 minutes, zero output).

## Run

```bash
pnpm perf:compare BASELINE_PACKAGE CANDIDATE_PACKAGE result.json --calibrate --check
pnpm perf:images result.json
pnpm perf:report result.json --platform win32-x64 --output report.md
```

- `--calibrate` measures the host's noise floor first; `--check` validates
  every capture (a rejected npm capture is never counted as a fast baseline).
- The matrix covers render cases (both Bilibili articles, the benchmark
  corpus, PNG/JPEG/WebP, full page, selector, clip, alpha, file output,
  scales 0.5 to 8), HTTP/cache cases, fresh-process startup, memory
  release/restart, queue and multi-process cases, daemon cases,
  failure/recovery, and a 1000-pair soak. The two whole articles and the
  tiles API have no working npm equivalent before 0.3.4; `scripts/verify/bilibili.ts`
  validates their output separately.
- Stopping is budget-driven, not count-driven: at least 20 pairs and 3 s per
  side per case, then until the p50 and mean ratios are known to within 2%
  (99% interval), capped at 8 s per side and 1,000 pairs. Five warm-ups
  precede each resident case; sides alternate AB/BA; every sample stays in
  the JSON. A case where both sides do identical work never converges to 2%
  and stops at the cap; that is the cap, not noise.
- A full matrix is about 20 minutes.

## Reading the result

- **Only local-file and written-fixture cases describe the engine.** A remote
  URL includes DNS + TCP + TLS: 240-390 ms cold, 0.6 ms from cache on the
  development host. The cache default flipped between releases (0.1 on,
  0.2 off, 0.3 on, at `$TMPDIR/.shotium/cache/<project sha1>` with a 256 MB
  cap), so a cross-version remote-URL comparison is a false regression or a
  false win. To compare remote fetches anyway, pass the same explicit
  `cacheDir` to both sides and warm both.
- `stats.fromCache` is `URLRequest::was_cached()`; a 304 revalidation counts
  as a hit and still shows tens of milliseconds in `timing.fetch`.
- Report numbers in a table: case, baseline p50, candidate p50, ratio with
  its interval, pairs. Say which platform and which binary. Do not average
  across cases.
- Memory columns: report peak private and working set separately; the
  discardable shared segment shows only in the working set.

## The CI gate

`perf-gate.yml` runs the same comparison on six platforms. It
is dispatched with `baseline_version` (an exact published version) and,
optionally, `fingerprint`; empty means the fingerprint of the dispatched
ref, and the six candidate engines are the `engine-<platform>-<fingerprint>`
artifacts with evidence -- whatever `engine.yml` built or found. If one is
missing the resolve job says which; `engine.yml` is the fix, there are no
run ids to paste. `scripts/perf/ci.ts` drives it;
`scripts/lib/perf-gate.ts` decides pass/fail; `scripts/lib/perf-gate.test.ts` is its
unit test and runs in `checks.yml` on every push. Change the test before
changing a threshold.

The default `acceptance=improvement` retains that strict improvement objective.
For an unchanged-runtime release rehearsal, explicitly dispatch with
`-f acceptance=identical-runtime`. This is release validation: all runtime files
must match before and after a full matrix, workers must load those exact files,
and every case must execute successfully. Pixel/article checks still apply.
All timing verdicts, including `slower`, remain visible as diagnostics; identity
acceptance makes no performance or non-regression claim. CI uses the documented
20-pair minimum for identity validation, retaining 100 for improvement; both
include the complete matrix, calibration and the 1000-pair soak. Changed
runtime files cannot use this mode. Locally, pass `--acceptance identical-runtime`
to both `perf:compare` and `perf:report`; see `apps/docs/performance.md` for the
snapshot scope and `scripts/ci/performance-acceptance.test.ts` for refusal tests.

Six-platform *competitor* benchmarks (Puppeteer, Playwright) are a different
harness, `apps/benchmark`, dispatched through `benchmark.yml`; see its README
for shards and for what does and does not fail a run.
