#!/usr/bin/env python3
"""Stamp a synced Chromium workspace with content-faithful modification times.

ninja decides what is stale by comparing modification times, and both
actions/checkout and gclient write every file with "now". A build directory
restored from a previous run is therefore always older than the sources that
produced it, and the entire build reruns -- which makes carrying a build across
several CI runs impossible.

The fix is to give every source file the time of the revision it came from:

  * files in the main repository get the time of the last commit that touched
    them, read in a single pass over the log. A commit that changes ten files
    then moves ten timestamps, and ninja rebuilds exactly those.
  * files in a DEPS-managed repository get that repository's HEAD commit time.
    A dependency moves as a unit -- when the pinned revision changes, every
    file in it is suspect anyway.
  * anything a hook produced or downloaded is not in any repository, so it
    keeps the time it was written. Those are inputs to few edges and there is
    nothing better to say about them.

The output directory is left alone: its timestamps are what the comparison is
against.
"""

import argparse
import os
import subprocess
import sys


def git(repo, *args):
    return subprocess.run(['git', '-C', repo, *args],
                          capture_output=True, text=True, check=True).stdout


def read_gclient_entries(workspace):
    """The dependency list gclient wrote, as paths relative to the workspace."""
    path = os.path.join(workspace, '.gclient_entries')
    if not os.path.exists(path):
        return []
    namespace = {}
    with open(path, encoding='utf-8') as f:
        exec(compile(f.read(), path, 'exec'), namespace)  # noqa: S102
    return sorted(namespace.get('entries', {}))


def file_times_from_log(repo):
    """The last time each tracked path was touched, from one pass over the log."""
    out = git(repo, 'log', '--format=@%ct', '--name-only', '--no-renames')
    times = {}
    current = None
    for line in out.splitlines():
        if line.startswith('@'):
            current = int(line[1:])
        elif line and current is not None:
            times.setdefault(line, current)  # first seen == most recent
    return times


def stamp(path, when, counters):
    try:
        os.utime(path, (when, when))
        counters['stamped'] += 1
    except OSError:
        counters['failed'] += 1


def walk(root, skip_dirs):
    """Files under root, not descending into skip_dirs, .git or out."""
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [
            d for d in dirnames
            if d != '.git' and os.path.join(dirpath, d) not in skip_dirs
        ]
        for name in filenames:
            yield os.path.join(dirpath, name)


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('workspace', help='the directory holding .gclient')
    parser.add_argument('--solution', default='src')
    parser.add_argument('--out-dir', default='out',
                        help='relative to the solution; left untouched')
    args = parser.parse_args(argv[1:])

    workspace = os.path.abspath(args.workspace)
    solution = os.path.join(workspace, args.solution)
    entries = read_gclient_entries(workspace)

    # Directories that belong to a dependency, so the walk of the main
    # repository does not stamp them with the wrong repository's time.
    dep_dirs = {
        os.path.join(workspace, e.replace('/', os.sep))
        for e in entries if e != args.solution
    }
    skip = dep_dirs | {os.path.join(solution, args.out_dir)}

    counters = {'stamped': 0, 'failed': 0}

    times = file_times_from_log(solution)
    head = int(git(solution, 'log', '-1', '--format=%ct').strip())
    print(f'{args.solution}: {len(times)} tracked paths, head {head}')
    for path in walk(solution, skip):
        rel = os.path.relpath(path, solution).replace(os.sep, '/')
        stamp(path, times.get(rel, head), counters)

    for entry in entries:
        if entry == args.solution:
            continue
        repo = os.path.join(workspace, entry.replace('/', os.sep))
        if not os.path.isdir(os.path.join(repo, '.git')):
            continue  # a CIPD or GCS dependency: no revision to speak of
        try:
            when = int(git(repo, 'log', '-1', '--format=%ct').strip())
        except subprocess.CalledProcessError:
            continue
        for path in walk(repo, dep_dirs):
            stamp(path, when, counters)

    print(f"stamped {counters['stamped']} files, {counters['failed']} failed")
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
