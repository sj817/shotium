"""Erase every trace of a deleted component from the build files and sources.

Usage:
  strip_component.py <spec> [<spec> ...]

A spec is `label_prefix[,gni][,guard_var][,BUILDFLAG_NAME]`, for example:

  //components/vrp_flags,//components/vrp_flags/features.gni,enable_vrp_flags,ENABLE_VRP_FLAGS

Four things get removed, tree-wide:

  1. `import("<gni>")` lines.
  2. deps entries whose label starts with <label_prefix>, both the
     one-per-line form and single-element `deps += [ ... ]` assignments.
  3. `if (<guard_var>) { ... }` blocks, which are dead once the gni is gone.
  4. In .cc/.h: `#include "<dir>/..."` lines and
     `#if BUILDFLAG(<NAME>) ... #endif` blocks.

Deleting a component leaves references in three different languages spread
over dozens of files; doing them one at a time means one `gn gen` round per
reference. This does the whole component in a single pass and prints what it
touched.
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
SKIP_DIRS = {".git", "out"}


def parse(spec):
    parts = (spec.split(",") + ["", "", ""])[:4]
    return parts[0], parts[1], parts[2], parts[3]


specs = [parse(s) for s in sys.argv[1:]]
touched = {}

for dirpath, dirnames, filenames in os.walk(ROOT):
    dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
    if "depot_tools" in dirpath:
        continue
    for fn in filenames:
        is_gn = fn == "BUILD.gn" or fn.endswith(".gni")
        is_src = fn.endswith((".cc", ".h", ".mm"))
        if not (is_gn or is_src):
            continue
        fp = os.path.join(dirpath, fn)
        try:
            src = open(fp, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        out = src
        for label, gni, guard, flag in specs:
            if is_gn:
                if gni:
                    out = out.replace('import("%s")\n' % gni, "")
                    # The wrapped form GN's formatter produces for long paths.
                    out = re.sub(r'import\(\s*\n?\s*"%s"\)\n' % re.escape(gni),
                                 "", out)
                esc = re.escape(label)
                out = re.sub(r'^[ \t]*"%s[^"]*",?\n' % esc, "", out, flags=re.M)
                out = re.sub(
                    r'^[ \t]*(?:public_deps|deps|data_deps|public_configs|configs)'
                    r'\s*\+?=\s*\[\s*"%s[^"]*"\s*,?\s*\]\n' % esc,
                    "", out, flags=re.M)
                if guard:
                    out = re.sub(
                        r"[ \t]*if \(%s\) \{(?:[^{}]|\{[^{}]*\})*\}\n"
                        % re.escape(guard), "", out)
            elif is_src:
                inc = label.lstrip("/")
                out = re.sub(r'^#include "%s/[^"]*"\n' % re.escape(inc), "",
                             out, flags=re.M)
                # Vendored libraries are just as often included with angle
                # brackets, and matching only the quoted form is how the whole
                # WebGPU implementation in //gpu survived three convergence
                # rounds: `#include <dawn/dawn_proc_table.h>` was invisible.
                base = inc.rsplit("/", 1)[-1]
                out = re.sub(r'^#include <(?:%s|%s)/[^>]*>\n'
                             % (re.escape(inc), re.escape(base)), "",
                             out, flags=re.M)
                if flag:
                    out = re.sub(
                        r"^#if BUILDFLAG\(%s\)\n.*?^#endif[^\n]*\n"
                        % re.escape(flag), "", out, flags=re.M | re.S)
        if out != src:
            open(fp, "w", encoding="utf-8", newline="").write(out)
            rel = os.path.relpath(fp, ROOT).replace("\\", "/")
            touched[rel] = len(src) - len(out)

for rel in sorted(touched):
    print("  -%-6d %s" % (touched[rel], rel))
print("---- %d file(s) touched" % len(touched))
