// Stamp a synced Chromium workspace with content-faithful modification times.
//
// ninja decides what is stale by comparing modification times, and both
// actions/checkout and gclient write every file with "now". A build directory
// restored from a previous run is therefore always older than the sources
// that produced it, and the entire build reruns -- which makes carrying a
// build across several CI runs impossible.
//
// The fix is to give every source file the time of the revision it came from:
//
//   * files in the main repository get the time of the last commit that
//     touched them, read in a single pass over the log. A commit that changes
//     ten files then moves ten timestamps, and ninja rebuilds exactly those.
//   * files in a DEPS-managed repository get that repository's HEAD commit
//     time. A dependency moves as a unit -- when the pinned revision changes,
//     every file in it is suspect anyway.
//   * anything a hook produced or downloaded gets the time of the last commit
//     that touched DEPS, which is what pins it. "Inputs to few edges" was the
//     first guess here and it was wrong twice over: the clang binary is an
//     explicit input of the libc++ module.pcm, which every CXX edge depends
//     on, and the rust stdlib .rlibs are inputs of libstd -- a freshly
//     downloaded toolchain with "now" for a timestamp re-dirtied the entire
//     build, which ninja -d explain named directly. LASTCHANGE is one of
//     these: the DEPS hook pins it to the root commit, so its content is a
//     constant and DEPS's time is right for it too. (It used to follow HEAD,
//     and through base/check.cc, libbase and every host tool that recompiled
//     ~1,300 steps on every "warm" run.)
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

import {resolve} from './lib/repo.ts';

function git(repo: string, ...args: string[]): string {
  return execaSync('git', ['-C', repo, ...args], {maxBuffer: 1 << 30}).stdout;
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

// The last time each tracked path was touched, from one pass over the log.
function fileTimesFromLog(repo: string): Map<string, number> {
  const out = git(repo, 'log', '--format=@%ct', '--name-only', '--no-renames');
  const times = new Map<string, number>();
  let current: number | null = null;
  for (const line of out.split(/\r?\n/)) {
    if (line.startsWith('@')) current = Number(line.slice(1));
    else if (line && current !== null && !times.has(line)) times.set(line, current);  // first seen == most recent
  }
  return times;
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
  // repository does not stamp them with the wrong repository's time.
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

  const times = fileTimesFromLog(solution);
  const head = Number(git(solution, 'log', '-1', '--format=%ct').trim());
  const depsTime = Number(git(solution, 'log', '-1', '--format=%ct', '--', 'DEPS').trim());
  console.log(`${solutionName}: ${times.size} tracked paths, head ${head}, DEPS ${depsTime}`);
  for (const file of walk(solution, skip)) {
    const rel = path.relative(solution, file).replace(/\\/g, '/');
    let when = times.get(rel);
    if (when === undefined) {
      // Untracked: a hook wrote it (llvm-build, rust-toolchain, LASTCHANGE)
      // or it is local debris. DEPS pins the toolchains and the LASTCHANGE
      // value, so DEPS's time is their provenance; if a toolchain ever
      // changes without a DEPS edit, the CR_CLANG_REVISION / rustflags in
      // every command line still force the rebuild mtimes no longer would.
      when = depsTime;
    }
    stamp(file, when);
  }

  for (const entry of entries) {
    if (entry === solutionName) continue;
    const repo = path.join(workspace, entry);
    if (!existsSync(path.join(repo, '.git'))) {
      // A CIPD or GCS dependency: no commit of its own, but DEPS pins it, so
      // DEPS's time is the honest answer -- "downloaded just now" makes every
      // cached compile downstream of it stale.
      if (existsSync(repo)) for (const file of walk(repo, depDirs)) stamp(file, depsTime);
      continue;
    }
    let when: number;
    try {
      when = Number(git(repo, 'log', '-1', '--format=%ct').trim());
    } catch {
      continue;
    }
    for (const file of walk(repo, depDirs)) stamp(file, when);
  }
  console.log(`stamped ${counters.stamped} files, ${counters.failed} failed`);
  return 0;
}

const cli = cac('ci-stamp-mtimes');
cli.command('<workspace>', 'give every synced file the modification time of the revision it came from')
    .option('--solution <name>', 'the main checkout inside the workspace', {default: 'src'})
    .option('--out-dir <dir>', 'relative to the solution; left untouched', {default: 'out'})
    .action((workspace: string, options: {solution: string; outDir: string}) => {
      process.exitCode = main(workspace, options.solution, options.outDir);
    });
cli.help();
cli.parse();
