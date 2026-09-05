// PNG helpers for the check suites, on pngjs.
//
// pngjs decodes every colour type to 8-bit RGBA, which is what the pixel
// comparisons want. What it does not report is what the file itself said, and
// two checks depend on that: whether the encoder kept an alpha channel is a
// question about the IHDR colour type, not about the decoded buffer. So the
// header is read directly, the way the Python suite did.

import {PNG} from 'pngjs';

export interface Image {
  width: number;
  height: number;
  data: Buffer;  // RGBA, 4 bytes per pixel
}

export type Rgb = [number, number, number];
export type Rgba = [number, number, number, number];

// (width, height) out of the IHDR, which is always the first chunk.
export function pngSize(png: Buffer): [number, number] {
  return [png.readUInt32BE(16), png.readUInt32BE(20)];
}

// Channels as the file declares them: 1 grey, 2 grey+alpha, 3 RGB, 4 RGBA.
export function pngChannels(png: Buffer): number {
  const colour = png[25];
  return ({0: 1, 2: 3, 4: 2, 6: 4} as Record<number, number>)[colour] ?? 0;
}

export function decodePng(png: Buffer): Image {
  const image = PNG.sync.read(png);
  return {width: image.width, height: image.height, data: image.data};
}

export function pixel(image: Image, x: number, y: number): Rgba {
  const i = (y * image.width + x) * 4;
  return [image.data[i], image.data[i + 1], image.data[i + 2], image.data[i + 3]];
}

export function rgb(image: Image, x: number, y: number): Rgb {
  const i = (y * image.width + x) * 4;
  return [image.data[i], image.data[i + 1], image.data[i + 2]];
}

export const sameRgb = (a: readonly number[], b: readonly number[]): boolean =>
    a[0] === b[0] && a[1] === b[1] && a[2] === b[2];

export function row(image: Image, y: number): Buffer {
  return image.data.subarray(y * image.width * 4, (y + 1) * image.width * 4);
}

export interface Difference {
  moved: number;   // pixels whose RGB differs at all
  worst: number;   // largest single-channel delta
  bbox: [number, number, number, number] | null;  // x0, y0, x1, y1 (exclusive)
  total: number;
}

export type Measure = {kind: 'size'; a: [number, number]; b: [number, number]} |
    ({kind: 'diff'} & Difference);

// The RGB difference between two images of the same size: how many pixels
// moved, by how much at worst, and where. Alpha is ignored, as Pillow's
// convert('RGB') ignored it.
export function measure(a: Image, b: Image): Measure {
  if (a.width !== b.width || a.height !== b.height) {
    return {kind: 'size', a: [a.width, a.height], b: [b.width, b.height]};
  }
  let moved = 0, worst = 0;
  let x0 = a.width, y0 = a.height, x1 = -1, y1 = -1;
  for (let y = 0; y < a.height; y++) {
    for (let x = 0; x < a.width; x++) {
      const i = (y * a.width + x) * 4;
      const d = Math.max(
          Math.abs(a.data[i] - b.data[i]), Math.abs(a.data[i + 1] - b.data[i + 1]),
          Math.abs(a.data[i + 2] - b.data[i + 2]));
      if (d > 0) {
        moved++;
        if (d > worst) worst = d;
        if (x < x0) x0 = x;
        if (y < y0) y0 = y;
        if (x > x1) x1 = x;
        if (y > y1) y1 = y;
      }
    }
  }
  return {kind: 'diff', moved, worst, bbox: moved ? [x0, y0, x1 + 1, y1 + 1] : null, total: a.width * a.height};
}

export function describeDifference(a: Image, b: Image): string {
  const m = measure(a, b);
  if (m.kind === 'size') return `different sizes: ${m.a[0]}x${m.a[1]} vs ${m.b[0]}x${m.b[1]}`;
  const pct = (100 * m.moved / m.total).toFixed(3);
  return `${m.moved} px differ (${pct}%), worst channel delta ${m.worst}, bbox ${m.bbox ? `(${m.bbox.join(', ')})` : 'None'}`;
}

// Number of distinct RGB colours, counting up to `cap`.
export function distinctColours(image: Image, cap = 64): number {
  const seen = new Set<number>();
  for (let i = 0; i < image.data.length; i += 4) {
    seen.add((image.data[i] << 16) | (image.data[i + 1] << 8) | image.data[i + 2]);
    if (seen.size >= cap) return cap;
  }
  return seen.size;
}

export function crop(image: Image, x: number, y: number, width: number, height: number): Image {
  const data = Buffer.alloc(width * height * 4);
  for (let r = 0; r < height; r++) {
    image.data.copy(data, r * width * 4, ((y + r) * image.width + x) * 4, ((y + r) * image.width + x + width) * 4);
  }
  return {width, height, data};
}

export function encodePng(image: Image): Buffer {
  const png = new PNG({width: image.width, height: image.height});
  image.data.copy(png.data);
  return PNG.sync.write(png);
}
