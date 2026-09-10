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
import {InputMtimes} from '../lib/input-mtimes.ts';

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
  const downloaded = new InputMtimes(path.join(solution, outDir, 'Shot', 'ci-input-mtimes.json'), depsTime, head);
  console.log(`${solutionName}: ${times.size} tracked paths, head ${head}, DEPS ${depsTime}`);
  for (const file of walk(solution, skip)) {
    const rel = path.relative(solution, file).replace(/\\/g, '/');
    let when = times.get(rel);
    if (when === undefined) {
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
    let when: number;
    try {
      when = Number(git(repo, 'log', '-1', '--format=%ct').trim());
    } catch {
      continue;
    }
    for (const file of walk(repo, depDirs)) stamp(file, when);
  }
  if (counters.failed) throw new Error(`Failed to stamp ${counters.failed} input files`);
  downloaded.save();
  console.log(`stamped ${counters.stamped} files, ${counters.failed} failed`);
  return 0;
}

const cli = cac('pnpm ci:stamp-mtimes');
cli.command('<workspace>', 'give every synced file the modification time of the revision it came from')
    .option('--solution <name>', 'the main checkout inside the workspace', {default: 'src'})
    .option('--out-dir <dir>', 'relative to the solution; left untouched', {default: 'out'})
    .action((workspace: string, options: {solution: string; outDir: string}) => {
      process.exitCode = main(workspace, options.solution, options.outDir);
    });
cli.help();
cli.parse();
