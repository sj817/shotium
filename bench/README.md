# Shot benchmark corpus

Two benchmarks share this corpus. This one compares `shot` against a Chromium
built from the same tree, over native CLI flags. `bench/cross/` compares the
npm package against puppeteer and playwright, which is the comparison a caller
choosing a library actually faces.

The corpus covers `simple`, `css-heavy`, `flex`, `grid`, `text`, `fonts`,
`images`, `gradient`, `filter`, `long-page`, and `remote-page`. Every case is
static and fixed at 1280 × 720, scale 1. The v1 contract is viewport-only: a
long document increases style/layout work but does not request a full-page PNG.

Run a pinned Chromium source baseline against Shot:

```powershell
pwsh ./bench/run.ps1 `
  -BaselineEngine headless-shell `
  -BaselineExecutable ./out/Release/headless_shell.exe `
  -ShotExecutable ./out/Release/shot.exe `
  -Iterations 10 -WarmupIterations 2
```

To use an installed Chrome, set `-BaselineEngine system-chrome` and pass its
exact executable path. Reports label it `external-system-chrome`; a source
`headless_shell` is `source-build-headless-shell-baseline`; Shot is always
`source-build-shot`. All capture adapters use native CLI flags, never CDP.

`remote-page` is served automatically over loopback HTTP. It covers a top-level
GET, linked CSS, CSS `@import`, WOFF2, and SVG while avoiding public-network and
DNS variability.

Each recorded sample launches one fresh process tree, so `wall_time_ms` includes
startup, load, layout, raster, PNG encoding, and output write. Warmup iterations
only warm host file caches; they are not mislabeled as an in-process warm mode.
The process sampler requests a 10 ms pause between snapshots by default; native
snapshot overhead is additional, so raw rows also record the observed mean
sample period. Peak RSS is the maximum sampled sum of working sets; process and
thread values are sampled maxima, not exact kernel job-accounting values.

The JSON report stores raw samples and min/p50/p95/max/mean summaries for wall
time, peak RSS, process count, and summed thread count. It also stores executable
SHA-256 and size. For an honest runtime footprint, stage each engine's complete
distributable closure in its own directory and pass `-BaselineRuntimeRoot` and
`-ShotRuntimeRoot`. If omitted, the report explicitly marks runtime size as
`executable-only-runtime-root-not-supplied` instead of pretending the binary is
the whole runtime.

For repeatable comparisons, use the same commit, build type, hardware, power
plan, host load, viewport, iteration count, runtime bundle, and benchmark
manifest. Compare medians and p95 values together with raw samples.
