# Precompiled C ABI / 预编译 C ABI

## Distribution

Download the matching `.7z` from [GitHub Releases](https://github.com/sj817/shotium/releases). Use the host process's architecture, not merely the OS architecture. Each archive contains the CLI, C ABI shared library, `shot_api.h`, two `.pak` resources, `LICENSE` and this guide (`C_ABI.md`). Windows also includes `shotium.dll.lib` for callers that link at build time. The five language demos load the library by absolute path and do not need an import library.

| System / 系统 | Archive / 压缩包 | Library / 动态库 |
|---|---|---|
| Windows x64 / arm64 | `shotium-windows-{amd64,arm64}-v<VERSION>.7z` | `shotium.dll` |
| Linux x64 / arm64 | `shotium-linux-{amd64,arm64}-v<VERSION>.7z` | `libshotium.so` |
| macOS x64 / arm64 | `shotium-macos-{amd64,arm64}-v<VERSION>.7z` | `libshotium.dylib` |

Extract the entire archive. Keep `shotium_data.pak` and `shotium_strings.pak` together with the library and pass that directory as `resourceDir`. System libraries and fonts are still required; precompiled does not mean compatible with every older OS or Linux libc. Linux artifacts target glibc, not Alpine/musl. Do not replace resource files with files from another release.

```bash
# Example with GitHub CLI and 7-Zip; select your actual platform and release.
gh release download v0.7.0 -R sj817/shotium --pattern 'shotium-linux-amd64-v0.7.0.7z' --dir apps/native
7z x apps/native/shotium-linux-amd64-v0.7.0.7z -oapps/native
python3 apps/python/screenshot.py apps/native/shotium-linux-amd64 apps/fixtures/hello.html output.png
```

```powershell
gh release download v0.7.0 -R sj817/shotium --pattern 'shotium-windows-amd64-v0.7.0.7z' --dir apps/native
7z x apps/native/shotium-windows-amd64-v0.7.0.7z -oapps/native
python apps/python/screenshot.py apps/native/shotium-windows-amd64 apps/fixtures/hello.html output.png
```

The project currently publishes **no separate Shotium packages for Go, Python, Rust, C# or Java**. These are examples using generic FFI libraries. The existing Node npm package continues independently, and its platform package contains `.node`, CLI and resources, not the C ABI shared library.

## Contract

The authoritative declaration is the `shot_api.h` included in the same archive, also available [in source](https://github.com/sj817/shotium/blob/main/shot/shot_api.h). Current ABI version: **3**. Call `shot_abi_version()` before creating an engine and reject a mismatch. All strings crossing this ABI are NUL-terminated UTF-8 JSON or UTF-8 error messages. Status is a 32-bit integer; handles are pointers; lengths and indices are `size_t` (64-bit on all six supported targets, including Windows).

1. Load the library and keep it mapped until process exit. Do not unload it while native objects or threads may reference it.
2. Call `shot_engine_create(options_json, &engine, &error)`. Explicitly set `resourceDir`. The C ABI defaults to no disk cache; enable it with `cacheDir` and optionally `cacheMaxBytes`.
3. Call `shot_engine_capture(engine, request_json, &image, &stats, &error)` for each image. Calls block the calling thread and serialize onto the dedicated engine thread. Use your host's executor if its event loop must remain responsive.
4. Read `shot_buffer_data/size` while the buffer is alive. Copy or consume its bytes, then call **`shot_buffer_free`** for every returned image, stats and error buffer. Never use the host's `free`, Go allocator, Rust allocator, `Marshal.FreeHGlobal`, or JNA `Memory.close` on an engine buffer. Null buffers are safe to free.
5. Call `shot_engine_destroy` after callers have finished. It drains accepted work and joins the thread. Do not race a new call using a raw handle against destruction.

Only one engine can be created **for the lifetime of a process**. Destroying it does not permit creating another. Reuse a live engine for many captures; use separate processes for parallel engines. Node's public `stop/start` reuses its internal service and is not the C ABI's `destroy/create` contract. No JavaScript is executed in rendered pages.

| Status | Meaning |
|---|---|
| `0 SHOT_OK` | Success |
| `1 SHOT_ERR_USAGE` | Invalid JSON/options |
| `2 SHOT_ERR_CAPTURE` | Load, timeout, selector or rendering failure |
| `3 SHOT_ERR_STATE` | Invalid lifecycle, including repeat creation |

Check status before using image bytes. Capture failures can return both error and statistics; malformed requests may have no stats. Error buffers are text; image buffers are binary and must always use their explicit length.

## Requests and additional functions

```json
{"resourceDir":"/absolute/native/directory","cacheDir":"/absolute/cache","cacheMaxBytes":268435456}
```

```json
{"file":"/absolute/page.html","allowFileAccess":true,"width":800,"height":600,"type":"png"}
```

`file` also accepts HTTP(S) URLs. Relative filesystem paths resolve against the host process's current directory, so examples use absolute paths. `allowFileAccess` explicitly enables local input and local resources. Request fields follow the [Node options reference](https://github.com/sj817/shotium/blob/main/apps/demo/shotium/src/types.ts), **except `viewport.width/height` become top-level `width/height`**. The complete wire definition and defaults are in [shot_request.h](https://github.com/sj817/shotium/blob/main/shot/shot_request.h); Node-only conveniences such as an options object are not a separate C function signature.

- File output: set `path`; success returns an empty image buffer and writes the file. Still free the buffer.
- Full page / clipping: `fullPage`, `selector`, `clip`, `scale` share the core validation and rendering behavior.
- Formats: `type` is `png`, `jpeg` or `webp`; `quality` configures lossy output.
- Tiles: call `shot_engine_capture_tiles` with `tile: {"height": 8000}`. Enumerate using `shot_tile_list_count/region/path`; `shot_tile_list_take_image` transfers one image's ownership to the caller. Free taken images separately, then `shot_tile_list_free` frees remaining images. With file output, `path` must contain `{n}`.
- `shot_engine_status` and `shot_engine_purge` expose cache status and memory release. Status/error outputs follow the same buffer ownership rules.
- `shot_cache_list/clear` use the active engine's cache when available and can access a standalone disk cache before engine creation. Consult the header for arguments and JSON fields.

## 中文说明

下载与宿主进程架构一致的 Release 压缩包，完整解压，动态库与两个资源包必须来自同一版本。通过 `resourceDir` 传入资源目录；示例均使用绝对路径，并显式允许读取本地 HTML。Linux 版本面向 glibc，不是 musl/Alpine；运行仍需要系统库和字体。

先检查 ABI 版本 3，再创建引擎。C ABI 是同步阻塞接口，内部专用线程串行渲染；一个进程一生只能创建一次引擎，多次截图复用该引擎，销毁后不能重建。需要并行引擎时使用多个进程。不要将 Node 层的 `stop/start` 等同于 C ABI 的 `destroy/create`。

返回的图片、统计、错误缓冲区都由引擎分配，必须分别调用 `shot_buffer_free`，不能由调用语言释放。先读取或复制数据，再释放；失败时也应读取和释放可用统计。调用方结束所有请求后再销毁引擎，并让动态库保持加载直到进程退出。

请求的宽高位于 JSON 顶层，不使用 Node 的 `viewport` 嵌套对象。文件输出返回空图片缓冲区，也要释放。分片列表会释放尚未取走的图片；通过 `take_image` 取出的图片转归调用方，需单独释放。完整声明以压缩包中的 `shot_api.h` 为准。

项目尚处初期，目前仅维护通用 C ABI、预编译库和[五种语言示例](https://github.com/sj817/shotium/tree/main/apps)，暂不发布对应语言包；有需求后再做。npm 的 Node 包继续发布，npm 平台包不携带独立 C ABI 动态库。
