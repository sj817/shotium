#!/usr/bin/env python3
"""Attribute shotium.exe's bytes to the code that produced them.

The question "what is in the binary" cannot be answered from the GN graph. A
target being in the dependency closure does not mean it contributed bytes:
/OPT:REF drops sections nothing references and /OPT:ICF folds identical ones,
so the only authority on what survived is the linker's own record.

This reads that record out of the PDB rather than re-linking with /MAP. The
PDB's section-contribution table is written *after* the layout is decided, so
every entry in it is a range of the final image, named by the object file it
came from. symbol_level = 0 is enough -- contributions are not debug info.

    python tools/shot/size_report.py out/Shot/shotium.exe

Reported per component, where a component is the source directory of the object
file (obj/<dir>/<target>/<file>.obj -> <dir>), rolled up to --depth for the
headline and printed in full with --detail.

The sum of contributions is smaller than the file on disk, and the difference is
reported rather than hidden: the import and relocation tables, resources,
section padding and the PE headers are made by the linker and belong to no
object file.
"""

import argparse
import collections
import os
import re
import struct
import subprocess
import sys

# `SC[.text]  | mod = 12, 0001:0000, size = 68, data crc = ..., reloc crc = ...`
SC_RE = re.compile(
    r"SC\[([^\]]+)\]\s*\|\s*mod = (\d+), ([0-9a-fA-F]+):([0-9a-fA-F]+), size = (\d+)")
# `Mod 0000 | `o:\fake\prefix\obj\shot\shot\main.obj`:`
MOD_RE = re.compile(r"^\s*Mod (\d+) \| `([^`]*)`")


def find_pdbutil():
    """Locate llvm-pdbutil in the tree's own toolchain."""
    exe = "llvm-pdbutil.exe" if os.name == "nt" else "llvm-pdbutil"
    path = os.path.join("third_party", "llvm-build", "Release+Asserts", "bin", exe)
    if os.path.exists(path):
        return path
    raise SystemExit("could not find %s -- is the clang toolchain unpacked?" % path)


def pe_sections(binary):
    """Section sizes straight from the PE header, as the denominator.

    The contribution table is a claim about the image; this is what the image
    actually is. Reported side by side so a parsing mistake in either shows up
    as a mismatch instead of a plausible-looking number.
    """
    with open(binary, "rb") as f:
        data = f.read()
    if data[:2] != b"MZ":
        raise SystemExit("%s is not a PE image" % binary)
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise SystemExit("%s has no PE signature" % binary)
    nsections, = struct.unpack_from("<H", data, pe + 6)
    opt_size, = struct.unpack_from("<H", data, pe + 20)
    table = pe + 24 + opt_size
    out = []
    for i in range(nsections):
        entry = table + i * 40
        name = data[entry:entry + 8].rstrip(b"\0").decode("latin-1")
        vsize, _vaddr, rawsize, _rawptr = struct.unpack_from("<IIII", data, entry + 8)
        out.append((name, vsize, rawsize))
    return out


def component_of(module):
    """obj/<dir>/<target>/<file>.obj -> <dir>; everything else kept verbatim.

    The target segment is dropped because it is a build-system name, not a
    place in the source tree: //third_party/blink/renderer/core:core and
    :prerequisites are the same code to anyone asking where the bytes went.
    """
    path = module.replace("\\", "/")
    low = path.lower()
    marker = "/obj/"
    idx = low.rfind(marker)
    if idx < 0:
        # Import libraries, the linker's own contributions, prebuilt .libs.
        base = os.path.basename(path)
        return "[%s]" % (base if base else path)
    parts = path[idx + len(marker):].split("/")
    if len(parts) <= 2:
        return parts[0] if parts else "[?]"
    return "/".join(parts[:-2])


