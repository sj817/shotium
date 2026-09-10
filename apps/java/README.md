# shotium Java demo

English · [简体中文](./README.zh.md)

[![Java](https://img.shields.io/badge/Java-17+-ED8B00?logo=openjdk&logoColor=white)](https://openjdk.org/) [![JNA](https://img.shields.io/badge/FFI-JNA%205.17-blue.svg)](https://github.com/java-native-access/jna) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

Demonstrates rendering HTML via the shotium C ABI directly from Java using JNA; this project is a standalone runnable demo, not a published Maven artifact

## Table of Contents

[Requirements](#requirements) · [Get the files](#get-the-files) · [Run](#run) · [Unicode on Windows](#unicode-paths-on-windows) · [How it works](#how-it-works) · [Java notes](#java-notes) · [See also](#see-also) · [License](#license)

## Requirements

- JDK 17 or newer and Maven 3.9 or newer (JNA 5.17.0 and Gson 2.13.1 are pinned in `pom.xml`)
- 7-Zip (`7z`) to extract release packages
- Prebuilt engine binary matching target JVM architecture (reported by `java -XshowSettings:properties -version` as `os.arch`)

## Get the files

This example is packaged as [`shotium-example-java.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-java.7z). Extract it alongside the matching native C ABI package `shotium-c-abi-<platform>.7z`:

```bash
# Linux / macOS (example: linux-amd64, requires 7z or 7zz)
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-java.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-java.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-java/native
cd shotium-example-java
```

```powershell
# Windows PowerShell (example: windows-amd64)
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-java.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-java.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-java\native
Set-Location shotium-example-java
```

> Integrity check: Download [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) and run `sha256sum --check --ignore-missing SHA256SUMS`; native libraries reside in `native/shotium-c-abi-<platform>/`, containing the shared library, headers, and `.pak` resource files

## Run

```bash
mvn package dependency:copy-dependencies
java -cp "target/classes:target/dependency/*" ShotiumDemo native/shotium-c-abi-linux-amd64 card.html card.png
```

On Windows, use a semicolon `;` as the classpath separator:

```powershell
mvn package dependency:copy-dependencies
java -cp "target/classes;target/dependency/*" ShotiumDemo native/shotium-c-abi-windows-amd64 card.html card.png
```

Positional arguments are `<library-dir> <input.html> <output.png>`, where `<library-dir>` is also passed to the engine as `resourceDir`; the program renders `card.html` at 720×380, prints capture statistics JSON to stdout, and writes `card.png` (pixel-identical to output from the `shotium` CLI)

Providing a nonexistent input file demonstrates the error handling flow: exit code is 1, stderr outputs `capture failed (2): ...`, statistics are still reported, and no output image is generated

When testing against local engine build artifacts, specify `../../out/Shot` as the library directory

## Unicode paths on Windows

Certain Windows JDK launchers convert command-line arguments through the active ANSI code page before `main(String[] args)` receives them, potentially corrupting non-ASCII paths; to handle arbitrary Unicode file paths, specify arguments in a UTF-8 JSON file (without BOM) and pass it via `--config`, with relative paths inside resolving against the current working directory

```json
{
  "libraryDir": "native/shotium-c-abi-windows-amd64",
  "input": "示例/card.html",
  "output": "截图.png"
}
```

```powershell
java -cp "target/classes;target/dependency/*" ShotiumDemo --config java-config.json
```

## How it works

`src/main/java/ShotiumDemo.java` demonstrates the complete invocation lifecycle:

```java
public interface Api extends Library {
    int shot_abi_version();
    int shot_engine_create(String options, PointerByReference engine, PointerByReference error);
    int shot_engine_capture(Pointer engine, String request, PointerByReference image, PointerByReference stats, PointerByReference error);
    void shot_engine_destroy(Pointer engine);
    Pointer shot_buffer_data(Pointer buffer);
    SizeT shot_buffer_size(Pointer buffer);     // size_t, pointer-sized on every platform
    void shot_buffer_free(Pointer buffer);
}

api = Native.load(directory.resolve("libshotium.so").toString(), Api.class, Map.of(Library.OPTION_STRING_ENCODING, "UTF-8"));
if (api.shot_abi_version() != 3) {
    throw new IllegalStateException("C ABI mismatch");
}

PointerByReference engine = new PointerByReference();
PointerByReference error = new PointerByReference();
int status = api.shot_engine_create(gson.toJson(Map.of("resourceDir", directory.toString())), engine, error);
try {
    if (status != 0) {
        throw new IllegalStateException("create failed: " + text(error.getValue()));
    }
} finally {
    api.shot_buffer_free(error.getValue());
}

try {
    PointerByReference image = new PointerByReference();
    PointerByReference stats = new PointerByReference();
    String request = gson.toJson(Map.of(
        "file", input,
        "allowFileAccess", true,
        "width", 720,
        "height", 380,
        "type", "png"
    ));
    status = api.shot_engine_capture(engine.getValue(), request, image, stats, error);
    try {
        if (stats.getValue() != null) {
            System.out.println(text(stats.getValue())); // present on failure too
        }
        if (status != 0) {
            throw new IllegalStateException("capture failed (" + status + "): " + text(error.getValue()));
        }
        Files.write(Path.of(args[2]), bytes(image.getValue()));
    } finally {
        api.shot_buffer_free(image.getValue());
        api.shot_buffer_free(stats.getValue());
        api.shot_buffer_free(error.getValue());
    }
} finally {
    api.shot_engine_destroy(engine.getValue());
}
```

## Java notes

- `size_t` must not be mapped directly to Java `long`: on 64-bit Windows, C `long` remains 32 bits; subclassing `IntegerType` with `Native.SIZE_T_SIZE` ensures correct pointer-width integer mapping across all platforms
- Set `Library.OPTION_STRING_ENCODING` to UTF-8 so JNA avoids falling back to platform-default character sets for JSON strings
- The `bytes()` helper uses `Pointer.getByteArray` to copy memory before invoking `shot_buffer_free()`; never wrap engine buffer pointers in JNA `Memory` instances, as JNA will attempt JVM garbage collection on them
- The `Api` proxy instance is referenced in a static field to remain loaded throughout the JVM lifecycle
- Operations are enclosed in `finally` blocks to guarantee all native buffers and engine instances are freed even on failure

## See also

Complete ownership contracts, request options, performance statistics, tiling, and cache maintenance APIs are detailed in the [C ABI Guide](../c-abi/README.md); the C header `shot_api.h` serves as the authoritative interface specification

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)
