"""Remove entries from the top-level DEPS file by checkout path.

DEPS entries come in two shapes -- a one-liner whose value is a `Var(...)`
expression, and a dict spanning many lines -- and the same path also shows up in
`recursedeps` lists and in hook `action` argument lists. This removes all three
by tracking bracket depth from the key line instead of pattern-matching a shape.

Usage:
  deps_drop.py <path> [<path> ...]      # e.g. src/third_party/ffmpeg
"""

import re
import sys

DEPS = r"D:\Github\chromium\DEPS"
paths = sys.argv[1:]
if not paths:
    sys.exit(__doc__)

lines = open(DEPS, encoding="utf-8").read().split("\n")
drop = [False] * len(lines)
key_re = re.compile(r"^\s*'(%s)(/[^']*)?'\s*:" % "|".join(re.escape(p) for p in paths))
lit_re = re.compile(r"^\s*'(%s)(/[^']*)?'\s*,?\s*$" % "|".join(re.escape(p) for p in paths))

i = 0
removed_keys = removed_lits = 0
while i < len(lines):
    line = lines[i]
    if lit_re.match(line):
        drop[i] = True
        removed_lits += 1
        i += 1
        continue
    if not key_re.match(line):
        i += 1
        continue
    removed_keys += 1
    # Consume until brackets opened on the key line are balanced *and* the
    # entry has been terminated by a comma at depth zero.
    depth = 0
    while i < len(lines):
        text = re.sub(r"'[^']*'", "''", lines[i])
        depth += text.count("{") + text.count("[") + text.count("(")
        depth -= text.count("}") + text.count("]") + text.count(")")
        drop[i] = True
        done = depth <= 0 and text.rstrip().endswith(",")
        i += 1
        if done:
            break

out = [l for l, d in zip(lines, drop) if not d]
open(DEPS, "w", encoding="utf-8", newline="").write("\n".join(out))
print("removed %d dep entr(ies) and %d list mention(s); %d -> %d lines"
      % (removed_keys, removed_lits, len(lines), len(out)))
