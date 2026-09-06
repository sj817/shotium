// Put back #include lines a component sweep removed from surviving files.
//
// A component's removal deleted every `#include "<component>/..."` naming it.
// When the component comes back -- as base/trace_event, base/tracing and
// third_party/perfetto did, because base/check.h reaches them through
// base/location.h -- those includes have to come back too, and only in the
// files that still exist.
//
// Missing them is not a link error, it is a compile error a long way from the
// cause: dropping base/tracing/protos/chrome_track_event.pbzero.h from
// base/task/sequence_manager/task_queue.h leaves
//
//     using QueueName = ::perfetto::protos::pbzero::SequenceManagerTask::QueueName;
//
// with nothing declaring SequenceManagerTask, and the diagnostic names
// perfetto rather than the header that went missing.
//
// Each restored line is put back after the line that preceded it in the
// original file, which keeps the include block in its original order.
//
//   pnpm restore:includes <git-rev> <prefix> [<prefix> ...] [--apply]
//
//   <git-rev>   revision holding the pre-deletion contents, e.g. abc1234^
//   <prefix>    include path prefix to restore, e.g. base/tracing/

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
  const pattern = new RegExp(`^#include "(?:${prefixes.map(escape).join('|')})`);
  const diff = git('diff', '--name-only', rev, '--', '*.cc', '*.h', '*.mm');
  let touched = 0, restored = 0;
  for (const raw of diff.split('\n')) {
    const rel = raw.trim();
    if (!rel) continue;
    const file = path.join(root, rel);
    if (!existsSync(file)) continue;  // File was deleted outright; nothing to restore into.
    const original = git('show', `${rev}:${rel}`);
    if (!original) continue;
    const oldLines = original.split('\n');
    let current: string;
    try {
      // Universal newlines, as the Python original read (and wrote) them.
      current = readFileSync(file, 'utf8').replace(/\r\n/g, '\n');
    } catch {
      continue;
    }
    const curLines = current.split('\n');
    const curSet = new Set(curLines);
    const additions: Array<[string | null, string]> = [];
    for (const [i, line] of oldLines.entries()) {
      if (pattern.test(line) && !curSet.has(line)) additions.push([i ? oldLines[i - 1] : null, line]);
    }
    if (!additions.length) continue;
    touched++;
    restored += additions.length;
    if (!apply) {
      console.log(`${String(additions.length).padStart(3)}  ${rel}`);
      continue;
    }
    for (const [predecessor, line] of additions) {
      if (predecessor !== null && curLines.includes(predecessor)) {
        curLines.splice(curLines.indexOf(predecessor) + 1, 0, line);
      } else {
        // No anchor: put it after the last include in the file.
        let last = 0;
        for (const [n, l] of curLines.entries()) if (l.startsWith('#include ')) last = n;
        curLines.splice(last + 1, 0, line);
      }
    }
    writeFileSync(file, curLines.join('\n'));
    console.log(`${String(additions.length).padStart(3)}  ${rel}`);
  }
  console.log(`---- ${restored} include(s) in ${touched} file(s)${apply ? ', restored' : ''}`);
  return 0;
}

const argv = process.argv.slice(2);
const apply = argv.includes('--apply');
const cli = cac('restore-includes');
cli.command('<rev> [...prefixes]', 'put back #include lines a component sweep removed')
    .option('--apply', 'write the files; without it, report only')
    .action((rev: string, prefixes: string[]) => {
      if (prefixes.length === 0) throw new Error('at least one include prefix is required');
      process.exitCode = main(rev, prefixes, apply);
    });
cli.help();
cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '--apply')]);
