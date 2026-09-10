# shotium Java 示例

[English](./README.md) · 简体中文

[![Java](https://img.shields.io/badge/Java-17+-ED8B00?logo=openjdk&logoColor=white)](https://openjdk.org/) [![JNA](https://img.shields.io/badge/FFI-JNA%205.17-blue.svg)](https://github.com/java-native-access/jna) [![platforms](https://img.shields.io/badge/platforms-win%20%7C%20mac%20%7C%20linux%20%C2%B7%20x64%20%7C%20arm64-4c8.svg)](https://github.com/sj817/shotium/releases) [![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](../../LICENSE)

展示如何通过 JNA 在 Java 中直接调用 shotium C ABI 进行网页渲染与截图；本工程为独立运行示例，未作为 Maven 构件发布

## 目录

[环境要求](#环境要求) · [获取文件](#获取文件) · [运行](#运行) · [Windows 路径](#windows-上的-unicode-路径) · [代码结构](#代码结构) · [Java 相关说明](#java-相关说明) · [延伸阅读](#延伸阅读) · [许可证](#许可证)

## 环境要求

- JDK 17 或更高版本与 Maven 3.9 或更高版本（依赖 JNA 5.17.0 与 Gson 2.13.1 版本已在 `pom.xml` 中锁定）
- 7-Zip（`7z`），用于解压预编译原生引擎包
- 匹配当前 JVM 运行时架构的引擎二进制包（可通过 `java -XshowSettings:properties -version` 查看 `os.arch`）

## 获取文件

本示例源码独立打包为 [`shotium-example-java.7z`](https://github.com/sj817/shotium/releases/latest/download/shotium-example-java.7z)，解压后结合对应平台的原生动态库 `shotium-c-abi-<平台>.7z` 即可运行：

```bash
# Linux / macOS（以 linux-amd64 为例，需要 7z 或 7zz）
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-java.7z
curl -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-linux-amd64.7z

7z x shotium-example-java.7z
7z x shotium-c-abi-linux-amd64.7z -oshotium-example-java/native
cd shotium-example-java
```

```powershell
# Windows PowerShell（以 windows-amd64 为例）
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-example-java.7z
curl.exe -fLO https://github.com/sj817/shotium/releases/latest/download/shotium-c-abi-windows-amd64.7z

7z x shotium-example-java.7z
7z x shotium-c-abi-windows-amd64.7z -oshotium-example-java\native
Set-Location shotium-example-java
```

> 完整性校验：可下载 [`SHA256SUMS`](https://github.com/sj817/shotium/releases/latest/download/SHA256SUMS) 并执行 `sha256sum --check --ignore-missing SHA256SUMS` 校验；原生动态库解压位于 `native/shotium-c-abi-<平台>/`，包含动态库、头文件与 `.pak` 资源包

## 运行

```bash
mvn package dependency:copy-dependencies
java -cp "target/classes:target/dependency/*" ShotiumDemo native/shotium-c-abi-linux-amd64 card.html card.png
```

Windows 环境下类路径分隔符为分号 `;`：

```powershell
mvn package dependency:copy-dependencies
java -cp "target/classes;target/dependency/*" ShotiumDemo native/shotium-c-abi-windows-amd64 card.html card.png
```

命令行参数依次为 `<动态库目录> <输入 HTML 路径> <输出 PNG 路径>`，其中动态库目录会同时传入引擎作为 `resourceDir`；程序会将 `card.html` 渲染为 720×380 的图像，把性能统计指标以 JSON 打印到标准输出，并生成 `card.png`，其渲染输出与 `shotium` CLI 命令行完全一致

如指定不存在的文件可测试错误处理流程：程序退出码为 1，向标准错误输出 `capture failed (2): ...`，输出中仍包含统计信息，但不会生成输出图片

在本地编译产物中测试时，动态库目录可指定为 `../../out/Shot`

## Windows 上的 Unicode 路径

部分 Windows JDK 启动程序在将参数传入 `main()` 前会按系统 ANSI 代码页进行转换，可能导致非 ANSI 字符集路径受损；如需处理复杂 Unicode 路径，可将参数组织在无 BOM 的 UTF-8 JSON 配置文件中并通过 `--config` 传入，配置文件名建议使用 ASCII 字符，内部包含的相对路径均基于当前工作目录解析

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

## 代码结构

`src/main/java/ShotiumDemo.java` 包含了完整的调用逻辑：

```java
public interface Api extends Library {
    int shot_abi_version();
    int shot_engine_create(String options, PointerByReference engine, PointerByReference error);
    int shot_engine_capture(Pointer engine, String request, PointerByReference image, PointerByReference stats, PointerByReference error);
    void shot_engine_destroy(Pointer engine);
    Pointer shot_buffer_data(Pointer buffer);
    SizeT shot_buffer_size(Pointer buffer);     // size_t，在所有平台上都是指针宽度
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
            System.out.println(text(stats.getValue())); // 失败时同样提供统计指标
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

## Java 相关说明

- `size_t` 类型不能直接映射为 Java `long`：在 64 位 Windows 上 C `long` 仍为 32 位；通过继承 `IntegerType` 并指定 `Native.SIZE_T_SIZE` 自定义 `SizeT` 类型，可确保在所有平台上与指针宽度匹配
- 加载库时通过 `Library.OPTION_STRING_ENCODING` 显式指定 UTF-8 编码，防止 JNA 回退到操作系统默认字符集产生乱码
- `bytes()` 辅助方法在调用 `shot_buffer_free` 之前通过 `Pointer.getByteArray` 完成数据拷贝；严禁将原生缓冲区封装为 JNA `Memory` 对象（否则 JNA 在 GC 时会尝试由 JVM 释放该指针导致崩溃）
- `Api` 动态库代理实例保存在静态引用中，在 JVM 进程退出前保持加载，避免动态库被提前卸载
- 通过 `finally` 块确保在捕获异常时也能完整释放原生缓冲区并调用 `shot_engine_destroy` 销毁引擎

## 延伸阅读

完整的接口规范、请求参数字段、性能统计指标、长图分片和缓存管理详见 [C ABI 文档](../c-abi/README.zh.md)；C 头文件 `shot_api.h` 是权威的接口定义

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)
