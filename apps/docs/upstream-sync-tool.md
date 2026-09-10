# 批量同步工具

命令：`pnpm upstream:sync`。这是选择范围后的 Git 三方合并器，不是把 Chromium main 合进本仓，也不会自动前移上游基线。

## 生成候选

在仓库根目录执行：

```powershell
pnpm upstream:sync plan --base c0bba1026178fe2a8b441fead7928b697a801c1e --target c099bd180a2db0fa6a313d43653529ba02665c84 --scope third_party/blink/renderer/core/css --out out/upstream-sync-css-next
```

`--scope` 可用逗号分隔多个目录或文件；目录不要加末尾斜杠。`--out` 必须是尚不存在的目录。所有相对路径都从仓库根目录解析。

工具比较固定的 OLD/HEAD/NEW 三棵提交树；工作区未提交内容不参与候选合并，应用时会拒绝覆盖脏文件。先读取对象 ID，再按需批量获取缺失 blob，最后用 Git merge-file 合并，避免每个文件请求一次网络。

输出：

- `files/`：上游直接更新或干净三方合并的候选源码。
- `conflicts/`、`conflicts.json`：带 OLD/本地/上游上下文的冲突及路径。
- `review.json`：新增文件、上游删除、二进制冲突、文件模式变化等人工决策项。
- `ready.json`：可由 apply 写入的现存文件清单。
- `manifest.json`：三个提交 SHA、路径来源对象 ID、结果摘要哈希与分类。
- `summary.md`：数量摘要。

本地已删除、上游仍保留的文件始终 `keep-deleted`。上游新增路径不自动进入源码，上游删除也不自动删除本地文件。没有变化的本地独有文件保留。

## 选定一批写入源码

从 ready.json 复制需要一起处理的路径到一个 JSON 数组文件，比如 `out/selected-css.json`。选择应包含相互依赖的实现、头文件、调用者和开关，不能把“文本合并成功”当成语义无关。

```powershell
pnpm upstream:sync apply --plan out/upstream-sync-css-next --selection out/selected-css.json
```

应用前检查整个选定批次：HEAD 未变、路径仍是该提交中的文件、没有暂存或未暂存修改、没有符号链接路径、候选文件哈希未变。所有原始文件先备份到计划目录下 `backup-<timestamp>/`，再开始写入。不会自动 stage、commit、修改 DEPS 或构建。

该写入不是文件系统事务。如果写入时出现 IO 错误或并发编辑，命令报错并保留备份；需检查已写入范围后按备份恢复，不能在共享工作区无条件 reset。成功时备份目录有 receipt.json 记录写入路径与目标 SHA。

冲突文件先在独立候选中解决，再通过 `resolve` 登记原因和内容哈希，然后用选定路径应用。决策 JSON 格式为 `[{"path":"仓库相对路径","source":"已解决文件的绝对路径","reason":"保留哪些产品差异，接收哪些上游改动"}]`：

```powershell
pnpm upstream:sync resolve --plan out/upstream-sync-155 --decisions out/upstream-sync-155/decisions.json
pnpm upstream:sync apply --plan out/upstream-sync-155 --selection out/upstream-sync-155/selection.json
```

`resolve` 会拒绝残留冲突标记，决策写入 manifest 的 resolution 字段。原 conflicts/ 文件仍保留作证据。ready.json/summary.md 是初始计划快照；后续状态以 manifest.json、conflicts.json 和各 backup 目录下的 receipt.json 为准。不要把所有候选写入后看到编译缺失就恢复整个旧浏览器模块。

已经应用的自动合并文件若需要配套修正，使用相同决策格式执行 `pnpm upstream:sync revise --plan <计划目录> --decisions <决策文件>`。它只允许覆盖与 manifest 哈希完全一致的已应用文件，拒绝其他并发修改；先完整备份，再更新源码、候选和哈希，并在 `revision-<timestamp>/receipt.json` 记录原因及前后哈希。与 apply 一样，写入中断时应检查备份和实际写入范围，不能无条件重跑或 reset。

