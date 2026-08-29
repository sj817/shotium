> **Legacy local result.** This was produced on one Windows workstation by the
> retired PowerShell/JavaScript harness. It is preserved for historical
> evidence only and is not part of the six-platform CI result series.

<!-- Historical generator: retired bench/cross implementation. -->

# Cross-engine screenshot benchmark

Generated 2026-08-25T13:06:10.2803793Z from `1969e13118eb5c4cff8dbaadbad50620b2f047a1`.

|  |  |
|:--|:--|
| host | unknown CPU, 32 logical processors |
| os | Microsoft Windows NT 10.0.26340.0 |
| node | v22.22.2 |
| shot.exe | 41.6 MiB, sha256 39cb71dda72d3690 |
| puppeteer | 25.8.0 |
| playwright | 1.62.1 |
| repeats | 7 per cell |
| warm iterations | 10 timed shots after 3 warmups |
| concurrency | 4 |
| corpus | bench/cases.json, local cases only, 1280x720, scale 1, PNG, viewport |

Every number below is milliseconds unless it says MiB.

Memory is two columns, because one column cannot say what a tree of
processes costs. `peak RSS` is the maximum sampled sum of working sets
over the whole tree, node included -- the number task manager adds up,
and the one that charges every process separately for pages they share.
Four shot workers each map the same 43 MiB of shot.exe; twenty-one
chrome processes each map the same chrome.dll. `private` is the sum of
the private working sets at that same instant: the pages that belong to
exactly one process, with nothing counted twice. The truth is between
them -- the shared part is real memory, it is just real once rather than
once per process. `engine RSS` is peak RSS with the node processes taken
out.

## 1. Cold start

From `node runner.js` to a PNG in hand, with nothing warm: process
startup, `require`, engine launch, one screenshot. This is what a
one-shot invocation costs.

| engine | wall p50 | wall worst | require | launch | first shot | peak RSS (MiB) | private (MiB) | procs |
|:--|--:|--:|--:|--:|--:|--:|--:|--:|
| shotium (shot.exe pool) | 352.0 | 625.8 | 5.2 | 111.5 | 79.7 | 187.0 | 39.9 | 6 |
| puppeteer, chrome-headless-shell | 946.2 | 1376.1 | 132.7 | 268.1 | 283.8 | 365.1 | 104.6 | 8 |
| puppeteer, headless Chrome | 1559.1 | 2607.9 | 144.6 | 498.2 | 532.4 | 552.9 | 165.7 | 11 |
| playwright, chrome-headless-shell | 961.7 | 1360.5 | 207.1 | 217.3 | 258.8 | 359.8 | 133.3 | 6 |
| playwright, headless Chrome | 1384.5 | 2041.6 | 223.7 | 340.8 | 453.9 | 523.6 | 173.9 | 10 |

## 2. Cold start, one second later

The same first screenshot, taken a second after launch returned. The
difference from table 1 is work an engine finishes on its own once it is
running -- which a caller who starts the engine at boot never pays for,
and a caller who starts it per request always does.

| engine | first shot p50 | first shot worst | cold first shot p50 | settled by |
|:--|--:|--:|--:|--:|
| shotium (shot.exe pool) | 63.2 | 64.5 | 79.7 | 16.5 ms |
| puppeteer, chrome-headless-shell | 150.8 | 174.3 | 283.8 | 133.0 ms |
| puppeteer, headless Chrome | 442.8 | 507.2 | 532.4 | 89.6 ms |
| playwright, chrome-headless-shell | 160.1 | 186.0 | 258.8 | 98.7 ms |
| playwright, headless Chrome | 258.6 | 265.6 | 453.9 | 195.3 ms |

## 3. Warm: the same page, over and over

One page, 3 warmups thrown away, 10 timed
screenshots. Startup is entirely out of this number: it is the marginal
cost of one more screenshot on an engine that is already running.

| engine | per shot p50 | p95 | max | peak RSS (MiB) | private (MiB) | engine RSS (MiB) | procs |
|:--|--:|--:|--:|--:|--:|--:|--:|
| shotium (shot.exe pool) | 46.9 | 48.5 | 50.4 | 189.6 | 42.1 | 131.6 | 6 |
| puppeteer, chrome-headless-shell | 133.3 | 158.5 | 202.8 | 376.8 | 107.7 | 299.9 | 8 |
| puppeteer, headless Chrome | 131.7 | 175.9 | 197.4 | 1203.0 | 345.4 | 1124.5 | 21 |
| playwright, chrome-headless-shell | 149.9 | 205.3 | 234.9 | 379.8 | 145.7 | 256.5 | 6 |
| playwright, headless Chrome | 145.7 | 171.6 | 193.9 | 563.3 | 209.9 | 441.1 | 10 |
| puppeteer, chrome-headless-shell, one page reused | 60.5 | 76.6 | 80.8 | 478.2 | 115.3 | 402.3 | 8 |
| puppeteer, headless Chrome, one page reused | 49.6 | 68.2 | 133.2 | 575.7 | 183.2 | 499.2 | 11 |
| playwright, chrome-headless-shell, one page reused | 33.4 | 46.9 | 51.7 | 484.9 | 153.4 | 364.1 | 6 |
| playwright, headless Chrome, one page reused | 45.3 | 50.1 | 52.3 | 553.8 | 206.2 | 433.3 | 10 |

