import {mkdir, mkdtemp, readFile, readdir, rm, writeFile} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
import os from 'node:os';
import path from 'node:path';

import {execa} from 'execa';

import {root} from '../lib/repo.ts';

export const MAX_BATCH_SIZE = 80 * 1024 * 1024;
// Leave a 1 MiB margin below pkg.pr.new's 99 MiB multipart boundary. A
// package at or above this limit cannot be published without whitelist access.
export const MAX_NON_MULTIPART_PACKAGE_SIZE = 98 * 1024 * 1024;
const MAIN_PACKAGE = '@pixel.js/shotium';
const PLATFORM_PREFIX = `${MAIN_PACKAGE}-`;
const PREVIEW_DIR = path.join(root, 'dist', 'preview');
const MAIN_PACKAGE_JSON = path.join(root, 'apps', 'typescript', 'package.json');

type PackageJson = {
  name: string;
  optionalDependencies?: Record<string, string>;
  [key: string]: unknown;
};

export interface PreviewPackage {
  name: string;
  directory: string;
  packedSize: number;
}

export interface PublishBatch {
  packages: PreviewPackage[];
  totalSize: number;
}

export interface PreviewMetadata {
  packages: Array<{name: string; url: string; shasum: string}>;
  templates?: unknown[];
}

export function createPublishBatches(packages: readonly PreviewPackage[], maxSize = MAX_BATCH_SIZE): PublishBatch[] {
  if (maxSize <= 0) throw new Error('maxSize must be positive');
  const sorted = [...packages].sort((a, b) => b.packedSize - a.packedSize || a.name.localeCompare(b.name));
  const batches: PublishBatch[] = [];
  for (const pkg of sorted) {
    if (pkg.packedSize >= MAX_NON_MULTIPART_PACKAGE_SIZE) {
      throw new Error(`${pkg.name} is too large for pkg.pr.new non-multipart upload. This package requires multipart upload / whitelist.`);
    }
    let batch = batches.find(candidate => candidate.totalSize + pkg.packedSize <= maxSize);
    if (!batch) {
      batch = {packages: [], totalSize: 0};
      batches.push(batch);
    }
    batch.packages.push(pkg);
    batch.totalSize += pkg.packedSize;
  }
  return batches;
}

function formatMiB(bytes: number): string {
  return `${(bytes / 1024 / 1024).toFixed(2)} MiB`;
}

async function packageNamesFromMain(): Promise<Set<string>> {
  const manifest = JSON.parse(await readFile(MAIN_PACKAGE_JSON, 'utf8')) as PackageJson;
  return new Set(Object.keys(manifest.optionalDependencies ?? {}).filter(name => name.startsWith(PLATFORM_PREFIX)));
}

async function collectPlatformPackages(): Promise<PreviewPackage[]> {
  const expected = await packageNamesFromMain();
  const entries = await readdir(path.join(root, 'dist', 'npm'), {withFileTypes: true});
  const packages: PreviewPackage[] = [];
  for (const entry of entries) {
    if (!entry.isDirectory()) continue;
    const directory = path.join(root, 'dist', 'npm', entry.name);
    const manifest = JSON.parse(await readFile(path.join(directory, 'package.json'), 'utf8')) as PackageJson;
    if (manifest.name.startsWith(PLATFORM_PREFIX)) {
      if (packages.some(pkg => pkg.name === manifest.name)) throw new Error(`duplicate platform package name: ${manifest.name}`);
      packages.push({name: manifest.name, directory, packedSize: 0});
    }
  }
  const actual = new Set(packages.map(pkg => pkg.name));
  const missing = [...expected].filter(name => !actual.has(name));
  const extra = [...actual].filter(name => !expected.has(name));
  if (missing.length || extra.length) {
    throw new Error(`platform package set mismatch; missing: ${missing.join(', ') || 'none'}; extra: ${extra.join(', ') || 'none'}`);
  }
  if (!packages.length) throw new Error('no platform packages found in dist/npm');
  return packages.sort((a, b) => a.name.localeCompare(b.name));
}

async function measurePackedPackages(packages: PreviewPackage[]): Promise<void> {
  const temporary = await mkdtemp(path.join(os.tmpdir(), 'shotium-preview-pack-'));
  try {
    for (const pkg of packages) {
      const result = await execa('npm', ['pack', '--json', '--pack-destination', temporary], {cwd: pkg.directory});
      const packed = JSON.parse(result.stdout) as Array<{filename: string; size: number}>;
      if (packed.length !== 1 || !packed[0].filename || !Number.isSafeInteger(packed[0].size)) {
        throw new Error(`npm pack returned invalid metadata for ${pkg.name}`);
      }
      pkg.packedSize = packed[0].size;
    }
  } finally {
    await rm(temporary, {recursive: true, force: true});
  }
}

