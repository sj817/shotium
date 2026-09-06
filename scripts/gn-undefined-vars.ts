// Find GN globals that are read but never assigned anywhere in the tree.
//
// Deleting a `.gni` takes its `declare_args()` names with it, and `gn gen`
// reports exactly one undefined identifier per run. This approximates the
// whole set in one pass: collect every name assigned at any point in any
// surviving GN file, then report names that are only ever read.
//
// The approximation is one-sided on purpose. It ignores scoping, so a name
// that is only ever a local or a template parameter looks defined and is
// skipped -- this under-reports rather than inventing work. Names it does
// report are read somewhere and assigned nowhere, which is always a real
// break.
//
//   pnpm gn:undefined-vars [dir ...]     # default: the whole tree minus vendored repos

import {readFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {globSync} from 'tinyglobby';

import {resolve, root} from './lib/repo.ts';

// Same rationale as gn-dangling-imports: these resolve `//` against their own
// root and are not loaded by Chromium's gn.
const SKIP_TREES = [
  'third_party/angle', 'third_party/skia', 'third_party/crashpad', 'third_party/mini_chromium', 'third_party/OpenCL-CTS',
  'third_party/clspv', 'third_party/swiftshader', 'third_party/fuchsia-sdk', 'third_party/perfetto',
];

const ASSIGN = /^[ \t]*([a-z_][a-z0-9_]*)[ \t]*(?:=|\+=|-=)/gm;
const EXPAND = /\$([a-z_][a-z0-9_]*)/g;
const COND = /\b(?:if|assert)[ \t]*\([ \t]*!?([a-z_][a-z0-9_]*)[ \t]*[,)&|]/g;
// `sources += blink_core_sources_bindings` -- a bare identifier on the right of
// an assignment, which is how the per-directory source lists are pulled in.
const RHS = /^[ \t]*[a-z_][a-z0-9_]*[ \t]*(?:\+=|=)[ \t]*([a-z_][a-z0-9_]*)[ \t]*$/gm;
// The same read hiding in a list: `inputs = [ web_idl_database_filepath ]`, a
// bare identifier on its own line inside one, or `inputs = shared_list + [`.
const LIST = /\[[ \t]*([a-z_][a-z0-9_]*)[ \t]*\]|^[ \t]+([a-z_][a-z0-9_]*),[ \t]*$|(?:\+=|=)[ \t]*([a-z_][a-z0-9_]*)[ \t]*\+/gm;
// And a fourth: passed to a builtin, as core/BUILD.gn does for every one of
// its per-directory source lists -- `rebase_path(blink_core_sources_css, "", "css")`.
const CALL = /\b(?:rebase_path|get_path_info|filter_include|filter_exclude|string_join|string_split|foreach)\([ \t]*\n?[ \t]*([a-z_][a-z0-9_]*)[ \t]*[,)]/g;
// GN builtins and target-scope variables that are never assigned at top level.
const BUILTIN = new Set(`
true false current_cpu current_os current_toolchain default_toolchain
host_cpu host_os root_build_dir root_gen_dir root_out_dir target_cpu target_os
target_gen_dir target_out_dir python_path invoker target_name defined rebase_path
`.split(/\s+/).filter(Boolean));

function main(dirs: string[]): number {
  const roots = dirs.length ? dirs.map((d) => resolve(d)) : [root];
  const assigned = new Set<string>();
  const read = new Map<string, Set<string>>();
  for (const base of roots) {
    // BUILD.gn, .gni, and BUILDCONFIG.gn (which defines is_win and the rest of
    // the platform booleans). Every other bare `.gn` is an args file, where an
    // assignment overrides a declare_args default but does not put a new name
    // in scope for anyone.
    const files = globSync(['**/BUILD.gn', '**/BUILDCONFIG.gn', '**/*.gni'], {cwd: base, absolute: true, ignore: ['**/.git/**', 'out/**', 'out*/**', '**/depot_tools/**']});
    for (const fp of files) {
      const rel = path.relative(root, fp).replace(/\\/g, '/');
      let src: string;
      try {
        src = readFileSync(fp, 'utf8').replace(/\r\n/g, '\n');
      } catch {
        continue;
      }
      for (const m of src.matchAll(ASSIGN)) assigned.add(m[1]);
      // Vendored trees still *define* names that Chromium files read (ANGLE's
      // angle.gni is the common case), so they count on the assignment side.
      // Only their own reads are ignored.
      if (SKIP_TREES.some((t) => rel.startsWith(t + '/'))) continue;
      const names = new Set<string>();
      for (const re of [EXPAND, COND, RHS, CALL]) for (const m of src.matchAll(re)) names.add(m[1]);
      for (const m of src.matchAll(LIST)) for (const g of m.slice(1)) if (g) names.add(g);
      for (const name of names) read.set(name, (read.get(name) ?? new Set()).add(rel));
    }
  }
  const undefined_ = [...read.entries()].filter(([n]) => !assigned.has(n) && !BUILTIN.has(n));
  undefined_.sort((a, b) => b[1].size - a[1].size);
  for (const [name, set] of undefined_) {
    const files = [...set].sort();
    console.log(`${String(files.length).padStart(3)}  ${name.padEnd(42)} ${files[0]}`);
    for (const f of files.slice(1, 6)) console.log(`     ${''.padEnd(42)} ${f}`);
    if (files.length > 6) console.log(`     ${''.padEnd(42)} ... ${files.length - 6} more`);
  }
  console.log(`---- ${undefined_.length} name(s) read but never assigned`);
  return 0;
}

const cli = cac('gn-undefined-vars');
cli.command('[...dirs]', 'GN names read somewhere and assigned nowhere')
    .action((dirs: string[]) => {
      process.exitCode = main(dirs);
    });
cli.help();
cli.parse();
