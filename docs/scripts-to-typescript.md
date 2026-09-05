# 脚本迁移到 TypeScript

> 2026-09-07。规则已定：仓库脚本一律 TypeScript，能用库就不自己写，精力留给 `shot/`。
> 本文是把现有 `.py` / `.ps1` / `.cjs` 迁过去的评估：每个脚本手写了什么、哪个库能直接顶替、
> 迁移顺序，以及工作区（pnpm workspace）实测出来的两条硬约束。
>
> **迁移已完成（2026-09-06，分支 `build/scripts-typescript`）。** 下文的评估保留作记录。
> 现状：`tools/shot/`、`bootstrap/`、`tests/render/lib/` 已删空，每个脚本的去处是同名
> kebab-case 的 `scripts/<name>.ts`（对照第 4 节），命令一律用根 `package.json` 的别名
> （`pnpm verify:serve`、`pnpm render run`、`pnpm gen:idl all`、`pnpm bootstrap` 等）。
> 每个脚本都拿旧版对同一输入的输出做过对照，结论写在各自的提交正文里。第 3 节里的
> `@clack/prompts` 与 `pidusage` 最终没有用上：bootstrap 没有交互，子进程峰值改用
> Toolhelp32 快照（`scripts/lib/measured-process.ts`）。`tools/shot/PERFORMANCE.md`
> 成了 `docs/performance.md`。

## 0. 结论

- 现存脚本 17,184 行（Python 11,301、PowerShell 3,461、CommonJS 2,061、ESM 337、Shell 24），
  另有 1,067 行散在 8 个 workflow 的 `run: |` 里。
- 其中约 7,700 行是裁剪期的一次性工具，**不迁**：要么已经完成使命该删，要么只在上游同步时
  才用，到时按需迁。
- 真正常驻的是验证套件、性能工具、`tests/render`、构建循环和 CI 辅助，合计约 6,000 行。
  它们手写的部分主要是四样：PNG 解码与像素比较、进程调用与重试、本地 HTTP 服务、
  `--serve` 协议帧。前三样各有一个库，第四样已经在 `shotium/src/lib/protocol.ts` 里，
  迁过去是复用而不是再写一遍。
- 工作区暂时只收 `tools/shot`。根目录放 `pnpm-workspace.yaml` 会让 `shotium/` 和
  `apps/benchmark` 里的 `pnpm install` 变成安装整个工作区，而 shotium 因为六个平台包的版本
  钉在尚未发布的号上，进不了任何带 `--frozen-lockfile` 的工作区。两条都实测过，见第 6 节。
- 本次已完成：`build.ps1` → `scripts/build-engine.ts`、`link_agent_skills.ps1` →
  `scripts/link-agent-skills.ts`，`scripts/` 成为独立 pnpm 项目，规则写进 `CLAUDE.md`。

## 1. 规则

写在 `CLAUDE.md` 第 11 条，这里只列要点：

- 脚本只有一个家：根目录 `scripts/`。新脚本是 `scripts/<kebab-case>.ts`，用 `tsx` 运行，
  根 `package.json` 转发常用命令（`pnpm build:engine`、`pnpm skills:link`、`pnpm scripts:check`）。
  `tools/shot/`、`tests/render/`、`bootstrap/` 是旧脚本的所在地，冻结：每迁一个就在原地删一个，
  最后三个目录清空。`checks.yml` 每次 push 对 `scripts/` 做类型检查。
- 不是本项目自己的逻辑就用库：进程 `execa`，通配 `tinyglobby`，参数 `cac`，
  重试 `p-retry`，像素 `sharp` / `pngjs` / `pixelmatch`，并发 `p-limit`，测试 `node:test`。
- 不再给现存 `.py` / `.ps1` / `.cjs` 加功能；要改就顺手迁。
- `tsconfig.json` 开了 `erasableSyntaxOnly`，所以这些文件在 Node 22.18+ 的原生剥类型模式下
  也能直接跑（`node tools/shot/x.ts`），`tsx` 只是省掉扩展名和参数上的麻烦。

