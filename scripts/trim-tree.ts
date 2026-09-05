// Trim the tree to the files the engine builds actually read.
//
// This repository is a slice of Chromium, and most of what the slice still
// carries is never opened by any build: tests, Android and iOS code, tooling
// for a browser that is not built here. Whether a file is needed is not a
// judgement call -- ninja and gn record exactly what they read -- so this
// script turns those records into the list of files to delete, and nothing
// else decides.
//
// Three subcommands:
//
//   export   Write one build directory's records to a folder: ninja's inputs
//            for the two engine targets, the nodes of `ninja -t graph` (the
//            order-only edges `-t inputs` skips), its deps log (headers),
//            the depfiles of reachable actions (what grit read), gn's
//            build.ninja.d (every .gn/.gni/exec_script it read), and the
//            sources named by the jumbo translation units (the merged .cc
//            files are what ninja sees; the real sources only appear as
//            #include lines). CI runs this after every engine build and
//            uploads the folder, because a macOS graph cannot be generated
//            on the Windows host.
//
//            Four things no record names and the whitelist carries with a
//            reason: what an exec_script opens itself, what a Python script
//            imports, files passed to the linker as flags (/NATVIS:), and
//            grit's resource-id depfile (its allocator skips missing .grd).
//
//   plan     Union the exports of every platform, intersect with git's
//            tracked files, apply the closure rules below, and write the
//            keep and delete lists with a summary. Nothing is touched.
//
//   apply    `git rm` the delete list from a plan. Explicit paths only; this
//            never stages anything it did not list.
//
// Closure rules, in order (later rules only add to the keep set, except the
// last, which removes from it):
//   1. Anything a platform's records name.
//   2. The whitelist: this project's own directories and root files.
//   3. Python: gn records the exec_script it ran, not the modules that script
//      imports, so every .py under a directory that holds a kept .py stays.
//   4. build/ stays whole; the toolchain scripts import each other freely.
//   5. Licence files and nested .gitignore files in any ancestor directory of
//      a kept file stay.
//   6. A vendored Rust crate the build reads stays whole (Cargo.toml, LICENSE).
//   7. An extra keep list (--keep FILE, one path or prefix/ per line).
//   8. Under build/, paths for platforms this project never builds (android,
//      fuchsia, ios, chromeos ...) are dropped again unless a record names them.
//
// Usage (from anywhere in the repository):
//
//   pnpm trim-tree export --build-dir out/Shot --out graph/win-x64
//   pnpm trim-tree plan --graph graph/win-x64 --graph graph/linux-x64 ... --out trim
//   pnpm trim-tree apply --plan trim/plan.json
//
// Relative paths are resolved against the repository root, not the directory
// the command was typed in (pnpm runs this with cwd = scripts/).

import {existsSync} from 'node:fs';
import {copyFile, mkdir, readFile, rm, stat, writeFile} from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import pc from 'picocolors';
import {glob} from 'tinyglobby';

import {parseDepfile} from './lib/depfile.ts';

const root = path.resolve(import.meta.dirname, '..');
const resolve = (p: string) => path.resolve(root, p);

// ---------------------------------------------------------------------------
// Path normalisation shared by export and plan.

const driveLetter = /^[A-Za-z]:\//;

// A record entry as ninja or gn wrote it, or null when it is not a file of
// this repository: an absolute SDK path, a generated file, the build's own
// args.gn.
function normalise(raw: string): string | null {
  let p = raw.trim().replace(/\\/g, '/');
  if (!p) return null;
  if (p.startsWith('"') && p.endsWith('"')) p = p.slice(1, -1);
  while (p.startsWith('../../')) p = p.slice(6);
  if (p.startsWith('./')) p = p.slice(2);
  if (driveLetter.test(p) || p.startsWith('/')) return null;
  if (p.startsWith('obj/') || p.startsWith('gen/') || p.startsWith('clang_x64/')) return null;
  if (!p.includes('/')) return null;  // args.gn, brotli.exe, toolchain.ninja ...
  return p;
}

// ---------------------------------------------------------------------------
// export

function findNinja(): string {
  const local = resolve(process.platform === 'win32' ? 'third_party/ninja/ninja.exe' : 'third_party/ninja/ninja');
  return existsSync(local) ? local : 'ninja';
}

