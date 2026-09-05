// Find file entries in GN lists whose files no longer exist.
//
// `gn gen` does not check that the paths in `sources`, `inputs`, `public` or
// `data` exist -- only ninja does, at build time, one missing file per run.
// After a large deletion that is dozens of rounds, each one a full `gn gen`
// plus a ninja start-up.
//
// This resolves every literal path in those lists and reports the ones that
// are gone. Entries that are obviously not source paths are skipped: anything
// containing `$` (generated), anything with a `:` (a label), directory
// entries in `data`, and anything inside a `#` comment.
//
// It does not evaluate conditionals, so an entry inside `if (is_linux)` is
// reported even though ninja never reaches it: the output is a candidate list
// rather than a defect list. It only sees string literals, so
// `sources = [ crate_root ]` is invisible to it.
//
// Only the GN files that `gn gen` actually loaded are examined. That list is
// in <out>/build.ninja.d, and using it is what keeps the result honest: a
// full-tree scan reports ten thousand "missing" entries, almost all of them
// in BUILD.gn files for dependencies that are simply not checked out on this
// platform, which GN never reads and ninja never cares about.
//
//   pnpm gn:missing-sources <out-dir> [--delete] [--verbose]

import {existsSync, readdirSync, readFileSync, statSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';

import {resolve, root} from './lib/repo.ts';

const LISTS = [
  'sources', 'inputs', 'public', 'data', 'filelist',
  // mojom()'s cpp_typemaps, which is how a deleted component's mojo traits
  // stay referenced from a surviving BUILD.gn.
  'traits_sources', 'traits_headers',
  // Blink's code generators take their json5 inputs as in_files or
  // json_inputs depending on the template, so a deleted core subdirectory
  // leaves one of these behind rather than a `sources` entry.
  'in_files', 'json_inputs', 'templates',
];
// `sources = [` ... `]`, including the single-line `sources += [ "a.cc" ]` form.
// The body must exclude brackets: requiring the closing `]` to be alone on its
// own line means a single-line list never terminates the match.
const BLOCK = new RegExp(`^[ \\t]*(?:${LISTS.join('|')})[ \\t]*\\+?=[ \\t]*\\[([^\\[\\]]*)\\]`, 'gms');
const ENTRY = /"([^"]+)"/g;
const COMMENT = /#[^\n]*/g;
const escape = (s: string) => s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

function resolveEntry(entry: string, buildDir: string): string {
  return entry.startsWith('//') ? path.join(root, entry.slice(2)) : path.join(buildDir, entry);
}

// Every file name below a BUILD.gn's directory, three levels deep. Not every
// template resolves relative paths against the directory of the BUILD.gn that
// invokes it: aggregate_vector_icons resolves against its own
// `icon_directory`, so ui/views/BUILD.gn lists "check.icon" for a file that
// lives in ui/views/vector_icons/.
const basenamesCache = new Map<string, Set<string>>();
function basenamesUnder(buildDir: string): Set<string> {
  const cached = basenamesCache.get(buildDir);
  if (cached) return cached;
  const names = new Set<string>();
  const walk = (dir: string, depth: number) => {
    let entries: string[];
    try {
      entries = readdirSync(dir);
    } catch {
      return;
    }
    for (const name of entries) {
      const full = path.join(dir, name);
      let isDir = false;
      try {
        isDir = statSync(full).isDirectory();
      } catch {
        continue;
      }
      if (isDir) {
        if (depth < 3) walk(full, depth + 1);
      } else {
        names.add(name);
      }
    }
  };
  walk(buildDir, 0);
  basenamesCache.set(buildDir, names);
  return names;
}

// The .gn/.gni files gn gen read, from <out>/build.ninja.d.
function loadedGnFiles(outDir: string): string[] {
  const text = readFileSync(path.join(outDir, 'build.ninja.d'), 'utf8');
  const deps = text.slice(text.indexOf(':') + 1);
  const files: string[] = [];
  for (const raw of deps.replace(/\\\r?\n/g, ' ').split(/\s+/)) {
    const token = raw.replace(/\\ /g, ' ');
    if (token.endsWith('.gn') || token.endsWith('.gni')) files.push(path.normalize(path.join(outDir, token)));
  }
  return files;
}

