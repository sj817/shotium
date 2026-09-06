// why: a Chromium build on a free 4-core runner is two hours cold, and the
// only way to make it shorter without paying for a bigger machine is to
// compile on several runners at once. ninja cannot do that by itself, but it
// does not need to: N runners that ran the same `gn gen` share one graph, so
// each can build a slice of it and hand its outputs and its ninja state
// (.ninja_log, .ninja_deps; see lib/ninja-state.ts) to one final runner,
// which merges them and finds only the link left to do.
//
// The alternative, a compiler cache in front of the final runner, was
// measured first (sccache 0.17, 1,549 objects, four passes on the
// development host): every hit still cost 1-2 s of preprocessing, which is
// 20 minutes on the final runner for nothing. Moving the objects costs a
// download.
//
//   list  --build-dir out/Shot [--since <unix seconds>]
//         Prints, one per line relative to the build directory, what a shard
//         has to send: every output .ninja_log names that exists on disk;
//         every file under gen/ that ninja did not produce, because GN wrote
//         it at gen time (the jumbo units) and the final job's own gn gen
//         would write it again, newer than the objects compiled from it;
//         and every depfile (*.d), because edges without `deps =` keep theirs
//         on disk and ninja calls the edge dirty when it is missing. With
//         --since, only files newer than that moment: a shard that restored
//         the build-directory cache must not send the cache back.
//
//   merge --build-dir out/Shot --shard <dir>...
//         Copies every file of each unpacked shard into the build directory
//         and writes one merged .ninja_log and .ninja_deps. The mtimes are
//         what make ninja believe the transplant: an object is dirty when the
//         file is newer than its deps record ("stored deps info out of date")
//         or when the log's mtime is older than an input. A copy gets a new
//         mtime, so a logged output is set back to its record and then read
//         again exactly (statSync bigint; utimes takes a double and cannot
//         place a nanosecond), and the value read is what goes into both
//         logs. A file ninja did not produce keeps the shard's mtime, which
//         predates the objects that read it.
//
// Shards must come from the same commit and the same GN args as the final
// directory: the log stores a hash of each command, and a different command
// line is a rebuild, which is correct.

import {existsSync, mkdirSync, readFileSync, statSync, utimesSync, writeFileSync} from 'node:fs';
import {copyFile} from 'node:fs/promises';
import path from 'node:path';

import {cac} from 'cac';
import pc from 'picocolors';
import {glob} from 'tinyglobby';

import {
  type DepsLog, type DepsRecord, type LogEntry, LOG_HEADER,
  assignShards, formatDeps, formatLog, fromNinjaTime, parseDeps, parseLog, toNinjaTime,
} from './lib/ninja-state.ts';
import {resolve as resolveInRepo} from './lib/repo.ts';

const NINJA_STATE = ['.ninja_log', '.ninja_deps', '.ninja_lock'];

// Files in a build directory that ninja did not log fall in two groups: what
// GN wrote at gen time (jumbo units, grit's *_expected_outputs.txt, buildflag
// inputs, the .rsp files the mojom parser actions read, depfiles kept by
// edges without `deps =`), which the final job needs with the shard's older
// mtimes; and what is not an input to anything -- the ninja files themselves
// and stale binaries from a restored cache whose log entries were
// superseded. The second group is excluded by name; everything else unlogged
// travels. Response files are not excluded on purpose: the ones ninja writes
// are harmless to carry, and the ones GN writes are inputs.
const GN_TIME_EXCLUDED = [
  '.ninja_*', '**/*.ninja', 'build.ninja.d', 'build.ninja.stamp', 'args.gn', '.landmines',
  '**/*.o', '**/*.obj', '**/*.pdb', '**/*.a', '**/*.lib', '**/*.rlib', '**/*.so', '**/*.dylib',
  '**/*.dll', '**/*.exe', '**/*.pak', 'thinlto-cache/**',
];

interface State {
  header: string;
  entries: Map<string, LogEntry>;
  deps: DepsLog;
}