## 2. 现状盘点

| 语言 | 文件 | 行数 | 位置 |
|---|---:|---:|---|
| Python | 62 | 11,301 | `tools/shot/*.py`、`bootstrap/lib/deps_lock.py` |
| PowerShell | 15 | 3,461 | `tools/shot/*.ps1`、`tests/render/*.ps1` + `lib/*.psm1`、`bootstrap/*.ps1` + `lib/*.psm1` |
| CommonJS | 9 | 2,061 | `tools/shot/*.cjs` |
| ESM JS | 3 | 337 | `tools/shot/docs_*.mjs` |
| Shell | 1 | 24 | `tools/shot/measure_memory.sh` |
| workflow 内联 | 8 个文件 | 1,067 | `.github/workflows/*.yml` 的 `run: \|` 块，bash 10 处、pwsh 15 处 |

CI 直接调用的脚本（次数为 workflow 里出现的次数）：`make_platform_package.cjs` 7、
`ci_stamp_mtimes.py` 6、`bilibili_check.py` 5、`node_check.cjs` 4、`daemon_protocol_check.cjs` 4、
`serve_check.py` 3、`restore_from_upstream.py` 3、`node_perf_ci.cjs` 3、`net_check.py` 3、
`icu_repack.py` 3、`demo_check.py` 3、`daemon_check.cjs` 3、`charset_check.py` 3、
`node_perf.cjs` 2、`node_perf_report.py` / `node_perf_images.py` / `node_perf_gate.test.cjs` /
`bilibili_capture.cjs` 各 1。迁这些时 workflow 里的调用行要一起改。

## 3. 手写的部分由谁顶替

| 需求 | 现在怎么写的 | 库 | 涉及的脚本 |
|---|---|---|---|
| 起进程、收输出、重试 | `subprocess.run`、PowerShell `&` + `$LASTEXITCODE`、`child_process.spawnSync` | `execa`（已在 `apps/benchmark` 用）+ `p-retry` | 几乎全部 |
| PNG 解码 | `serve_check.py` 自带一个手写解码器；`ShotHarness.psm1` 用 `System.Drawing`；其余用 Pillow | `pngjs`（`apps/benchmark` 已用） | `serve_check`、`demo_check`、`bilibili_check`、`tests/render`、`node_perf_images` |
| 像素比较、差异图 | 逐像素循环 + 手画红黑差异图 | `pixelmatch`（已用）；WPT 式 `maxDifference` 逐通道上限用 10 行循环即可 | `demo_check`、`tests/render`、`pixel_diff` |
| JPEG / WebP 解码、缩放 | Pillow | `sharp` | `bilibili_check`、`node_perf_images`、`node_perf_report` |
| 本地 HTTP 服务 | `http.server` + 手写重定向 handler | `node:http`（内置足够） | `net_check` |
| `--serve` 协议帧 | Python 手写 4 字节长度前缀 | 直接 import `shotium/src/lib/protocol.ts` | `serve_check`、`bilibili_capture` |
| 通配、递归找文件 | `os.walk`、`Get-ChildItem -Recurse` | `tinyglobby` | `charset_check`、`demo_check`、`tests/render` |
| 命令行参数 | `argparse`、`param()` | `cac` | 全部 |
| 并发限流 | `concurrent.futures` | `p-limit` / `p-map` | `check.py`、`ci_stamp_mtimes` |
| 哈希、临时目录、路径 | `hashlib`、`Get-FileHash`、`mkdtemp` | `node:crypto`、`node:fs/promises`、`node:os` | 全部 |
| 统计（p50、均值、区间） | 手算 | `simple-statistics` | `node_perf` |
| 表格、颜色 | 手工对齐 | `picocolors`、`console.table` | 报告类脚本 |
| 测试 | 脚本末尾自报 N 过 M 挂 | `node:test`（`node_perf_gate.test.cjs` 已在用） | 验证套件 |
| 子进程峰值内存 | `.NET Process` 轮询、`/usr/bin/time` | `pidusage` 轮询取最大值 | `peak_memory.ps1`、`measure_memory.sh` |
| 交互式引导 | `Read-Host` | `@clack/prompts` | `bootstrap`（远期） |

