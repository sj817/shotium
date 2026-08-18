"""Put back GN deps entries that strip_component.py removed.

The companion to restore_includes.py. Restoring a component needs three things
done, and doing only some of them fails in ways that point away from the cause:

  1. the directory back on disk        -- otherwise `gn gen` fails, loudly
  2. its `deps` entries back           -- otherwise the generated headers are
                                          never built, and the error is
                                          "'...mojom-forward.h' file not found"
                                          in a consumer that looks unrelated
  3. its `#include` lines back         -- otherwise the symbol is undeclared at
                                          the point of use, naming the symbol
                                          rather than the missing header

Each restored entry is put back after the line that preceded it in the original
file, keeping the list in its original order.

Usage:
  restore_gn_deps.py <git-rev> <label-prefix> [<label-prefix> ...] [--apply]

  <git-rev>       revision holding the pre-deletion contents, e.g. abc1234^
  <label-prefix>  e.g. //services/metrics
"""

import os
import re
import subprocess
import sys

ROOT = r"D:\Github\chromium"


def git(*args):
    return subprocess.run(["git"] + list(args), cwd=ROOT, capture_output=True,
                          text=True, encoding="utf-8", errors="replace").stdout


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) < 2:
        sys.exit(__doc__)
    rev, prefixes = args[0], args[1:]
    apply_changes = "--apply" in sys.argv
    entry = re.compile(r'^\s*"(?:%s)[^"]*",\s*$'
                       % "|".join(re.escape(p) for p in prefixes))

    changed = git("diff", "--name-only", rev, "--", "*.gn", "*.gni")
    touched = 0
    restored = 0
    for rel in changed.split("\n"):
        rel = rel.strip()
        if not rel:
            continue
        path = os.path.join(ROOT, rel.replace("/", os.sep))
        if not os.path.exists(path):
            continue
        original = git("show", "%s:%s" % (rev, rel))
        if not original:
            continue
        old_lines = original.split("\n")
        try:
            cur_lines = open(path, encoding="utf-8").read().split("\n")
        except (OSError, UnicodeDecodeError):
            continue
        cur_set = set(cur_lines)

        additions = []
        for i, line in enumerate(old_lines):
            if entry.match(line) and line not in cur_set:
                additions.append((old_lines[i - 1] if i else None, line))
        if not additions:
            continue
        touched += 1
        restored += len(additions)
        print("%3d  %s" % (len(additions), rel))
        if not apply_changes:
            continue
        for predecessor, line in additions:
            # Only anchor on a predecessor that appears exactly once. A common
            # line like `"//base",` occurs in every deps list in the file, and
            # list.index() returns the first, which lands the entry in the
            # wrong target -- that is how //components/unexportable_keys:test_support
            # ended up in the non-testonly //net:net.
            if predecessor is not None and cur_lines.count(predecessor) == 1:
                cur_lines.insert(cur_lines.index(predecessor) + 1, line)
            else:
                # No anchor left; append to the first deps list in the file.
                for n, l in enumerate(cur_lines):
                    if re.match(r"^\s*deps\s*\+?=\s*\[\s*$", l):
                        cur_lines.insert(n + 1, line)
                        break
        open(path, "w", encoding="utf-8", newline="").write(
            "\n".join(cur_lines))

    print("---- %d entr(ies) in %d file(s)%s"
          % (restored, touched, ", restored" if apply_changes else ""))


main()
