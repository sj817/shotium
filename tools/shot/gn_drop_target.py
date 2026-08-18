"""Delete whole target blocks from a GN file.

Usage:
  gn_drop_target.py <BUILD.gn> <target-name> [<target-name> ...]

A target block starts at a line whose text before the first '(' ends with the
declaration, e.g. `source_set("browser_tests") {`, and ends at the matching
closing brace, counted by tracking brace depth outside of strings and comments.
Any `if (...) { ... }` nesting inside the block is therefore handled correctly.

Blank-line runs left behind by the deletion are collapsed to one.

Exits 2 if a requested target was not found, because a silent miss looks
exactly like a successful deletion on the next gn gen.
"""

import re
import sys

path = sys.argv[1]
targets = set(sys.argv[2:])

with open(path, "r", encoding="utf-8") as f:
    lines = f.readlines()

# Matches `foo("bar") {` for any template/rule name.
decl = re.compile(r'^\s*[A-Za-z_][A-Za-z0-9_]*\("([^"]+)"\)\s*\{')


def depth_delta(line):
    """Net brace depth change for a line, ignoring braces in strings/comments."""
    d, in_str, i = 0, False, 0
    while i < len(line):
        c = line[i]
        if in_str:
            if c == "\\":
                i += 2
                continue
            if c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif c == "#":
            break
        elif c == "{":
            d += 1
        elif c == "}":
            d -= 1
        i += 1
    return d


kept, found, i = [], set(), 0
while i < len(lines):
    m = decl.match(lines[i])
    if m and m.group(1) in targets:
        found.add(m.group(1))
        depth = depth_delta(lines[i])
        i += 1
        while i < len(lines) and depth > 0:
            depth += depth_delta(lines[i])
            i += 1
        continue
    kept.append(lines[i])
    i += 1

collapsed = []
for line in kept:
    if not line.strip() and collapsed and not collapsed[-1].strip():
        continue
    collapsed.append(line)

with open(path, "w", encoding="utf-8", newline="") as f:
    f.writelines(collapsed)

print("%s: removed %d target(s), %d line(s)" %
      (path, len(found), len(lines) - len(collapsed)))
missing = sorted(targets - found)
if missing:
    print("NOT FOUND: " + ", ".join(missing))
    sys.exit(2)