`zx` 也能做进程调用，但 `execa` 已经在基准里用着、类型完整、没有模板字符串魔法，不再引入第二套。

## 4. 逐脚本评估

「迁后行数」是估计，用来排优先级，不是承诺。「风险」指迁移过程中让行为悄悄变化的可能。

### 4.1 验证套件（CI 每次引擎构建都跑）

| 脚本 | 行数 | 手写了什么 | 迁后 | 风险 |
|---|---:|---|---:|---|
| `serve_check.py` | 684 | PNG 解码器（约 120 行）、协议帧、临时夹具、分节汇报 | ~350 | 低；它断言的是字节相等，迁后必须仍逐字节比较 |
| `net_check.py` | 386 | 带重定向和缓存头的 HTTP 服务、跨进程缓存目录、摘要比较 | ~220 | 低 |
| `demo_check.py` | 236 | Pillow 解码比较、`fuzzy` meta 解析、smoke 判定 | ~150 | 低；`maxDifference` 是逐通道上限，`pixelmatch` 的阈值不是同一语义，自己循环 |
| `bilibili_check.py` | 180 | Pillow、spawn node 跑截图 | ~120 | 低；改为直接 import 包 |
| `node_check.cjs` | 414 | 已是 JS | ~330 | 无；改名加类型，分节改 `node:test` |
| `daemon_check.cjs` | 248 | 同上 | ~200 | 无 |
| `daemon_protocol_check.cjs` | 182 | 同上 | ~150 | 无 |
| `bilibili_capture.cjs` | 62 | 同上 | ~50 | 无 |

四个 `.cjs` 改名即可运行；两个 Python 检查迁完后，验证套件是一种语言、一套汇报格式，
`/verify-engine` 可以缩成一条 `pnpm verify`。

### 4.2 构建循环（本机）

| 脚本 | 行数 | 手写了什么 | 迁后 | 状态 |
|---|---:|---|---:|---|
| `build.ps1` | 121 | gn / ninja 重试、ICU、补丁 | 190 | **已迁** `scripts/build-engine.ts` |
| `link_agent_skills.ps1` | 40 | junction | 70 | **已迁** `scripts/link-agent-skills.ts` |
| `errors.py`、`build_errors.py` | 197 | ninja 日志正则归类 | ~120 | 迁后由 `build-engine.ts` 进程内调用 |
| `check.py` | 248 | compdb 取命令 + `-fsyntax-only` + 12 路并发 | ~150 | `p-limit` |
| `missing_inputs.py` | 99 | `ninja -t inputs` + stat | ~60 | |
| `accept.ps1` | 76 | 构建、渲染、diff、体积 | ~60 | 等 `tests/render` 迁完一起 |
| `git_retry.ps1`、`stack.ps1` | 80 | 重试、cdb 包装 | ~60 | `p-retry` |
| `peak_memory.ps1`、`measure_memory.sh` | 53 | 峰值内存轮询 | ~60 | 合成一个 `measure_peak.ts`，`pidusage` |
| `docs_*.mjs` | 337 | 只用 `node:` 内置 | 337 | 改扩展名 |

### 4.3 `tests/render`（PowerShell 953 行）

`ShotHarness.psm1`（620）是 `System.Drawing` 解码 + 逐像素比较 + 差异图 + 清单读写；
`run.ps1`（124）、`update-baselines.ps1`（104）、`png-diff.ps1`（29）是它的三个入口。
`pngjs` + `pixelmatch` 覆盖解码、比较和差异图输出，清单是 JSON，估计 300 行以内。
基线目录本来就在 `.gitignore` 里，迁移不改这一点。

