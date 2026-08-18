"""Remove references to a dead DevTools protocol type from .pdl domains.

The browser protocol's domains refer to types declared in v8's js_protocol.pdl
-- Runtime.RemoteObjectId, Runtime.StackTrace and friends. V8 is deleted and v8/
was a git submodule, so that file cannot be recovered even as data. The
inspector protocol's *compatibility check* can be told to accept the missing
types via --expected_errors, but the code *generator* cannot: it does a direct
dictionary lookup and dies with `KeyError: 'Runtime.RemoteObjectId'`. The
references have to go.

.pdl is indentation structured:

    domain DOM
      type Node extends object
        properties
          NodeId nodeId
      command requestNode
        parameters
          Runtime.RemoteObjectId objectId
        returns
          NodeId nodeId

so a reference is classified by what encloses it, and that decides the edit:

  optional entry        drop the entry (and the comment block above it). An
                        optional parameter that is never sent changes nothing
                        for any other field.
  required entry, in a  drop the whole command or event. A command whose
  command/event         required argument cannot exist is not callable, and
                        leaving it would generate a signature naming a type
                        that was never generated.
  required entry, in a  reported, not edited. Dropping a required property
  type's properties     silently changes a struct every other domain may embed;
                        that is a decision, not a mechanical edit.

Prints what it did per file. Read the diff.

Usage:
  pdl_drop_type.py <TypePrefix> <dir> [--apply]

  <TypePrefix>  e.g. "Runtime." -- matched against the entry's type token
"""

import os
import re
import sys


def indent_of(line):
    return len(line) - len(line.lstrip(" "))


def block_end(lines, start, base):
    """Index one past the last line belonging to the block opened at `start`."""
    i = start + 1
    while i < len(lines):
        line = lines[i]
        if line.strip() and indent_of(line) <= base:
            break
        i += 1
    return i


def comment_start(lines, i):
    """Walk back over the comment block immediately above line i."""
    while i > 0 and lines[i - 1].lstrip().startswith("#"):
        i -= 1
    return i


def process(path, prefix, apply_changes):
    lines = open(path, encoding="utf-8").read().split("\n")
    drop = [False] * len(lines)
    notes = []

    for i, line in enumerate(lines):
        if prefix not in line or line.lstrip().startswith("#"):
            continue
        stripped = line.strip()
        # Only entries, i.e. `[optional] Type name` or `[optional] array of Type`.
        if not re.match(r"(?:optional\s+)?(?:array of\s+)?%s" % re.escape(prefix),
                        stripped):
            # A `command x` line cannot name a type, so anything else here is a
            # reference in a position this does not model. Say so rather than
            # guessing.
            notes.append("    ? unmodelled reference at line %d: %s"
                         % (i + 1, stripped))
            continue

        optional = stripped.startswith("optional ")

        # Find the enclosing member (command/event/type) by walking back to the
        # nearest line indented less than the section header above this entry.
        j = i
        member = None
        while j > 0:
            j -= 1
            s = lines[j].strip()
            if not s or s.startswith("#"):
                continue
            if indent_of(lines[j]) < indent_of(line):
                if re.match(r"(command|event|type)\s", s):
                    member = (j, s.split()[0])
                    break
        if member is None:
            notes.append("    ? no enclosing member for line %d" % (i + 1))
            continue

        if optional:
            start = comment_start(lines, i)
            for n in range(start, i + 1):
                drop[n] = True
            notes.append("    - optional entry line %d (%s)" % (i + 1, stripped))
            continue

        mstart, kind = member
        if kind == "type":
            notes.append("    ! REQUIRED property of a type, left alone: "
                         "line %d (%s) in %s"
                         % (i + 1, stripped, lines[mstart].strip()))
            continue

        start = comment_start(lines, mstart)
        end = block_end(lines, mstart, indent_of(lines[mstart]))
        for n in range(start, end):
            drop[n] = True
        notes.append("    - whole %s: %s" % (kind, lines[mstart].strip()))

    if not notes:
        return 0
    print("%s" % os.path.basename(path))
    for n in notes:
        print(n)
    removed = sum(drop)
    if apply_changes and removed:
        out = [l for l, d in zip(lines, drop) if not d]
        open(path, "w", encoding="utf-8", newline="").write("\n".join(out))
    return removed


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) < 2:
        sys.exit(__doc__)
    prefix, directory = args[0], args[1]
    apply_changes = "--apply" in sys.argv
    total = 0
    for fn in sorted(os.listdir(directory)):
        if fn.endswith(".pdl"):
            total += process(os.path.join(directory, fn), prefix, apply_changes)
    print("---- %d line(s)%s" % (total, " removed" if apply_changes else ""))


main()
