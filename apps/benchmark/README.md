# Six-platform benchmark

[简体中文](README.zh-CN.md)

This TypeScript application is the canonical Shotium performance and resilience
harness. It runs Shotium plus every competitor browser whose executable is
native to the current runner. Unsupported competitor architectures are recorded
as `n/a`; installation or startup failures on a supported architecture are not
hidden as `n/a`.

The application uses a conventional, intentionally shallow layout:

```text
apps/benchmark/
├─ src/       TypeScript CLI, engines, lifecycle and aggregation
├─ test/      TypeScript unit tests executed through tsx
├─ schema/    permanent-result JSON Schema
└─ fixtures/  shared static render corpus and assets
```

```bash
pnpm install --frozen-lockfile
pnpm run benchmark -- --shotium-version 0.3.2 --profile smoke --output ./out --seed local-check
```

Run one scenario shard by adding `--shard startup`, `--shard throughput`,
`--shard parallel`, `--shard resident`, or `--shard resilience`. Omitting the
option (or passing `--shard all`) keeps the single-machine local run:

```bash
pnpm run benchmark -- --shotium-version 0.3.2 --profile full --shard throughput --output ./out --seed local-check
```

The shard boundaries are `startup` for cold, cold-settled and lifecycle;
`throughput` for warm and batch; `parallel` for the concurrency scenarios;
`resident` for resident and reuse-page; and `resilience` for faults and soak.

The CI workflow expands this into a 30-job `platform x shard` matrix. Every
shard still runs all available engines on one native runner with balanced engine
ordering, so comparisons within a scenario remain same-machine comparisons.
The five shards are merged into one platform result before the six platform
results are aggregated. Runner metadata remains attached to each shard; raw
timings from different shards or platforms are never pooled. When a cross-shard
summary is needed, only same-cell ratios measured inside a runner are combined
geometrically.

To compare source-built executables directly on one machine, run:

```bash
pnpm run benchmark:native -- --baseline-executable /path/to/headless_shell --baseline-engine headless-shell --shot-executable /path/to/shotium --iterations 5 --warmup-iterations 1 --output ./out-native
```

The JSON/CSV report contains raw samples, validated PNG metadata, executable
SHA-256/version metadata, and same-machine `baseline p50 / Shot p50` ratios.
The five rows are engine variants: Shotium, plus Puppeteer and Playwright each
driving Chrome and the headless shell. They are not five independent packages.
Each passing platform also gets its own geometric-mean ranking. It includes only cells
where Shotium and the compared engine both passed and were ranking-eligible on
the same scenario and concurrency; lower normalized elapsed time is better.
Coverage and per-cell wins are shown, and platforms are never mixed together.
Only engines covering every comparable cell receive a formal rank; partial
coverage stays visible with its score but is explicitly left unranked. Failed
and noisy cells keep their diagnostic rows and never enter a formal rank. A
platform may still rank its remaining paired passing cells when its shards and
evidence are complete and Shotium and the harness stayed trusted. Incomplete or
evidence-incomplete platforms never produce a formal rank or winner.

This is a fair comparison for one deliberately narrow question: how each
locked, out-of-the-box engine variant performs the same static HTML/CSS
screenshot workload with the browser binary it normally ships. It is not a
package-level verdict on Puppeteer or Playwright, and it does not isolate their
driver overhead: both competitors use their own locked Chromium revisions,
while Shotium necessarily uses its stripped Chromium build. A same-browser
track between Puppeteer and Playwright would answer that separate driver-only
question, but a three-way same-browser track is not possible because Shotium is
the browser engine rather than a controller for stock Chrome. The fixture set
also exercises only the shared static-rendering surface, not JavaScript or
general browser automation. Every engine receives the same offered concurrency,
viewport, cache policy, fixtures, PNG format and operation timeout; the result
still includes each product's real internal process and memory topology rather
than assuming those implementations consume identical resources.

To regenerate only the derived Markdown/CSV views of an archived result
(including an older four-shard result), run:

```bash
pnpm run render-report -- --result-directory ../../benchmark-results/v0.3.2/<run-directory>
```

