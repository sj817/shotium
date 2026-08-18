"""Drop mojom typemap entries whose traits header no longer exists.

A typemap says "wire type blink.mojom.Manifest is C++ type ::blink::Manifest,
and here is the header that converts between them". When a component is cut, the
traits header goes with it but the typemap entry stays behind in BUILD.gn. mojom
then generates a header that names a type nobody declares:

    gen/.../manifest.mojom.h(63,26):
        error: 'Manifest' is not a class, namespace, or enumeration

which points at generated code, so it reads like a codegen bug rather than a
stale three lines of GN.

Without the typemap the mojom is not broken -- it generates its own struct, the
same one every non-typemapped mojom struct gets. That is the correct end state
for a cut feature: the wire format is unchanged, and nothing on either side of
it is left to convert to.

Two shapes are dead, and the second is the common one here:

  * the block still lists a traits header that is gone;
  * the block lists *no* header at all, because an earlier sweep emptied
    `traits_headers = []` and left the `cpp =` lines behind. A typemap with no
    header cannot work -- the StructTraits/EnumTraits specialisation has to come
    from somewhere -- so an empty header set is not a valid configuration, it is
    a half-finished cut.

Blocks are matched by brace depth inside a `*cpp_typemaps = [ ... ]` list, not
by regex over the whole file, because the entries nest three levels
(list -> block -> types -> type).

Usage:
  gn_drop_dead_typemaps.py <BUILD.gn> [...] [-n]
"""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TYPEMAP_LIST = re.compile(r"^\s*(?:\w+_)?cpp_typemaps\s*(?:\+)?=\s*\[\s*$")
# traits_sources counts as well as traits_headers: a block can keep a header
# that happens to still exist (common_export.h) while the .cc that actually
# implements the traits is gone, which ninja reports as a missing input rather
# than a compile error.
HEADER = re.compile(r'"//([^"]+\.(?:h|cc))"')
MOJOM = re.compile(r'mojom\s*=\s*"([^"]+)"')


def block_ranges(lines, start):
    """Yield (first, last) line indices of each top-level { } inside a list.

    `start` is the index of the line opening the list. Stops at the line that
    closes it.
    """
    depth = 0
    open_at = None
    i = start + 1
    while i < len(lines):
        s = lines[i].strip()
        if depth == 0 and s in ("]", "],"):
            return
        for ch in lines[i]:
            if ch == "{":
                if depth == 0:
                    open_at = i
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    yield open_at, i
        i += 1


def main(argv):
    dry = "-n" in argv
    files = [a for a in argv if a != "-n"]
    if not files:
        sys.exit(__doc__)

    for path in files:
        src = io.open(path, encoding="utf-8", newline="").read()
        nl = "\r\n" if "\r\n" in src else "\n"
        lines = src.split(nl)

        drop = []  # (first, last, [types], [missing headers])
        for i, line in enumerate(lines):
            if not TYPEMAP_LIST.match(line):
                continue
            for first, last in block_ranges(lines, i):
                body = nl.join(lines[first:last + 1])
                headers = HEADER.findall(body)
                missing = [h for h in headers
                           if not os.path.exists(
                               os.path.join(ROOT, h.replace("/", os.sep)))]
                if not headers:
                    missing = ["<no traits header at all>"]
                if missing:
                    drop.append((first, last, MOJOM.findall(body), missing))

        if not drop:
            print("  ok     %s" % os.path.relpath(path, ROOT))
            continue

        for first, last, types, missing in drop:
            print("  drop   %s" % ", ".join(types))
            for h in missing:
                print("           (no %s)" % h)

        for first, last, _t, _m in reversed(drop):
            end = last
            # Swallow a trailing comma line if the block closes with "},".
            while end + 1 < len(lines) and lines[end + 1].strip() == "":
                end += 1
            del lines[first:end + 1]

        if not dry:
            io.open(path, "w", encoding="utf-8", newline="").write(nl.join(lines))
        print("  -%-2d    %s%s" % (len(drop), os.path.relpath(path, ROOT),
                                   " (dry run)" if dry else ""))


if __name__ == "__main__":
    main(sys.argv[1:])
