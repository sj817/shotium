# Shotium 0.3.4 基准报告

[交互式基准站点](https://sj817.github.io/shotium/)

结果：**不完整**；质量：**失败**；证据：**不完整**。配置：**完整测试**；种子：`6bf4cb2fbc8e69e59371281916ce6bf49a73ef24`。

结论：本次结果不完整。0 个平台形成了有效的平台内对比；缺失结果不会被推测或补齐。

每个比率只在 Shotium 与对比引擎位于同一平台、同一场景、同一并发度，且双方状态均为“通过”并允许排名时计算。本报告不进行跨平台混排。

## 六平台总览

| 平台 | 质量状态 | 正式第一名 | 正式参赛引擎数 | 平台可比项数 |
|:--|:--|:--|--:|--:|
| linux-x64 | 失败 | 无有效排名 | 0 | 9 |
| linux-arm64 | 噪声过大 | 无有效排名 | 0 | 9 |
| win32-x64 | 基础设施错误 | 无有效排名 | 0 | 0 |
| win32-arm64 | 基础设施错误 | 无有效排名 | 0 | 0 |
| darwin-x64 | 失败 | 无有效排名 | 0 | 3 |
| darwin-arm64 | 失败 | 无有效排名 | 0 | 2 |

## linux-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“失败”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 噪声过大 | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | 通过 | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | 失败 | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | 通过 | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | 基础设施错误 | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | playwright-chrome | 通过 | 否 | 536 | 600 | 1.810 | 不适用 |
| 冷启动 | 1 | playwright-shell | 通过 | 否 | 332 | 405 | 2.989 | 不适用 |
| 冷启动 | 1 | puppeteer-chrome | 通过 | 否 | 583 | 610 | 1.722 | 不适用 |
| 冷启动 | 1 | puppeteer-shell | 通过 | 否 | 343 | 355 | 2.950 | 不适用 |
| 冷启动 | 1 | shotium | 通过 | 否 | 65 | 72 | 15.054 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-chrome | 通过 | 否 | 168.92 | 192.91 | 5.983 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 噪声过大 | 否 | 184.866 | 184.866 | 0.048 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-shell | 通过 | 否 | 130.708 | 141.063 | 7.737 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 通过 | 否 | 16.158 | 22.212 | 58.739 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 通过 | 否 | 117.893 | 127.631 | 8.387 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 279.571 | 330.136 | 3.501 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 607.464 | 652.171 | 1.696 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-shell | 通过 | 否 | 363.373 | 458.215 | 2.679 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 64.013 | 129.632 | 13.455 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-chrome | 通过 | 否 | 757.717 | 799.481 | 1.352 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 噪声过大 | 否 | 156.293 | 188.066 | 2.731 | 不适用 |
| 预热截图 | 1 | shotium | 通过 | 否 | 12.889 | 19.721 | 69.256 | 不适用 |
| 预热截图 | 1 | playwright-shell | 通过 | 否 | 117.402 | 136.254 | 8.403 | 不适用 |
| 预热截图 | 1 | puppeteer-shell | 通过 | 否 | 130.303 | 180.449 | 7.717 | 不适用 |
| 预热截图 | 1 | puppeteer-chrome | 通过 | 否 | 161.309 | 204.583 | 6.147 | 不适用 |
| 顺序批量 | 1 | shotium | 通过 | 否 | 28.024 | 259.312 | 20.359 | 不适用 |
| 顺序批量 | 1 | puppeteer-shell | 通过 | 否 | 147.279 | 364.756 | 5.854 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 通过 | 否 | 134.622 | 355.192 | 6.324 | 不适用 |
| 顺序批量 | 1 | puppeteer-chrome | 通过 | 否 | 183.902 | 397.02 | 4.865 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 噪声过大 | 否 | 168.929 | 374.159 | 4.347 | 不适用 |
| 并发截图 | 1 | playwright-chrome | 通过 | 否 | 173.234 | 377.585 | 5.140 | 不适用 |
| 并发截图 | 1 | puppeteer-shell | 通过 | 否 | 147.893 | 479.555 | 5.761 | 不适用 |
| 并发截图 | 1 | puppeteer-chrome | 通过 | 否 | 181.746 | 399.016 | 4.908 | 不适用 |
| 并发截图 | 1 | shotium | 通过 | 否 | 27.599 | 261.072 | 20.651 | 不适用 |
| 并发截图 | 1 | playwright-shell | 通过 | 否 | 137.752 | 352.973 | 6.216 | 不适用 |
| 并发截图 | 2 | puppeteer-chrome | 通过 | 否 | 316.638 | 460.433 | 5.744 | 不适用 |
| 并发截图 | 2 | shotium | 通过 | 否 | 62.573 | 276.373 | 20.539 | 不适用 |
| 并发截图 | 2 | playwright-shell | 通过 | 否 | 247.68 | 457.936 | 7.171 | 不适用 |
| 并发截图 | 2 | playwright-chrome | 噪声过大 | 否 | 319.515 | 483.075 | 3.799 | 不适用 |
| 并发截图 | 2 | puppeteer-shell | 通过 | 否 | 255.298 | 393.974 | 6.752 | 不适用 |
| 并发截图 | 4 | playwright-shell | 通过 | 否 | 426.332 | 786.175 | 7.814 | 不适用 |
| 并发截图 | 4 | playwright-chrome | 噪声过大 | 否 | 558.014 | 724.559 | 2.978 | 不适用 |
| 并发截图 | 4 | puppeteer-shell | 通过 | 否 | 502.642 | 699.073 | 7.197 | 不适用 |
| 并发截图 | 4 | puppeteer-chrome | 失败 | 否 | 595.886 | 76540.607 | 1.067 | 不适用 |
| 并发截图 | 4 | shotium | 通过 | 否 | 131.628 | 328.4 | 20.264 | 不适用 |
| 复用页面 | 1 | playwright-shell | 通过 | 否 | 67.145 | 83.32 | 14.437 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 通过 | 否 | 67.933 | 94.144 | 14.176 | 不适用 |
| 复用页面 | 1 | puppeteer-shell | 通过 | 否 | 82.118 | 85.25 | 12.702 | 不适用 |
| 复用页面 | 1 | puppeteer-chrome | 通过 | 否 | 85.815 | 115.92 | 10.923 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-shell | 通过 | 否 | 858 | 949 | 1.169 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 噪声过大 | 否 | 190 | 466 | 0.290 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 通过 | 否 | 886 | 1065 | 1.090 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 通过 | 否 | 1011 | 1133 | 0.962 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-chrome | 噪声过大 | 否 | 16399.376 | 16504.764 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-shell | 通过 | 否 | 9660.592 | 9660.592 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-chrome | 通过 | 否 | 11308.981 | 11308.981 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 通过 | 否 | 9781.608 | 9781.608 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 通过 | 否 | 14911.922 | 14911.922 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 噪声过大 | 否 | 3463.032 | 3463.032 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-shell | 通过 | 否 | 434.917 | 878.386 | 8.707 | 不适用 |
| 持续压力 | 4 | puppeteer-chrome | 失败 | 否 | 614.702 | 180941.964 | 1.958 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 基础设施错误 | 否 | 549.083 | 993.122 | 6.996 | 不适用 |
| 持续压力 | 4 | puppeteer-shell | 通过 | 否 | 483.301 | 905.498 | 8.080 | 不适用 |
| 持续压力 | 4 | shotium | 通过 | 否 | 141.308 | 392.196 | 21.703 | 不适用 |

## linux-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“噪声过大”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 噪声过大 | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | 不适用 | 启动场景：该软件包没有适用于此平台架构的原生浏览器；吞吐场景：该软件包没有适用于此平台架构的原生浏览器；并发场景：该软件包没有适用于此平台架构的原生浏览器；常驻场景：该软件包没有适用于此平台架构的原生浏览器；韧性场景：该软件包没有适用于此平台架构的原生浏览器 |
| puppeteer-chrome | 不适用 | 启动场景：该软件包没有适用于此平台架构的原生浏览器；吞吐场景：该软件包没有适用于此平台架构的原生浏览器；并发场景：该软件包没有适用于此平台架构的原生浏览器；常驻场景：该软件包没有适用于此平台架构的原生浏览器；韧性场景：该软件包没有适用于此平台架构的原生浏览器 |
| playwright-shell | 通过 | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | 噪声过大 | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | playwright-chrome | 通过 | 否 | 387 | 436 | 2.568 | 不适用 |
| 冷启动 | 1 | playwright-shell | 通过 | 否 | 234 | 437 | 3.806 | 不适用 |
| 冷启动 | 1 | shotium | 通过 | 否 | 61 | 65 | 16.204 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 通过 | 否 | 100.514 | 117.535 | 9.825 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 通过 | 否 | 139.2 | 151.137 | 7.348 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 通过 | 否 | 34.135 | 37.535 | 28.873 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 223.327 | 285.695 | 4.295 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 414.74 | 459.802 | 2.399 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 69.436 | 97.919 | 13.550 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 通过 | 否 | 132.457 | 170.762 | 7.421 | 不适用 |
| 预热截图 | 1 | playwright-shell | 通过 | 否 | 103.057 | 131.859 | 9.281 | 不适用 |
| 预热截图 | 1 | shotium | 通过 | 否 | 31.115 | 36.981 | 30.619 | 不适用 |
| 顺序批量 | 1 | shotium | 通过 | 否 | 41.573 | 263.164 | 17.258 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 通过 | 否 | 127.508 | 324.973 | 7.033 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 通过 | 否 | 137.543 | 352.136 | 6.225 | 不适用 |
| 并发截图 | 1 | playwright-chrome | 噪声过大 | 否 | 167.68 | 371.411 | 4.467 | 不适用 |
| 并发截图 | 1 | shotium | 通过 | 否 | 44.512 | 264.778 | 16.510 | 不适用 |
| 并发截图 | 1 | playwright-shell | 通过 | 否 | 132.379 | 352.231 | 6.451 | 不适用 |
| 并发截图 | 2 | shotium | 通过 | 否 | 86.974 | 301.296 | 16.826 | 不适用 |
| 并发截图 | 2 | playwright-shell | 通过 | 否 | 204.366 | 362.629 | 8.550 | 不适用 |
| 并发截图 | 2 | playwright-chrome | 通过 | 否 | 266.92 | 394.053 | 7.000 | 不适用 |
| 并发截图 | 4 | playwright-shell | 通过 | 否 | 356.323 | 576.342 | 9.585 | 不适用 |
| 并发截图 | 4 | playwright-chrome | 通过 | 否 | 457.045 | 607.196 | 7.786 | 不适用 |
| 并发截图 | 4 | shotium | 通过 | 否 | 174.22 | 369.477 | 16.852 | 不适用 |
| 复用页面 | 1 | playwright-shell | 通过 | 否 | 50.213 | 69.855 | 18.773 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 通过 | 否 | 64.971 | 67.096 | 15.817 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 通过 | 否 | 836 | 867 | 1.232 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 通过 | 否 | 890 | 931 | 1.144 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 噪声过大 | 否 | 338 | 415 | 0.333 | 不适用 |
| 异常与恢复 | 1 | shotium | 通过 | 否 | 9137.552 | 9137.552 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 通过 | 否 | 17250.492 | 17250.492 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 通过 | 否 | 13145.578 | 13145.578 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 通过 | 否 | 375.958 | 673.265 | 10.135 | 不适用 |
| 持续压力 | 4 | playwright-shell | 通过 | 否 | 285.434 | 543.489 | 12.844 | 不适用 |
| 持续压力 | 4 | shotium | 通过 | 否 | 179.177 | 424.282 | 17.776 | 不适用 |

## win32-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“基础设施错误”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|

## win32-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“基础设施错误”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|

## darwin-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“失败”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 噪声过大 | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | 噪声过大 | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | 失败 | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | 噪声过大 | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | 失败 | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | playwright-chrome | 通过 | 否 | 4623 | 15502 | 0.167 | 不适用 |
| 冷启动 | 1 | playwright-shell | 通过 | 否 | 1118 | 2172 | 0.769 | 不适用 |
| 冷启动 | 1 | puppeteer-chrome | 通过 | 否 | 3273 | 7469 | 0.248 | 不适用 |
| 冷启动 | 1 | puppeteer-shell | 通过 | 否 | 1301 | 1805 | 0.764 | 不适用 |
| 冷启动 | 1 | shotium | 通过 | 否 | 172 | 529 | 4.375 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-chrome | 噪声过大 | 否 | 11176.351 | 13033.562 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 通过 | 否 | 817.63 | 901.08 | 1.209 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-shell | 通过 | 否 | 312.791 | 418.888 | 2.976 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 噪声过大 | 否 | 20.469 | 26.325 | 0.412 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 噪声过大 | 否 | 319.144 | 339.822 | 0.092 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 1234.88 | 2082.948 | 0.749 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 3379.47 | 4631.868 | 0.281 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-shell | 通过 | 否 | 1586.12 | 1942.095 | 0.660 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 209.276 | 555.73 | 4.293 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-chrome | 通过 | 否 | 4188.048 | 5142.899 | 0.243 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 噪声过大 | 否 | 882.866 | 1220.821 | 1.098 | 不适用 |
| 预热截图 | 1 | shotium | 噪声过大 | 否 | 24.167 | 25.145 | 0.368 | 不适用 |
| 预热截图 | 1 | playwright-shell | 噪声过大 | 否 | 8723.494 | 9590.523 | 不适用 | 不适用 |
| 预热截图 | 1 | puppeteer-shell | 通过 | 否 | 350.143 | 446.201 | 2.934 | 不适用 |
| 预热截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 11966.652 | 12461.073 | 不适用 | 不适用 |
| 顺序批量 | 1 | shotium | 噪声过大 | 否 | 59.423 | 385.714 | 6.034 | 不适用 |
| 顺序批量 | 1 | puppeteer-shell | 通过 | 否 | 369.255 | 662.248 | 2.544 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 噪声过大 | 否 | 324.869 | 532.382 | 0.586 | 不适用 |
| 顺序批量 | 1 | puppeteer-chrome | 噪声过大 | 否 | 11228.043 | 11675.331 | 不适用 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 通过 | 否 | 903.096 | 1296.935 | 1.082 | 不适用 |
| 并发截图 | 1 | playwright-chrome | 噪声过大 | 否 | 906.564 | 1410.417 | 0.900 | 不适用 |
| 并发截图 | 1 | puppeteer-shell | 噪声过大 | 否 | 341.134 | 660.43 | 1.853 | 不适用 |
| 并发截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 9977.01 | 11744.662 | 不适用 | 不适用 |
| 并发截图 | 1 | shotium | 噪声过大 | 否 | 63.165 | 329.298 | 8.532 | 不适用 |
| 并发截图 | 1 | playwright-shell | 噪声过大 | 否 | 341.323 | 590.311 | 0.592 | 不适用 |
| 并发截图 | 2 | puppeteer-chrome | 噪声过大 | 否 | 8579.806 | 11607.587 | 不适用 | 不适用 |
| 并发截图 | 2 | shotium | 噪声过大 | 否 | 85.199 | 288.071 | 6.311 | 不适用 |
| 并发截图 | 2 | playwright-shell | 噪声过大 | 否 | 449.869 | 680.605 | 0.740 | 不适用 |
| 并发截图 | 2 | playwright-chrome | 失败 | 否 | 1415.253 | 3636.407 | 1.031 | 不适用 |
| 并发截图 | 2 | puppeteer-shell | 通过 | 否 | 464.775 | 809.432 | 4.052 | 不适用 |
| 并发截图 | 4 | playwright-shell | 噪声过大 | 否 | 741.027 | 1451.816 | 1.207 | 不适用 |
| 并发截图 | 4 | playwright-chrome | 失败 | 否 | 2682.982 | 6731.402 | 1.347 | 不适用 |
| 并发截图 | 4 | puppeteer-shell | 通过 | 否 | 769.684 | 1959.266 | 4.472 | 不适用 |
| 并发截图 | 4 | puppeteer-chrome | 噪声过大 | 否 | 9199.031 | 10794.529 | 不适用 | 不适用 |
| 并发截图 | 4 | shotium | 噪声过大 | 否 | 186.087 | 526.292 | 4.476 | 不适用 |
| 复用页面 | 1 | playwright-shell | 噪声过大 | 否 | 5259.146 | 5259.146 | 不适用 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | puppeteer-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | puppeteer-chrome | 噪声过大 | 否 | 7021.205 | 7021.205 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-shell | 噪声过大 | 否 | 3363 | 4197 | 0.101 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 噪声过大 | 否 | 21916.349 | 27706.591 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 噪声过大 | 否 | 2592 | 4148 | 0.055 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 失败 | 否 | 49293.081 | 61064.171 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-chrome | 失败 | 否 | 2157 | 2281 | 0.012 | 不适用 |
| 异常与恢复 | 1 | puppeteer-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-chrome | 噪声过大 | 否 | 11490.006 | 11490.006 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 噪声过大 | 否 | 6030.664 | 6030.664 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 噪声过大 | 否 | 9267.233 | 9267.233 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 通过 | 否 | 42488.346 | 42488.346 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-shell | 噪声过大 | 否 | 6376.069 | 6376.069 | 不适用 | 不适用 |
| 持续压力 | 4 | puppeteer-chrome | 噪声过大 | 否 | 9222.694 | 9222.694 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 失败 | 否 | 2954.813 | 7149.026 | 1.093 | 不适用 |
| 持续压力 | 4 | puppeteer-shell | 通过 | 否 | 808.473 | 1768.91 | 4.783 | 不适用 |
| 持续压力 | 4 | shotium | 通过 | 否 | 176.215 | 870.766 | 18.436 | 不适用 |

## darwin-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“失败”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 噪声过大 | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | 噪声过大 | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | 噪声过大 | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | 噪声过大 | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | 失败 | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | playwright-chrome | 噪声过大 | 否 | 5536.5 | 7438 | 0.181 | 不适用 |
| 冷启动 | 1 | playwright-shell | 噪声过大 | 否 | 1363 | 1956 | 0.733 | 不适用 |
| 冷启动 | 1 | puppeteer-chrome | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 冷启动 | 1 | puppeteer-shell | 噪声过大 | 否 | 1341 | 1583 | 0.746 | 不适用 |
| 冷启动 | 1 | shotium | 噪声过大 | 否 | 107 | 174 | 8.065 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-chrome | 噪声过大 | 否 | 6834.059 | 8000.398 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 噪声过大 | 否 | 216.014 | 271.225 | 4.328 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-shell | 噪声过大 | 否 | 297.03 | 338.777 | 0.151 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 噪声过大 | 否 | 10.31 | 14.387 | 0.417 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 噪声过大 | 否 | 188.779 | 413.709 | 0.527 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 493.453 | 906.426 | 1.885 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 1977.412 | 3194.205 | 0.476 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-shell | 通过 | 否 | 638.561 | 1166.601 | 1.419 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 95.997 | 131.49 | 10.833 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-chrome | 通过 | 否 | 2180.04 | 3540.398 | 0.431 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 噪声过大 | 否 | 258.445 | 421.296 | 3.839 | 不适用 |
| 预热截图 | 1 | shotium | 通过 | 否 | 11 | 61.271 | 64.737 | 不适用 |
| 预热截图 | 1 | playwright-shell | 通过 | 否 | 135.149 | 232.58 | 7.155 | 不适用 |
| 预热截图 | 1 | puppeteer-shell | 噪声过大 | 否 | 185.069 | 264.455 | 2.134 | 不适用 |
| 预热截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 6288.231 | 7460.062 | 不适用 | 不适用 |
| 顺序批量 | 1 | shotium | 噪声过大 | 否 | 24.017 | 268.205 | 21.985 | 不适用 |
| 顺序批量 | 1 | puppeteer-shell | 噪声过大 | 否 | 205.189 | 531.302 | 3.831 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 噪声过大 | 否 | 146.666 | 369.336 | 5.702 | 不适用 |
| 顺序批量 | 1 | puppeteer-chrome | 噪声过大 | 否 | 5940.092 | 6809.501 | 不适用 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 噪声过大 | 否 | 321.694 | 1433.614 | 2.514 | 不适用 |
| 并发截图 | 1 | playwright-chrome | 噪声过大 | 否 | 366.408 | 1266.863 | 1.399 | 不适用 |
| 并发截图 | 1 | puppeteer-shell | 噪声过大 | 否 | 225.159 | 603.233 | 4.082 | 不适用 |
| 并发截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 6247.266 | 10181.269 | 不适用 | 不适用 |
| 并发截图 | 1 | shotium | 噪声过大 | 否 | 37.506 | 264.318 | 11.532 | 不适用 |
| 并发截图 | 1 | playwright-shell | 噪声过大 | 否 | 242.984 | 568.835 | 2.186 | 不适用 |
| 并发截图 | 2 | puppeteer-chrome | 噪声过大 | 否 | 7287.501 | 9168.148 | 不适用 | 不适用 |
| 并发截图 | 2 | shotium | 噪声过大 | 否 | 56.422 | 287.083 | 15.532 | 不适用 |
| 并发截图 | 2 | playwright-shell | 噪声过大 | 否 | 304.852 | 695.425 | 4.097 | 不适用 |
| 并发截图 | 2 | playwright-chrome | 失败 | 否 | 669.503 | 1868.575 | 2.596 | 不适用 |
| 并发截图 | 2 | puppeteer-shell | 噪声过大 | 否 | 385.679 | 1121.22 | 3.956 | 不适用 |
| 并发截图 | 4 | playwright-shell | 噪声过大 | 否 | 539.9 | 899.575 | 6.833 | 不适用 |
| 并发截图 | 4 | playwright-chrome | 失败 | 否 | 915.103 | 3267.395 | 2.065 | 不适用 |
| 并发截图 | 4 | puppeteer-shell | 噪声过大 | 否 | 536.122 | 1028.033 | 6.329 | 不适用 |
| 并发截图 | 4 | puppeteer-chrome | 噪声过大 | 否 | 6908.631 | 7642.027 | 不适用 | 不适用 |
| 并发截图 | 4 | shotium | 噪声过大 | 否 | 126.6 | 387.593 | 20.618 | 不适用 |
| 复用页面 | 1 | playwright-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 通过 | 否 | 140.034 | 295.315 | 6.563 | 不适用 |
| 复用页面 | 1 | puppeteer-shell | 通过 | 否 | 155.391 | 243.211 | 5.903 | 不适用 |
| 复用页面 | 1 | puppeteer-chrome | 噪声过大 | 否 | 5501.616 | 5501.616 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-shell | 噪声过大 | 否 | 1899 | 2737 | 0.056 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 噪声过大 | 否 | 12358.764 | 18277.514 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 噪声过大 | 否 | 1543 | 1888 | 0.079 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 噪声过大 | 否 | 1948 | 2768 | 0.498 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-chrome | 噪声过大 | 否 | 20668.658 | 23370.146 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-shell | 通过 | 否 | 12629.116 | 12629.116 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-chrome | 噪声过大 | 否 | 4884.457 | 4884.457 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 通过 | 否 | 12131.197 | 12131.197 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 通过 | 否 | 15198.843 | 15198.843 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 通过 | 否 | 30451.051 | 30451.051 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-shell | 通过 | 否 | 484.79 | 1439.729 | 7.995 | 不适用 |
| 持续压力 | 4 | puppeteer-chrome | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 持续压力 | 4 | puppeteer-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 持续压力 | 4 | shotium | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |

