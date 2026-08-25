'use strict';

// benchmark.json -> REPORT.md.
//
// Every table is a median and a worst case over the repeats, never a single run
// and never a mean: on a desktop with a browser and a build system on it, the
// mean is whatever the noisiest sample was, and the spread is the part worth
// reading.
//
// `worst` rather than p95 wherever the sample count is the repeat count. With
// five or seven runs, the 95th percentile *is* the slowest one, and calling it
// a percentile dresses up a single bad sample as a distribution. Per-screenshot
// timings are the exception -- there are ten of those per run -- so those keep
// a real p95.
//
//   node report.js out/benchmark.json

const fs = require('fs');
const path = require('path');

const source = process.argv[2] || path.join(__dirname, 'out', 'benchmark.json');
const report = JSON.parse(fs.readFileSync(source, 'utf8'));
const outputPath = path.join(path.dirname(source), 'REPORT.md');

const ENGINE_LABELS = {
  'shotium': 'shotium (shot.exe pool)',
  'shotium-daemon': 'shotium (resident daemon)',
  'puppeteer-shell': 'puppeteer, chrome-headless-shell',
  'puppeteer-chrome': 'puppeteer, headless Chrome',
  'playwright-shell': 'playwright, chrome-headless-shell',
  'playwright-chrome': 'playwright, headless Chrome',
};

const ORDER = [
  'shotium', 'shotium-daemon', 'puppeteer-shell', 'puppeteer-chrome',
  'playwright-shell', 'playwright-chrome',
];

const ms = (value) => (value === null || value === undefined) ? '--' :
                                                               `${value.toFixed(1)}`;
const mib = (bytes) => (bytes === null || bytes === undefined) ?
    '--' :
    `${(bytes / (1024 * 1024)).toFixed(1)}`;

function rowsFor(scenario, {reusePage = false} = {}) {
  return report.summary
      .filter((s) => s.scenario === scenario && Boolean(s.reuse_page) === reusePage)
      .sort((a, b) => ORDER.indexOf(a.engine) - ORDER.indexOf(b.engine));
}

function label(engine, reusePage) {
  const name = ENGINE_LABELS[engine] || engine;
  return reusePage ? `${name}, one page reused` : name;
}

function table(header, alignments, lines) {
  return [
    `| ${header.join(' | ')} |`,
    `|${alignments.map((a) => a === 'l' ? ':--' : '--:').join('|')}|`,
    ...lines.map((cells) => `| ${cells.join(' | ')} |`),
  ].join('\n');
}

const out = [];
const config = report.config;
const host = report.host;

out.push('# Cross-engine screenshot benchmark');
out.push('');
out.push(`Generated ${report.generated_utc} from \`${report.source_revision || 'unknown revision'}\`.`);
out.push('');
out.push(table(
    ['', ''], ['l', 'l'],
    [
      ['host', `${host.processor || 'unknown CPU'}, ${host.logical_processors} logical processors`],
      ['os', host.os],
      ['node', host.node],
      ['shot.exe', `${mib(report.engines.shot.bytes)} MiB, sha256 ${report.engines.shot.sha256.slice(0, 16)}`],
      ['puppeteer', report.engines.packages.puppeteer || 'not installed'],
      ['playwright', report.engines.packages.playwright || 'not installed'],
      ['repeats', `${config.repeats} per cell`],
      ['warm iterations', `${config.iterations} timed shots after ${config.warmup} warmups`],
      ['concurrency', `${config.concurrency}`],
      ['corpus', report.measurement_model.corpus],
    ]));
out.push('');
out.push('Every number below is milliseconds unless it says MiB.');
out.push('');
out.push('Memory is two columns, because one column cannot say what a tree of');
out.push('processes costs. `peak RSS` is the maximum sampled sum of working sets');
out.push('over the whole tree, node included -- the number task manager adds up,');
out.push('and the one that charges every process separately for pages they share.');
out.push('Four shot workers each map the same 43 MiB of shot.exe; twenty-one');
out.push('chrome processes each map the same chrome.dll. `private` is the sum of');
out.push('the private working sets at that same instant: the pages that belong to');
out.push('exactly one process, with nothing counted twice. The truth is between');
out.push('them -- the shared part is real memory, it is just real once rather than');
out.push('once per process. `engine RSS` is peak RSS with the node processes taken');
out.push('out.');
out.push('');

