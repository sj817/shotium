#!/usr/bin/env python3
"""Build, train and rebuild Shot with LLVM profile-guided optimization.

One output directory is reused for both phases so a Chromium checkout does not
need room for two multi-gigabyte object trees. The final args.gn records the
profile path and phase, making the produced binary reproducible.
"""

import argparse
import glob
import os
import pathlib
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib


ROOT = pathlib.Path(__file__).resolve().parents[2]


def host_tool(*parts):
    suffix = ".exe" if sys.platform == "win32" else ""
    return ROOT.joinpath(*parts).with_suffix(suffix)


def gn_binary():
    platform_dir = {
        "win32": "win",
        "darwin": "mac",
    }.get(sys.platform, "linux64")
    return host_tool("buildtools", platform_dir, "gn")


def ninja_binary():
    return host_tool("third_party", "ninja", "ninja")


def profdata_binary():
    bundled = host_tool(
        "third_party", "llvm-build", "Release+Asserts", "bin",
        "llvm-profdata")
    if bundled.is_file():
        return bundled, False
    on_path = shutil.which("llvm-profdata")
    if on_path:
        return pathlib.Path(on_path), True
    if sys.platform == "win32":
        installed = pathlib.Path(
            os.environ.get("ProgramFiles", r"C:\Program Files"),
            "LLVM", "bin", "llvm-profdata.exe")
        if installed.is_file():
            return installed, True
    raise FileNotFoundError(
        "llvm-profdata is neither in Chromium's toolchain nor on PATH")


def read_uleb(blob, position):
    value = 0
    shift = 0
    while True:
        byte = blob[position]
        position += 1
        value |= (byte & 0x7f) << shift
        if byte < 0x80:
            return value, position
        shift += 7


def write_uleb(value):
    result = bytearray()
    while True:
        byte = value & 0x7f
        value >>= 7
        result.append(byte | (0x80 if value else 0))
        if not value:
            return bytes(result)


def uncompress_profile_names(source):
    """Make a raw profile readable by llvm-profdata built without zlib.

    Chromium normally downloads a matching coverage-tools package. A trimmed
    checkout may omit it while the compiler runtime still writes compressed
    __llvm_prf_names segments. The raw-profile header and segment framing are
    public LLVM formats; only those name payloads are changed here.
    """
    data = bytearray(source.read_bytes())
    header = list(struct.unpack_from("<16Q", data))
    raw_magic_64 = 0xff6c70726f667281
    if header[0] != raw_magic_64:
        raise RuntimeError(
            f"{source} is not a little-endian 64-bit LLVM raw profile")
    header_size = 16 * 8
    data_record_size = 64
    names_offset = (header_size + header[2] +
                    header[3] * data_record_size + header[4] +
                    header[5] * 8 + header[6] + header[7] + header[8])
    old_size = header[9]
    old_end = (names_offset + old_size + 7) & ~7
    if old_end > len(data):
        raise RuntimeError(f"{source} has an invalid names section")
    blob = data[names_offset:names_offset + old_size]
    position = 0
    replacement = bytearray()
    while position < len(blob):
        uncompressed, position = read_uleb(blob, position)
        compressed, position = read_uleb(blob, position)
        payload_size = compressed or uncompressed
        payload = blob[position:position + payload_size]
        position += payload_size
        raw = zlib.decompress(payload) if compressed else payload
        if len(raw) != uncompressed:
            raise RuntimeError(f"{source} has a corrupt compressed name block")
        replacement.extend(write_uleb(uncompressed))
        replacement.extend(write_uleb(0))
        replacement.extend(raw)

    header[9] = len(replacement)
    struct.pack_into("<16Q", data, 0, *header)
    padding = b"\0" * ((-len(replacement)) & 7)
    output = source.with_name(source.stem + "-plain.profraw")
    output.write_bytes(
        data[:names_offset] + replacement + padding + data[old_end:])
    return output


def run(command, **kwargs):
    print("+", " ".join(str(part) for part in command), flush=True)
    subprocess.run([str(part) for part in command], check=True, cwd=ROOT,
                   **kwargs)


