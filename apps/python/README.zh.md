# shotium Python 示例

[English](./README.md) · 简体中文

[![Python](https://img.shields.io/badge/Python-3.9+-3776AB?logo=python&logoColor=white)](https://python.org/) [![ctypes](https://img.shields.io/badge/FFI-ctypes%20(zero%20deps)-blue.svg)](https://docs.python.org/3/library/ctypes.html) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

展示如何通过 Python 标准库 `ctypes` 调用 shotium C ABI 进行网页渲染与截图；本工程为独立运行示例，未打包发布为 PyPI 模块

## 目录

[环境要求](#环境要求) · [获取文件](#获取文件) · [运行](#运行) · [代码结构](#代码结构) · [Python 相关说明](#python-相关说明) · [延伸阅读](#延伸阅读) · [许可证](#许可证)

## 环境要求

- Python 3.9 或更高版本（仅使用标准库 `ctypes` 与 `json`，无需安装三方依赖）
- 7-Zip（`7z`），用于解压预编译原生引擎包
- 匹配当前 Python 解释器架构的原生二进制包（注意匹配 Python 解释器架构，如 Windows arm64 下运行 x86_64 仿真解释器需选用对应包）

## 获取文件

本示例源码独立打包为 [`shotium-example-python.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-python.7z)，解压后结合对应平台的原生动态库 `shotium-c-abi-<平台>.7z` 即可运行：

```bash
# Linux / macOS（以 linux-amd64 为例，需要 7z 或 7zz）
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-python.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-python.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-python/native
cd shotium-example-python
```

```powershell
# Windows PowerShell（以 windows-amd64 为例）
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-python.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-python.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-python\native
Set-Location shotium-example-python
```

> 完整性校验：可下载 [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) 并执行 `sha256sum --check --ignore-missing SHA256SUMS` 校验；原生动态库解压位于 `native/shotium-c-abi-<平台>/`，包含动态库、头文件与 `.pak` 资源包


## 运行

```bash
python3 screenshot.py native/shotium-c-abi-linux-amd64 card.html card.png
```

Windows 环境下通常执行 `python`；命令行参数依次为 `<动态库目录> <输入 HTML 路径> <输出 PNG 路径>`，其中动态库目录会同时传入引擎作为 `resourceDir`；脚本会将 `card.html` 渲染为 720×380 的图像，把性能统计指标以 JSON 打印到标准输出，并生成 `card.png`，其渲染输出与 `shotium` CLI 命令行完全一致

如指定不存在的文件可测试错误处理流程：程序退出码为 1，向标准错误输出 `capture failed (2): ...`，输出中仍包含统计信息，但不会生成输出图片

在本地编译产物中测试时，动态库目录可指定为 `../../out/Shot`

## 代码结构

`screenshot.py` 包含了完整的调用逻辑：

```python
lib = ctypes.CDLL(str(directory / "libshotium.so"))   # 进程生命周期内保持加载
ptr, out = ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)
lib.shot_abi_version.restype = ctypes.c_int32
lib.shot_engine_create.argtypes = [ctypes.c_char_p, out, out]
lib.shot_engine_capture.argtypes = [ptr, ctypes.c_char_p, out, out, out]
lib.shot_buffer_size.restype = ctypes.c_size_t
lib.shot_buffer_data.restype = ptr
# 以及 shot_engine_destroy、shot_buffer_free

if lib.shot_abi_version() != 3:
    raise RuntimeError("C ABI mismatch")

engine = ptr()
error = ptr()
if lib.shot_engine_create(json.dumps({"resourceDir": str(directory)}).encode(), byref(engine), byref(error)):
    raise RuntimeError(take(error))          # take() 先复制字节，再释放缓冲区

try:
    image = ptr()
    stats = ptr()
    error = ptr()
    request = {
        "file": str(input.resolve()),
        "allowFileAccess": True,
        "width": 720,
        "height": 380,
    }
    status = lib.shot_engine_capture(engine, json.dumps(request).encode(), byref(image), byref(stats), byref(error))
    data, statistics, message = take(image), take(stats), take(error)
    print(statistics)                        # 失败时同样提供统计指标
    if status:
        raise RuntimeError(f"capture failed ({status}): {message}")
    output.write_bytes(data)
finally:
    lib.shot_engine_destroy(engine)
```

## Python 相关说明

- 调用前需严格为每个 C 函数声明 `argtypes` 与 `restype`；若未显式声明，`ctypes` 在 64 位 Windows 上会将指针隐式截断为 32 位整数（`shot_buffer_size` 返回值必须声明为 `ctypes.c_size_t`）
- `take()` 辅助函数通过 `ctypes.string_at` 按 `shot_buffer_size` 长度将 C 原生内存拷贝为 Python `bytes` 对象，随后立即调用 `shot_buffer_free`；严禁将原生指针交由 Python 内存管理器释放
- 引擎实例在 `finally` 块中调用 `shot_engine_destroy` 销毁，确保即使发生异常也能正确等待并回收引擎后台线程
- 文件路径在构造请求 JSON 前需转换为绝对路径，避免由于引擎工作目录与调用方执行目录不同产生路径解析歧义

## 延伸阅读

完整的接口规范、请求参数字段、性能统计指标、长图分片和缓存管理详见 [C ABI 文档](../c-abi/README.zh.md)；C 头文件 `shot_api.h` 是权威的接口定义

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)