async function publishBatch(batch: PublishBatch, index: number): Promise<PreviewMetadata> {
  const metadataPath = path.join(PREVIEW_DIR, `batch-${index}.json`);
  const args = [
    '-C', 'scripts', 'exec', 'pkg-pr-new', 'publish',
    '--comment=off', '--packageManager=npm', '--no-template', '--previewVersion',
    '--json', metadataPath,
    ...batch.packages.map(pkg => pkg.directory),
  ];
  await execa('pnpm', args, {cwd: root, stdio: 'inherit'});
  return JSON.parse(await readFile(metadataPath, 'utf8')) as PreviewMetadata;
}

function mergeMetadata(metadata: PreviewMetadata[], expected: readonly PreviewPackage[]): Map<string, {url: string; shasum: string}> {
  const expectedNames = new Set(expected.map(pkg => pkg.name));
  const result = new Map<string, {url: string; shasum: string}>();
  for (const entry of metadata.flatMap(item => item.packages ?? [])) {
    if (!expectedNames.has(entry.name)) throw new Error(`pkg-pr-new returned unexpected package ${entry.name}`);
    if (!entry.url || !entry.shasum) throw new Error(`pkg-pr-new metadata is missing URL or shasum for ${entry.name}`);
    if (result.has(entry.name)) throw new Error(`duplicate preview metadata for ${entry.name}`);
    result.set(entry.name, {url: entry.url, shasum: entry.shasum});
  }
  for (const pkg of expected) if (!result.has(pkg.name)) throw new Error(`pkg-pr-new did not publish ${pkg.name}`);
  return result;
}

async function publishMain(platformPreviews: Map<string, {url: string; shasum: string}>): Promise<PreviewMetadata> {
  const original = await readFile(MAIN_PACKAGE_JSON);
  try {
    const manifest = JSON.parse(original.toString()) as PackageJson;
    const optionalDependencies = {...manifest.optionalDependencies};
    for (const name of Object.keys(optionalDependencies)) {
      if (!name.startsWith(PLATFORM_PREFIX)) continue;
      const preview = platformPreviews.get(name);
      if (!preview) throw new Error(`cannot rewrite optionalDependency ${name}: preview is missing`);
      optionalDependencies[name] = preview.url;
    }
    if (Object.keys(optionalDependencies).some(name => name.startsWith(PLATFORM_PREFIX) && !optionalDependencies[name].startsWith('https://pkg.pr.new/'))) {
      throw new Error('optionalDependencies could not be completely rewritten to preview URLs');
    }
    await writeFile(MAIN_PACKAGE_JSON, JSON.stringify({...manifest, optionalDependencies}, null, 2) + '\n');
    const metadataPath = path.join(PREVIEW_DIR, 'main.json');
    await execa('pnpm', [
      '-C', 'scripts', 'exec', 'pkg-pr-new', 'publish',
      '--comment=update', '--packageManager=npm', '--no-template', '--previewVersion',
      '--json', metadataPath, path.join(root, 'apps', 'typescript'),
    ], {cwd: root, stdio: 'inherit'});
    return JSON.parse(await readFile(metadataPath, 'utf8')) as PreviewMetadata;
  } finally {
    await writeFile(MAIN_PACKAGE_JSON, original);
  }
}

async function main(): Promise<void> {
  await rm(PREVIEW_DIR, {recursive: true, force: true});
  await mkdir(PREVIEW_DIR, {recursive: true});
  const packages = await collectPlatformPackages();
  await measurePackedPackages(packages);
  console.log('Preview packages:');
  for (const pkg of packages) console.log(`  ${pkg.name.padEnd(42)} ${formatMiB(pkg.packedSize)}`);
  const batches = createPublishBatches(packages);
  console.log('\nPreview batches:');
  batches.forEach((batch, index) => {
    console.log(`\nBatch ${index + 1}`);
    batch.packages.forEach(pkg => console.log(`  ${pkg.name}`));
    console.log(`  total: ${formatMiB(batch.totalSize)}`);
  });
  const platformMetadata: PreviewMetadata[] = [];
  for (const [index, batch] of batches.entries()) platformMetadata.push(await publishBatch(batch, index));
  const platformPreviews = mergeMetadata(platformMetadata, packages);
  console.log('\nPublished platform previews:');
  for (const [name, preview] of platformPreviews) console.log(`  ${name} -> ${preview.url}`);
  console.log('\nRewriting main package optionalDependencies:');
  const mainMetadata = await publishMain(platformPreviews);
  const mainPackages = mainMetadata.packages ?? [];
  if (mainPackages.length !== 1 || mainPackages[0].name !== MAIN_PACKAGE || !mainPackages[0].url) {
    throw new Error(`pkg-pr-new did not return exactly one ${MAIN_PACKAGE} preview`);
  }
  const combined = [...packages.map(pkg => ({name: pkg.name, ...platformPreviews.get(pkg.name)!})), ...mainPackages];
  await writeFile(path.join(root, 'dist', 'preview.json'), JSON.stringify({packages: combined}, null, 2) + '\n');
  console.log(`\nPublished ${MAIN_PACKAGE}:\n${mainPackages[0].url}`);
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) await main();
