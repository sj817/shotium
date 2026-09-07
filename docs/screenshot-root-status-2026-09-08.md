# 静态截图裁剪：逐根目录状态（2026-09-08）

> 下文是 stage16 的历史根目录快照，含旧暂停指令和旧计数。用户已恢复工作；最新三库源码迁移、补丁移除、提交和未完成清单见 [stage18–19 进度报告](screenshot-cut-stage19-report.md)。当前不再以本表计数或暂停状态作为实时结论。

**本批源码已经收尾，按用户要求暂停。** 相对最后验证提交 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb，本轮累计删除 1,404 个普通跟踪文件、5 个 gitlink，新增 3 个 Mojo traits 头文件。七组明确授权的源码范围已完成 7/7；整体任务仍有未处理范围，本批也尚未编译或运行验收。详细接手清单见 [交接报告](screenshot-cut-handoff-2026-09-08.md)。

## 为什么目录曾反复留下

此前既有运行入口删了但实现、协议和 GN 引用没一起删的情况，也有实现已删却仍留同步配置、第三方骨架或磁盘空目录的情况。这轮分别清理调用链、同步入口及空目录。仍在引用也可能只是残留之间相互依赖，不能单凭有引用就永久保留。

ui 不是 CSS 引擎，也不等于产品界面。CSS、DOM、布局位于 third_party/blink；ui/gfx 的几何、颜色、字体类型及 ui/native_theme 的表单默认绘制实际影响截图，需保留最小实现。Ozone、平台窗口、WM、Lottie、GL、旧 UI IPC 和输入预测已删除，事件、无障碍、显示等尚需收窄。

crypto 仍承载网络和证书所需密码学设施；设备绑定认证和不可导出密钥链前批已删。gpu、sql、sandbox、google_apis、device、ipc 已从根目录移除。Skia 的 SkCanvas 是整页 CPU 绘制设施；Web Canvas 仅保留标签备用内容和既有静态布局。

## 当前所有根目录

数量来自 out/cut-stage16/current-root-inventory.json：提交前仍在磁盘的主仓库跟踪条目，gitlink 计一个，不递归统计第三方 checkout，不包括新建报告。0 不表示空目录。

| 根目录 | 跟踪条目 | 当前职责、已处理内容及剩余工作 |
|---|---:|---|
| `.agents` | 0 | 本地技能链接，开发辅助；不属于引擎功能。 |
| `.claude` | 5 | 项目构建、裁剪、验收技能，保留。 |
| `.git` | 0 | 仓库历史和索引，保留。 |
| `.github` | 12 | 六平台构建、检查、发布工作流；失效依赖仍需最终同步复核，本任务不发布。 |
| `apps` | 132 | 基准工具与展示站；是实际产品验证/展示项目，不按浏览器运行代码删除。 |
| `base` | 1,772 | 调度、线程、分配、文件和检查等仍使用；旧 IPC 友元和采样 profiler 已处理，Perfetto、诊断、平台空壳仍待收窄。 |
| `benchmark-results` | 200 | 已记录的测量数据，保留证据。 |
| `build` | 747 | 当前编译工具链与跨平台配置；代理、GPU、旧 IPC 和八个第三方骨架的失效入口已部分清理，其余模板/依赖待最终图复核。 |
| `buildtools` | 76 | GN 与构建工具；按实际宿主/六平台工具链保留。 |
| `build_overrides` | 11 | 第三方 GN 覆盖；ANGLE/SPIR-V/Vulkan 的配置入口已清理，剩余覆盖需最终跨平台图复核。 |
| `cc` | 143 | 合成器主机、调度、GPU raster、tiles、帧率指标已删；这轮再删 PaintOp 序列化和 transfer cache，保留 CPU paint、滤镜、图片与实际几何算法。 |
| `components` | 94 | 颜色、像素格式、内存、资源与网络辅助的最小实现仍需保留；scenario 已删，诊断、URLPattern 等剩余闭包待处理。 |
| `crypto` | 67 | 设备绑定认证链已删；保留实际网络/证书密码学调用，剩余方法仍需逐项核查。 |
| `docs` | 16 | 产品、构建、同步分歧及裁剪证据；新增本批交接和逐根目录状态报告。 |
| `mojo` | 597 | 普通类型化 Mojo 与 ipcz 仍为现存接口使用；旧 IPC/native 序列化桥已删，五个旧 Native 类型改为显式 enum/struct，生成器拒绝重新引入旧 Native。 |
| `net` | 1,244 | HTTP(S)、DNS、TLS、HTTP2、资源缓存与安全检查保留；PAC/WPAD、NQE/FileNetLog 已删，ECT 值类型和原空估计器超时回退保留；公共服务/协议尾巴尚未清完。 |
| `out` | 0 | 构建产物、原始基线、逐文件备份和完整 vendor checkout 备份，保留。这里的证据通常被 Git 忽略，接手时不要清空。 |
| `patches` | 4 | DEPS checkout 的必要修改，需在第三方最终收窄时同步复核。 |
| `scripts` | 74 | 构建、验收、生成与冷同步工具；旧工具已部分清理，prune-deps/trim-tree 等需最终防回流复核。 |
| `services` | 401 | 网络公共类型和必要辅助保留；Viz 仅余 12 个 CPU 颜色/像素格式文件，Service Manager、UKM 专用链已删，其余网络接口待处理。 |
| `shot` | 643 | 实际截图引擎、公共接口、资源和回归语料；保持加载等待、CaptureStats、CPU 绘制与输出。 |
| `shotium` | 21 | npm/Node/C ABI 接入及 daemon、缓存、分片截图协议，保留公开行为。 |
| `skia` | 90 | Chromium 侧 CPU 绘制与编码接入；GPU/Skottie 等编译入口已删，第三方实际 checkout 尚未完整收窄。 |
| `testing` | 19 | 公共测试支持保留；失效 Mutter 启动分支已删，普通 Xvfb/Xorg/Weston 保留；其他失效测试目标待最终复核。 |
| `tests` | 8 | 实际像素回归语料/配置，保留。 |
| `third_party` | 15,044 | Blink/Skia、字体、ICU、XML、TLS、图片与必要协议库保留；八个空骨架、输入预测/one_euro_filter、ANGLE/SPIR-V/Vulkan 已处理。Sanitizer/Skeleton、Origin Trial 已删；Route/URLPattern、Skia 实际 checkout、JSON/fuzztest 及其他余项待处理。 |
| `tools` | 300 | 生成/资源/测量工具；IPC fuzzer 和失效 DevTools PDL 更新工具已删，诊断与生成器余项待处理。 |
| `ui` | 1,708 | CPU 颜色/几何、字体和表单主题具有实际截图用途；364 个原生 UI 文件、旧 IPC、GL、输入预测已删。AX/事件/显示及剩余拖放数据/图片辅助尚需收窄。 |
| `url` | 57 | URL 解析、规范化、Origin 与加载安全检查保留；旧 url/ipc 已删，普通 Mojo 类型保留。 |