async function exportGraph(buildDirArg: string, outArg: string): Promise<number> {
  const buildDir = resolve(buildDirArg);
  const out = resolve(outArg);
  if (!existsSync(path.join(buildDir, 'build.ninja'))) {
    console.log(pc.red(`no build.ninja in ${buildDir}; run gn gen first`));
    return 1;
  }
  await mkdir(out, {recursive: true});
  const ninja = findNinja();

  // ninja -t inputs: every input of the two engine targets, transitively.
  // buffer: false -- the deps log of a full build is over 100 MB, which is
  // execa's default maxBuffer, and it would kill ninja at that point even
  // though the output goes to a file.
  await execa(ninja, ['-C', buildDir, '-t', 'inputs', 'shot', 'shot_c'],
              {cwd: root, stdout: {file: path.join(out, 'inputs.txt')}, stderr: 'inherit', buffer: false});

  // ninja -t deps: the deps log, i.e. what the compiler reported reading. Only
  // present for objects that were actually built, which is why a probe (gn
  // gen + ninja -n) exports far fewer headers than a build.
  await execa(ninja, ['-C', buildDir, '-t', 'deps'],
              {cwd: root, stdout: {file: path.join(out, 'deps.txt')}, stderr: 'inherit', buffer: false});

  await copyFile(path.join(buildDir, 'build.ninja.d'), path.join(out, 'build.ninja.d'));

  // ninja -t graph: every node reachable from the two targets, including the
  // order-only edges that `-t inputs` leaves out. GN attaches a target's
  // `inputs` (natvis files, .grd files read by the resource-id allocator)
  // through a phony ".inputs" edge on the order-only side, so a tree trimmed
  // from `-t inputs` alone passes gn gen and fails `ninja -n`. The dot output
  // is only mined for its node labels.
  const dot = await execa(ninja, ['-C', buildDir, '-t', 'graph', 'shot', 'shot_c'], {cwd: root, stderr: 'inherit', maxBuffer: 1 << 30});
  const nodes = new Set<string>();
  for (const m of dot.stdout.matchAll(/label="([^"]*)"/g)) nodes.add(m[1].replace(/\\\\/g, '/'));
  await writeFile(path.join(out, 'graph.txt'), [...nodes].sort().join('\n') + '\n');

  // Depfiles: what an action read while it ran, which is the only record of
  // the files grit inlines into a .pak (png, css, json, the .xtb
  // translations). Only the depfiles of edges the graph reaches count; a
  // warm directory also holds depfiles of edges that no longer exist. One is
  // skipped on purpose: gen/tools/gritsettings/default_resource_ids.d lists
  // every .grd in resource_ids.spec that happened to exist, and the allocator
  // skips a missing one (grit/tool/update_resource_ids/reader.py), so those
  // files are not inputs in any sense that matters.
  const depfileDeps = new Set<string>();
  for (const file of await glob('**/*.d', {cwd: buildDir, absolute: true})) {
    const rel = path.relative(buildDir, file).replace(/\\/g, '/');
    if (rel === 'gen/tools/gritsettings/default_resource_ids.d') continue;
    for (const {target, deps} of parseDepfile(await readFile(file, 'utf8'))) {
      if (!nodes.has(target)) continue;
      for (const dep of deps) depfileDeps.add(dep);
    }
  }
  await writeFile(path.join(out, 'depfiles.txt'), [...depfileDeps].sort().join('\n') + '\n');

  // The jumbo TUs: gen/<dir>/<target>_shot_jumbo_N.cc, each a list of
  // #include "../../real/source.cc".
  const jumbo = await glob('gen/**/*_shot_jumbo_*.cc', {cwd: buildDir, absolute: true});
  const sources = new Set<string>();
  for (const file of jumbo) {
    for (const line of (await readFile(file, 'utf8')).split(/\r?\n/)) {
      const m = /^\s*#include\s+"([^"]+)"/.exec(line);
      if (m) sources.add(m[1]);
    }
  }
  await writeFile(path.join(out, 'jumbo.txt'), [...sources].sort().join('\n') + '\n');

  const args = existsSync(path.join(buildDir, 'args.gn')) ? await readFile(path.join(buildDir, 'args.gn'), 'utf8') : '';
  const cpu = /target_cpu\s*=\s*"([^"]+)"/.exec(args)?.[1] ?? os.arch();
  const hostOs: Record<string, string> = {win32: 'win', darwin: 'mac'};
  const targetOs = /target_os\s*=\s*"([^"]+)"/.exec(args)?.[1] ?? hostOs[process.platform] ?? process.platform;
  const meta = {
    hostPlatform: process.platform,
    targetOs,
    targetCpu: cpu,
    buildDir: buildDirArg,
    jumboUnits: jumbo.length,
    date: new Date().toISOString(),
  };
  await writeFile(path.join(out, 'meta.json'), JSON.stringify(meta, null, 2) + '\n');
  console.log(`exported ${buildDirArg} (${targetOs}/${cpu}, ${jumbo.length} jumbo units) to ${outArg}`);
  return 0;
}

