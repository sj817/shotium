# Six-platform benchmark

[简体中文](README.zh.md)

This TypeScript application is the canonical Shotium performance and resilience
harness. It evaluates Shotium alongside competitor browser automation setups
that offer native binary support on the current runner. Unsupported competitor
architectures are recorded as `n/a`; installation or startup failures on a
supported architecture are recorded as failures rather than masked as `n/a`.

The application uses a standard, lightweight layout:

```text
apps/benchmark/
├─ src/       TypeScript CLI, engines, lifecycle, and aggregation
├─ test/      TypeScript unit tests executed through tsx
├─ schema/    Permanent-result JSON Schema
└─ fixtures/  Shared static render corpus and assets
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

The shard boundaries are: `startup` covers cold start, cold-settled first screenshot,
and lifecycle; `throughput` covers warm and batch; `parallel` covers concurrency
scenarios; `resident` covers resident clients and page reuse; and `resilience` covers
faults and soak testing.

The CI workflow expands this into a 30-job `platform x shard` matrix. Every
shard runs all available engines on one native runner with balanced engine
ordering, ensuring comparisons within a scenario remain same-machine comparisons.
The five shards are merged into one platform result before aggregating all six
platforms. Runner metadata remains attached to each shard; raw timings from
different shards or platforms are never pooled. When a cross-shard summary is
needed, only same-cell ratios measured inside a runner are combined geometrically.

To compare source-built executables directly on one machine, run:

```bash
pnpm run benchmark:native -- --baseline-executable /path/to/headless_shell --baseline-engine headless-shell --shot-executable /path/to/shotium --iterations 5 --warmup-iterations 1 --output ./out-native
```

The JSON/CSV report contains raw samples, validated PNG metadata, executable
SHA-256/version metadata, and same-machine `baseline p50 / Shot p50` ratios.
The five evaluated engine variants are: Shotium native engine, plus Puppeteer
and Playwright each driving Chrome and the headless shell.

Each passing platform generates its own geometric-mean ranking. It includes only
cells where Shotium and the compared engine both passed and were ranking-eligible
on the same scenario and concurrency; lower normalized elapsed time is better.
Coverage and per-cell wins are displayed, and platforms are never mixed together.
Only engines covering every comparable cell receive a formal rank; partial
coverage remains visible with its score but is explicitly left unranked. Failed,
noisy, or incomplete platforms retain diagnostic data but never produce a formal rank.

The benchmark focuses on a deliberately defined scope: evaluating how each
out-of-the-box engine variant performs static HTML/CSS screenshot workloads using
its standard browser binary. It does not represent general JavaScript interaction
or browser automation capabilities. Every engine receives the exact same offered
concurrency, viewport, cache policy, fixtures, PNG format, and operation timeout;
the results reflect each implementation's real process topology and resource footprint.

To regenerate only the derived Markdown/CSV views of an archived result
(including older four-shard results), run:

```bash
pnpm run render-report -- --result-directory ../../apps/docs/benchmarks/v0.3.2/<run-directory>
```

This reads the archived manifest and platform summaries, replacing only
`report.md`, `report.zh-CN.md`, `summary.csv`, and the existing index-backed
`LATEST.md`. It does not alter raw samples, quality records, failures, or the
manifest. Reports link to the [VitePress benchmark explorer](https://sj817.github.io/shotium/).

### Detailed Measurement and Environmental Isolation

- **Test Scale**: The `full` profile runs 7 cold repetitions, concurrency levels of 1/2/4, 20 lifecycle cycles, and a 1000-request continuous soak (or a 10-minute cap)
- **Warmup and Sampling**: Non-cold cells run 3 fixed warmup iterations; warmup latency coefficient of variation (CV) and process-tree RSS drift are recorded as engine diagnostics; host stability checks sample 5 seconds of idle CPU before each shard starts (while running the two process samplers), setting a dynamic threshold of `max(25%, idle p95 + 10 points)`; each sampler's CPU usage is capped at 20% of one core; the observed interval is recorded in `observed_mean_period_ms`
- **Retries and Budgets**: A cell that cannot find a quiet host within 6 seconds is marked as `noisy` and retried once (up to 15 seconds); if a shard exhausts its allocated profile budget, it stops scheduling subsequent cells and preserves collected results and evidence
- **Image Verdicts**: Navigation and screenshot operations enforce a 30-second timeout ceiling; Puppeteer and Playwright adapters wait for network idle and two animation frames after `load` before capturing, aligning with Shotium's internal paint-clean lifecycle requirement; render correctness is evaluated using Pixelmatch (perceptual threshold `0.1`), tolerating sub-perceptual GPU rounding differences while strictly catching missing tiles or compositing defects
- **Page Model**: Every capture opens a new page and closes it afterwards, and page reuse stays a separate scenario; Puppeteer pages are created as their own headless window. Chrome's own headless mode leaves an extra `newPage()` as a background tab that never paints: hidden pages report `visibilityState: hidden`, skip `requestAnimationFrame` and keep an empty or stale compositor surface, so concurrent captures return blank PNGs, frames that differ from the same fixture's first capture, and readiness waits that run into the timeout ceiling. Playwright's pages and Chrome's headless shell are already visible without being asked, so this removes an adapter asymmetry; launch flags stay at the package defaults. Concurrent Puppeteer Chrome numbers from archives before this change also measured the tab activation those hidden pages waited on
- **Data Retention**: Resident mode reuses a settled engine host across multiple client requests; sequential batch and parallel modes collect 7 sample rounds on settled instances; only compact summary records are committed to the repository; rendered PNGs, logs, and fine-grained process timelines are retained in CI artifacts for 90 days

Use the `benchmark` GitHub Actions workflow to test a published
semver or npm dist-tag. Release publishing dispatches the same workflow with the
exact published version after the GitHub Release exists.

For focused manual diagnosis, set `platform_filter` to one native platform,
`shard_filter` to one scenario shard, or both. Keeping both inputs at `all`
retains the complete 30-job run. Filtered runs upload their numerical shard
results and detailed Actions evidence, but deliberately skip platform merging,
repository aggregation, and result commits to prevent partial runs from being
archived as full benchmarks.

Until at least five comparable full runs have accumulated on the same runner family,
the benchmark records data without enforcing arbitrary regression thresholds;
subsequent threshold policies will be reviewed separately.
