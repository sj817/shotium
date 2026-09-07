# 截图裁剪交接：2026-09-08

**按用户要求，本批源码收尾后暂停，等待用户接手。不得因旧目标仍显示 active 而自动继续裁剪或启动构建。**

当前七组源码范围已落实（7/7）；本批统一生成、编译和运行验收尚未执行。整个根目录裁剪目标没有完成，不能把本批源码完成当成产品验收完成。此前 11 批的最后验证提交为 `d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb`。

## 当前留下的工作

从上述提交起，本轮累计删除 **1,404 个主仓库普通跟踪文件、5 个第三方 gitlink**，并新增 3 个显式 Mojo traits 头文件；源码及交接文档合并为一个本地提交，不创建 PR、不 push、不发布。最终提交号和精确路径统计写在 `out/cut-stage16-final/commit.json`，也可用 `git log -1` 查看。

前半轮已完成 GPU/Viz 运行链、CC PaintOp 序列化和 transfer cache、Skia GPU 编译入口、原生 UI/Ozone/平台窗口/WM、Skottie/Lottie、scenario、旧 IPC、八个第三方空骨架、XSLT 脚本 API 和输入预测/one_euro_filter 清理。随后完成这次明确授权的七组：

| 组 | 本组文件动作 | 落盘结果 |
|---|---:|---|
| Service Manager | 9 修改 / 53 删除 | 旧服务管理实现、协议、GN 引用与友元清理。 |
| ANGLE / SPIR-V / Vulkan | 18 修改 / 12 普通文件删除 / 5 gitlink 删除 | 断开运行/构建/同步入口及失效脚本配置；完整 vendor checkout 移入备份。 |
| Blink 拖放 | 12 修改 / 3 删除 | DragController/DragState、输入与 Page/Client 调用链清理，补掉 LocalFrame 的旧 include。 |
| UKM | 23 修改 / 6 删除 | HTMLParserMetrics、帧聚合器和同步滚动诊断链删除，保留 parser yielding、FCP、lazy loading 与内部 observer 判定。 |
| NQE / FileNetLog | 41 修改 / 40 删除 | 网络质量估计器、专用回调与文件导出器删除，ECT 值类型和原空估计器超时回退、HTTP2 PING、TCP RTT/字节统计保留。 |
| Sanitizer / Skeleton | 58 修改 / 49 删除 | 删除不可达净化 API/状态/生成输入与默认关闭的 Skeleton 整条链，保留普通 HTML、template patchfor、声明式 Shadow DOM 与正常 DOM 插入。 |
| Origin Trial | 70 修改 / 27 删除 | 删除 token、上下文、导航字段、协议、生成器和试验样例；保留普通 feature 状态、依赖及有效 context override。 |

各组有共享文件，修改数不能相加当作唯一文件数。Origin Trial 的生成器保留 1,154 个普通 feature，XSLT 默认开启；两个内部测试旗标 TestFeatureDependent/TestFeatureImplied 的 context override 传播与旧实现不同，说明见 `out/cut-stage16-origin-generator/review.md`。

另完成 libei、libinput、libgudev、mutter、libdisplay-info 的 5 个配置修改及 14 个空目录清理；这些包此前就没有源码。普通 Xvfb/Xorg/Weston 路径保留，失效 Mutter 启动分支删除。

## 验证到哪里

- 已做：当前路径/补丁核对、精确源文件备份、已删除输入引用扫描、改动 C/C++ 预处理配对、差异空白检查；少量 Python/Jinja 只做语法解析。最终静态结果在 `out/cut-stage16/static-proof-combined.json`。
- **未做：本批 GN 生成、missing-inputs、C++ 编译/链接、Node addon 重建、运行检查、像素对照、六平台实际编译。** 可能仍有只能在生成/编译时发现的问题。
- `out/Shot` 中现有 EXE/DLL 是第十一批旧二进制，不能用于证明本批源码通过。旧版本完成 183 张像素对照；新增第 184 张 Unbounded 仅有旧二进制基线。
- 用户要求先整批删除、最后集中编译，暂停前没有再启动长构建。编译并发 20 已多次实际 OOM；下次采用成功过的 8，并保持项目默认配置不变。

## 回退与文件位置

