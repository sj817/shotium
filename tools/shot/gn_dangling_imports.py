"""List every `import("//...")` in the tree whose target file no longer exists.

GN reports one load error per `gn gen` run, so converging by re-running gn is
one 25-second round per dangling import. This finds all of them at once.

Usage:
  gn_dangling_imports.py            # report
  gn_dangling_imports.py --delete   # also delete the import lines
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
SKIP_DIRS = {".git", "out"}

# Vendored projects that also build standalone. Their BUILD files resolve `//`
# against their own root, so an import that looks dangling from here is fine
# there, and Chromium's `gn gen` never loads those files anyway -- if it did,
# GN would already be erroring on them.
SKIP_TREES = tuple(os.path.join(ROOT, p) for p in (
    r"third_party\angle",
    r"third_party\skia",
    r"third_party\crashpad",
    r"third_party\mini_chromium",
    r"third_party\OpenCL-CTS",
    r"third_party\clspv",
    r"third_party\swiftshader",
    r"third_party\fuchsia-sdk",
))
# `gn format` wraps a long path onto its own line, so the open paren and the
# string are not always on the same line:
#     import(
#         "//third_party/blink/renderer/core/lcp_critical_path_predictor/build.gni")
IMPORT = re.compile(r'^[ \t]*import\(\s*"(//[^"]+)"\)\n', re.M)

delete = "--delete" in sys.argv
missing = {}

for dirpath, dirnames, filenames in os.walk(ROOT):
    dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
    if "depot_tools" in dirpath or dirpath.startswith(SKIP_TREES):
        continue
    for fn in filenames:
        if fn != "BUILD.gn" and not fn.endswith(".gni"):
            continue
        fp = os.path.join(dirpath, fn)
        try:
            src = open(fp, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        gone = [label for label in IMPORT.findall(src)
                if not os.path.exists(os.path.join(ROOT, label[2:].replace("/", os.sep)))]
        if not gone:
            continue
        rel = os.path.relpath(fp, ROOT).replace("\\", "/")
        missing[rel] = gone
        if delete:
            out = src
            for label in set(gone):
                out = re.sub(r'^[ \t]*import\(\s*"%s"\)\n' % re.escape(label),
                             "", out, flags=re.M)
            open(fp, "w", encoding="utf-8", newline="").write(out)

by_label = {}
for rel, labels in missing.items():
    for label in labels:
        by_label.setdefault(label, []).append(rel)

for label in sorted(by_label, key=lambda x: -len(by_label[x])):
    print("%4d  %s" % (len(by_label[label]), label))
print("---- %d dangling import(s) in %d file(s)%s"
      % (sum(len(v) for v in by_label.values()), len(missing),
         ", deleted" if delete else ""))
