"""Remove C++ declarations that mention a dead type.

After a subsystem is cut, the types it defined are still named in declarations
all over the code that used to call it -- `virtual std::unique_ptr<
DawnImageRepresentation> ProduceDawn(..., const wgpu::Device&, ...);` and its
overrides. Deleting the line the compiler points at is never enough: these
signatures wrap across five or six lines, and half a signature is worse than
all of it.

So the unit of removal is the declaration, not the line. From a matching line
this walks back to the end of the previous statement and forward to the `;`
that ends this one, brace-matching if the declaration has a body, and takes any
comment block immediately above it as well.

Forward declarations (`class DawnImageRepresentation;`) and enumerator lines
are handled by the same rule, since both are single-line statements.

The backward walk stops at a comment line as well as at `;`/`{`/`}`/`:`/`#`,
and that is not a nicety. Without it, a declaration written as

    // Comment for the previous method.
    void Previous(Args...)
        override;

    void Doomed(MediaType);

walks back past `override;`, past the blank line, through the previous
declaration and into its comment, and takes the lot. It cost four hand repairs
in renderer_blink_platform_impl.h and browser_main_loop.h before it was found --
the tell is a *neighbouring* declaration losing its trailing `override;`, which
compiles as something else entirely rather than failing where the edit was.

This does not know C++. It is a way to make a mechanical edit consistently
across dozens of files; read the diff, every time.

Usage:
  cpp_drop_decls.py <pattern> <file> [<file> ...] [--apply]

  <pattern>  Python regex, e.g. "wgpu::|DawnImageRepresentation"
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"


def statement_bounds(lines, i):
    """Return (start, end) line indices of the declaration containing line i."""
    start = i
    while start > 0:
        prev = lines[start - 1].rstrip()
        if (not prev or prev.endswith((";", "{", "}", ":")) or
                prev.lstrip().startswith("//") or
                prev.lstrip().startswith("#")):
            break
        start -= 1
    # Take a comment block directly above.
    while start > 0:
        above = lines[start - 1].lstrip()
        if above.startswith("//"):
            start -= 1
        else:
            break

    end = i
    depth = 0
    while end < len(lines):
        line = lines[end]
        depth += line.count("{") - line.count("}")
        stripped = line.rstrip()
        if depth <= 0 and (stripped.endswith(";") or stripped.endswith("}")):
            break
        end += 1
    return start, min(end, len(lines) - 1)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) < 2:
        sys.exit(__doc__)
    pattern = re.compile(args[0])
    apply_changes = "--apply" in sys.argv

    for rel in args[1:]:
        path = os.path.join(ROOT, rel.replace("/", os.sep))
        if not os.path.exists(path):
            print("MISSING: %s" % rel)
            continue
        lines = open(path, encoding="utf-8").read().split("\n")
        drop = [False] * len(lines)
        for i, line in enumerate(lines):
            if drop[i] or not pattern.search(line):
                continue
            start, end = statement_bounds(lines, i)
            for n in range(start, end + 1):
                drop[n] = True
        removed = sum(drop)
        if not removed:
            print("  0  %s" % rel)
            continue
        out = [l for l, d in zip(lines, drop) if not d]
        print("%3d  %s" % (removed, rel))
        if apply_changes:
            open(path, "w", encoding="utf-8", newline="").write("\n".join(out))


main()
