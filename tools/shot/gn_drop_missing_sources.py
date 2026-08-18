"""Remove BUILD.gn entries that name a source file which no longer exists.

ninja reports exactly one of these per run:

    ninja: error: '../../third_party/blink/common/manifest/manifest_mojom_traits.cc',
    needed by 'obj/.../manifest_mojom_traits.obj', missing and no known rule to make it

and then stops -- before compiling anything. So each dangling entry costs a full
gn+ninja round trip to discover. After a cut wave that deletes hundreds of
files, that is the single most expensive way to spend an afternoon.

This finds them all in one pass: every string literal in a .gn/.gni file that
looks like a source path, resolved the way GN resolves it (`//x` from the source
root, anything else relative to the file's own directory), reported if it is not
on disk.

Skipped, because absence does not imply dangling:
  * anything containing `$` -- $target_gen_dir, $root_out_dir, expansions;
  * paths under a gen/ directory -- produced by an action at build time;
  * entries that are not obviously source files (no known extension), since
    those are as likely to be labels, directories or data as files;
  * .gni files, with one exception. A path in a .gni normally resolves against
    whichever BUILD.gn imports it, not against the .gni's own directory, so
    checking it reports build/scripts/scripts.gni's `css/css_properties.json5`
    as missing when it is fine one directory over. blink's per-directory
    `build.gni` is the exception: core/BUILD.gn rebases those lists back onto
    the .gni's own directory, so they are swept;
  * anything inside an `outputs = [` / `args = [` list, however many lines it
    spans. These name what a target *produces*, so of course they are absent --
    and stripping them is worse than useless: it left
    inspector_protocol_generate() with an empty outputs list and gn died with
    "Array subscript out of range" pointing at a .gni, three files away from
    the edit.

Only pure list entries in a BUILD.gn are ever touched -- a path embedded in a
larger expression is reported and left alone.

Usage:
  gn_drop_missing_sources.py <path> [...] [-n]

  <path>  directory to walk, or a single .gn/.gni file
  -n      dry run
"""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXTS = (".cc", ".c", ".h", ".hpp", ".cpp", ".mm", ".m", ".S", ".asm",
        ".idl", ".mojom", ".pdl", ".json5", ".proto", ".rs", ".py",
        ".typemap", ".gni", ".java", ".ts")
LITERAL = re.compile(r'"([^"$\s]+)"')
# Lists whose entries are what the target produces or passes on, not files it
# reads. Their contents are absent from the source tree by definition.
PRODUCED = {"outputs", "args", "defines", "cflags", "ldflags", "public_configs",
            "configs", "deps", "public_deps", "data_deps"}


def resolve(gn_path, value):
    if value.startswith("//"):
        return os.path.join(ROOT, value[2:].replace("/", os.sep))
    return os.path.join(os.path.dirname(gn_path), value.replace("/", os.sep))


def gn_files(paths):
    for p in paths:
        full = p if os.path.isabs(p) else os.path.join(ROOT, p)
        if os.path.isfile(full):
            yield full
            continue
        for dirpath, dirs, files in os.walk(full):
            dirs[:] = [d for d in dirs if d not in (".git", "out")]
            for name in files:
                if name == "BUILD.gn" or name == "build.gni":
                    # build.gni is blink's per-directory source list. Its paths
                    # are relative to its own directory, because core/BUILD.gn
                    # rebases them back with rebase_path(list, "", "<dir>").
                    # No other .gni is swept; see the module docstring.
                    yield os.path.join(dirpath, name)


def main(argv):
    dry = "-n" in argv
    paths = [a for a in argv if a != "-n"]
    if not paths:
        sys.exit(__doc__)

    total = 0
    for gn_path in gn_files(paths):
        src = io.open(gn_path, encoding="utf-8", errors="replace",
                      newline="").read()
        nl = "\r\n" if "\r\n" in src else "\n"
        lines = src.split(nl)
        drop = []
        # Name of the list currently open, so a multi-line `outputs = [` body
        # can be skipped entry by entry.
        open_list = None
        for i, line in enumerate(lines):
            stripped = line.strip()
            if stripped.startswith("#"):
                continue
            m = re.match(r"(\w+)\s*\+?=\s*\[\s*$", stripped)
            if m:
                open_list = m.group(1)
            elif stripped in ("]", "],"):
                open_list = None
            if open_list in PRODUCED:
                continue
            for value in LITERAL.findall(line):
                if not value.endswith(EXTS):
                    continue
                if "/gen/" in value or value.startswith(("gen/", "grit/")):
                    continue  # grit/x.h is written into gen by a grit action.
                if ".mojom" in value and value.endswith(".h"):
                    continue  # x.mojom.h / -shared.h / -forward.h come from mojom.
                if os.path.exists(resolve(gn_path, value)):
                    continue
                # Only a pure list entry is ever removed. An assignment names an
                # output or a generated header and is expected to be absent; a
                # path inside a larger expression needs a human.
                if re.fullmatch(r'\s*"[^"]+",?\s*', line):
                    drop.append((i, value))
                elif "=" not in line:
                    print("  MANUAL %s:%d  %s"
                          % (os.path.relpath(gn_path, ROOT), i + 1, stripped))
                break
        if not drop:
            continue
        rel = os.path.relpath(gn_path, ROOT)
        for _i, value in drop:
            print("  drop   %s  ->  %s" % (rel, value))
        for i, _v in reversed(drop):
            del lines[i]
        if not dry:
            io.open(gn_path, "w", encoding="utf-8",
                    newline="").write(nl.join(lines))
        total += len(drop)

    print("%d dangling source entries%s" % (total, " (dry run)" if dry else ""))


if __name__ == "__main__":
    main(sys.argv[1:])
