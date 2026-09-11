// Find, fetch and account for the engine artifacts, which are named after
// the fingerprint of the tree that built them rather than the commit.
//
// why: a release used to need six builds on the tagged commit, and a
// pull request could not test the package against a real engine at all,
// because artifacts were looked up by commit SHA. With content-addressed
// names -- engine-linux-amd64-<fingerprint>, ffi-evidence-…, build-dir-… --
// any run can ask "does the engine for this tree already exist" and get the
// bytes from whichever run made them. This file is the one place that asks.
//
// Trust is the whole question. The artifacts API lists every artifact in
// the repository including those a fork's pull request uploaded, and a fork
// can name an artifact anything it likes. So a record counts only when the
// run that made it ran from this repository's own code -- head repository
// equals repository -- or when it is this very run's own upload, which a
// fork PR needs to verify what it just built. On top of that the chosen run
// must be one of the workflows that produce engines, so a same-repository
// experiment cannot pass off a stray upload as a build.
//
//   pnpm ci:engine-artifacts status --fingerprint <id> [--targets all|a,b] [--force] --output --summary
//   pnpm ci:engine-artifacts find --name engine-linux-amd64-<id> [--output]
//   pnpm ci:engine-artifacts download --name <n> --dir <d> [--run-id <r>]
//   pnpm ci:engine-artifacts download-set --fingerprint <id> --engine-dir d1 --node-dir d2 --evidence-dir d3
//   pnpm ci:engine-artifacts marker|provenance --platform <label> --fingerprint <id> --out <file>
//   pnpm ci:engine-artifacts prune --name build-dir-linux-amd64 --keep 3
//   pnpm ci:engine-artifacts refresh-plan --fingerprint <id> --output

