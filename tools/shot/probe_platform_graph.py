#!/usr/bin/env python3
"""List every GN label a platform wants that this tree cannot provide.

This tree is a pruned Chromium, and the pruning judged every file by whether a
*Windows* build reached it. Bringing another platform up therefore means
finding what that judgement discarded -- and `gn gen` reports exactly one
missing label before it stops, so discovering them one at a time costs a round
trip each. On CI that is fifteen minutes per label, with no way to tell how
many are left.

Two things make it cheaper. The first is that a Linux `gn gen` runs fine on a
Windows host, because gn evaluates build files and never invokes a compiler:

    python tools/shot/probe_platform_graph.py --os linux

writes out/ProbeLinux/args.gn with

    target_os = "linux"
    use_sysroot = false                                   # no sysroot here
    host_toolchain = "//build/toolchain/linux:clang_x64"  # host is not win

and runs gn against it. The errors come back word for word the same as the
runner's, in about half a minute.

The second is this script's own trick: when gn names a directory with no
BUILD.gn, it writes a stub there and runs gn again, so one pass reports the
whole set rather than its first element. The stubs are scaffolding and never a
fix -- every file written is recorded and removed before the script exits, and
it refuses to overwrite a path that already exists. What it produces is a list
to make decisions from: restore the directory from upstream with
tools/shot/restore_from_upstream.py, or cut the dependency that named it.

Two things it cannot tell you:

  * macOS. build/config/BUILDCONFIG.gn asserts host_os is mac or linux, so
    there is no cross-configure to run. --os mac is rejected rather than
    quietly producing something misleading.

  * Anything that only shows up when a compiler runs. A source file that
    exists but is wrong for the target -- the x64 assembly in an arm64 build,
    say -- resolves perfectly here and fails thousands of edges into ninja.

A clean run still ends in errors, and they are worth recognising rather than
chasing: cxxbridge.exe and the rust build scripts are reported as inputs
nothing generates, because rust_cxx.gni appends .exe when host_os == "win".
That is the host showing through, not the target, and the script says so.
"""

import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GN = os.path.join(ROOT, 'buildtools', 'win', 'gn.exe')
if not os.path.exists(GN):
    GN = os.path.join(ROOT, 'buildtools', 'linux64', 'gn')

UNABLE = re.compile(r'Unable to load "([^"]+)"')
NEED_TARGET = re.compile(r'no target named "([^"]+)" in "([^"]+)"')

# The tail of a run that got all the way through the graph. See the module
# docstring: host_os is win, so rust_cxx.gni suffixes the host tools it hands
# to actions, and gn cannot match them against the linux-toolchain outputs that
# actually produce them.
HOST_ARTIFACT = re.compile(r'(cxxbridge|_build_script)\.exe')

ARGS_FOR = {
    'linux': [
        'target_os = "linux"',
        # No sysroot in this checkout; build/config/sysroot.gni asserts on it.
        # Irrelevant to which BUILD.gn files exist, which is all this asks.
        'use_sysroot = false',
        # Without this, anything reaching a host tool loads
        # build/toolchain/win/toolchain.gni, which opens with assert(is_win).
        'host_toolchain = "//build/toolchain/linux:clang_x64"',
    ],
}
BASE_ARGS = {
    'linux': 'import("//build/args/shot-linux.gn")',
}


