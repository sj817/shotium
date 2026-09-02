import assert from 'node:assert/strict';
import test from 'node:test';
import {PNG} from 'pngjs';
import {comparePng, inspectPng, isDeterministicComparison} from '../src/image.ts';

function png(width: number, height: number, pixels: number[][]): Buffer {
  const image = new PNG({width, height});
  pixels.forEach((pixel, index) => {
    for (let channel = 0; channel < 4; channel += 1) {
      image.data[(index * 4) + channel] = pixel[channel];
    }
  });
  return PNG.sync.write(image);
}

test('rejects a uniform PNG even when its RGB channels differ', () => {
  const solidRed = png(2, 1, [[255, 0, 0, 255], [255, 0, 0, 255]]);
  assert.throws(() => inspectPng(solidRed, {width: 2, height: 1}), /uniform/);
});

test('determinism keeps exact RGBA diagnostics but ignores imperceptible rounding', () => {
  const left = png(2, 1, [[255, 0, 0, 255], [0, 0, 255, 255]]);
  const right = png(2, 1, [[254, 0, 0, 255], [0, 0, 255, 255]]);
  const comparison = comparePng(left, right);
  assert.equal(comparison.differing_pixels, 1);
  assert.equal(comparison.pixelmatch_differing_pixels, 0);
  assert.equal(comparison.pixelmatch_threshold, 0.1);
  assert.equal(isDeterministicComparison(comparison), true);
});

test('determinism still rejects a perceptually different frame', () => {
  const left = png(2, 1, [[255, 255, 255, 255], [0, 0, 255, 255]]);
  const right = png(2, 1, [[0, 0, 0, 255], [0, 0, 255, 255]]);
  const comparison = comparePng(left, right);
  assert.equal(comparison.differing_pixels, 1);
  assert.equal(comparison.pixelmatch_differing_pixels, 1);
  assert.equal(isDeterministicComparison(comparison), false);
});
