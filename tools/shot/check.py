# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Syntax-check individual translation units without building anything.

The edit/build loop for this cut was costing tens of minutes per round, and
almost all of that was code generation and linking for a binary that was not
going to link anyway. Every error we are chasing is a *front-end* error: a
missing declaration, a dangling include, a member that no longer exists. Those
are all decided before the back end runs.

So: pull the exact compile command out of ninja's compdb, add -fsyntax-only,
and run it. No object file, no optimizer, no linker. A core/ TU that takes ~40s
to compile syntax-checks in ~8s, and twelve run at once.

Usage:
    python tools/shot/check.py path/to/foo.cc [more.cc ...]
    python tools/shot/check.py --from-log out/Shot/build.log
    python tools/shot/check.py --dir third_party/blink/renderer/core/frame

--from-log re-checks exactly the TUs that failed in a previous ninja run, which
is the normal way to use this: build once with -k 0 to get the full failure set,
then iterate here until it is empty, and only then build again.
"""

import argparse
import concurrent.futures
import json
import os
import pickle
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, 'out', 'Shot')
CCDB = os.path.join(OUT, 'ccdb.json')
INDEX = os.path.join(OUT, 'ccdb.index.pickle')


def norm(path):
    return os.path.normpath(path).replace('\\', '/').lower()


def load_index():
    """Map normalised source path -> compile command.

    The compdb is ~170MB; parsing it costs several seconds, so the derived
    index is cached next to it and invalidated by mtime.
    """
    if not os.path.exists(CCDB):
        sys.exit('no %s; run: ninja -C out/Shot -t compdb cxx > out/Shot/ccdb.json'
                 % CCDB)
    stamp = os.path.getmtime(CCDB)
    if os.path.exists(INDEX):
        with open(INDEX, 'rb') as f:
            cached = pickle.load(f)
        if cached.get('stamp') == stamp:
            return cached['index']

    with open(CCDB, encoding='utf-8') as f:
        entries = json.load(f)
    index = {}
    for entry in entries:
        src = entry['file']
        if not os.path.isabs(src):
            src = os.path.join(entry.get('directory', OUT), src)
        # First writer wins: a source compiled into several targets (core and
        # core_hot, say) gets checked once, and the flags that matter here --
        # include paths and defines -- are the same in both.
        index.setdefault(norm(src), entry['command'])
    with open(INDEX, 'wb') as f:
        pickle.dump({'stamp': stamp, 'index': index}, f)
    return index


def tokenize(command):
    """Split a Windows command line into argv.

    These are the real MSVCRT rules, not an approximation: a backslash is
    literal except immediately before a quote, where 2n backslashes plus a quote
    give n backslashes and toggle quoting, and 2n+1 give n backslashes and a
    literal quote. Approximating this is not safe here because at least one
    argument depends on the escaped form --
        "-DSK_USER_CONFIG_HEADER=\\"../../skia/config/SkUserConfig.h\\""
    whose value must still carry its quotes when the preprocessor sees it,
    otherwise skia's #include SK_USER_CONFIG_HEADER has nothing to include.
    """
    argv, current, in_quote, started = [], [], False, False
    i, n = 0, len(command)
    while i < n:
        ch = command[i]
        if ch == '\\':
            slashes = 0
            while i < n and command[i] == '\\':
                slashes += 1
                i += 1
            if i < n and command[i] == '"':
                current.append('\\' * (slashes // 2))
                if slashes % 2:
                    current.append('"')
                else:
                    in_quote = not in_quote
                started = True
                i += 1
            else:
                current.append('\\' * slashes)
                started = True
            continue
        if ch == '"':
            in_quote = not in_quote
            started = True
        elif ch.isspace() and not in_quote:
            if started:
                argv.append(''.join(current))
                current, started = [], False
        else:
            current.append(ch)
            started = True
        i += 1
    if started:
        argv.append(''.join(current))
    return argv


def syntax_only(command):
    out = []
    for tok in tokenize(command):
        # /Fo names the object file we do not want; /showIncludes prints a
        # dependency list nobody reads here.
        if tok.startswith('/Fo') or tok.startswith('/showIncludes'):
            continue
        # /c means "compile, don't link"; with -fsyntax-only it is unused, and
        # this build turns unused-command-line-argument into an error.
        if tok == '/c':
            continue
        out.append(tok)
    out.append('-fsyntax-only')
    return out


def check_one(source, command):
    argv = syntax_only(command)
    # CreateProcess resolves a relative executable against the *calling*
    # process's directory, not against the cwd= argument, so the compdb's
    # ..\..\third_party\llvm-build\... has to be made absolute here.
    if not os.path.isabs(argv[0]):
        argv[0] = os.path.normpath(os.path.join(OUT, argv[0]))
    proc = subprocess.run(argv, cwd=OUT, capture_output=True, text=True,
                          errors='replace')
    return source, proc.returncode, (proc.stdout or '') + (proc.stderr or '')


# ninja writes "FAILED: [code=1] obj/foo/bar.obj"; the exit-code prefix is not
# always there, and older logs have neither it nor the obj/ prefix.
FAILED_OBJ = re.compile(r'^FAILED: (?:\[code=\d+\] )?(?:obj/)?(\S+\.obj)', re.M)


def sources_from_log(path, index):
    """Recover the source files behind a ninja log's FAILED: lines.

    ninja names the object; the compdb names the source. The object path mirrors
    the source path under obj/<target-dir>/<target-name>/<basename>.obj, so the
    basename plus the directory prefix identifies it -- but rather than guess the
    mapping, this looks the object up in the compdb by scanning for the -o.
    """
    with open(path, encoding='utf-8', errors='replace') as f:
        text = f.read()
    objs = set(FAILED_OBJ.findall(text))
    if not objs:
        return []
    # Build the reverse map lazily: object -> source.
    by_obj = {}
    for src, command in index.items():
        m = re.search(r'/Fo(\S+\.obj)', command)
        if m:
            by_obj[m.group(1)] = src
    found, missing = [], []
    for obj in sorted(objs):
        src = by_obj.get(obj) or by_obj.get('obj/' + obj)
        (found if src else missing).append(src or obj)
    if missing:
        print('%d failed object(s) are no longer in the build graph '
              '(target deleted or renamed):' % len(missing))
        for obj in missing[:10]:
            print('    ' + obj)
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('sources', nargs='*')
    parser.add_argument('--from-log', help='re-check the TUs that failed here')
    parser.add_argument('--dir', help='check every TU under this directory')
    parser.add_argument('-j', type=int, default=12,
                        help='parallelism (12 is the OOM ceiling on this host)')
    parser.add_argument('--lines', type=int, default=12,
                        help='error lines to show per file')
    args = parser.parse_args()

    index = load_index()

    wanted = []
    for source in args.sources:
        key = norm(os.path.join(ROOT, source) if not os.path.isabs(source)
                   else source)
        if key in index:
            wanted.append(key)
        else:
            print('not in the build graph: %s' % source)
    if args.from_log:
        wanted += sources_from_log(args.from_log, index)
    if args.dir:
        prefix = norm(os.path.join(ROOT, args.dir)) + '/'
        wanted += [k for k in index if k.startswith(prefix)]

    wanted = sorted(set(wanted))
    if not wanted:
        sys.exit('nothing to check')

    print('checking %d translation unit(s) with -j %d\n' % (len(wanted), args.j))
    bad = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.j) as pool:
        futures = [pool.submit(check_one, s, index[s]) for s in wanted]
        done = 0
        for future in concurrent.futures.as_completed(futures):
            source, code, output = future.result()
            done += 1
            if code == 0:
                continue
            bad.append(source)
            rel = os.path.relpath(source, norm(ROOT)).replace('\\', '/')
            print('=== %s' % rel)
            lines = [l for l in output.splitlines() if l.strip()]
            for line in lines[:args.lines]:
                print('  ' + line)
            if len(lines) > args.lines:
                print('  ... %d more line(s)' % (len(lines) - args.lines))
            print()

    print('%d/%d clean, %d failing' % (len(wanted) - len(bad), len(wanted),
                                       len(bad)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
