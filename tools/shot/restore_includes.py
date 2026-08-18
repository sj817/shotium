"""Put back #include lines that strip_component.py removed from surviving files.

strip_component.py deletes every `#include "<component>/..."` for a component
that is being removed. When the component comes back -- as base/trace_event,
base/tracing and third_party/perfetto did, because base/check.h reaches them
through base/location.h -- those includes have to come back too, and only in
the files that still exist.

Missing them is not a link error, it is a compile error a long way from the
cause: dropping base/tracing/protos/chrome_track_event.pbzero.h from
base/task/sequence_manager/task_queue.h leaves

    using QueueName = ::perfetto::protos::pbzero::SequenceManagerTask::QueueName;

with nothing declaring SequenceManagerTask, and the diagnostic names perfetto
rather than the header that went missing.

Each restored line is put back after the line that preceded it in the original
file, which keeps the include block in its original order.

Usage:
  restore_includes.py <git-rev> <prefix> [<prefix> ...] [--apply]

  <git-rev>   revision holding the pre-deletion contents, e.g. abc1234^
  <prefix>    include path prefix to restore, e.g. base/tracing/
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
    pattern = re.compile(
        r'^#include "(?:%s)' % "|".join(re.escape(p) for p in prefixes))

    diff = git("diff", "--name-only", rev, "--", "*.cc", "*.h", "*.mm")
    touched = 0
    restored = 0
    for rel in diff.split("\n"):
        rel = rel.strip()
        if not rel:
            continue
        path = os.path.join(ROOT, rel.replace("/", os.sep))
        if not os.path.exists(path):
            continue  # File was deleted outright; nothing to restore into.
        original = git("show", "%s:%s" % (rev, rel))
        if not original:
            continue
        old_lines = original.split("\n")
        try:
            current = open(path, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        cur_lines = current.split("\n")
        cur_set = set(cur_lines)

        additions = []  # (predecessor line, line to restore)
        for i, line in enumerate(old_lines):
            if pattern.match(line) and line not in cur_set:
                predecessor = old_lines[i - 1] if i else None
                additions.append((predecessor, line))
        if not additions:
            continue
        touched += 1
        restored += len(additions)
        if not apply_changes:
            print("%3d  %s" % (len(additions), rel))
            continue

        for predecessor, line in additions:
            if predecessor is not None and predecessor in cur_lines:
                cur_lines.insert(cur_lines.index(predecessor) + 1, line)
            else:
                # No anchor: put it after the last include in the file.
                last = max((n for n, l in enumerate(cur_lines)
                            if l.startswith("#include ")), default=0)
                cur_lines.insert(last + 1, line)
        open(path, "w", encoding="utf-8", newline="").write(
            "\n".join(cur_lines))
        print("%3d  %s" % (len(additions), rel))

    print("---- %d include(s) in %d file(s)%s"
          % (restored, touched, ", restored" if apply_changes else ""))


main()