### 4.4 性能工具

| 脚本 | 行数 | 手写了什么 | 迁后 |
|---|---:|---|---:|
| `node_perf.cjs` | 542 | 采样、AB/BA 交替、停止规则、p50 与区间手算 | ~450，统计交给 `simple-statistics` |
| `node_perf_gate.cjs` + `.test.cjs` | 337 | 门槛判定 | ~300，测试已是 `node:test` |
| `node_perf_ci.cjs` | 72 | CI 编排 | ~60 |
| `node_perf_images.py`、`node_perf_report.py` | 195 | Pillow 缩略图、Markdown 拼接 | ~120，`sharp` |
| `make_platform_package.cjs` | 204 | 组包、`npm pack` | ~170 |

### 4.5 CI 辅助

| 脚本 | 行数 | 手写了什么 | 迁后 | 风险 |
|---|---:|---|---:|---|
| `ci_stamp_mtimes.py` | 150 | 按 `git log` 把 mtime 改成提交时间 | ~80 | 低；缓存命中率取决于它，迁后对比一次缓存日志 |
| `restore_from_upstream.py` | 179 | 从 `upstream/main` 取回文件 | ~100 | 低 |
| `charset_check.py` | 180 | 扫描编码 | ~80 | 低 |
| `icu_repack.py` | 218 | 重写 ICU `.dat` 的目录表 | ~180 | **中**；二进制格式，迁后必须与 Python 输出逐字节相同再删旧版 |

### 4.6 裁剪期一次性工具（约 7,700 行 Python，不迁）

- **已经完成使命，按「没有活调用方就删」的规矩删掉**：`cut_webgpu.py`、`cut_blink_webgpu.py`、
  `blink_core_coupling.py`、`blink_cut_core_dirs.py`、`blink_drop_core_dir.py`（WebGPU 和
  core 子目录的裁剪早已结束）；`perf_compare.py`（被 `node_perf` 取代）；`diff_report.py`、
  `pixel_diff.py`（被 `tests/render` 和 `pixelmatch` 取代）；`size_cut_groups.py`、
  `size_attrib.py`（§17 的测量已完成）。
- **上游同步时还会用，留在原地，用到再迁**：`gen_idl_*.py`（2,762 行，IDL 枚举 / 联合 / 字典
  生成器，带 `--check`）、`restore_includes*.py`、`restore_gn_deps.py`、`probe_platform_graph.py`、
  `jumbo_collision_scan.py`、`gn_missing_sources.py`、`gn_undefined_vars.py`、
  `gn_dangling_imports.py`、`mojom_dangling_imports.py`、`missing_inputs.py`（这一个常用，
  已列入 4.2）。
- **可能再用一次的裁剪工具，冻结**：`gn_drop*.py`、`cpp_drop_*.py`、`strip_component.py`、
  `cut_orphan_sources.py`、`find_*.py`、`deps_drop.py`、`pdl_drop_type.py`、`vendor_cppgc.py`、
  `size_report.py`、`pgo.py`、`pgo_train.py`。建议挪进 `tools/shot/legacy/`，加一个 README
  写明「不再维护，需要时先迁再用」。

### 4.7 `bootstrap/`（2,529 行，暂不迁）

`bootstrap.ps1` 加四个 `.psm1`（Core 613、Phase1 514、Lock 495、Guard 365）和 `deps_lock.py`
（291），负责新机器的 depot_tools 钉版、父目录 `.gclient`、SDK 检查和锁文件。CI 不用它，
`engine-windows.yml` 有自己的内联步骤。它是每台机器跑一次的东西，等 checkout 布局定型再迁；
到时的形状是 `execa` + `@clack/prompts` + `zod` 校验锁文件，估计 800 行。

### 4.8 workflow 内联脚本（1,067 行）

