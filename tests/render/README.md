# Static render regression

These fixtures contain no JavaScript, animation, clocks, random values, or
public-network dependencies. They run at a fixed viewport and scale. The text
case uses the checked-in Ahem font (`shot/testdata/ahem.ttf`) rather than a
host font for its metric-sensitive block.

The harness is `scripts/render-regression.ts` (`pnpm render`). This directory
holds only the data: `cases/`, `cases.json`, and the gitignored `baselines/`.

Create reference images from a pinned source build:

```sh
pnpm render update-baselines \
  --baseline-engine headless-shell \
  --baseline-executable ./out/Release/headless_shell.exe \
  --accept
```

An installed Chrome can be measured explicitly with
`--baseline-engine system-chrome`; the manifest records it as
`external-system-chrome`. It is never confused with either the source-built
`headless_shell` baseline or `source-build-shot`.

Run Shot and perform decoded-pixel comparison:

```sh
pnpm render run --shot ./out/Shot/shotium.exe
```

The report includes PNG width/height, encoded byte count, SHA-256, changed pixel
count/fraction, maximum channel delta, mean absolute channel delta, and RMSE.
SHA-256 is an artifact-integrity signal; the decoded pixel result is the render
correctness signal because two lossless PNG encoders can produce different byte
streams for identical pixels.

Use `pnpm render diff <expected.png> <actual.png>` for one-off comparisons.
Thresholds default to exact decoded pixel equality. Only relax a per-case
threshold after reviewing the generated red-on-black diff image and documenting
the reason in `cases.json`.
