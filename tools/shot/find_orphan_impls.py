"""Find .cc files whose own header no longer exists.

Chromium's style rule is that a .cc's first #include is its own header. That
makes "was this file's header cut out from under it?" a mechanical question
instead of a judgement call, and it catches the same damage in any directory --
cut_orphan_sources.py only knew about blink/common's split.

The failure this prevents is the expensive kind. ninja compiles the orphan,
clang reports

    fatal error: 'third_party/blink/public/platform/modules/mediastream/
    web_media_stream_audio_sink.h' file not found

and that is one file per round, because the rest of the target's failures are
hidden behind it.

Reports rather than deletes: a missing header sometimes means the header should
come back (it was collateral damage) and sometimes means the .cc should go (the
feature was cut). That is the restore-vs-cut decision in docs/cut-progress.md
section 8.8, and it is not mechanical.

Usage:
  find_orphan_impls.py <path> [...]
"""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
INCLUDE = re.compile(r'^\s*#include\s+"([^"]+)"', re.M)


def first_include(path):
    """The first quoted include, skipping the licence header and any <> ones."""
    try:
        src = io.open(path, encoding="utf-8", errors="replace").read(8192)
    except OSError:
        return None
    m = INCLUDE.search(src)
    return m.group(1) if m else None


def main(argv):
    if not argv:
        sys.exit(__doc__)
    found = 0
    for arg in argv:
        base = arg if os.path.isabs(arg) else os.path.join(ROOT, arg)
        for dirpath, dirs, files in os.walk(base):
            dirs[:] = [d for d in dirs if d not in (".git", "out")]
            for name in sorted(files):
                if not name.endswith(".cc"):
                    continue
                path = os.path.join(dirpath, name)
                header = first_include(path)
                if not header or not header.endswith(".h"):
                    continue
                # Only trust the convention when the include really looks like
                # this file's own header; otherwise a .cc that legitimately
                # includes something else first would be reported.
                if os.path.basename(header) != name[:-3] + ".h":
                    continue
                native = header.replace("/", os.sep)
                if os.path.exists(os.path.join(ROOT, native)):
                    continue
                # Some headers are generated: core/style/scroll_start_data.h
                # is written by blink's style generator and never existed in
                # the source tree, upstream included.
                if os.path.exists(os.path.join(ROOT, "out", "Shot", "gen",
                                               native)):
                    continue
                # A bare "foo.h" resolves against the .cc's own directory.
                if "/" not in header and os.path.exists(
                        os.path.join(dirpath, header)):
                    continue
                found += 1
                print("  orphan %-70s (no %s)"
                      % (os.path.relpath(path, ROOT).replace(os.sep, "/"),
                         header))
    print("%d orphan implementation file(s)" % found)


if __name__ == "__main__":
    main(sys.argv[1:])
