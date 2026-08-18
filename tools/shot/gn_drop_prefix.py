"""Drop `"<prefix>..."` entries from a GN file, for whole deleted directories.

Usage: gn_drop_prefix.py <BUILD.gn> <prefix> [<prefix> ...]

A line is removed when, after stripping whitespace, it is a single quoted
string starting with one of the prefixes, optionally followed by a comma:

    "webrtc/webrtc_internals.cc",
    "//content/browser/webrtc:foo",

This exists because `gn gen` does NOT check that files listed in `sources`
exist -- only ninja does, at build time. After deleting a directory the stale
source entries are therefore invisible until a build runs, which may be a long
way off. Removing them here keeps the BUILD file honest in the meantime.

Prints a per-prefix count so a prefix that matched nothing is visible rather
than being mistaken for a completed deletion.
"""

import collections
import re
import sys

path = sys.argv[1]
prefixes = sys.argv[2:]

with open(path, "r", encoding="utf-8") as f:
    lines = f.readlines()

entry = re.compile(r'^"([^"]+)",?$')
counts = collections.Counter()
kept = []
for line in lines:
    m = entry.match(line.strip())
    hit = None
    if m:
        for p in prefixes:
            if m.group(1).startswith(p):
                hit = p
                break
    if hit is not None:
        counts[hit] += 1
    else:
        kept.append(line)

with open(path, "w", encoding="utf-8", newline="") as f:
    f.writelines(kept)

print("%s: dropped %d line(s)" % (path, len(lines) - len(kept)))
for p in prefixes:
    if counts[p] == 0:
        print("  0  %s   <- matched nothing" % p)
