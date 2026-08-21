#!/usr/bin/env python3
"""Put deleted files back from the upstream baseline, driven by a build log.

This tree is a pruned Chromium. The pruning judged a file by whether three
records of a *Windows* build mentioned it, so anything only a Linux or macOS
build reads was invisible to that judgement and may be gone. When one of those
builds then fails, the fix is never to write a replacement: upstream already
has the file, and hand-writing platform code we deleted by accident is how a
fork acquires subtly wrong copies of things that used to be right.

So this reads a gn or ninja failure, works out which paths it is complaining
about, and restores exactly those from the pristine baseline.

The baseline is the clone root, before any cut, and it is an ancestor of
chromium/main -- so `git cat-file` reaches it even though this is a blobless
clone, fetching the one object it needs on demand. There is nothing to sync
beforehand and nothing to keep in step.

    python tools/shot/restore_from_upstream.py --log build.log
    python tools/shot/restore_from_upstream.py path/one.cc path/two.h
    python tools/shot/restore_from_upstream.py --log build.log --dry-run

What it will not do is decide that a file *should* come back. A source the
pruning removed on purpose, that a stale BUILD.gn still names, should be
dropped from that BUILD.gn instead -- tools/shot/gn_drop_missing_sources.py is
for exactly that. This prints what it restored so the choice stays visible.
"""

import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# The clone root: upstream, before any of this fork's cuts. Same constant as
# tools/shot/restore_includes_by_prefix.py, and for the same reason.
PRISTINE = 'c0bba1026178'

# The shapes gn and ninja use to say a file is not there. Each captures one
# repository-relative path.
PATTERNS = [
    # gn says "Source file not found." and then quotes the label on the next
    # line, indented, as "//path/to/file.cc" -- so the quotes are part of it.
    re.compile(r'^\s*"?//([^\s":]+\.[A-Za-z0-9_]+)"?,?\s*$'),
    # ninja: 'foo/bar.cc', needed by 'obj/...', missing and no known rule
    re.compile(r"ninja: error: '([^']+)', needed by"),
    # clang/msvc: fatal error: 'foo/bar.h' file not found
    re.compile(r"fatal error: ['\"]([^'\"]+)['\"] file not found"),
    re.compile(r"fatal error C1083: Cannot open include file: '([^']+)'"),
    # gn: Unable to load "/abs/path/to/foo.cc"
    re.compile(r'Unable to load "([^"]+)"'),
]

# A path we are told about may be absolute, or relative to the build directory,
# or already repository-relative. Reduce it to the last of those.
def normalise(path):
    path = path.replace('\\', '/').strip()
    if path.startswith('//'):
        path = path[2:]
    path = re.sub(r'^(\.\./)+', '', path)
    absolute = os.path.abspath(os.path.join(ROOT, path))
    try:
        rel = os.path.relpath(absolute, ROOT).replace('\\', '/')
    except ValueError:
        return None
    if rel.startswith('..'):
        return None
    return rel


def paths_from_log(text):
    found = []
    for line in text.splitlines():
        for pattern in PATTERNS:
            m = pattern.search(line)
            if m and m.groups():
                rel = normalise(m.group(1))
                if rel:
                    found.append(rel)
    # Preserve order, drop repeats.
    seen = set()
    return [p for p in found if not (p in seen or seen.add(p))]


def exists_upstream(rel):
    result = subprocess.run(
        ['git', 'cat-file', '-e', '%s:%s' % (PRISTINE, rel)],
        cwd=ROOT, capture_output=True)
    return result.returncode == 0


def restore(rel):
    blob = subprocess.run(
        ['git', 'cat-file', 'blob', '%s:%s' % (PRISTINE, rel)],
        cwd=ROOT, capture_output=True)
    if blob.returncode != 0:
        return False, blob.stderr.decode('utf-8', 'replace').strip()
    target = os.path.join(ROOT, rel)
    os.makedirs(os.path.dirname(target), exist_ok=True)
    with open(target, 'wb') as f:
        f.write(blob.stdout)
    return True, len(blob.stdout)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('paths', nargs='*',
                    help='repository-relative paths to restore')
    ap.add_argument('--log', help='a gn or ninja failure to read paths out of')
    ap.add_argument('--dry-run', action='store_true',
                    help='say what would be restored and stop')
    args = ap.parse_args(argv)

    wanted = [normalise(p) for p in args.paths]
    wanted = [p for p in wanted if p]
    if args.log:
        with open(args.log, encoding='utf-8', errors='replace') as f:
            wanted += paths_from_log(f.read())
    if not wanted:
        print('nothing to restore: pass paths, or --log with a failure in it')
        return 2

    seen = set()
    wanted = [p for p in wanted if not (p in seen or seen.add(p))]

    restored, already, absent = [], [], []
    for rel in wanted:
        if os.path.exists(os.path.join(ROOT, rel)):
            already.append(rel)
            continue
        if not exists_upstream(rel):
            absent.append(rel)
            continue
        if args.dry_run:
            restored.append((rel, None))
            continue
        ok, detail = restore(rel)
        if ok:
            restored.append((rel, detail))
        else:
            absent.append('%s (%s)' % (rel, detail))

    verb = 'would restore' if args.dry_run else 'restored'
    for rel, size in restored:
        print('  %s  %s%s' % (verb, rel,
                              '' if size is None else '  (%d bytes)' % size))
    for rel in already:
        print('  present already  %s' % rel)
    for rel in absent:
        # Not upstream either. Either the path was misread out of the log, or
        # a BUILD.gn names something that never existed, which is a different
        # bug and not one this script should paper over.
        print('  NOT IN UPSTREAM  %s' % rel)

    print()
    print('%s %d, already present %d, not in upstream %d'
          % (verb, len(restored), len(already), len(absent)))
    if restored and not args.dry_run:
        print('\nRe-run gn gen; a build usually names only the first few '
              'missing files, so expect to repeat this.')
    return 1 if absent else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
