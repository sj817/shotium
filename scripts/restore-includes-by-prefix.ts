// Put back #include lines the component sweeps deleted, for a header prefix
// whose files exist again today.
//
// The sweeps that removed a component also removed every
// `#include "<prefix>/..."` naming it. That is correct while the component is
// gone. It stops being correct the moment the component comes back -- and
// components do come back, because "working beats cleanly cut". Nothing put
// the includes back, so the restore looked complete (the directory is there,
// the GN dep is there) while every consumer still failed to compile.
//
// The failure does not name the include. It names the *symbol*:
//
//     storage/browser/quota/quota_database.h(83,16):
//         error: use of undeclared identifier 'BucketInfo'
//
// so it reads like a missing type, not a missing line, and the file it points
// at visibly has an include list that looks reasonable.
//
// This walks the pristine revision, collects every include under <prefix>, and
// re-inserts the ones that are (a) absent from the file now and (b) backed by a
// header that exists on disk now. Condition (b) is what keeps this from
// undoing a deliberate cut: a component that is still deleted contributes
// nothing.
//
//   pnpm restore:includes-by-prefix <prefix> [<path> ...] [--pristine <rev>] [-n]
//
//   <prefix>    include path prefix, e.g. components/services/storage/public/cpp
//   <path>      limit to these repo paths (default: whole tree)
//   -n          dry run

import {existsSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execaSync} from 'execa';
import pRetry from 'p-retry';

import {root} from './lib/repo.ts';

const DEFAULT_PRISTINE = 'c0bba1026178';  // upstream baseline: the clone root, no cuts.

// Write a source file that a compiler may currently have open. Windows fails
// the open-for-write with EINVAL -- not a sharing error, not EACCES -- while
// clang holds the header for reading, so this looks like a bad path rather
// than contention. Editing during a build is normal here (the builds are
// long), so retry rather than abort halfway through a sweep and leave the
// tree half-edited.
async function writeRetry(file: string, text: string): Promise<void> {
  await pRetry(() => {
    writeFileSync(file, text);
  }, {retries: 59, factor: 1, minTimeout: 500, maxTimeout: 500})
      .catch(() => {
        throw new Error(`could not write ${file}; a compiler has held it for 30s`);
      });
}

// Index of the last #include of the block that opens the file. Stops at the
// first namespace/template/forward declaration so an include is never
// inserted into the body, and never past a conditional that starts a
// platform section.
function leadingIncludeBlockEnd(lines: string[]): number {
  let last = -1;
  for (const [i, line] of lines.entries()) {
    const s = line.trim();
    if (s.startsWith('namespace ') || s.startsWith('template ') || s.startsWith('BASE_') || s.startsWith('COMPONENT_EXPORT')) break;
    // A conditional after real includes: everything below is guarded.
    if (s.startsWith('#if') && last >= 0) break;
    if (s.startsWith('#include ')) last = i;
  }
  return last;
}

async function main(prefixArg: string, paths: string[], pristine: string, dry: boolean): Promise<number> {
  const prefix = prefixArg.replace(/\/+$/, '');
  const args = ['grep', '-n', `include "${prefix}/`, pristine];
  if (paths.length) args.push('--', ...paths);
  const out = execaSync('git', args, {cwd: root, reject: false, maxBuffer: 1 << 28}).stdout;

  const wanted = new Map<string, string[]>();
  for (const line of out.split('\n')) {
    if (!line.trim()) continue;
    const rest = line.slice(line.indexOf(':') + 1);
    const first = rest.indexOf(':'), second = rest.indexOf(':', first + 1);
    const rel = rest.slice(0, first), text = rest.slice(second + 1);
    wanted.set(rel, [...(wanted.get(rel) ?? []), text.trimEnd()]);
  }

  let added = 0, skipped = 0, gone = 0;
  for (const rel of [...wanted.keys()].sort()) {
    const file = path.join(root, rel);
    if (!existsSync(file)) continue;
    const src = readFileSync(file, 'utf8');
    const nl = src.includes('\r\n') ? '\r\n' : '\n';
    const lines = src.split(nl);
    const missing: string[] = [];
    for (const inc of wanted.get(rel)!) {
      const header = inc.split('"')[1];
      // x.mojom.h / -blink.h / -forward.h / -shared.h are generated into
      // <out>/gen, so they are never on disk here. Their existence is decided
      // by whether the .mojom is still in the tree.
      let probe = header;
      if (header.includes('.mojom') && header.endsWith('.h')) probe = header.slice(0, header.indexOf('.mojom') + '.mojom'.length);
      if (!existsSync(path.join(root, probe))) {
        gone++;  // still cut; leaving it out is correct.
        continue;
      }
      if (lines.some((l) => l.trimStart().startsWith('#include') && l.includes(header))) {
        skipped++;
        continue;
      }
      missing.push(inc);
    }
    if (!missing.length) continue;
    let at = leadingIncludeBlockEnd(lines);
    if (at < 0) {
      console.log(`  MANUAL ${rel} (no include block)`);
      continue;
    }
    // Keep the block sorted: chromium include order is plain lexicographic
    // within the project group.
    let blockStart = at;
    while (blockStart > 0 && lines[blockStart - 1].trim() && lines[blockStart - 1].trimStart().startsWith('#include')) blockStart--;
    for (const inc of missing) {
      let pos = at + 1;
      for (let i = blockStart; i <= at; i++) {
        if (lines[i].trimStart().startsWith('#include "') && lines[i] > inc) {
          pos = i;
          break;
        }
      }
      lines.splice(pos, 0, inc);
      at++;
    }
    if (!dry) await writeRetry(file, lines.join(nl));
    added += missing.length;
    console.log(`  +${String(missing.length).padEnd(2)}    ${rel}`);
  }
  console.log(`${added} restored, ${skipped} already present, ${gone} still-cut headers left out${dry ? ' (dry run)' : ''}`);
  return 0;
}

const argv = process.argv.slice(2);
const dry = argv.includes('-n');
const cli = cac('restore-includes-by-prefix');
cli.command('<prefix> [...paths]', 'put back includes under a prefix whose headers exist again')
    .option('--pristine <rev>', 'the revision to read the includes from', {default: DEFAULT_PRISTINE})
    .option('-n', 'dry run')
    .action(async (prefix: string, paths: string[], options: {pristine: string}) => {
      process.exitCode = await main(prefix, paths, options.pristine, dry);
    });
cli.help();
cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '-n')]);
