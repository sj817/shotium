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

Every platform this fork targets is vendored, not just the host. The first
version of this script kept only the files a Windows x64 build reaches, and
wrote them into sources.gni as a flat list. That is invisible until another
platform builds: an arm64 Windows run got as far as edge 6362 of 7548 before
clang-cl was handed src/heap/base/asm/x64/push_registers_masm.asm with
--target=aarch64-pc-windows and failed on the first `;;` of the copyright
header. `ninja -n` cannot catch that class of error either, because the file
exists -- it is simply the wrong one. So the platform split now lives in
sources.gni as GN conditionals, and PLATFORM_RULES below is transcribed from
V8's own BUILD.gn rather than inferred from filenames.

Usage:
  vendor_cppgc.py <v8-checkout> [--revision <sha>]
  vendor_cppgc.py --regenerate

--regenerate rewrites sources.gni from the files already under
third_party/cppgc/v8, with no V8 checkout involved. It is what to run after
adding or removing a vendored file by hand; the generated list then still
matches what is on disk, which is the only thing that makes the "do not edit"
header true.
"""

import os
import shutil
import sys

DEST = os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
    "third_party", "cppgc")
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

# The six configurations this fork builds. Anything only reachable outside this
# set is not vendored -- there is no aix, zos, fuchsia, android or ios here, and
# no 32-bit, mips, ppc, s390, loong64 or riscv.
#
# Adding a platform means adding its files here *and* to PLATFORM_RULES, and the
# two have to agree: a file vendored but unclassified would be compiled on every
# platform, which is the bug this replaced.
PLATFORM_KEEP = (
    "platform-posix", "platform-posix-time", "platform-linux",
    "platform-darwin", "platform-win32",
    "stack_trace_posix", "stack_trace_win",
)
CPU_KEEP = ("cpu-x86", "cpu-arm")
ASM_KEEP = (
    "src/heap/base/asm/x64/push_registers_masm.asm",
    "src/heap/base/asm/x64/push_registers_asm.cc",
    "src/heap/base/asm/arm64/push_registers_asm.cc",
)

# Every platform- and cpu-specific stem V8 has, so that one it grows later is
# not silently swept into the unconditional list by a filter that only knows
# what to exclude. A stem here that is not in PLATFORM_KEEP/CPU_KEEP is dropped;
# a stem in neither list is a source this script has no opinion about, and it
# says so rather than guessing.
PLATFORM_ALL = PLATFORM_KEEP + (
    "platform-aix", "platform-cygwin", "platform-freebsd", "platform-fuchsia",
    "platform-openbsd", "platform-qnx", "platform-solaris",
    "platform-starboard", "platform-zos",
    "stack_trace_android", "stack_trace_fuchsia", "stack_trace_zos",
)
CPU_ALL = CPU_KEEP + ("cpu-loong64", "cpu-mips64", "cpu-ppc", "cpu-riscv",
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

# Which GN condition guards each platform-specific source, transcribed from
# V8's BUILD.gn at the vendored revision -- v8_heap_base for the assembly,
# v8_libbase for the rest. Sources not named here are unconditional.
#
# Two of V8's conditions are narrowed rather than copied, because the platforms
# they distinguish do not exist in this tree:
#   is_posix || is_fuchsia          -> is_posix
#   is_linux || is_chromeos         -> is_linux
# and platform-posix-time.cc, which upstream excludes on aix and zos, is simply
# posix here. Narrowing an unreachable branch away is safe; widening one would
# not be, so nothing below is broader than upstream.
PLATFORM_RULES = [
    # v8_heap_base. Note the x64 split: Windows prefers the masm version
    # because it carries unwind directives, and every other target compiles the
    # inline-asm .cc. arm64 has no masm variant at all -- Windows arm64 builds
    # push_registers_asm.cc too, which is why it guards on _WIN64 internally.
    ("src/heap/base/asm/x64/push_registers_masm.asm",
     'current_cpu == "x64" && is_win'),
    ("src/heap/base/asm/x64/push_registers_asm.cc",
     'current_cpu == "x64" && !is_win'),
    ("src/heap/base/asm/arm64/push_registers_asm.cc",
     'current_cpu == "arm64"'),

    # v8_libbase: CPU feature detection. Upstream keys these on target_cpu, not
    # current_cpu, and the difference is deliberate -- keep it.
    ("src/base/cpu/cpu-x86.cc", 'target_cpu == "x86" || target_cpu == "x64"'),
    ("src/base/cpu/cpu-arm.cc", 'target_cpu == "arm" || target_cpu == "arm64"'),

    # v8_libbase: the platform layer.
    ("src/base/platform/platform-posix.cc", "is_posix"),
    ("src/base/platform/platform-posix-time.cc", "is_posix"),
    ("src/base/platform/platform-linux.cc", "is_linux"),
    ("src/base/platform/platform-darwin.cc", "is_mac"),
    ("src/base/platform/platform-win32.cc", "is_win"),
    ("src/base/debug/stack_trace_posix.cc", "is_posix"),
    ("src/base/debug/stack_trace_win.cc", "is_win"),
]
RULE_FOR = dict(PLATFORM_RULES)


def wanted(rel):
    """rel is a forward-slash path relative to the V8 root."""
    name = rel.rsplit("/", 1)[-1]
    stem = name.rsplit(".", 1)[0]
    if name in SKIP_NAMES:
        return False
    if "/asm/" in rel:
        return rel in ASM_KEEP
    if stem in PLATFORM_ALL:
        # Headers stay whichever platform they belong to: platform-posix.h is
        # included by platform-linux.cc and platform-darwin.cc alike, and a
        # header that is never compiled costs nothing.
        return stem in PLATFORM_KEEP or name.endswith(".h")
    if stem in CPU_ALL:
        return stem in CPU_KEEP or name.endswith(".h")
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


def scan_vendored():
    """The same list copy_all returns, read back off disk instead."""
    found = []
    for dirpath, dirnames, filenames in os.walk(VENDOR):
        for fn in sorted(filenames):
            rel = os.path.relpath(os.path.join(dirpath, fn),
                                  VENDOR).replace("\\", "/")
            found.append(rel)
    return sorted(found)


# One list per V8 target. They have to stay separate: src/base/logging.cc and
# src/heap/cppgc-internal/logging.cc would otherwise produce the same object
# file name, which GN rejects.
GROUPS = [
    ("v8_libplatform_sources", "src/libplatform/"),
    ("cppgc_base_sources", "src/heap/cppgc-internal/"),
    ("v8_heap_base_sources", "src/heap/base/"),
    ("v8_libbase_sources", "src/base/"),
]


def write_sources_gni(copied, revision):
    sources = sorted(r for r in copied
                     if r.endswith((".cc", ".asm")) and r.startswith("src/"))

    lines = [
        "# Generated by //tools/shot/vendor_cppgc.py from V8 %s." % revision[:12],
        "# Do not edit; re-run the script instead.",
        "#",
        "# One list per upstream target. Merging them would collide:",
        "# src/base/logging.cc and src/heap/cppgc-internal/logging.cc",
        "# map to the same object file name.",
        "#",
        "# The conditionals are transcribed from V8's own BUILD.gn -- see",
        "# PLATFORM_RULES in the generator. A source under a condition is",
        "# reachable on some platform this fork builds and not on others; one",
        "# outside every condition compiles everywhere.",
    ]

    claimed = set()
    unclassified = []
    for var, prefix in GROUPS:
        mine = [r for r in sources
                if r.startswith(prefix) and r not in claimed]
        claimed.update(mine)

        plain = [r for r in mine if r not in RULE_FOR]
        lines.append("")
        lines.append("%s = [" % var)
        for rel in plain:
            lines.append('  "v8/%s",' % rel)
        lines.append("]")

        # Group by condition, in the order PLATFORM_RULES lists them, so a
        # regeneration produces the same bytes as long as the rules and the
        # tree have not changed.
        conditional = [r for r in mine if r in RULE_FOR]
        seen_conditions = []
        for rel, cond in PLATFORM_RULES:
            if rel in conditional and cond not in seen_conditions:
                seen_conditions.append(cond)
        for cond in seen_conditions:
            group = [r for r in conditional if RULE_FOR[r] == cond]
            lines.append("if (%s) {" % cond)
            lines.append("  %s += [" % var)
            for rel in group:
                lines.append('    "v8/%s",' % rel)
            lines.append("  ]")
            lines.append("}")

    unclassified = [r for r in sources if r not in claimed]

    with open(os.path.join(DEST, "sources.gni"), "w", newline="") as f:
        f.write("\n".join(lines) + "\n")

    return sources, unclassified


def main():
    argv = sys.argv[1:]
    revision = "unknown"
    if "--revision" in argv:
        i = argv.index("--revision")
        revision = argv[i + 1]
        del argv[i:i + 2]

    if "--regenerate" in argv:
        copied = scan_vendored()
        # The revision is a property of what was vendored, not of this run, so
        # a regeneration must not overwrite it with "unknown".
        if revision == "unknown":
            revision = read_recorded_revision()
        stray = [r for r in copied if not wanted(r)]
        if stray:
            print("VENDORED BUT NOT WANTED (the filter and the tree disagree):")
            for rel in stray:
                print("  %s" % rel)
    else:
        if not argv:
            sys.exit(__doc__)
        src_root = argv[0]
        if os.path.isdir(VENDOR):
            shutil.rmtree(VENDOR)
        copied = copy_all(src_root)

    sources, unclassified = write_sources_gni(copied, revision)

    print("%d file(s), %d source(s)" % (len(copied), len(sources)))
    if unclassified:
        print("UNCLAIMED (in no group -- they will not be built):")
        for rel in unclassified:
            print("  %s" % rel)

    # Which sources ended up behind which condition, so the split is readable
    # without opening the generated file.
    by_cond = {}
    for rel in sources:
        by_cond.setdefault(RULE_FOR.get(rel, "(always)"), []).append(rel)
    for cond in sorted(by_cond, key=lambda c: (c != "(always)", c)):
        print("  %-42s %d" % (cond, len(by_cond[cond])))
    return 0


def read_recorded_revision():
    """The revision README.chromium records for the vendored tree."""
    path = os.path.join(DEST, "README.chromium")
    try:
        with open(path, encoding="utf-8") as f:
            for line in f:
                if line.startswith("Revision:"):
                    return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return "unknown"


if __name__ == "__main__":
    sys.exit(main())
