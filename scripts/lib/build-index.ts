// What a saved build directory would have to rebuild for a given change,
// answered without the build directory.
//
// why: ci:select-shards decides how many runners a platform gets before any
// runner has restored the 2 GB build-directory artifact, so it cannot ask
// ninja. It used to assume the worst -- three shards for amd64, two for
// arm64 -- for every change, which turns a one-file fix into twenty setup
// phases across the eight platforms and queues the whole run behind the
// repository's concurrency budget. A build directory already knows the
// answer: .ninja_deps says which sources each object read, .ninja_log says
// how long each object took. Folding the two into "source path -> seconds
// of objects that read it" gives a table of a few hundred kilobytes that
// travels as its own artifact, and the resolve job can price a change by
// summing the table over the paths `git diff-tree` reports.
//
// The estimate is deliberately one-sided. It only prices changes whose every
// path is an ordinary source or header the table knows about; a changed
// GN file, generator input or toolchain script falls back to the platform
// default, because the table cannot see what a regenerated build.ninja
// dirties. An underestimate costs time, never correctness: ninja rebuilds
// what is stale regardless of how many runners were chosen.

import {existsSync, readFileSync} from 'node:fs';
import path from 'node:path';

import {parseDeps, parseLog} from './ninja-state.ts';

export const INDEX_VERSION = 1;

export interface BuildIndex {
  version: number;
  platform: string;
  /** The commit the build directory was saved from. */
  head_sha: string;
  run_id: number;
  /** Whether the run linked all three products; a partial directory prices nothing. */
  complete: boolean;
  /** Milliseconds of every logged edge whose output still exists: one cold build. */
  total_ms: number;
  /** Repository-relative source path -> milliseconds of the objects that read it. */
  paths: Record<string, number>;
}

// Files whose change is priced by the objects that include them. Everything
// else -- GN, DEPS, generator inputs, scripts, data -- is priced as unknown.
const SOURCE = /\.(c|cc|cpp|cxx|m|mm|h|hh|hpp|hxx|inc|inl|rs|S|s|asm|def)$/i;
const IMPLEMENTATION = /\.(c|cc|cpp|cxx|m|mm|rs|S|s|asm)$/i;

// A source file the table has never seen is a new one: one object, or a
// regenerated jumbo unit, on the runner that owns it.
export const NEW_SOURCE_MS = 15_000;

/** Fold the ninja log and deps log of a build directory into a source-path cost table. */
export function buildIndex(buildDir: string, meta: {platform: string; headSha: string; runId: number; complete: boolean}): BuildIndex {
  const logFile = path.join(buildDir, '.ninja_log');
  const depsFile = path.join(buildDir, '.ninja_deps');
  const entries = existsSync(logFile) ? parseLog(readFileSync(logFile, 'utf8')).entries : new Map();
  const deps = existsSync(depsFile) ? parseDeps(readFileSync(depsFile)) : {paths: [], records: new Map()};
  const paths: Record<string, number> = {};
  let total = 0;
  for (const [output, entry] of entries) {
    if (!existsSync(path.join(buildDir, output))) continue;
    const ms = Math.max(0, entry.end - entry.start);
    total += ms;
    const record = deps.records.get(output);
    if (!record) continue;
    for (const dep of record.deps) {
      const source = sourcePath(dep);
      if (source !== null) paths[source] = (paths[source] ?? 0) + ms;
    }
  }
  return {version: INDEX_VERSION, platform: meta.platform, head_sha: meta.headSha, run_id: meta.runId, complete: meta.complete, total_ms: total, paths};
}

/** A deps-log path relative to the build directory, as a repository path; null for generated files and anything outside the tree. */
export function sourcePath(dep: string): string | null {
  const normalized = dep.replace(/\\/g, '/');
  if (!normalized.startsWith('../../')) return null;
  const rel = normalized.slice('../../'.length);
  if (rel.startsWith('../') || rel.startsWith('out/')) return null;
  return rel;
}

export interface Estimate {
  /** Milliseconds of compile work the change dirties, when every path could be priced. */
  ms: number | null;
  /** Paths the table cannot price; any of them makes `ms` null. */
  unknown: string[];
}

/** Price a change: the sum over its paths of the objects that read them. */
export function estimate(index: BuildIndex, changed: readonly string[]): Estimate {
  if (!index.complete) return {ms: null, unknown: ['<the build directory is incomplete>']};
  let ms = 0;
  const unknown: string[] = [];
  for (const file of changed) {
    const known = index.paths[file];
    if (known !== undefined) ms += known;
    else if (IMPLEMENTATION.test(file)) ms += NEW_SOURCE_MS;
    else if (!SOURCE.test(file)) unknown.push(file);
    // A header nothing on this platform includes costs nothing.
  }
  return {ms: unknown.length ? null : Math.min(ms, index.total_ms), unknown};
}

// The work one runner takes before a second one pays for itself, as a
// share of a cold build. An extra shard costs its own source setup and the
// final job's download and merge -- six to eight minutes -- and a tenth of
// the linux-amd64 build (59 of 593 logged edge-minutes, about 15 minutes of
// wall clock on a four-core runner) is where splitting starts to win. The
// share is platform-neutral where a minute is not: Windows runners log
// slower edges, macOS arm64 faster ones.
export const SHARD_SHARE = 0.1;

/** How many runners a priced change deserves: one per SHARD_SHARE of a cold build, never more than the platform default. */
export function shardsFor(priced: number, index: BuildIndex, defaultCount: number): number {
  if (defaultCount <= 1 || index.total_ms <= 0) return Math.max(1, defaultCount);
  const perShard = index.total_ms * SHARD_SHARE;
  return Math.min(defaultCount, Math.max(1, Math.ceil(priced / perShard)));
}

export function parseIndex(text: string): BuildIndex {
  const index = JSON.parse(text) as BuildIndex;
  if (index.version !== INDEX_VERSION) throw new Error(`build index version ${index.version}; this reader knows ${INDEX_VERSION}`);
  return index;
}