// ---------------------------------------------------------------------------
// plan

async function readRecords(dir: string): Promise<Set<string>> {
  const union = new Set<string>();
  const add = (raw: string) => {
    const p = normalise(raw);
    if (p) union.add(p);
  };
  const text = async (name: string) => {
    const file = path.join(dir, name);
    if (!existsSync(file)) {
      console.log(pc.yellow(`${dir}: no ${name}`));
      return '';
    }
    return readFile(file, 'utf8');
  };

  for (const line of (await text('inputs.txt')).split(/\r?\n/)) add(line);
  for (const line of (await text('graph.txt')).split(/\r?\n/)) add(line);
  for (const line of (await text('depfiles.txt')).split(/\r?\n/)) add(line);
  for (const line of (await text('jumbo.txt')).split(/\r?\n/)) add(line);
  // deps.txt: "obj/x.obj: #deps N, ..." header lines, then one indented path
  // per line.
  for (const line of (await text('deps.txt')).split(/\r?\n/)) {
    if (line.startsWith('    ')) add(line);
  }
  // build.ninja.d: "build.ninja: a b c \" with line continuations. Paths with
  // spaces are escaped as "\ ", which no path of this repository has.
  const d = (await text('build.ninja.d')).replace(/\\\r?\n/g, ' ');
  for (const token of d.split(/\s+/)) {
    if (token && !token.endsWith(':')) add(token);
  }
  return union;
}

// Tracked files, without the gitlinks: a DEPS checkout registered as a
// submodule is an index entry of mode 160000, and whether it stays is the
// DEPS entry's decision (scripts/prune-deps.ts), not the build graph's.
async function trackedFiles(): Promise<string[]> {
  const {stdout} = await execa('git', ['--no-optional-locks', 'ls-files', '-s', '-z'], {cwd: root, maxBuffer: 1 << 28});
  const files: string[] = [];
  for (const entry of stdout.split('\0')) {
    if (!entry) continue;
    // "<mode> <object> <stage>\t<path>"
    const tab = entry.indexOf('\t');
    if (tab < 0 || entry.startsWith('160000 ')) continue;
    files.push(entry.slice(tab + 1));
  }
  return files;
}

// Rule 2. Directories end with '/'; files match exactly.
const whitelist = [
  '.clang-format', '.rustfmt.toml',
  // build/compute_build_timestamp.py open()s chrome/VERSION from inside an
  // exec_script, which no graph records.
  'chrome/VERSION',
  // build/win/set_appcontainer_acls.py appends testing/scripts to sys.path and
  // imports common, which imports test_env (and xvfb on Linux) from testing/;
  // an import is not an input either.
  'testing/scripts/common.py', 'testing/test_env.py', 'testing/xvfb.py',
  // tools/win/DebugVisualizers/BUILD.gn passes its .natvis files to the linker
  // as /NATVIS: ldflags; a flag is not an input, and lld-link fails without
  // the file.
  'tools/win/DebugVisualizers/', '.gitattributes', '.gitignore', '.gitmodules', '.gn', '.vpython3',
  'AGENTS.md', 'AUTHORS', 'BUILD.gn', 'CLAUDE.md', 'DEPS', 'LICENSE', 'README.md', 'README.zh.md',
  'package.json',
  '.claude/', '.github/', 'apps/', 'benchmark-results/', 'bootstrap/', 'build_overrides/',
  'buildtools/', 'build/args/', 'build/config/shot_build.gni', 'docs/', 'patches/', 'scripts/',
  'shot/', 'shotium/', 'tests/', 'tools/shot/',
  // .sha1 stamps the dsymutil_mac_* gclient hooks download by; no build reads them.
  'tools/clang/dsymutil/',
];

// Rule 8. A path under build/ with one of these as a directory name belongs to
// a platform this project never builds.
const foreignPlatforms = new Set([
  'android', 'fuchsia', 'ios', 'chromeos', 'cros', 'lacros', 'cast', 'nacl', 'aix', 'zos',
]);

// Kept beside anything kept, in every ancestor directory: licences, and the
// nested .gitignore files that hide the DEPS checkouts (third_party/.gitignore
// is why `git status` does not list third_party/llvm-build).
const licenceFile = /^(LICENSE|LICENCE|COPYING|NOTICE|PATENTS|README\.chromium|\.gitignore)(\.[A-Za-z0-9_-]+)?$/i;

function matchesWhitelist(file: string, extra: string[]): boolean {
  for (const entry of [...whitelist, ...extra]) {
    if (entry.endsWith('/') ? file.startsWith(entry) : file === entry) return true;
  }
  return false;
}

