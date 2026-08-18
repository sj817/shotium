"""Drop `"..."` list entries from a GN file by exact inner-string match.

Usage:
  gn_drop.py <BUILD.gn> <entry> [<entry> ...]
  gn_drop.py <BUILD.gn> --from <file-with-one-entry-per-line>

An entry is the text inside the quotes, e.g. gpu/gpu_internals_ui.cc or
//content/browser/tracing:resources. A line is removed only when, after
stripping whitespace, it is exactly `"<entry>",` or `"<entry>"`. That keeps the
edit from silently matching a substring of some unrelated path.

Reports every entry that matched nothing, because a silent miss here looks
identical to a successful deletion in the next gn gen.
"""

import sys

path = sys.argv[1]
if sys.argv[2] == "--from":
    entries = [l.strip() for l in open(sys.argv[3], encoding="utf-8") if l.strip()]
else:
    entries = sys.argv[2:]

wanted = set()
for e in entries:
    wanted.add('"%s",' % e)
    wanted.add('"%s"' % e)

with open(path, "r", encoding="utf-8") as f:
    lines = f.readlines()

kept, dropped = [], []
for line in lines:
    if line.strip() in wanted:
        dropped.append(line.strip())
    else:
        kept.append(line)

with open(path, "w", encoding="utf-8", newline="") as f:
    f.writelines(kept)

print("%s: dropped %d line(s)" % (path, len(dropped)))
hit = {d.rstrip(",").strip('"') for d in dropped}
missed = [e for e in entries if e not in hit]
if missed:
    print("NO MATCH for %d entr(ies):" % len(missed))
    for m in missed:
        print("  " + m)
    sys.exit(2)
