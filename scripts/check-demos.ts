// Run the reftest suite in shot/testdata/demos.
//
// Every test is a pair: NAME.html exercises a feature, NAME-ref.html produces
// the same pixels using only absolutely positioned blocks with a
// background-color, or the same text through a path the feature under test
// does not touch. If the two renders are not byte-identical, the feature is
// broken. There are no golden images, so nothing here goes stale when the
// renderer legitimately changes.
//
// That matters for this tree specifically. Cutting code out of Blink does not
// usually produce a build error when it goes wrong -- it produces a page that
// lays out slightly differently, and a suite that compares against stored
// images would either have to be re-blessed after every cut (hiding
// regressions) or would drown in diffs. A reftest states the expected result
// in CSS the cut cannot plausibly break, so it keeps meaning the same thing.
//
// A page with no -ref.html is a smoke test instead: it must render, produce
// more than one distinct colour, and produce identical bytes on a second run.
// That is for features whose output cannot be restated exactly -- blurs,
// shadows, anything with antialiasing -- where the useful question is only
// whether it still runs and still runs deterministically.
//
// A test may allow a bounded difference the way WPT does, by declaring
//
//     <meta name="fuzzy" content="maxDifference=1;totalPixels=0-5000">
//
// Use it only where the difference is inherent rather than suspicious: two
// correct code paths that round in different places, for instance. The
// allowance is stated in the test file so it sits next to the reason for it,
// and both bounds are upper limits -- a render that differs by less still
// passes. maxDifference is a per-channel bound, which is not pixelmatch's
// perceptual threshold; the comparison is the plain loop in lib/png.ts.
//
//   pnpm verify:demos out/Shot/shotium.exe [--filter SUBSTRING] [--jobs N] [--out DIR]
//
// Relative paths are resolved against the repository root.

