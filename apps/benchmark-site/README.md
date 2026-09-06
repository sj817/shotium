# Shotium benchmark site

VitePress 1.x + Vue 3 + TypeScript report for the repository's archived
benchmark results, published to GitHub Pages at `https://sj817.github.io/shotium/`.
Chinese (`/`) is the root locale, English lives at `/en/`; the language switch
and the light/dark toggle are VitePress's own nav-bar controls.

Element Plus provides every control (tabs, select, table with sorting and
pagination, tags, tooltips, statistics, alerts, collapse); ECharts
(`vue-echarts`, SVG renderer, loaded on demand) draws the scenario line chart
and the ranking bars; one token file maps Element Plus's CSS variables onto
the glass palette, so the dark theme is nothing but a second token palette.

## Layout

| Path | Role |
| --- | --- |
| `docs/index.md`, `docs/en/index.md` | Page entries; each mounts `<BenchmarkReport>` inside `<ClientOnly>`. |
| `docs/.vitepress/config.ts` | Locales, base (`BENCHMARK_SITE_BASE`), dev middleware that serves `../../benchmark-results/`, `buildEnd` that copies it into `dist/`. |
| `docs/.vitepress/theme/tokens.css` | The only source of colours, type, radii and glass parameters (light + `html.dark`). |
| `docs/.vitepress/theme/glass.css` | Wallpaper, GPU-bounded glass tiers, VitePress nav/footer/menu overrides, paused-background and reduced-transparency fallbacks. |
| `docs/.vitepress/theme/report.css` | Page container, Element Plus variable mapping, nested-blur prevention, offscreen rendering containment and the glass popper surface (`popper-class="bench-pop"`). |
| `docs/lib/types.ts`, `data.ts` | Result schema and the browser-side loader (fetches relative to the site base). |
| `docs/lib/ranking.ts` | The ranking rules (`test/ranking.test.ts`). |
| `docs/lib/report.ts` | Run-level aggregation: per-platform ranks, "no ranking" reasons, the verdict numbers (`test/report.test.ts`). |
| `docs/lib/messages.ts`, `i18n.ts` | Typed copy for both locales; a missing English key fails `vue-tsc`. |
| `docs/lib/format.ts`, `labels.ts` | Number/date formatting, engine/scenario ordering and colour variables. |
| `docs/lib/theme.ts`, `echarts.ts` | Reading tokens from CSS at render time (and re-reading on theme flip); tree-shaken ECharts registration. |
| `docs/components/` | `BenchmarkReport` (root, `el-config-provider`) → `RunBar`, `VerdictPanel`, `PlatformOverview` (→ `ScenarioCardGrid`), `PlatformPanel` (→ `RankingSection` → `RankingChart`, `ScenarioTable`, `QualitySection`), `Methodology`; plus `GlassPanel` (`el-card`), `StatusChip` (`el-tag` + `el-tooltip`), `Tip`, `Wallpaper`. |
| `DESIGN.md` | Information architecture and the visual system, with the reasons. |

## Data

The site embeds no result snapshot. In dev the middleware serves the
repository's `benchmark-results/` directly; `vitepress build` copies it to
`docs/.vitepress/dist/benchmark-results/`. The browser reads, relative to the
site base:

- `benchmark-results/index.json`
- `benchmark-results/<run>/manifest.json`
- `benchmark-results/<run>/<platform>/summary.json`
- `benchmark-results/<run>/<platform>/failures.json` (optional)

`samples.jsonl`, `quality.json`, `report.md` and `summary.csv` are linked, not
parsed. The selected run and platform are kept in the URL
(`?run=<path>&platform=<id>`).

## Ranking

Engines are compared only within one platform. A scenario's p50 is
`latency_ms.p50 ?? wall_time_ms.p50` and must be `> 0`. A comparable item is a
scenario where Shotium passed, is ranking-eligible and has a valid p50, and at
least one competitor does too. An engine's score is the geometric mean of
`engine p50 ÷ Shotium p50` over the comparable items it covers (lower is
faster, Shotium is 1×); only full coverage earns an official rank, partial
coverage shows a reference score plus its exclusion reasons; the smallest
ratio on an item is a win, ties count for everyone; a platform has a ranking
only when at least two engines hold official ranks. Nothing in the copy is a
hard-coded number — the verdict sentence, every chart and every table are
computed from the loaded run.

## Commands

```bash
pnpm install --frozen-lockfile
pnpm run dev        # http://localhost:5173/
pnpm run typecheck  # vue-tsc -b
pnpm test           # vitest
pnpm run build      # vue-tsc -b && vitepress build docs
pnpm run check      # typecheck + test + build (what CI runs)
```

CI (`.github/workflows/benchmark-site.yml`, Ubuntu + Node 24) runs
`pnpm install --frozen-lockfile && pnpm run check` with `BENCHMARK_SITE_BASE=/shotium/` and uploads
`docs/.vitepress/dist`. The build fails if `benchmark-results/index.json` is
missing rather than publishing an empty report.

## Visual checks

Playwright is available in `apps/benchmark/node_modules`. With `pnpm run dev`
running, a script that launches `chromium.launch({channel: 'msedge'})`, sets
`localStorage['vitepress-theme-appearance']` to `dark` and reloads, then takes
full-page screenshots at 1280 / 1024 / 390 px is the intended check: no
horizontal document overflow, text readable on every glass tier, table rows
untinted by the wallpaper, every status word explained by its tooltip.
