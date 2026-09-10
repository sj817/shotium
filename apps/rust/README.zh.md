# shotium Rust 示例

[English](./README.md) · 简体中文

[![Rust](https://img.shields.io/badge/Rust-1.85+-orange?logo=rust&logoColor=white)](https://rust-lang.org/) [![libloading](https://img.shields.io/badge/FFI-libloading-blue.svg)](https://crates.io/crates/libloading) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

展示如何通过 `libloading` 在 Rust 中直接调用 shotium C ABI 进行网页渲染与截图，并使用 RAII 模式安全管理原生句柄；本工程为独立运行示例 crate（配置了 `publish = false`），未在 crates.io 上发布

## 目录

[环境要求](#环境要求) · [获取文件](#获取文件) · [运行](#运行) · [代码结构](#代码结构) · [Rust 相关说明](#rust-相关说明) · [延伸阅读](#延伸阅读) · [许可证](#许可证)

## 环境要求

- Rust 1.85 或更高版本与 Cargo 工具链（依赖 `libloading` 与 `serde_json` 已在 `Cargo.toml` 声明并由 `Cargo.lock` 锁定）
- 7-Zip（`7z`），用于解压预编译原生引擎包
- 匹配当前目标编译架构的引擎二进制包（可通过 `rustc -vV` 查看 `host` 架构）

## 获取文件

本示例源码独立打包为 [`shotium-example-rust.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-rust.7z)，解压后结合对应平台的原生动态库 `shotium-c-abi-<平台>.7z` 即可运行：

```bash
# Linux / macOS（以 linux-amd64 为例，需要 7z 或 7zz）
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-rust.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-rust.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-rust/native
cd shotium-example-rust
```

```powershell
# Windows PowerShell（以 windows-amd64 为例）
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-rust.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-rust.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-rust\native
Set-Location shotium-example-rust
```

> 完整性校验：可下载 [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) 并执行 `sha256sum --check --ignore-missing SHA256SUMS` 校验；原生动态库解压位于 `native/shotium-c-abi-<平台>/`，包含动态库、头文件与 `.pak` 资源包


## 运行

```bash
cargo run --release --locked -- native/shotium-c-abi-linux-amd64 card.html card.png
```

命令行参数依次为 `<动态库目录> <输入 HTML 路径> <输出 PNG 路径>`，其中动态库目录会同时传入引擎作为 `resourceDir`；程序会将 `card.html` 渲染为 720×380 的图像，把性能统计指标以 JSON 打印到标准输出，并生成 `card.png`，其渲染输出与 `shotium` CLI 命令行完全一致

如指定不存在的文件可测试错误处理流程：程序退出码为 1，向标准错误输出 `capture failed (2): ...`，输出中仍包含统计信息，但不会生成输出图片

在本地编译产物中测试时，动态库目录可指定为 `../../out/Shot`

## 代码结构

`src/main.rs` 包含了完整的调用逻辑，通过 RAII 封装原生资源句柄，确保无论正常返回还是发生错误都能正确析构：

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
    println!("{}", stats.text()); // 失败时同样提供统计指标
}
if status != 0 {
    return Err(format!("capture failed ({status}): {}", error.text()).into());
}

fs::write("card.png", image.bytes())?;
```

## Rust 相关说明

- `Library` 使用 `ManuallyDrop` 包裹，防止触发 `dlclose`；引擎内部维持着独立的后台线程与线程局部存储状态，在其存续期间卸载动态库会导致未定义行为
- `Buffer::bytes()` 根据 `shot_buffer_data` 与 `shot_buffer_size` 构造出受 `Buffer` 生命周期约束的原生切片；`Buffer` 的 `Drop` 实现显式调用 `shot_buffer_free`，严禁使用 Rust 内存分配器介入回收
- 通过 Rust 的作用域与变量声明顺序，保证 `Engine` 在所有借用 `Api` 的 `Buffer` 释放后才执行析构销毁
- 输入路径仅转换为绝对路径，未调用 `canonicalize`，以便让不存在的文件顺利传递至引擎，从而完整验证 C ABI 的错误处理与统计指标返回逻辑
- 动态库加载会触发库内部的初始化流程，请确保仅从可信来源加载匹配目标架构的预编译产物

## 延伸阅读

完整的接口规范、请求参数字段、性能统计指标、长图分片和缓存管理详见 [C ABI 文档](../c-abi/README.zh.md)；C 头文件 `shot_api.h` 是权威的接口定义

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)
