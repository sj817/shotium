# 静态截图裁剪：stage18–19 进度报告

本轮完成了测试入口清理和三库直接源码管理迁移。整个截图裁剪任务尚未完成，新源码尚未编译或运行验收。

## 本轮完成

| 项目 | 完成量 | 比例 | 验证边界 |
|---|---:|---:|---|
| ICU、Skia、Perfetto 转为主仓普通源码 | 3/3 | 100% | 保留文件原文哈希一致；完整原子仓已备份 |
| 根 patches 中的补丁移除 | 4/4 | 100% | 补丁文件及应用逻辑已移除，空目录删除被自动策略拦截 |
| 三库 DEPS/submodule 检出入口移除 | 3/3 | 100% | 主仓索引中不再是 gitlink |
| 本地构建和三平台 CI 补丁应用入口移除 | 4/4 | 100% | TypeScript 检查通过；未实际启动 CI |
| 旧 fuzzer 入口清理 | 105 处调用/声明 | 本次列定范围完成 | 按提交 diff 统计，含模板调用和 BoringSSL 循环；通用支持链尚未完成 |
| 最新源码六平台完整验收 | 0/6 | 0% | 本轮按要求未编译，不以历史图或旧二进制代替 |

百分比仅对应表中明确分母，不能相加作为整个任务的完成率。原始根目录 A/B/C 清单还有下述未完成范围，尚不具备准确的全任务总百分比。

源码管理迁移保留 ICU 996、Skia 4147、Perfetto 9153 个原文件，另外增加三份来源说明和一份生成数据忽略规则。保留全部已有局部修改、312 个执行位、8 个符号链接及其目标。约 154 MiB 是迁入主仓的源码/数据，不是新功能，也不是二进制体积变化。

三个原始完整 Git 子仓（包括 .git、既有修改和未纳入的文件）保存在 `out/cut-stage19-direct-source/vendor-backup/`。不要清理这个备份。Skia/Perfetto 此次保留的实现范围仍较宽，后续继续按截图实际用途裁剪；不能把“迁入主仓”当成“库内裁剪已完成”。

## 提交与检查

- `d21776f19892`：测试/fuzzing 入口，35 个文件，27 行增加、2672 行删除；包括 MojoLPM、fuzzable proto、自测入口和 Breakpad 配套分支。
- 三库直接源码维护使用本报告所在的独立提交；未 push、未创建 PR。
- 已通过：受影响 GN 文件语法解析、脚本 `pnpm scripts:check`、DEPS Python 语法解析；逐文件迁移哈希和文件模式检查。
- 未执行：当前版本 GN 生成/缺失输入门、C++ 编译、EXE/DLL、Node addon、运行检查、像素对照、Linux/macOS/六平台实编译。
- 最后完整 Windows 验证基线仍是 `d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb`：183 张像素一致，serve/net/demos/Node/daemon/协议/Bilibili/accept 通过。这不证明最新源码通过。

## 原始任务剩余清单

| 范围 | 已完成的主要部分 | 仍需完成 |
|---|---|---|
| 浏览器/GPU/交互 | 已有批次移除 GPU/Viz/GL、浏览器嵌入主链、Canvas 绘图、IME/编辑和多项合成器功能 | UI/display/事件/AX 等实际用途收窄、少量拖放/平台辅助尾巴 |
| 网络与文件接口 | PAC/WPAD、旧 IPC、ServiceManager、NQE/NetLog 导出、FileReader、脚本 Object URL、内存 HTTP cache 后端等已处理 | NetworkContext/未连接协议、Blob/Worker/Worklet 等支持链；保留实际安全检查和产品缓存 |
| Blink 扩展 | Sanitizer/Skeleton、Origin Trial 等已处理，XSLT 和静态布局兼容边界有记录 | Route/CSS/URLPattern 92 路径提案被自动审批拒绝，尚未应用；残余 observer/probe 等 |
| 诊断/遥测 | UKM 和部分 profiler/指标入口已处理 | Perfetto tracing、Crashpad 接入、metrics/probe/AX、device_event_log；保留 FCP/CaptureStats/CHECK/实际日志 |
| 测试和第三方 | 已清理多项空骨架和 105 处 fuzzer 调用/声明，三库无需 patch | net/DNS 支持库和 proto、Mojo fuzzer group、base buildflags、Blink/skia 包装器、通用 FuzzTest/libFuzzer/sanitizer 分支及 vendor 测试源 |
| 最终审计/验收 | 历史批次证据和原始审计仍在 | 全根目录 A/B/C 回填、同步冷检出、缺失输入、统一编译与运行/像素/六平台验收 |

当前两个自动审批限制：Route 92 路径补丁尚未获具体确认；空目录清理曾被策略拦截，包括 patches 空目录。没有绕过这些限制。其余授权范围可以继续推进。

接续入口：`docs/screenshot-cut-task.md`；本轮文件清单和证据：`out/cut-stage18-fuzzing/`、`out/cut-stage19-direct-source/`。
