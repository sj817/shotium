# CPU 恢复毛玻璃和 CSS 3D：本地验证记录

2026-10-11，在现有 CPU 绘制链上恢复三项行为：空透明元素的
`backdrop-filter`、`preserve-3d` 的深度排序、`backface-visibility:hidden`。
Windows x64 CLI 的实际增量为 **42,496 B（41.5 KiB）**。普通页面样例的
内存差很小；大面积模糊会分配额外图像缓冲，开销取决于面积和滤镜。

## 实现与覆盖

空元素保留空绘制记录，滤镜输出范围参与裁剪，因此没有背景、内容和
命中测试记录的元素也能处理背后的像素。`pointer-events:none`、裁剪、
遮罩均有颜色对照；隐藏元素保持不可见。

3D 平面按 Blink 的 rendering context 分组，使用 CPU 几何排序；相交平面
切分后按远近回放，共面保持原绘制顺序。平面内部的 opacity、filter、mask
及嵌套上下文保留分组，排序矩阵保留深度，回放矩阵压平，避免内层深度
穿出倾斜的外层平面。背面判断使用累计矩阵，并处理 paint offset/scroll
节点的委托关系。

V8、GPU 进程、ANGLE、Ganesh 和完整 compositor 仍未接入；新增的是几何
计算和 CPU 回放。对应维护说明见 [upstream-sync.md](../../../upstream-sync.md)。

## 体积

配对构建使用相同的当前源码、构建配置及原有无关修改。基线只把本次修改的
三个 C++/GN 文件恢复到 HEAD 内容；候选恢复本次修改后重新构建。

| Windows x64 文件 | 恢复前 | 恢复后 | 增量 |
|---|---:|---:|---:|
| shotium.exe | 36,867,584 B | 36,910,080 B | 42,496 B / 41.5 KiB |
| 两个资源 PAK 合计 | 38,453 B | 38,453 B | 0 |
| CLI 加资源 PAK | 36,906,037 B | 36,948,533 B | 42,496 B |

恢复后 DLL 为 36,913,664 B，Node addon 为 36,925,952 B，两者也已重新构建。
这是未压缩的 Windows 运行时文件大小；不代表下载包大小或其他平台结果。

## 内存

构建和渲染检查结束后，用恢复前后 CLI 做 **5 轮交替 AB/BA**。每个页面
每次启动独立 `--serve` 进程，渲染 3 次，记录 OS 进程生命周期峰值工作集，
以及渲染结束后的私有字节数。表中为各侧 5 次的中位数，单位 MiB。

| 页面 | 峰值工作集：前 → 后 | 差值 | 渲染后私有内存：前 → 后 |
|---|---:|---:|---:|
| 卡片，800×450 | 38.64 → 38.68 | +0.04 | 13.92 → 13.85 |
| 综合样例，1280×720 | 45.34 → 45.42 | +0.08 | 21.34 → 21.32 |
| 小毛玻璃，400×200 页面 | 30.62 → 30.85 | +0.23 | 8.84 → 8.84 |
| 3D 深度样例，400×200 | 30.98 → 31.06 | +0.08 | 9.12 → 9.09 |
| 大毛玻璃，1600×900 页面 | 38.47 → 65.94 | **+27.47** | 14.96 → 25.96 |

大毛玻璃为一个 **1200×650 的空透明元素，`blur(20px)`**。恢复前该空元素
滤镜未执行，恢复后实际执行模糊。单个同面积 RGBA 缓冲约 3 MiB，滤镜可能
需要多个缓冲和扩展采样范围；这次测得的 27.47 MiB 不是固定启动成本。
更大面积、多个滤镜元素或更高输出比例需要单独测量。

普通页面的小差值处于本次观察的波动量级，不能据此声明零开销或性能提升。
私有内存是渲染后的快照，**不是峰值私有内存**；峰值工作集包含共享映射。
本记录没有运行完整性能矩阵，也不作耗时回归结论。

全部 50 个页面/侧样本保存在 [paired-memory.json](paired-memory.json)，
源码、配置、脚本和最终产物的 SHA-256 见 [provenance.json](provenance.json)。

## 验证结果

| 检查 | 结果 |
|---|---|
| Windows CLI / Node / DLL 构建 | 三个目标成功构建，0 FAILED edge(s) |
| verify:demos | **92 个通过：69 PASS、2 FUZZY、21 SMOKE** |
| 恢复前四张像素基线 | basic/layout/paint/text，均 0 像素变化，PNG SHA 相同 |
| 40,000 px 长页面 | 滤镜、3D 和背面隐藏跨绘制窗口，参考图与实际图完全相同 |
| verify:serve / verify:net | 通过，含确定性、长页窗口、资源访问、缓存和网络行为 |
| verify:charset / verify:fonts | 15 种编码通过；3 个字体回退用例通过 |
| verify:node / verify:node-entry | 通过；node-entry 7 PASS，含 tiles、并发和退出行为 |
| verify:daemon / verify:daemon-protocol | 通过；测试启动的 daemon 已结束 |
| verify:bilibili | 两篇离线整页、每个 tile、图片和两个页脚 QR 检查通过 |
| verify:ffi | Python 调用最终 DLL，PNG 与 CLI 相同，Unicode 路径及失败统计通过 |
| accept --skip-build | 通过；综合样例相对已有 Chrome oracle 的差异保持原有水平 |
| 其他平台和交付包 | 未运行；本次是 Windows 本地恢复，没有发布 |

旧 CLI 对新增毛玻璃和 3D 功能用例会失败，确认这些用例能捕捉原先缺失的
行为。早期编译诊断、Chrome 边缘差异的失败对照及调试证据保存在本地
`out/GpuResearch` / `out/Shot/css-effects-*.log`，没有用通过结果覆盖这些记录。

## 与 Chrome 的像素边界

对 7 对功能样例及参考页共 14 张图做独立 Chrome 对照：12 张完全相同。
深度样例的 opacity 区域有 4,800 个像素相差 1 个色阶；三个相交平面的
样例有 39 个像素不同，全部位于共享对角线的一像素边缘。检查明确限制了
允许差异的位置，其他区域必须完全相同，见
[relinked-chrome-effects-spatial-check.json](relinked-chrome-effects-spatial-check.json)。
此次对照使用最终 CLI；[chrome-comparison.json](chrome-comparison.json) 保留
其产物身份及默认阈值检查中三平面边缘的失败结果。四张原基线的最终 CLI
身份和像素结果见 [pixel-regression.json](pixel-regression.json)。

三平面 CPU reftest 与 SVG 参考图的抗锯齿斜边有 80 个像素不同，最大色阶
差 128，处于该样例预先声明的有限容差内。这里没有把整张图的任意差异
都算成通过，也不声明 CPU 与 GPU 在相交边界上逐像素一致。

综合样例的既有 Chrome oracle 差异为全部像素 1.5245%、色阶差大于 8 的
像素 1.1487%；旧版与恢复版四张基线的完全一致提供了本次回归检查。