`engine-windows.yml` 278 行、`engine-linux.yml` 204、`engine-macos.yml` 188、`publish.yml` 189、
`checks.yml` 110、`benchmark.yml` 90。里面是 SDK 探测、gclient、缓存、`.7z` 打包、artifact 收集、
npm 发布顺序、五段 `node -e` 的包检查。目标形状：每段变成 `tools/shot/ci_*.ts` 的一条命令，
YAML 只剩 `run: pnpm ci:sdk`、`pnpm ci:package`、`pnpm release:collect` 这种一行；
`checks.yml` 里五段 `node -e` 变成 `shotium/test/*.test.ts`。好处是本机能跑、有类型、
同一逻辑只在一处。

## 5. 运行方式

- 选 `tsx`：`apps/benchmark` 已用，`.ts` 之间 import 不用写扩展名，脚本参数原样透传。
- Node 原生剥类型（22.18+ 默认开）也能跑，`tsconfig.json` 的 `erasableSyntaxOnly` 保证不写
  它剥不掉的语法（enum、命名空间、参数属性）。CI 的 `checks.yml` 用 Node 22，其余用 24，都满足。
- pnpm 跑包脚本时 `cwd` 是 `scripts/`，用户给的相对路径一律按仓库根
  （`path.resolve(import.meta.dirname, '..')`）解析，并在 `--help` 里说明。不能用
  `INIT_CWD`：根目录别名是 `pnpm -C scripts <name>`，里层的 pnpm 会把 `INIT_CWD` 覆盖成
  外层包目录，也就是仓库根，用户敲命令的目录已经拿不到了。`build-engine.ts` 第一版按
  `INIT_CWD` 解析，就是这样栽的。

## 6. 工作区：两条实测约束

**约束一：根目录的 `pnpm-workspace.yaml` 会接管所有子目录的 `pnpm install`。**
在工作区根之下、但没列进 `packages` 的项目里跑 `pnpm install`，pnpm 打印
`Scope: all N workspace projects`，安装的是工作区，该项目自己一个依赖都不装。
子项目里放 `.npmrc` 写 `ignore-workspace=true` 不起作用（pnpm 9.15.9），只有命令行
`--ignore-workspace` 有效。受影响的是 `checks.yml`、`benchmark.yml`、`performance-regression.yml`
和三个 `engine-*.yml` 里所有在 `shotium/`、`apps/benchmark` 内执行的 `pnpm install`。

**约束二：shotium 进不了带 `--frozen-lockfile` 的工作区。** 它的六个平台包钉在本版本号上，
版本号提交到 tag 之间这个号在 registry 上不存在。实测：普通 `pnpm install` 会静默跳过解析
不到的 optionalDependency 并写出一份不含它的 lockfile；随后 `--frozen-lockfile` 因为
lockfile 与 `package.json` 的 specifier 不一致而失败。所以工作区 lockfile 在每次发版窗口
都会坏，这就是 `checks.yml` 一直用 `--no-lockfile` 的原因，进了工作区也改不了。

**现在的做法**：`scripts/` 是独立 pnpm 项目（自己的 `pnpm-lock.yaml`），根 `package.json`
用 `pnpm -C scripts` 转发，对 shotium 和 apps 零影响。

**要做成真正的工作区**，成员应该是 `tools/shot` + `apps/*`，shotium 永远在外面：

1. 根目录 `pnpm-workspace.yaml`：`packages: [tools/shot, apps/*]`，根 `pnpm-lock.yaml` 入库，
   删掉 `apps/benchmark/pnpm-lock.yaml`。
2. `checks.yml` 的 benchmark job：去掉只取 `apps/benchmark` 的 sparse-checkout（工作区需要根
   目录的三个文件），`cache-dependency-path` 改成根 lockfile，安装改成
   `pnpm install --frozen-lockfile --filter shotium-benchmark`。`benchmark.yml`、
   `performance-regression.yml` 同样。
