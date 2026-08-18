"""Delete .cc files whose own header was cut, and unlist them from BUILD.gn.

blink splits every //third_party/blink/public/common/<f>/<n>.h from its
implementation at //third_party/blink/common/<f>/<n>.cc. The component sweeps
worked on the public/ side, so the headers went and the implementations stayed:
files that define types nothing declares, still listed in
third_party/blink/common/BUILD.gn, still compiled.

They fail late and loudly -- `manifest.cc` cannot even open its first include --
but only once the build gets far enough to reach them, which is why a tree can
look like it is converging and then sprout forty new failures at once.

The pairing is positional, not by include text: <root>/common/<rel>.cc pairs
with <root>/public/common/<rel>.h. A .cc whose partner header is gone is dead by
construction; there is no configuration in which it compiles.

Not every file follows the split, though, and the first version of this script
did not check: a handful keep their header right beside them under common/
(crash_helpers, rust_crash, and the frame_policy / policy_value /
use_counter_feature traits). Those have no public/common/ partner and never
did, so they were reported as orphans and deleted. A sibling <name>.h in the
same directory counts as the partner too.

Usage:
  cut_orphan_sources.py <build.gn> [-n]

  Deletes the orphans under third_party/blink/common and removes their lines
  from the given BUILD.gn.
"""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
IMPL = os.path.join(ROOT, "third_party", "blink", "common")
PUB = os.path.join(ROOT, "third_party", "blink", "public", "common")


def orphans():
    for dirpath, _dirs, files in os.walk(IMPL):
        for name in files:
            if not name.endswith(".cc"):
                continue
            src = os.path.join(dirpath, name)
            rel = os.path.relpath(src, IMPL)
            # A unittest pairs with the header of the thing it tests.
            base = rel[:-3]
            for suffix in ("_unittest", "_fuzzer", "_perftest", ""):
                if base.endswith(suffix) and suffix:
                    base = base[:-len(suffix)]
                    break
            header = os.path.join(PUB, base + ".h")
            sibling = os.path.join(IMPL, base + ".h")
            if os.path.exists(header) or os.path.exists(sibling):
                continue
            yield src, os.path.relpath(header, ROOT).replace(os.sep, "/")


def main(argv):
    dry = "-n" in argv
    argv = [a for a in argv if a != "-n"]
    if not argv:
        sys.exit(__doc__)
    build_gn = argv[0]

    dead = sorted(orphans())
    if not dead:
        print("no orphans")
        return

    rels = set()
    for src, header in dead:
        rel = os.path.relpath(src, IMPL).replace(os.sep, "/")
        rels.add(rel)
        print("  dead   third_party/blink/common/%-58s (no %s)" % (rel, header))

    src = io.open(build_gn, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in src else "\n"
    lines = src.split(nl)
    kept = []
    removed = 0
    entry = re.compile(r'^\s*"([^"]+)",\s*$')
    for line in lines:
        m = entry.match(line)
        if m and m.group(1) in rels:
            removed += 1
            continue
        kept.append(line)

    if not dry:
        io.open(build_gn, "w", encoding="utf-8", newline="").write(nl.join(kept))
        for s, _h in dead:
            os.remove(s)
        # Leave no empty feature directories behind.
        for dirpath, _dirs, files in os.walk(IMPL, topdown=False):
            if not os.listdir(dirpath):
                os.rmdir(dirpath)

    print("%d orphan .cc deleted, %d BUILD.gn entries removed%s"
          % (len(dead), removed, " (dry run)" if dry else ""))


if __name__ == "__main__":
    main(sys.argv[1:])
