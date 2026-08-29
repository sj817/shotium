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
npm ci
npm run benchmark -- --shotium-version 0.3.2 --profile smoke --output ./out --seed local-check
```

To compare source-built executables directly on one machine, run:

```bash
npm run benchmark:native -- --baseline-executable /path/to/headless_shell --baseline-engine headless-shell --shot-executable /path/to/shotium --iterations 5 --warmup-iterations 1 --output ./out-native
```

The JSON/CSV report contains raw samples, validated PNG metadata, executable
SHA-256/version metadata, and same-machine `baseline p50 / Shot p50` ratios.

`full` adds seven cold repetitions, concurrency 1/2/4, 20 lifecycle cycles and
a continuous 1000-request (or ten-minute) soak. Every cell waits for host
stability; non-cold cells also wait for measured engine readiness. The harness
retries one noisy settling attempt, records all samples, and
terminates only the PID tree it started. The repository stores the compact
`permanent` output. PNGs, logs and process timelines are CI artifacts retained
for 90 days.

Use the `Six-platform benchmark` GitHub Actions workflow to test a published
semver or npm dist-tag. Release publishing dispatches the same workflow with the
exact published version after the GitHub Release exists.

Results are record-only until five comparable full runs have accumulated on the
same runner family. This harness intentionally does not invent an initial
regression threshold.
