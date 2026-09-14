// Publish the nine npm packages of a pull request to pkg.pr.new.
//
// why: pkg.pr.new takes one `publish` per workflow run. The GitHub App
// registers a key for every run attempt when the run is requested, the CLI
// looks the key up before uploading, and the server deletes it as soon as
// one publish has gone through (publish.post.ts, `workflowsBucket.removeItem`).
// A second publish from the same run answers "There is no workflow defined".
// A publish is also one HTTP request of at most ~99 MiB: above that the CLI
// switches to multipart uploads, which the service only accepts from
// repositories on its whitelist. The eight platform packages together are
// 130 MiB, so no single run can publish them all.
//
// So the overflow is published by other runs. `pack` measures the packages
// and cuts them into batches under MAX_BATCH_SIZE; `publish`, in the pull
// request's own run, dispatches preview.yml once per overflow batch, waits,
// and then makes this run's one publish: the main package with the first
// batch, its optionalDependencies for the other batches rewritten to the URLs
// the child runs reported. `child`, in a dispatched run, publishes the
// tarballs of one batch from the artifact the parent uploaded and reports the
// URLs back as an artifact of its own. When everything fits in one batch
// there are no children and this is the original single publish.
//
// The comment on the pull request is written here too, not by pkg-pr-new:
// its comment lists the packages of one publish, which would be the main
// package and batch 0. Every publish runs with --comment=off and `publish`
// keeps one comment, found again by a marker, that names all nine.
//
//   pnpm ci:publish-preview pack --sha <head sha>            # dist/preview/packed
//   pnpm ci:publish-preview publish --branch <head branch> --pr <number>
//   pnpm ci:publish-preview child --batch <n> --incoming <dir>
//
// The tarballs carry the version pkg-pr-new's --previewVersion gives the main
// package, 0.0.0-preview-<sha7>, because a prebuilt tarball is published as
// it is: the version is written before `npm pack`, not by the CLI.

import {appendFileSync} from 'node:fs';
import {mkdir, readFile, readdir, rm, writeFile} from 'node:fs/promises';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import pc from 'picocolors';

import {root, sleep} from '../lib/repo.ts';

// The CLI uploads anything up to 99 MiB in one request; the main package,
// under a megabyte, rides with the first batch, and the rest is margin.
export const MAX_BATCH_SIZE = 80 * 1024 * 1024;
// A package at or above this cannot be published without whitelist access.
export const MAX_NON_MULTIPART_PACKAGE_SIZE = 98 * 1024 * 1024;
export const WORKFLOW = 'preview.yml';
const MAIN_PACKAGE = '@pixel.js/shotium';
const PREVIEW_URL_PREFIX = 'https://pkg.pr.new/';
/** Marks the comment `publish` maintains, so the next push edits it instead of adding another. */
export const COMMENT_MARKER = '<!-- shotium preview -->';
const PLATFORM_PREFIX = `${MAIN_PACKAGE}-`;
const NPM_DIR = path.join(root, 'dist', 'npm');
const PREVIEW_DIR = path.join(root, 'dist', 'preview');
const PACKED_DIR = path.join(PREVIEW_DIR, 'packed');
const BATCHES_FILE = 'batches.json';
const MAIN_PACKAGE_DIR = path.join(root, 'apps', 'typescript');
const MAIN_PACKAGE_JSON = path.join(MAIN_PACKAGE_DIR, 'package.json');

type PackageJson = {
  name: string;
  version: string;
  optionalDependencies?: Record<string, string>;
  [key: string]: unknown;
};

export interface PreviewPackage {
  name: string;
  /** Directory under dist/npm. */
  directory: string;
  /** File name under dist/preview/packed, once packed. */
  tarball: string;
  packedSize: number;
}

export interface PublishBatch {
  packages: PreviewPackage[];
  totalSize: number;
}

