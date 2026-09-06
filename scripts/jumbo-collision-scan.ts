// Find Jumbo symbol collisions before the compiler does.
//
//   pnpm jumbo:collisions out/ProbeLinux out/Shot
//
// Jumbo concatenates translation units, so two files that each define the
// same internal-linkage name -- a file-scope static in C, a type or function
// inside an anonymous namespace in C++ -- stop being separate and the second
// becomes a redefinition of the first. Which files share a chunk follows the
// source list, so the same latent collision fires on one platform and not
// another. Several turned up on Linux one CI round at a time, twenty to
// thirty minutes each, none of them visible from a Windows build.
//
// The second argument is a build directory for a platform that is known to
// compile, and it is what makes the output short enough to read. Every pair
// of files sharing a chunk there is already proved not to collide, so only
// pairs the target platform newly puts together can be a problem. Without it
// the scan reports around a hundred candidates that Windows compiles every
// day.
//
// Candidates are also limited to translation units the binary actually
// compiles. In a Jumbo build `ninja -t inputs` names the generated .cc files
// rather than the original sources, so that is the form to match.
//
// What it cannot tell you, both learned by checking its output against the
// source: it compares names, not signatures, so legal overloads look like
// collisions (read the declarations before acting); and it reads symbols,
// not macros. Files under a DEPS checkout that this host does not have are
// counted and named rather than passed over silently.

import {existsSync, readdirSync, readFileSync, statSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execaSync} from 'execa';

import {resolve, root} from './lib/repo.ts';

