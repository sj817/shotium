# Cross-engine benchmark

shotium against puppeteer and playwright, on one corpus, with one measurement
model.

```powershell
cd bench/cross
npm install
# npm 11 does not run a dependency's install script until it is approved, and
# puppeteer's install script is what downloads its browser. Running it directly
# gets the browser without deciding anything about the rest of the tree.
node node_modules/puppeteer/install.mjs
npx playwright install chromium chromium-headless-shell
pwsh ./run.ps1 -Repeats 7 -IncludeReusePage
```

The result is `out/benchmark.json`, `out/benchmark.csv`, `out/REPORT.md` and one
sample PNG per engine per case in `out/samples/`.

`bench/run.ps1` next door is a different benchmark: it compares shot against a
Chromium built from this same tree, over native CLI flags. This one compares the
npm packages a caller would actually reach for.

## The engines

| id | what it runs |
|---|---|
| `shotium` | `shotium.exe --serve` workers driven by this checkout's `shotium/` |
| `shotium-daemon` | the same workers, resident, reached over a named pipe |
| `puppeteer-shell` | `puppeteer.launch({headless: 'shell'})` — chrome-headless-shell |
| `puppeteer-chrome` | `puppeteer.launch({headless: true})` — headless Chrome |
| `playwright-shell` | `chromium.launch({channel: 'chromium-headless-shell'})` |
| `playwright-chrome` | `chromium.launch({channel: 'chromium'})` |

Both Chrome-based libraries get two rows because their defaults differ from
their fastest configuration, and quoting only one of them would be a choice
made on someone else's behalf. Playwright's default and its
`chromium-headless-shell` channel run the same binary; `channel: 'chromium'` is
what selects full `chrome.exe`. That was established by looking at the running
processes, not at a changelog.

## What is held equal

- **Default launch configuration.** No tuning flags on any side. What you get
  from `npm install puppeteer` and from `require('@shotkit/shotium')`.
- **One fresh page per screenshot**, closed afterwards. shot builds and tears
  down a `Page` per request whether you want it or not, so holding one page open
  across ten documents would be measuring a different thing. It is a faster
  thing, so `-IncludeReusePage` measures it too and the report carries both
  rows — for the sequential scenarios. Holding four pages open *and* using them
  at once is a configuration full headless Chrome does not like: captures of the
  pages that were not in front took tens of seconds each on this host, which
  times Chrome's frame scheduling rather than screenshot throughput.
- **`waitUntil: 'load'`** everywhere.
- **PNG, viewport only, 1280×720, deviceScaleFactor 1**, returned as bytes in
  process. Nothing writes to disk on the timed path.
- **No disk cache anywhere.** A warm HTTP cache on one side would be measuring
  the cache.
- **The corpus** is `bench/cases.json` minus its loopback HTTP case: ten static
  local documents — CSS, flex, grid, text, webfonts, images, gradients, filters,
  a long page.
- **One fresh process tree per sample**, sampled from outside.

## What is measured

`runner.js` measures what happens inside itself. `run.ps1` measures the process
tree from outside and joins the two on wall-clock timestamps. Neither can
flatter the other: a process cannot honestly measure its own startup, and a
sampler outside cannot see which millisecond belonged to which page.

| scenario | what it answers |
|---|---|
| `cold` | one screenshot from nothing: node, `require`, launch, render |
| `cold-settled` | the same, one second after launch — how much of a cold start is background work that finishes on its own |
| `warm` | the marginal cost of one more screenshot, startup entirely excluded |
| `batch` | ten different documents, sequentially, on a warm engine |
| `batch-parallel` | the same ten, four in flight |
| `reuse` | a fresh short-lived process against an engine that is already up, plus what that resident engine costs while idle |

`reuse` is the scenario the shotium daemon exists for, so the other side gets
its own equivalent: puppeteer attaches to a browser over `browserWSEndpoint`,
playwright connects to a `chromium.launchServer()`. Each resident engine is
warmed with a throwaway screenshot before the measurement, exactly as the
shotium daemon prewarms its workers.

Memory is reported twice, because a tree of processes does not have one number.

`peak RSS` is the maximum sampled sum of **working sets** over the tree, with
the breakdown taken at that same instant — the maxima of the parts do not have
to happen together, and adding them up would report a total the machine never
held. It is what task manager shows, and it charges every process separately
for the pages it shares with its siblings: four shot workers each map the same
43 MiB of `shotium.exe`, twenty-one chrome processes each map the same
`chrome.dll`, and the sum counts all of it once per process.

`private` is the sum of the **private working sets** at that same instant — the
pages belonging to exactly one process, nothing counted twice. On this tree the
first number came to about 3.8× the second.

Neither one alone is the answer. Private working set leaves out a binary the
machine genuinely has to keep resident; working set counts that binary once per
process mapping it. The real cost is between them, and the two columns bracket
it.

`engine RSS` is peak RSS with the node processes taken out, because all four
engines are driven from node and the Node heap is not the thing being compared.

## What it does not measure

- **Script.** The corpus is static documents. shot has no JavaScript engine, so
  a page that builds itself in the browser photographs as an empty page. No
  number here changes that, and none of these numbers apply to that case.
- **The network.** Every case is a local file.
- **Fidelity.** Geometry is checked; pixels are not compared across engines,
  which rasterise text differently by design. The sample PNGs are written for
  exactly this reason: a number nobody looked at a picture for is not a result.
- **A quiet machine.** These are desktop numbers, taken with a browser and
  several other things running. Read the median and the worst case together —
  the worst case is one sample, not a distribution — and re-run on the hardware
  you care about.
