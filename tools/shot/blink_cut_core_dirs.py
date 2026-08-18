"""Delete blink core subdirectories and unhook them from core/BUILD.gn.

Deleting the directory alone is never enough: core/BUILD.gn pulls one source
list and one test list per subdirectory,

    sources += rebase_path(blink_core_sources_css, "", "css")
    sources += rebase_path(blink_core_tests_css, "", "css")

and those statements outlive the build.gni that defined the variables, so GN
fails with an undefined identifier -- one per `gn gen`, at 25 seconds each.
Doing both halves in one operation is what keeps that from happening.

The statements wrap across lines when the name is long, so the source-list
removal matches to the closing paren rather than to end of line.

Usage:
  blink_cut_core_dirs.py <subdir> [<subdir> ...]
  blink_cut_core_dirs.py --unhook-only <subdir> [...]   # directory already gone
"""

import os
import re
import shutil
import sys

CORE = r"D:\Github\chromium\third_party\blink\renderer\core"
BUILD = os.path.join(CORE, "BUILD.gn")

args = sys.argv[1:]
unhook_only = "--unhook-only" in args
names = [a for a in args if not a.startswith("--")]
if not names:
    sys.exit(__doc__)

src = open(BUILD, encoding="utf-8").read()
out = src
for name in names:
    path = os.path.join(CORE, name)
    files = 0
    size = 0
    if os.path.isdir(path):
        for dp, dn, fns in os.walk(path):
            for fn in fns:
                try:
                    size += os.path.getsize(os.path.join(dp, fn))
                    files += 1
                except OSError:
                    pass
        if not unhook_only:
            shutil.rmtree(path, ignore_errors=True)

    hooks = 0
    for kind in ("sources", "tests"):
        pat = re.compile(
            r"^[ \t]*sources \+=\s*\n?[ \t]*"
            r"rebase_path\(blink_core_%s_%s,(?:[^()]|\([^()]*\))*\)\n"
            % (kind, re.escape(name)), re.M)
        out, n = pat.subn("", out)
        hooks += n
    # The per-directory build.gni import. `gn format` puts the path on its own
    # line when the name is long, so the paren and the string may be separated.
    imp = re.compile(
        r'^[ \t]*import\(\s*'
        r'"//third_party/blink/renderer/core/%s/build\.gni"\)\n'
        % re.escape(name), re.M)
    out, n = imp.subn("", out)

    print("%-28s %5d files %8.2f MiB  %d hook(s) %s import"
          % (name, files, size / 1048576.0, hooks, "-" if n else "no"))

open(BUILD, "w", encoding="utf-8", newline="").write(out)
print("---- %d subdirector%s"
      % (len(names), "y" if len(names) == 1 else "ies"))
