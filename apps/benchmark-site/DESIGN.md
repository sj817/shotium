# Design

Two readers, one page. A developer choosing a screenshot engine wants one
number for their platform and a reason to believe it; a maintainer wants to
know whether this CI round archived all six platforms and where the evidence
is. Both are in a hurry, so the page runs conclusion → basis → detail → method
and never asks anyone to learn a chart first.

## Information architecture

```
nav (VitePress: title · language · appearance · GitHub)          glass T0
run bar     run picker · version · date · profile · sha · CI · archive/quality/evidence chips
verdict     one sentence with every number computed; ratio range and closest competitor;
            platforms without a ranking, each with its reason
scenarios   a responsive scenario card matrix (ScenarioCardGrid) for the selected platform:
            cards grouped/filtered by shard (startup, throughput, parallel, resident, resilience);
            in-place horizontal bars for each engine with local linear scaling (shorter is faster in ms/ratio,
            longer is faster in throughput); metric switcher (p50 ms / ratio vs Shotium / throughput);
            Shotium highlighted in green with crown for winner; clear status chips for noisy/failed runs;
            the six platforms are Element Plus tabs with `tab-position="bottom"`
platform    Element Plus tabs → ranking (ECharts bars + el-table + exclusions +
            comparable items) → scenario table (el-table: custom sort over the
            whole filtered set, el-pagination / show all)
            → quality & evidence (counts, warnings, anomalies, engine reasons,
              failure records with full error text, raw file links)           glass T1-dense
method      collapsed <details>: fairness, p50, comparable items, geometric mean,
            how noisy is decided, evidence                                    glass T1
footer                                                                         glass T0
```

Why this order: the verdict and the scenario chart fit on the first screen
of a laptop, so reader 1 is done before scrolling. Every platform view shows
"N comparable" because a ranking built on one scenario and one built on ten
must not look equally trustworthy — with the real data most platforms rank on
`lifecycle` alone. Reader 2 reads the three run chips and the per-platform
warnings, then jumps to Quality & evidence for the full error text.

Run and platform selection live in the URL so a view can be shared.

## Visual system

**Wallpaper.** One fixed colour field: a near-neutral base and three large
radial blobs in mint, ice blue and apricot (deep green, navy and warm brown in
dark mode). They drift over 80–110 s on the compositor (`transform` only, no
`filter`) and stop under `prefers-reduced-motion`. Three hues, low saturation,
so nothing muddies where they overlap.

**Glass tiers** (`tokens.css` → `glass.css`), told apart by fill and depth:

| Tier | Where | Compositor treatment | Fill (light / dark) |
| --- | --- | --- | --- |
| T0 | nav bar, footer | 20 px / 150 % | white .55 / `#141820` .50 |
| T1 | run bar, verdict, overview, method | pre-softened wallpaper; no live blur | white .62 / .52 |
| T1-dense | the platform panel (tables) | pre-softened wallpaper; no live blur | white .84 / .76 |
| T2 | tooltips, VitePress menus | 16 px / 150 % | white .92 / `#1a1f28` .92 |

Every tier has a 1 px translucent edge, a brighter inset highlight on the top
and left, a faint inset shade on the bottom and right, and a soft long shadow
— that is what reads as thick glass; no SVG displacement is used. The wallpaper
is itself a low-frequency colour field, so persistent report panels and their
controls reuse that softened field instead of stacking live blur passes. Only
the fixed nav/footer and short-lived popovers use `backdrop-filter`; table rows,
bars and chart marks never get their own. `@supports not
(backdrop-filter)` and `prefers-reduced-transparency` swap every tier for a
solid fill.

**Contrast.** Text tokens were checked against each fill composited over the
brightest and darkest wallpaper colours: body text ≥ 13:1, secondary ≥ 6:1,
hints ≥ 5:1, status chips ≥ 4.9:1 in both themes.

**Colour.** The interface is neutral. One accent (Shotium green `#146c43`,
`#4fb37f` in dark) is used for the brand, rank 1 and selection. Engines have
fixed colours: Shotium green, Puppeteer blues, Playwright oranges, `-shell`
the darker tone and `-chrome` the lighter. Status colours — pass green, noisy
amber, fail red, n/a grey, infra-error slate blue — always appear with a text
label and a tooltip that explains the word.

**Type.** Inter plus the system CJK stack, sizes 12/13/14/16/20/28,
`tabular-nums` everywhere. The verdict is the only large text on the page.

**Components.** Element Plus named component registration for every control;
the unused JavaScript components are tree-shaken while the shared theme CSS is
kept for exact visual compatibility. `report.css`
maps its CSS variables (`--el-color-*`, `--el-text-color-*`, `--el-fill-*`,
`--el-bg-color`, table/card/collapse variables) onto the tokens inside
`.bench`, and every popper the report opens carries `popper-class="bench-pop"`
so tooltips and dropdowns are T2 glass. Element Plus's own dark variables
follow VitePress's `html.dark`.

**Charts.** ECharts through `vue-echarts` (SVG renderer, bar/grid/tooltip modules,
imported dynamically so nothing reaches the SSR bundle or the first paint)
is used for the overall ranking score chart. The scenario-by-scenario breakdown
is powered by high-performance Vue 3 + CSS responsive cards (`ScenarioCardGrid`),
using local linear scale bars per scenario for immediate visual clarity without
log-scale cognitive overhead. Bar colors follow the design tokens (engine colors).

## Accessibility and responsiveness

Element Plus tabs (arrow-key navigation), selects, switches and pagination;
tooltips open on hover and on keyboard focus (`trigger: ['hover', 'focus']`)
and are teleported to `<body>` so scroll containers never clip them; sortable
table headers expose `aria-sort`; focus rings are visible on every control. Breakpoints:
1280 (everything fits), 1024 (the scenario table scrolls inside itself), 390
(single column, run metadata in two columns, chart labels tilted). Wide tables
scroll inside their own container; the document never scrolls horizontally.

## Trade-offs

- Element Plus keeps its shared theme CSS (about 0.5 MB before gzip) for exact
  component styling, while JavaScript registration is restricted to the
  controls used by the report.
- ECharts tooltips are mouse-only; the tables next to the charts carry the
  same numbers for keyboard and screen-reader users.
- The six-platform matrix was replaced by one platform at a time (tabs under
  the chart); the cross-platform picture lives in the verdict sentence.
- A typed dictionary instead of vue-i18n: the same centralised copy with
  compile-time completeness and no SSR configuration.
- Ranking bars use a linear scale (the sliver for 1× *is* the message); only
  the per-scenario chart is logarithmic.
- Liquid-glass refraction (SVG `feDisplacementMap`) was left out on purpose:
  it costs a filter per panel and adds nothing to legibility.
