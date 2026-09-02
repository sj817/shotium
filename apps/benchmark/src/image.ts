import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import pixelmatch from 'pixelmatch';
import {PNG} from 'pngjs';

export const PERCEPTUAL_DIFFERENCE_THRESHOLD = 0.1;

export function inspectPng(buffer: Buffer | Uint8Array, expected: {width?: number; height?: number} = {}) {
  const bytes = Buffer.from(buffer);
  const png = PNG.sync.read(bytes);
  if (expected.width && png.width !== expected.width) {
    throw new Error(`PNG width ${png.width}, expected ${expected.width}`);
  }
  if (expected.height && png.height !== expected.height) {
    throw new Error(`PNG height ${png.height}, expected ${expected.height}`);
  }
  let uniform = true;
  for (let offset = 4; offset < png.data.length && uniform; offset += 4) {
    for (let channel = 0; channel < 4; channel += 1) {
      if (png.data[offset + channel] !== png.data[channel]) {
        uniform = false;
        break;
      }
    }
  }
  if (!bytes.length || uniform) {
    throw new Error('PNG appears empty or uniform');
  }
  return {
    width: png.width,
    height: png.height,
    bytes: bytes.length,
    sha256: crypto.createHash('sha256').update(bytes).digest('hex'),
  };
}

export function comparePng(leftBuffer: Buffer, rightBuffer: Buffer) {
  const left = PNG.sync.read(leftBuffer);
  const right = PNG.sync.read(rightBuffer);
  if (left.width !== right.width || left.height !== right.height) {
    return {
      comparable: false,
      differing_pixels: null,
      ratio: null,
      pixelmatch_differing_pixels: null,
      pixelmatch_ratio: null,
      pixelmatch_threshold: PERCEPTUAL_DIFFERENCE_THRESHOLD,
    };
  }
  const pixelmatchDiffering = pixelmatch(
      left.data, right.data, undefined, left.width, left.height,
      {threshold: PERCEPTUAL_DIFFERENCE_THRESHOLD, includeAA: true, alpha: 1});
  let differing = 0;
  for (let offset = 0; offset < left.data.length; offset += 4) {
    if (left.data[offset] !== right.data[offset] ||
        left.data[offset + 1] !== right.data[offset + 1] ||
        left.data[offset + 2] !== right.data[offset + 2] ||
        left.data[offset + 3] !== right.data[offset + 3]) {
      differing += 1;
    }
  }
  return {
    comparable: true,
    differing_pixels: differing,
    ratio: differing / (left.width * left.height),
    pixelmatch_differing_pixels: pixelmatchDiffering,
    pixelmatch_ratio: pixelmatchDiffering / (left.width * left.height),
    pixelmatch_threshold: PERCEPTUAL_DIFFERENCE_THRESHOLD,
    comparison: 'exact-rgba-diagnostic-plus-pixelmatch-correctness',
  };
}

export function isDeterministicComparison(comparison) {
  return comparison.comparable === true && comparison.pixelmatch_differing_pixels === 0;
}

function safeName(value: string) {
  return value.replace(/[^a-zA-Z0-9_.-]+/g, '-');
}

export function saveSample(directory: string, engine: string, caseName: string, sampleId: string,
    buffer: Buffer | Uint8Array) {
  fs.mkdirSync(directory, {recursive: true});
  const file = path.join(directory,
      `${safeName(engine)}.${safeName(caseName)}.${safeName(sampleId)}.png`);
  fs.writeFileSync(file, buffer);
  return file;
}

export function saveBaseline(directory: string, engine: string, caseName: string,
    buffer: Buffer | Uint8Array) {
  fs.mkdirSync(directory, {recursive: true});
  const file = path.join(directory, `${safeName(engine)}.${safeName(caseName)}.png`);
  if (!fs.existsSync(file)) fs.writeFileSync(file, buffer);
  return file;
}
