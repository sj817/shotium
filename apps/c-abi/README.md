# shotium C ABI

English · [简体中文](./README.zh.md)

Cross-platform native C shared library powered by Chromium's static rendering architecture, accessed through standard C header `shot_api.h`

[![C ABI version](https://img.shields.io/badge/C%20ABI-v3-blue.svg?logo=c&logoColor=white)](shot_api.h) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

The shared library interface powering language integrations: `shotium.dll`, `libshotium.so`, or `libshotium.dylib`, accessible through the C header `shot_api.h`

This document is distributed as `C_ABI.md` within release archives and language demo packages; it documents the package layout, memory ownership and lifecycle contracts, and the JSON schemas for engine configuration and capture requests; note that `shot_api.h` serves as the authoritative interface definition

## Table of Contents

| Category | Topics and Quick Links |
|:---|:---|
| **Architecture** | [Archive layout](#archive-layout) · [Lifecycle](#lifecycle) · [Buffers and ownership](#buffers-and-ownership) · [Status codes](#status-codes) |
| **API & Config** | [C API declarations](#c-api-declarations) · [Engine options](#engine-options) · [Capture request](#capture-request) · [Capture statistics](#capture-statistics) · [Tiles](#tiles) |
| **Operations** | [Status and memory](#status-and-memory) · [HTTP cache](#http-cache) · [Minimal C program](#minimal-c-program) · [Language demos](#language-demos) · [License](#license) |

## Archive layout

Each release provides prebuilt archives per platform: `shotium-<os>-<arch>-v<version>.7z`, where `<os>` is `windows`, `linux`, or `macos`, and `<arch>` is `amd64` or `arm64`; select the architecture matching the host runtime process loading the library; extracting the archive yields:

| File | Purpose |
|---|---|
| `shotium` / `shotium.exe` | Standalone CLI utility |
| `libshotium.so` / `libshotium.dylib` / `shotium.dll` | C ABI shared library |
| `shotium.dll.lib` | Windows import library (for compile-time linking; demos load by dynamic path) |
| `shot_api.h` | C interface header |
| `shotium_data.pak`, `shotium_strings.pak` | Engine resource packages required at startup |
| `C_ABI.md`, `C_ABI.zh.md` | Interface documentation |
| `LICENSE` | BSD-3-Clause license |

Ensure the library binary and both `.pak` files are kept in the same directory from the matching release; pass this directory path as `resourceDir` during engine initialization; because shared libraries cannot reliably locate their own directory across all operating systems (e.g. on Linux, module path lookups typically resolve to the host executable), the host application must explicitly specify the resource directory

Linux binaries are compiled against glibc (musl environments such as Alpine are not supported out of the box); font rendering relies on system fonts; ensure required font packages (e.g., Fontconfig, DejaVu, Noto) are installed in container environments

## Lifecycle

1. Load the shared library and keep it mapped throughout the process lifecycle; do not unload the library while engine handles or background threads remain active
2. Call `shot_abi_version()` and verify it matches `SHOT_ABI_VERSION` from the header (currently **3**); abort if there is a version mismatch
3. Call `shot_engine_create(options_json, &engine, &error)` once; this call blocks until Blink initialization completes
4. Invoke `shot_engine_capture()` or `shot_engine_capture_tiles()` as needed; each call blocks until rendering and image encoding complete
5. Call `shot_engine_destroy(engine)` during process shutdown, waiting for any in-flight capture to finish and joining the engine worker thread

Because Blink operates as a process-level singleton, callers must observe the following constraints:

- **Single engine instance per process lifecycle**: Recreating an engine after destroying one is not supported; subsequent calls to `shot_engine_create()` will return `SHOT_ERR_STATE`; applications should reuse a single engine instance for captures and spawn additional worker processes if concurrency or isolation is needed
- **Thread-safe sequential dispatch**: `shot_engine_capture()` can be invoked concurrently from any thread; incoming requests are internally serialized onto the engine thread; host applications using event loops should invoke capture calls on worker threads to avoid blocking the event loop
- **Rendering constraints**: JavaScript execution is disabled in rendered pages; top-level `data:` URLs are not supported as the primary document URL; callers should save HTML content to local files or serve them over HTTP/HTTPS

## Buffers and ownership

All dynamic payloads returned by the C ABI are wrapped in a `shot_buffer*` struct, including image bytes, diagnostic metrics, error messages, and JSON status strings; access the payload using `shot_buffer_data()` and `shot_buffer_size()`, and release it via `shot_buffer_free()`

> [!WARNING]
> Always free buffers with `shot_buffer_free()`. Never pass a buffer pointer to standard libc `free()` or host language garbage collectors/allocators; the engine and host may link against different allocators; releasing memory across allocator boundaries will cause undefined behavior or heap corruption; calling `shot_buffer_free(NULL)` is a safe no-op

- Error and JSON strings are NUL-terminated and can be read directly as C strings
- Image byte buffers contain raw encoded binary data and are **not** NUL-terminated (always use `shot_buffer_size()`)
- Output parameters (`out_*`) are initialized and cleared by API functions upon invocation, allowing variable reuse across calls
- On `shot_engine_capture()`, exactly one of `*out_image` and `*out_error` will be populated; the other is set to `NULL` (the `out_stats` parameter is optional and can be `NULL` if metrics are not required)
- When `path` is specified in the request, the engine writes the output image directly to disk; `*out_image` still receives an empty buffer that must be released with `shot_buffer_free()`
- Performance metrics (`out_stats`) are generated on both success and failure; callers should inspect and free `out_stats` in both branches

## Status codes

| Value | Name | Meaning |
|---|---|---|
| `0` | `SHOT_OK` | Operation succeeded |
| `1` | `SHOT_ERR_USAGE` | Invalid JSON syntax, type mismatch, or missing required field (error string identifies invalid property) |
| `2` | `SHOT_ERR_CAPTURE` | Execution failed (e.g. document failed to load, selector matched no element, encoder error, or timeout reached) |
| `3` | `SHOT_ERR_STATE` | Invalid lifecycle state (e.g., attempting to create a second engine, or invoking capture on an uninitialized/failed engine) |

Always inspect the return status code before accessing output image buffers

## C API declarations

Core interface types and exported function prototypes from `shot_api.h`:

```c
#define SHOT_ABI_VERSION 3

// Opaque handle types
typedef struct shot_engine shot_engine;
typedef struct shot_buffer shot_buffer;
typedef struct shot_tile_list shot_tile_list;

// Status codes
typedef enum shot_status {
  SHOT_OK = 0,
  SHOT_ERR_USAGE = 1,
  SHOT_ERR_CAPTURE = 2,
  SHOT_ERR_STATE = 3,
} shot_status;

// Core lifecycle and capture
int32_t shot_abi_version(void);
shot_status shot_engine_create(const char* options_json, shot_engine** out_engine, shot_buffer** out_error);
shot_status shot_engine_capture(shot_engine* engine, const char* request_json, shot_buffer** out_image, shot_buffer** out_stats, shot_buffer** out_error);
shot_status shot_engine_capture_tiles(shot_engine* engine, const char* request_json, shot_tile_list** out_tiles, shot_buffer** out_stats, shot_buffer** out_error);
void shot_engine_destroy(shot_engine* engine);

// Buffer access and reclamation
const void* shot_buffer_data(const shot_buffer* buffer);
size_t shot_buffer_size(const shot_buffer* buffer);
void shot_buffer_free(shot_buffer* buffer);

// Status and memory reclamation
shot_status shot_engine_status(shot_engine* engine, shot_buffer** out_json, shot_buffer** out_error);
void shot_engine_purge(shot_engine* engine, int release_working_set);

// Tiled capture list access
size_t shot_tile_list_count(const shot_tile_list* tiles);
void shot_tile_list_region(const shot_tile_list* tiles, size_t index, int* x, int* y, int* w, int* h);
const char* shot_tile_list_path(const shot_tile_list* tiles, size_t index);
shot_buffer* shot_tile_list_take_image(shot_tile_list* tiles, size_t index);
void shot_tile_list_free(shot_tile_list* tiles);

// HTTP disk cache management
shot_status shot_cache_list(shot_engine* engine, shot_buffer** out_json, shot_buffer** out_error);
shot_status shot_cache_clear(shot_engine* engine, const char* clear_json, shot_buffer** out_json, shot_buffer** out_error);
```

## Engine options

`shot_engine_create()` accepts a JSON configuration object. All fields are optional:

| Field | Type | Default | Meaning |
|---|---|---|---|
| `resourceDir` | string | Executable directory | Directory containing `shotium_data.pak` and `shotium_strings.pak`. Set to the extracted package directory |
| `cacheDir` | string | none | HTTP disk cache directory. If omitted, disk caching is disabled and requests will make network round-trips |
| `cacheMaxBytes` | integer | `0` | Maximum disk cache size in bytes. When set to `0`, Chromium dynamically calculates the cache ceiling based on available disk space |
| `userAgent` | string | built-in | Custom `User-Agent` string |
| `allowFileAccess` | boolean | `false` | Default file access policy for local file URLs (`file://`). Requests explicitly specifying `allowFileAccess` override this value |

```json
{"resourceDir": "/opt/shotium/shotium-linux-amd64", "cacheDir": "/var/cache/shotium", "cacheMaxBytes": 268435456}
```

## Capture request

`shot_engine_capture()` accepts a JSON request string whose schema corresponds to the TypeScript SDK's `ScreenshotOptions`, with a flattened viewport definition (`width` and `height` are top-level fields). The `file` property is required; all other fields are optional:

| Field | Type | Default | Meaning |
|---|---|---|---|
| `file` | string | required | Document URL (`http:`, `https:`, `file:`) or local file path |
| `width`, `height` | integer | `1280`, `720` | Layout viewport in CSS pixels |
| `type` | `"png"` / `"jpeg"` / `"webp"` | `"png"` | Target image format |
| `quality` | integer 1-100 | `90` | Encoding quality (`jpeg` and `webp` only) |
| `scale` | number 0.01-8 | `1` | Device scale factor |
| `fullPage` | boolean | `false` | Capture the complete scrollable document |
| `selector` | string | none | Clip capture to the bounding box of the first matching CSS selector |
| `clip` | `{x, y, width, height}` | none | Specific document clip rectangle in CSS pixels |
| `omitBackground` | boolean | `false` | Preserve transparent background (not supported for `jpeg`) |
| `path` | string | none | Output destination file path |
| `timeout` | integer ms | `30000` | Navigation and render timeout |
| `waitUntil` | `"load"` / `"networkidle"` | `"load"` | Ready condition (`networkidle` waits for 500 ms of network quiescence) |
| `cache` | `"default"` / `"reload"` / `"no-store"` / `"only-if-cached"` | `"default"` | HTTP cache policy for document and subresources |
| `headers` | object of strings | none | Custom request headers (sent only to same-origin requests) |
| `allowFileAccess` | boolean | engine default | Allow document to load local `file:` subresources |
| `tile` | `{height}` | none | Tiled capture configuration (see below) |

`fullPage`, `selector`, and `clip` are mutually exclusive. Relative paths in `file` resolve against the host process's current working directory; passing absolute paths is recommended

```json
{"file": "/abs/card.html", "allowFileAccess": true, "width": 720, "height": 380, "type": "png"}
```

## Capture statistics

When `out_stats` is provided, it receives a JSON object detailing execution metrics. Timings are reported in milliseconds as floating-point numbers:

| Field | Meaning |
|---|---|
| `requests` | Total number of HTTP requests issued (including main document) |
| `fromCache` | Number of subresource requests served from HTTP cache (including 304 Not Modified revalidations) |
| `failed` | Number of failed subresource requests |
| `bytes` | Total decoded payload bytes received |
| `httpStatus` | HTTP status code of the main document (`0` for `file:` URLs) |
| `finalUrl` | Final URL after following HTTP redirects |
| `timing.fetch` | Document network fetch duration (DNS, TCP, TLS, and round-trip) |
| `timing.render` | Combined duration of document parsing, resource loading, style recalculation, layout, and painting |
| `timing.setup`, `timing.wait`, `timing.lifecycle`, `timing.paint`, `timing.raster`, `timing.encode` | Breakdown of internal capture lifecycle phases |
| `timing.total` | Total end-to-end elapsed time |

## Tiles

When capturing exceptionally tall pages or documents exceeding bitmap format limits (e.g., PNG/JPEG maximum dimension 65535px, WebP 16383px), use `shot_engine_capture_tiles()` with the `tile` property configured (for example, `{"file": "...", "fullPage": true, "tile": {"height": 8000}}`)

The page layout and resource loading occur once, after which the render target is sliced horizontally into tile segments of at most `tile.height` CSS pixels (maximum allowed: 32000px); each tile is encoded independently

The result is returned as a `shot_tile_list*`:

| Function | Purpose |
|---|---|
| `shot_tile_list_count(tiles)` | Total number of tiles generated |
| `shot_tile_list_region(tiles, i, &x, &y, &w, &h)` | Bounding box of tile `i` in document CSS pixels (any coordinate pointer may be `NULL`) |
| `shot_tile_list_path(tiles, i)` | Written file path for tile `i` when `path` was specified, or `NULL` |
| `shot_tile_list_take_image(tiles, i)` | Transfers ownership of tile `i`'s image buffer (must be released with `shot_buffer_free()`) |
| `shot_tile_list_free(tiles)` | Releases the tile list and any remaining unconsumed image buffers |

When specifying `path` for tiled captures, the template string must include `{n}` as a placeholder for the 1-based tile index; on completion, exactly one of `*out_tiles` and `*out_error` will be populated

## Status and memory

`shot_engine_status(engine, &json, &error)` returns engine runtime information formatted as `{"cacheDir": string | null, "cacheActive": boolean}`; if `cacheActive` is `false` despite a configured `cacheDir`, the directory could not be initialized and the engine operates without caching; multiple processes may safely share the same cache directory

`shot_engine_purge(engine, release_working_set)` triggers resource reclamation, clearing Blink GC objects, Skia font/glyph caches, and internal allocator pools; if `release_working_set` is non-zero, it additionally requests the operating system to reclaim physical memory pages; it is recommended to invoke purge after completing a batch workload rather than between every single capture

## HTTP cache

`shot_cache_list()` and `shot_cache_clear()` provide out-of-band inspection and maintenance of HTTP cache directories; pass the active `engine` pointer when an engine exists in the current process; pass `NULL` if called in a utility process where no engine has been initialized

Cache maintenance options:

| Field | Applies to | Meaning |
|---|---|---|
| `cacheDir` | Both | Target cache directory path (required) |
| `urls` | `clear` | Specific resource URLs to evict |
| `unusedSinceMs` | `clear` | Evict cache entries not accessed since this Unix millisecond timestamp |
| `maxBytes` | `clear` | Evict entries via LRU until cache directory size is at or below this threshold |

Filter criteria compose when combined; executing a clear without filters purges the entire cache directory; the C ABI does not include glob or wildcard matching; applications should retrieve the entry list with `shot_cache_list()`, filter targets programmatically, and supply specific URLs for removal

Return structures: `shot_cache_list()` outputs an array of `{url, lastUsedMs, bytes}`; `shot_cache_clear()` outputs `{removed, bytesBefore, bytesAfter}` (`removed` is `-1` if the entire directory was removed at once)

## Minimal C program

```c
#include <stdio.h>
#include "shot_api.h"

int main(void) {
  if (shot_abi_version() != SHOT_ABI_VERSION) return 1;
  shot_engine* engine = NULL;
  shot_buffer* error = NULL;
  if (shot_engine_create("{\"resourceDir\":\"/opt/shotium\"}", &engine, &error) != SHOT_OK) {
    fprintf(stderr, "%s\n", (const char*)shot_buffer_data(error));
    shot_buffer_free(error);
    return 1;
  }
  shot_buffer* png = NULL;
  shot_buffer* stats = NULL;
  shot_status status = shot_engine_capture(
      engine, "{\"file\":\"https://example.com\",\"width\":720,\"height\":380}", &png, &stats, &error);
  if (status == SHOT_OK) {
    FILE* out = fopen("example.png", "wb");
    fwrite(shot_buffer_data(png), 1, shot_buffer_size(png), out);
    fclose(out);
  } else {
    fprintf(stderr, "capture failed (%d): %s\n", status, (const char*)shot_buffer_data(error));
  }
  if (stats) printf("%s\n", (const char*)shot_buffer_data(stats));
  shot_buffer_free(png);
  shot_buffer_free(stats);
  shot_buffer_free(error);
  shot_engine_destroy(engine);
  return status == SHOT_OK ? 0 : 1;
}
```

## Language demos

Runnable demo implementations are provided across five languages using generic foreign function interfaces without custom wrapper packages: [Go](../go/README.md) (purego), [Python](../python/README.md) (ctypes), [Rust](../rust/README.md) (libloading), [C#](../csharp/README.md) (P/Invoke), and [Java](../java/README.md) (JNA); each demo is distributed as an independent archive on the release page, rendering the standard `card.html` at 720×380 with pixel-identical output

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)

