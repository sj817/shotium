# shotium Rust demo

English · [简体中文](./README.zh.md)

[![Rust](https://img.shields.io/badge/Rust-1.85+-orange?logo=rust&logoColor=white)](https://rust-lang.org/) [![libloading](https://img.shields.io/badge/FFI-libloading-blue.svg)](https://crates.io/crates/libloading) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

Demonstrates rendering HTML via the shotium C ABI directly from Rust using `libloading` with RAII encapsulation for native handles; this project is a standalone runnable example crate (`publish = false`), not a published crate on crates.io

## Table of Contents

[Requirements](#requirements) · [Get the files](#get-the-files) · [Run](#run) · [How it works](#how-it-works) · [Rust notes](#rust-notes) · [See also](#see-also) · [License](#license)

## Requirements

- Rust 1.85 or newer and Cargo (`libloading` and `serde_json` are pinned in `Cargo.lock`)
- 7-Zip (`7z`) to extract release packages
- Prebuilt engine binary matching target host architecture (as reported by `rustc -vV`)

## Get the files

This demo is distributed as `shotium-rust-example-v<version>.zip` on the [Releases page](https://github.com/sj817/shotium/releases), identical to `apps/rust` in the repository; download the matching platform archive from the same release and extract it into `native/`:

```bash
# Linux / macOS, from this directory
version=v0.7.0
platform=linux-amd64        # linux-arm64, macos-amd64, or macos-arm64
curl -fLO "https://github.com/sj817/shotium/releases/download/$version/shotium-$platform-$version.7z"
7z x "shotium-$platform-$version.7z" -onative
```

```powershell
# Windows PowerShell, from this directory
$version = 'v0.7.0'
$platform = 'windows-amd64'   # or windows-arm64
curl.exe -fLO "https://github.com/sj817/shotium/releases/download/$version/shotium-$platform-$version.7z"
7z x "shotium-$platform-$version.7z" -onative
```

Extraction yields `native/shotium-<platform>/` containing the shared library, `shot_api.h`, and the two `.pak` resource files; keep these files together from the matching release

## Run

```bash
cargo run --release --locked -- native/shotium-linux-amd64 card.html card.png
```

Positional arguments are `<library-dir> <input.html> <output.png>`, where `<library-dir>` is also passed to the engine as `resourceDir`; the program renders `card.html` at 720×380, prints capture statistics JSON to stdout, and writes `card.png` (pixel-identical to output from the `shotium` CLI)

Providing a nonexistent input file demonstrates the error handling flow: exit code is 1, stderr outputs `capture failed (2): ...`, statistics are still reported, and no output image is generated

When testing against local engine build artifacts, specify `../../out/Shot` as the library directory

## How it works

`src/main.rs` demonstrates the complete invocation lifecycle, encapsulating native resource handles via RAII to ensure deterministic cleanup on error or early return:

```rust
struct Buffer<'a>(&'a Api, Handle); // Drop → shot_buffer_free
struct Engine<'a>(&'a Api, Handle); // Drop → shot_engine_destroy

let library = ManuallyDrop::new(unsafe { Library::new(directory.join("libshotium.so"))? });
let abi = unsafe { library.get::<unsafe extern "C" fn() -> i32>(b"shot_abi_version\0")? };
if unsafe { abi() } != 3 {
    return Err("C ABI mismatch".into());
}

let api = Api {
    create: *library.get(b"shot_engine_create\0")?,
    /* capture, destroy, data, size, free... */
    _library: library,
};

let (mut handle, mut error) = (ptr::null_mut(), ptr::null_mut());
let status = unsafe { (api.create)(options.as_ptr(), &mut handle, &mut error) };
let error = Buffer(&api, error);
if status != 0 {
    return Err(error.text().into());
}
let engine = Engine(&api, handle);

let (mut image, mut stats, mut error) = (ptr::null_mut(), ptr::null_mut(), ptr::null_mut());
let status = unsafe { (api.capture)(engine.1, request.as_ptr(), &mut image, &mut stats, &mut error) };
let image = Buffer(&api, image);
let stats = Buffer(&api, stats);
let error = Buffer(&api, error);

if !stats.1.is_null() {
    println!("{}", stats.text()); // present on failure too
}
if status != 0 {
    return Err(format!("capture failed ({status}): {}", error.text()).into());
}

fs::write("card.png", image.bytes())?;
```

## Rust notes

- `Library` is wrapped in `ManuallyDrop` to prevent `dlclose`; the engine maintains background threads and thread-local state; unloading the library while these threads are active causes undefined behavior
- `Buffer::bytes()` constructs a byte slice bounded by the lifetime of `Buffer` using `shot_buffer_data` and `shot_buffer_size`; the `Drop` implementation calls `shot_buffer_free()`, never Rust's memory allocator
- Leveraging Rust's lexical scoping and variable declaration order, `Engine` is dropped only after all `Buffer` instances borrowing `Api` have been dropped
- The input path is converted to an absolute path without calling `canonicalize()`, allowing nonexistent file paths to reach the engine and verify C ABI error handling and diagnostic stats
- Dynamic library loading triggers library initialization routines; ensure only trusted, architecture-compatible binaries are loaded

## See also

Complete ownership contracts, request options, performance statistics, tiling, and cache maintenance APIs are detailed in the [C ABI Guide](../c-abi/README.md); the C header `shot_api.h` serves as the authoritative interface specification

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)