def run_gn(out_dir):
    result = subprocess.run([GN, 'gen', out_dir], cwd=ROOT,
                            capture_output=True, text=True, errors='replace')
    return result.returncode, result.stdout + result.stderr


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument('--os', default='linux', choices=sorted(ARGS_FOR) + ['mac'],
                    help='platform to configure for')
    ap.add_argument('--out', help='build directory to use (default out/Probe<Os>)')
    ap.add_argument('--max-steps', type=int, default=200)
    args = ap.parse_args(argv)

    if args.os == 'mac':
        print('macOS cannot be configured from here: build/config/BUILDCONFIG.gn\n'
              'asserts host_os is "mac" or "linux" ("Mac cross-compiles are\n'
              'unsupported"). Use the engine-macos workflow in probe mode.')
        return 2

    out_dir = args.out or ('out/Probe' + args.os.capitalize())
    os.makedirs(os.path.join(ROOT, out_dir), exist_ok=True)
    with open(os.path.join(ROOT, out_dir, 'args.gn'), 'w', newline='\n') as f:
        f.write(BASE_ARGS[args.os] + '\n')
        f.write('\n'.join(ARGS_FOR[args.os]) + '\n')

    created = []          # stub files written, to be removed at the end
    stub_targets = {}     # build file -> target names it has to define
    missing_files = []
    missing_targets = []

    def write_stub(path):
        body = '# TEMPORARY STUB from tools/shot/probe_platform_graph.py.\n'
        for name in sorted(stub_targets.get(path, ())):
            body += 'group("%s") {\n}\n' % name
        with open(path, 'w', newline='\n') as f:
            f.write(body)

    verdict = 'gave up'
    for step in range(args.max_steps):
        code, text = run_gn(out_dir)
        if code == 0:
            verdict = 'gn gen succeeded'
            break

        m = UNABLE.search(text)
        if m:
            path = m.group(1).replace('\\', '/')
            rel = os.path.relpath(path, ROOT).replace('\\', '/')
            if os.path.exists(path):
                print('gn cannot load a file that exists; stopping:\n%s'
                      % text[:1500])
                verdict = 'unreadable build file'
                break
            os.makedirs(os.path.dirname(path), exist_ok=True)
            stub_targets.setdefault(path, set()).add(
                os.path.basename(os.path.dirname(path)))
            write_stub(path)
            created.append(path)
            missing_files.append(rel)
            print('[%3d] no BUILD.gn    %s' % (step, rel))
            continue

        m = NEED_TARGET.search(text)
        if m:
            target, in_file = m.group(1), m.group(2).replace('\\', '/')
            path = os.path.join(ROOT, in_file.lstrip('/')).replace('\\', '/')
            if path not in stub_targets:
                # A real BUILD.gn is missing a target. That is a genuine
                # finding, not something to stub over.
                print('a build file in this tree does not define a target that '
                      'is asked for:\n%s' % text[:1500])
                verdict = 'missing target in a real build file'
                break
            stub_targets[path].add(target)
            write_stub(path)
            missing_targets.append((in_file, target))
            print('[%3d] no target     %s:%s' % (step, in_file, target))
            continue

        if HOST_ARTIFACT.search(text):
            verdict = ('graph resolved; only host-suffix artifacts left '
                       '(rust_cxx.gni appends .exe when host_os == "win")')
            break

        print('gn failed in a way this script does not recognise:\n%s'
              % text[:2500])
        verdict = 'unrecognised gn failure'
        break

    print()
    print('=== %s ===' % verdict)
    print('directories with no BUILD.gn (%d)' % len(missing_files))
    for rel in missing_files:
        print('  %s' % rel)
    if missing_targets:
        print('target names a stub had to define (%d)' % len(missing_targets))
        for f, t in missing_targets:
            print('  %s:%s' % (f, t))

    print('\nremoving %d stub file(s)' % len(created))
    for path in created:
        try:
            os.remove(path)
            d = os.path.dirname(path)
            while d.startswith(ROOT) and d != ROOT and not os.listdir(d):
                os.rmdir(d)
                d = os.path.dirname(d)
        except OSError as e:
            print('  COULD NOT REMOVE %s: %s' % (path, e))

    if missing_files or missing_targets:
        print('\nFor each one: restore it with\n'
              '  python tools/shot/restore_from_upstream.py <paths>\n'
              'or cut whatever names it. A directory only the test targets of '
              'a\nloaded BUILD.gn reach is usually the second.')
    return 0 if verdict.startswith(('gn gen succeeded', 'graph resolved')) else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
