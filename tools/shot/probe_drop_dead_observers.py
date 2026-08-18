"""Remove probe observers whose implementation header is gone.

blink's probe instrumentation is generated from core_probes.pidl plus
core_probes.json5 by build/scripts/make_instrumenting_probes.py. The json5
lists, for each probe, which classes observe it; the generator emits
core_probes_impl.cc with **one #include per observer** and a dispatch loop that
calls each one.

So an observer whose class was deleted is not a dead entry that sits there
harmlessly -- it is an #include of a header that does not exist, in a generated
file, reported as

    gen/third_party/blink/renderer/core/core_probes_impl.cc(N,10):
        fatal error: '.../inspector_page_agent.h' file not found

which points at generated code and gives no hint that the cause is three lines
of json5.

The probes themselves are left alone. A probe with no observers still compiles
and still costs nothing; removing it would mean editing every call site in
core, and a probe is exactly the kind of thing that comes back.

Usage:
  probe_drop_dead_observers.py <core_probes.json5> [-n]
"""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_INCLUDE_PATH = "third_party/blink/renderer/core/inspector"


def snake(name):
    name = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name)
    name = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", "_", name)
    return name.lower()


def observer_blocks(lines, start):
    """Yield (name, first, last) for each observer entry in the block."""
    depth = 0
    name = None
    first = None
    for i in range(start, len(lines)):
        stripped = lines[i].strip()
        if depth == 0:
            m = re.match(r"(\w+):\s*\{", stripped)
            if m:
                name, first = m.group(1), i
                depth = 1
                depth += lines[i].count("{") - 1 - lines[i].count("}")
                if depth == 0:
                    yield name, first, i
                    name = None
                continue
            if stripped in ("}", "},"):
                return
            continue
        depth += lines[i].count("{") - lines[i].count("}")
        if depth == 0:
            yield name, first, i
            name = None


def main(argv):
    dry = "-n" in argv
    argv = [a for a in argv if a != "-n"]
    if not argv:
        sys.exit(__doc__)
    path = argv[0]

    src = io.open(path, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in src else "\n"
    lines = src.split(nl)

    start = next(i for i, l in enumerate(lines)
                 if l.strip().startswith("observers:")) + 1

    drop = []
    for name, first, last in observer_blocks(lines, start):
        body = nl.join(lines[first:last + 1])
        m = re.search(r'include_path:\s*"([^"]+)"', body)
        include_path = m.group(1) if m else DEFAULT_INCLUDE_PATH
        header = os.path.join(ROOT, include_path.replace("/", os.sep),
                              snake(name) + ".h")
        if os.path.exists(header):
            continue
        # Take any comment lines immediately above the entry with it.
        top = first
        while top > start and (lines[top - 1].strip().startswith("//")
                               or not lines[top - 1].strip()):
            top -= 1
        drop.append((top, last, name, include_path))

    for _t, _l, name, include_path in drop:
        print("  drop   %-42s (no %s/%s.h)" % (name, include_path, snake(name)))

    for top, last, _n, _p in reversed(drop):
        del lines[top:last + 1]

    if not dry:
        io.open(path, "w", encoding="utf-8", newline="").write(nl.join(lines))
    print("%d observer(s) dropped%s" % (len(drop), " (dry run)" if dry else ""))


if __name__ == "__main__":
    main(sys.argv[1:])