经依赖审查确认必需的新增文件，使用 `pnpm upstream:sync adopt --plan <计划目录> --decisions <决策文件>`；决策格式为 `[{"path":"仓库相对路径","reason":"保留功能为什么需要它"}]`。只接收计划中 `new-file-review` 的普通文件，从固定上游对象读取，拒绝已有文件和符号链接父目录。新增内容及原因写入 manifest，`adoption-<timestamp>/` 保存意图和成功收据。新目录需要先用明确的 scope 生成独立计划；gitlink 和文件模式变化仍需单独审查。

确认上游删除项的调用已经迁移后，以同样的 path/reason 格式执行 `pnpm upstream:sync retire --plan <计划目录> --decisions <决策文件>`。它只删除 `upstream-delete-review` 中未被本地修改的普通文件；先备份整批，再逐文件删除，记录前置意图和成功收据。它不递归删除目录，也不把“上游删除”当成“本地不用”的证明。

如果编译发现保留功能新近依赖了以前裁掉的原生文件，先确认调用与依赖闭包，再以明确文件 scope 生成计划（不要加 `--retained-only`），用 `restore --plan <计划目录> --decisions <决策文件>` 从固定 target 恢复 `keep-deleted` 项。决策 reason 必须说明真实依赖。普通 adopt 仍拒绝此类文件，plan/apply 从不自动恢复本地删除。不能据此恢复 V8、浏览器进程或其他产品禁用模块，也不能调用硬编码旧基线的恢复脚本混入旧接口。

同一计划的 CLI 写入命令由 `.operation.lock` 互斥保护。必须等上一命令结束再启动下一命令；命令中断后先核对进程、意图、收据和源码，不能盲目移除锁。所有计划应用结束之前不要提交，HEAD 变化会触发保护。

## 直接维护的第三方源码

Skia、ICU、Perfetto 使用各自上游仓库的基线与目标，不能用 Chromium 的 gitlink 当源码内容合并。先获取固定提交，再以 `--upstream-prefix` 将独立仓库根目录映射到本地路径：

```powershell
git fetch --depth=1 --filter=blob:none --no-tags --no-write-fetch-head --recurse-submodules=no https://skia.googlesource.com/skia.git 653397c6be15b87fe8f89a4492582fbb825f6da8 a3e5b88809bb6c178286d008805a1028ca3106f4
pnpm upstream:sync plan --base 653397c6be15b87fe8f89a4492582fbb825f6da8 --target a3e5b88809bb6c178286d008805a1028ca3106f4 --scope third_party/skia --upstream-prefix third_party/skia --out out/upstream-sync-skia-next --remote https://skia.googlesource.com/skia.git --retained-only
```

后续仍使用 resolve/apply/revise/adopt/retire。adopt 的 `--remote` 也应指定对应第三方仓库。保留本地裁剪和直接源码修改，不生成需要日后重放的补丁队列。基线、目标、决策原因与最终验收状态要一起更新到同步记录中。

全范围同步可加 `--retained-only`，避免为上游几十万个已裁剪文件逐个生成行；仍列出保留目录中的新增同级文件。完全新增目录不会自动接收，要从上游 GN/include/生成器依赖继续审查。本选项不会证明“所有需要的新文件已找到”。

## 首次实际运行

2026-09-09，以本仓 `4035292e387b` 为本地 HEAD，对上述固定基线和目标，CSS 范围共 1,167 个路径：

| 分类 | 数量 |
| --- | ---: |
| 本地与新上游相同 | 702 |
| 直接接收上游候选 | 89 |
| 三方合并无文本冲突 | 45 |
| 上游没变，保留本地 | 74 |
| 文本冲突 | 20 |
| 保持本地删除 | 229 |
| 上游新增，待审查 | 6 |
| 上游删除，待审查 | 2 |

154 个实际更新候选中 134 个无文本冲突（约 87%）。这是源码机械合并率，不是 87% 已完成同步或验证。完整结果在 `out/upstream-sync-css-20260909-ready/`；源码没有被应用。对象已缓存后的整次 CLI 实跑约 5 秒，首次网络获取时间另计。

工具已用临时真实 Git 仓库验证：互不重叠修改、真正冲突、本地删除、上游新增/删除、二进制冲突、脏文件全批预检、候选篡改拒绝、路径越界拒绝、HEAD 变化拒绝及成功写入。下一步处理冲突和依赖后，按现有 build-engine/verify-engine 流程集中编译及做像素对照，不因本工具输出 ready 就宣称可发布。