const INCLUDE = /^\s*#include\s+"([^"]+)"/gm;
// A static function or variable at file scope. Both the one-line form and the
// K&R-ish form fontconfig uses, where the type is on its own line.
const STATIC_DEF = /^static\s+(?:[A-Za-z_][\w:<>,\s*&]*?)\s*\**\s*([A-Za-z_]\w*)\s*[([=;]/gm;
const ANON_OPEN = /^namespace\s*\{\s*$/gm;
const ANON_CLOSE = /^\}\s*\/\/\s*namespace\s*$/gm;
// Inside an anonymous namespace: types, and functions declared as
// "<type> <name>(". The type must end in real whitespace, so static_assert(
// and BASE_FEATURE( are not read as a type "s" and a name "tatic_assert", and
// the name must not be preceded by ::, so Class::method defined out of line
// is a member function and not an internal-linkage name.
const ANON_NAME = /^(?:struct|class|union|enum(?:\s+class)?)\s+([A-Za-z_]\w*)|^[A-Za-z_][\w:<>,\s*&]*?\s+\**\s*(?<![:\w])([A-Za-z_]\w*)\s*\(/gm;
const NOT_NAMES = new Set(['if', 'for', 'while', 'switch', 'return', 'sizeof', 'return_type']);

// Internal-linkage names this file defines, as best a regex can tell.
function namesDefined(file: string): Set<string> | null {
  let text: string;
  try {
    text = readFileSync(file, 'utf8').replace(/\r\n/g, '\n');
  } catch {
    return null;
  }
  const found = new Set<string>();
  for (const m of text.matchAll(STATIC_DEF)) found.add(m[1]);
  // The two-line form: "static void\nfree_lock (void)".
  const lines = text.split('\n');
  for (let i = 0; i < lines.length - 1; i++) {
    if (/^static\s+[\w\s*]+$/.test(lines[i])) {
      const m = /^\s*\**\s*([A-Za-z_]\w*)\s*\(/.exec(lines[i + 1]);
      if (m) found.add(m[1]);
    }
  }
  // Anonymous namespace blocks.
  for (const m of text.matchAll(ANON_OPEN)) {
    const start = m.index! + m[0].length;
    ANON_CLOSE.lastIndex = start;
    const close = ANON_CLOSE.exec(text);
    const block = close ? text.slice(start, close.index) : text.slice(start);
    for (const n of block.matchAll(ANON_NAME)) {
      const name = n[1] || n[2];
      if (name && !NOT_NAMES.has(name)) found.add(name);
    }
  }
  return found;
}

function ninjaBinary(): string {
  const local = resolve('third_party/ninja', process.platform === 'win32' ? 'ninja.exe' : 'ninja');
  return existsSync(local) ? local : '';
}

// The Jumbo TUs the target actually compiles, build-dir relative.
function compiledTus(outDir: string, target = 'shot'): Set<string> | null {
  const ninja = ninjaBinary();
  if (!ninja) return null;
  const r = execaSync(ninja, ['-C', outDir, '-t', 'inputs', target], {cwd: root, reject: false, maxBuffer: 1 << 28});
  if (r.exitCode !== 0) return null;
  return new Set(r.stdout.split('\n').map((l) => l.trim().replace(/\\/g, '/')).filter((l) => l.includes('_shot_jumbo_')));
}

function* jumboFiles(gen: string): Generator<string> {
  const stack = [gen];
  while (stack.length) {
    const dir = stack.pop()!;
    let entries: string[];
    try {
      entries = readdirSync(dir);
    } catch {
      continue;
    }
    for (const name of entries) {
      const full = path.join(dir, name);
      if (statSync(full).isDirectory()) stack.push(full);
      else if (name.includes('_shot_jumbo_') && /\.(cc|c|mm)$/.test(name)) yield full;
    }
  }
}

function membersOf(text: string): string[] {
  const members: string[] = [];
  for (const m of text.matchAll(INCLUDE)) {
    // Written as "../../base/foo.cc": -I../.. from the build directory is the
    // source root, so what is left after the ../../ is repository-relative.
    let rel = m[1];
    while (rel.startsWith('../')) rel = rel.slice(3);
    members.push(rel);
  }
  return members;
}

// Every unordered pair of files that share a chunk in this build dir.
function pairsOf(outDir: string): Set<string> {
  const seen = new Set<string>();
  for (const file of jumboFiles(path.join(outDir, 'gen'))) {
    let text: string;
    try {
      text = readFileSync(file, 'utf8');
    } catch {
      continue;
    }
    const mem = membersOf(text).sort();
    for (let i = 0; i < mem.length; i++) for (let j = i + 1; j < mem.length; j++) seen.add(`${mem[i]}\0${mem[j]}`);
  }
  return seen;
}

function main(outArg: string, baselineArg: string): number {
  const outDir = resolve(outArg), baseline = resolve(baselineArg);
  const wanted = compiledTus(outDir);
  if (wanted === null) console.log('could not ask ninja what is compiled; scanning every chunk');
  else console.log(`jumbo TUs the target compiles: ${wanted.size}`);

  const chunks: Array<[string, string[]]> = [];
  for (const full of jumboFiles(path.join(outDir, 'gen'))) {
    if (wanted !== null && !wanted.has(path.relative(outDir, full).replace(/\\/g, '/'))) continue;
    let text: string;
    try {
      text = readFileSync(full, 'utf8');
    } catch {
      continue;
    }
    const members = membersOf(text).map((rel) => path.join(root, rel));
    if (members.length > 1) chunks.push([path.relative(root, full).replace(/\\/g, '/'), members]);
  }
  console.log(`jumbo translation units with more than one member: ${chunks.length}`);
  const baselinePairs = existsSync(baseline) ? pairsOf(baseline) : new Set<string>();
  console.log(`pairs already proved safe by ${baselineArg}: ${baselinePairs.size}`);

  const unreadable = new Map<string, number>();
  const collisions: Array<[string, string, string[]]> = [];
  for (const [tu, members] of chunks) {
    const perFile = new Map<string, Set<string>>();
    for (const m of members) {
      const names = namesDefined(m);
      if (names === null) {
        const rel = path.relative(root, m).replace(/\\/g, '/');
        const key = rel.split('/').slice(0, 3).join('/');
        unreadable.set(key, (unreadable.get(key) ?? 0) + 1);
        continue;
      }
      perFile.set(m, names);
    }
    const seen = new Map<string, string[]>();
    for (const [file, names] of perFile) for (const n of names) seen.set(n, [...(seen.get(n) ?? []), file]);
    for (const [n, files] of seen) {
      if (files.length > 1) collisions.push([tu, n, files.map((p) => path.relative(root, p).replace(/\\/g, '/')).sort()]);
    }
  }
  if (unreadable.size) {
    console.log('\nfiles not on this host (DEPS checkouts) -- not scanned:');
    for (const [d, n] of [...unreadable.entries()].sort()) console.log(`  ${d.padEnd(44)} ${n} file(s)`);
  }
  console.log(`\n=== candidate collisions: ${collisions.length} ===`);
  const byTu = new Map<string, Array<[string, string[]]>>();
  for (const [tu, name, files] of collisions) byTu.set(tu, [...(byTu.get(tu) ?? []), [name, files]]);
  for (const tu of [...byTu.keys()].sort()) {
    console.log(`\n${tu}`);
    const items = byTu.get(tu)!.sort((a, b) => a[0].localeCompare(b[0]));
    for (const [name, files] of items.slice(0, 12)) console.log(`    ${name.padEnd(34)} ${files.map((p) => p.split('/').pop()).join(' + ')}`);
    if (items.length > 12) console.log(`    ... and ${items.length - 12} more`);
  }
  return 0;
}

const cli = cac('jumbo-collision-scan');
cli.command('[out] [baseline]', 'internal-linkage names two files in one jumbo chunk both define')
    .action((out: string | undefined, baseline: string | undefined) => {
      process.exitCode = main(out ?? 'out/ProbeLinux', baseline ?? 'out/Shot');
    });
cli.help();
cli.parse();
