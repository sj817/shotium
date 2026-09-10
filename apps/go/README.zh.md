# shotium Go 示例

[English](./README.md) · 简体中文

[![Go](https://img.shields.io/badge/Go-1.23+-00ADD8?logo=go&logoColor=white)](https://go.dev/) [![purego](https://img.shields.io/badge/FFI-purego%20(no%20cgo)-blue.svg)](https://github.com/ebitengine/purego) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

展示如何通过 purego 在 Go 中免 cgo 调用 shotium C ABI 进行网页渲染与截图；本工程为独立运行示例，未封装为可导入的 Go package

## 目录

[环境要求](#环境要求) · [获取文件](#获取文件) · [运行](#运行) · [代码结构](#代码结构) · [Go 相关说明](#go-相关说明) · [延伸阅读](#延伸阅读) · [许可证](#许可证)

## 环境要求

- Go 1.23 或更高版本（无需 C/C++ 编译器，通过 [purego](https://github.com/ebitengine/purego) 动态加载）
- 7-Zip（`7z`），用于解压预编译原生引擎包
- 匹配当前架构的引擎二进制包（注意匹配目标 Go 进程运行时的 CPU 架构 `go env GOOS GOARCH`）

## 获取文件

本示例源码独立打包为 [`shotium-example-go.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-go.7z)，解压后结合对应平台的原生动态库 `shotium-c-abi-<平台>.7z` 即可运行：

```bash
# Linux / macOS（以 linux-amd64 为例，需要 7z 或 7zz）
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-go.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-go.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-go/native
cd shotium-example-go
```

```powershell
# Windows PowerShell（以 windows-amd64 为例）
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-go.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-go.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-go\native
Set-Location shotium-example-go
```

> 完整性校验：可下载 [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) 并执行 `sha256sum --check --ignore-missing SHA256SUMS` 校验；原生动态库解压位于 `native/shotium-c-abi-<平台>/`，包含动态库、头文件与 `.pak` 资源包


## 运行

```bash
go run -mod=readonly . native/shotium-c-abi-linux-amd64 card.html card.png
```

命令行参数依次为 `<动态库目录> <输入 HTML 路径> <输出 PNG 路径>`，其中动态库目录会同时传入引擎作为 `resourceDir`；程序会将 `card.html` 渲染为 720×380 的图像，把统计指标以 JSON 打印到标准输出，并生成 `card.png`，其渲染输出与 `shotium` CLI 命令行完全一致

如指定不存在的文件可测试错误处理流程：程序退出码为 1，向标准错误输出 `capture failed (2): ...`，输出中仍包含统计信息，但不会生成输出图片

在本地编译产物中测试时，动态库目录可指定为 `../../out/Shot`

## 代码结构

`main.go` 包含了完整的调用逻辑，典型交互流程如下：

```go
handle, _ := loadLibrary(filepath.Join(dir, "libshotium.so")) // 进程生命周期内保持映射
purego.RegisterLibFunc(&abi, handle, "shot_abi_version")
purego.RegisterLibFunc(&create, handle, "shot_engine_create")
purego.RegisterLibFunc(&capture, handle, "shot_engine_capture")
purego.RegisterLibFunc(&free, handle, "shot_buffer_free")
// 以及 shot_engine_destroy、shot_buffer_data、shot_buffer_size

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
	fmt.Println(read(stats)) // 失败时同样提供统计指标
}
if status != 0 {
	return errors.New(read(err))
}

os.WriteFile("card.png", read(image), 0644)
```

`read()` 辅助函数在 `defer free()` 释放原生内存前，将 C 缓冲区中的数据拷贝到 Go 托管的字节切片中

## Go 相关说明

- `load_windows.go` 与 `load_unix.go` 通过构建标签选择加载方式：Windows 使用 `syscall.LoadLibrary`，Unix/macOS 使用 `purego.Dlopen`；句柄在整个进程生命周期中保持有效，不执行卸载
- C ABI 返回的所有 `shot_buffer*` 必须通过 `shot_buffer_free` 释放，严禁交由 Go 运行时垃圾回收；确保在 `defer free(...)` 前完成数据拷贝
- 项目提供 `go.sum` 锁定依赖版本，保证跨环境构建一致性
- 示例以 `main` 包形式组织，实际业务集成可将 `RegisterLibFunc` 绑定和缓冲区处理逻辑提取封装为独立 package

## 延伸阅读

完整的接口规范、请求参数字段、性能统计指标、长图分片和缓存管理详见 [C ABI 文档](../c-abi/README.zh.md)；C 头文件 `shot_api.h` 是权威的接口定义

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)
