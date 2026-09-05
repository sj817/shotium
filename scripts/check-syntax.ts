// Syntax-check individual translation units without building anything.
//
// The edit/build loop for this cut was costing tens of minutes per round, and
// almost all of that was code generation and linking for a binary that was
// not going to link anyway. Every error we are chasing is a *front-end* error:
// a missing declaration, a dangling include, a member that no longer exists.
// Those are all decided before the back end runs.
//
// So: pull the exact compile command out of ninja's compdb, add -fsyntax-only,
// and run it. No object file, no optimizer, no linker. A core/ TU that takes
// ~40s to compile syntax-checks in ~8s, and twelve run at once.
//
//   pnpm check:syntax path/to/foo.cc [more.cc ...]
//   pnpm check:syntax --from-log out/Shot/build.log
//   pnpm check:syntax --dir third_party/blink/renderer/core/frame
//
// --from-log re-checks exactly the TUs that failed in a previous ninja run,
// which is the normal way to use this: build once with -k 0 to get the full
// failure set, then iterate here until it is empty, and only then build again.
//
// The compdb is out/Shot/ccdb.json (ninja -C out/Shot -t compdb cxx > ...);
// its parsed index is cached beside it and invalidated by mtime. Relative
// paths are resolved against the repository root.

import {existsSync, readFileSync, statSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import pLimit from 'p-limit';

import {resolve, root} from './lib/repo.ts';

const OUT = resolve('out/Shot');
const CCDB = path.join(OUT, 'ccdb.json');
const INDEX = path.join(OUT, 'ccdb.index.json');

const norm = (p: string) => path.normalize(p).replace(/\\/g, '/').toLowerCase();

// Map normalised source path -> compile command. The compdb is ~170 MB;
// parsing it costs seconds, so the derived index is cached and keyed on mtime.
function loadIndex(): Record<string, string> {
  if (!existsSync(CCDB)) {
    throw new Error(`no ${CCDB}; run: ninja -C out/Shot -t compdb cxx > out/Shot/ccdb.json`);
  }
  const stamp = statSync(CCDB).mtimeMs;
  if (existsSync(INDEX)) {
    const cached = JSON.parse(readFileSync(INDEX, 'utf8')) as {stamp: number; index: Record<string, string>};
    if (cached.stamp === stamp) return cached.index;
  }
  const entries = JSON.parse(readFileSync(CCDB, 'utf8')) as Array<{file: string; directory?: string; command: string}>;
  const index: Record<string, string> = {};
  for (const entry of entries) {
    const src = path.isAbsolute(entry.file) ? entry.file : path.join(entry.directory ?? OUT, entry.file);
    // First writer wins: a source compiled into several targets (core and
    // core_hot, say) gets checked once, and the flags that matter here --
    // include paths and defines -- are the same in both.
    const key = norm(src);
    if (!(key in index)) index[key] = entry.command;
  }
  writeFileSync(INDEX, JSON.stringify({stamp, index}));
  return index;
}

// Split a Windows command line into argv, by the real MSVCRT rules: a
// backslash is literal except immediately before a quote, where 2n backslashes
// plus a quote give n backslashes and toggle quoting, and 2n+1 give n
// backslashes and a literal quote. Approximating this is not safe here because
// at least one argument depends on the escaped form --
//     "-DSK_USER_CONFIG_HEADER=\"../../skia/config/SkUserConfig.h\""
// whose value must still carry its quotes when the preprocessor sees it.
export function tokenize(command: string): string[] {
  const argv: string[] = [];
  let current = '', inQuote = false, started = false;
  for (let i = 0; i < command.length;) {
    const ch = command[i];
    if (ch === '\\') {
      let slashes = 0;
      while (i < command.length && command[i] === '\\') {
        slashes++;
        i++;
      }
      if (i < command.length && command[i] === '"') {
        current += '\\'.repeat(Math.floor(slashes / 2));
        if (slashes % 2) current += '"';
        else inQuote = !inQuote;
        started = true;
        i++;
      } else {
        current += '\\'.repeat(slashes);
        started = true;
      }
      continue;
    }
    if (ch === '"') {
      inQuote = !inQuote;
      started = true;
    } else if (/\s/.test(ch) && !inQuote) {
      if (started) {
        argv.push(current);
        current = '';
        started = false;
      }
    } else {
      current += ch;
      started = true;
    }
    i++;
  }
  if (started) argv.push(current);
  return argv;
}

function syntaxOnly(command: string): string[] {
  const out: string[] = [];
  for (const tok of tokenize(command)) {
    // /Fo names the object file we do not want; /showIncludes prints a
    // dependency list nobody reads here; /c means "compile, don't link",
    // unused with -fsyntax-only, and this build makes an unused argument an
    // error.
    if (tok.startsWith('/Fo') || tok.startsWith('/showIncludes') || tok === '/c') continue;
    out.push(tok);
  }
  out.push('-fsyntax-only');
  return out;
}

async function checkOne(source: string, command: string): Promise<[string, number, string]> {
  const argv = syntaxOnly(command);
  // CreateProcess resolves a relative executable against the *calling*
  // process's directory, not against the cwd, so the compdb's
  // ..\..\third_party\llvm-build\... has to be made absolute here.
  if (!path.isAbsolute(argv[0])) argv[0] = path.normalize(path.join(OUT, argv[0]));
  const proc = await execa(argv[0], argv.slice(1), {cwd: OUT, reject: false, all: true});
  return [source, proc.exitCode ?? 1, proc.all ?? ''];
}

// ninja writes "FAILED: [code=1] obj/foo/bar.obj"; the exit-code prefix is not
// always there, and older logs have neither it nor the obj/ prefix.
const FAILED_OBJ = /^FAILED: (?:\[code=\d+\] )?(?:obj\/)?(\S+\.obj)/gm;

// The source files behind a ninja log's FAILED: lines, looked up in the
// compdb by their /Fo object rather than by guessing the path mapping.
function sourcesFromLog(log: string, index: Record<string, string>): string[] {
  const text = readFileSync(log, 'utf8');
  const objs = new Set([...text.matchAll(FAILED_OBJ)].map((m) => m[1]));
  if (objs.size === 0) return [];
  const byObj = new Map<string, string>();
  for (const [src, command] of Object.entries(index)) {
    const m = /\/Fo(\S+\.obj)/.exec(command);
    if (m) byObj.set(m[1], src);
  }
  const found: string[] = [], missing: string[] = [];
  for (const obj of [...objs].sort()) {
    const src = byObj.get(obj) ?? byObj.get('obj/' + obj);
    if (src) found.push(src);
    else missing.push(obj);
  }
  if (missing.length) {
    console.log(`${missing.length} failed object(s) are no longer in the build graph (target deleted or renamed):`);
    for (const obj of missing.slice(0, 10)) console.log(`    ${obj}`);
  }
  return found;
}

async function main(sources: string[], opts: {fromLog?: string; dir?: string; j: number; lines: number}): Promise<number> {
  const index = loadIndex();
  let wanted: string[] = [];
  for (const source of sources) {
    const key = norm(path.isAbsolute(source) ? source : path.join(root, source));
    if (key in index) wanted.push(key);
    else console.log(`not in the build graph: ${source}`);
  }
  if (opts.fromLog) wanted.push(...sourcesFromLog(resolve(opts.fromLog), index));
  if (opts.dir) {
    const prefix = norm(path.join(root, opts.dir)) + '/';
    wanted.push(...Object.keys(index).filter((k) => k.startsWith(prefix)));
  }
  wanted = [...new Set(wanted)].sort();
  if (wanted.length === 0) {
    console.log('nothing to check');
    return 2;
  }

  console.log(`checking ${wanted.length} translation unit(s) with -j ${opts.j}\n`);
  const limit = pLimit(opts.j);
  const bad: string[] = [];
  await Promise.all(wanted.map((s) => limit(async () => {
    const [source, code, output] = await checkOne(s, index[s]);
    if (code === 0) return;
    bad.push(source);
    const rel = path.relative(norm(root), source).replace(/\\/g, '/');
    const lines = output.split(/\r?\n/).filter((l) => l.trim());
    const shown = [`=== ${rel}`, ...lines.slice(0, opts.lines).map((l) => `  ${l}`)];
    if (lines.length > opts.lines) shown.push(`  ... ${lines.length - opts.lines} more line(s)`);
    console.log(shown.join('\n') + '\n');
  })));
  console.log(`${wanted.length - bad.length}/${wanted.length} clean, ${bad.length} failing`);
  return bad.length ? 1 : 0;
}

const cli = cac('check-syntax');
cli.command('[...sources]', 'syntax-check translation units from the compdb, without building')
    .option('--from-log <log>', 're-check the TUs that failed in this ninja log')
    .option('--dir <dir>', 'check every TU under this directory')
    .option('-j <n>', 'parallelism (12 is the OOM ceiling on the development host)', {default: 12})
    .option('--lines <n>', 'error lines to show per file', {default: 12})
    .action(async (sources: string[], options: {fromLog?: string; dir?: string; j: number; lines: number}) => {
      try {
        process.exitCode = await main(sources, {...options, j: Number(options.j), lines: Number(options.lines)});
      } catch (error) {
        console.log(error instanceof Error ? error.message : String(error));
        process.exitCode = 2;
      }
    });
cli.help();
cli.parse();
