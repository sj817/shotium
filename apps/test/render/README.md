# Static render regression testing

English · [简体中文](./README.zh.md)

These fixtures contain no JavaScript, CSS animations, system clocks, random values,
or external network dependencies. They run at a fixed viewport size and scale.
Metric-sensitive text fixtures use the checked-in Ahem font (`shot/testdata/ahem.ttf`)
rather than host system fonts to ensure deterministic typography.

The test harness is `scripts/verify/render-regression.ts` (invoked via `pnpm render`).
This directory contains only test fixtures and metadata: `cases/`, `cases.json`, and
the gitignored `baselines/` directory.

Generate reference baseline images from a pinned source build:

```sh
pnpm render update-baselines \
  --baseline-engine headless-shell \
  --baseline-executable ./out/Release/headless_shell.exe \
  --accept
```

An installed host Chrome can be evaluated explicitly via `--baseline-engine system-chrome`;
the manifest records it as `external-system-chrome`, keeping it strictly separated from
source-built `headless_shell` baselines and `source-build-shot`.

Run Shotium and perform decoded-pixel comparison:

```sh
pnpm render run --shot ./out/Shot/shotium.exe
```

The report includes PNG dimensions, encoded byte size, SHA-256 checksum, changed pixel
count and percentage, maximum channel delta, mean absolute channel delta, and RMSE.
SHA-256 serves as an artifact integrity signal; the decoded pixel buffer is the canonical
signal for rendering correctness, as differing lossless PNG encoders can produce different
byte streams for identical pixel data.

Use `pnpm render diff <expected.png> <actual.png>` for one-off comparisons.
Thresholds default to exact decoded pixel equality. Only relax a per-case threshold
after inspecting the generated red-on-black diff image and documenting the technical
rationale in `cases.json`.
