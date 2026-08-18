"""Find GN globals that are read but never assigned anywhere in the tree.

Deleting a `.gni` takes its `declare_args()` names with it, and `gn gen` reports
exactly one undefined identifier per run. This approximates the whole set in one
pass: collect every name assigned at any point in any surviving GN file, then
report names that are only ever read.

The approximation is one-sided on purpose. It ignores scoping, so a name that is
only ever a local or a template parameter looks defined and is skipped -- this
under-reports rather than inventing work. Names it does report are read somewhere
and assigned nowhere, which is always a real break.

Usage:
  gn_undefined_vars.py [dir ...]     # default: the whole tree minus vendored repos
"""

import os
import re
import sys

ROOT = r"D:\Github\chromium"
SKIP_DIRS = {".git", "out"}

# Same rationale as gn_dangling_imports.py: these resolve `//` against their own
# root and are not loaded by Chromium's gn.
SKIP_TREES = tuple(os.path.join(ROOT, p) for p in (
    r"third_party\angle",
    r"third_party\skia",
    r"third_party\crashpad",
    r"third_party\mini_chromium",
    r"third_party\OpenCL-CTS",
    r"third_party\clspv",
    r"third_party\swiftshader",
    r"third_party\fuchsia-sdk",
    r"third_party\perfetto",
))

ASSIGN = re.compile(r"^[ \t]*([a-z_][a-z0-9_]*)[ \t]*(?:=|\+=|-=)", re.M)
EXPAND = re.compile(r"\$([a-z_][a-z0-9_]*)")
COND = re.compile(r"\b(?:if|assert)[ \t]*\([ \t]*!?([a-z_][a-z0-9_]*)[ \t]*[,)&|]")
# `sources += blink_core_sources_bindings` -- a bare identifier on the right of
# an assignment, which is how the per-directory source lists are pulled in.
RHS = re.compile(r"^[ \t]*[a-z_][a-z0-9_]*[ \t]*(?:\+=|=)[ \t]*"
                 r"([a-z_][a-z0-9_]*)[ \t]*$", re.M)
# The same read hiding in a list: `inputs = [ web_idl_database_filepath ]`, a
# bare identifier on its own line inside one, or `inputs = shared_list + [`.
LIST = re.compile(r"\[[ \t]*([a-z_][a-z0-9_]*)[ \t]*\]"
                  r"|^[ \t]+([a-z_][a-z0-9_]*),[ \t]*$"
                  r"|(?:\+=|=)[ \t]*([a-z_][a-z0-9_]*)[ \t]*\+", re.M)
# And a fourth: passed to a builtin, as core/BUILD.gn does for every one of its
# per-directory source lists -- `rebase_path(blink_core_sources_css, "", "css")`.
CALL = re.compile(r"\b(?:rebase_path|get_path_info|filter_include|filter_exclude"
                  r"|string_join|string_split|foreach)\([ \t]*\n?[ \t]*"
                  r"([a-z_][a-z0-9_]*)[ \t]*[,)]")
# GN builtins and target-scope variables that are never assigned at top level.
BUILTIN = set("""
true false current_cpu current_os current_toolchain default_toolchain
host_cpu host_os root_build_dir root_gen_dir root_out_dir target_cpu target_os
target_gen_dir target_out_dir python_path invoker target_name defined rebase_path
""".split())

roots = [os.path.join(ROOT, a) for a in sys.argv[1:]] or [ROOT]
assigned, read = set(), {}

for base in roots:
    for dirpath, dirnames, filenames in os.walk(base):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        if "depot_tools" in dirpath:
            continue
        # Vendored trees still *define* names that Chromium files read (ANGLE's
        # angle.gni is the common case), so they count on the assignment side.
        # Only their own reads are ignored.
        vendored = dirpath.startswith(SKIP_TREES)
        for fn in filenames:
            # BUILD.gn, .gni, and BUILDCONFIG.gn (which defines is_win and the
            # rest of the platform booleans). Every other bare `.gn` is an args
            # file, where an assignment overrides a declare_args default but
            # does not put a new name in scope for anyone.
            if fn not in ("BUILD.gn", "BUILDCONFIG.gn") \
                    and not fn.endswith(".gni"):
                continue
            fp = os.path.join(dirpath, fn)
            try:
                src = open(fp, encoding="utf-8").read()
            except (OSError, UnicodeDecodeError):
                continue
            assigned.update(ASSIGN.findall(src))
            if vendored:
                continue
            rel = os.path.relpath(fp, ROOT).replace("\\", "/")
            names = (set(EXPAND.findall(src)) | set(COND.findall(src))
                     | set(RHS.findall(src)) | set(CALL.findall(src)))
            for groups in LIST.findall(src):
                names.update(g for g in groups if g)
            for name in names:
                read.setdefault(name, set()).add(rel)

undefined = {n: v for n, v in read.items()
             if n not in assigned and n not in BUILTIN}

for name in sorted(undefined, key=lambda n: -len(undefined[n])):
    files = sorted(undefined[name])
    print("%3d  %-42s %s" % (len(files), name, files[0]))
    for f in files[1:6]:
        print("     %-42s %s" % ("", f))
    if len(files) > 6:
        print("     %-42s ... %d more" % ("", len(files) - 6))
print("---- %d name(s) read but never assigned" % len(undefined))