function readState(dir: string): State {
  const logFile = path.join(dir, '.ninja_log');
  const depsFile = path.join(dir, '.ninja_deps');
  const {header, entries} = existsSync(logFile)
    ? parseLog(readFileSync(logFile, 'utf8'))
    : {header: LOG_HEADER, entries: new Map<string, LogEntry>()};
  const deps: DepsLog = existsSync(depsFile)
    ? parseDeps(readFileSync(depsFile))
    : {paths: [], records: new Map<string, DepsRecord>()};
  return {header, entries, deps};
}

const mtimeNs = (file: string): bigint => statSync(file, {bigint: true}).mtimeNs;

async function list(buildDir: string, sinceSeconds?: number): Promise<number> {
  const {entries} = readState(buildDir);
  const sinceNs = sinceSeconds === undefined ? undefined : BigInt(Math.floor(sinceSeconds)) * 1000000000n;
  const sinceStamp = sinceNs === undefined ? undefined : toNinjaTime(sinceNs);
  const chosen = new Set<string>();
  let older = 0;
  for (const [output, entry] of entries) {
    if (sinceStamp !== undefined && entry.mtime < sinceStamp) { older++; continue; }
    if (existsSync(path.join(buildDir, output))) chosen.add(output);
  }
  const logged = chosen.size;
  // dot: true -- the Rust sysroot's lib/.empty is a gen-time file too.
  const extra = await glob('**/*', {cwd: buildDir, onlyFiles: true, dot: true, ignore: GN_TIME_EXCLUDED});
  let unlogged = 0;
  for (const file of extra) {
    const rel = file.replace(/\\/g, '/');
    if (chosen.has(rel) || entries.has(rel)) continue;
    if (sinceNs !== undefined && mtimeNs(path.join(buildDir, rel)) < sinceNs) { older++; continue; }
    chosen.add(rel);
    unlogged++;
  }
  for (const file of chosen) process.stdout.write(file + '\n');
  console.error(`${logged} logged outputs and ${unlogged} gen-time files or depfiles from ${buildDir}` +
    (sinceNs === undefined ? '' : `; ${older} predate --since and were left out`));
  return 0;
}

// Set a file's mtime as close to `wanted` (ns since 1970) as utimes allows,
// then return what the file system actually recorded.
function setMtime(file: string, wantedNs: bigint): bigint {
  const seconds = Number(wantedNs / 1000000000n) + Number(wantedNs % 1000000000n) / 1e9;
  utimesSync(file, seconds, seconds);
  return mtimeNs(file);
}

async function merge(buildDir: string, shards: string[]): Promise<number> {
  const target = readState(buildDir);
  let logged = 0;
  let unlogged = 0;
  let kept = 0;
  for (const shard of shards) {
    const src = readState(shard);
    const files = await glob('**/*', {cwd: shard, onlyFiles: true, dot: true, ignore: NINJA_STATE});
    let fromShard = 0;
    for (const file of files) {
      const rel = file.replace(/\\/g, '/');
      const from = path.join(shard, rel);
      const to = path.join(buildDir, rel);
      const entry = src.entries.get(rel);
      // Two shards often build the same thing (a buildflag header, the Rust
      // standard library, a host tool) because both needed it. The copies are
      // identical -- same graph, same sources -- but not the same age, and
      // every consumer in a shard is newer than that shard's copy, so the
      // oldest copy is the one every consumer agrees with. Keep it.
      //
      // For a logged output the log says which copy is older. For a gen-time
      // file the file itself does: the final job's own gn gen ran after every
      // shard, so its copy is the newest and loses; a cache-restored copy is
      // the oldest and stays. Nothing here may depend on process memory --
      // the workflow calls merge once per shard.
      const have = target.entries.get(rel);
      if (entry && have && have.mtime <= entry.mtime) { kept++; continue; }
      if (!entry && !have && existsSync(to) && mtimeNs(to) <= mtimeNs(from)) { kept++; continue; }
      mkdirSync(path.dirname(to), {recursive: true});
      await copyFile(from, to);
      if (entry) {
        const stamp = toNinjaTime(setMtime(to, fromNinjaTime(entry.mtime)));
        target.entries.set(rel, {...entry, mtime: stamp});
        const record = src.deps.records.get(rel);
        if (record) target.deps.records.set(rel, {mtime: stamp, deps: record.deps});
        logged++;
      } else {
        setMtime(to, mtimeNs(from));
        unlogged++;
      }
      fromShard++;
    }
    console.log(`${pc.cyan(shard)}: ${fromShard} files, ${src.entries.size} log entries, ${src.deps.records.size} deps records`);
  }
  writeFileSync(path.join(buildDir, '.ninja_log'), formatLog(target.header, target.entries.values()));
  writeFileSync(path.join(buildDir, '.ninja_deps'), formatDeps(target.deps));
  console.log(`merged into ${buildDir}: ${logged} logged outputs and ${unlogged} other files copied, ` +
    `${kept} older copies kept; now ${target.entries.size} log entries, ${target.deps.records.size} deps records`);
  return 0;
}