## 4. Ten different pages, one at a time

The corpus, sequentially, on a warm engine. Ten documents rather than one
page ten times, so nothing is answering out of a cache it built on the
previous iteration.

| engine | ten pages p50 | worst | per page p50 | peak RSS (MiB) | private (MiB) | procs |
|:--|--:|--:|--:|--:|--:|--:|
| shotium (shot.exe pool) | 753.6 | 813.5 | 61.0 | 208.2 | 56.9 | 6 |
| puppeteer, chrome-headless-shell | 1585.4 | 1617.3 | 148.7 | 404.2 | 126.9 | 8 |
| puppeteer, headless Chrome | 2266.3 | 2694.5 | 217.8 | 1239.7 | 376.5 | 21 |
| playwright, chrome-headless-shell | 1504.0 | 1548.9 | 141.6 | 401.2 | 156.4 | 6 |
| playwright, headless Chrome | 1920.6 | 2275.6 | 188.0 | 596.1 | 236.5 | 10 |
| puppeteer, chrome-headless-shell, one page reused | 801.9 | 865.0 | 74.2 | 510.3 | 138.0 | 8 |
| puppeteer, headless Chrome, one page reused | 1527.7 | 1567.6 | 143.6 | 617.3 | 221.3 | 11 |
| playwright, chrome-headless-shell, one page reused | 564.6 | 583.0 | 46.9 | 520.9 | 174.7 | 6 |
| playwright, headless Chrome, one page reused | 998.1 | 1125.1 | 65.8 | 592.3 | 238.1 | 10 |

## 5. Ten different pages, 4 at a time

The same ten with 4 in flight: 4 shot.exe
workers on one side, 4 pages on the other.

| engine | ten pages p50 | worst | pages/s | peak RSS (MiB) | private (MiB) | procs | threads |
|:--|--:|--:|--:|--:|--:|--:|--:|
| shotium (shot.exe pool) | 236.9 | 267.0 | 42.2 | 256.1 | 72.7 | 6 | 144 |
| puppeteer, chrome-headless-shell | 904.5 | 1039.1 | 11.1 | 646.8 | 180.2 | 11 | 185 |
| puppeteer, headless Chrome | 1889.9 | 2380.2 | 5.3 | 1286.8 | 378.7 | 22 | 444 |
| playwright, chrome-headless-shell | 1170.6 | 1981.1 | 8.5 | 652.4 | 214.8 | 9 | 180 |
| playwright, headless Chrome | 1275.8 | 2435.5 | 7.8 | 789.3 | 282.3 | 13 | 312 |

## 6. Reuse: a fresh process against an engine that is already up

A short-lived client -- a CLI invocation, a queue worker, a request
handler -- attaching to a resident engine and taking one screenshot.
shotium connects to its daemon over a named pipe; puppeteer attaches to a
browser over `browserWSEndpoint`; playwright connects to a
`launchServer()`. The resident columns are what each of those costs while
nothing at all is happening -- sampled after every engine has been left
alone for 15s, so what is measured is the cost of being
there rather than the tail of the warmup shot. `engine only` takes the
node processes out of the resident total, which for shotium is most of
what is left: its workers give their pages back when the queue goes
quiet, and the node supervising them does not.

| engine | client wall p50 | connect p50 | shot p50 | resident RSS (MiB) | resident private (MiB) | engine only (MiB) | resident procs |
|:--|--:|--:|--:|--:|--:|--:|--:|
| shotium (resident daemon) | 250.1 | 2.3 | 56.5 | 58.3 | 10.4 | 2.8 | 5 |
| puppeteer, chrome-headless-shell | 511.7 | 16.8 | 169.9 | 354.9 | 96.3 | 287.2 | 8 |
| puppeteer, headless Chrome | 587.9 | 18.7 | 203.6 | 587.0 | 163.6 | 519.4 | 12 |
| playwright, chrome-headless-shell | 764.0 | 37.6 | 189.2 | 272.2 | 108.0 | 154.3 | 5 |
| playwright, headless Chrome | 679.5 | 34.4 | 227.9 | 399.5 | 133.2 | 298.8 | 7 |

## What this does not measure

- **Script.** The corpus is static documents. shot has no JavaScript
  engine at all, so a page that builds itself in the browser photographs
  as an empty page -- no benchmark number changes that, and none of these
  numbers apply to that case.
- **The network.** Every case is a local file, so no engine is being
  timed on its HTTP stack, its DNS, or a server's latency.
- **Fidelity.** Identical geometry is checked; identical pixels are not.
  The engines rasterise text differently by design. The sample PNGs are
  written next to this report, one per engine per case, to be looked at.
- **A quiet machine.** These were taken on a desktop that was doing other
  things -- that is what the worst-case columns are for, and one of them
  is a browser wedging for a minute rather than a slow render.

