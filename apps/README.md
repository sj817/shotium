# Applications and language examples / 应用与语言示例

Shotium is available as a Node package and as a precompiled C ABI shared library.
The project is still young: **Go, Python, Rust, C# and Java examples are source examples, not separately published packages**. Download the native library from GitHub Releases and call it through your language's FFI. Language packages can be added when there is demand.

项目初期暂不发布 Go/Python/Rust/NuGet/Maven 的 Shotium 包。各语言直接使用 **C ABI + Release 预编译动态库**；有需求后再提供正式语言包。已有 npm 包 `@shotkit/shotium` 继续发布。

| Directory | Purpose / 用途 |
|---|---|
| [demo/shotium](demo/shotium/README.md) | Complete npm package source, Node-API binding and API reference / 完整 npm 源码与文档 |
| [demo](demo/README.md) | Node + Express screenshot example / Node 服务示例 |
| [c-abi](c-abi/README.md) | Download, ABI, ownership and JSON contract / 通用接入说明 |
| [go](go/README.md) | Go + purego, no C compiler / 无需 C 编译器 |
| [python](python/README.md) | Python standard-library ctypes / 标准库调用 |
| [rust](rust/README.md) | Rust + libloading, RAII ownership / RAII 资源管理 |
| [csharp](csharp/README.md) | .NET + P/Invoke / 无额外 NuGet 依赖 |
| [java](java/README.md) | Java + JNA / Java 原生调用 |
| [benchmark](benchmark/README.md) | Six-platform benchmark harness / 六平台基准 |
| [benchmark-site](benchmark-site/README.md) | Benchmark report site / 基准报告站点 |

All five C ABI demos accept `<library-dir> <input.html> <output.png>`, render the same offline [fixture](fixtures/hello.html) at 800×600, print statistics, and report a nonzero exit code on failure. Pass a missing input file to exercise error reporting and failure statistics. No browser process, Node installation or locally compiled engine is required.

五种 C ABI 示例统一接受“动态库目录、HTML 路径、PNG 输出路径”三个参数，以 800×600 渲染同一离线页面。失败返回非零退出码并打印错误和可用统计；输入不存在的文件可验证失败路径。引擎在当前进程内运行，不需要 Node 或自行编译 Chromium。

From a development checkout, `out/Shot` can be used as the library directory after `pnpm build:engine --target shot_c`. Release users should follow the download instructions in [c-abi](c-abi/README.md). Never mix architectures or resources from different releases.
