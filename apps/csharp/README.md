# shotium C# demo

English · [简体中文](./README.zh.md)

[![.NET](https://img.shields.io/badge/.NET-8.0+-512BD4?logo=dotnet&logoColor=white)](https://dotnet.microsoft.com/) [![P/Invoke](https://img.shields.io/badge/FFI-P%2FInvoke%20(zero%20deps)-blue.svg)](https://learn.microsoft.com/dotnet/standard/native-interop/pinvoke) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

Demonstrates rendering HTML via the shotium C ABI directly from .NET using P/Invoke with no third-party NuGet dependencies; this project is a standalone runnable demo, not a published package

## Table of Contents

[Requirements](#requirements) · [Get the files](#get-the-files) · [Run](#run) · [How it works](#how-it-works) · [.NET notes](#net-notes) · [See also](#see-also) · [License](#license)

## Requirements

- .NET SDK 8.0 or newer (targeting `net8.0` with no external package references)
- 7-Zip (`7z`) to extract release packages
- Prebuilt engine binary matching target .NET runtime architecture (reported by `dotnet --info` as `RID`)

## Get the files

This example is packaged as [`shotium-example-csharp.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-csharp.7z). Extract it alongside the matching native C ABI package `shotium-c-abi-<platform>.7z`:

```bash
# Linux / macOS (example: linux-amd64, requires 7z or 7zz)
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-csharp.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-csharp.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-csharp/native
cd shotium-example-csharp
```

```powershell
# Windows PowerShell (example: windows-amd64)
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-csharp.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-csharp.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-csharp\native
Set-Location shotium-example-csharp
```

> Integrity check: Download [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) and run `sha256sum --check --ignore-missing SHA256SUMS`; native libraries reside in `native/shotium-c-abi-<platform>/`, containing the shared library, headers, and `.pak` resource files

## Run

```bash
dotnet run -c Release -- native/shotium-c-abi-linux-amd64 card.html card.png
```

Positional arguments are `<library-dir> <input.html> <output.png>`, where `<library-dir>` is also passed to the engine as `resourceDir`; the program renders `card.html` at 720×380, prints capture statistics JSON to stdout, and writes `card.png` (pixel-identical to output from the `shotium` CLI)

Providing a nonexistent input file demonstrates the error handling flow: exit code is 1, stderr outputs `capture failed (2): ...`, statistics are still reported, and no output image is generated

When testing against local engine build artifacts, specify `../../out/Shot` as the library directory

## How it works

`Program.cs` demonstrates the complete invocation lifecycle; P/Invoke declarations target `shotium`, resolved dynamically to the library path via `SetDllImportResolver`:

```csharp
var library = NativeLibrary.Load(Path.Combine(directory, "libshotium.so")); // kept loaded for process lifetime
NativeLibrary.SetDllImportResolver(typeof(Shotium).Assembly, (name, _, _) => name == "shotium" ? library : IntPtr.Zero);

if (Shotium.shot_abi_version() != 3)
{
    throw new InvalidOperationException("C ABI mismatch");
}

var status = Shotium.shot_engine_create(JsonSerializer.Serialize(new { resourceDir = directory }), out var engine, out var error);
var message = Shotium.TakeText(error); // copies data, then calls shot_buffer_free
if (status != 0)
{
    throw new InvalidOperationException($"create failed ({status}): {message}");
}

try
{
    var request = JsonSerializer.Serialize(new
    {
        file = Path.GetFullPath(args[1]),
        allowFileAccess = true,
        width = 720,
        height = 380,
        type = "png"
    });
    status = Shotium.shot_engine_capture(engine, request, out var image, out var stats, out error);
    try
    {
        if (stats != IntPtr.Zero)
        {
            Console.WriteLine(Shotium.Text(stats)); // present on failure too
        }
        if (status != 0)
        {
            throw new InvalidOperationException($"capture failed ({status}): {Shotium.Text(error)}");
        }
        File.WriteAllBytes(args[2], Shotium.Bytes(image));
    }
    finally
    {
        Shotium.shot_buffer_free(image);
        Shotium.shot_buffer_free(stats);
        Shotium.shot_buffer_free(error);
    }
}
finally
{
    Shotium.shot_engine_destroy(engine);
}

static class Shotium
{
    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int shot_engine_capture(
        IntPtr engine,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string request,
        out IntPtr image,
        out IntPtr stats,
        out IntPtr error);

    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint shot_buffer_size(IntPtr buffer);
    // ... shot_abi_version, shot_engine_create, shot_engine_destroy, shot_buffer_data, shot_buffer_free
}
```

## .NET notes

- String arguments require explicit `UnmanagedType.LPUTF8Str` marshalling; the engine expects UTF-8 encoding, whereas default .NET string marshalling on Windows uses ANSI
- `shot_buffer_size` returns `size_t`, represented in C# as `nuint`; native handles and pointers are typed as `IntPtr`; the calling convention is `CallingConvention.Cdecl` across all platforms
- The `Bytes()` helper uses `Marshal.Copy` to copy native memory into a managed byte array before releasing the buffer; native memory must only be freed via `shot_buffer_free()` (calling `Marshal.FreeHGlobal` corrupts the native heap)
- The library handle returned by `NativeLibrary.Load` is kept loaded for the process lifetime and never passed to `NativeLibrary.Free`, ensuring engine worker threads and thread-local storage remain intact
- Enclosing operations in `finally` blocks guarantees all native buffers and engine instances are freed even if an exception occurs during capture

## See also

Complete ownership contracts, request options, performance statistics, tiling, and cache maintenance APIs are detailed in the [C ABI Guide](../c-abi/README.md); the C header `shot_api.h` serves as the authoritative interface specification

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)