import {appendFileSync, existsSync, mkdirSync, readdirSync, renameSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import pRetry from 'p-retry';
import pc from 'picocolors';

import {type EngineOS, type Platform, platformByLabel, selectPlatforms} from '../lib/platforms.ts';
import {releaseLanguages} from '../lib/release-artifacts.ts';
import {resolve} from '../lib/repo.ts';

export interface ArtifactRecord {
  id: number;
  name: string;
  size_in_bytes: number;
  expired: boolean;
  created_at: string;
  expires_at?: string;
  workflow_run: {id: number; repository_id: number; head_repository_id: number; head_branch: string; head_sha: string} | null;
}

/** GitHub REST, relative to the repository: `actions/artifacts?name=x`. Injected so the tests can stub it. */
export type Api = (route: string, init?: {method?: 'GET' | 'DELETE'}) => Promise<unknown>;

export const RETENTION_DAYS = 90;

/** The workflows that produce engine artifacts. A trusted upload from any other file is still not an engine. */
export const PRODUCERS = new Set([
  '.github/workflows/engine-windows.yml', '.github/workflows/engine-linux.yml', '.github/workflows/engine-macos.yml',
  '.github/workflows/engine.yml', '.github/workflows/preview.yml', '.github/workflows/publish.yml', '.github/workflows/refresh.yml',
]);

export const names = (label: string, fingerprint: string) => ({
  engine: `engine-${label}-${fingerprint}`,
  evidence: `ffi-evidence-${label}-${fingerprint}`,
  buildDir: `build-dir-${label}`,
  marker: `build-dir-${label}-${fingerprint}`,
});

export function trusted(record: ArtifactRecord, currentRunId?: number): boolean {
  if (record.expired || !record.workflow_run) return false;
  if (currentRunId !== undefined && record.workflow_run.id === currentRunId) return true;
  return record.workflow_run.head_repository_id === record.workflow_run.repository_id;
}

const newestFirst = (a: ArtifactRecord, b: ArtifactRecord) => Date.parse(b.created_at) - Date.parse(a.created_at);

/** Every non-expired artifact with exactly this name, newest first. */
export async function listByName(api: Api, name: string): Promise<ArtifactRecord[]> {
  const records: ArtifactRecord[] = [];
  for (let page = 1; page <= 10; page++) {
    const result = await api(`actions/artifacts?name=${encodeURIComponent(name)}&per_page=100&page=${page}`) as {total_count: number; artifacts: ArtifactRecord[]};
    records.push(...result.artifacts);
    if (records.length >= result.total_count || result.artifacts.length === 0) break;
  }
  return records.filter((r) => !r.expired).sort(newestFirst);
}

async function producedByEngineWorkflow(api: Api, runId: number): Promise<boolean> {
  const run = await api(`actions/runs/${runId}`) as {path?: string};
  return typeof run.path === 'string' && PRODUCERS.has(run.path);
}

export interface EngineSet {
  runId: number;
  engine: ArtifactRecord;
  evidence: ArtifactRecord;
}

// An engine is usable only with the evidence from the same run: a
// `run_checks=false` dispatch uploads the engine alone, and pairing that
// with an older run's evidence would certify bytes nobody checked.
export async function findEngineSet(api: Api, fingerprint: string, label: string, currentRunId?: number): Promise<EngineSet | null> {
  const {engine, evidence} = names(label, fingerprint);
  const engines = (await listByName(api, engine)).filter((r) => trusted(r, currentRunId));
  const evidences = (await listByName(api, evidence)).filter((r) => trusted(r, currentRunId));
  for (const record of engines) {
    const runId = record.workflow_run!.id;
    const proof = evidences.find((e) => e.workflow_run!.id === runId);
    if (!proof) continue;
    if (!(await producedByEngineWorkflow(api, runId))) continue;
    return {runId, engine: record, evidence: proof};
  }
  return null;
}

export interface BuildDir {
  runId: number;
  /** The blob was saved from a tree with this very fingerprint: nothing to compile, one runner. */
  exact: boolean;
  blob: ArtifactRecord;
}

// The build directory is a warm start, not a product: the newest trusted
// one is right even when it came from another branch, because ninja
// rebuilds whatever differs. The marker beside it says which fingerprint it
// was saved at, which is what decides between one shard and several.
export async function findBuildDir(api: Api, label: string, fingerprint: string | undefined, currentRunId?: number): Promise<BuildDir | null> {
  const {buildDir, marker} = names(label, fingerprint ?? '');
  const blobs = (await listByName(api, buildDir)).filter((r) => trusted(r, currentRunId));
  if (blobs.length === 0) return null;
  const markers = fingerprint ? (await listByName(api, marker)).filter((r) => trusted(r, currentRunId)) : [];
  const exact = blobs.find((b) => markers.some((m) => m.workflow_run!.id === b.workflow_run!.id));
  for (const blob of exact ? [exact, ...blobs.filter((b) => b !== exact)] : blobs) {
    if (!(await producedByEngineWorkflow(api, blob.workflow_run!.id))) continue;
    return {runId: blob.workflow_run!.id, exact: blob === exact, blob};
  }
  return null;
}

export interface Status {
  fingerprint: string;
  platforms: Array<{label: string; runId: number | null}>;
  /** Public architectures still to build, per OS: the matrices engine.yml fans out. */
  build: Record<EngineOS, string[]>;
  missing: string[];
  complete: boolean;
}

export async function status(api: Api, fingerprint: string, targets: Platform[], force: boolean, currentRunId?: number): Promise<Status> {
  const platforms: Status['platforms'] = [];
  const build: Status['build'] = {windows: [], linux: [], macos: []};
  for (const target of targets) {
    const set = force ? null : await findEngineSet(api, fingerprint, target.label, currentRunId);
    platforms.push({label: target.label, runId: set?.runId ?? null});
    if (!set) build[target.os].push(target.arch);
  }
  const missing = platforms.filter((p) => p.runId === null).map((p) => p.label);
  return {fingerprint, platforms, build, missing, complete: missing.length === 0};
}

export function statusOutputs(result: Status): string {
  const lines = [`fingerprint=${result.fingerprint}`];
  for (const os of ['windows', 'linux', 'macos'] as const) {
    lines.push(`${os}=${JSON.stringify(result.build[os])}`, `${os}_count=${result.build[os].length}`);
  }
  lines.push(`missing=${result.missing.join(',')}`, `complete=${result.complete}`);
  return lines.join('\n') + '\n';
}

function statusSummary(result: Status, repo: string): string {
  const rows = result.platforms.map((p) => `| ${p.label} | ${p.runId === null ? 'to build' : `[run ${p.runId}](https://github.com/${repo}/actions/runs/${p.runId})`} |`);
  return `### engine ${result.fingerprint}\n\n| platform | engine + evidence |\n| --- | --- |\n${rows.join('\n')}\n\n`;
}

// --- the command line -------------------------------------------------------

/** The real thing: GitHub REST for one repository, with retries on 5xx and 429 only. */
export function githubApi(repo: string, token: string): Api {
  return (route, init) => pRetry(async () => {
    const response = await fetch(`https://api.github.com/repos/${repo}/${route}`, {
      method: init?.method ?? 'GET',
      headers: {accept: 'application/vnd.github+json', authorization: `Bearer ${token}`, 'x-github-api-version': '2022-11-28'},
      signal: AbortSignal.timeout(30_000),
    });
    if (!response.ok) {
      const error = new Error(`GitHub API ${response.status} ${response.statusText} for ${route}`);
      if (response.status < 500 && response.status !== 429) throw Object.assign(error, {shouldRetry: false});
      throw error;
    }
    return response.status === 204 ? null : response.json();
  }, {retries: 4, shouldRetry: ({error}) => (error as {shouldRetry?: boolean}).shouldRetry !== false});
}

/** Repository, token and run id the way Actions provides them. */
export function environment(): {repo: string; api: Api; currentRunId: number | undefined} {
  const repo = process.env.GITHUB_REPOSITORY ?? 'sj817/shotium';
  const token = process.env.GITHUB_TOKEN ?? process.env.GH_TOKEN ?? '';
  const currentRunId = process.env.GITHUB_RUN_ID ? Number(process.env.GITHUB_RUN_ID) : undefined;
  if (!token) throw new Error('GITHUB_TOKEN (or GH_TOKEN) is not set');
  return {repo, api: githubApi(repo, token), currentRunId};
}

function output(text: string): void {
  if (!process.env.GITHUB_OUTPUT) throw new Error('GITHUB_OUTPUT is not set');
  appendFileSync(process.env.GITHUB_OUTPUT, text);
}

async function download(repo: string, runId: number, name: string, dir: string): Promise<void> {
  mkdirSync(dir, {recursive: true});
  await execa('gh', ['run', 'download', String(runId), '-R', repo, '-n', name, '-D', dir], {stdio: ['ignore', 'inherit', 'inherit']});
}

function fail(message: string): never {
  throw new Error(message);
}

const cli = cac('pnpm ci:engine-artifacts');

cli.command('status', 'which platforms already have an engine and evidence at this fingerprint')
  .option('--fingerprint <id>', 'the engine fingerprint (pnpm ci:fingerprint)')
  .option('--targets <list>', 'all, or comma-separated platform labels', {default: 'all'})
  .option('--force', 'report every target as missing, so everything is rebuilt')
  .option('--require-complete', 'exit 1 unless every target is present')
  .option('--output', 'write the per-OS build matrices to $GITHUB_OUTPUT')
  .option('--summary', 'append a table to $GITHUB_STEP_SUMMARY')
  .action(async (options: {fingerprint?: string; targets: string; force?: boolean; requireComplete?: boolean; output?: boolean; summary?: boolean}) => {
    const {repo, api, currentRunId} = environment();
    const fingerprint = options.fingerprint ?? fail('--fingerprint is required');
    const result = await status(api, fingerprint, selectPlatforms(options.targets), options.force ?? false, currentRunId);
    for (const p of result.platforms) console.log(`  ${p.label.padEnd(14)} ${p.runId === null ? pc.yellow('to build') : pc.green(`run ${p.runId}`)}`);
    if (options.output) output(statusOutputs(result));
    if (options.summary && process.env.GITHUB_STEP_SUMMARY) appendFileSync(process.env.GITHUB_STEP_SUMMARY, statusSummary(result, repo));
    if (options.requireComplete && !result.complete) fail(`no engine at ${fingerprint} for ${result.missing.join(', ')}`);
  });

cli.command('find', 'the newest trusted artifact with this exact name')
  .option('--name <name>', 'artifact name')
  .option('--output', 'write run_id= and artifact_id= to $GITHUB_OUTPUT')
  .action(async (options: {name?: string; output?: boolean}) => {
    const {api, currentRunId} = environment();
    const name = options.name ?? fail('--name is required');
    const record = (await listByName(api, name)).find((r) => trusted(r, currentRunId));
    if (!record) fail(`no trusted artifact named ${name}`);
    console.log(`${name}: run ${record.workflow_run!.id}, artifact ${record.id}, ${(record.size_in_bytes / 1e6).toFixed(1)} MB, expires ${record.expires_at ?? '?'}`);
    if (options.output) output(`run_id=${record.workflow_run!.id}\nartifact_id=${record.id}\n`);
  });

cli.command('download', 'download the newest trusted artifact with this name')
  .option('--name <name>', 'artifact name')
  .option('--dir <dir>', 'destination, relative to the repository root')
  .option('--run-id <id>', 'take it from this run instead of the newest')
  .action(async (options: {name?: string; dir?: string; runId?: string}) => {
    const {repo, api, currentRunId} = environment();
    const name = options.name ?? fail('--name is required');
    const dir = resolve(options.dir ?? fail('--dir is required'));
    let runId = options.runId ? Number(options.runId) : undefined;
    if (runId === undefined) {
      const record = (await listByName(api, name)).find((r) => trusted(r, currentRunId)) ?? fail(`no trusted artifact named ${name}`);
      runId = record.workflow_run!.id;
    }
    await download(repo, runId, name, dir);
  });

// Twelve downloads laid out the way publish.yml and check-ffi expect them:
// the CLI and C ABI archives per platform for the Release, the node archive
// and provenance per platform for the npm packages, and the evidence.
cli.command('download-set', 'fetch engine and evidence artifacts for all six platforms at a fingerprint')
  .option('--fingerprint <id>', 'the engine fingerprint')
  .option('--targets <list>', 'all, or comma-separated platform labels', {default: 'all'})
  .option('--engine-dir <dir>', 'CLI + C ABI archives go to <dir>/<label>/')
  .option('--node-dir <dir>', 'node archive + provenance.json go to <dir>/<label>/')
  .option('--evidence-dir <dir>', 'ffi-check/ and delivery-check/ go to <dir>/<label>/')
  .action(async (options: {fingerprint?: string; targets: string; engineDir?: string; nodeDir?: string; evidenceDir?: string}) => {
    const {repo, api, currentRunId} = environment();
    const fingerprint = options.fingerprint ?? fail('--fingerprint is required');
    const engineDir = resolve(options.engineDir ?? fail('--engine-dir is required'));
    const nodeDir = resolve(options.nodeDir ?? fail('--node-dir is required'));
    const evidenceDir = resolve(options.evidenceDir ?? fail('--evidence-dir is required'));
    for (const platform of selectPlatforms(options.targets)) {
      const set = await findEngineSet(api, fingerprint, platform.label, currentRunId) ?? fail(`no engine at ${fingerprint} for ${platform.label}`);
      const engine = path.join(engineDir, platform.label);
      const node = path.join(nodeDir, platform.label);
      const evidence = path.join(evidenceDir, platform.label);
      await download(repo, set.runId, set.engine.name, engine);
      await download(repo, set.runId, set.evidence.name, evidence);
      mkdirSync(node, {recursive: true});
      for (const file of [`shotium-node-${platform.label}.7z`, 'provenance.json']) {
        if (!existsSync(path.join(engine, file))) fail(`${set.engine.name} lacks ${file}`);
        renameSync(path.join(engine, file), path.join(node, file));
      }
      const left = readdirSync(engine).sort();
      const expected = [`shotium-c-abi-${platform.label}.7z`, `shotium-cli-${platform.label}.7z`];
      if (left.join('\n') !== expected.join('\n')) fail(`${set.engine.name} holds ${left.join(', ')}; expected ${expected.join(', ')}`);
      for (const report of [...releaseLanguages.map((l) => `ffi-check/${l}/report.json`), 'delivery-check/report.json']) {
        if (!existsSync(path.join(evidence, report))) fail(`${set.evidence.name} lacks ${report}`);
      }
      console.log(`${pc.cyan(platform.label.padEnd(14))} run ${set.runId}: engine, node, evidence`);
    }
  });

const stamp = (platform: string, fingerprint: string) => ({
  fingerprint, platform,
  run_id: Number(process.env.GITHUB_RUN_ID ?? 0), run_attempt: Number(process.env.GITHUB_RUN_ATTEMPT ?? 0),
  source_sha: process.env.GITHUB_SHA ?? '', ref: process.env.GITHUB_REF ?? '',
});

cli.command('marker', 'write the build-dir marker: which fingerprint a saved build directory belongs to')
  .option('--platform <label>', 'platform label')
  .option('--fingerprint <id>', 'the engine fingerprint')
  .option('--out <file>', 'where to write the JSON')
  .action((options: {platform?: string; fingerprint?: string; out?: string}) => {
    const platform = platformByLabel(options.platform ?? fail('--platform is required'));
    const out = resolve(options.out ?? fail('--out is required'));
    mkdirSync(path.dirname(out), {recursive: true});
    writeFileSync(out, JSON.stringify(stamp(platform.label, options.fingerprint ?? fail('--fingerprint is required')), null, 2) + '\n');
  });

// What the artifact cannot say for itself: the fingerprint is the tree, but
// the compiler and SDK versions come from the runner image, which the
// fingerprint does not see. Recorded so a release can say what built it.
cli.command('provenance', 'write provenance.json for an engine artifact')
  .option('--platform <label>', 'platform label')
  .option('--fingerprint <id>', 'the engine fingerprint')
  .option('--out <file>', 'where to write the JSON')
  .option('--toolchain <kv>', 'key=value, repeatable: what the image supplied', {type: [String]})
  .action((options: {platform?: string; fingerprint?: string; out?: string; toolchain?: string[]}) => {
    const platform = platformByLabel(options.platform ?? fail('--platform is required'));
    const out = resolve(options.out ?? fail('--out is required'));
    // cac hands an absent array option over as the string "undefined";
    // only key=value pairs count.
    const toolchain = Object.fromEntries((options.toolchain ?? [])
      .filter((kv): kv is string => typeof kv === 'string' && kv.includes('='))
      .map((kv) => [kv.slice(0, kv.indexOf('=')), kv.slice(kv.indexOf('=') + 1)]));
    mkdirSync(path.dirname(out), {recursive: true});
    writeFileSync(out, JSON.stringify({
      ...stamp(platform.label, options.fingerprint ?? fail('--fingerprint is required')),
      runner: {os: process.env.RUNNER_OS ?? '', arch: process.env.RUNNER_ARCH ?? '', image: process.env.ImageOS ?? '', image_version: process.env.ImageVersion ?? ''},
      toolchain,
    }, null, 2) + '\n');
  });

cli.command('prune', 'delete all but the newest N trusted artifacts with this name')
  .option('--name <name>', 'artifact name')
  .option('--keep <n>', 'how many to keep', {default: '3'})
  .action(async (options: {name?: string; keep: string}) => {
    const {api} = environment();
    const name = options.name ?? fail('--name is required');
    const keep = Number(options.keep);
    if (!Number.isInteger(keep) || keep < 1) fail('--keep must be a positive integer');
    const records = (await listByName(api, name)).filter((r) => trusted(r));
    for (const record of records.slice(keep)) {
      await api(`actions/artifacts/${record.id}`, {method: 'DELETE'});
      console.log(`deleted ${name} from run ${record.workflow_run!.id} (${record.created_at})`);
    }
    console.log(`${name}: kept ${Math.min(keep, records.length)} of ${records.length}`);
  });

// What refresh.yml re-uploads so nothing expires: engine and evidence at
// the fingerprint, and each platform's newest build directory with the
// marker its run wrote, because the marker only means something when it
// shares a run with the blob.
cli.command('refresh-plan', 'list the artifacts a refresh run should download and re-upload')
  .option('--fingerprint <id>', 'the engine fingerprint of the default branch')
  .option('--output', 'write plan=<json> to $GITHUB_OUTPUT')
  .action(async (options: {fingerprint?: string; output?: boolean}) => {
    const {api, currentRunId} = environment();
    const fingerprint = options.fingerprint ?? fail('--fingerprint is required');
    const plan: Array<{name: string; run_id: number}> = [];
    for (const platform of selectPlatforms('all')) {
      const set = await findEngineSet(api, fingerprint, platform.label, currentRunId);
      if (set) plan.push({name: set.engine.name, run_id: set.runId}, {name: set.evidence.name, run_id: set.runId});
      else console.log(pc.yellow(`${platform.label}: no engine at ${fingerprint}; nothing to refresh`));
      const dir = await findBuildDir(api, platform.label, fingerprint, currentRunId);
      if (!dir) continue;
      plan.push({name: dir.blob.name, run_id: dir.runId});
      const {artifacts} = await api(`actions/runs/${dir.runId}/artifacts?per_page=100`) as {artifacts: ArtifactRecord[]};
      const prefix = `${names(platform.label, '').marker}`;
      for (const marker of artifacts.filter((a) => !a.expired && a.name.startsWith(prefix))) plan.push({name: marker.name, run_id: dir.runId});
    }
    for (const item of plan) console.log(`  ${item.name} from run ${item.run_id}`);
    if (options.output) output(`plan=${JSON.stringify(plan)}\n`);
  });

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  cli.help();
  cli.parse(process.argv, {run: false});
  if (!cli.matchedCommand && !cli.options.help) {
    console.error('usage: pnpm ci:engine-artifacts <status|find|download|download-set|marker|provenance|prune|refresh-plan>');
    process.exitCode = 2;
  } else {
    // marker and provenance are synchronous and return nothing to chain on;
    // await handles both them and the promises the API commands return.
    try {
      await cli.runMatchedCommand();
    } catch (error) {
      console.error(pc.red(error instanceof Error ? error.message : String(error)));
      process.exitCode = 1;
    }
  }
}
