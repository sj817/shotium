"""Delete `if` blocks whose condition mentions a now-undefined GN variable.

When a `.gni` is deleted its `declare_args()` names go with it, and every
`if (that_name)` left behind is a hard error. The feature is gone, so the
if-body is dead; an `else` body, however, is exactly what the build should now
take, so it is inlined rather than dropped.

Handles the three shapes that occur:

    if (X) { ... }                  -> deleted
    if (X) { ... } else { BODY }    -> BODY  (GN if/else share the enclosing
                                              scope, so inlining is exact)
    if (X) { ... } else if (Y) {    -> if (Y) {

Usage:
  gn_drop_if.py <var> [<var> ...]
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
SKIP_DIRS = {".git", "out"}

names = sys.argv[1:]
if not names:
    sys.exit(__doc__)
COND = re.compile(r"^([ \t]*)if[ \t]*\(([^\n]*?)\)[ \t]*\{[ \t]*\n", re.M)
MENTIONS = re.compile(r"\b(?:%s)\b" % "|".join(re.escape(n) for n in names))


def match_brace(src, open_idx):
    """Index just past the `}` closing the `{` at open_idx, or None."""
    depth = 0
    i = open_idx
    while i < len(src):
        c = src[i]
        if c == '"':
            i += 1
            while i < len(src) and src[i] != '"':
                i += 2 if src[i] == "\\" else 1
        elif c == "#":
            i = src.find("\n", i)
            if i < 0:
                return None
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return None


def strip_one(src):
    for m in COND.finditer(src):
        cond = m.group(2)
        if not MENTIONS.search(cond):
            continue
        # `A || gone || B` is not false just because `gone` is: only that term
        # is. Drop the term and keep the block. (An `&&` compound *is* false,
        # so it falls through to the block-removal path below.)
        if "||" in cond:
            kept = [t for t in cond.split("||") if not MENTIONS.search(t)]
            if kept:
                new = "||".join(kept).strip()
                return (src[:m.start(2)] + new + src[m.end(2):]), True
        indent = m.group(1)
        open_idx = src.index("{", m.start())
        end = match_brace(src, open_idx)
        if end is None:
            continue
        tail = src[end:]
        rest = tail.lstrip(" \t")
        if rest.startswith("else if"):
            # Drop this arm; the next one becomes the leading `if`.
            return src[:m.start()] + indent + rest[len("else "):], True
        if rest.startswith("else"):
            body_open = end + tail.index("{")
            body_end = match_brace(src, body_open)
            if body_end is None:
                continue
            body = src[body_open + 1:body_end - 1].strip("\n")
            body = "\n".join(l[2:] if l.startswith("  ") else l
                             for l in body.split("\n"))
            after = src[body_end:].lstrip(" \t")
            return src[:m.start()] + body + "\n" + after, True
        # Plain if: drop it and the newline that followed the closing brace.
        after = src[end:]
        if after.startswith("\n"):
            after = after[1:]
        return src[:m.start()] + after, True
    return src, False


touched = {}
for dirpath, dirnames, filenames in os.walk(ROOT):
    dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
    if "depot_tools" in dirpath:
        continue
    for fn in filenames:
        if fn != "BUILD.gn" and not fn.endswith(".gni"):
            continue
        fp = os.path.join(dirpath, fn)
        try:
            src = open(fp, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        if not MENTIONS.search(src):
            continue
        out, n = src, 0
        while True:
            out, changed = strip_one(out)
            if not changed:
                break
            n += 1
        if out != src:
            open(fp, "w", encoding="utf-8", newline="").write(out)
            touched[os.path.relpath(fp, ROOT).replace("\\", "/")] = n

for rel in sorted(touched):
    print("  %2d  %s" % (touched[rel], rel))
print("---- %d block(s) in %d file(s)"
      % (sum(touched.values()), len(touched)))
