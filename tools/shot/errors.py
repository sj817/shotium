# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Reduce a ninja log to the errors, grouped by target.

ninja echoes the full compile command after every FAILED:, which for this build
is about 5KB of include paths and warning flags per failure. At 200 failures
that is a megabyte of noise around the twenty lines that say what is wrong.

Usage:
    python tools/shot/errors.py out/Shot/build.log            # summary
    python tools/shot/errors.py out/Shot/build.log --full     # every message
    python tools/shot/errors.py out/Shot/build.log --top 30   # commonest first
"""

import argparse
import collections
import re
import sys

FAILED = re.compile(r'^FAILED: (?:\[code=\d+\] )?(\S+)')
PROGRESS = re.compile(r'^\[\d+/\d+\]')
# clang-cl diagnostics: path(line,col): error: message
DIAG = re.compile(r'^(.*?)\((\d+),(\d+)\): (error|fatal error): (.*)$')
LINK = re.compile(r'^(?:lld-link|LINK): (?:error|warning): (.*)$')


def parse(path):
    """Yield (object, [diagnostic lines]) for each failed edge."""
    with open(path, encoding='utf-8', errors='replace') as f:
        lines = f.read().splitlines()

    failures, current, obj = [], None, None
    for line in lines:
        m = FAILED.match(line)
        if m:
            if current is not None:
                failures.append((obj, current))
            obj, current = m.group(1), []
            continue
        if current is None:
            continue
        if PROGRESS.match(line):
            failures.append((obj, current))
            current, obj = None, None
            continue
        # The echoed command line is one enormous line starting with the
        # compiler path; nothing else in the block is remotely that long.
        if line.startswith('..\\..\\third_party\\llvm-build') or len(line) > 900:
            continue
        current.append(line)
    if current is not None:
        failures.append((obj, current))
    return failures


def message_of(block):
    """The first real diagnostic in a block, without the include chain."""
    for line in block:
        m = DIAG.match(line)
        if m:
            return m.group(5).strip()
        m = LINK.match(line)
        if m:
            return m.group(1).strip()
    for line in block:
        if line.strip():
            return line.strip()
    return '(no diagnostic)'


def where_of(block):
    for line in block:
        m = DIAG.match(line)
        if m:
            return '%s:%s' % (m.group(1).replace('\\', '/'), m.group(2))
    return ''


# "no member named 'foo' in 'blink::Bar'" and the same for 'baz' are one
# problem, not two; collapsing the quoted parts groups them.
QUOTED = re.compile(r"'[^']*'")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('log')
    parser.add_argument('--full', action='store_true',
                        help='print every diagnostic line, not just the first')
    parser.add_argument('--top', type=int, default=0,
                        help='instead of listing failures, group them by kind')
    args = parser.parse_args()

    failures = parse(args.log)
    if not failures:
        print('no failures in %s' % args.log)
        return 0

    if args.top:
        kinds = collections.Counter()
        examples = {}
        for obj, block in failures:
            msg = message_of(block)
            key = QUOTED.sub("'_'", msg)
            kinds[key] += 1
            examples.setdefault(key, (obj, msg))
        for key, count in kinds.most_common(args.top):
            obj, msg = examples[key]
            print('%4d  %s' % (count, key))
            print('      e.g. %s: %s' % (obj, msg))
        print('\n%d failing edge(s), %d distinct kind(s)'
              % (len(failures), len(kinds)))
        return 0

    by_target = collections.Counter()
    for obj, block in failures:
        target = obj.rsplit('/', 1)[0] if '/' in obj else obj
        by_target[target] += 1
        print('=== %s' % obj)
        if args.full:
            for line in block:
                print('  ' + line)
        else:
            where = where_of(block)
            print('  %s%s%s' % (where, ': ' if where else '', message_of(block)))
    print()
    for target, count in by_target.most_common():
        print('%5d  %s' % (count, target))
    print('%5d  TOTAL' % len(failures))
    return 0


if __name__ == '__main__':
    sys.exit(main())
