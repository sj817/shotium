#!/usr/bin/env python3
"""Every file the shot build graph depends on, checked against the disk.

GN writes an edge's inputs into build.ninja whether or not the file is there.
Ninja only looks when it comes to build that edge, and on a warm build
directory that may be never. So a file deleted from the tree can sit in the
graph unnoticed for as long as the cache keeps holding its output -- and then
the first cold build stops twelve seconds in with

  ninja: error: '../../.rustfmt.toml', needed by
  'gen/third_party/crubit/support/rs_std/rs_alloc.h',
  missing and no known rule to make it

which is what happened after 1ee6e5a, on two platforms, an hour of CI apiece.

`gn gen` does not catch this. GN never opened .rustfmt.toml; it copied the
path into an action's `inputs` and moved on. Neither does a probe -- `ninja -n`
walks the same graph and gives up on the first missing input rather than
listing them. What catches it is asking ninja for the input set and stat()ing
every entry, which is all this does.

  python tools/shot/missing_inputs.py                 # out/ShotWip
  python tools/shot/missing_inputs.py out/Shot

Exit status is 1 if anything is missing, so it works as a build step.

One build directory answers for one platform: the graph names the sources that
platform selects. Run it against a Linux out/ directory to answer for Linux --
`gn gen` for a non-host platform works from any host, and is minutes rather
than the hours a build would take.
"""

import os
import subprocess
import sys

# The two targets the engine workflows build. Anything not reachable from them
# is not this project's problem -- the graph also carries Chromium's tests,
# whose sources were deliberately cut.
TARGETS = ['shot', 'shot_c']

DEFAULT_OUT = os.path.join('out', 'ShotWip')
NINJA = os.path.join('third_party', 'ninja', 'ninja')


def ninja_binary():
    for candidate in (NINJA, NINJA + '.exe'):
        if os.path.exists(candidate):
            return candidate
    return 'ninja'


def main(argv):
    out = argv[1] if len(argv) > 1 else DEFAULT_OUT
    if not os.path.exists(os.path.join(out, 'build.ninja')):
        sys.stderr.write('%s has no build.ninja -- run gn gen first\n' % out)
        return 2

    listed = subprocess.run(
        [ninja_binary(), '-C', out, '-t', 'inputs'] + TARGETS,
        capture_output=True, text=True)
    if listed.returncode != 0:
        sys.stderr.write(listed.stderr)
        return 2

    checked = 0
    missing = []
    for line in listed.stdout.splitlines():
        path = line.strip().strip('"')
        # Three kinds of input come back. `../../x` is a file in this tree and
        # is the only kind worth checking; an absolute path belongs to the SDK
        # or the toolchain, which is the host's business; anything else is
        # generated and lives under out/, so its absence is a build order
        # question rather than a missing file.
        if not path.startswith('../../'):
            continue
        checked += 1
        source = path[len('../../'):]
        if not os.path.exists(source.replace('/', os.sep)):
            missing.append(source)

    print('%s: %d source-tree inputs of %s' %
          (out, checked, ' + '.join(TARGETS)))
    if not missing:
        print('all present')
        return 0

    print('%d missing:' % len(missing))
    for path in sorted(missing):
        print('  ' + path)
    print('\nEach one is an edge in the graph with nothing behind it. Restore'
          '\nit from before the commit that removed it -- `git show <sha>^:'
          '<path>`\n-- or cut the target that wants it. See'
          ' docs/upstream-sync.md.')
    return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