/** dist/preview/packed/batches.json: what `pack` measured, for `publish` and the children. */
export interface PackedPreview {
  sha: string;
  version: string;
  packages: PreviewPackage[];
  /** Package names per batch; batch 0 is the parent's. */
  batches: string[][];
}

export interface PreviewMetadata {
  packages: Array<{name: string; url: string; shasum: string}>;
  templates?: unknown[];
}

export type PreviewUrls = Map<string, {url: string; shasum: string}>;

/** What pkg-pr-new --previewVersion writes: the first seven characters of the commit. */
export function previewVersion(sha: string): string {
  if (!/^[0-9a-f]{40}$/.test(sha)) throw new Error(`not a commit sha: ${sha}`);
  return `0.0.0-preview-${sha.slice(0, 7)}`;
}

/** The run-name preview.yml gives a dispatched batch run; `publish` finds its children by it. */
export function childRunName(parentRunId: string, batch: number): string {
  return `preview-publish: run ${parentRunId} batch ${batch}`;
}

// First-fit decreasing, so the batches are the same for the same sizes
// whatever order the packages were found in.
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

/** One map from every publish's metadata, refusing gaps, strangers and duplicates. */
export function mergeMetadata(metadata: readonly PreviewMetadata[], expected: readonly string[]): PreviewUrls {
  const expectedNames = new Set(expected);
  const result: PreviewUrls = new Map();
  for (const entry of metadata.flatMap(item => item.packages ?? [])) {
    if (!expectedNames.has(entry.name)) throw new Error(`pkg-pr-new returned unexpected package ${entry.name}`);
    if (!entry.url?.startsWith(PREVIEW_URL_PREFIX) || !entry.shasum) {
      throw new Error(`pkg-pr-new metadata is missing URL or shasum for ${entry.name}`);
    }
    if (result.has(entry.name)) throw new Error(`duplicate preview metadata for ${entry.name}`);
    result.set(entry.name, {url: entry.url, shasum: entry.shasum});
  }
  for (const name of expected) if (!result.has(name)) throw new Error(`pkg-pr-new did not publish ${name}`);
  return result;
}

/**
 * The main package's optionalDependencies for one publish: the packages the
 * children published get their URLs; the ones in this publish keep their
 * version, which pkg-pr-new rewrites itself to the URL of the sibling it is
 * publishing alongside.
 */
export function rewriteOptionalDependencies(
    manifest: PackageJson, published: PreviewUrls, alongside: readonly string[]): Record<string, string> {
  const optionalDependencies = {...manifest.optionalDependencies};
  for (const name of Object.keys(optionalDependencies)) {
    if (!name.startsWith(PLATFORM_PREFIX)) continue;
    if (alongside.includes(name)) continue;
    const preview = published.get(name);
    if (!preview) throw new Error(`cannot rewrite optionalDependency ${name}: no child published it`);
    optionalDependencies[name] = preview.url;
  }
  return optionalDependencies;
}

/**
 * The URL of a package as pkg.pr.new's own pull request comment shows it:
 * the same form, compact or with owner and repository, at `@<pull request
 * number>`, which the server resolves to the latest publish of this pull
 * request.
 */
export function pullRequestUrl(url: string, pr: string): string {
  const at = url.lastIndexOf('@');
  if (at <= 0 || !/^[0-9a-f]{7,40}$/.test(url.slice(at + 1))) throw new Error(`not a pkg.pr.new commit URL: ${url}`);
  return `${url.slice(0, at)}@${pr}`;
}

/**
 * The pull request comment, laid out as pkg.pr.new lays out its own when a
 * publish has more than four packages: one collapsible block per package
 * with the install line, the main package first, and the commit last.
 */