// The objects arrive on stdin (`ninja -t inputs shot shot_c | grep '\.o$'`),
// one per line; the slice for shard `index` of `shards` goes to stdout.
async function slice(buildDir: string, shards: number, index: number): Promise<number> {
  const chunks: Buffer[] = [];
  for await (const chunk of process.stdin) chunks.push(chunk as Buffer);
  const objects = Buffer.concat(chunks).toString('utf8').split(/\r?\n/).map((l) => l.trim()).filter(Boolean);
  const {entries} = readState(buildDir);
  let known = 0;
  const duration = (o: string): number | undefined => {
    const e = entries.get(o);
    if (!e) return undefined;
    known++;
    return e.end - e.start;
  };
  const mine = assignShards(objects, shards, duration)[index];
  for (const o of mine) process.stdout.write(o + '\n');
  console.error(`shard ${index}/${shards}: ${mine.length} of ${objects.length} objects, ${known} costs from .ninja_log, the rest estimated`);
  return 0;
}

const cli = cac('build-shards');
cli.command('slice', 'read objects on stdin, print the ones shard <index> of <shards> should build')
  .option('--build-dir <dir>', 'ninja build directory whose .ninja_log, if any, gives past durations', {default: 'out/Shot'})
  .option('--shards <n>', 'number of shards', {default: '1'})
  .option('--index <i>', 'this shard, 0-based', {default: '0'})
  .action(async (options: {buildDir: string; shards: string; index: string}) => {
    const shards = Number(options.shards);
    const index = Number(options.index);
    if (!Number.isInteger(shards) || shards < 1 || !Number.isInteger(index) || index < 0 || index >= shards) {
      console.error(`bad --shards ${options.shards} / --index ${options.index}`);
      process.exitCode = 2;
      return;
    }
    process.exitCode = await slice(resolveInRepo(options.buildDir), shards, index);
  });
cli.command('list', 'print what this build directory has to send to the final job')
  .option('--build-dir <dir>', 'ninja build directory (relative paths resolve against the repository root)', {default: 'out/Shot'})
  .option('--since <seconds>', 'only files newer than this Unix time (`date +%s` at job start)')
  .action(async (options: {buildDir: string; since?: string}) => {
    const since = options.since === undefined ? undefined : Number(options.since);
    if (since !== undefined && !Number.isFinite(since)) {
      console.error(`--since must be a Unix time in seconds, got ${options.since}`);
      process.exitCode = 2;
      return;
    }
    process.exitCode = await list(resolveInRepo(options.buildDir), since);
  });
cli.command('merge', 'transplant unpacked shard directories into one build directory and merge their ninja logs')
  .option('--build-dir <dir>', 'the build directory that will link', {default: 'out/Shot'})
  .option('--shard <dir>', 'an unpacked shard (repeatable)', {type: [String]})
  .action(async (options: {buildDir: string; shard?: string[]}) => {
    const shards = (options.shard ?? []).map((s) => resolveInRepo(s));
    if (shards.length === 0) {
      console.error('merge needs at least one --shard');
      process.exitCode = 2;
      return;
    }
    process.exitCode = await merge(resolveInRepo(options.buildDir), shards);
  });
cli.help();
cli.parse();
