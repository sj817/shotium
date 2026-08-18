"""Remove a deleted blink core subdirectory from core/BUILD.gn.

core/BUILD.gn pulls one source list and one test list per subdirectory:

    sources += rebase_path(blink_core_sources_css, "", "css")
    sources += rebase_path(blink_core_tests_css, "", "css")

Deleting the subdirectory takes its build.gni -- and therefore both variables --
with it, but the statements survive and GN then fails on an undefined
identifier, one per `gn gen`. The statements wrap across lines when the name is
long, so this matches to the closing paren rather than to end of line.

Usage:
  blink_drop_core_dir.py <subdir> [<subdir> ...]
"""

import re
import sys

PATH = r"D:\Github\chromium\third_party\blink\renderer\core\BUILD.gn"
names = sys.argv[1:]
if not names:
    sys.exit(__doc__)

src = open(PATH, encoding="utf-8").read()
out = src
report = []
for name in names:
    hits = 0
    for kind in ("sources", "tests"):
        pat = re.compile(
            r"^[ \t]*sources \+=\s*\n?[ \t]*"
            r"rebase_path\(blink_core_%s_%s,(?:[^()]|\([^()]*\))*\)\n"
            % (kind, re.escape(name)), re.M)
        out, n = pat.subn("", out)
        hits += n
    report.append("%s:%d" % (name, hits))
    if not hits:
        print("NO MATCH: %s" % name)

open(PATH, "w", encoding="utf-8", newline="").write(out)
print("removed %s" % " ".join(report))
