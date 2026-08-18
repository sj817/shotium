"""Find file entries in GN lists whose files no longer exist.

`gn gen` does not check that the paths in `sources`, `inputs`, `public` or
`data` exist -- only ninja does, at build time, one missing file per run. After
a large deletion that is dozens of rounds, each one a full `gn gen` plus a
ninja start-up.

This resolves every literal path in those lists and reports the ones that are
gone. Entries that are obviously not source paths are skipped:

  * anything containing `$` -- generated files under $target_gen_dir and the
    like, which are produced by an action rather than checked in;
  * anything with a `:` -- those are labels, not files;
  * directory entries in `data`, which are legitimately allowed to be paths
    that only exist in some configurations;
  * anything inside a `#` comment -- third_party/ced/BUILD.gn documents the
    command that generated its source list, quotes and all.

This does not evaluate conditionals, so an entry inside `if (is_linux)` is
reported even though ninja never reaches it. Deleting those is harmless for a
Windows-only build -- the file is genuinely absent either way -- but it does
mean the output is a candidate list rather than a defect list.

It also only sees string literals, so an indirection is invisible to it:

    crate_root = "audio/rustfft_ffi.rs"
    sources = [ crate_root ]

is a missing source this will never report. Those surface as a ninja "missing
and no known rule to make it" and have to be fixed by hand.

Only the GN files that `gn gen` actually loaded are examined. That list is in
<out>/build.ninja.d, and using it is what keeps the result honest: a full-tree
scan reports ten thousand "missing" entries, almost all of them in BUILD.gn
files for dependencies that are simply not checked out on this platform
(third_party/mutter, glib, junit, weston), which GN never reads and ninja never
cares about.

Usage:
  gn_missing_sources.py <out-dir> [--delete]
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
SKIP_DIRS = {".git", "out"}
# traits_sources/traits_headers belong to mojom()'s cpp_typemaps, which is how
# a deleted component's mojo traits stay referenced from a surviving BUILD.gn.
LISTS = ("sources", "inputs", "public", "data", "filelist",
         # mojom()'s cpp_typemaps, which is how a deleted component's mojo
         # traits stay referenced from a surviving BUILD.gn.
         "traits_sources", "traits_headers",
         # Blink's code generators take their json5 inputs as in_files or
         # json_inputs depending on the template, so a deleted core
         # subdirectory leaves one of these behind rather than a `sources`
         # entry. Each new list name costs a full build round to discover;
         # when adding one, check that its strings really are paths.
         "in_files", "json_inputs", "templates")

# `sources = [` ... `]`, including the single-line `sources += [ "a.cc" ]` form.
# The body must exclude brackets: requiring the closing `]` to be alone on its
# own line means a single-line list never terminates the match, and the scan
# runs on through labels and defines, reporting them all as missing files.
BLOCK = re.compile(
    r"^[ \t]*(?:%s)[ \t]*\+?=[ \t]*\[([^\[\]]*)\]" % "|".join(LISTS),
    re.M | re.S)
ENTRY = re.compile(r'"([^"]+)"')
COMMENT = re.compile(r"#[^\n]*")


def resolve(path, build_dir):
    if path.startswith("//"):
        return os.path.join(ROOT, path[2:].replace("/", os.sep))
    return os.path.join(build_dir, path.replace("/", os.sep))


_basenames_cache = {}


def basenames_under(build_dir):
    """Every file name below a BUILD.gn's directory, three levels deep.

    Not every template resolves relative paths against the directory of the
    BUILD.gn that invokes it. aggregate_vector_icons resolves against its own
    `icon_directory`, so ui/views/BUILD.gn lists "check.icon" for a file that
    lives in ui/views/vector_icons/. Treating those as missing and deleting
    them empties the list the template turns into a response file, and GN then
    fails with "Missing response_file_contents definition" -- which is how this
    check was found.
    """
    if build_dir in _basenames_cache:
        return _basenames_cache[build_dir]
    names = set()
    base_depth = build_dir.rstrip(os.sep).count(os.sep)
    for dirpath, dirnames, filenames in os.walk(build_dir):
        if dirpath.count(os.sep) - base_depth >= 3:
            dirnames[:] = []
        names.update(filenames)
    _basenames_cache[build_dir] = names
    return names


def loaded_gn_files(out_dir):
    """The .gn/.gni files gn gen read, from <out>/build.ninja.d."""
    dep_file = os.path.join(ROOT, out_dir, "build.ninja.d")
    with open(dep_file, encoding="utf-8") as f:
        text = f.read()
    _, _, deps = text.partition(":")
    paths = []
    for token in deps.replace("\\\n", " ").split():
        token = token.replace("\\ ", " ")
        if token.endswith((".gn", ".gni")):
            paths.append(os.path.normpath(os.path.join(ROOT, out_dir, token)))
    return paths


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        sys.exit(__doc__)
    out_dir = args[0]
    delete = "--delete" in sys.argv

    missing = {}
    for fp in loaded_gn_files(out_dir):
        # Only BUILD.gn. A relative path inside a .gni resolves against the
        # directory of whichever BUILD.gn imported it, which this script has no
        # way to know, so every such entry would be reported wrongly.
        if os.path.basename(fp) != "BUILD.gn":
            continue
        try:
            src = open(fp, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        dirpath = os.path.dirname(fp)
        gone = []
        for block in BLOCK.findall(COMMENT.sub("", src)):
            for entry in ENTRY.findall(block):
                if "$" in entry or ":" in entry or entry.endswith("/"):
                    continue
                # Must look like a path. A sources list can contain strings
                # that are function arguments rather than files --
                # `get_label_info(invoker.target, "dir")` -- and "dir" resolves
                # to nothing, gets called missing, and deleting it leaves a
                # trailing comma inside the call.
                if "/" not in entry and "." not in entry:
                    continue
                if os.path.exists(resolve(entry, dirpath)):
                    continue
                if os.path.basename(entry) in basenames_under(dirpath):
                    continue  # A template resolved it against another base.
                gone.append(entry)
        if not gone:
            continue
        rel = os.path.relpath(fp, ROOT).replace("\\", "/")
        missing[rel] = gone
        if delete:
            out = src
            for entry in set(gone):
                esc = re.escape(entry)
                # Entry alone on its line.
                out = re.sub(r'^[ \t]*"%s",?[ \t]*\n' % esc, "", out, flags=re.M)
                # Entry inside a single-line list: `traits_headers = [ "a.h" ]`.
                # Matching only the own-line form is why a --delete pass could
                # report 68 removed and still leave 61 behind.
                out = re.sub(r'"%s"[ \t]*,?[ \t]*' % esc, "", out)
            # Tidy what removal can leave: `[  ]` and a dangling comma.
            out = re.sub(r"\[[ \t]*\]", "[]", out)
            out = re.sub(r",[ \t]*\]", " ]", out)
            # An emptied list is often an invalid target rather than a smaller
            # one: an action with no inputs has no outputs ("Action has no
            # outputs"), and aggregate_vector_icons turns its list into a
            # response file ("Missing response_file_contents definition").
            # Both showed up this way, so say which targets to look at.
            emptied = len(re.findall(r"\[[ \t]*\]", out)) - \
                len(re.findall(r"\[[ \t]*\]", src))
            if emptied > 0:
                print("     ^ emptied %d list(s) here -- check those targets"
                      % emptied)
            open(fp, "w", encoding="utf-8", newline="").write(out)

    verbose = "--verbose" in sys.argv
    for rel in sorted(missing, key=lambda r: -len(missing[r])):
        print("%4d  %s" % (len(missing[rel]), rel))
        if verbose:
            for entry in missing[rel]:
                print("        %s" % entry)
    print("---- %d missing entr(ies) in %d file(s)%s"
          % (sum(len(v) for v in missing.values()), len(missing),
             ", deleted" if delete else ""))


main()