This reads the archived manifest and platform summaries, then replaces only
`report.md`, `report.zh-CN.md`, `summary.csv`, and the existing index-backed
`LATEST.md`. It does not alter raw samples, quality records, failures, or the
manifest. Reports link to the [VitePress benchmark explorer](https://sj817.github.io/shotium/).

`full` adds seven cold repetitions, concurrency 1/2/4, 20 lifecycle cycles and
a continuous 1000-request (or ten-minute) soak. Every cell waits for host
stability; non-cold cells also run three fixed warmups. Warmup latency CV and
process-tree RSS drift are recorded as engine diagnostics, but do not gate a
cell: creating and reaping Chrome renderer processes is real engine behavior,
not shared-runner instability, and using it as an eligibility condition
systematically rejected the browser adapters. The host gate is calibrated per
shard: five seconds of idle CPU are sampled before the
first cell, with the same two process samplers running that every cell runs, so
their cost is part of the baseline rather than of the noise. The limit is
`max(25%, idle p95 + 10 points)` with no ceiling: GitHub's Windows and macOS
runners idle well above a fixed 25%, and a ceiling below the host's own floor is
a gate nothing can pass. When the limit lands above 80% the summary records
`cpu_limit_exceeds_ceiling` so a quiet-host claim is never made on a busy host.
Each sampler is capped at 20% of one core - a process-table query costs tens of
milliseconds on Linux and about 700 ms on Windows, and an unthrottled loop spent
a whole core enumerating processes, which is load the gate then measured as the
host's. `observed_mean_period_ms` in the telemetry records the sampling
resolution that cap produced. Preflight stability means three consecutive
one-second samples under that limit with steady free memory. After the fixed
warmups, the same CPU gate is checked again while the engine remains alive;
free-memory drift there stays diagnostic because it includes the engine under
test. A cell that cannot get a quiet host within six seconds is marked noisy and
retried once. Shards stop scheduling new
cells once the profile's budget is spent, so results and evidence are written
instead of being lost to a job timeout. A baseline engine that fails - a browser
missing for the platform, a screenshot that differs from its own first render, a
soak that blanks - is recorded in `summary.json` and `failures.json` but does not
abort the shard before its evidence is uploaded. The aggregate is rejected only
when a platform or shard is missing, evidence is incomplete, the harness is
untrusted, or Shotium itself fails; noisy and failed competitor cells are
published as labeled outcomes and excluded from paired rankings. Browser
navigation and screenshot operations use the same 30-second ceiling for all
engines. After `load`, the Puppeteer and Playwright adapters wait for fonts and
two animation frames before capture; that wait is included in the measured
operation. Both browser adapters also use Chrome's
`--run-all-compositor-stages-before-draw` mode. Together these match Shotium's
internal paint-clean lifecycle requirement and prevent concurrent Chrome
captures from returning partially rasterised 256 px tiles without giving the
browser adapters free unmeasured work. Every exact RGBA difference remains in
the raw sample for audit, while correctness uses Pixelmatch's standard `0.1`
perceptual threshold. This accepts one-level GPU/filter rounding that is visually
identical, but still rejects missing or duplicated compositor tiles.
Cold-start timing includes importing
each package inside the timed launch hook. Resident mode starts and settles one
host per engine, then measures seven new clients against that host instead of
restarting and re-warming it seven times. Batch and parallel modes likewise keep
all seven rounds and all per-case samples while reusing one settled engine for
each engine/concurrency combination; independent launches remain in the cold
and lifecycle scenarios. The harness records all samples and
terminates only the PID tree it started. The repository stores the compact
`permanent` output. PNGs, logs and process timelines are CI artifacts retained
for 90 days.

Use the `Six-platform benchmark` GitHub Actions workflow to test a published
semver or npm dist-tag. Release publishing dispatches the same workflow with the
exact published version after the GitHub Release exists.

For a focused manual diagnosis, set `platform_filter` to one native platform,
`shard_filter` to one scenario shard, or both. Keeping both inputs at `all`
retains the complete 30-job run. Any filtered run uploads its numerical shard
result and detailed Actions evidence, but deliberately skips platform merging,
repository aggregation and result commits; a partial diagnostic is never
presented as a complete archived benchmark. Release dispatches omit these
optional filters and therefore keep the full six-platform behavior.

Results are record-only until five comparable full runs have accumulated on the
same runner family. This harness intentionally does not invent an initial
regression threshold.
