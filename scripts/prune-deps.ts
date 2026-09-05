// Prune DEPS, .gitmodules and the gclient hooks to what the engine builds use.
//
// DEPS still lists the 300-odd checkouts of a full Chromium: Android SDKs,
// ChromeOS internal resources, Fuchsia images, test corpora. Most are gated on
// conditions this project never sets, but every one is a line CI's `gclient
// sync` evaluates, a submodule git knows about, and a hook that may run. The
// build graph says which checkouts are read: `trim-tree plan` writes the
// records that name files git does not track (untracked-inputs.txt), and a
// DEPS path is used when one of those records lives under it.
//
// Three things are kept without a record, and the script says so in its
// output: the toolchain (clang, rust, gn, ninja, depot_tools, the sysroots),
// which the records name only indirectly; paths listed in --keep; and the
// hooks named in `keptHooks`, which are the ones that do something on the
// three platforms built here (the others fetch profiles, SDKs and images for
// platforms and bots this repository has no use for).
//
// Usage:
//
//   pnpm prune-deps --inputs out/trim/untracked-inputs.txt [--keep FILE] [--dry-run]
//
// It rewrites DEPS and .gitmodules in place, entry by entry, leaving the
// entries it keeps byte-identical. Vars nothing references any more are left
// alone: they are harmless, and gclient does not mind.
//
// Relative paths are resolved against the repository root.

import {readFile, writeFile} from 'node:fs/promises';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import pc from 'picocolors';

const root = path.resolve(import.meta.dirname, '..');
const resolve = (p: string) => path.resolve(root, p);

// DEPS paths (without the src/ prefix) the records do not name but the build
// cannot run without.
const toolchain = [
  'third_party/depot_tools',
  'third_party/ninja',
  'third_party/llvm-build/Release+Asserts',
  'third_party/rust-toolchain',
  'third_party/libc++/src',
  'third_party/libc++abi/src',
  'third_party/libunwind/src',
  'third_party/compiler-rt/src',
  'third_party/llvm-libc/src',
  'buildtools/win',
  'buildtools/linux64',
  'buildtools/mac',
  'buildtools/win-format',
  'buildtools/linux64-format',
  'buildtools/mac-format',
  'buildtools/mac_arm64-format',
  'build/linux/debian_bullseye_amd64-sysroot',
  'build/linux/debian_bullseye_arm64-sysroot',
  'tools/clang/dsymutil',
];

// gclient hooks that do something for a Windows, Linux or macOS engine build.
const keptHooks = new Set([
  'vpython3_common',
  'landmines',
  'disable_depot_tools_selfupdate',
  'win_toolchain',
  'mac_toolchain',
  'clang',
  'rust',
  'lastchange',
  'lastchange_commit_position_cros',
  'gpu_lists_version',
  'lastchange_skia',
  'dsymutil_mac_arm64',
  'dsymutil_mac_x64',
  'rc_win',
  'rc_mac',
  'rc_linux',
]);

// ---------------------------------------------------------------------------
// A small scanner for Python literal syntax: bracket depth outside strings
// and comments, which is all that is needed to find where an entry ends.

function depthDelta(line: string): number {
  let delta = 0;
  let quote: string | null = null;
  for (let i = 0; i < line.length; i++) {
    const c = line[i];
    if (quote) {
      if (c === '\\') i++;
      else if (c === quote) quote = null;
      continue;
    }
    if (c === '#') break;
    if (c === '\'' || c === '"') quote = c;
    else if (c === '{' || c === '[' || c === '(') delta++;
    else if (c === '}' || c === ']' || c === ')') delta--;
  }
  return delta;
}

interface Entry {
  key: string;       // the DEPS path or hook name
  lines: string[];   // comments and blanks before it, then the entry itself
}

// Split the body of a `name = {` / `name = [` block into entries. An entry
// starts at a line `isStart` recognises and ends on the first line after which
// the bracket depth is back to zero. Comment and blank lines attach to the
// entry that follows them.
function splitEntries(body: string[], isStart: (line: string) => string | null): {entries: Entry[]; trailing: string[]} {
  const entries: Entry[] = [];
  let pending: string[] = [];
  let current: Entry | null = null;
  let depth = 0;
  for (const line of body) {
    if (current) {
      current.lines.push(line);
      depth += depthDelta(line);
      if (depth <= 0) {
        entries.push(current);
        current = null;
        depth = 0;
      }
      continue;
    }
    const key = isStart(line);
    if (key === null) {
      pending.push(line);
      continue;
    }
    current = {key, lines: [...pending, line]};
    pending = [];
    depth = depthDelta(line);
    if (depth <= 0) {
      entries.push(current);
      current = null;
      depth = 0;
    }
  }
  if (current) entries.push(current);
  return {entries, trailing: pending};
}