def ensure_icu():
    source = ROOT / "third_party" / "icu" / "cast" / "icudtl.dat"
    target = ROOT / "third_party" / "icu" / "shot" / "icudtl.dat"
    target.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=target.parent, delete=False) as temp:
        temporary = pathlib.Path(temp.name)
    try:
        run([sys.executable, "tools/shot/icu_repack.py", source, temporary,
             "--preset", "shot"])
        if not target.exists() or target.read_bytes() != temporary.read_bytes():
            os.replace(temporary, target)
    finally:
        if temporary.exists():
            temporary.unlink()


def gn_source_path(path):
    relative = path.resolve().relative_to(ROOT)
    return "//" + relative.as_posix()


def write_args(out_dir, phase, profile, extra_args):
    args_file = {
        "win32": "shot.gn",
        "darwin": "shot-mac.gn",
    }.get(sys.platform, "shot-linux.gn")
    lines = [
        f'import("//build/args/{args_file}")',
        "",
        "# PGO is trained on Shot itself, not on Chromium's Chrome profile.",
        f"chrome_pgo_phase = {phase}",
        "optimize_for_size = false",
    ]
    if phase == 2:
        lines.append(f'pgo_data_path = "{gn_source_path(profile)}"')
    lines.extend(extra_args)
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "args.gn").write_text("\n".join(lines) + "\n", encoding="utf-8")


def gn_gen(out_dir):
    command = [gn_binary(), "gen", out_dir]
    attempts = 8 if sys.platform == "win32" else 1
    for attempt in range(1, attempts + 1):
        result = subprocess.run(
            [str(part) for part in command], cwd=ROOT, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        print(result.stdout, end="")
        if result.returncode == 0:
            return
        if "PermissionError" not in result.stdout or attempt == attempts:
            raise subprocess.CalledProcessError(result.returncode, command)


def build(out_dir, jobs):
    run([ninja_binary(), "-C", out_dir, "shot", "shot_c", "-j", str(jobs),
         "-k", "0"])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default="out/ShotPgo")
    parser.add_argument("--jobs", type=int, default=12)
    parser.add_argument("--repeats", type=int, default=4)
    parser.add_argument(
        "--optimize-only", action="store_true",
        help="reuse an existing merged profile and build phase 2")
    parser.add_argument(
        "--gn-arg", action="append", default=[],
        help='additional args.gn line, for example target_cpu="arm64"')
    args = parser.parse_args()

    out_dir = (ROOT / args.out).resolve()
    out_dir.relative_to(ROOT / "out")
    raw_dir = out_dir / "pgo-raw"
    profile_dir = ROOT / ".shot-pgo"
    profile_dir.mkdir(exist_ok=True)
    profile = profile_dir / f"{out_dir.name}.profdata"

    ensure_icu()
    if not args.optimize_only:
        write_args(out_dir, 1, profile, args.gn_arg)
        gn_gen(out_dir)
        build(out_dir, args.jobs)

        if raw_dir.exists():
            shutil.rmtree(raw_dir)
        raw_dir.mkdir()
        if profile.exists():
            profile.unlink()
        environment = os.environ.copy()
        environment["LLVM_PROFILE_FILE"] = str(
            raw_dir / "shotium-%2m.profraw")
        run([sys.executable, "tools/shot/pgo_train.py", out_dir, "--repeats",
             str(args.repeats)], env=environment)

        raw_profiles = sorted(glob.glob(str(raw_dir / "*.profraw")))
        if not raw_profiles:
            raise RuntimeError(f"training produced no profiles in {raw_dir}")
        profdata, needs_plain_names = profdata_binary()
        merge_inputs = [pathlib.Path(path) for path in raw_profiles]
        if needs_plain_names:
            print("Chromium llvm-profdata is absent; expanding compressed raw "
                  "profile names for the system tool", flush=True)
            merge_inputs = [uncompress_profile_names(path)
                            for path in merge_inputs]
        run([profdata, "merge", "-o", profile, *merge_inputs])
        shutil.rmtree(raw_dir)
    elif not profile.is_file():
        raise FileNotFoundError(f"no merged profile at {profile}")

    write_args(out_dir, 2, profile, args.gn_arg)
    gn_gen(out_dir)
    build(out_dir, args.jobs)
    print(f"PGO build ready in {out_dir}")
    print(f"profile: {profile} ({profile.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
