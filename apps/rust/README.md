# Rust C ABI demo

This is a source example, **not a published Shotium language package**. It loads the precompiled shared library directly in the host process. / 这是源码示例，不发布独立语言包；直接加载预编译 C ABI 动态库。

## Prepare / 准备

Rust 1.85+ and Cargo; pinned libloading/serde_json, committed Cargo.lock. This crate has publish=false. / 示例 crate 禁止发布，不需要编译引擎。

Follow the [shared download and ABI guide](../c-abi/README.md) to extract the matching Release into `apps/native/`. Keep the library and both `.pak` files from the same release. The language runtime and native library must use the same architecture. On macOS, substitute `shotium-macos-arm64` or `shotium-macos-amd64` for the Linux directory below.

先按通用文档下载并完整解压对应平台的 Release；动态库与资源包必须同版本，宿主进程与库架构一致。macOS 请替换目录名。无需 Node，也无需自行编译 Chromium。

## Run / 运行

Start in the repository root. / 从仓库根目录执行。

Linux / macOS:

```bash
cargo run --manifest-path apps/rust/Cargo.toml --release --locked -- apps/native/shotium-linux-amd64 apps/fixtures/hello.html rust.png
```

Windows PowerShell:

```powershell
cargo run --manifest-path apps/rust/Cargo.toml --release --locked -- apps/native/shotium-windows-amd64 apps/fixtures/hello.html rust.png
```

Arguments: `<library-dir> <input.html> <output.png>`. The first argument also becomes `resourceDir`. The example renders at 800×600, explicitly permits local input, prints capture stats and writes PNG bytes. For a locally built engine, substitute `out/Shot` (Go: `../../out/Shot` after changing directory).

参数分别为动态库目录、输入 HTML、输出 PNG。示例显式设置资源目录、本地文件权限和 800×600 视口，打印统计并保存 PNG。使用本机构建时替换为 `out/Shot`（Go 切换目录后为 `../../out/Shot`）。

## Failures and ownership / 错误与所有权

Replace the input with a nonexistent file: the program must exit nonzero, print `capture failed (2)` and any available failure statistics, and write no image. ABI mismatches are rejected before engine creation. RAII guards free native buffers and destroy the engine, including early error returns. The library stays mapped for the process lifetime. / RAII 保证错误提前返回时仍释放资源，动态库保留到进程退出。

输入不存在的文件可验证失败路径：非零退出码、错误原因和可用失败统计，不产生图片。每个进程只能创建一次引擎；重复截图复用它，销毁后不能重建。动态库保持加载直到进程退出。

The examples use only capture functions; the shared guide also documents file output, tiles, cache operations and memory release. The shipped `shot_api.h` is the authoritative API. / 文件输出、分片、缓存等见通用文档，完整接口以同包头文件为准。
