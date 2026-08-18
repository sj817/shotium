"""Delete the WebGPU/Dawn implementation from //gpu.

Dawn itself was removed in wave 3, but the client, service, command-buffer and
shared-image code that drives it stayed. It does not fail at `gn gen`, because
GN never checks that a source exists, and it does not fail as a missing
dependency either: the tell is an angle-bracket include,

    #include <dawn/dawn_proc_table.h>

which strip_component.py cannot see -- its pattern only ever matched the quoted
form. That is why this survived three convergence rounds.

Files are selected by name, which is reliable here because Chromium names every
one of these after the feature (webgpu_*, dawn_*, *_dawn_*). Files that merely
mention wgpu:: are left alone; those are the ones with a Dawn *section* inside
otherwise-needed code, and they are edited by hand.

Usage:
  cut_webgpu.py [--apply]
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
GPU = os.path.join(ROOT, "gpu")

NAME = re.compile(r"(^|_)(webgpu|dawn)(_|$)|_dawn_|dawn_", re.I)


def main():
    apply_changes = "--apply" in sys.argv
    victims = []
    for dirpath, dirnames, filenames in os.walk(GPU):
        for fn in filenames:
            if not fn.endswith((".cc", ".h")):
                continue
            stem = fn.rsplit(".", 1)[0]
            if not NAME.search(stem):
                continue
            victims.append(os.path.join(dirpath, fn))

    total = 0
    for path in sorted(victims):
        total += os.path.getsize(path)
        print("  %s" % os.path.relpath(path, ROOT).replace("\\", "/"))
        if apply_changes:
            os.remove(path)
    print("---- %d file(s), %.2f MiB%s"
          % (len(victims), total / 1048576.0,
             ", deleted" if apply_changes else ""))


main()
