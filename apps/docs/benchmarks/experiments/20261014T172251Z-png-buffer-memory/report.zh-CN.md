# PNG 压缩缓冲内存优化（2026-10-15，Windows x64）

本记录只比较同一台 Windows x64 主机上的改动前后版本。用户提供的外部报告没有对应页面或脚本，因此没有复现或直接比较其中约 93–97 MiB 的 RSS。

改动位于 `shot/shot_image_stream.cc` 的 `PngBlockEncoder::EncodeBlock()`。旧实现按条带原始像素字节的一半预分配并清零压缩输出数组；压缩结果即使只有几 KB，最后的 `resize()` 也不会缩小数组容量。多个正在编码或等待写出的条带会同时保留这些空间。

候选将初始容量限制为 64 KiB；写满后直接增长到原先按像素计算的容量，再继续按需翻倍。没有改变 PNG 过滤器、压缩级别、条带大小、线程数或输出像素。

最初试验只把初始容量缩小、仍然逐级翻倍。难压缩噪声图的平均耗时增加约 1.9%，因此没有采用该版本。加入首次扩容跳转后，136 对交替样本中噪声图平均耗时 36.894 → 36.874 ms，p50 36.376 → 36.683 ms；p50、均值和 p95 均处于预先设定的等效区间内。两版结果都保留，不能只读取复测结果。

基线分两种：内存探针和独立 CLI A/B 使用本次源码构建、修改前保存的可执行文件；Node 性能矩阵使用独立安装的 npm `@pixel.js/shotium@0.12.2`。本地基线保留了会话开始时已有的 `.gitignore` 和 `third_party/blink/renderer/core/page/page.cc` 改动。候选 CLI、Node addon 和 C ABI 库均通过 `pnpm build:engine` 顺序构建，构建日志、源码差异及文件哈希随记录保存。

性能与内存分开运行。性能测量关闭 `SHOT_PROFILE`，采用串行 AB/BA 配对；内存探针开启现有阶段日志，读取进程工作集、私有提交及操作系统维护的峰值计数。内存探针的耗时不参与性能结论。所有测量期间均没有并行构建或其他渲染检查。

内存表为每侧五个独立进程的峰值中位数，单位 MiB。普通页面每个进程连续渲染 35 次；8 倍卡片渲染 6 次。工作集含已驻留的共享映射等，私有提交单独统计；这里不含外部 Node 测试驱动的内存，也不是图片中未知测试框架的 RSS 口径。

| 页面 | 工作集峰值：基线 → 候选 | 私有提交峰值：基线 → 候选 | 私有提交变化 |
|---|---:|---:|---:|
| 仓库简单页，1280×720 | 51.1 → 50.3 | 29.6 → 28.8 | −2.7% |
| 仓库 Grid 页，1280×720 | 52.2 → 51.5 | 31.2 → 29.9 | −4.2% |
| demo-card，800×450 | 99.5 → 99.3 | 77.7 → 77.2 | −0.6% |
| 确定性 RGB 噪声图，1280×720 | 46.6 → 46.5 | 27.0 → 27.1 | +0.1 MiB，重复测量范围重叠 |
| demo-card，8 倍输出 6400×3600 | 394.2 → 394.3 | 515.7 → 515.7 | 基本不变 |

这是普通 PNG 截图的小幅缓冲内存收益，复杂大图的总峰值没有改善。每个进程的实际值、范围、阶段计数与图片哈希保存在 [memory.json](memory.json) 和 [memory-evidence.json.gz](memory-evidence.json.gz)。内存表中的 demo-card 与下方 Node 矩阵生成的简化卡片是不同夹具，不能把两表混成同一页面的内存/延迟测量。

Node 矩阵使用 npm 0.12.2 对比当前候选，共 30 个 PNG 相关场景。六项 A/A 校准得到 p50/均值噪声带 +2.5755%，p95 噪声带 +14.1780%；判定使用 99% 配对 bootstrap 区间。首轮为 **11 项 faster、16 项 equivalent、3 项 unproven，0 项 slower**。以下是其中的独立场景，不对不同页面取平均：

| 场景 | 基线 p50 ms | 候选 p50 ms | 候选/基线 p50 的 99% 区间 | 配对数 | 场景判定 |
|---|---:|---:|---:|---:|---|
| standard-simple-png | 3.851 | 3.784 | 0.964–1.002 | 794 | equivalent |
| standard-grid-png | 3.941 | 3.856 | 0.964–0.998 | 763 | faster |
| corpus-scale2-png | 14.972 | 14.348 | 0.948–0.969 | 208 | faster |
| standard-long-page-png | 11.203 | 10.904 | 0.959–0.986 | 272 | faster |
| scale-8-png | 83.650 | 82.956 | 0.982–1.005 | 96 | equivalent |
| startup-png：首张截图部分 | 4.953 | 4.908 | 0.983–1.000 | 605 | equivalent |
| daemon-png-buffer | 2.943 | 2.872 | 0.955–0.997 | 1000 | faster |