export function pullRequestComment(
    repo: string, sha: string, pr: string, packages: ReadonlyArray<{name: string; url: string}>, runId: string): string {
  const main = packages.find(pkg => pkg.name === MAIN_PACKAGE);
  if (!main) throw new Error(`${MAIN_PACKAGE} is not among the published packages`);
  const blocks = [main, ...packages.filter(pkg => pkg !== main)].map(pkg => [
    `<details><summary><b>${pkg.name}</b></summary><p>`,
    '',
    '```',
    `npm i ${pullRequestUrl(pkg.url, pr)}`,
    '```',
    '',
    '</p></details>',
  ].join('\n'));
  return [
    COMMENT_MARKER,
    ...blocks,
    '',
    `_commit: <a href="https://github.com/${repo}/actions/runs/${runId}"><code>${sha.slice(0, 7)}</code></a>_`,
    '',
  ].join('\n');
}

function formatMiB(bytes: number): string {
  return `${(bytes / 1024 / 1024).toFixed(2)} MiB`;
}

function output(text: string): void {
  if (!process.env.GITHUB_OUTPUT) throw new Error('GITHUB_OUTPUT is not set');
  appendFileSync(process.env.GITHUB_OUTPUT, text);
}

function environment(): {repo: string; runId: string} {
  const repo = process.env.GITHUB_REPOSITORY ?? '';
  const runId = process.env.GITHUB_RUN_ID ?? '';
  if (!repo || !runId) throw new Error('GITHUB_REPOSITORY and GITHUB_RUN_ID are not set; this runs inside preview.yml');
  if (!process.env.GH_TOKEN && !process.env.GITHUB_TOKEN) throw new Error('GH_TOKEN is not set');
  return {repo, runId};
}

const gh = (args: string[]): Promise<{stdout: string}> => execa('gh', args, {stdio: ['ignore', 'pipe', 'inherit']});

async function readJson<T>(file: string): Promise<T> {
  return JSON.parse(await readFile(file, 'utf8')) as T;
}

async function readPacked(dir: string): Promise<PackedPreview> {
  return readJson<PackedPreview>(path.join(dir, BATCHES_FILE));
}

async function packageNamesFromMain(): Promise<Set<string>> {
  const manifest = await readJson<PackageJson>(MAIN_PACKAGE_JSON);
  return new Set(Object.keys(manifest.optionalDependencies ?? {}).filter(name => name.startsWith(PLATFORM_PREFIX)));
}