function isForeignBuildPath(file: string): boolean {
  if (!file.startsWith('build/')) return false;
  const dirs = file.split('/').slice(1, -1);
  return dirs.some((d) => foreignPlatforms.has(d));
}

function* ancestors(file: string): Generator<string> {
  let dir = path.posix.dirname(file);
  while (dir !== '.' && dir !== '') {
    yield dir;
    dir = path.posix.dirname(dir);
  }
}

interface Plan {
  graphs: string[];
  tracked: number;
  keep: string[];
  delete: string[];
}

async function plan(graphDirs: string[], outArg: string, keepListArg?: string): Promise<number> {
  if (graphDirs.length === 0) {
    console.log(pc.red('plan needs at least one --graph directory'));
    return 2;
  }
  const union = new Set<string>();
  for (const g of graphDirs) {
    const records = await readRecords(resolve(g));
    console.log(`${g}: ${records.size} paths`);
    for (const p of records) union.add(p);
  }
  const extra: string[] = keepListArg ?
      (await readFile(resolve(keepListArg), 'utf8')).split(/\r?\n/).map((l) => l.trim()).filter((l) => l && !l.startsWith('#')) :
      [];

  const tracked = await trackedFiles();
  const trackedSet = new Set(tracked);
  const keep = new Set<string>();

  // 1 + 2 + 7, and the Blink IDL files: no build reads them, but they are
  // the source the committed V8<Enum>/union/dictionary bindings were
  // generated from (tools/shot/gen_idl_*.py), and a sync regenerates from them.
  for (const f of tracked) {
    if (union.has(f) || matchesWhitelist(f, extra)) keep.add(f);
    else if (f.startsWith('third_party/blink/renderer/') && f.endsWith('.idl')) keep.add(f);
  }
  // 3: Python packages around a kept script.
  const pyDirs = new Set<string>();
  for (const f of keep) if (f.endsWith('.py')) pyDirs.add(path.posix.dirname(f));
  for (const f of tracked) {
    if (!f.endsWith('.py') || keep.has(f)) continue;
    for (const dir of ancestors(f)) {
      if (pyDirs.has(dir)) {
        keep.add(f);
        break;
      }
    }
  }
  // 4: build/ whole.
  for (const f of tracked) if (f.startsWith('build/')) keep.add(f);
  // 6: vendored crates whole, and the crate registry's own files.
  const crates = new Set<string>();
  const vendor = 'third_party/rust/chromium_crates_io/vendor/';
  for (const f of keep) {
    if (f.startsWith(vendor)) crates.add(f.slice(vendor.length).split('/')[0]);
  }
  for (const f of tracked) {
    if (f.startsWith('third_party/rust/chromium_crates_io/') && !f.startsWith(vendor)) keep.add(f);
    else if (f.startsWith(vendor) && crates.has(f.slice(vendor.length).split('/')[0])) keep.add(f);
  }
  // 5: licences beside anything kept.
  const byDir = new Map<string, string[]>();
  for (const f of tracked) {
    if (licenceFile.test(path.posix.basename(f))) {
      const dir = path.posix.dirname(f);
      byDir.set(dir, [...(byDir.get(dir) ?? []), f]);
    }
  }
  for (const f of [...keep]) {
    for (const dir of ancestors(f)) for (const l of byDir.get(dir) ?? []) keep.add(l);
    for (const l of byDir.get('.') ?? []) keep.add(l);
  }
  // 8: foreign platforms under build/, unless a record names the file.
  for (const f of [...keep]) {
    if (isForeignBuildPath(f) && !union.has(f) && !matchesWhitelist(f, extra)) keep.delete(f);
  }

  const del = tracked.filter((f) => !keep.has(f));
  const result: Plan = {graphs: graphDirs, tracked: tracked.length, keep: [...keep].sort(), delete: del.sort()};
  const out = resolve(outArg);
  await mkdir(out, {recursive: true});
  await writeFile(path.join(out, 'plan.json'), JSON.stringify(result, null, 1) + '\n');
  await writeFile(path.join(out, 'delete.txt'), result.delete.join('\n') + '\n');
  await writeFile(path.join(out, 'keep.txt'), result.keep.join('\n') + '\n');
  // Records naming files git does not track: DEPS checkouts and generated
  // files, useful for pruning DEPS.
  const untracked = [...union].filter((p) => !trackedSet.has(p)).sort();
  await writeFile(path.join(out, 'untracked-inputs.txt'), untracked.join('\n') + '\n');

  summarise(result, tracked);
  console.log(`\nwrote ${outArg}/plan.json, delete.txt, keep.txt, untracked-inputs.txt`);
  return 0;
}

