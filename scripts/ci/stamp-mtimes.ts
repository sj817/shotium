// Stamp a synced Chromium workspace with content-faithful modification times.
//
// ninja decides what is stale by comparing modification times, and both
// actions/checkout and gclient write every file with "now". A build directory
// restored from a previous run is therefore always older than the sources
// that produced it, and the entire build reruns -- which makes carrying a
// build across several CI runs impossible.
//
// Compare input bytes with the state saved alongside the restored objects:
//
//   * Git-tracked files and files in DEPS-managed source repositories retain
//     their timestamp only when their content is unchanged. A branch switch,
//     rollback or replayed upstream commit must invalidate changed sources
//     even when their Git dates predate the cached objects. Unknown legacy
//     sources are invalidated once; no objects or compiler caches are deleted.
//   * runtime feature definitions and Blink generator helpers keep their
//     existing content state so this migration preserves that prior fix.
//   * downloads and hook outputs keep their prior timestamp when SHA-256 is
//     unchanged. The fingerprint state travels inside the restored build cache.
//     A DEPS hook-only edit must not invalidate an unchanged clang binary and
//     Rust sysroot, which otherwise dirties almost every compile edge. Changed
//     bytes get a deterministic timestamp newer than the cached ninja log;
//     caches without fingerprint state use the legacy DEPS timestamp once.
//     Restore the build directory before running this script.
//
// The output directory is left alone: its timestamps are what the comparison
// is against.
//
//   pnpm ci:stamp-mtimes <workspace> [--solution src] [--out-dir out]
//
// The workspace is the directory holding .gclient; a relative path is
// resolved against the repository root.

import {existsSync, readFileSync, utimesSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execaSync} from 'execa';
import {globSync} from 'tinyglobby';

import {resolve} from '../lib/repo.ts';
import {createSourceMtimes, InputMtimes} from '../lib/input-mtimes.ts';
import {createRuntimeFeatureMtimes, isRuntimeFeatureInput} from '../lib/runtime-feature-mtimes.ts';

function git(repo: string, ...args: string[]): string {
  return execaSync('git', ['--no-optional-locks', '-C', repo, ...args], {maxBuffer: 1 << 30}).stdout;
}

// The dependency list gclient wrote, as paths relative to the workspace. The
// file is a Python literal, `entries = { 'src/x': 'url@rev', ... }`; the keys
// are all that is needed.
function readGclientEntries(workspace: string): string[] {
  const file = path.join(workspace, '.gclient_entries');
  if (!existsSync(file)) return [];
  const text = readFileSync(file, 'utf8');
  const start = text.indexOf('entries');
  return [...text.slice(start).matchAll(/^\s*['"]([^'"]+)['"]\s*:/gm)].map((m) => m[1]).sort();
}

// Files under root, not descending into skipDirs, .git or out.
function walk(rootDir: string, skipDirs: Set<string>): string[] {
  const ignore = [...skipDirs].filter((d) => d.startsWith(rootDir + path.sep) || d.startsWith(rootDir + '/'))
                     .map((d) => path.relative(rootDir, d).replace(/\\/g, '/') + '/**');
  return globSync('**/*', {cwd: rootDir, dot: true, onlyFiles: true, ignore: ['**/.git/**', ...ignore], absolute: true});
}

function main(workspaceArg: string, solutionName: string, outDir: string): number {
  const workspace = resolve(workspaceArg);
  const solution = path.join(workspace, solutionName);
  const entries = readGclientEntries(workspace);
  // Directories that belong to a dependency, so the walk of the main
  // repository does not classify them as untracked downloads.
  const depDirs = new Set(entries.filter((e) => e !== solutionName).map((e) => path.join(workspace, e)));
  const skip = new Set([...depDirs, path.join(solution, outDir)]);
  const counters = {stamped: 0, failed: 0};
  const stamp = (file: string, when: number) => {
    try {
      utimesSync(file, when, when);
      counters.stamped++;
    } catch {
      counters.failed++;
    }
  };

  const tracked = new Set(git(solution, 'ls-files', '-z').split('\0').filter(Boolean));
  const head = Number(git(solution, 'log', '-1', '--format=%ct').trim());
  const depsTime = Number(git(solution, 'log', '-1', '--format=%ct', '--', 'DEPS').trim());
  const buildDir = path.join(solution, outDir, 'Shot');
  const sources = createSourceMtimes(buildDir, head);
  const downloaded = new InputMtimes(path.join(buildDir, 'ci-input-mtimes.json'), depsTime, head);
  const runtimeFeatures = createRuntimeFeatureMtimes(buildDir, head);
  console.log(`${solutionName}: ${tracked.size} tracked paths, head ${head}, DEPS ${depsTime}`);
  for (const file of walk(solution, skip)) {
    const rel = path.relative(solution, file).replace(/\\/g, '/');
    let when: number;
    if (isRuntimeFeatureInput(rel)) {
      // Used by Windows, macOS and Linux before gn gen and shard compilation.
      // Stamping the inputs makes every dependent generator rerun, including
      // public features and policy helpers, not just one generated header.
      when = runtimeFeatures.time(file, rel);
    } else if (tracked.has(rel)) {
      when = sources.time(file, path.relative(workspace, file).replace(/\\/g, '/'));
    } else {
      // Compare downloaded/generated bytes with the same cache as the objects.
      when = downloaded.time(file, path.relative(workspace, file).replace(/\\/g, '/'));
    }
    stamp(file, when);
  }

  for (const entry of entries) {
    if (entry === solutionName) continue;
    const repo = path.join(workspace, entry);
    if (!existsSync(path.join(repo, '.git'))) {
      // CIPD/GCS dependencies have no Git commit; compare their actual bytes.
      if (existsSync(repo)) for (const file of walk(repo, depDirs)) {
        stamp(file, downloaded.time(file, path.relative(workspace, file).replace(/\\/g, '/')));
      }
      continue;
    }
    for (const file of walk(repo, depDirs)) {
      stamp(file, sources.time(file, path.relative(workspace, file).replace(/\\/g, '/')));
    }
  }
  if (counters.failed) throw new Error(`Failed to stamp ${counters.failed} input files`);
  sources.save();
  downloaded.save();
  runtimeFeatures.save();
  console.log(`stamped ${counters.stamped} files, ${counters.failed} failed`);
  return 0;
}

const cli = cac('pnpm ci:stamp-mtimes');
cli.command('<workspace>', 'give synced inputs cache-relative, content-faithful modification times')
    .option('--solution <name>', 'the main checkout inside the workspace', {default: 'src'})
    .option('--out-dir <dir>', 'relative to the solution; left untouched', {default: 'out'})
    .action((workspace: string, options: {solution: string; outDir: string}) => {
      process.exitCode = main(workspace, options.solution, options.outDir);
    });
cli.help();
cli.parse();