out.push('## 1. Cold start');
out.push('');
out.push('From `node runner.js` to a PNG in hand, with nothing warm: process');
out.push('startup, `require`, engine launch, one screenshot. This is what a');
out.push('one-shot invocation costs.');
out.push('');
out.push(table(
    ['engine', 'wall p50', 'wall worst', 'require', 'launch', 'first shot',
     'peak RSS (MiB)', 'private (MiB)', 'procs'],
    ['l', 'r', 'r', 'r', 'r', 'r', 'r', 'r', 'r'],
    rowsFor('cold').map((s) => [
      label(s.engine, s.reuse_page),
      ms(s.wall_time_ms.p50),
      ms(s.wall_time_ms.max),
      ms(report.samples.filter((r) => r.engine === s.engine && r.scenario === 'cold')
             .map((r) => r.require_ms)
             .sort((a, b) => a - b)[0]),
      ms(s.launch_ms ? s.launch_ms.p50 : null),
      ms(s.first_shot_ms ? s.first_shot_ms.p50 : null),
      mib(s.peak_rss_bytes ? s.peak_rss_bytes.max : null),
      mib(s.peak_private_bytes ? s.peak_private_bytes.max : null),
      s.peak_processes ? s.peak_processes.max : '--',
    ])));
out.push('');

out.push('## 2. Cold start, one second later');
out.push('');
out.push('The same first screenshot, taken a second after launch returned. The');
out.push('difference from table 1 is work an engine finishes on its own once it is');
out.push('running -- which a caller who starts the engine at boot never pays for,');
out.push('and a caller who starts it per request always does.');
out.push('');
out.push(table(
    ['engine', 'first shot p50', 'first shot worst', 'cold first shot p50', 'settled by'],
    ['l', 'r', 'r', 'r', 'r'],
    rowsFor('cold-settled').map((s) => {
      const cold = rowsFor('cold').find((c) => c.engine === s.engine);
      const coldFirst = cold && cold.first_shot_ms ? cold.first_shot_ms.p50 : null;
      const settled = s.first_shot_ms ? s.first_shot_ms.p50 : null;
      return [
        label(s.engine, s.reuse_page),
        ms(settled),
        ms(s.first_shot_ms ? s.first_shot_ms.max : null),
        ms(coldFirst),
        coldFirst && settled ? `${(coldFirst - settled).toFixed(1)} ms` : '--',
      ];
    })));
out.push('');

out.push('## 3. Warm: the same page, over and over');
out.push('');
out.push(`One page, ${config.warmup} warmups thrown away, ${config.iterations} timed`);
out.push('screenshots. Startup is entirely out of this number: it is the marginal');
out.push('cost of one more screenshot on an engine that is already running.');
out.push('');
const warmRows = [...rowsFor('warm'), ...rowsFor('warm', {reusePage: true})];
out.push(table(
    ['engine', 'per shot p50', 'p95', 'max', 'peak RSS (MiB)', 'private (MiB)',
     'engine RSS (MiB)', 'procs'],
    ['l', 'r', 'r', 'r', 'r', 'r', 'r', 'r'],
    warmRows.map((s) => [
      label(s.engine, s.reuse_page),
      ms(s.shot_ms.p50),
      ms(s.shot_ms.p95),
      ms(s.shot_ms.max),
      mib(s.peak_rss_bytes.max),
      mib(s.peak_private_bytes ? s.peak_private_bytes.max : null),
      mib(s.peak_rss_engine_bytes ? s.peak_rss_engine_bytes.max : null),
      s.peak_processes.max,
    ])));
out.push('');

out.push('## 4. Ten different pages, one at a time');
out.push('');
out.push('The corpus, sequentially, on a warm engine. Ten documents rather than one');
out.push('page ten times, so nothing is answering out of a cache it built on the');
out.push('previous iteration.');
out.push('');
const batchRows = [...rowsFor('batch'), ...rowsFor('batch', {reusePage: true})];
out.push(table(
    ['engine', 'ten pages p50', 'worst', 'per page p50', 'peak RSS (MiB)',
     'private (MiB)', 'procs'],
    ['l', 'r', 'r', 'r', 'r', 'r', 'r'],
    batchRows.map((s) => [
      label(s.engine, s.reuse_page),
      ms(s.total_ms.p50),
      ms(s.total_ms.max),
      ms(s.shot_ms.p50),
      mib(s.peak_rss_bytes.max),
      mib(s.peak_private_bytes ? s.peak_private_bytes.max : null),
      s.peak_processes.max,
    ])));
out.push('');

