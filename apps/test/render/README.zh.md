# 静态渲染回归测试

[English](./README.md) · 简体中文

这些用例不包含 JavaScript、CSS 动画、系统时钟、随机数或外部网络依赖，均在固定的视口尺寸与缩放比例下运行。排版敏感的文字用例使用仓库内置的 Ahem 字体（`shot/testdata/ahem.ttf`），避免受宿主系统安装字体的影响。

测试驱动脚本为 `scripts/verify/render-regression.ts`（通过 `pnpm render` 调用）。本目录仅存放测试用例与元数据：`cases/`、`cases.json`，以及被版本控制忽略的 `baselines/` 目录。

从指定版本的源码构建生成参考基准图：

```sh
pnpm render update-baselines \
  --baseline-engine headless-shell \
  --baseline-executable ./out/Release/headless_shell.exe \
  --accept
```

宿主系统安装的 Chrome 可通过 `--baseline-engine system-chrome` 显式指定并度量，基准清单中会明确标记为 `external-system-chrome`，与源码构建的 `headless_shell` 基线及 `source-build-shot` 严格区分。

运行 Shotium 并进行解码后的逐像素比对：

```sh
pnpm render run --shot ./out/Shot/shotium.exe
```

比对报告包含 PNG 宽高、编码后字节数、SHA-256 校验和、差异像素数及占比、最大通道差、平均绝对通道差与 RMSE。SHA-256 作为文件完整性校验信号；解码后的像素矩阵才是渲染正确性的判定标准，因为不同的无损 PNG 编码实现可能对相同像素生成不同的字节流。

单次图片比对可使用 `pnpm render diff <expected.png> <actual.png>`。默认判定阈值要求解码后像素完全一致。仅在人工核对生成的红黑差异图并在 `cases.json` 中记录技术原因后，方可放宽特定用例的容差阈值。
