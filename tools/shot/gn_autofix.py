"""Iterate `gn gen`, deleting dependency lines that point at deleted targets.

Usage: gn_autofix.py <out-dir> [max-rounds]

GN reports one load failure per run, and each report pins the exact file and
line of the offending reference:

    ERROR at //components/foo/BUILD.gn:26:5: Unable to load ".../BUILD.gn".
        "//components/bar",
        ^------------------

When that line is a bare label inside a list, the fix is always the same:
delete it. This loop does exactly that and nothing else. It stops and prints
the error whenever the pinned line is anything but a bare label -- an import,
a template invocation, a variable assignment -- because those need a human
decision about the symbols the file was getting from it.

Every deletion is logged so the whole run can be reviewed afterwards, and the
loop stops on the first round that changes nothing, so it can never spin.
"""

import os
import re
import subprocess
import sys
import time

ROOT = r"D:\Github\chromium"
GN = os.path.join(ROOT, "buildtools", "win", "gn.exe")
OUT = sys.argv[1]
MAX = int(sys.argv[2]) if len(sys.argv) > 2 else 40

# ERROR at //path/to/BUILD.gn:26:5: Unable to load "...".
ERR = re.compile(r'^ERROR at //([^:]+):(\d+):\d+: (Unable to load|Could not load) ')
# GN has already pinned this exact line as the unresolvable reference, so the
# label form does not matter: absolute (//foo:bar), relative (../foo:bar) and
# same-file (:bar) are all equally safe to delete.
_L = r'"[A-Za-z0-9_./:+-]+"'
# A line holding just one label, as an element of a list...
BARE_LABEL = re.compile(r"^%s,?$" % _L)
# ...or a whole single-element list assignment, which is equally safe to drop
# because removing it leaves `deps`/`public_deps`/`data_deps` simply unextended.
INLINE_LIST = re.compile(
    r"^(?:public_deps|deps|data_deps|public_configs|configs|traits_public_deps"
    r"|traits_deps|export_header_deps|input_data_files|sources|inputs|data)"
    r"\s*\+?=\s*\[\s*%s\s*,?\s*\]$" % _L)


def droppable(text):
    return bool(BARE_LABEL.match(text) or INLINE_LIST.match(text))

# `gn gen` evaluates each toolchain definition in parallel, and several of the
# Windows toolchain variants run setup_toolchain.py, which writes environment.x86
# / environment.x64 into the out dir under a fixed name. Two of them landing at
# once loses the race and reports PermissionError. It is transient and unrelated
# to anything being edited here, so retry rather than treating it as a finding.
TRANSIENT = "PermissionError: [Errno 13] Permission denied: 'environment."


def gn_gen():
    for attempt in range(6):
        p = subprocess.run([GN, "gen", OUT], cwd=ROOT, capture_output=True,
                           text=True)
        out = (p.stdout or "") + (p.stderr or "")
        if p.returncode == 0 or TRANSIENT not in out:
            return p.returncode, out
        print("   (toolchain environment file race, retrying)")
        time.sleep(3)
    return p.returncode, out


fixed = []
for rnd in range(1, MAX + 1):
    rc, out = gn_gen()
    if rc == 0:
        print("\n=== gn gen PASSED after %d round(s)" % (rnd - 1))
        break
    m = None
    for line in out.splitlines():
        m = ERR.match(line.strip())
        if m:
            break
    if not m:
        print("\n=== round %d: not a load error, stopping\n" % rnd)
        print(out[:4000])
        break
    rel, lineno = m.group(1), int(m.group(2))
    fp = os.path.join(ROOT, rel.replace("/", os.sep))
    lines = open(fp, encoding="utf-8").readlines()
    target = lines[lineno - 1]
    if not droppable(target.strip()):
        print("\n=== round %d: %s:%d is not a droppable label, stopping" % (rnd, rel, lineno))
        print("    " + target.rstrip())
        print()
        print(out[:4000])
        break
    del lines[lineno - 1]
    open(fp, "w", encoding="utf-8", newline="").writelines(lines)
    fixed.append((rel, lineno, target.strip()))
    print("round %2d  %s:%d  %s" % (rnd, rel, lineno, target.strip()))
else:
    print("\n=== hit the %d round cap, still failing" % MAX)

print("\n---- deleted %d dependency line(s)" % len(fixed))
