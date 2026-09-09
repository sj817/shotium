# Java C ABI demo

This is a source example, **not a published Shotium language package**. It loads the precompiled shared library directly in the host process. / 这是源码示例，不发布独立语言包；直接加载预编译 C ABI 动态库。

## Prepare / 准备

JDK 17+ and Maven 3.9+; JNA 5.17.0 and Gson 2.13.1 are pinned in pom.xml. / 首次构建由 Maven 下载固定版本的通用 FFI 和 JSON 依赖，不是 Shotium Java 包。

Follow the [shared download and ABI guide](../c-abi/README.md) to extract the matching Release into `apps/native/`. Keep the library and both `.pak` files from the same release. The language runtime and native library must use the same architecture. On macOS, substitute `shotium-macos-arm64` or `shotium-macos-amd64` for the Linux directory below.

先按通用文档下载并完整解压对应平台的 Release；动态库与资源包必须同版本，宿主进程与库架构一致。macOS 请替换目录名。无需 Node，也无需自行编译 Chromium。

## Run / 运行

Start in the repository root. / 从仓库根目录执行。

Linux / macOS:

```bash
mvn -f apps/java/pom.xml package dependency:copy-dependencies
java -cp "apps/java/target/classes:apps/java/target/dependency/*" ShotiumDemo apps/native/shotium-linux-amd64 apps/fixtures/hello.html java.png
```

Windows PowerShell:

```powershell
mvn -f apps/java/pom.xml package dependency:copy-dependencies
java -cp "apps/java/target/classes;apps/java/target/dependency/*" ShotiumDemo apps/native/shotium-windows-amd64 apps/fixtures/hello.html java.png
```

Arguments: `<library-dir> <input.html> <output.png>`. The first argument also becomes `resourceDir`. The example renders at 800×600, explicitly permits local input, prints capture stats and writes PNG bytes. For a locally built engine, substitute `out/Shot` (Go: `../../out/Shot` after changing directory).

参数分别为动态库目录、输入 HTML、输出 PNG。示例显式设置资源目录、本地文件权限和 800×600 视口，打印统计并保存 PNG。使用本机构建时替换为 `out/Shot`（Go 切换目录后为 `../../out/Shot`）。

### Unicode paths / Unicode 路径

Some Windows JDK launchers replace command-line characters outside the system code page before Java receives them. For Unicode paths, save this as UTF-8 `java-config.json` (without a BOM) and pass `--config java-config.json`. Keep the config filename itself in ASCII. Paths in the JSON resolve against the working directory.

部分 Windows JDK 启动器会提前替换系统代码页以外的命令行字符。包含中文等 Unicode 路径时，把以下配置保存为无 BOM 的 UTF-8 `java-config.json`，通过配置文件传入；配置文件自身路径使用 ASCII。JSON 中的相对路径以当前工作目录为基准。

```json
{
  "libraryDir": "apps/native/shotium-windows-amd64",
  "input": "apps/fixtures/中文页面.html",
  "output": "截图.png"
}
```

```powershell
java -cp "apps/java/target/classes;apps/java/target/dependency/*" ShotiumDemo --config java-config.json
```

On Linux/macOS, use `:` instead of `;` in the classpath and the matching library directory. / Linux/macOS 使用对应动态库目录，并把 classpath 分隔符换成 `:`。

## Failures and ownership / 错误与所有权

Replace the input with a nonexistent file: the program must exit nonzero, print `capture failed (2)` and any available failure statistics, and write no image. ABI mismatches are rejected before engine creation. JNA loads the absolute library path with UTF-8 string encoding. SizeT uses Native.SIZE_T_SIZE, including on Windows. Every result is freed in finally; the proxy is kept alive until JVM shutdown. / size_t 按指针宽度映射，不能用 Windows 的 C long 替代。

输入不存在的文件可验证失败路径：非零退出码、错误原因和可用失败统计，不产生图片。每个进程只能创建一次引擎；重复截图复用它，销毁后不能重建。动态库保持加载直到进程退出。

The examples use only capture functions; the shared guide also documents file output, tiles, cache operations and memory release. The shipped `shot_api.h` is the authoritative API. / 文件输出、分片、缓存等见通用文档，完整接口以同包头文件为准。
