# Static render regression

These fixtures contain no JavaScript, animation, clocks, random values, or
public-network dependencies. They run at a fixed viewport and scale. The text
case uses Chromium's checked-in Ahem WOFF2 fixture rather than a host font for
its metric-sensitive block.

Create reference images from a pinned source build:

```powershell
pwsh ./tests/render/update-baselines.ps1 `
  -BaselineEngine headless-shell `
  -BaselineExecutable ./out/Release/headless_shell.exe `
  -Accept
```

An installed Chrome can be measured explicitly with
`-BaselineEngine system-chrome`; the manifest records it as
`external-system-chrome`. It is never confused with either the source-built
`headless_shell` baseline or `source-build-shot`.

Run Shot and perform decoded-pixel comparison:

```powershell
pwsh ./tests/render/run.ps1 -ShotExecutable ./out/Release/shotium.exe
```

The report includes PNG width/height, encoded byte count, SHA-256, changed pixel
count/fraction, maximum channel delta, mean absolute channel delta, and RMSE.
SHA-256 is an artifact-integrity signal; the decoded pixel result is the render
correctness signal because two lossless PNG encoders can produce different byte
streams for identical pixels.

Use `png-diff.ps1` for one-off comparisons. Thresholds default to exact decoded
pixel equality. Only relax a per-case threshold after reviewing the generated
red-on-black diff image and documenting the reason in `cases.json`.
