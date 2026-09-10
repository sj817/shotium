# Shotium 基准测试站点

[English](./README.md) · 简体中文

基于 VitePress 1.x、Vue 3 与 TypeScript 构建的基准评测可视化站点，用于直观展示仓库归档的基准测试报告，托管于 GitHub Pages：`https://sj817.github.io/shotium/`。默认语言为中文（`/`），英文版本位于 `/en/`；支持多语言切换与明暗主题模式切换。

UI 层面基于 Element Plus 实现交互组件（标签页、下拉选择、带排序与分页的数据表格、状态指示器、统计面板等）；图表部分采用 ECharts（通过 `vue-echarts` 按需加载 SVG 渲染器）绘制多场景耗时对比图与综合排名条形图。样式系统通过 design tokens 统一管理色彩与毛玻璃质感，暗色主题通过切换 token 调色板平滑生效。

## 布局

| 路径 | 作用 |
| --- | --- |
| `docs/index.md`、`docs/en/index.md` | 页面入口，各自在 `<ClientOnly>` 内挂载 `<BenchmarkReport>` |
| `docs/.vitepress/config.ts` | 语言、站点根路径（`BENCHMARK_SITE_BASE`）、开发时直接提供 `../../docs/benchmarks/` 的中间件，以及复制进 `dist/` 的 `buildEnd` 钩子 |
| `docs/.vitepress/theme/tokens.css` | 颜色、字体、圆角与毛玻璃参数的统一设计令牌（亮色与 `html.dark`） |
| `docs/.vitepress/theme/glass.css` | 壁纸、限定 GPU 开销的玻璃层级、VitePress 导航栏/页脚/菜单覆盖，以及动画暂停与降低透明度的降级回退 |
| `docs/.vitepress/theme/report.css` | 页面容器、Element Plus 变量映射、避免嵌套模糊、屏外渲染隔离及玻璃风格弹层表面（`popper-class="bench-pop"`） |
| `docs/lib/types.ts`、`data.ts` | 结果 schema 与浏览器端加载器（相对站点根路径获取） |
| `docs/lib/ranking.ts` | 排名规则（`test/ranking.test.ts`） |
| `docs/lib/report.ts` | 运行级聚合：各平台排名、「无排名」原因、结论数据统计（`test/report.test.ts`） |
| `docs/lib/messages.ts`、`i18n.ts` | 双语国际化类型化文案（保证键名完备性） |
| `docs/lib/format.ts`、`labels.ts` | 数字与日期格式化、引擎与场景顺序及颜色变量 |
| `docs/lib/theme.ts`、`echarts.ts` | 渲染时从 CSS 读取设计令牌（主题切换时响应重读）；按需注册的 ECharts 模块 |
| `docs/components/` | `BenchmarkReport` 根组件及其子组件树、面板与状态组件 |
| `DESIGN.md` | 信息架构与视觉系统设计文档 |


## 数据流设计

站点本身不硬编码或打包任何评测快照数据。本地开发模式下通过 Vite 中间件直接伺服 `apps/docs/benchmarks/` 目录；在执行生产构建（`vitepress build`）时将归档数据复制到静态输出目录 `docs/.vitepress/dist/benchmark-results/`。浏览器端基于相对路径动态拉取 JSON 数据：

- `benchmark-results/index.json`
- `benchmark-results/<run>/manifest.json`
- `benchmark-results/<run>/<platform>/summary.json`
- `benchmark-results/<run>/<platform>/failures.json`（可选）

`samples.jsonl`、`quality.json`、`report.md` 与 `summary.csv` 只链接不解析。选中的运行与平台保存在 URL 里（`?run=<path>&platform=<id>`）。

## 排名算法与计分模型

评测比对严格限定在同一测试平台上进行。单项场景耗时指标取 p50 分位数（`latency_ms.p50 ?? wall_time_ms.p50`），有效值必须大于 0。纳入比对的用例需满足：Shotium 与至少一个竞品同时测试通过、具备计分资格且具备有效 p50 数据。各引擎综合得分为其覆盖用例中相对 Shotium 耗时比值（`引擎 p50 ÷ Shotium p50`）的几何平均值（数值越小速度越快，Shotium 基准为 1.0×）。只有覆盖全部可比场景的引擎才参与正式榜单排名；部分覆盖场景仅列出参考得分与排除原因。页面展示的综合结论、图表数据与明细表格均由前端运行时根据当前数据动态计算生成，无人工写死数据。

## 命令

```bash
pnpm install --frozen-lockfile
pnpm run dev        # http://localhost:5173/
pnpm run typecheck  # vue-tsc -b
pnpm test           # vitest
pnpm run build      # vue-tsc -b && vitepress build docs
pnpm run check      # typecheck + test + build（CI 执行的内容）
```

CI（`.github/workflows/benchmark-site.yml`，Ubuntu 与 Node 24）在 `BENCHMARK_SITE_BASE=/shotium/` 下执行 `pnpm install --frozen-lockfile && pnpm run check` 并上传 `docs/.vitepress/dist`。`apps/docs/benchmarks/index.json` 缺失时构建失败，而不是发布一份空报告。

## 视觉自检与回归验证

本地开发期间，可利用 `apps/benchmark` 中的 Playwright 自动化工具对前端样式进行多端视觉核验：在开发服务运行状态下，启动浏览器分别在 1280px、1024px、390px 视口下验证明暗主题呈现，核查页面无水平溢出、图表与文本对比度符合可读性标准、毛玻璃样式无渲染穿透，并且所有状态标签的 Tooltip 提示完整可读。
