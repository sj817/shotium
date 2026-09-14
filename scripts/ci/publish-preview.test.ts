import assert from 'node:assert/strict';
import test from 'node:test';

import {
  childRunName, createPublishBatches, MAX_BATCH_SIZE, MAX_NON_MULTIPART_PACKAGE_SIZE, mergeMetadata, previewVersion,
  rewriteOptionalDependencies, type PreviewMetadata, type PreviewPackage,
} from './publish-preview.ts';

function packages(...sizes: number[]): PreviewPackage[] {
  return sizes.map((packedSize, index) => ({name: `@pixel.js/package-${index}`, directory: '', tarball: '', packedSize}));
}

const SHA = '297882ba02daa2ec314a3d0859f90be06797cec4';
const url = (name: string, sha = SHA) => `https://pkg.pr.new/sj817/shotium/${name}@${sha}`;
const published = (...names: string[]): PreviewMetadata =>
  ({packages: names.map(name => ({name, url: url(name), shasum: 'a'.repeat(40)}))});

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

// The eight packages of 2026-09-14: 130 MiB in two batches, the fuller one
// first, so the pull request's own run publishes it with the main package
// and one dispatched run publishes the rest.
test('the eight platform packages make two batches, batch 0 the fuller', () => {
  const mib = (n: number) => Math.round(n * 1024 * 1024);
  const batches = createPublishBatches(packages(mib(14.46), mib(15.78), mib(15.88), mib(15.81), mib(16.86), mib(16.79), mib(16.47), mib(18.65)));
  assert.equal(batches.length, 2);
  assert.ok(batches[0].totalSize >= batches[1].totalSize);
  assert.ok(batches.every(batch => batch.totalSize <= MAX_BATCH_SIZE));
  assert.equal(batches.flatMap(batch => batch.packages).length, 8);
});

test('the preview version is what pkg-pr-new --previewVersion writes', () => {
  assert.equal(previewVersion(SHA), '0.0.0-preview-297882b');
  assert.throws(() => previewVersion('297882b'), /not a commit sha/);
});

test('a child run is named after the run that dispatched it and its batch', () => {
  assert.equal(childRunName('34822865479', 1), 'preview-publish: run 34822865479 batch 1');
});

test('mergeMetadata joins the publishes and insists on the whole set', () => {
  const merged = mergeMetadata([published('@pixel.js/shotium-a'), published('@pixel.js/shotium-b')],
      ['@pixel.js/shotium-a', '@pixel.js/shotium-b']);
  assert.deepEqual([...merged.keys()], ['@pixel.js/shotium-a', '@pixel.js/shotium-b']);
  assert.equal(merged.get('@pixel.js/shotium-a')!.url, url('@pixel.js/shotium-a'));
  assert.throws(() => mergeMetadata([published('@pixel.js/shotium-a')], ['@pixel.js/shotium-a', '@pixel.js/shotium-b']),
      /did not publish @pixel\.js\/shotium-b/);
  assert.throws(() => mergeMetadata([published('@pixel.js/shotium-c')], ['@pixel.js/shotium-a']), /unexpected package/);
  assert.throws(() => mergeMetadata([published('@pixel.js/shotium-a'), published('@pixel.js/shotium-a')], ['@pixel.js/shotium-a']),
      /duplicate preview metadata/);
  assert.throws(() => mergeMetadata([{packages: [{name: '@pixel.js/shotium-a', url: 'https://example.com/x', shasum: 'a'}]}],
      ['@pixel.js/shotium-a']), /missing URL or shasum/);
});

test('optionalDependencies get the children\'s URLs and keep the siblings of this publish', () => {
  const manifest = {
    name: '@pixel.js/shotium', version: '0.8.0',
    optionalDependencies: {'@pixel.js/shotium-linux-x64': '0.8.0', '@pixel.js/shotium-win32-x64': '0.8.0', 'fsevents': '2.3.3'},
  };
  const children = mergeMetadata([published('@pixel.js/shotium-linux-x64')], ['@pixel.js/shotium-linux-x64']);
  assert.deepEqual(rewriteOptionalDependencies(manifest, children, ['@pixel.js/shotium-win32-x64']), {
    '@pixel.js/shotium-linux-x64': url('@pixel.js/shotium-linux-x64'),
    '@pixel.js/shotium-win32-x64': '0.8.0',
    'fsevents': '2.3.3',
  });
  assert.throws(() => rewriteOptionalDependencies(manifest, children, []), /no child published it/);
});
