# shotium Go demo

English · [简体中文](./README.zh.md)

[![Go](https://img.shields.io/badge/Go-1.23+-00ADD8?logo=go&logoColor=white)](https://go.dev/) [![purego](https://img.shields.io/badge/FFI-purego%20(no%20cgo)-blue.svg)](https://github.com/ebitengine/purego) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

Demonstrates rendering HTML via the shotium C ABI directly from Go using purego without cgo; this project is a standalone runnable demo, not a published Go package

## Table of Contents

[Requirements](#requirements) · [Get the files](#get-the-files) · [Run](#run) · [How it works](#how-it-works) · [Go notes](#go-notes) · [See also](#see-also) · [License](#license)

## Requirements

- Go 1.23 or newer (no C compiler required; the shared library is loaded dynamically at runtime via [purego](https://github.com/ebitengine/purego))
- 7-Zip (`7z`) to extract release packages
- Prebuilt engine binary matching target runtime architecture (`go env GOOS GOARCH`)

## Get the files

This example is packaged as [`shotium-example-go.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-go.7z). Extract it alongside the matching native C ABI package `shotium-c-abi-<platform>.7z`:

```bash
# Linux / macOS (example: linux-amd64, requires 7z or 7zz)
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-go.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-go.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-go/native
cd shotium-example-go
```

```powershell
# Windows PowerShell (example: windows-amd64)
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-go.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-go.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-go\native
Set-Location shotium-example-go
```

> Integrity check: Download [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) and run `sha256sum --check --ignore-missing SHA256SUMS`; native libraries reside in `native/shotium-c-abi-<platform>/`, containing the shared library, headers, and `.pak` resource files


## Run

```bash
go run -mod=readonly . native/shotium-c-abi-linux-amd64 card.html card.png
```

The positional arguments are `<library-dir> <input.html> <output.png>`, where `<library-dir>` is also passed to the engine as `resourceDir`; the program renders `card.html` at 720×380, prints capture statistics JSON to stdout, and writes `card.png` (pixel-identical to output from the `shotium` CLI)

Providing a nonexistent input file demonstrates the error handling flow: exit code is 1, stderr outputs `capture failed (2): ...`, statistics are still reported, and no output image is generated

When testing against local engine build artifacts, specify `../../out/Shot` as the library directory

## How it works

`main.go` demonstrates the complete invocation lifecycle:

```go
handle, _ := loadLibrary(filepath.Join(dir, "libshotium.so")) // kept mapped for the process lifetime
purego.RegisterLibFunc(&abi, handle, "shot_abi_version")
purego.RegisterLibFunc(&create, handle, "shot_engine_create")
purego.RegisterLibFunc(&capture, handle, "shot_engine_capture")
purego.RegisterLibFunc(&free, handle, "shot_buffer_free")
// ... shot_engine_destroy, shot_buffer_data, shot_buffer_size

if abi() != 3 {
	return fmt.Errorf("C ABI mismatch")
}

var engine, failure uintptr
status := create(`{"resourceDir":"…"}`, &engine, &failure)
defer free(failure)
if status != 0 {
	return errors.New(read(failure))
}
defer destroy(engine)

var image, stats, err uintptr
status = capture(engine, `{"file":"…","width":720,"height":380}`, &image, &stats, &err)
defer free(image)
defer free(stats)
defer free(err)

if stats != 0 {
	fmt.Println(read(stats)) // present on failure too
}
if status != 0 {
	return errors.New(read(err))
}

os.WriteFile("card.png", read(image), 0644)
```

The `read()` helper copies native buffer memory into a Go-managed byte slice before the deferred `shot_buffer_free()` executes

## Go notes

- `load_windows.go` and `load_unix.go` select platform loading mechanisms via build tags: `syscall.LoadLibrary` on Windows and `purego.Dlopen` on Unix/macOS; handles remain loaded for the entire process lifetime
- Every `shot_buffer*` returned by the C ABI must be released with `shot_buffer_free()`, never by the Go garbage collector or standard libc `free`; deferred free calls run after data copying is complete
- `go.sum` pins dependency versions to guarantee repeatable builds across environments
- The example is organized as a `main` package; production integrations can factor out `RegisterLibFunc` definitions and buffer helpers into a reusable package

## See also

Complete ownership contracts, request options, performance statistics, tiling, and cache maintenance APIs are detailed in the [C ABI Guide](../c-abi/README.md); the C header `shot_api.h` serves as the authoritative interface specification

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)
