"""List directories that no surviving GN file outside them refers to.

Deleting a subsystem usually orphans several third_party libraries at once, and
they are only findable after the fact. This looks for `//<dir>` mentions in every
BUILD.gn/.gni outside the directory itself; a directory with none is dead weight
in the checkout even though nothing in the build graph will complain about it.

This is a *candidate* list, not a delete list. Two things it cannot see, both of
which have already burned us once:

  * a directory that only defines GN templates (components/vector_icons) is
    referenced by `import()` of a .gni whose path may be spelled relatively;
  * a directory built by someone else's BUILD.gn by path rather than by label
    (third_party/skia holds no GN targets of its own).

Usage:
  find_unreferenced.py <parent-dir> [<parent-dir> ...]   # e.g. third_party
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
SKIP_DIRS = {".git", "out"}

parents = sys.argv[1:]
if not parents:
    sys.exit(__doc__)

candidates = {}
for parent in parents:
    base = os.path.join(ROOT, parent.replace("/", os.sep))
    if not os.path.isdir(base):
        continue
    for name in os.listdir(base):
        if os.path.isdir(os.path.join(base, name)):
            candidates["%s/%s" % (parent.replace("\\", "/"), name)] = 0

if not candidates:
    sys.exit("no subdirectories found")

# Longest first: regex alternation takes the first branch that matches, so with
# `protobuf` before `protobuf-javascript` every mention of the latter would be
# credited to the former and the latter would look dead.
pattern = re.compile("|".join(re.escape("//%s" % c)
                              for c in sorted(candidates, key=len, reverse=True)))
owner = {c: os.path.join(ROOT, c.replace("/", os.sep)) for c in candidates}

for dirpath, dirnames, filenames in os.walk(ROOT):
    dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
    if "depot_tools" in dirpath:
        continue
    for fn in filenames:
        if fn != "BUILD.gn" and not fn.endswith((".gni", ".gn")):
            continue
        fp = os.path.join(dirpath, fn)
        try:
            src = open(fp, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        for m in set(pattern.findall(src)):
            c = m[2:]
            # A directory referring to itself does not keep itself alive.
            if not fp.startswith(owner[c]):
                candidates[c] += 1

dead = sorted(c for c, n in candidates.items() if n == 0)
for c in dead:
    total = 0
    count = 0
    for dp, dn, fns in os.walk(os.path.join(ROOT, c.replace("/", os.sep))):
        for fn in fns:
            try:
                total += os.path.getsize(os.path.join(dp, fn))
                count += 1
            except OSError:
                pass
    print("%9.1f MiB %7d files  %s" % (total / 1048576.0, count, c))
print("---- %d of %d directories have no external reference"
      % (len(dead), len(candidates)))
