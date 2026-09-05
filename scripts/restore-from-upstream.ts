// Put deleted files back from the upstream baseline, driven by a build log.
//
// This tree is a pruned Chromium. The pruning judged a file by what the
// builds read, so anything only one platform reads may be gone if that
// platform's records were incomplete. When one of those builds then fails,
// the fix is never to write a replacement: upstream already has the file, and
// hand-writing platform code we deleted by accident is how a fork acquires
// subtly wrong copies of things that used to be right.
//
// So this reads a gn or ninja failure, works out which paths it is
// complaining about, and restores exactly those from the pristine baseline.
//
// The baseline is the clone root, before any cut, and it is an ancestor of
// chromium/main -- so `git cat-file` reaches it even though this is a
// blobless clone, fetching the one object it needs on demand. There is
// nothing to sync beforehand and nothing to keep in step.
//
//   pnpm restore:upstream --log build.log
//   pnpm restore:upstream path/one.cc path/two.h
//   pnpm restore:upstream --log build.log --dry-run
//
// What it will not do is decide that a file *should* come back. A source the
// pruning removed on purpose, that a stale BUILD.gn still names, should be
// dropped from that BUILD.gn instead. This prints what it restored so the
// choice stays visible. Relative paths are resolved against the repository
// root.

import {existsSync, mkdirSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execaSync} from 'execa';

import {resolve, root} from './lib/repo.ts';

// The clone root: upstream, before any of this fork's cuts. The same constant
// as restore-includes, and for the same reason.
export const PRISTINE = 'c0bba1026178';

// The shapes gn and ninja use to say a file is not there. Each captures one
// repository-relative path.
const PATTERNS = [
  // gn says "Source file not found." and then quotes the label on the next
  // line, indented, as "//path/to/file.cc" -- so the quotes are part of it.
  /^\s*"?\/\/([^\s":]+\.[A-Za-z0-9_]+)"?,?\s*$/,
  // ninja: 'foo/bar.cc', needed by 'obj/...', missing and no known rule
  /ninja: error: '([^']+)', needed by/,
  // clang/msvc: fatal error: 'foo/bar.h' file not found
  /fatal error: ['"]([^'"]+)['"] file not found/,
  /fatal error C1083: Cannot open include file: '([^']+)'/,
  // gn: Unable to load "/abs/path/to/foo.cc"
  /Unable to load "([^"]+)"/,
];

function existsUpstream(rel: string): boolean {
  return execaSync('git', ['cat-file', '-e', `${PRISTINE}:${rel}`], {cwd: root, reject: false}).exitCode === 0;
}

// A path we are told about may be absolute, or relative to the build
// directory, or already repository-relative. Reduce it to the last of those.
//
// The absolute case is the one that matters, because the interesting logs
// come from a CI runner: gn reports
//   Unable to load "/home/runner/work/shotium/shotium/src/third_party/x/BUILD.gn"
// and that prefix has nothing to do with this machine. Rather than guess
// where the checkout root was, try each suffix of the path from longest to
// shortest and take the first one upstream recognises. A wrong guess cannot
// survive that test, because it would have to name a file that exists in
// Chromium.
export function normalise(raw: string): string | null {
  let p = raw.replace(/\\/g, '/').trim();
  if (p.startsWith('//')) p = p.slice(2);
  p = p.replace(/^(\.\.\/)+/, '');
  const absolute = path.resolve(root, p);
  const rel = path.relative(root, absolute).replace(/\\/g, '/');
  if (!rel.startsWith('..') && !path.isAbsolute(rel)) return rel;
  const parts = p.split('/').filter((s) => s && s !== '..');
  for (let start = 0; start < parts.length; start++) {
    const candidate = parts.slice(start).join('/');
    if (existsUpstream(candidate)) return candidate;
  }
  return null;
}

export function pathsFromLog(text: string): string[] {
  const found: string[] = [];
  for (const line of text.split(/\r?\n/)) {
    for (const pattern of PATTERNS) {
      const m = pattern.exec(line);
      if (m && m[1]) {
        const rel = normalise(m[1]);
        if (rel) found.push(rel);
      }
    }
  }
  return [...new Set(found)];
}

function restore(rel: string): [true, number] | [false, string] {
  const blob = execaSync('git', ['cat-file', 'blob', `${PRISTINE}:${rel}`], {cwd: root, reject: false, encoding: 'buffer'});
  if (blob.exitCode !== 0) return [false, Buffer.from(blob.stderr).toString('utf8').trim()];
  const target = path.join(root, rel);
  mkdirSync(path.dirname(target), {recursive: true});
  writeFileSync(target, blob.stdout);
  return [true, blob.stdout.length];
}

function main(paths: string[], log: string | undefined, dryRun: boolean): number {
  let wanted = paths.map(normalise).filter((p): p is string => p !== null);
  if (log) wanted.push(...pathsFromLog(readFileSync(resolve(log), 'utf8')));
  if (wanted.length === 0) {
    console.log('nothing to restore: pass paths, or --log with a failure in it');
    return 2;
  }
  wanted = [...new Set(wanted)];

  const restored: Array<[string, number | null]> = [], already: string[] = [], absent: string[] = [];
  for (const rel of wanted) {
    if (existsSync(path.join(root, rel))) {
      already.push(rel);
      continue;
    }
    if (!existsUpstream(rel)) {
      absent.push(rel);
      continue;
    }
    if (dryRun) {
      restored.push([rel, null]);
      continue;
    }
    const [ok, detail] = restore(rel);
    if (ok) restored.push([rel, detail as number]);
    else absent.push(`${rel} (${detail})`);
  }
  const verb = dryRun ? 'would restore' : 'restored';
  for (const [rel, size] of restored) console.log(`  ${verb}  ${rel}${size === null ? '' : `  (${size} bytes)`}`);
  for (const rel of already) console.log(`  present already  ${rel}`);
  // Not upstream either. Either the path was misread out of the log, or a
  // BUILD.gn names something that never existed, which is a different bug
  // and not one this script should paper over.
  for (const rel of absent) console.log(`  NOT IN UPSTREAM  ${rel}`);
  console.log();
  console.log(`${verb} ${restored.length}, already present ${already.length}, not in upstream ${absent.length}`);
  if (restored.length && !dryRun) {
    console.log('\nRe-run gn gen; a build usually names only the first few missing files, so expect to repeat this.');
  }
  return absent.length ? 1 : 0;
}

// cac 7 registers a boolean option under its camelCase name only, so `--dry-run
// path` hands the path to the option instead of the positionals. The flag is
// taken off argv here and left in the option list for --help.
const argv = process.argv.slice(2);
const dryRun = argv.includes('--dry-run');
const cli = cac('restore-from-upstream');
cli.command('[...paths]', 'restore repository-relative paths from the pristine upstream baseline')
    .option('--log <file>', 'a gn or ninja failure to read paths out of')
    .option('--dry-run', 'say what would be restored and stop')
    .action((paths: string[], options: {log?: string}) => {
      process.exitCode = main(paths, options.log, dryRun);
    });
cli.help();
cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '--dry-run')]);
