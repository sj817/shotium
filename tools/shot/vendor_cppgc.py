"""Vendor cppgc out of V8 into //third_party/cppgc.

Blink's Oilpan *is* cppgc: `blink::GarbageCollected<T>` is a direct alias for
`cppgc::GarbageCollected<T>`, with no abstraction layer in between, and every
DOM, CSS and layout object derives from it. So "delete V8" has to be split in
two: the JavaScript engine goes, and the garbage collector -- which happens to
live inside the V8 repository at src/heap/cppgc-internal -- stays.

cppgc is built to support exactly this. V8's own build has a `cppgc_is_standalone`
mode, and cppgc's external dependency surface is just two headers (v8config.h and
v8-platform.h, the latter pulling v8-source-location.h) plus abseil, which
Chromium already has. Nothing here drags in the parser, the interpreter,
TurboFan, the snapshot or the bindings.

The source list is computed from the files on disk rather than transcribed from
V8's BUILD.gn, because that file expresses the platform split through ~350 lines
of conditionals. Files for other platforms are excluded by name; if that filter
is wrong the compiler says so immediately, which is a better check than a
hand-copied list nobody re-reads.

Usage:
  vendor_cppgc.py <v8-checkout> [--revision <sha>]
"""

import os
import shutil
import sys

DEST = r"D:\Github\chromium\third_party\cppgc"
VENDOR = os.path.join(DEST, "v8")

# Directories taken wholesale, relative to the V8 checkout root.
TREES = [
    "include/cppgc",
    "include/libplatform",
    "src/base",
    "src/heap/base",
    "src/heap/cppgc-internal",
    "src/libplatform",
    # src/base/hashing.h includes it by path; it lives in V8's repo, not DEPS.
    "third_party/rapidhash-v8",
]
FILES = [
    "include/v8config.h",
    "include/v8-platform.h",
    "include/v8-source-location.h",
    "src/tracing/trace-event-no-perfetto.h",
    "LICENSE",
]

# Other platforms' implementations. V8 selects these with current_cpu/is_win
# conditionals; here they are simply not vendored.
OTHER_PLATFORM = (
    "platform-aix", "platform-cygwin", "platform-darwin", "platform-freebsd",
    "platform-fuchsia", "platform-linux", "platform-openbsd",
    "platform-posix", "platform-posix-time", "platform-qnx",
    "platform-solaris", "platform-starboard", "platform-zos",
    "stack_trace_android", "stack_trace_fuchsia", "stack_trace_posix",
    "stack_trace_zos",
)
# Only the x64 register-push assembly is reachable on this host.
ASM_KEEP = "src/heap/base/asm/x64/push_registers_masm.asm"

# Other architectures' CPU feature detection.
OTHER_CPU = ("cpu-arm", "cpu-loong64", "cpu-mips64", "cpu-ppc", "cpu-riscv",
             "cpu-s390")

# Built by V8 only when cppgc_enable_caged_heap is on; caged-heap.h has an
# #error for the case we are in. Same for the 32-bit-UBSan-only translation
# unit.
DISABLED_FEATURE = ("caged-heap", "caged-heap-local-data", "ubsan",
                    # V8 builds these only under
                    # V8_ENABLE_SYSTEM_INSTRUMENTATION; recorder-win.cc wants
                    # ETW and recorder-mac.cc wants os_log.
                    "recorder-win", "recorder-mac")

SKIP_NAMES = {"DEPS", "OWNERS", "DIR_METADATA", "README.md", "BUILD.gn",
              "PRESUBMIT.py"}


def wanted(rel):
    """rel is a forward-slash path relative to the V8 root."""
    name = rel.rsplit("/", 1)[-1]
    stem = name.rsplit(".", 1)[0]
    if name in SKIP_NAMES:
        return False
    if "/asm/" in rel:
        return rel == ASM_KEEP
    if stem in OTHER_PLATFORM or stem in OTHER_CPU:
        return False
    # Headers stay -- they are included unconditionally; only the translation
    # units are dropped.
    if stem in DISABLED_FEATURE and name.endswith(".cc"):
        return False
    if stem.endswith(("-unittest", "_unittest", "-test", "_test")):
        return False
    return name.endswith((".h", ".cc", ".asm", ".inc")) or name == "LICENSE"


def copy_all(src_root):
    copied = []
    for tree in TREES:
        src = os.path.join(src_root, tree.replace("/", os.sep))
        for dirpath, dirnames, filenames in os.walk(src):
            for fn in sorted(filenames):
                full = os.path.join(dirpath, fn)
                rel = os.path.relpath(full, src_root).replace("\\", "/")
                if not wanted(rel):
                    continue
                dst = os.path.join(VENDOR, rel.replace("/", os.sep))
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.copy2(full, dst)
                copied.append(rel)
    for rel in FILES:
        full = os.path.join(src_root, rel.replace("/", os.sep))
        if not os.path.exists(full):
            print("MISSING: %s" % rel)
            continue
        dst = os.path.join(VENDOR, rel.replace("/", os.sep))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy2(full, dst)
        copied.append(rel)
    return copied


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    src_root = sys.argv[1]
    revision = "unknown"
    if "--revision" in sys.argv:
        revision = sys.argv[sys.argv.index("--revision") + 1]

    if os.path.isdir(VENDOR):
        shutil.rmtree(VENDOR)
    copied = copy_all(src_root)

    sources = sorted(r for r in copied
                     if r.endswith((".cc", ".asm"))
                     and (r.startswith("src/")))
    headers = [r for r in copied if r.endswith((".h", ".inc"))]

    # One list per V8 target. They have to stay separate: src/base/logging.cc
    # and src/heap/cppgc-internal/logging.cc would otherwise produce the same
    # object file name, which GN rejects.
    groups = [
        ("v8_libplatform_sources", "src/libplatform/"),
        ("cppgc_base_sources", "src/heap/cppgc-internal/"),
        ("v8_heap_base_sources", "src/heap/base/"),
        ("v8_libbase_sources", "src/base/"),
    ]
    with open(os.path.join(DEST, "sources.gni"), "w", newline="") as f:
        f.write("# Generated by //tools/shot/vendor_cppgc.py from V8 %s.\n"
                "# Do not edit; re-run the script instead.\n"
                "#\n"
                "# One list per upstream target. Merging them would collide:\n"
                "# src/base/logging.cc and src/heap/cppgc-internal/logging.cc\n"
                "# map to the same object file name.\n"
                % revision[:12])
        claimed = set()
        for var, prefix in groups:
            f.write("\n%s = [\n" % var)
            for rel in sources:
                if rel.startswith(prefix) and rel not in claimed:
                    claimed.add(rel)
                    f.write('  "v8/%s",\n' % rel)
            f.write("]\n")
        leftover = [r for r in sources if r not in claimed]
        if leftover:
            print("UNCLAIMED: %s" % ", ".join(leftover))

    print("copied %d file(s): %d source(s), %d header(s)"
          % (len(copied), len(sources), len(headers)))
    by_tree = {}
    for rel in copied:
        key = "/".join(rel.split("/")[:3]) if rel.startswith("src/heap") \
            else "/".join(rel.split("/")[:2])
        by_tree[key] = by_tree.get(key, 0) + 1
    for key in sorted(by_tree):
        print("  %-32s %4d" % (key, by_tree[key]))


main()
