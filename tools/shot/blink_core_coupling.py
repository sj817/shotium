"""Rank blink core subdirectories by how entangled they are with the rest.

A subdirectory is cheap to delete when the files that reference it are
themselves deletable, and expensive when they are in the render pipeline we
have to keep working. So the useful number is not "how many references" but
"how many references land in css/layout/paint/dom/style/svg/html".

Usage:
  blink_core_coupling.py [<subdir> ...]     # default: every subdirectory
"""

import os
import re
import sys

CORE = r"D:\Github\chromium\third_party\blink\renderer\core"
# The DOM -> CSS -> Layout -> Paint pipeline plus what it directly needs.
PIPELINE = {"css", "layout", "paint", "dom", "style", "svg", "html",
            "animation", "frame", "page", "loader", "fetch", "scroll"}

names = sys.argv[1:] or sorted(
    d for d in os.listdir(CORE) if os.path.isdir(os.path.join(CORE, d)))

rows = []
for name in names:
    pat = re.compile(r'core/%s/' % re.escape(name))
    outside = pipeline = files = 0
    for dirpath, dirnames, filenames in os.walk(CORE):
        rel = os.path.relpath(dirpath, CORE).replace("\\", "/").split("/")[0]
        if rel == name:
            continue
        for fn in filenames:
            if not fn.endswith((".cc", ".h")):
                continue
            try:
                src = open(os.path.join(dirpath, fn), encoding="utf-8").read()
            except (OSError, UnicodeDecodeError):
                continue
            n = len(pat.findall(src))
            if not n:
                continue
            files += 1
            outside += n
            if rel in PIPELINE:
                pipeline += n
    own = 0
    for dirpath, dirnames, filenames in os.walk(os.path.join(CORE, name)):
        own += sum(1 for fn in filenames if fn.endswith((".cc", ".h")))
    rows.append((pipeline, outside, files, own, name))

rows.sort()
print("%8s %8s %7s %6s  %s" % ("PIPELINE", "REFS", "FILES", "OWN", "SUBDIR"))
for pipeline, outside, files, own, name in rows:
    print("%8d %8d %7d %6d  %s" % (pipeline, outside, files, own, name))
print("\nPIPELINE = references from css/layout/paint/dom/style/svg/html/"
      "animation/frame/page/loader/fetch/scroll; those are the ones that turn\n"
      "a deletion into a rewrite of code we have to keep working.")
