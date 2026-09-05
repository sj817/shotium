// Break a shot-vs-oracle difference down by region, and say what kind it is.
//
// pixel-diff answers "how far apart are these two images". That number alone
// cannot satisfy the acceptance criterion for this tree, which is that every
// difference is measured *and accounted for*. A single 3% is compatible with
// "antialiasing is a shade different everywhere" and with "one element is
// missing entirely", and those call for opposite responses.
//
// So this splits the corpus into the regions it was built out of -- one per
// feature under test -- and reports each separately, plus a classification of
// the difference's shape:
//
//   coverage    fraction of the region's pixels that differ at all
//   visible     fraction differing by more than --threshold (default 8/255)
//   mean/max    magnitude
//   runs        mean horizontal run length of differing pixels. An edge that
//               is antialiased slightly differently gives runs of 1-2; a
//               region that is uniformly wrong gives runs as wide as the
//               region. This separates "the same picture, rasterised
//               differently" from "a different picture".
//   channels    per-channel mean delta. A difference that is equal across R,
//               G and B is geometric or gamma; one that is not is a colour
//               path.
//
// Regions are given as `name:x,y,w,h` on the command line, or read from a
// file with --regions (one per line, '#' comments allowed), so the corpus
// layout lives next to the corpus rather than in this script.
//
//   pnpm diff-report oracle.png shot.png --regions shot/testdata/regions.txt
//   pnpm diff-report oracle.png shot.png text:23,35,1200,160
//
// Relative paths are resolved against the repository root.

import {readFileSync} from 'node:fs';

import {cac} from 'cac';

import {crop, decodePng, type Image} from './lib/png.ts';
import {histogram, worstChannel} from './pixel-diff.ts';
import {resolve} from './lib/repo.ts';

export type Region = [string, [number, number, number, number]];  // name, [x0, y0, x1, y1]

export function parseRegion(spec: string): Region {
  const colon = spec.indexOf(':');
  const name = spec.slice(0, colon);
  const [x, y, w, h] = spec.slice(colon + 1).split(',').map(Number);
  return [name, [x, y, x + w, y + h]];
}

export function loadRegions(file: string | undefined, specs: string[]): Region[] {
  const regions: Region[] = [];
  if (file) {
    for (const raw of readFileSync(file, 'utf8').split(/\r?\n/)) {
      const line = raw.split('#', 1)[0].trim();
      if (line) regions.push(parseRegion(line));
    }
  }
  for (const spec of specs) if (spec.includes(':')) regions.push(parseRegion(spec));
  return regions;
}

// Mean length of a horizontal run of pixels at or over the threshold.
function meanRunLength(worst: Uint8Array, width: number, height: number, threshold: number): number {
  let runs = 0, total = 0;
  for (let y = 0; y < height; y++) {
    let run = 0;
    for (let x = 0; x < width; x++) {
      if (worst[y * width + x] >= threshold) {
        run++;
        total++;
      } else if (run) {
        runs++;
        run = 0;
      }
    }
    if (run) runs++;
  }
  return runs ? total / runs : 0;
}

function report(name: string, a: Image, b: Image, box: [number, number, number, number], threshold: number): string {
  const ra = crop(a, box[0], box[1], box[2] - box[0], box[3] - box[1]);
  const rb = crop(b, box[0], box[1], box[2] - box[0], box[3] - box[1]);
  const total = ra.width * ra.height;
  const worst = worstChannel(ra, rb);
  const hist = histogram(worst);
  const differing = hist.slice(1).reduce((s, n) => s + n, 0);
  const visible = hist.slice(threshold).reduce((s, n) => s + n, 0);
  let maxDelta = 0;
  for (let i = 255; i >= 0; i--) if (hist[i]) { maxDelta = i; break; }
  const mean = hist.reduce((s, n, i) => s + i * n, 0) / total;
  const channelMeans = [0, 1, 2].map((c) => {
    let sum = 0;
    for (let i = 0; i < total; i++) sum += Math.abs(ra.data[i * 4 + c] - rb.data[i * 4 + c]);
    return sum / total;
  });
  const runs = meanRunLength(worst, ra.width, ra.height, threshold);
  const pct = (n: number) => (100 * n / total).toFixed(3).padStart(7) + '%';
  return `${name.padEnd(22)} ${pct(differing)} ${pct(visible)} ${mean.toFixed(2).padStart(6)} ${String(maxDelta).padStart(4)} ${runs.toFixed(2).padStart(6)}   ${channelMeans.map((m) => m.toFixed(2)).join('/')}`;
}

export function diffReport(aPath: string, bPath: string, regions: Region[], threshold: number): number {
  const a = decodePng(readFileSync(aPath));
  const b = decodePng(readFileSync(bPath));
  if (a.width !== b.width || a.height !== b.height) {
    console.log(`size mismatch: (${a.width}, ${a.height}) vs (${b.width}, ${b.height})`);
    return 1;
  }
  const all: Region[] = [...regions, ['WHOLE IMAGE', [0, 0, a.width, a.height]]];
  console.log(`${'region'.padEnd(22)} ${'differ'.padStart(8)} ${'visible'.padStart(8)} ${'mean'.padStart(6)} ${'max'.padStart(4)} ${'runs'.padStart(6)}   R/G/B mean`);
  console.log('-'.repeat(78));
  for (const [name, box] of all) console.log(report(name, a, b, box, threshold));
  return 0;
}

if (process.argv[1] && resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('diff-report');
  cli.command('<a> <b> [...regions]', 'break the difference between two PNGs down by region')
      .option('--regions <file>', 'a file of name:x,y,w,h lines')
      .option('--threshold <n>', 'the delta below which a difference is not visible', {default: 8})
      .action((a: string, b: string, specs: string[], options: {regions?: string; threshold: number}) => {
        process.exitCode = diffReport(resolve(a), resolve(b), loadRegions(options.regions ? resolve(options.regions) : undefined, specs), Number(options.threshold));
      });
  cli.help();
  cli.parse();
}