import {existsSync, mkdirSync, mkdtempSync, openSync, readFileSync, readSync, closeSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import pLimit from 'p-limit';

import {decodePng, describeDifference, distinctColours, measure} from './lib/png.ts';
import {resolve} from './lib/repo.ts';

const DEMOS = resolve('shot/testdata/demos');

// The demos are authored against this viewport. Both halves of a pair get it,
// so a mismatch is never the harness's doing.
const WIDTH = 400, HEIGHT = 200;

type Verdict = 'FAIL' | 'ERROR' | 'FUZZY' | 'SMOKE' | 'PASS';
interface Result {
  name: string;
  verdict: Verdict;
  detail: string;
}

async function render(exe: string, html: string, png: string): Promise<[Buffer | null, string]> {
  const proc = await execa(exe, ['--file', html, '--width', String(WIDTH), '--height', String(HEIGHT), '--output', png], {reject: false, all: true});
  if (!existsSync(png)) {
    const lines = (proc.all ?? '').trim().split(/\r?\n/).filter(Boolean);
    return [null, lines.length ? lines[lines.length - 1] : 'no output and no message'];
  }
  return [readFileSync(png), ''];
}

const FUZZY_RE = /<meta\s+name=["']?fuzzy["']?\s+content=["']([^"']+)["']/i;

// (max channel delta, max differing pixels) allowed by the test.
function readFuzzy(html: string): [number, number] {
  const fd = openSync(html, 'r');
  const head = Buffer.alloc(4096);
  const n = readSync(fd, head, 0, 4096, 0);
  closeSync(fd);
  const m = FUZZY_RE.exec(head.subarray(0, n).toString('utf8'));
  if (!m) return [0, 0];
  let delta = 0, pixels = 0;
  for (const part of m[1].split(';')) {
    const [key, value = ''] = part.trim().split('=');
    const hi = value.split('-').pop() ?? '0';
    if (key.trim() === 'maxDifference') delta = Number(hi);
    else if (key.trim() === 'totalPixels') pixels = Number(hi);
  }
  return [delta, pixels];
}

async function runOne(exe: string, tmp: string, name: string): Promise<Result> {
  const test = path.join(DEMOS, `${name}.html`);
  const ref = path.join(DEMOS, `${name}-ref.html`);
  const [got, err] = await render(exe, test, path.join(tmp, `${name}.png`));
  if (got === null) return {name, verdict: 'FAIL', detail: `did not render: ${err}`};

  if (existsSync(ref)) {
    const [want, refErr] = await render(exe, ref, path.join(tmp, `${name}-ref.png`));
    if (want === null) return {name, verdict: 'ERROR', detail: `the reference did not render: ${refErr}`};
    if (got.equals(want)) return {name, verdict: 'PASS', detail: 'matches its reference'};

    const [maxDelta, maxPixels] = readFuzzy(test);
    const wantImage = decodePng(want), gotImage = decodePng(got);
    if (maxDelta || maxPixels) {
      const m = measure(wantImage, gotImage);
      if (m.kind === 'diff' && m.worst <= maxDelta && m.moved <= maxPixels) {
        return {name, verdict: 'FUZZY', detail: `within the declared allowance: ${m.moved} px differ by at most ${m.worst} (allowed ${maxPixels} px, ${maxDelta})`};
      }
    }
    return {name, verdict: 'FAIL', detail: describeDifference(wantImage, gotImage)};
  }

  // Smoke test.
  const n = distinctColours(decodePng(got));
  if (n < 2) return {name, verdict: 'FAIL', detail: 'rendered a single flat colour'};
  const [again] = await render(exe, test, path.join(tmp, `${name}.2.png`));
  if (again === null || !again.equals(got)) {
    // Say how they differ, not just that they do. The two answers look nothing
    // alike and lead in opposite directions: a handful of pixels off by one or
    // two is text rasterising against a font cache that was cold for the first
    // render, while a large or structural difference is the engine itself
    // being non-deterministic. Reporting only "differ" once cost a CI round
    // that could not distinguish them.
    const how = again === null ? 'the second render produced nothing' : describeDifference(decodePng(got), decodePng(again));
    return {name, verdict: 'FAIL', detail: `two renders of the same page differ: ${how}`};
  }
  return {name, verdict: 'SMOKE', detail: `renders, deterministic, ${n >= 64 ? '>=' : ''}${n} colours`};
}

async function main(exeArg: string, opts: {filter: string; jobs: number; out?: string}): Promise<number> {
  const exe = resolve(exeArg);
  if (!existsSync(exe)) {
    console.log(`no such binary: ${exe}`);
    return 2;
  }
  if (!existsSync(DEMOS)) {
    console.log(`no demos directory at ${DEMOS}`);
    return 2;
  }
  const {readdirSync} = await import('node:fs');
  let names = readdirSync(DEMOS).filter((f) => f.endsWith('.html') && !f.endsWith('-ref.html')).map((f) => f.slice(0, -5)).sort();
  if (opts.filter) names = names.filter((n) => n.includes(opts.filter));
  if (names.length === 0) {
    console.log('no demos matched');
    return 2;
  }

  const tmp = opts.out ? resolve(opts.out) : mkdtempSync(path.join(os.tmpdir(), 'shot-demos-'));
  mkdirSync(tmp, {recursive: true});

  // The first render on a cold system font cache can differ from every later
  // one, which reads as a determinism failure on whichever text-heavy demo
  // happens to run first. Warm the cache once, on a page outside the suite, so
  // that cost lands somewhere it cannot be mistaken for a result.
  const warm = path.join(tmp, '_warmup.html');
  writeFileSync(warm, '<!doctype html><meta charset="utf-8"><div style="font:16px serif">Aa Bb 0123 漢字 עב</div>' +
      '<div style="font:16px sans-serif">Aa Bb</div><div style="font:16px monospace">Aa Bb</div>\n');
  await render(exe, warm, path.join(tmp, '_warmup.png'));

  const limit = pLimit(opts.jobs);
  const results = await Promise.all(names.map((n) => limit(() => runOne(exe, tmp, n))));

  const order: Record<Verdict, number> = {FAIL: 0, ERROR: 1, FUZZY: 2, SMOKE: 3, PASS: 4};
  results.sort((a, b) => order[a.verdict] - order[b.verdict] || a.name.localeCompare(b.name));
  for (const r of results) console.log(`  ${r.verdict.padEnd(6)} ${r.name.padEnd(22)} ${r.detail}`);

  const counts: Partial<Record<Verdict, number>> = {};
  for (const r of results) counts[r.verdict] = (counts[r.verdict] ?? 0) + 1;
  console.log();
  console.log(Object.keys(counts).sort().map((k) => `${k} ${counts[k as Verdict]}`).join('  '));
  const bad = (counts.FAIL ?? 0) + (counts.ERROR ?? 0);
  if (bad) {
    console.log(`\n${bad} of ${results.length} demos FAILED`);
    if (opts.out) console.log(`renders kept in ${tmp}`);
    return 1;
  }
  console.log(`\nALL ${results.length} DEMOS PASSED`);
  return 0;
}

const cli = cac('check-demos');
cli.command('<exe>', 'run the reftest pairs and smoke pages in shot/testdata/demos')
    .option('--filter <substring>', 'only demos whose name contains this', {default: ''})
    .option('--jobs <n>', 'renders in parallel', {default: 8})
    .option('--out <dir>', 'keep the PNGs in this directory')
    .action(async (exe: string, options: {filter: string; jobs: number; out?: string}) => {
      process.exitCode = await main(exe, {...options, jobs: Number(options.jobs)});
    });
cli.help();
cli.parse();