// Find `<name> = {` ... `}` (or [ ... ]) at column 0.
function findBlock(lines: string[], name: string): {start: number; end: number} {
  const start = lines.findIndex((l) => new RegExp(`^${name} = [\\[{]\\s*$`).test(l));
  if (start < 0) throw new Error(`no "${name} = " block in DEPS`);
  const closer = lines[start].trimEnd().endsWith('{') ? '}' : ']';
  const end = lines.findIndex((l, i) => i > start && l === closer);
  if (end < 0) throw new Error(`unterminated ${name} block`);
  return {start, end};
}

function replaceBlock(lines: string[], block: {start: number; end: number}, body: string[]): string[] {
  return [...lines.slice(0, block.start + 1), ...body, ...lines.slice(block.end)];
}

// ---------------------------------------------------------------------------

async function main(inputsArgs: string[], keepArg: string | undefined, dryRun: boolean): Promise<number> {
  const inputs: string[] = [];
  for (const f of inputsArgs) {
    for (const line of (await readFile(resolve(f), 'utf8')).split(/\r?\n/)) {
      if (line.trim()) inputs.push(line.trim());
    }
  }
  if (inputs.length === 0) {
    console.log(pc.red('no records; pass --inputs out/trim/untracked-inputs.txt'));
    return 2;
  }
  const extraKeep = keepArg ?
      (await readFile(resolve(keepArg), 'utf8')).split(/\r?\n/).map((l) => l.trim()).filter((l) => l && !l.startsWith('#')) :
      [];

  // Fast prefix test: the set of every directory prefix of every record.
  const prefixes = new Set<string>();
  for (const p of inputs) {
    const parts = p.split('/');
    for (let i = 1; i < parts.length; i++) prefixes.add(parts.slice(0, i).join('/'));
    prefixes.add(p);
  }
  const why = (depsPath: string): string | null => {
    const p = depsPath.replace(/^src\//, '');
    if (prefixes.has(p)) return 'read by a build';
    if (toolchain.includes(p)) return 'toolchain';
    if (extraKeep.some((k) => k.endsWith('/') ? p.startsWith(k) || (p + '/').startsWith(k) : k === p)) return '--keep';
    return null;
  };

  const depsFile = resolve('DEPS');
  const original = await readFile(depsFile, 'utf8');
  const eol = original.includes('\r\n') ? '\r\n' : '\n';
  let lines = original.split(/\r?\n/);

  // deps = { ... }
  const depsBlock = findBlock(lines, 'deps');
  const deps = splitEntries(lines.slice(depsBlock.start + 1, depsBlock.end), (l) => /^  '([^']+)':/.exec(l)?.[1] ?? null);
  const keptDeps = new Set<string>();
  const keptBody: string[] = [];
  const dropped: string[] = [];
  for (const e of deps.entries) {
    const reason = why(e.key);
    if (reason) {
      keptDeps.add(e.key);
      keptBody.push(...e.lines);
      console.log(`${pc.green('keep')} ${e.key}  (${reason})`);
    } else {
      dropped.push(e.key);
    }
  }
  keptBody.push(...deps.trailing);
  lines = replaceBlock(lines, depsBlock, keptBody);

  // recursedeps = [ ... ]: only paths still present.
  const rec = findBlock(lines, 'recursedeps');
  const recBody = lines.slice(rec.start + 1, rec.end).filter((l) => {
    const m = /^\s*'([^']+)'/.exec(l);
    return m ? keptDeps.has(m[1]) : false;
  });
  lines = replaceBlock(lines, rec, recBody);

  // hooks = [ ... ]
  const hooksBlock = findBlock(lines, 'hooks');
  const hooks = splitEntries(lines.slice(hooksBlock.start + 1, hooksBlock.end), (l) => (/^  \{\s*$/.test(l) ? '{' : null));
  const hookBody: string[] = [];
  const droppedHooks: string[] = [];
  for (const e of hooks.entries) {
    const name = /'name':\s*'([^']+)'/.exec(e.lines.join('\n'))?.[1] ?? '(unnamed)';
    if (keptHooks.has(name)) hookBody.push(...e.lines);
    else droppedHooks.push(name);
  }
  hookBody.push(...hooks.trailing);
  lines = replaceBlock(lines, hooksBlock, hookBody);

  // .gitmodules: sections whose path is a kept git dep.
  const modulesFile = resolve('.gitmodules');
  const modules = (await readFile(modulesFile, 'utf8')).split(/\r?\n/);
  const sections: string[][] = [];
  for (const line of modules) {
    if (/^\[submodule /.test(line) || sections.length === 0) sections.push([line]);
    else sections[sections.length - 1].push(line);
  }
  const keptSections = sections.filter((s) => {
    const p = s.map((l) => /^\s*path = (.+)$/.exec(l)?.[1]).find(Boolean);
    return p ? keptDeps.has('src/' + p.trim()) : true;
  });
  const droppedModules = sections.length - keptSections.length;

  // Gitlinks: a git dep is also an index entry of mode 160000. One whose DEPS
  // entry goes, or that has no DEPS entry at all, leaves the index too, so
  // git and gclient keep telling the same story.
  const {stdout: indexEntries} = await execa('git', ['--no-optional-locks', 'ls-files', '-s', '-z'], {cwd: root, maxBuffer: 1 << 28});
  const gitlinks = indexEntries.split('\0').filter((e) => e.startsWith('160000 ')).map((e) => e.slice(e.indexOf('\t') + 1));
  const staleGitlinks = gitlinks.filter((p) => !keptDeps.has('src/' + p));

  console.log(`\nDEPS: ${deps.entries.length} entries, keep ${keptDeps.size}, drop ${dropped.length}`);
  console.log(`gitlinks: ${gitlinks.length}, remove ${staleGitlinks.length}`);
  console.log(`hooks: ${hooks.entries.length}, keep ${hookBody.length ? hooks.entries.length - droppedHooks.length : 0}, drop ${droppedHooks.length}: ${droppedHooks.join(', ')}`);
  console.log(`.gitmodules: ${sections.length} sections, drop ${droppedModules}`);
  const orphanModules = keptSections.map((s) => s.map((l) => /^\s*path = (.+)$/.exec(l)?.[1]).find(Boolean)).filter(Boolean);
  const missing = [...keptDeps].filter((k) => !orphanModules.includes(k.replace(/^src\//, '')));
  console.log(`kept DEPS entries without a .gitmodules section (gcs/cipd, expected): ${missing.length}`);

  if (dryRun) {
    console.log(pc.yellow('\n--dry-run: nothing written'));
    return 0;
  }
  await writeFile(depsFile, lines.join(eol));
  await writeFile(modulesFile, keptSections.map((s) => s.join('\n')).join('\n'));
  if (staleGitlinks.length > 0) {
    // --cached: the index entry goes, whatever is on disk stays.
    await execa('git', ['rm', '-q', '--cached', '--', ...staleGitlinks], {cwd: root, stdio: 'inherit'});
  }
  console.log(pc.green(`\nwrote DEPS and .gitmodules, removed ${staleGitlinks.length} gitlinks from the index`));
  return 0;
}

const cli = cac('prune-deps');
cli.command('', 'prune DEPS, hooks and .gitmodules to what the build graph reads')
    .option('--inputs <file>', 'untracked-inputs.txt from `trim-tree plan`; repeat to union')
    .option('--keep <file>', 'extra DEPS paths to keep, one per line, a trailing / for a prefix')
    .option('--dry-run', 'report only')
    .action(async (options: {inputs?: string | string[]; keep?: string; dryRun?: boolean}) => {
      const inputs = options.inputs === undefined ? [] : Array.isArray(options.inputs) ? options.inputs : [options.inputs];
      process.exitCode = await main(inputs, options.keep, options.dryRun === true);
    });
cli.help();
try {
  cli.parse(process.argv, {run: false});
  if (!cli.options.help) await cli.runMatchedCommand();
} catch (error) {
  console.log(pc.red(`prune-deps: ${error instanceof Error ? error.message : String(error)}`));
  process.exitCode = 2;
}