def roll_up(component, depth):
    if component.startswith("["):
        return component
    parts = component.split("/")
    return "/".join(parts[:depth])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("binary", help="path to shotium.exe (its .pdb must sit beside it)")
    ap.add_argument("--depth", type=int, default=3,
                    help="directory depth for the headline rollup (default 3)")
    ap.add_argument("--top", type=int, default=40, help="rows to print (default 40)")
    ap.add_argument("--detail", action="store_true",
                    help="also print every component at full directory depth")
    ap.add_argument("--by-object", metavar="SUBSTRING",
                    help="instead of a rollup, list the individual object files "
                         "whose path contains SUBSTRING -- a directory total "
                         "cannot tell a generated table from a feature")
    ap.add_argument("--csv", help="write the full per-component table here")
    args = ap.parse_args()

    pdb = args.binary + ".pdb"
    if not os.path.exists(pdb):
        root, _ = os.path.splitext(args.binary)
        pdb = root + ".pdb"
    if not os.path.exists(pdb):
        raise SystemExit("no PDB beside %s" % args.binary)

    tool = find_pdbutil()

    sys.stderr.write("reading modules...\n")
    modules = {}
    proc = subprocess.Popen([tool, "dump", "--modules", pdb],
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                            universal_newlines=True, errors="replace")
    for line in proc.stdout:
        m = MOD_RE.match(line)
        if m:
            modules[int(m.group(1))] = m.group(2)
    proc.wait()

    sys.stderr.write("reading %d modules' section contributions...\n" % len(modules))
    # (component, section) -> bytes, and a per-component object-file count so a
    # big number can be read as "one huge file" or "ten thousand small ones".
    sizes = collections.defaultdict(int)
    per_section = collections.defaultdict(int)
    files = collections.defaultdict(set)
    # A contribution is a range of the image. Two entries at the same address
    # would be double counting, so addresses are only counted once.
    seen = set()
    dupes = 0
    proc = subprocess.Popen([tool, "dump", "--section-contribs", pdb],
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                            universal_newlines=True, errors="replace")
    for line in proc.stdout:
        m = SC_RE.search(line)
        if not m:
            continue
        section, mod, sect_idx, offset, size = m.groups()
        size = int(size)
        if size == 0:
            continue
        key = (int(sect_idx, 16), int(offset, 16))
        if key in seen:
            dupes += 1
            continue
        seen.add(key)
        raw = modules.get(int(mod), "[unknown module %s]" % mod)
        if args.by_object:
            path = raw.replace("\\", "/")
            if args.by_object not in path:
                continue
            idx = path.lower().rfind("/obj/")
            comp = path[idx + 5:] if idx >= 0 else path
        else:
            comp = component_of(raw)
        sizes[(comp, section)] += size
        per_section[section] += size
        files[comp].add(int(mod))
    proc.wait()

    totals = collections.defaultdict(int)
    for (comp, _section), size in sizes.items():
        totals[comp] += size
    attributed = sum(totals.values())
    on_disk = os.path.getsize(args.binary)

    print("")
    print("shotium.exe size composition")
    print("=" * 78)
    print("binary            %s" % args.binary)
    print("on disk           %14s bytes  %8.2f MB" % (f"{on_disk:,}", on_disk / 1048576))
    print("attributed        %14s bytes  %8.2f MB  (%.1f%% of the file)"
          % (f"{attributed:,}", attributed / 1048576, 100.0 * attributed / on_disk))
    print("unattributed      %14s bytes  %8.2f MB  (import/reloc tables,"
          % (f"{on_disk - attributed:,}", (on_disk - attributed) / 1048576))
    print("                  %14s         %8s   resources, padding, PE headers)" % ("", ""))
    if dupes:
        print("overlapping contributions skipped: %d" % dupes)

    print("")
    print("PE sections (the file's own account)")
    print("-" * 78)
    print("%-12s %16s %16s %16s" % ("section", "virtual", "raw", "attributed"))
    for name, vsize, rawsize in pe_sections(args.binary):
        print("%-12s %16s %16s %16s"
              % (name, f"{vsize:,}", f"{rawsize:,}", f"{per_section.get(name, 0):,}"))

    rolled = collections.defaultdict(int)
    rolled_files = collections.defaultdict(int)
    for comp, size in totals.items():
        rolled[roll_up(comp, args.depth)] += size
    for comp, mods in files.items():
        rolled_files[roll_up(comp, args.depth)] += len(mods)

    def table(title, items, counts):
        print("")
        print(title)
        print("-" * 78)
        print("%-64s %14s %7s %6s" % ("component", "bytes", "MB", "% bin"))
        shown = 0
        for comp, size in sorted(items, key=lambda kv: -kv[1])[:args.top]:
            print("%-64s %14s %7.2f %5.1f%%   %s objs"
                  % (comp[:64], f"{size:,}", size / 1048576,
                     100.0 * size / on_disk, counts.get(comp, 0)))
            shown += size
        rest = sum(v for _, v in items) - shown
        if rest > 0:
            print("%-64s %14s %7.2f %5.1f%%" % ("(everything else)", f"{rest:,}",
                                                rest / 1048576, 100.0 * rest / on_disk))

    table("by component, depth %d" % args.depth, list(rolled.items()), rolled_files)
    if args.detail:
        table("by source directory", list(totals.items()),
              {c: len(m) for c, m in files.items()})

    if args.csv:
        with open(args.csv, "w") as f:
            f.write("component,section,bytes\n")
            for (comp, section), size in sorted(sizes.items(), key=lambda kv: -kv[1]):
                f.write('"%s","%s",%d\n' % (comp, section, size))
        sys.stderr.write("wrote %s\n" % args.csv)


if __name__ == "__main__":
    main()
