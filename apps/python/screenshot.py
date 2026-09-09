"""Use the prebuilt C ABI directly; no Shotium Python package is needed."""
import argparse
import ctypes as C
import json
from pathlib import Path
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library_dir", type=Path)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    directory = args.library_dir.resolve(strict=True)
    name = {"win32": "shotium.dll", "darwin": "libshotium.dylib"}.get(sys.platform, "libshotium.so")
    lib = C.CDLL(str(directory / name))  # cdecl; keep loaded until process exit
    ptr = C.c_void_p
    out = C.POINTER(ptr)
    signatures = {
        "shot_abi_version": ([], C.c_int32),
        "shot_engine_create": ([C.c_char_p, out, out], C.c_int32),
        "shot_engine_capture": ([ptr, C.c_char_p, out, out, out], C.c_int32),
        "shot_engine_destroy": ([ptr], None),
        "shot_buffer_data": ([ptr], ptr),
        "shot_buffer_size": ([ptr], C.c_size_t),
        "shot_buffer_free": ([ptr], None),
    }
    for name, (arguments, result) in signatures.items():
        function = getattr(lib, name)
        function.argtypes, function.restype = arguments, result
    version = lib.shot_abi_version()
    if version != 3:
        raise RuntimeError(f"C ABI mismatch: expected 3, got {version}")

    def take(buffer):
        if not buffer.value:
            return b""
        try:
            size = lib.shot_buffer_size(buffer)
            return C.string_at(lib.shot_buffer_data(buffer), size) if size else b""
        finally:
            lib.shot_buffer_free(buffer)
            buffer.value = None

    engine, error = ptr(), ptr()
    status = lib.shot_engine_create(json.dumps({"resourceDir": str(directory)}).encode(), C.byref(engine), C.byref(error))
    message = take(error).decode("utf-8", errors="replace").rstrip("\0")
    if status:
        raise RuntimeError(f"create failed ({status}): {message}")
    try:
        image, stats, error = ptr(), ptr(), ptr()
        request = {"file": str(args.input.resolve()), "allowFileAccess": True,
                   "width": 800, "height": 600, "type": "png"}
        status = lib.shot_engine_capture(engine, json.dumps(request).encode(), C.byref(image), C.byref(stats), C.byref(error))
        # Copy bytes before freeing their native owners, including failure stats.
        try:
            data = take(image)
            statistics = take(stats).decode("utf-8", errors="replace").rstrip("\0")
            message = take(error).decode("utf-8", errors="replace").rstrip("\0")
        finally:
            for buffer in (image, stats, error):
                if buffer.value:
                    lib.shot_buffer_free(buffer)
        if statistics:
            print(statistics)
        if status:
            raise RuntimeError(f"capture failed ({status}): {message}")
        args.output.write_bytes(data)
        print(f"Wrote {args.output} ({len(data)} bytes)")
    finally:
        lib.shot_engine_destroy(engine)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(error, file=sys.stderr)
        sys.exit(1)
