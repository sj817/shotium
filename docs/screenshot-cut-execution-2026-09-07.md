# 截图无用代码清理执行记录（2026-09-07）

本轮按 `screenshot-unused-code-audit-2026-09-07.md` 的根目录清单实施。审计报告是删除前快照；本文件记录实际删除、提交和验证状态。删除授权覆盖耦合残留，采用少量大批次提交，不创建 PR。

## 第一批：SQL、脚本编译缓存和已失去消费者的构建依赖

已实际删除 `sql/`、`components/persistent_cache/`、`components/sqlite_vfs/`、`components/sqlite_proto/`、`third_party/sqlite/`、`third_party/pffft/`。SQLite 的 DEPS 条目和 gitlink、`.gitmodules` 条目一并删除。

同时删除 Blink 的 CodeCacheHost、后台代理、持久缓存代理、脚本缓存元数据处理器、代码缓存 fetcher、WorkerMainScriptLoader 及其 Mojo 协议；清除 DocumentLoader、FrameLoader、ResourceFetcher、ResourceRequestSender、URLLoader 和 ShotURLLoader 的相应接口、成员及调用。普通资源加载、HTTP Simple 缓存和加载冻结/重定向仍保留。

清除 SQL 磁盘缓存开关、工厂分支、实验参数及构建目标。Perfetto 的离线 SQL trace processor 使用它自身的独立开关关闭，并增加可重复应用到 DEPS checkout 的补丁，使 GN 不再求值该目标；本地构建入口与三个平台的 CI 源码准备步骤均应用此补丁。运行时 tracing 并未在这一批删除。

删除 `ui/gl` 的占位 EGL/GLES DLL 目标和 dummy 源码，移除 Vulkan loader 的 data dependency。**Vulkan loader checkout、GPU/GL 主体尚不属于这一批的已删除成果**，继续随后续消费者清理。

### 验证

| 项目 | 实测结果 |
|---|---|
| Windows GN | 8,392 个目标 / 991 个输入构建文件；删除前为 9,164 / 1,214 |
| 缺失输入 | `shot` + `shot_c` 的 7,472 个源码树输入全部存在 |
| Windows 编译 | EXE、DLL 均成功；0 FAILED、0 编译诊断 |
| serve / net | 全部通过，含实际 HTTPS、跨 worker Simple 缓存、HTTP 与 file 图片相同 |
| demos | 62 精确匹配、1 在原有容差内、21 smoke；84 项全通过 |
| 删除前后图片 | 169 张 demos 输出、4 张 render cases、1 张验收 corpus 全部逐字节相同 |
| Node / daemon / daemon protocol | 用本轮重建的 addon 和 DLL 验证，全部通过 |
| Bilibili 离线长文 | 两篇长文、分片与全图、照片和底部二维码全部通过 |
| acceptance | 运行完成；相对既有 Chrome oracle 有 1.5245% 不同像素，但本轮 corpus 与删除前完全相同，差异非本次引入 |
| Linux probe | 构建图解析通过；0 缺少的 BUILD.gn、0 主仓库缺失文件。Windows 宿主工具后缀和未安装的 Linux toolchain/DEPS 不算 Linux 实编译 |
| macOS / 六平台实编译 | 尚待最终批次提交后验证；本机通过不代表六平台通过 |

验证二进制：EXE SHA256 `b7ba1bc0eaed9072765846321ffc3ee56057d315dce6b7616de391024c1713f5`；DLL SHA256 `bf66377ef21dd41500e9bbb1db2bcfac2bcd9f03b4e2f7b82920e3ed5fddbbbb`。

中间曾遇到临时源码复制保留旧时间戳，导致 Ninja 漏编加载器对象、运行时新旧接口混用。已刷新所有修改过的 C++ 文件和头文件时间戳、重新构建，并通过上述运行检查。后续临时区改动使用重新写入内容的方式落盘，不沿用临时文件时间戳。

第一批提交：`71ebd6e65771`。

## 第二批：硬件认证、脚本缓存传输和失效构建骨架

删除设备绑定会话的整个网络实现、协议、traits、URLRequest/HTTPJob/Blink 调用链，以及 `components/unexportable_keys`、TPM Rust 解析器、不可导出密钥、用户验证密钥和专用 Apple Keychain 包装。正常 HTTPS、系统证书、TLS 私钥适配和基本密码算法保留。WebFeature 中三个历史编号仅为枚举身份，不再对应实现。

继续清除脚本编译缓存的传输尾巴：URLLoaderClient 的缓存载荷参数、各级回调/转发、Resource 的缓存元数据方法和统计、JS 源码哈希开关均删除。字体响应处理仍处理字体内容本身。