function summarise(result: Plan, tracked: string[]): void {
  const top = (f: string) => f.split('/')[0];
  const ext = (f: string) => path.posix.extname(f) || '(none)';
  const count = (files: string[], key: (f: string) => string) => {
    const m = new Map<string, number>();
    for (const f of files) m.set(key(f), (m.get(key(f)) ?? 0) + 1);
    return m;
  };
  console.log(`\ntracked ${tracked.length}  keep ${result.keep.length}  delete ${result.delete.length}`);

  const keepTop = count(result.keep, top), allTop = count(tracked, top);
  const rows = [...allTop.entries()].sort((a, b) => b[1] - a[1]).slice(0, 40)
      .map(([dir, n]) => ({dir, tracked: n, keep: keepTop.get(dir) ?? 0, delete: n - (keepTop.get(dir) ?? 0)}));
  console.table(rows);

  const keepExt = count(result.keep, ext), allExt = count(tracked, ext);
  const exts = ['.cc', '.h', '.c', '.mm', '.java', '.rs', '.py', '.gn', '.gni', '.mojom', '.idl', '.md', '.ps1', '.cjs'];
  console.table(exts.map((e) => ({ext: e, tracked: allExt.get(e) ?? 0, keep: keepExt.get(e) ?? 0, delete: (allExt.get(e) ?? 0) - (keepExt.get(e) ?? 0)})));
}

// ---------------------------------------------------------------------------
// apply

async function apply(planArg: string): Promise<number> {
  const file = resolve(planArg);
  const parsed = JSON.parse(await readFile(file, 'utf8')) as Plan;
  if (parsed.delete.length === 0) {
    console.log('nothing to delete');
    return 0;
  }
  // `git status` on this tree holds index.lock for tens of seconds and a
  // killed one leaves it behind; a lock older than a minute is nobody's.
  const lock = path.join(root, '.git', 'index.lock');
  if (existsSync(lock) && Date.now() - (await stat(lock)).mtimeMs > 60_000) {
    await rm(lock);
    console.log('removed a stale .git/index.lock');
  }
  const list = path.join(os.tmpdir(), `trim-tree-${process.pid}.txt`);
  await writeFile(list, parsed.delete.join('\n') + '\n');
  console.log(`git rm ${parsed.delete.length} files ...`);
  const rmResult = await execa('git', ['rm', '-q', '--pathspec-from-file', list], {cwd: root, reject: false, stdio: 'inherit'});
  await rm(list);
  if (rmResult.exitCode !== 0) {
    console.log(pc.red(`git rm exited ${rmResult.exitCode}`));
    return rmResult.exitCode ?? 1;
  }
  console.log(pc.green(`removed ${parsed.delete.length} files from the index and the working tree`));
  return 0;
}

// ---------------------------------------------------------------------------

const cli = cac('trim-tree');
cli.command('export', 'write one build directory\'s records to a folder')
    .option('--build-dir <dir>', 'a generated (ideally built) directory', {default: 'out/Shot'})
    .option('--out <dir>', 'where to write inputs.txt, graph.txt, deps.txt, build.ninja.d, jumbo.txt, meta.json')
    .action(async (options: {buildDir: string; out?: string}) => {
      if (!options.out) throw new Error('--out is required');
      process.exitCode = await exportGraph(options.buildDir, options.out);
    });
cli.command('plan', 'compute the keep and delete lists from one or more exports')
    .option('--graph <dir>', 'an export directory; repeat for every platform')
    .option('--keep <file>', 'extra paths to keep, one per line, a trailing / for a prefix')
    .option('--out <dir>', 'where plan.json and the lists go', {default: 'out/trim'})
    .action(async (options: {graph?: string | string[]; keep?: string; out: string}) => {
      const graphs = options.graph === undefined ? [] : Array.isArray(options.graph) ? options.graph : [options.graph];
      process.exitCode = await plan(graphs, options.out, options.keep);
    });
cli.command('apply', 'git rm the delete list of a plan')
    .option('--plan <file>', 'plan.json from `plan`', {default: 'out/trim/plan.json'})
    .action(async (options: {plan: string}) => {
      process.exitCode = await apply(options.plan);
    });
cli.help();

try {
  cli.parse(process.argv, {run: false});
  if (!cli.matchedCommand && !cli.options.help) {
    cli.outputHelp();
    process.exitCode = 2;
  } else {
    await cli.runMatchedCommand();
  }
} catch (error) {
  console.log(pc.red(`trim-tree: ${error instanceof Error ? error.message : String(error)}`));
  process.exitCode = 2;
}