`startup-png` 的场景判定还包含初始化。其引擎初始化 p50 为 20.231 → 20.071 ms，导入加初始化为 28.152 → 28.479 ms；Node 进程自身启动 p50 为 223.173 → 223.744 ms，单独记录、不计入引擎优化判定。表中的首张截图耗时不等于用户看到的端到端冷启动时间。

对三项未定结果追加独立测量，完整保留首轮。复测命令没有重新校准，其原始 JSON 使用更严格的 2% 默认带，三项原始判定仍为 `unproven`。另用**首轮已经测定的同一组 A/A 噪声带**重算，结果为：

| 复测场景 | 配对数 | p50 ms：基线 → 候选 | 均值 ms：基线 → 候选 | 沿用首轮校准后的判定 |
|---|---:|---:|---:|---|
| scale-0.5-png | 3000 | 1.526 → 1.534 | 1.639 → 1.636 | equivalent |
| many-resources-default-png | 400 | 34.467 → 34.534 | 42.566 → 42.588 | unproven |
| many-resources-reload-png | 400 | 41.530 → 41.088 | 48.978 → 48.224 | unproven |

两个 HTTP 多资源场景仍不能给出有足够精度的性能结论。最终证据支持 28 项离线、启动和 daemon 场景等效或更快；**没有通过完整性能门槛，也不声称所有输入或其他平台性能不回退**。仓库默认门槛还要求每个引擎场景都更快，因此本次两个过滤运行均以 `improvement acceptance: not-passed`、退出码 1 结束；没有修改门槛，也没有把它改成通过。

验证全部针对最终构建的候选：

| 检查 | 结果 |
|---|---|
| Windows CLI / Node addon / C ABI 库 | 三个目标构建成功，文件哈希见 provenance |
| 矩阵图片比较 | 30/30，通过且解码像素完全相同；所有测量样本哈希稳定 |
| 复测图片比较 | 3/3 通过 |
| 难压缩噪声图 | 候选与基线解码像素完全相同；136 对性能样本等效 |
| verify:serve | 100 项检查通过 |
| verify:net | 18 项检查通过，包括本地 HTTP、缓存和真实 HTTPS |
| verify:demos | 69 exact、2 fuzzy、21 smoke，共 92 页通过 |
| verify:charset / verify:fonts | 15 个编码、3 个系统字体回退场景通过 |
| verify:node / verify:node-entry | 66 项 Node 检查及 7 组入口/生命周期检查通过 |
| verify:daemon / verify:daemon-protocol | 24 项 daemon 检查及协议检查通过 |
| verify:bilibili | 135 项检查通过，覆盖两篇整页、tiles、图片和二维码 |
| verify:ffi --languages python | 与 CLI 一致的图片、Unicode 输入、错误及失败统计通过 |
| 本地静态像素基线套件 | 无基线 manifest，未运行；本次使用旧二进制配对图片及 reftest |
| 其他语言交付、全平台 CI、发布 | 本轮没有执行 |

执行的主要命令（仓库根目录）：

```powershell
pnpm build:engine --log out/Shot/memory-baseline-cli.log
# 保存基线 CLI 与两个资源包后修改 PNG 缓冲代码。
pnpm build:engine --log out/Shot/memory-png-growth-cli.log
pnpm build:engine --target shot_node --log out/Shot/memory-png-growth-node.log
pnpm build:engine --target shot_c --log out/Shot/memory-png-growth-c.log
npm install --prefix out/memory-20261015/npm-baseline --no-audit --no-fund '@pixel.js/shotium@0.12.2'
pnpm perf:compare out/memory-20261015/npm-baseline/node_modules/@pixel.js/shotium apps/typescript out/memory-20261015/png-matrix.json --calibrate --check --filter 'png|features-alpha|bili-.*-viewport'
pnpm perf:compare out/memory-20261015/npm-baseline/node_modules/@pixel.js/shotium apps/typescript out/memory-20261015/png-recheck.json --check --samples 400 --max-samples 3000 --min-seconds 5 --max-seconds 16 --filter '^scale-0.5-png$|^many-resources-(default|reload)-png$'
pnpm perf:images out/memory-20261015/png-matrix.json
pnpm perf:images out/memory-20261015/png-recheck.json
```

[provenance.json](provenance.json) 记录主机、源码、成功构建、实际加载的 npm/native 文件和归档哈希；[summary.json](summary.json) 保留逐项区间；两个 `png-*.json.gz` 保留全部原始样本，包括失败的接受判定。[recheck-with-original-calibration.json](recheck-with-original-calibration.json) 单独保存沿用原校准的复测分析，不覆盖原始结果。

`block-paired.json.gz` 是被替换的逐级扩容试验，`growth-noise.json.gz` 是最终扩容策略的噪声图复测。`harness-sources.json.gz` 保存仓库测量脚本快照及本次 `out/memory-20261015/` 下的临时探针源码；`build-and-check-logs.json.gz` 和 [verification.json](verification.json) 保存构建、功能检查命令和结果。图片和本地基线二进制仍在 `out/memory-20261015/` 中；它们未作为大型二进制加入 Git。
