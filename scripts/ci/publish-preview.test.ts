import assert from 'node:assert/strict';
import test from 'node:test';

import {createPublishBatches, MAX_BATCH_SIZE, MAX_NON_MULTIPART_PACKAGE_SIZE, type PreviewPackage} from './publish-preview.ts';

function packages(...sizes: number[]): PreviewPackage[] {
  return sizes.map((packedSize, index) => ({name: `@pixel.js/package-${index}`, directory: '', packedSize}));
}

test('keeps packages that fit under the safe limit together', () => {
  assert.equal(createPublishBatches(packages(40, 40), MAX_BATCH_SIZE).length, 1);
});

test('splits an aggregate larger than the safe limit', () => {
  const batches = createPublishBatches(packages(40 * 1024 * 1024, 40 * 1024 * 1024, 40 * 1024 * 1024), MAX_BATCH_SIZE);
  assert.equal(batches.length, 2);
  assert.deepEqual(batches.map(batch => batch.totalSize).sort((a, b) => a - b), [40 * 1024 * 1024, 80 * 1024 * 1024]);
});

test('allows one package above the safe limit below multipart threshold', () => {
  const batches = createPublishBatches(packages(85 * 1024 * 1024));
  assert.equal(batches.length, 1);
  assert.equal(batches[0].totalSize, 85 * 1024 * 1024);
});

test('rejects packages that would require multipart upload', () => {
  assert.throws(
      () => createPublishBatches(packages(MAX_NON_MULTIPART_PACKAGE_SIZE)),
      /too large for pkg\.pr\.new non-multipart upload/);
});

test('uses stable first-fit decreasing batches regardless of input order', () => {
  const first = createPublishBatches(packages(35, 25, 20, 15), 50);
  const second = createPublishBatches(packages(15, 20, 35, 25), 50);
  assert.deepEqual(first.map(batch => batch.packages.map(pkg => pkg.packedSize)),
      second.map(batch => batch.packages.map(pkg => pkg.packedSize)));
});
