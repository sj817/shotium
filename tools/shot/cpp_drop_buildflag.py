"""Remove `#if BUILDFLAG(NAME)` blocks for buildflags that no longer exist.

When a subsystem goes, its generated buildflag header goes with it, but the
`#if BUILDFLAG(USE_VAAPI)` sites scattered through surviving files do not. The
diagnostic is unhelpful -- clang reports

    function-like macro 'BUILDFLAG_INTERNAL_USE_VAAPI' is not defined

which names a macro nobody wrote, because BUILDFLAG(X) expands to
BUILDFLAG_INTERNAL_##X().

This is the C++ counterpart of gn_drop_if.py, and it has the same rule about
which side to keep: the flag is false, so the `#if` body is dead and an `#else`
body is exactly what the build should now take. `#elif` becomes the new `#if`.

Nested `#if`s are tracked by depth, so a block containing its own conditionals
is removed whole rather than up to the first inner `#endif`.

Block removal only handles `#if BUILDFLAG(X)` on its own. For compound
conditions -- `#if BUILDFLAG(USE_VAAPI) && BUILDFLAG(USE_V4L2_CODEC)`, or an
`#elif` chain -- use --substitute, which rewrites every `BUILDFLAG(X)` for a
dead flag to `0` and lets the preprocessor evaluate what is left.

Substituting *all* the dead flags together is not optional. `#if 0 &&
BUILDFLAG(USE_V4L2_CODEC)` still fails: the preprocessor expands macros across
the whole line before evaluating, so `&&` does not spare the second operand
from being an undefined function-like macro.

Usage:
  cpp_drop_buildflag.py <FLAG> [<FLAG> ...] [--apply] [--root <dir>]
  cpp_drop_buildflag.py <FLAG> [...] --substitute [--apply]
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
SKIP_DIRS = {".git", "out"}


def strip_blocks(lines, flag):
    """Return (new_lines, count) with every `#if BUILDFLAG(flag)` block gone."""
    opener = re.compile(r"^\s*#if\s+BUILDFLAG\(\s*%s\s*\)\s*$" % re.escape(flag))
    out = []
    i = 0
    removed = 0
    while i < len(lines):
        if not opener.match(lines[i]):
            out.append(lines[i])
            i += 1
            continue

        removed += 1
        depth = 1
        i += 1
        body_start = i
        else_body = None
        while i < len(lines) and depth:
            line = lines[i]
            stripped = line.lstrip()
            if stripped.startswith("#if"):
                depth += 1
            elif stripped.startswith("#endif"):
                depth -= 1
                if depth == 0:
                    i += 1
                    break
            elif depth == 1 and stripped.startswith("#elif"):
                # This arm becomes the leading condition.
                else_body = ["#if" + stripped[len("#elif"):]]
                body_start = i + 1
                else_body = None
                out.append(re.sub(r"^(\s*)#elif", r"\1#if", line))
                depth = 0
                i += 1
                break
            elif depth == 1 and stripped.startswith("#else"):
                else_body = []
                body_start = i + 1
            elif else_body is not None:
                else_body.append(line)
            i += 1
        if else_body:
            out.extend(else_body)
    return out, removed


def main():
    flags = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not flags:
        sys.exit(__doc__)
    apply_changes = "--apply" in sys.argv
    substitute = "--substitute" in sys.argv
    root = ROOT
    if "--root" in sys.argv:
        root = os.path.join(ROOT, sys.argv[sys.argv.index("--root") + 1])

    mentions = re.compile(r"BUILDFLAG\(\s*(?:%s)\s*\)"
                          % "|".join(re.escape(f) for f in flags))
    touched = {}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        if "depot_tools" in dirpath:
            continue
        for fn in filenames:
            if not fn.endswith((".cc", ".h", ".mm", ".cpp")):
                continue
            fp = os.path.join(dirpath, fn)
            try:
                src = open(fp, encoding="utf-8").read()
            except (OSError, UnicodeDecodeError):
                continue
            if not mentions.search(src):
                continue
            if substitute:
                out, total = mentions.subn("0", src)
                lines = out.split("\n")
            else:
                lines = src.split("\n")
                total = 0
                for flag in flags:
                    lines, n = strip_blocks(lines, flag)
                    total += n
            if not total:
                continue
            touched[os.path.relpath(fp, ROOT).replace("\\", "/")] = total
            if apply_changes:
                open(fp, "w", encoding="utf-8", newline="").write(
                    "\n".join(lines))

    for rel in sorted(touched, key=lambda r: -touched[r]):
        print("%3d  %s" % (touched[rel], rel))
    print("---- %d block(s) in %d file(s)%s"
          % (sum(touched.values()), len(touched),
             ", removed" if apply_changes else ""))


main()
