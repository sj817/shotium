# shotium Python demo

English · [简体中文](./README.zh.md)

[![Python](https://img.shields.io/badge/Python-3.9+-3776AB?logo=python&logoColor=white)](https://python.org/) [![ctypes](https://img.shields.io/badge/FFI-ctypes%20(zero%20deps)-blue.svg)](https://docs.python.org/3/library/ctypes.html) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

Demonstrates rendering HTML via the shotium C ABI directly from Python using the standard library `ctypes` module; this project is a standalone runnable demo, not a published package

## Table of Contents

[Requirements](#requirements) · [Get the files](#get-the-files) · [Run](#run) · [How it works](#how-it-works) · [Python notes](#python-notes) · [See also](#see-also) · [License](#license)

## Requirements

- Python 3.9 or newer (standard library only; no third-party dependencies required)
- 7-Zip (`7z`) to extract release packages
- Prebuilt engine binary matching target Python interpreter architecture

## Get the files

This example is packaged as [`shotium-example-python.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-python.7z). Extract it alongside the matching native C ABI package `shotium-c-abi-<platform>.7z`:

```bash
# Linux / macOS (example: linux-amd64, requires 7z or 7zz)
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-python.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-python.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-python/native
cd shotium-example-python
```

```powershell
# Windows PowerShell (example: windows-amd64)
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-python.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-python.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-python\native
Set-Location shotium-example-python
```

> Integrity check: Download [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) and run `sha256sum --check --ignore-missing SHA256SUMS`; native libraries reside in `native/shotium-c-abi-<platform>/`, containing the shared library, headers, and `.pak` resource files


## Run

```bash
python3 screenshot.py native/shotium-c-abi-linux-amd64 card.html card.png
```

On Windows, the interpreter executable is typically `python`; positional arguments are `<library-dir> <input.html> <output.png>`, where `<library-dir>` is also passed to the engine as `resourceDir`; the script renders `card.html` at 720×380, prints capture statistics JSON to stdout, and writes `card.png` (pixel-identical to output from the `shotium` CLI)

Providing a nonexistent input file demonstrates the error handling flow: exit code is 1, stderr outputs `capture failed (2): ...`, statistics are still reported, and no output image is generated

When testing against local engine build artifacts, specify `../../out/Shot` as the library directory

## How it works

`screenshot.py` demonstrates the complete invocation lifecycle:

```python
lib = ctypes.CDLL(str(directory / "libshotium.so"))   # stays loaded for the process lifetime
ptr, out = ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)
lib.shot_abi_version.restype = ctypes.c_int32
lib.shot_engine_create.argtypes = [ctypes.c_char_p, out, out]
lib.shot_engine_capture.argtypes = [ptr, ctypes.c_char_p, out, out, out]
lib.shot_buffer_size.restype = ctypes.c_size_t
lib.shot_buffer_data.restype = ptr
# ... shot_engine_destroy, shot_buffer_free

if lib.shot_abi_version() != 3:
    raise RuntimeError("C ABI mismatch")

engine = ptr()
error = ptr()
if lib.shot_engine_create(json.dumps({"resourceDir": str(directory)}).encode(), byref(engine), byref(error)):
    raise RuntimeError(take(error))          # take() copies the bytes, then frees the buffer

try:
    image = ptr()
    stats = ptr()
    error = ptr()
    request = {
        "file": str(input.resolve()),
        "allowFileAccess": True,
        "width": 720,
        "height": 380,
    }
    status = lib.shot_engine_capture(engine, json.dumps(request).encode(), byref(image), byref(stats), byref(error))
    data, statistics, message = take(image), take(stats), take(error)
    print(statistics)                        # present on failure too
    if status:
        raise RuntimeError(f"capture failed ({status}): {message}")
    output.write_bytes(data)
finally:
    lib.shot_engine_destroy(engine)
```

## Python notes

- Explicitly declare `argtypes` and `restype` for every C function before invoking it; without explicit signatures, `ctypes` treats pointer types as 32-bit integers on 64-bit Windows (`shot_buffer_size` must return `ctypes.c_size_t`)
- The `take()` helper uses `ctypes.string_at` with `shot_buffer_size` to copy native buffer memory into a Python `bytes` object before calling `shot_buffer_free()`; native memory is never transferred to Python's memory allocator
- Engine destruction is enclosed in a `finally` block to ensure engine background threads are joined and cleaned up even if exceptions are raised
- Input paths should be resolved to absolute paths before serialization into JSON, preventing ambiguity with the engine's internal working directory

## See also

Complete ownership contracts, request options, performance statistics, tiling, and cache maintenance APIs are detailed in the [C ABI Guide](../c-abi/README.md); the C header `shot_api.h` serves as the authoritative interface specification

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)
