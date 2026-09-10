# Shotium benchmark site

English · [简体中文](./README.zh.md)

A benchmark visualization and reporting site built with VitePress 1.x, Vue 3, and TypeScript to present archived benchmark runs, deployed to GitHub Pages at `https://sj817.github.io/shotium/`. Chinese (`/`) is the default locale with English at `/en/`; locale and theme toggles are integrated into the standard VitePress navigation bar.

UI components are powered by Element Plus (tabs, dropdowns, sortable/paginated tables, status tags, tooltips, KPI metrics, alerts, and accordions). Interactive charts are rendered via ECharts (`vue-echarts`, utilizing an on-demand SVG renderer) to display per-scenario latency curves and composite ranking bars. A central design token stylesheet maps component variables to the glassmorphism aesthetic, allowing dark mode to switch seamlessly via token reassignment.

## Layout

| Path | Role |
| --- | --- |
| `docs/index.md`, `docs/en/index.md` | Page entries; each mounts `<BenchmarkReport>` inside `<ClientOnly>` |
| `docs/.vitepress/config.ts` | Locales, base (`BENCHMARK_SITE_BASE`), dev middleware that serves `../../docs/benchmarks/`, and `buildEnd` copy hook |
| `docs/.vitepress/theme/tokens.css` | Canonical design tokens for colours, type, radii, and glass tiers (light and `html.dark`) |
| `docs/.vitepress/theme/glass.css` | Wallpaper, GPU-bounded glass tiers, navigation overrides, and reduced-transparency fallbacks |
| `docs/.vitepress/theme/report.css` | Page container, Element Plus variable mappings, nested-blur prevention, and glass popper surfaces |
| `docs/lib/types.ts`, `data.ts` | Result schema and browser-side data loader |
| `docs/lib/ranking.ts` | Ranking and scoring rules (`test/ranking.test.ts`) |
| `docs/lib/report.ts` | Run-level aggregation, platform rankings, and verdict statistics |
| `docs/lib/messages.ts`, `i18n.ts` | Typed bilingual copy |
| `docs/lib/format.ts`, `labels.ts` | Number/date formatting, engine/scenario ordering, and colour variables |
| `docs/lib/theme.ts`, `echarts.ts` | CSS token reader and tree-shaken ECharts registration |
| `docs/components/` | `BenchmarkReport` root component tree, panels, charts, and status indicators |
| `DESIGN.md` | Information architecture and visual system design specification |

## Data architecture

The site embeds no result snapshot. In dev the middleware serves the repository's `apps/docs/benchmarks/` directly; `vitepress build` copies it to `docs/.vitepress/dist/benchmark-results/`. The browser reads, relative to the site base:

- `benchmark-results/index.json`
- `benchmark-results/<run>/manifest.json`
- `benchmark-results/<run>/<platform>/summary.json`
- `benchmark-results/<run>/<platform>/failures.json` (optional)

`samples.jsonl`, `quality.json`, `report.md` and `summary.csv` are linked, not parsed. The selected run and platform are kept in the URL (`?run=<path>&platform=<id>`).

## Ranking algorithm and scoring model

Engines are compared strictly within the same platform environment. A scenario's latency metric uses p50 (`latency_ms.p50 ?? wall_time_ms.p50`) and must be `> 0`. A comparable scenario requires Shotium and at least one competing engine to both succeed, satisfy ranking criteria, and have valid p50 timings. An engine's composite score is the geometric mean of `engine p50 ÷ Shotium p50` across all comparable scenarios it covers (lower is faster, Shotium baseline is 1.0×). Only full scenario coverage earns an official rank; partial coverage displays reference scores alongside exclusion rationales. The lowest ratio in an individual scenario represents a win (ties are shared). A platform ranking is generated only when at least two engines hold official ranks. Text summaries, charts, and tables are computed dynamically from run data at load time without hard-coded constants.

## Commands

```bash
pnpm install --frozen-lockfile
pnpm run dev        # http://localhost:5173/
pnpm run typecheck  # vue-tsc -b
pnpm test           # vitest
pnpm run build      # vue-tsc -b && vitepress build docs
pnpm run check      # typecheck + test + build (what CI runs)
```

CI (`.github/workflows/benchmark-site.yml`, Ubuntu + Node 24) runs `pnpm install --frozen-lockfile && pnpm run check` with `BENCHMARK_SITE_BASE=/shotium/` and uploads `docs/.vitepress/dist`. The build fails if `apps/docs/benchmarks/index.json` is missing rather than publishing an empty report.

## Visual inspection and regression

Playwright is available in `apps/benchmark/node_modules`. With `pnpm run dev` running, launch automated visual tests across 1280px, 1024px, and 390px viewports across both light and dark themes to verify layout integrity: no horizontal document overflow, high text contrast across all glassmorphism tiers, clear table rows, and fully populated tooltips on all status indicators.
