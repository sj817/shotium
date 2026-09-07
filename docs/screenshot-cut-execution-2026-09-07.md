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

## 后续批次

第三批正在处理 device、printing、Chrome 版本路径、cc 微基准及其他剩余依赖。输入/合成器/GPU/media 等混合目标仍在清理范围内。此记录不把“待处理”或“已关闭开关”标为“已彻底删除”。