实际删除根目录 `content/`、`google_apis/`、`extensions/`；删除 components 下 15 组只有构建骨架的浏览器设施，以及 ui 下 aura、compositor、menus、webui 等空壳、cert_verifier/test 服务空壳。对应 import、deps、构建开关和 WebUI 资源条件一起清理。

删除 Mojo JS/TS/Fuzzilli 生成器、目标、调用选项、预编译模板、测试目标的失效生成依赖，以及 `mojo/public/js/`、`tools/typescript/`。C++、Rust 和仍有调用的生成工具保留。

| 项目 | 第二批实测结果 |
|---|---|
| Windows GN | 6,988 个目标 / 874 个输入构建文件 |
| 缺失输入 | 7,466 个源码树输入全部存在 |
| Windows 编译 | EXE / DLL 均通过，0 FAILED、0 编译诊断 |
| serve / net / demos | 全通过；84 个 demos 全通过，含实际 HTTPS |
| 删除前后图片 | 169 张 demos、4 张 render cases、1 张 corpus 共 174 张全部逐字节相同 |
| Node / daemon / daemon protocol / Bilibili | 用本轮重建的 addon 和 DLL，全部通过 |
| Linux probe | 0 缺少的 BUILD.gn、0 主仓库缺失文件；宿主后缀及 3 个未安装的 Linux DEPS 仍只是本机探测限制 |
| 六平台实编译 | 待最终批次验证，不把本地测试提升为跨平台通过 |

第二批日志前缀：`out/Shot/cut-batch2-combined-`；图片证据：`out/cut-batch2-combined/pixel-comparison.json`。

第二批提交：`f48e852f6cb8`。

## 第三批：媒体、旧缓存、设备/打印与版本构建残留

已实际删除根目录 `device/`、`printing/`、`chrome/`、`media/`，以及 `cc/benchmarks/`、`net/disk_cache/blockfile/`、`third_party/closure_compiler/`。Chrome 版本读取改为现有 `shot/VERSION`，macOS plist 移至 `shot/app-Info.plist`。打印预设协议删除；页面缩放仍用 Blink 自己的同值枚举，不改变 CSS 打印布局。

删除微基准控制器的成员与调用、无消费者的 dropped-frame 共享内存和视频粗糙度统计，删除媒体播放/音频设备/解密/捕获接口及其导出包装、线程接口、无播放器的绘制分支。保留 video 的海报图片与 HTML 布局，保留普通 MIME 分类。Simple 和内存缓存仍在，旧 blockfile 工厂、枚举、测试等待入口及实现一并清理。Protobuf JS/TS 的 GN 与 Python 调用链同步删除。

删除实体文件前，先编译修改后的消费者，确认候选未被其他工作修改，再核对 EXE/DLL 的 Ninja 输入和 GN 重生成输入均不读取候选。第三批的两组实体删除清单分别为 35 和 432 个文件。

| 项目 | 第三批实测结果 |
|---|---|
| Windows GN | 6,945 个目标 / 867 个输入构建文件 |
| 删除后的缺失输入检查 | 7,464 个源码树输入全部存在 |
| Windows EXE / DLL | 均通过；最后一次构建 0 FAILED / 0 编译诊断 |
| serve / net / demos | 全通过；84 项 demos，含实际 HTTPS |
| 像素比较 | 169 张 demos + 4 张 render + 1 张 corpus，共 174 张逐字节相同 |
| Node / daemon / 协议 / Bilibili | 本轮 DLL 与重建 addon，全部通过 |
| acceptance | 与原有 Chrome oracle 的 1.5245% 差异未扩大；与本轮删除前 corpus 逐字节相同 |
| IDL 枚举检查 | 155 个引用值，0 个未生成 |
| Linux probe | 0 缺 BUILD.gn / 0 主仓库缺失输入；本机宿主及未安装 Linux DEPS 限制同前 |
| 六平台实编译 | 尚未完成，不把本机结果当跨平台证明 |

第一次高并发构建出现 LLVM 内存耗尽，降低并发后通过；随后媒体拆除暴露的残留调用与间接 include 已修复并重编译。没有通过重新引入 media 或 blockfile 消除错误。

日志前缀：`out/Shot/cut-media-`；删除证明与像素证据：`out/cut-stage4/deletion-proof.json`、`out/cut-stage5/deletion-proof.json`、`out/cut-stage5/pixel-comparison.json`。

## 后续批次

下一批处理 Canvas、输入/合成器/GPU 等混合目标的剩余依赖。此记录不把“待处理”或“已关闭开关”标为“已彻底删除”。
