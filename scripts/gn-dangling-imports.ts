// List every `import("//...")` in the tree whose target file no longer exists.
//
// GN reports one load error per `gn gen` run, so converging by re-running gn
// is one 25-second round per dangling import. This finds all of them at once.
//
//   pnpm gn:dangling-imports            # report
//   pnpm gn:dangling-imports --delete   # also delete the import lines

import {existsSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {globSync} from 'tinyglobby';

import {root} from './lib/repo.ts';

// Vendored projects that also build standalone. Their BUILD files resolve `//`
// against their own root, so an import that looks dangling from here is fine
// there, and Chromium's `gn gen` never loads those files anyway -- if it did,
// GN would already be erroring on them.
const SKIP_TREES = [
  'third_party/angle', 'third_party/skia', 'third_party/crashpad', 'third_party/mini_chromium', 'third_party/OpenCL-CTS',
  'third_party/clspv', 'third_party/swiftshader', 'third_party/fuchsia-sdk',
];
// `gn format` wraps a long path onto its own line, so the open paren and the
// string are not always on the same line.
const IMPORT = /^[ \t]*import\(\s*"(\/\/[^"]+)"\)\n/gm;
const escape = (s: string) => s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

function main(del: boolean): number {
  const missing = new Map<string, string[]>();
  const files = globSync(['**/BUILD.gn', '**/*.gni'], {cwd: root, absolute: true, ignore: ['**/.git/**', 'out/**', 'out*/**', '**/depot_tools/**', ...SKIP_TREES.map((t) => `${t}/**`)]});
  for (const fp of files) {
    let src: string;
    try {
      // Universal newlines, as Python's text mode read them: the working copy
      // is CRLF on Windows and the patterns end at a line.
      src = readFileSync(fp, 'utf8').replace(/\r\n/g, '\n');
    } catch {
      continue;
    }
    const gone = [...src.matchAll(IMPORT)].map((m) => m[1]).filter((label) => !existsSync(path.join(root, label.slice(2))));
    if (!gone.length) continue;
    const rel = path.relative(root, fp).replace(/\\/g, '/');
    missing.set(rel, gone);
    if (del) {
      let out = src;
      for (const label of new Set(gone)) out = out.replace(new RegExp(`^[ \\t]*import\\(\\s*"${escape(label)}"\\)\\n`, 'gm'), '');
      writeFileSync(fp, out);
    }
  }
  const byLabel = new Map<string, string[]>();
  for (const [rel, labels] of missing) for (const label of labels) byLabel.set(label, [...(byLabel.get(label) ?? []), rel]);
  for (const [label, rels] of [...byLabel.entries()].sort((a, b) => b[1].length - a[1].length)) console.log(`${String(rels.length).padStart(4)}  ${label}`);
  const total = [...byLabel.values()].reduce((s, v) => s + v.length, 0);
  console.log(`---- ${total} dangling import(s) in ${missing.size} file(s)${del ? ', deleted' : ''}`);
  return 0;
}

const argv = process.argv.slice(2);
const del = argv.includes('--delete');
const cli = cac('gn-dangling-imports');
cli.command('', 'imports whose .gni no longer exists')
    .option('--delete', 'also delete the import lines')
    .action(() => {
      process.exitCode = main(del);
    });
cli.help();
cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '--delete')]);
