"""Attribute shotium.exe image bytes to GN directories using PDB section contributions.

Two numbers are produced per bucket:

  raw    sum of every contribution, which double counts any COMDAT that the
         linker folded (ICF) because each surviving module still claims it
  uniq   each (section, offset, size) range counted once, attributed to the
         first module that claims it

`uniq` sums to the real image size; `raw` is only useful to see how much
folding happened.
"""

import collections
import re
import sys

CONTRIBS, MODULES = sys.argv[1], sys.argv[2]

# Mod 0123 | `o:\fake\prefix\obj\base\base\foo.obj`:
mod_re = re.compile(r"^\s*Mod (\d+) \| `([^`]*)`")
mod_name = {}
with open(MODULES, "r", errors="replace") as f:
    for line in f:
        m = mod_re.match(line)
        if m:
            mod_name[int(m.group(1))] = m.group(2)

# SC[.text] | mod = 0, 0001:0000, size = 68, data crc = ...
sc_re = re.compile(
    r"^\s*SC\[([^\]]+)\]\s*\| mod = (\d+), ([0-9a-fA-F]+):([0-9a-fA-F]+), size = (\d+)")

raw = collections.defaultdict(lambda: collections.defaultdict(int))
uniq = collections.defaultdict(lambda: collections.defaultdict(int))
seen = set()
folded = 0


def bucket(path):
    """o:\\fake\\prefix\\obj\\third_party\\blink\\renderer\\core\\foo.obj -> a label."""
    p = path.replace("\\", "/").lower()
    i = p.find("/obj/")
    if i < 0:
        # Import libs and CRT objects arrive as bare names or absolute paths.
        tail = p.rsplit("/", 1)[-1]
        if tail.endswith(".lib") or tail.endswith(".dll"):
            return "<import/system> " + tail
        return "<other> " + tail
    parts = p[i + 5:].split("/")[:-1]  # drop the .obj filename
    if not parts:
        return "<obj root>"
    # obj/<dir...>/<target>/  -- keep at most 3 leading dirs so the report stays
    # readable while still separating e.g. blink/renderer/core from /modules.
    return "//" + "/".join(parts[:3])


for line in open(CONTRIBS, "r", errors="replace"):
    m = sc_re.match(line)
    if not m:
        continue
    sect, mod, seg, off, size = m.group(1), int(m.group(2)), m.group(3), m.group(4), int(m.group(5))
    if size == 0:
        continue
    b = bucket(mod_name.get(mod, "<unknown mod %d>" % mod))
    raw[b][sect] += size
    key = (seg, off, size)
    if key in seen:
        folded += size
        continue
    seen.add(key)
    uniq[b][sect] += size

total_uniq = sum(sum(v.values()) for v in uniq.values())
total_raw = sum(sum(v.values()) for v in raw.values())
print("unique image bytes attributed : {:,}".format(total_uniq))
print("raw contribution bytes        : {:,}".format(total_raw))
print("double counted (folded/dup)   : {:,}".format(folded))
print()

rows = sorted(uniq.items(), key=lambda kv: -sum(kv[1].values()))
print("{:<52} {:>13} {:>13} {:>12} {:>6}".format(
    "bucket", "total", ".text", ".rdata", "%"))
print("-" * 100)
shown = 0
for name, secs in rows[:45]:
    t = sum(secs.values())
    shown += t
    print("{:<52} {:>13,} {:>13,} {:>12,} {:>5.1f}".format(
        name[:52], t, secs.get(".text", 0), secs.get(".rdata", 0),
        100.0 * t / total_uniq))
rest = total_uniq - shown
print("-" * 100)
print("{:<52} {:>13,} {:>13} {:>12} {:>5.1f}".format(
    "(%d more buckets)" % max(0, len(rows) - 45), rest, "", "",
    100.0 * rest / total_uniq))