async function collectPlatformPackages(): Promise<PreviewPackage[]> {
  const expected = await packageNamesFromMain();
  const entries = await readdir(NPM_DIR, {withFileTypes: true});
  const packages: PreviewPackage[] = [];
  for (const entry of entries) {
    if (!entry.isDirectory()) continue;
    const manifest = await readJson<PackageJson>(path.join(NPM_DIR, entry.name, 'package.json'));
    if (manifest.name.startsWith(PLATFORM_PREFIX)) {
      if (packages.some(pkg => pkg.name === manifest.name)) throw new Error(`duplicate platform package name: ${manifest.name}`);
      packages.push({name: manifest.name, directory: entry.name, tarball: '', packedSize: 0});
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

async function packPackages(packages: PreviewPackage[], version: string): Promise<void> {
  for (const pkg of packages) {
    const directory = path.join(NPM_DIR, pkg.directory);
    const manifestPath = path.join(directory, 'package.json');
    const manifest = await readJson<PackageJson>(manifestPath);
    await writeFile(manifestPath, JSON.stringify({...manifest, version}, null, 2) + '\n');
    const result = await execa('npm', ['pack', '--json', '--pack-destination', PACKED_DIR], {cwd: directory});
    const packed = JSON.parse(result.stdout) as Array<{filename: string; size: number}>;
    if (packed.length !== 1 || !packed[0].filename || !Number.isSafeInteger(packed[0].size)) {
      throw new Error(`npm pack returned invalid metadata for ${pkg.name}`);
    }
    pkg.tarball = packed[0].filename;
    pkg.packedSize = packed[0].size;
  }
}

// The CLI reports the long URL form for every publish that has a Linux
// package in it: before choosing the compact form it validates each package's
// npm manifest with zod-package-json 2, whose `libc` is a string, and the
// Linux packages carry npm's `libc: ["glibc"]` array (scripts/package/
// platform.ts). The server has no such rule -- its query-registry predates
// the field -- and serves the compact form for all nine, so the comment
// checks for it itself (compactUrls) rather than trusting this report.
async function pkgPrNew(args: string[]): Promise<void> {
  await execa('pnpm', ['-C', 'scripts', 'exec', 'pkg-pr-new', 'publish',
    '--packageManager=npm', '--no-template', ...args], {cwd: root, stdio: 'inherit'});
}

/**
 * The compact URL, pkg.pr.new/<package>@<commit>, for every package the
 * server serves it for -- one HEAD each, at the commit just published -- and
 * the reported URL for any it does not, which is a package not yet on npm
 * with its repository field, where the server looks the owner and
 * repository up.
 */
async function compactUrls(packages: ReadonlyArray<{name: string; url: string}>, sha: string): Promise<Array<{name: string; url: string}>> {
  const result: Array<{name: string; url: string}> = [];
  for (const pkg of packages) {
    const compact = `${PREVIEW_URL_PREFIX}${pkg.name}@${sha.slice(0, 7)}`;
    let served = false;
    try {
      served = (await fetch(compact, {method: 'HEAD'})).ok;
    } catch (error) {
      console.log(pc.yellow(`  ${compact}: ${(error as Error).message}`));
    }
    if (!served) console.log(pc.yellow(`  ${pkg.name}: the compact URL is not served; showing ${pkg.url}`));
    result.push({name: pkg.name, url: served ? compact : pkg.url});
  }
  return result;
}

async function pack(sha: string): Promise<void> {
  const version = previewVersion(sha);
  await rm(PREVIEW_DIR, {recursive: true, force: true});
  await mkdir(PACKED_DIR, {recursive: true});
  const packages = await collectPlatformPackages();
  await packPackages(packages, version);
  console.log(`Preview packages, ${version}:`);
  for (const pkg of packages) console.log(`  ${pkg.name.padEnd(42)} ${formatMiB(pkg.packedSize)}`);
  const batches = createPublishBatches(packages);
  console.log('\nPreview batches:');
  batches.forEach((batch, index) => {
    console.log(`\nBatch ${index}${index === 0 ? ' (this run, with the main package)' : ' (a dispatched run)'}`);
    batch.packages.forEach(pkg => console.log(`  ${pkg.name}`));
    console.log(`  total: ${formatMiB(batch.totalSize)}`);
  });
  const packed: PackedPreview = {sha, version, packages, batches: batches.map(batch => batch.packages.map(pkg => pkg.name))};
  await writeFile(path.join(PACKED_DIR, BATCHES_FILE), JSON.stringify(packed, null, 2) + '\n');
  if (process.env.GITHUB_OUTPUT) output(`children=${batches.length - 1}\n`);
}

// The dispatch API does not return the run it created. preview.yml names a
// dispatched run after its inputs, so the child is the newest run with that
// name created after the dispatch.
async function findChildRun(repo: string, branch: string, name: string, after: number): Promise<string> {
  for (let attempt = 0; attempt < 30; attempt++) {
    const {stdout} = await gh(['run', 'list', '-R', repo, '--workflow', WORKFLOW, '--branch', branch,
      '--event', 'workflow_dispatch', '--limit', '20', '--json', 'databaseId,displayTitle,createdAt']);
    const runs = JSON.parse(stdout) as {databaseId: number; displayTitle: string; createdAt: string}[];
    const mine = runs.filter(run => run.displayTitle === name && Date.parse(run.createdAt) >= after);
    if (mine.length > 0) return String(mine[0].databaseId);
    await sleep(4000);
  }
  throw new Error(`dispatched "${name}" on ${branch}, but no such run appeared in two minutes`);
}

async function waitForRun(repo: string, id: string, deadline: number): Promise<void> {
  const url = `https://github.com/${repo}/actions/runs/${id}`;
  for (;;) {
    const {stdout} = await gh(['api', `repos/${repo}/actions/runs/${id}`, '--jq', '"\\(.status)\\t\\(.conclusion)"']);
    const [status, conclusion] = stdout.trim().split('\t');
    if (status === 'completed') {
      if (conclusion === 'success') {
        console.log(pc.green(`  ${url} succeeded`));
        return;
      }
      throw new Error(`child run ${url} ended as ${conclusion}`);
    }
    if (Date.now() > deadline) throw new Error(`child run ${url} is still ${status}`);
    await sleep(15_000);
  }
}

async function publishChildren(packed: PackedPreview, branch: string): Promise<PreviewMetadata[]> {
  const overflow = packed.batches.slice(1);
  if (overflow.length === 0) return [];
  const {repo, runId} = environment();
  const at = Date.now() - 5000;   // the run's createdAt can precede our clock
  const names: string[] = [];
  for (let batch = 1; batch <= overflow.length; batch++) {
    await gh(['workflow', 'run', WORKFLOW, '-R', repo, '--ref', branch,
      '-f', `run=${runId}`, '-f', `batch=${batch}`, '-f', `sha=${packed.sha}`]);
    names.push(childRunName(runId, batch));
  }
  console.log(`\nDispatched ${overflow.length} batch run(s) on ${branch}:`);
  const ids: string[] = [];
  for (const name of names) {
    const id = await findChildRun(repo, branch, name, at);
    console.log(`  ${name}: https://github.com/${repo}/actions/runs/${id}`);
    ids.push(id);
  }
  const deadline = Date.now() + 20 * 60_000;
  await Promise.all(ids.map(id => waitForRun(repo, id, deadline)));
  const metadata: PreviewMetadata[] = [];
  for (let batch = 1; batch <= overflow.length; batch++) {
    const dir = path.join(PREVIEW_DIR, 'children', String(batch));
    await mkdir(dir, {recursive: true});
    await execa('gh', ['run', 'download', ids[batch - 1], '-R', repo, '-n', `preview-batch-${runId}-${batch}`, '-D', dir],
        {stdio: ['ignore', 'inherit', 'inherit']});
    const reported = await readJson<PreviewMetadata>(path.join(dir, `batch-${batch}.json`));
    mergeMetadata([reported], overflow[batch - 1]);   // exactly its batch, before anything is trusted
    metadata.push(reported);
  }
  return metadata;
}

async function commentOnPullRequest(repo: string, pr: string, body: string): Promise<void> {
  const {stdout} = await gh(['api', `repos/${repo}/issues/${pr}/comments`, '--paginate',
    '--jq', `.[] | select(.body | contains("${COMMENT_MARKER}")) | .id`]);
  const [existing] = stdout.trim().split('\n').filter(Boolean);
  const input = path.join(PREVIEW_DIR, 'comment.json');
  await writeFile(input, JSON.stringify({body}));
  if (existing) {
    await gh(['api', '-X', 'PATCH', `repos/${repo}/issues/comments/${existing}`, '--input', input]);
    console.log(`\nUpdated the preview comment on #${pr}`);
  } else {
    await gh(['api', '-X', 'POST', `repos/${repo}/issues/${pr}/comments`, '--input', input]);
    console.log(`\nCommented the preview on #${pr}`);
  }
}

async function publish(branch: string, pr: string): Promise<void> {
  const packed = await readPacked(PACKED_DIR);
  const children = await publishChildren(packed, branch);
  const childUrls = mergeMetadata(children, packed.batches.slice(1).flat());
  const alongside = packed.batches[0];
  const original = await readFile(MAIN_PACKAGE_JSON);
  const metadataPath = path.join(PREVIEW_DIR, 'main.json');
  try {
    const manifest = JSON.parse(original.toString()) as PackageJson;
    const optionalDependencies = rewriteOptionalDependencies(manifest, childUrls, alongside);
    await writeFile(MAIN_PACKAGE_JSON, JSON.stringify({...manifest, optionalDependencies}, null, 2) + '\n');
    console.log(`\nPublishing ${MAIN_PACKAGE} with batch 0 from this run:`);
    await pkgPrNew(['--comment=off', '--previewVersion', '--json', metadataPath, MAIN_PACKAGE_DIR,
      ...alongside.map(name => path.join(NPM_DIR, packed.packages.find(pkg => pkg.name === name)!.directory))]);
  } finally {
    await writeFile(MAIN_PACKAGE_JSON, original);
  }
  const mine = await readJson<PreviewMetadata>(metadataPath);
  const all = mergeMetadata([mine, ...children], [MAIN_PACKAGE, ...packed.packages.map(pkg => pkg.name)]);
  const combined = [MAIN_PACKAGE, ...packed.packages.map(pkg => pkg.name)].map(name => ({name, ...all.get(name)!}));
  await writeFile(path.join(root, 'dist', 'preview.json'), JSON.stringify({packages: combined}, null, 2) + '\n');
  console.log('\nPublished:');
  for (const entry of combined) console.log(`  ${entry.name.padEnd(42)} ${entry.url}`);
  const {repo, runId} = environment();
  await commentOnPullRequest(repo, pr, pullRequestComment(repo, packed.sha, pr, await compactUrls(combined, packed.sha), runId));
}

async function child(batch: number, incoming: string): Promise<void> {
  const packed = await readPacked(incoming);
  const names = packed.batches[batch];
  if (!names || batch < 1) throw new Error(`batch ${batch} does not exist; the parent packed ${packed.batches.length} batch(es)`);
  const tarballs = names.map(name => path.join(incoming, packed.packages.find(pkg => pkg.name === name)!.tarball));
  await mkdir(PREVIEW_DIR, {recursive: true});
  const metadataPath = path.join(PREVIEW_DIR, `batch-${batch}.json`);
  console.log(`Publishing batch ${batch} of ${packed.batches.length} for ${packed.sha}:`);
  for (const name of names) console.log(`  ${name}`);
  await pkgPrNew(['--comment=off', '--json', metadataPath, ...tarballs]);
  const reported = await readJson<PreviewMetadata>(metadataPath);
  mergeMetadata([reported], names);
  // The URL names the commit in full or abbreviated; either contains the
  // abbreviation. A different commit means this branch moved between the
  // parent's checkout and this run's.
  const elsewhere = reported.packages.find(entry => !entry.url.includes(`@${packed.sha.slice(0, 7)}`));
  if (elsewhere) throw new Error(`${elsewhere.name} was published for another commit: ${elsewhere.url}`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('pnpm ci:publish-preview');
  cli.command('pack', 'measure the platform packages in dist/npm and cut them into publish batches')
    .option('--sha <sha>', 'the pull request head commit; the packages get version 0.0.0-preview-<sha7>')
    .action(async (options: {sha?: string}) => {
      if (!options.sha) throw new Error('--sha is required');
      await pack(options.sha);
    });
  cli.command('publish', 'dispatch the overflow batches, wait, publish the main package with batch 0, comment')
    .option('--branch <name>', 'the pull request head branch, where the batch runs are dispatched')
    .option('--pr <number>', 'the pull request to comment on')
    .action(async (options: {branch?: string; pr?: string}) => {
      if (!options.branch || !options.pr) throw new Error('--branch and --pr are required');
      await publish(options.branch, options.pr);
    });
  cli.command('child', 'publish one batch of tarballs from the parent run\'s artifact')
    .option('--batch <n>', 'batch index, 1 or more')
    .option('--incoming <dir>', 'where the parent\'s packed artifact was downloaded', {default: 'dist/preview/incoming'})
    .action(async (options: {batch?: string; incoming: string}) => {
      if (!options.batch) throw new Error('--batch is required');
      await child(Number(options.batch), path.resolve(root, options.incoming));
    });
  cli.help();
  cli.parse();
}
