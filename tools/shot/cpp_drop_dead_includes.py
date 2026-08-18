"""Remove `#include "..."` lines whose header no longer exists.

After a directory is gutted the surviving files still include what went. Every
one of those is a hard "file not found", and the compiler reports them one
translation unit at a time, so clearing them by build round is hopeless.

Removing a dead include cannot make things worse: the header is genuinely
absent, so the TU could not have compiled either way. What it does do is
uncover the *next* layer of error -- the undeclared identifiers that the
deleted header used to declare -- which is the information actually needed to
decide whether each caller should be rewritten or deleted.

Only quoted includes are resolved, and only against the source root (a quoted
include in chromium is always root-relative; the "relative to the including
file" fallback is not used in this tree). Angle-bracket includes are left
alone: they resolve against include paths this script does not know.

Usage:
  cpp_drop_dead_includes.py <dir> [<dir> ...] [--pattern RE] [--apply]

  --pattern  Only consider includes whose path matches this regex. Use it to
             scope a sweep to one deleted subsystem instead of every absent
             header in the tree, which would also delete includes of headers
             that are merely not checked out on this platform.
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
INCLUDE = re.compile(r'^#include "([^"]+)"[ \t]*\r?\n', re.M)
EXTS = (".cc", ".h", ".mm", ".cpp")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        sys.exit(__doc__)
    apply_changes = "--apply" in sys.argv
    pattern = None
    if "--pattern" in sys.argv:
        pattern = re.compile(args.pop(args.index(sys.argv[sys.argv.index("--pattern") + 1])))

    dropped = {}
    for d in args:
        for dirpath, dirnames, filenames in os.walk(os.path.join(ROOT, d)):
            for fn in filenames:
                if not fn.endswith(EXTS):
                    continue
                fp = os.path.join(dirpath, fn)
                try:
                    src = open(fp, encoding="utf-8").read()
                except (OSError, UnicodeDecodeError):
                    continue
                gone = []

                def keep(m):
                    inc = m.group(1)
                    if pattern and not pattern.search(inc):
                        return m.group(0)
                    if os.path.exists(os.path.join(ROOT, inc.replace("/", os.sep))):
                        return m.group(0)
                    gone.append(inc)
                    return ""

                out = INCLUDE.sub(keep, src)
                if not gone:
                    continue
                rel = os.path.relpath(fp, ROOT).replace("\\", "/")
                dropped[rel] = gone
                if apply_changes:
                    open(fp, "w", encoding="utf-8", newline="").write(out)

    counts = {}
    for gone in dropped.values():
        for inc in gone:
            counts[inc] = counts.get(inc, 0) + 1
    for inc in sorted(counts, key=lambda i: -counts[i]):
        print("%5d  %s" % (counts[inc], inc))
    print("---- %d include(s) in %d file(s)%s"
          % (sum(counts.values()), len(dropped),
             ", removed" if apply_changes else ""))


main()
