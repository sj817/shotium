// Select the warm build directory to start from and how many runners to
// compile on.
//
// why: the choice used to be "the newest saved directory, and the platform's
// full shard count unless the tree is identical". Newest is the wrong
// directory when several branches build in turn -- a pull request forked
// from main restored another pull request's objects -- and the full count
// is twenty runners for a one-line change. Both questions have the same
// answer: price each candidate directory by what the current tree would
// rebuild from it (lib/build-index.ts), start from the cheapest, and give
// it as many runners as that work is worth.
//
//   SHOT_TARGET=<label> SHOT_SHARDS=auto|N SHOT_FINGERPRINT=<id> pnpm ci:select-shards
//
// Runs in the resolve job on a blobless checkout: pricing a candidate
// fetches its commit's trees (no blobs) and its build-index artifact.

import {appendFileSync, mkdtempSync, readFileSync, rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import path from 'node:path';

import {execa} from 'execa';

import {type BuildIndex, estimate, parseIndex, shardsFor} from '../lib/build-index.ts';
import {platformByLabel} from '../lib/platforms.ts';
import {resolve} from '../lib/repo.ts';
import {environment, findBuildDirs, names} from './engine-artifacts.ts';
import {isEngineInput} from './fingerprint.ts';

export function shardCount(requested: string, platform: string, cpu: string): number {
  // The repository has 20 concurrent hosted-runner slots. A cold all-platform
  // run uses exactly that budget: Windows 3+2, Linux glibc 3+2, Linux musl
  // 3+2, and macOS 2+3. This lets all eight targets compile in the first wave
  // instead of allowing early matrix entries to occupy every slot.
  if (requested === 'auto') return platform === 'macos' ? (cpu === 'arm64' ? 3 : 2) : (cpu === 'arm64' ? 2 : 3);
  if (!/^[1-9]\d*$/.test(requested) || Number(requested) > 20) throw new Error('shards must be auto or an integer from 1 to 20');
  return Number(requested);
}

export interface Candidate {
  runId: number;
  /** Saved at this very fingerprint: nothing to compile. */
  exact: boolean;
  headSha: string;
  branch: string;
  createdAt: string;
}

export interface Selection {
  count: number;
  buildDirRunId: number|null;
  reason: string;
}

export interface Sources {
  /** Every usable saved directory for the platform, exact first, then newest first. */
  lookup: (label: string, fingerprint: string|undefined) => Promise<Candidate[]>;
  /** Engine-input paths that differ between the current tree and that commit; null when the commit cannot be reached. */
  changes: (headSha: string) => Promise<string[]|null>;
  /** The candidate run's build index, if it uploaded one. */
  index: (runId: number, label: string) => Promise<BuildIndex|null>;
}

interface Priced {
  candidate: Candidate;
  changed: number;
  /** Compile milliseconds the change dirties; null when it could not be priced. */
  ms: number|null;
  index: BuildIndex|null;
  unknown: string[];
}

const minutes = (ms: number) => (ms / 60_000).toFixed(1);

// Cheapest first among priced candidates; unpriced ones after, newest first.
// A priced candidate is comparable on two axes: work, then how many files
// differ, so two equal-cost directories prefer the closer tree.
function better(a: Priced, b: Priced): boolean {
  if (a.ms !== null && b.ms !== null) {
    if (a.ms !== b.ms) return a.ms < b.ms;
    if (a.changed !== b.changed) return a.changed < b.changed;
    return Date.parse(a.candidate.createdAt) > Date.parse(b.candidate.createdAt);
  }
  if (a.ms !== null || b.ms !== null) return a.ms !== null;
  return Date.parse(a.candidate.createdAt) > Date.parse(b.candidate.createdAt);
}

export async function select(options: {requested: string; target: string; fingerprint: string|undefined} & Sources): Promise<Selection> {
  const target = platformByLabel(options.target);
  const count = shardCount(options.requested, target.os, target.cpu);
  let candidates: Candidate[];
  try {
    candidates = await options.lookup(target.label, options.fingerprint);
  } catch (error) {
    return {count, buildDirRunId: null, reason: `could not look up a build directory; ${count} shard(s) from nothing: ${String(error)}`};
  }
  if (candidates.length === 0) return {count, buildDirRunId: null, reason: `no saved build directory for ${target.label}; ${count} shard(s) from nothing`};
  const exact = candidates.find((c) => c.exact);
  if (exact && options.requested === 'auto') {
    return {count: 1, buildDirRunId: exact.runId, reason: `run ${exact.runId} saved a build directory at this fingerprint: one runner, nothing to compile`};
  }

  let best: Priced|null = null;
  const notes: string[] = [];
  for (const candidate of candidates) {
    const priced = await price(candidate, target.label, options);
    notes.push(`run ${candidate.runId} (${candidate.branch}): ${describe(priced)}`);
    if (best === null || better(priced, best)) best = priced;
  }
  const chosen = best!;
  const runId = chosen.candidate.runId;
  if (options.requested !== 'auto' || chosen.ms === null || chosen.index === null) {
    return {count, buildDirRunId: runId, reason: `warm start from run ${runId}; ${count} shard(s)\n  ${notes.join('\n  ')}`};
  }
  const priced = shardsFor(chosen.ms, chosen.index, count);
  return {
    count: priced, buildDirRunId: runId,
    reason: `warm start from run ${runId}: ${chosen.changed} engine input(s) differ, about ${minutes(chosen.ms)} of ${minutes(chosen.index.total_ms)} compile minutes to redo; ${priced} shard(s)\n  ${notes.join('\n  ')}`,
  };
}

async function price(candidate: Candidate, label: string, sources: Sources): Promise<Priced> {
  let changed: string[]|null = null;
  let index: BuildIndex|null = null;
  try {
    changed = await sources.changes(candidate.headSha);
  } catch (error) {
    return {candidate, changed: -1, ms: null, index: null, unknown: [`cannot diff against ${candidate.headSha}: ${String(error)}`]};
  }
  if (changed === null) return {candidate, changed: -1, ms: null, index: null, unknown: [`${candidate.headSha} is not reachable`]};
  try {
    index = await sources.index(candidate.runId, label);
  } catch (error) {
    return {candidate, changed: changed.length, ms: null, index: null, unknown: [`no build index: ${String(error)}`]};
  }
  if (!index) return {candidate, changed: changed.length, ms: null, index: null, unknown: ['no build index']};
  const priced = estimate(index, changed);
  return {candidate, changed: changed.length, ms: priced.ms, index, unknown: priced.unknown};
}

function describe(p: Priced): string {
  if (p.ms !== null && p.index) return `${p.changed} input(s) differ, ~${minutes(p.ms)} min to redo`;
  const why = p.unknown.length > 3 ? [...p.unknown.slice(0, 3), `and ${p.unknown.length - 3} more`] : p.unknown;
  return `${p.changed >= 0 ? `${p.changed} input(s) differ, ` : ''}not priced (${why.join('; ')})`;
}

export function outputs(selection: Selection): string {
  const {count, buildDirRunId} = selection;
  return `count=${count}\nfinal=${count - 1}\nlist=${JSON.stringify(Array.from({length: count - 1}, (_, i) => i))}\nbuild_dir_run_id=${buildDirRunId ?? ''}\n`;
}

// --- the real sources: git and the artifacts API ---------------------------

// Engine inputs that differ between HEAD and the commit a directory was
// saved from. `diff-tree` compares tree entries, so a blobless checkout
// needs only that commit's trees: fetch it shallow and without blobs. A
// commit GitHub no longer serves (its branch was force-pushed or deleted)
// is unreachable, not an error.
async function changedInputs(headSha: string): Promise<string[]|null> {
  const cwd = resolve();
  const fetched = await execa('git', ['--no-optional-locks', 'fetch', '--quiet', '--depth=1', '--filter=blob:none', 'origin', headSha], {cwd, reject: false});
  if (fetched.exitCode !== 0) return null;
  const {stdout} = await execa('git', ['--no-optional-locks', 'diff-tree', '-r', '-z', '--name-only', '--no-renames', 'HEAD', headSha], {cwd, stripFinalNewline: false});
  return stdout.split('\0').filter((file) => file && isEngineInput(file));
}

async function downloadIndex(runId: number, label: string): Promise<BuildIndex|null> {
  const {repo} = environment();
  const dir = mkdtempSync(path.join(tmpdir(), 'build-index-'));
  try {
    const name = names(label, '').index;
    const result = await execa('gh', ['run', 'download', String(runId), '-R', repo, '-n', name, '-D', dir], {reject: false});
    if (result.exitCode !== 0) return null;
    return parseIndex(readFileSync(path.join(dir, 'build-index.json'), 'utf8'));
  } finally {
    rmSync(dir, {recursive: true, force: true});
  }
}

async function main(): Promise<void> {
  const {GITHUB_OUTPUT: output, SHOT_TARGET: target = '', SHOT_SHARDS: requested = 'auto', SHOT_FINGERPRINT: fingerprint} = process.env;
  const selection = await select({
    requested, target, fingerprint: fingerprint || undefined,
    lookup: async (label, fp) => {
      const {api, currentRunId} = environment();
      return (await findBuildDirs(api, label, fp, currentRunId))
        .map(({runId, exact, headSha, branch, createdAt}) => ({runId, exact, headSha, branch, createdAt}));
    },
    changes: changedInputs,
    index: downloadIndex,
  });
  console.log(`${selection.reason}.`);
  if (!output) throw new Error('GITHUB_OUTPUT is required');
  appendFileSync(output, outputs(selection));
}

if (process.argv[1] && path.basename(process.argv[1]) === 'select-shards.ts') {
  main().catch((error: unknown) => { console.error(error); process.exitCode = 1; });
}
