# shotium C# 示例

[English](./README.md) · 简体中文

[![.NET](https://img.shields.io/badge/.NET-8.0+-512BD4?logo=dotnet&logoColor=white)](https://dotnet.microsoft.com/) [![P/Invoke](https://img.shields.io/badge/FFI-P%2FInvoke%20(zero%20deps)-blue.svg)](https://learn.microsoft.com/dotnet/standard/native-interop/pinvoke) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

展示如何通过 P/Invoke 在 .NET 中直接调用 shotium C ABI 进行网页渲染与截图，无需引入任何第三方 NuGet 依赖；本工程为独立运行示例项目，未作为 NuGet 包发布

## 目录

[环境要求](#环境要求) · [获取文件](#获取文件) · [运行](#运行) · [代码结构](#代码结构) · [.NET 相关说明](#net-相关说明) · [延伸阅读](#延伸阅读) · [许可证](#许可证)

## 环境要求

- .NET SDK 8.0 或更高版本（目标框架为 `net8.0`，无额外包依赖）
- 7-Zip（`7z`），用于解压预编译原生引擎包
- 匹配目标 .NET 运行时架构的引擎二进制包（可通过 `dotnet --info` 查看当前环境的 `RID`）

## 获取文件

本示例源码独立打包为 [`shotium-example-csharp.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-csharp.7z)，解压后结合对应平台的原生动态库 `shotium-c-abi-<平台>.7z` 即可运行：

```bash
# Linux / macOS（以 linux-amd64 为例，需要 7z 或 7zz）
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-csharp.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-csharp.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-csharp/native
cd shotium-example-csharp
```

```powershell
# Windows PowerShell（以 windows-amd64 为例）
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-csharp.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-csharp.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-csharp\native
Set-Location shotium-example-csharp
```

> 完整性校验：可下载 [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) 并执行 `sha256sum --check --ignore-missing SHA256SUMS` 校验；原生动态库解压位于 `native/shotium-c-abi-<平台>/`，包含动态库、头文件与 `.pak` 资源包

## 运行

```bash
dotnet run -c Release -- native/shotium-c-abi-linux-amd64 card.html card.png
```

命令行参数依次为 `<动态库目录> <输入 HTML 路径> <输出 PNG 路径>`，其中动态库目录会同时传入引擎作为 `resourceDir`；程序会将 `card.html` 渲染为 720×380 的图像，把性能统计指标以 JSON 打印到标准输出，并生成 `card.png`，其渲染输出与 `shotium` CLI 命令行完全一致

如指定不存在的文件可测试错误处理流程：程序退出码为 1，向标准错误输出 `capture failed (2): ...`，输出中仍包含统计信息，但不会生成输出图片

在本地编译产物中测试时，动态库目录可指定为 `../../out/Shot`

## 代码结构

`Program.cs` 包含了完整的调用逻辑；P/Invoke 声明中的动态库标识为 `shotium`，并通过 `SetDllImportResolver` 动态解析到指定路径的原生库文件：

```csharp
var library = NativeLibrary.Load(Path.Combine(directory, "libshotium.so")); // 进程存续期内不卸载
NativeLibrary.SetDllImportResolver(typeof(Shotium).Assembly, (name, _, _) => name == "shotium" ? library : IntPtr.Zero);

if (Shotium.shot_abi_version() != 3)
{
    throw new InvalidOperationException("C ABI mismatch");
}

var status = Shotium.shot_engine_create(JsonSerializer.Serialize(new { resourceDir = directory }), out var engine, out var error);
var message = Shotium.TakeText(error); // 先复制数据，再调用 shot_buffer_free
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
            Console.WriteLine(Shotium.Text(stats)); // 失败时同样提供统计指标
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
    // 以及 shot_abi_version、shot_engine_create、shot_engine_destroy、shot_buffer_data、shot_buffer_free
}
```

## .NET 相关说明

- 字符串封送需显式声明为 `UnmanagedType.LPUTF8Str`；引擎要求 UTF-8 编码，而 .NET 在 Windows 上的默认封送格式为 ANSI
- `shot_buffer_size` 返回的 `size_t` 在 C# 中映射为 `nuint`；原生句柄与指针均声明为 `IntPtr`；跨平台调用约定统一指定为 `CallingConvention.Cdecl`
- `Bytes()` 辅助方法使用 `Marshal.Copy` 在释放原生内存前将数据拷贝至托管字节数组中；原生缓冲区必须且只能由 `shot_buffer_free` 释放，严禁调用 `Marshal.FreeHGlobal`（否则破坏引擎堆内存）
- 通过 `NativeLibrary.Load` 获取的原生库句柄在进程存活期内保持加载，不调用 `NativeLibrary.Free` 卸载，确保引擎后台线程与线程局部存储安全有效
- 通过 `finally` 块确保在捕获异常时也能完整释放原生缓冲区并调用 `shot_engine_destroy` 销毁引擎

## 延伸阅读

完整的接口规范、请求参数字段、性能统计指标、长图分片和缓存管理详见 [C ABI 文档](../c-abi/README.zh.md)；C 头文件 `shot_api.h` 是权威的接口定义

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)