out.push(`## 5. Ten different pages, ${config.concurrency} at a time`);
out.push('');
out.push(`The same ten with ${config.concurrency} in flight: ${config.concurrency} shot.exe`);
out.push(`workers on one side, ${config.concurrency} pages on the other.`);
out.push('');
const parallelRows =
    [...rowsFor('batch-parallel'), ...rowsFor('batch-parallel', {reusePage: true})];
out.push(table(
    ['engine', 'ten pages p50', 'worst', 'pages/s', 'peak RSS (MiB)',
     'private (MiB)', 'procs', 'threads'],
    ['l', 'r', 'r', 'r', 'r', 'r', 'r', 'r'],
    parallelRows.map((s) => [
      label(s.engine, s.reuse_page),
      ms(s.total_ms.p50),
      ms(s.total_ms.max),
      (10000 / s.total_ms.p50).toFixed(1),
      mib(s.peak_rss_bytes.max),
      mib(s.peak_private_bytes ? s.peak_private_bytes.max : null),
      s.peak_processes.max,
      s.peak_threads.max,
    ])));
out.push('');

out.push('## 6. Reuse: a fresh process against an engine that is already up');
out.push('');
out.push('A short-lived client -- a CLI invocation, a queue worker, a request');
out.push('handler -- attaching to a resident engine and taking one screenshot.');
out.push('shotium connects to its daemon over a named pipe; puppeteer attaches to a');
out.push('browser over `browserWSEndpoint`; playwright connects to a');
out.push('`launchServer()`. The resident columns are what each of those costs while');
out.push('nothing at all is happening -- sampled after every engine has been left');
out.push(`alone for ${(config.resident_settle_ms || 0) / 1000}s, so what is measured is the cost of being`);
out.push('there rather than the tail of the warmup shot. `engine only` takes the');
out.push('node processes out of the resident total, which for shotium is most of');
out.push('what is left: its workers give their pages back when the queue goes');
out.push('quiet, and the node supervising them does not.');
out.push('');
out.push(table(
    ['engine', 'client wall p50', 'connect p50', 'shot p50', 'resident RSS (MiB)',
     'resident private (MiB)', 'engine only (MiB)', 'resident procs'],
    ['l', 'r', 'r', 'r', 'r', 'r', 'r', 'r'],
    rowsFor('reuse').map((s) => {
      const resident = report.resident.find(
          (r) => r.host_engine === s.engine || r.engine === s.engine);
      return [
        label(s.engine, s.reuse_page),
        ms(s.wall_time_ms.p50),
        ms(s.connect_ms ? s.connect_ms.p50 : null),
        ms(s.first_shot_ms ? s.first_shot_ms.p50 : null),
        resident ? mib(resident.resident_rss_bytes) : '--',
        resident ? mib(resident.resident_private_bytes) : '--',
        resident ? mib(resident.resident_engine_bytes) : '--',
        resident ? resident.resident_processes : '--',
      ];
    })));
out.push('');

if (report.failures && report.failures.length) {
  out.push('## Samples that failed');
  out.push('');
  out.push('A cell with failures has fewer samples behind it than the others, and');
  out.push('a cell with nothing but failures is missing from the tables above.');
  out.push('');
  out.push(table(
      ['engine', 'scenario', 'repeat', 'error'], ['l', 'l', 'r', 'l'],
      report.failures.map((f) => [
        label(f.engine, f.reuse_page),
        f.scenario,
        f.repeat,
        String(f.error).replace(/\s+/g, ' ').slice(0, 160),
      ])));
  out.push('');
}

out.push('## What this does not measure');
out.push('');
out.push('- **Script.** The corpus is static documents. shot has no JavaScript');
out.push('  engine at all, so a page that builds itself in the browser photographs');
out.push('  as an empty page -- no benchmark number changes that, and none of these');
out.push('  numbers apply to that case.');
out.push('- **The network.** Every case is a local file, so no engine is being');
out.push('  timed on its HTTP stack, its DNS, or a server\'s latency.');
out.push('- **Fidelity.** Identical geometry is checked; identical pixels are not.');
out.push('  The engines rasterise text differently by design. The sample PNGs are');
out.push('  written next to this report, one per engine per case, to be looked at.');
out.push('- **A quiet machine.** These were taken on a desktop that was doing other');
out.push('  things -- that is what the worst-case columns are for, and one of them');
out.push('  is a browser wedging for a minute rather than a slow render.');
out.push('');

fs.writeFileSync(outputPath, `${out.join('\n')}\n`);
process.stdout.write(`${outputPath}\n`);
