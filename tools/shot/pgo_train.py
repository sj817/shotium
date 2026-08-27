#!/usr/bin/env python3
"""Exercise Shot's real entry points for an instrumented PGO build.

The caller sets LLVM_PROFILE_FILE. This script trains cold CLI startup, the
resident framed server, and the C ABI without keeping rendered images.
"""

import argparse
import ctypes
import json
import os
import struct
import subprocess
import sys
import tempfile


ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TESTDATA = os.path.join(ROOT, "shot", "testdata")


def read_exact(stream, size):
    chunks = bytearray()
    while len(chunks) < size:
        chunk = stream.read(size - len(chunks))
        if not chunk:
            raise RuntimeError("shotium closed its response stream")
        chunks.extend(chunk)
    return bytes(chunks)


def send_request(proc, request):
    payload = json.dumps(request, separators=(",", ":")).encode()
    proc.stdin.write(struct.pack("<I", len(payload)))
    proc.stdin.write(payload)
    proc.stdin.flush()
    header_size = struct.unpack("<I", read_exact(proc.stdout, 4))[0]
    header = json.loads(read_exact(proc.stdout, header_size))
    image_size = struct.unpack("<I", read_exact(proc.stdout, 4))[0]
    read_exact(proc.stdout, image_size)
    if not header.get("ok"):
        raise RuntimeError(header.get("error", "capture failed"))


def representative_requests(repeats):
    demos = os.path.join(TESTDATA, "demos")
    requests = [
        {
            "file": os.path.join(demos, name),
            "width": 400,
            "height": 200,
            "allowFileAccess": True,
        }
        for name in sorted(os.listdir(demos))
        if name.endswith(".html") and not name.endswith("-ref.html")
    ]

    corpus = os.path.join(TESTDATA, "render_corpus.html")
    features = os.path.join(TESTDATA, "features.html")
    png = {
        "file": corpus,
        "width": 1248,
        "height": 1320,
        "allowFileAccess": True,
    }
    for _ in range(repeats):
        requests.extend([
            png,
            {**png, "width": 1920, "height": 1080, "scale": 2},
            {**png, "type": "jpeg", "quality": 90},
            {**png, "type": "webp", "quality": 90},
            {
                "file": features,
                "width": 400,
                "height": 300,
                "fullPage": True,
                "allowFileAccess": True,
            },
            {
                "file": features,
                "width": 400,
                "height": 300,
                "selector": "#box",
                "allowFileAccess": True,
            },
            {
                "file": features,
                "width": 400,
                "height": 300,
                "omitBackground": True,
                "allowFileAccess": True,
            },
        ])
    return requests


def train_server(exe, requests):
    proc = subprocess.Popen(
        [exe, "--serve", "--allow-file-access"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    try:
        for request in requests:
            send_request(proc, request)
    finally:
        proc.stdin.close()
        code = proc.wait(timeout=120)
        if code:
            raise RuntimeError(f"shotium --serve exited with {code}")


def train_cli(exe):
    corpus = os.path.join(TESTDATA, "render_corpus.html")
    cases = [
        [],
        ["--type", "jpeg", "--quality", "90"],
        ["--type", "webp", "--quality", "90"],
        ["--scale", "2"],
    ]
    with tempfile.TemporaryDirectory(prefix="shot-pgo-cli-") as temp:
        for index, extra in enumerate(cases):
            output = os.path.join(temp, f"{index}.png")
            subprocess.run(
                [exe, "--file", corpus, "--width", "1248", "--height",
                 "1320", "--output", output, *extra],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )


def shared_library(out_dir):
    if sys.platform == "win32":
        return os.path.join(out_dir, "shotium.dll")
    if sys.platform == "darwin":
        return os.path.join(out_dir, "libshotium.dylib")
    return os.path.join(out_dir, "libshotium.so")


def train_c_abi(out_dir, requests):
    library_path = shared_library(out_dir)
    if sys.platform == "win32":
        os.add_dll_directory(out_dir)
    library = ctypes.CDLL(library_path)
    pointer = ctypes.c_void_p
    library.shot_engine_create.argtypes = [
        ctypes.c_char_p, ctypes.POINTER(pointer), ctypes.POINTER(pointer)]
    library.shot_engine_create.restype = ctypes.c_int32
    library.shot_engine_capture.argtypes = [
        pointer, ctypes.c_char_p, ctypes.POINTER(pointer),
        ctypes.POINTER(pointer), ctypes.POINTER(pointer)]
    library.shot_engine_capture.restype = ctypes.c_int32
    library.shot_engine_destroy.argtypes = [pointer]
    library.shot_buffer_data.argtypes = [pointer]
    library.shot_buffer_data.restype = ctypes.POINTER(ctypes.c_uint8)
    library.shot_buffer_size.argtypes = [pointer]
    library.shot_buffer_size.restype = ctypes.c_size_t
    library.shot_buffer_free.argtypes = [pointer]

    def buffer_text(value):
        if not value.value:
            return ""
        size = library.shot_buffer_size(value)
        return ctypes.string_at(library.shot_buffer_data(value), size).decode(
            "utf-8", "replace").rstrip("\0")

    engine = pointer()
    error = pointer()
    options = json.dumps({
        "resourceDir": out_dir,
        "allowFileAccess": True,
    }).encode()
    status = library.shot_engine_create(options, ctypes.byref(engine),
                                        ctypes.byref(error))
    if status:
        message = buffer_text(error)
        if error.value:
            library.shot_buffer_free(error)
        raise RuntimeError(f"shot_engine_create failed: {message}")

    try:
        for request in requests:
            image = pointer()
            stats = pointer()
            error = pointer()
            payload = json.dumps(request, separators=(",", ":")).encode()
            status = library.shot_engine_capture(
                engine, payload, ctypes.byref(image), ctypes.byref(stats),
                ctypes.byref(error))
            if status:
                raise RuntimeError(
                    f"shot_engine_capture failed: {buffer_text(error)}")
            for value in (image, stats, error):
                if value.value:
                    library.shot_buffer_free(value)
    finally:
        library.shot_engine_destroy(engine)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out_dir")
    parser.add_argument("--repeats", type=int, default=4)
    args = parser.parse_args()

    out_dir = os.path.abspath(args.out_dir)
    exe = os.path.join(out_dir, "shotium.exe" if sys.platform == "win32"
                       else "shotium")
    if not os.path.isfile(exe):
        parser.error(f"no instrumented executable at {exe}")
    if not os.path.isfile(shared_library(out_dir)):
        parser.error(f"no instrumented shared library in {out_dir}")

    requests = representative_requests(args.repeats)
    train_cli(exe)
    train_server(exe, requests)
    # C ABI uses a bounded representative subset; the shared core has already
    # seen the full corpus through the executable.
    train_c_abi(out_dir, requests[-min(24, len(requests)):])
    print(f"trained CLI, serve and C ABI with {len(requests)} serve requests")


if __name__ == "__main__":
    main()
