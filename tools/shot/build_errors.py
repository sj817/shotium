"""Summarise a ninja log into error classes rather than error lines.

A `ninja -k 0` run over this tree produces a log where each FAILED entry is
followed by the full clang-cl command line -- roughly 6 KB of flags per failure.
Reading the raw log costs more than the information in it. What actually drives
the next edit is: which diagnostic, how many times, and in which files.

Groups by the normalised diagnostic text (identifiers, types and numbers inside
quotes are kept, since 'use of undeclared identifier X' is a different problem
for each X), and prints the file list for each.

Usage:
  build_errors.py <log> [--limit N] [--files]
"""

import re
import sys
from collections import defaultdict

# clang-cl: `../../path/file.cc(122,10): error: text`
ERROR = re.compile(r"^(\S+?)\((\d+),\d+\): (?:error|fatal error): (.*)$")
# link errors: `lld-link: error: text`
LINK = re.compile(r"^(?:lld-link|ninja): (?:error|fatal error): (.*)$")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    limit = 40
    if "--limit" in sys.argv:
        limit = int(sys.argv[sys.argv.index("--limit") + 1])
    show_files = "--files" in sys.argv

    classes = defaultdict(list)
    failed = 0
    with open(sys.argv[1], encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("FAILED:"):
                failed += 1
                continue
            m = ERROR.match(line)
            if m:
                classes[m.group(3)].append("%s:%s" % (m.group(1), m.group(2)))
                continue
            m = LINK.match(line)
            if m:
                classes[m.group(1)].append("<link>")

    print("%d FAILED edge(s), %d distinct diagnostic(s)"
          % (failed, len(classes)))
    for text in sorted(classes, key=lambda t: -len(classes[t]))[:limit]:
        sites = classes[text]
        print("%4d  %s" % (len(sites), text))
        if show_files:
            for s in sorted(set(sites))[:8]:
                print("        %s" % s)
            if len(set(sites)) > 8:
                print("        ... %d more" % (len(set(sites)) - 8))


main()
