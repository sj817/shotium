# Candidate versus the published npm package

Build the candidate native library and JavaScript first. Compare actual installed
packages, on the same native host, without running builds or other render tests
at the same time:

```sh
node tools/shot/node_perf.cjs BASELINE_PACKAGE CANDIDATE_PACKAGE result.json --calibrate --check
python tools/shot/node_perf_images.py result.json
```

The current branch's requested acceptance scope is Windows x64. After its full
matrix completes, generate that scope's report with:

```sh
python tools/shot/node_perf_report.py result.json --platform win32-x64 --output report.md
```

This accepts only the measured Windows x64 scope. The optional workflow below
can separately require all six published platforms.

The 108-case matrix contains 69 render cases, 13 HTTP/cache cases, 3 fresh-process
startup cases, 3 memory-release/restart cases, 7 queue/multi-process cases,
8 daemon cases, 4 failure/recovery cases and a 1000-pair soak. Render cases include
both localized Bilibili articles, the existing benchmark corpus, PNG/JPEG/WebP,
full-page, selector, clip, alpha, file output and scale 0.5/1/1.5/2/4/8 paths.
The two whole articles and the new tile API have no successful npm equivalent;
run `bilibili_check.py` separately to validate their full output, every tile,
every article photo and both footer QR codes. A rejected npm capture is never counted as a fast baseline.

Five warmups precede each resident case. Both versions run in alternating AB/BA
order, and all measured samples remain in the JSON. Each case takes at least
`--samples` pairs (20), goes on until each side has spent `--min-seconds`
(3) capturing, and then, if the p50 or mean ratio is still not known to
within `--precision` (2%, the half-width of its 99% interval), keeps going
until it is or until `--max-seconds` (8) per side, never past
`--max-samples` (1000) pairs. The noise of a median shrinks with the square
root of the pairs, and it is the quick cases -- where a few percent is a few
hundredths of a millisecond -- that need the most and can afford them: a 3 ms
case gets a thousand pairs in six seconds, a one-second page its twenty. The
time budget, not the pair count, is what sizes a case: a hundred pairs of a
second each was fifteen minutes of a run spent on eight cases whose answer
twenty pairs had already given, and the whole matrix now takes about twenty
minutes rather than an hour. A case whose two sides do the same work never
settles to 2%, so the ceiling is what bounds it, at 8 s a side. The stopping
rule reads only the interval's width, never which side of 1 it lies on, so
it cannot favour either answer. A cold start's `processStartMs` -- node's own
process starting -- is recorded and not judged; its engine start, import and
first capture are.

The verdict is calibrated, not absolute. `--calibrate` first times the
candidate against itself on six quick cases -- every true ratio there is 1, so
how far a statistic strays above 1 is this machine's noise -- and reads two
bands off the result, each a high percentile of what it saw and never below
2%: a body band from the p50 and mean bounds, and a tail band from the p95
bounds. Two, because the two have nothing like the same noise: on a 3 ms case
the median of a binary against itself lands within a few percent of 1 while
its p95 wanders by a third, and one band wide enough for the tail called a 30%
slowdown of the median "equivalent". Without `--calibrate` both bands are the
2% floor. A gate that demanded "no statistic may rise at all" was tried and
could not be passed by npm 0.3.4 against npm 0.3.4: five of seven cases were
labelled a regression by noise alone, which made every later label worthless.

For each case, a paired bootstrap gives a 99% interval of the candidate/npm
ratio for p50, mean and p95. A statistic whose whole interval is under 1 is
`faster`; one whose interval sits inside its band is `equivalent` -- not
distinguishable from the same binary; one whose interval is past its band is
`slower`; one whose interval straddles the band's far edge is `uncertain`. The
case is `slower` if any statistic is; else `faster` if the mean is faster, or
the median is faster with the mean inside its band; else `unproven` if p50 or
the mean is uncertain; else `equivalent`. The mean is the throughput, so a
mean whose whole interval is under 1 settles the case even while a noisy
median's does not; a quicker median needs the mean at least within the band,
so that it is not bought with a heavier tail. p95 only ever guards the tail:
it can make a case `slower`, when its whole interval is past the tail band,
and nothing else. A run's slowest few samples are the noisiest thing
measured, and a real median win is not rejected because they did not also
fall.

Acceptance asks for `faster` on every engine case -- raster, encode, layout,
file output, startup, cache -- and for `faster` or `equivalent` on the two
cases pinned to an external wait (`http-slow`, a server that sleeps 250 ms;
`http-networkidle`, a 500 ms quiet window), which no code in this process can
make quicker. `equivalent` on an engine case is a tie, and a tie is not
accepted. Reports show every case's dev/npm median, the difference in
milliseconds and the verdict first, with the intervals in the expanded
details. Throughput is captures per sample divided by mean sample time; the
mean statistic keeps a throughput loss from hiding behind the median. This is
a statistical acceptance test, not a proof for every possible HTML document,
machine or individual invocation.

Raw resource counters, timing phases, RSS, output hashes, first-pair images,
library/addon/bundle hashes, source revision/diff and fixture/harness hashes are
retained. Pixel verification decodes paired output at its original dimensions;
it requires every measured output hash to match its verified static-case image,
and rejects blank images, dimension changes, mean channel error above 1/255 or
more than 1% of channels differing by over 16/255. Those tolerances are fixed
before sampling. Startup includes package import and separately records process
launch, `Runtime.start()` and first capture; the OS filesystem cache is **not**
flushed. RSS is diagnostic and does not prove memory peaks between samples.

`--filter=REGEX` and `--shard=render|network|startup|lifecycle|parallel|soak|daemon|resilience`
are diagnostics. A filtered result cannot pass the complete matrix gate. A shard
can pass only its own scope; it does not replace a full platform result. Retrying
must write a new result file: keep earlier failures and uncertainty, and do not
select only the best run.

The `Candidate versus npm performance` workflow runs the full matrix on all six
published native platforms. First build all six with the engine workflows at
the same source SHA. Dispatch the performance workflow at that SHA with an exact
npm baseline version and a `build_runs` JSON object mapping `linux-x64`,
`linux-arm64`, `win32-x64`, `win32-arm64`, `darwin-x64`, and `darwin-arm64` to their
successful build run IDs. Preflight rejects different SHAs, missing/expired
artifacts and incomplete platform coverage. This workflow neither publishes npm
packages nor commits benchmark reports. Every platform must pass the article,
performance and pixel checks; a green static check is not this acceptance gate.