3. 所有在 `shotium/` 里的 `pnpm install` 加 `--ignore-workspace`（`checks.yml` 一处、
   `engine-*.yml` 三处），`shotium/README.md` 和 `CLAUDE.md` 写明本机也要加。
4. `apps/benchmark-site`、`apps/demo` 顺带并入，它们没有特殊安装需求。

代价是 shotium 那一个 flag；收益是脚本和三个 app 共用一份 lockfile、`pnpm -r` 一次跑完
类型检查和测试。建议在 4.1 迁完之后做，那时 `tools/shot` 的依赖面已经稳定。

## 7. 迁移顺序

每一步的验收都是「新旧两版对同一输入给出相同结果」，不是「新版能跑」。

| 阶段 | 内容 | 要改的 workflow | 验收 |
|---|---|---|---|
| 1（已完成） | `scripts/build-engine.ts`、`scripts/link-agent-skills.ts`、`scripts/` 成为 pnpm 项目、规则入 `CLAUDE.md` | 无 | no-op 构建 33 秒跑通，junction 建删往返 |
| 2 | 4.1 验证套件；`errors` / `build_errors` 进程内化 | `engine-*.yml` 的 checks 步骤各 4 行 | 每个套件的通过 / 失败条数与旧版一致；`serve_check` 仍逐字节比较 |
| 3 | 4.3 `tests/render` + `accept.ps1` | 无 | 同一基线下逐像素结果一致 |
| 4 | 4.4 性能工具 | `performance-regression.yml`、`benchmark.yml` 调用行 | 同一份 `result.json` 出同一份报告 |
| 5 | 4.5 CI 辅助，`icu_repack` 最后 | `engine-*.yml` 调用行 | `icu_repack` 输出逐字节相同 |
| 6 | 4.8 workflow 内联脚本收进 `ci_*.ts` | 全部 | 一次完整的六平台构建 + 发版干跑 |
| 7 | 第 6 节的工作区 | 见第 6 节 | `checks.yml` 三个 job 全绿 |
| 不排期 | 4.6 删除与冻结、4.7 bootstrap | | |

阶段 2 是收益最大的一步：约 2,600 行 Python 和 CommonJS 变成 1,500 行左右的 TypeScript，
四种汇报格式变成 `node:test` 一种，PNG 解码器和协议帧各只剩一份。

## 8. 本次提交的内容

- `scripts/build-engine.ts`：`build.ps1` 的逐项等价移植。`execa` 起进程，`p-retry` 做
  `gn gen` 的八次重试并在非竞态错误上立即放弃，`cac` 解析 `--target` / `--jobs` / `--log`，
  ninja 输出经 `stream/promises.pipeline` 进日志文件。实测：补丁检查、ICU 重打包、
  `gn gen` 27 秒、ninja no-op、`build_errors.py` 汇总，全程 33 秒，二进制未被触碰。
- `scripts/link-agent-skills.ts`：`fs.symlink(..., 'junction')` 建链接，Windows 上
  `rmdir` 删链接（实测只删 junction，目标目录完好）。
- `scripts/package.json`、`tsconfig.json`、`pnpm-lock.yaml`：独立 pnpm 项目，
  依赖 `cac` 7.0.0、`execa` 10.0.1、`p-retry` 8.0.1、`picocolors` 1.1.1、`tsx` 4.23.13，
  开发依赖 `typescript` 5.9.2、`@types/node` 22.18.0，全部精确钉版。
- 根 `package.json`：`build:engine`、`skills:link`、`scripts:check` 转发。
- `tools/shot/accept.ps1`：改为调用 `pnpm build:engine`。
- `CLAUDE.md`：第 2 条、第 11 条、Building、Code conventions 的 Scripts 小节、目录树、文档表。
- 删除 `tools/shot/build.ps1`、`tools/shot/link_agent_skills.ps1`。
