// Put back GN deps entries a component sweep removed.
//
// The companion to restore-includes. Restoring a component needs three things
// done, and doing only some of them fails in ways that point away from the
// cause:
//
//   1. the directory back on disk        -- otherwise `gn gen` fails, loudly
//   2. its `deps` entries back           -- otherwise the generated headers are
//                                           never built, and the error is
//                                           "'...mojom-forward.h' file not found"
//                                           in a consumer that looks unrelated
//   3. its `#include` lines back         -- otherwise the symbol is undeclared
//                                           at the point of use, naming the
//                                           symbol rather than the header
//
// Each restored entry is put back after the line that preceded it in the
// original file, keeping the list in its original order.
//
//   pnpm restore:gn-deps <git-rev> <label-prefix> [<label-prefix> ...] [--apply]
//
//   <git-rev>       revision holding the pre-deletion contents, e.g. abc1234^
//   <label-prefix>  e.g. //services/metrics

import {existsSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execaSync} from 'execa';

import {root} from './lib/repo.ts';

const escape = (s: string) => s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

function git(...args: string[]): string {
  return execaSync('git', args, {cwd: root, reject: false, maxBuffer: 1 << 28}).stdout;
}

function main(rev: string, prefixes: string[], apply: boolean): number {
  const entry = new RegExp(`^\\s*"(?:${prefixes.map(escape).join('|')})[^"]*",\\s*$`);
  const changed = git('diff', '--name-only', rev, '--', '*.gn', '*.gni');
  let touched = 0, restored = 0;
  for (const raw of changed.split('\n')) {
    const rel = raw.trim();
    if (!rel) continue;
    const file = path.join(root, rel);
    if (!existsSync(file)) continue;
    const original = git('show', `${rev}:${rel}`);
    if (!original) continue;
    const oldLines = original.split('\n');
    let curLines: string[];
    try {
      // Universal newlines, as the Python original read (and wrote) them.
      curLines = readFileSync(file, 'utf8').replace(/\r\n/g, '\n').split('\n');
    } catch {
      continue;
    }
    const curSet = new Set(curLines);
    const additions: Array<[string | null, string]> = [];
    for (const [i, line] of oldLines.entries()) {
      if (entry.test(line) && !curSet.has(line)) additions.push([i ? oldLines[i - 1] : null, line]);
    }
    if (!additions.length) continue;
    touched++;
    restored += additions.length;
    console.log(`${String(additions.length).padStart(3)}  ${rel}`);
    if (!apply) continue;
    for (const [predecessor, line] of additions) {
      // Only anchor on a predecessor that appears exactly once. A common line
      // like `"//base",` occurs in every deps list in the file, and the first
      // occurrence lands the entry in the wrong target -- that is how
      // //components/unexportable_keys:test_support ended up in the
      // non-testonly //net:net.
      if (predecessor !== null && curLines.filter((l) => l === predecessor).length === 1) {
        curLines.splice(curLines.indexOf(predecessor) + 1, 0, line);
      } else {
        // No anchor left; append to the first deps list in the file.
        const n = curLines.findIndex((l) => /^\s*deps\s*\+?=\s*\[\s*$/.test(l));
        if (n >= 0) curLines.splice(n + 1, 0, line);
      }
    }
    writeFileSync(file, curLines.join('\n'));
  }
  console.log(`---- ${restored} entr(ies) in ${touched} file(s)${apply ? ', restored' : ''}`);
  return 0;
}

const argv = process.argv.slice(2);
const apply = argv.includes('--apply');
const cli = cac('restore-gn-deps');
cli.command('<rev> [...prefixes]', 'put back GN deps entries a component sweep removed')
    .option('--apply', 'write the files; without it, report only')
    .action((rev: string, prefixes: string[]) => {
      if (prefixes.length === 0) throw new Error('at least one label prefix is required');
      process.exitCode = main(rev, prefixes, apply);
    });
cli.help();
cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '--apply')]);