根目录控制文件 DEPS、.gitmodules、BUILD.gn 与相关同步配置也已随本批修改；最终冷同步和六平台依赖复核尚未执行。

## 当前验证状态

已授权的 Service Manager、ANGLE/SPIR-V/Vulkan、Blink 拖放、UKM、NQE/FileNetLog、Sanitizer/Skeleton、Origin Trial 七组均已应用，不再等待确认。此前审批记录仅为历史。前半轮 GPU/Viz、原生 UI、Skottie、旧 IPC、第三方骨架、XSLT 脚本 API 与输入预测也已落盘。五个 vendor 的 .git、未跟踪文件和既有修改完整保存于 out/cut-stage16-angle-authorized/vendor-backup，移动前后 HEAD/status 一致。

静态证据：15,905 个代码/构建输入、285 个改动 C/C++ 预处理配对、653 份新阶段 SHA 备份，当前 0 问题；详见 out/cut-stage16/static-proof-combined.json。静态检查不能证明编译通过。

**本批 GN 生成、缺失输入检查、C++ 编译/链接、addon 重建、运行和像素验收全部未执行。** 最后验证版本仍为第十一批，183 张像素通过；第 184 张 Unbounded 只有旧二进制基线，六平台实际编译未完成。

## 仍未完成，交由用户决定下一步

1. 网络公共协议与服务、FileReader/Blob/Worker 尾巴。
2. Perfetto/Crashpad、其余 metrics/probe/AX/device_event_log 等诊断闭包。
3. UI/display/事件及剩余拖放辅助；保留实际颜色、字体、表单与 ScreenInfo 值类型。
4. Route/Navigation/URLPattern、connection allowlist/SafeUrlPattern，及其生成器/协议。
5. Skia 实际 checkout、JSON/fuzztest 等余下第三方，以及资源、同步 hook 和系统依赖。
6. 根目录最小用途最终回填，集中构建、运行、像素与六平台实际验证。

已停止继续裁剪及子代理工作，不因旧 active 目标自动继续。历史已验证批次见 [执行记录](screenshot-cut-execution-2026-09-07.md)，初始候选见 [完整审计](screenshot-unused-code-audit-2026-09-07.md)。
