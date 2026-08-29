# Shotium Benchmark Site

VitePress + Vue 3 + TypeScript documentation site for the repository's archived benchmark results. Chinese is the default language; the dashboard switch enables English.

The site never embeds a benchmark snapshot in its source. After `vitepress build`, the typed VitePress config copies the repository-level `benchmark-results/` directory into `docs/.vitepress/dist/benchmark-results/`. At runtime the dashboard reads:

- `benchmark-results/index.json`
- `<run>/manifest.json`
- `<run>/<platform>/summary.json`
- `<run>/<platform>/failures.json` when available

Rankings never combine absolute timings across platforms. Within one platform, each eligible engine/scenario p50 is divided by the matching Shotium p50; the displayed aggregate is the geometric mean of those ratios. Failed, noisy, unavailable, or ranking-ineligible cells are excluded and reported.

## Commands

```bash
npm ci
npm run dev
npm run typecheck
npm test
npm run build
npm run check
```

The VitePress base defaults to `/`. CI provides the GitHub Pages project base explicitly:

```bash
BENCHMARK_SITE_BASE=/shotium/ npm run build
```

The build must contain `docs/.vitepress/dist/benchmark-results/index.json` and the archived run directories. If the repository has no result index, the copy step fails instead of publishing an empty report.

## 中文说明

该应用是基于 VitePress 的 Shotium 基准文档站。源码目录不保存任何结果副本；构建时从仓库根目录复制 `benchmark-results/`，页面运行后再动态读取索引、运行清单、六平台汇总和失败记录。

综合排名只在同一平台内计算：有效场景的引擎 p50 除以对应 Shotium p50，再计算几何平均。失败、噪声、不适用或明确禁止排名的数据不会混入排名，并会显示排除原因。