function main(outArg: string, del: boolean, verbose: boolean): number {
  const outDir = resolve(outArg);
  const missing = new Map<string, string[]>();
  for (const fp of loadedGnFiles(outDir)) {
    // Only BUILD.gn. A relative path inside a .gni resolves against the
    // directory of whichever BUILD.gn imported it, which this script has no
    // way to know, so every such entry would be reported wrongly.
    if (path.basename(fp) !== 'BUILD.gn') continue;
    let src: string;
    try {
      // Universal newlines, as the Python original read (and wrote) them.
      src = readFileSync(fp, 'utf8').replace(/\r\n/g, '\n');
    } catch {
      continue;
    }
    const dirpath = path.dirname(fp);
    const gone: string[] = [];
    for (const block of src.replace(COMMENT, '').matchAll(BLOCK)) {
      for (const [, entry] of block[1].matchAll(ENTRY)) {
        if (entry.includes('$') || entry.includes(':') || entry.endsWith('/')) continue;
        // Must look like a path: `get_label_info(invoker.target, "dir")` puts
        // "dir" in a sources list, and deleting it leaves a trailing comma
        // inside the call.
        if (!entry.includes('/') && !entry.includes('.')) continue;
        if (existsSync(resolveEntry(entry, dirpath))) continue;
        if (basenamesUnder(dirpath).has(path.basename(entry))) continue;  // A template resolved it against another base.
        gone.push(entry);
      }
    }
    if (!gone.length) continue;
    const rel = path.relative(root, fp).replace(/\\/g, '/');
    missing.set(rel, gone);
    if (del) {
      let out = src;
      for (const entry of new Set(gone)) {
        const esc = escape(entry);
        // Entry alone on its line, then inside a single-line list.
        out = out.replace(new RegExp(`^[ \\t]*"${esc}",?[ \\t]*\\n`, 'gm'), '');
        out = out.replace(new RegExp(`"${esc}"[ \\t]*,?[ \\t]*`, 'g'), '');
      }
      // Tidy what removal can leave: `[  ]` and a dangling comma.
      out = out.replace(/\[[ \t]*\]/g, '[]').replace(/,[ \t]*\]/g, ' ]');
      // An emptied list is often an invalid target rather than a smaller one:
      // an action with no inputs has no outputs, and aggregate_vector_icons
      // turns its list into a response file. Say which targets to look at.
      const emptied = (out.match(/\[[ \t]*\]/g) ?? []).length - (src.match(/\[[ \t]*\]/g) ?? []).length;
      if (emptied > 0) console.log(`     ^ emptied ${emptied} list(s) here -- check those targets`);
      writeFileSync(fp, out);
    }
  }
  for (const rel of [...missing.keys()].sort((a, b) => missing.get(b)!.length - missing.get(a)!.length)) {
    console.log(`${String(missing.get(rel)!.length).padStart(4)}  ${rel}`);
    if (verbose) for (const entry of missing.get(rel)!) console.log(`        ${entry}`);
  }
  const total = [...missing.values()].reduce((s, v) => s + v.length, 0);
  console.log(`---- ${total} missing entr(ies) in ${missing.size} file(s)${del ? ', deleted' : ''}`);
  return 0;
}

const argv = process.argv.slice(2);
const del = argv.includes('--delete'), verbose = argv.includes('--verbose');
const cli = cac('gn-missing-sources');
cli.command('<out>', 'list GN source-list entries whose files no longer exist')
    .option('--delete', 'remove the entries from the BUILD.gn files')
    .option('--verbose', 'list every entry, not just a count per file')
    .action((out: string) => {
      process.exitCode = main(out, del, verbose);
    });
cli.help();
cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '--delete' && a !== '--verbose')]);
