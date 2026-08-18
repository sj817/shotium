"""Put back #include lines the component sweeps deleted, for a header prefix
whose files exist again today.

The sweeps that removed a component also removed every `#include "<prefix>/..."`
naming it. That is correct while the component is gone. It stops being correct
the moment the component comes back -- and components do come back, because
"working beats cleanly cut". `strip_component.py` deletes; nothing put the
includes back, so the restore looked complete (the directory is there, the GN
dep is there) while every consumer still failed to compile.

The failure does not name the include. It names the *symbol*:

    storage/browser/quota/quota_database.h(83,16):
        error: use of undeclared identifier 'BucketInfo'

so it reads like a missing type, not a missing line, and the file it points at
visibly has an include list that looks reasonable.

This walks the pristine revision, collects every include under <prefix>, and
re-inserts the ones that are (a) absent from the file now and (b) backed by a
header that exists on disk now. Condition (b) is what keeps this from undoing a
deliberate cut: a component that is still deleted contributes nothing.

Usage:
  restore_includes_by_prefix.py <prefix> [<path> ...] [--pristine <rev>] [-n]

  <prefix>    include path prefix, e.g. components/services/storage/public/cpp
  <path>      limit to these repo paths (default: whole tree)
  -n          dry run
"""

import io
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_PRISTINE = "c0bba1026178"  # upstream baseline: the clone root, no cuts.


def write_retry(path, text):
    """Write a source file that a compiler may currently have open.

    Windows fails the open-for-write with EINVAL -- not a sharing error, not
    EACCES -- while clang holds the header for reading, so this looks like a
    bad path rather than contention. Editing during a build is normal here
    (the builds are long), so retry rather than abort halfway through a sweep
    and leave the tree half-edited.
    """
    for _ in range(60):
        try:
            io.open(path, "w", encoding="utf-8", newline="").write(text)
            return
        except OSError:
            time.sleep(0.5)
    raise OSError("could not write %s; a compiler has held it for 30s" % path)


def leading_include_block_end(lines):
    """Index of the last #include of the block that opens the file.

    Stops at the first namespace/template/forward declaration so an include is
    never inserted into the body, and never past a conditional that starts a
    platform section.
    """
    last = -1
    for i, line in enumerate(lines):
        s = line.strip()
        if s.startswith("namespace ") or s.startswith("template ") or \
                s.startswith("BASE_") or s.startswith("COMPONENT_EXPORT"):
            break
        if s.startswith("#if") and last >= 0:
            # A conditional after real includes: everything below is guarded.
            break
        if s.startswith("#include "):
            last = i
    return last


def main(argv):
    if not argv:
        sys.exit(__doc__)
    dry = "-n" in argv
    argv = [a for a in argv if a != "-n"]
    pristine = DEFAULT_PRISTINE
    if "--pristine" in argv:
        i = argv.index("--pristine")
        pristine = argv[i + 1]
        del argv[i:i + 2]
    prefix = argv[0].rstrip("/")
    paths = argv[1:]

    cmd = ["git", "grep", "-n", 'include "%s/' % prefix, pristine]
    if paths:
        cmd += ["--"] + paths
    out = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True).stdout

    wanted = {}
    for line in out.splitlines():
        if not line.strip():
            continue
        _rev, rest = line.split(":", 1)
        rel, _lineno, text = rest.split(":", 2)
        wanted.setdefault(rel, []).append(text.rstrip())

    added = skipped = gone = 0
    for rel, incs in sorted(wanted.items()):
        path = os.path.join(ROOT, rel.replace("/", os.sep))
        if not os.path.exists(path):
            continue
        src = io.open(path, encoding="utf-8", newline="").read()
        nl = "\r\n" if "\r\n" in src else "\n"
        lines = src.split(nl)

        missing = []
        for inc in incs:
            header = inc.split('"')[1]
            # x.mojom.h / -blink.h / -forward.h / -shared.h are generated into
            # <out>/gen, so they are never on disk here. Their existence is
            # decided by whether the .mojom is still in the tree.
            probe = header
            if ".mojom" in header and header.endswith(".h"):
                probe = header[:header.index(".mojom") + len(".mojom")]
            if not os.path.exists(os.path.join(ROOT, probe.replace("/", os.sep))):
                gone += 1          # still cut; leaving it out is correct.
                continue
            if any(l.lstrip().startswith("#include") and header in l
                   for l in lines):
                skipped += 1
                continue
            missing.append(inc)
        if not missing:
            continue

        at = leading_include_block_end(lines)
        if at < 0:
            print("  MANUAL %s (no include block)" % rel)
            continue
        # Keep the block sorted: chromium include order is plain lexicographic
        # within the project group.
        block_start = at
        while block_start > 0 and lines[block_start - 1].lstrip().startswith(
                ("#include", "")) and lines[block_start - 1].strip():
            if not lines[block_start - 1].lstrip().startswith("#include"):
                break
            block_start -= 1
        for inc in missing:
            pos = at + 1
            for i in range(block_start, at + 1):
                if lines[i].lstrip().startswith('#include "') and lines[i] > inc:
                    pos = i
                    break
            lines.insert(pos, inc)
            at += 1
        if not dry:
            write_retry(path, nl.join(lines))
        added += len(missing)
        print("  +%-2d    %s" % (len(missing), rel))

    print("%d restored, %d already present, %d still-cut headers left out%s"
          % (added, skipped, gone, " (dry run)" if dry else ""))


if __name__ == "__main__":
    main(sys.argv[1:])
