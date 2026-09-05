// Compare two PNGs and report how far apart they are.
//
// The acceptance criterion for this tree is not "the images match" -- a
// stripped renderer is allowed to differ -- it is that every difference is
// measured and explained. So this reports numbers that distinguish *kinds* of
// difference rather than a single pass/fail:
//
//   max channel delta      one antialiasing seam and a missing element both
//                          show 255 here, so this alone says very little
//   differing pixels       how much of the image moved at all
//   pixels over threshold  how much of it moved *visibly* (default 8/255,
//                          below which a difference is not perceptible)
//   mean delta             separates "everything shifted slightly", which is
//                          a colour-space or gamma difference, from "a few
//                          regions are completely wrong", a missing feature
//   bounding box           where the damage is; a box around one element
//                          points at that element's code path
//
// A difference image is written alongside, amplified so that small deltas are
// actually visible.
//
//   pnpm pixel-diff a.png b.png [--out diff.png] [--threshold 8]
//
// Relative paths are resolved against the repository root.

import {readFileSync, writeFileSync} from 'node:fs';

import {cac} from 'cac';
import {PNG} from 'pngjs';

import {crop, decodePng, type Image} from './lib/png.ts';
import {resolve} from './lib/repo.ts';

// Per-pixel maximum RGB delta, as a grayscale plane.
export function worstChannel(a: Image, b: Image): Uint8Array {
  const worst = new Uint8Array(a.width * a.height);
  for (let i = 0; i < worst.length; i++) {
    const o = i * 4;
    worst[i] = Math.max(Math.abs(a.data[o] - b.data[o]), Math.abs(a.data[o + 1] - b.data[o + 1]), Math.abs(a.data[o + 2] - b.data[o + 2]));
  }
  return worst;
}

export function histogram(plane: Uint8Array): number[] {
  const h = new Array<number>(256).fill(0);
  for (const v of plane) h[v]++;
  return h;
}

export function pixelDiff(aPath: string, bPath: string, outPath: string, threshold: number): void {
  let a = decodePng(readFileSync(aPath));
  let b = decodePng(readFileSync(bPath));
  console.log(`a: ${aPath}  ${a.width}x${a.height}`);
  console.log(`b: ${bPath}  ${b.width}x${b.height}`);
  if (a.width !== b.width || a.height !== b.height) {
    // Comparing after a resize would invent differences that are artifacts of
    // the resampling, so crop both to the overlap and say so.
    const w = Math.min(a.width, b.width), h = Math.min(a.height, b.height);
    console.log(`SIZE MISMATCH -- comparing the common ${w}x${h} region only`);
    a = crop(a, 0, 0, w, h);
    b = crop(b, 0, 0, w, h);
  }
  const total = a.width * a.height;
  const worst = worstChannel(a, b);
  const hist = histogram(worst);
  const differing = hist.slice(1).reduce((s, n) => s + n, 0);
  const visible = hist.slice(threshold).reduce((s, n) => s + n, 0);
  let maxDelta = 0;
  for (let i = 255; i >= 0; i--) if (hist[i]) { maxDelta = i; break; }
  const mean = hist.reduce((s, n, i) => s + i * n, 0) / total;

  console.log(`max channel delta       ${maxDelta}`);
  console.log(`differing pixels        ${differing} / ${total}  (${(100 * differing / total).toFixed(4)}%)`);
  console.log(`over threshold ${String(threshold).padEnd(8)} ${visible} / ${total}  (${(100 * visible / total).toFixed(4)}%)`);
  console.log(`mean delta              ${mean.toFixed(4)}`);

  let x0 = a.width, y0 = a.height, x1 = -1, y1 = -1;
  for (let i = 0; i < worst.length; i++) {
    if (worst[i] >= threshold) {
      const x = i % a.width, y = Math.floor(i / a.width);
      if (x < x0) x0 = x;
      if (y < y0) y0 = y;
      if (x > x1) x1 = x;
      if (y > y1) y1 = y;
    }
  }
  console.log(`bounding box            ${x1 >= 0 ? `(${x0}, ${y0}, ${x1 + 1}, ${y1 + 1})` : 'None'}`);

  // Amplify so a delta of 8 is actually visible in the written image.
  const png = new PNG({width: a.width, height: a.height});
  for (let i = 0; i < worst.length; i++) {
    const v = Math.min(255, worst[i] * 8);
    png.data[i * 4] = png.data[i * 4 + 1] = png.data[i * 4 + 2] = v;
    png.data[i * 4 + 3] = 255;
  }
  writeFileSync(outPath, PNG.sync.write(png, {colorType: 0}));
  console.log(`wrote                   ${outPath}`);
}

if (process.argv[1] && resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('pixel-diff');
  cli.command('<a> <b>', 'compare two PNGs and report how far apart they are')
      .option('--out <file>', 'where the amplified difference image goes', {default: 'diff.png'})
      .option('--threshold <n>', 'the delta below which a difference is not visible', {default: 8})
      .action((a: string, b: string, options: {out: string; threshold: number}) => {
        pixelDiff(resolve(a), resolve(b), resolve(options.out), Number(options.threshold));
      });
  cli.help();
  cli.parse();
}