主仓库代码可与 `d9b409db334c` 比较并逐文件取回。`out/cut-stage16-final/owned-paths.json` 是本轮明确拥有的路径；`commit.json` 保存最终本地提交号。不要把其他工作区修改混入本批。

5 个完整第三方仓库位于：

`D:/Github/chromium/out/cut-stage16-angle-authorized/vendor-backup/third_party/`

其下为 `angle`、`spirv-headers/src`、`spirv-tools/src`、`vulkan-headers/src`、`vulkan-loader/src`。它们的 `.git`、未跟踪文件和已有修改都保留，移动前后 HEAD/git status 一致；ANGLE、SPIRV-Tools、Vulkan loader 都有原有局部修改。精确记录为 `vendor-move-proof.json` 和 `vendor-plan.json`。Skia、Perfetto 等其他共享 checkout 的原有修改没有纳入这次提交。

实际应用的清单分别在：

- `out/cut-stage16`、`cut-stage16-skottie`、`cut-stage16-ipc`、`cut-stage16-ipc-generator`、`cut-stage16-scaffolds`、`cut-stage16-xslt-api`、`cut-stage16-gfx-tail`、`cut-stage16-input-prediction`。
- `out/cut-stage16-applied-sanitizer`、`cut-stage16-applied-origin-generator`、`cut-stage16-approved-origin-cpp`、`cut-stage16-approved-ukm`、`cut-stage16-approved-nqe`、`cut-stage16-applied-drag`、`cut-stage16-applied-service-manager`。
- `out/cut-stage16-angle-authorized`、`cut-stage16-platform-input-scaffolds`、`cut-stage16-reports`。

早先被拒的 review/projection 保留作历史证据；七组已经获得用户明确授权并实际应用，以以上 applied/approved 清单为准。旧提案不应再次重放。

## 整体任务仍剩什么

| 范围 | 剩余内容 |
|---|---|
| 网络与公共协议 | NetworkContext/未连接服务、公共类型、FileReader/Blob/Worker 等尾巴；保留实际资源加载、TLS/证书、HTTP2、缓存、安全检查和产品协议。 |
| 诊断与基础库 | Perfetto tracing、Crashpad 接入、其余 metrics/probe/AX、device_event_log 等；CaptureStats、FCP 加载等待、CHECK 与实际错误日志仍需保留。 |
| UI/display/输入尾巴 | 桌面多屏管理、事件、无障碍和剩余拖放数据/图片辅助。ScreenInfo 是截图实际值类型；NativeThemeWin 滚动条、DPI/字体辅助有真实调用，需继续拆最小部分。已写 `out/cut-stage16/ui-display-next-boundary.md`。 |
| Blink 声明式边界 | Route/Navigation/URLPattern 及网络 connection allowlist/SafeUrlPattern 闭包未清完。保留 XSLT 原生转换和 view-transition-name 的静态 3D 分组/backdrop 效果。 |
| 第三方与工具 | Skia 实际 checkout 尚未完整收窄；JSON/fuzztest、其他未用包、资源/生成器/系统包清单及同步 hook 仍需最终复核。不能把主 GN 不再引用当成第三方磁盘已清完。 |
| 最终验收 | 根目录 A/B/C 全部回填最小保留范围；统一构建、运行、像素和六平台验证。 |

逐根目录状态见 [根目录报告](screenshot-root-status-2026-09-08.md)，最初候选见 [完整审计](screenshot-unused-code-audit-2026-09-07.md)，历史已验证批次见 [执行记录](screenshot-cut-execution-2026-09-07.md)。

## 用户决定继续验证时

在仓库根目录使用项目原入口；先生成并收集缺失输入，再集中修复，不通过恢复 V8/content/GPU 进程解决错误：

```powershell
pnpm build:engine --gen-only
pnpm missing-inputs out/Shot
pnpm build:engine --jobs 8 --log out/Shot/cut-handoff-build.log
pnpm build:engine --target shot_c --jobs 8 --log out/Shot/cut-handoff-dll.log
```

编译通过后再按 `.claude/skills/verify-engine/SKILL.md` 完成 serve/net、84 demos、Node/daemon/协议、Bilibili 与冻结的 184 张基线对照。当前没有自动执行这些命令；交接后父任务与子代理均停止，等待用户明确继续。
