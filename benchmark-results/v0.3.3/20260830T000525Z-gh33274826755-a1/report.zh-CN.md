# Shotium 0.3.3 基准报告

[交互式基准站点](https://sj817.github.io/shotium/)

结果：**不完整**；质量：**失败**；证据：**不完整**。配置：**完整测试**；种子：`bac114e9f8c57fb04875ee8ceec7e8e7c11905d4`。

结论：本次结果不完整。0 个平台形成了有效的平台内对比；缺失结果不会被推测或补齐。

每个比率只在 Shotium 与对比引擎位于同一平台、同一场景、同一并发度，且双方状态均为“通过”并允许排名时计算。本报告不进行跨平台混排。

## 六平台总览

| 平台 | 质量状态 | 正式第一名 | 正式参赛引擎数 | 平台可比项数 |
|:--|:--|:--|--:|--:|
| linux-x64 | 失败 | 无有效排名 | 0 | 10 |
| linux-arm64 | 噪声过大 | 无有效排名 | 0 | 10 |
| win32-x64 | 基础设施错误 | 无有效排名 | 0 | 1 |
| win32-arm64 | 噪声过大 | 无有效排名 | 0 | 0 |
| darwin-x64 | 失败 | 无有效排名 | 0 | 1 |
| darwin-arm64 | 噪声过大 | 无有效排名 | 0 | 1 |

## linux-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“失败”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 通过 | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | 噪声过大 | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | 失败 | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | 通过 | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | 噪声过大 | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | shotium | 通过 | 否 | 53 | 54 | 18.919 | 不适用 |
| 冷启动 | 1 | playwright-chrome | 通过 | 否 | 410 | 464 | 2.396 | 不适用 |
| 冷启动 | 1 | puppeteer-shell | 通过 | 否 | 282 | 375 | 3.235 | 不适用 |
| 冷启动 | 1 | puppeteer-chrome | 通过 | 否 | 471 | 642 | 2.044 | 不适用 |
| 冷启动 | 1 | playwright-shell | 通过 | 否 | 256 | 281 | 3.857 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-shell | 通过 | 否 | 104.061 | 124.113 | 9.287 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-chrome | 通过 | 否 | 114.146 | 142.731 | 8.569 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 通过 | 否 | 118.888 | 170.457 | 7.536 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 通过 | 否 | 21.028 | 21.967 | 46.435 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 通过 | 否 | 95.619 | 105.772 | 10.494 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 53.989 | 80.148 | 16.839 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-chrome | 通过 | 否 | 565.139 | 1648.594 | 1.548 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-shell | 通过 | 否 | 289.344 | 409.664 | 3.402 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 262.976 | 301.556 | 3.901 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 488.753 | 723.828 | 1.978 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 噪声过大 | 否 | 145.381 | 179.195 | 2.815 | 不适用 |
| 预热截图 | 1 | shotium | 通过 | 否 | 24.905 | 29.44 | 38.201 | 不适用 |
| 预热截图 | 1 | puppeteer-chrome | 通过 | 否 | 157.438 | 201.988 | 6.217 | 不适用 |
| 预热截图 | 1 | playwright-shell | 通过 | 否 | 122.864 | 149.809 | 8.026 | 不适用 |
| 预热截图 | 1 | puppeteer-shell | 通过 | 否 | 131.601 | 157.137 | 7.542 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 通过 | 否 | 138.668 | 360.009 | 6.215 | 不适用 |
| 顺序批量 | 1 | puppeteer-shell | 通过 | 否 | 147.616 | 349.613 | 5.845 | 不适用 |
| 顺序批量 | 1 | shotium | 通过 | 否 | 32.151 | 267.234 | 18.545 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 噪声过大 | 否 | 165.397 | 374.801 | 4.373 | 不适用 |
| 顺序批量 | 1 | puppeteer-chrome | 通过 | 否 | 187.216 | 401.322 | 4.816 | 不适用 |
| 并发截图 | 1 | playwright-shell | 通过 | 否 | 107.387 | 1160.162 | 6.843 | 不适用 |
| 并发截图 | 1 | playwright-chrome | 通过 | 否 | 137.004 | 358.306 | 6.273 | 不适用 |
| 并发截图 | 1 | puppeteer-chrome | 通过 | 否 | 149.739 | 375.189 | 5.844 | 不适用 |
| 并发截图 | 1 | puppeteer-shell | 通过 | 否 | 120.234 | 330.217 | 6.806 | 不适用 |
| 并发截图 | 1 | shotium | 通过 | 否 | 29.44 | 258.629 | 20.560 | 不适用 |
| 并发截图 | 2 | puppeteer-chrome | 通过 | 否 | 255.832 | 427.23 | 6.888 | 不适用 |
| 并发截图 | 2 | puppeteer-shell | 通过 | 否 | 214.92 | 624.344 | 7.957 | 不适用 |
| 并发截图 | 2 | shotium | 通过 | 否 | 59.133 | 290.96 | 20.488 | 不适用 |
| 并发截图 | 2 | playwright-shell | 通过 | 否 | 188.534 | 348.728 | 8.873 | 不适用 |
| 并发截图 | 2 | playwright-chrome | 通过 | 否 | 238.892 | 588.97 | 7.556 | 不适用 |
| 并发截图 | 4 | shotium | 通过 | 否 | 128.032 | 347.711 | 20.665 | 不适用 |
| 并发截图 | 4 | playwright-shell | 通过 | 否 | 355.746 | 600.237 | 9.743 | 不适用 |
| 并发截图 | 4 | playwright-chrome | 通过 | 否 | 406.883 | 595.639 | 8.416 | 不适用 |
| 并发截图 | 4 | puppeteer-chrome | 失败 | 否 | 472.409 | 920.936 | 7.578 | 不适用 |
| 并发截图 | 4 | puppeteer-shell | 通过 | 否 | 391.442 | 609.159 | 9.002 | 不适用 |
| 复用页面 | 1 | playwright-shell | 通过 | 否 | 65.7 | 102.291 | 15.320 | 不适用 |
| 复用页面 | 1 | puppeteer-shell | 通过 | 否 | 66.998 | 86.19 | 14.172 | 不适用 |
| 复用页面 | 1 | puppeteer-chrome | 通过 | 否 | 82.989 | 124.292 | 11.755 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 通过 | 否 | 68.45 | 96.443 | 13.499 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 通过 | 否 | 455 | 465 | 2.222 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-chrome | 噪声过大 | 否 | 15305.456 | 15810.345 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 通过 | 否 | 971 | 1042 | 1.038 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-shell | 噪声过大 | 否 | 767.5 | 825 | 0.094 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 通过 | 否 | 1061 | 1095 | 0.957 | 不适用 |
| 异常与恢复 | 1 | puppeteer-shell | 通过 | 否 | 10025.711 | 10025.711 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-chrome | 通过 | 否 | 13297.764 | 13297.764 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 通过 | 否 | 16319.469 | 16319.469 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 通过 | 否 | 17197.933 | 17197.933 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 通过 | 否 | 10491.485 | 10491.485 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 噪声过大 | 否 | 3332.398 | 3332.398 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-shell | 通过 | 否 | 403.487 | 885.848 | 9.084 | 不适用 |
| 持续压力 | 4 | puppeteer-shell | 通过 | 否 | 460.5 | 856.388 | 8.358 | 不适用 |
| 持续压力 | 4 | shotium | 通过 | 否 | 165.14 | 387.537 | 19.653 | 不适用 |
| 持续压力 | 4 | puppeteer-chrome | 基础设施错误 | 否 | 591.049 | 180896.151 | 1.404 | 不适用 |

## linux-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“噪声过大”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 通过 | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | 不适用 | 启动场景：该软件包没有适用于此平台架构的原生浏览器；吞吐场景：该软件包没有适用于此平台架构的原生浏览器；并发场景：该软件包没有适用于此平台架构的原生浏览器；常驻场景：该软件包没有适用于此平台架构的原生浏览器；韧性场景：该软件包没有适用于此平台架构的原生浏览器 |
| puppeteer-chrome | 不适用 | 启动场景：该软件包没有适用于此平台架构的原生浏览器；吞吐场景：该软件包没有适用于此平台架构的原生浏览器；并发场景：该软件包没有适用于此平台架构的原生浏览器；常驻场景：该软件包没有适用于此平台架构的原生浏览器；韧性场景：该软件包没有适用于此平台架构的原生浏览器 |
| playwright-shell | 噪声过大 | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | 噪声过大 | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | playwright-shell | 通过 | 否 | 230 | 238 | 4.345 | 不适用 |
| 冷启动 | 1 | shotium | 通过 | 否 | 63 | 67 | 15.730 | 不适用 |
| 冷启动 | 1 | playwright-chrome | 通过 | 否 | 406 | 1069 | 2.020 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 噪声过大 | 否 | 120.816 | 132.088 | 1.845 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 通过 | 否 | 34.088 | 34.76 | 29.329 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 通过 | 否 | 110.935 | 123.177 | 9.125 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 73.365 | 101.532 | 12.932 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 238.236 | 312.762 | 4.085 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 418.73 | 494.885 | 2.362 | 不适用 |
| 预热截图 | 1 | shotium | 通过 | 否 | 32.049 | 34.055 | 30.984 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 通过 | 否 | 117.495 | 158.248 | 8.267 | 不适用 |
| 预热截图 | 1 | playwright-shell | 通过 | 否 | 103.611 | 132.446 | 9.290 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 通过 | 否 | 124.002 | 323.849 | 7.079 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 通过 | 否 | 142.813 | 356.542 | 6.137 | 不适用 |
| 顺序批量 | 1 | shotium | 通过 | 否 | 42.861 | 260.261 | 16.716 | 不适用 |
| 并发截图 | 1 | playwright-chrome | 通过 | 否 | 152.331 | 356.975 | 5.805 | 不适用 |
| 并发截图 | 1 | playwright-shell | 通过 | 否 | 126.921 | 348.32 | 6.861 | 不适用 |
| 并发截图 | 1 | shotium | 通过 | 否 | 44.16 | 262.453 | 16.439 | 不适用 |
| 并发截图 | 2 | playwright-shell | 通过 | 否 | 184.844 | 343.981 | 8.918 | 不适用 |
| 并发截图 | 2 | shotium | 通过 | 否 | 90.209 | 295.828 | 16.317 | 不适用 |
| 并发截图 | 2 | playwright-chrome | 通过 | 否 | 251.896 | 413.124 | 7.179 | 不适用 |
| 并发截图 | 4 | shotium | 通过 | 否 | 184.187 | 379.566 | 16.228 | 不适用 |
| 并发截图 | 4 | playwright-chrome | 通过 | 否 | 438.408 | 608.686 | 8.080 | 不适用 |
| 并发截图 | 4 | playwright-shell | 通过 | 否 | 343.492 | 544.654 | 9.785 | 不适用 |
| 复用页面 | 1 | playwright-shell | 通过 | 否 | 50.118 | 60.441 | 19.224 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 通过 | 否 | 58.17 | 317.889 | 12.928 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 噪声过大 | 否 | 715.5 | 884 | 0.369 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 通过 | 否 | 629 | 876 | 1.542 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 通过 | 否 | 92 | 249 | 8.526 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 通过 | 否 | 15569.272 | 15569.272 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 通过 | 否 | 10383.181 | 10383.181 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 通过 | 否 | 14505.632 | 14505.632 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-shell | 通过 | 否 | 308.776 | 597.304 | 11.949 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 通过 | 否 | 417.275 | 753.151 | 9.330 | 不适用 |
| 持续压力 | 4 | shotium | 通过 | 否 | 200.7 | 451.49 | 16.216 | 不适用 |

## win32-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“基础设施错误”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 噪声过大 | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | 噪声过大 | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | 噪声过大 | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | 噪声过大 | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | 噪声过大 | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | shotium | 噪声过大 | 否 | 84 | 99 | 11.628 | 不适用 |
| 冷启动 | 1 | playwright-chrome | 通过 | 否 | 701 | 821 | 1.383 | 不适用 |
| 冷启动 | 1 | puppeteer-shell | 通过 | 否 | 560 | 605 | 1.751 | 不适用 |
| 冷启动 | 1 | puppeteer-chrome | 噪声过大 | 否 | 794.5 | 815 | 1.263 | 不适用 |
| 冷启动 | 1 | playwright-shell | 噪声过大 | 否 | 397 | 423 | 2.524 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-shell | 噪声过大 | 否 | 46420.41 | 47329.032 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-chrome | 噪声过大 | 否 | 6183.054 | 47633.995 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 噪声过大 | 否 | 46821.762 | 47631.277 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 噪声过大 | 否 | 45767.839 | 46756.035 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 噪声过大 | 否 | 46789.648 | 47227.197 | 不适用 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 112.285 | 238.905 | 8.263 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-chrome | 通过 | 否 | 1904.429 | 2179.802 | 0.534 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-shell | 通过 | 否 | 963.584 | 1077.043 | 1.032 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 607.973 | 759.499 | 1.634 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 1275.404 | 1419.992 | 0.777 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 噪声过大 | 否 | 47053.403 | 47438.84 | 不适用 | 不适用 |
| 预热截图 | 1 | shotium | 噪声过大 | 否 | 46803.022 | 47044.192 | 不适用 | 不适用 |
| 预热截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 6236.182 | 47466.327 | 不适用 | 不适用 |
| 预热截图 | 1 | playwright-shell | 噪声过大 | 否 | 46779.769 | 47023.534 | 不适用 | 不适用 |
| 预热截图 | 1 | puppeteer-shell | 噪声过大 | 否 | 46625.748 | 47179.071 | 不适用 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 噪声过大 | 否 | 46600.013 | 46744.045 | 不适用 | 不适用 |
| 顺序批量 | 1 | puppeteer-shell | 噪声过大 | 否 | 47137.276 | 47557.484 | 不适用 | 不适用 |
| 顺序批量 | 1 | shotium | 噪声过大 | 否 | 46709.656 | 47046.573 | 不适用 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 噪声过大 | 否 | 5836.161 | 47849.154 | 不适用 | 不适用 |
| 顺序批量 | 1 | puppeteer-chrome | 噪声过大 | 否 | 5841.33 | 47693.235 | 不适用 | 不适用 |
| 复用页面 | 1 | playwright-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | puppeteer-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | puppeteer-chrome | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 噪声过大 | 否 | 46756.185 | 46756.185 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 噪声过大 | 否 | 22673.875 | 46230.858 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-chrome | 噪声过大 | 否 | 25002.277 | 29852.297 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 噪声过大 | 否 | 47052.666 | 47378.883 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-shell | 噪声过大 | 否 | 46932.998 | 47286.106 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 噪声过大 | 否 | 47856.224 | 48019.072 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-shell | 噪声过大 | 否 | 5315.461 | 5315.461 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-chrome | 噪声过大 | 否 | 6092.129 | 6092.129 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 噪声过大 | 否 | 47240.803 | 47240.803 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 噪声过大 | 否 | 47674.012 | 47674.012 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 噪声过大 | 否 | 46275.446 | 46275.446 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 噪声过大 | 否 | 6495.607 | 6495.607 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-shell | 噪声过大 | 否 | 47337.462 | 47337.462 | 不适用 | 不适用 |
| 持续压力 | 4 | puppeteer-shell | 噪声过大 | 否 | 47154.786 | 47154.786 | 不适用 | 不适用 |
| 持续压力 | 4 | shotium | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 持续压力 | 4 | puppeteer-chrome | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |

## win32-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“噪声过大”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 噪声过大 | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | 不适用 | 启动场景：该软件包没有适用于此平台架构的原生浏览器；吞吐场景：该软件包没有适用于此平台架构的原生浏览器；并发场景：该软件包没有适用于此平台架构的原生浏览器；常驻场景：该软件包没有适用于此平台架构的原生浏览器；韧性场景：该软件包没有适用于此平台架构的原生浏览器 |
| puppeteer-chrome | 不适用 | 启动场景：该软件包没有适用于此平台架构的原生浏览器；吞吐场景：该软件包没有适用于此平台架构的原生浏览器；并发场景：该软件包没有适用于此平台架构的原生浏览器；常驻场景：该软件包没有适用于此平台架构的原生浏览器；韧性场景：该软件包没有适用于此平台架构的原生浏览器 |
| playwright-shell | 不适用 | 启动场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器；吞吐场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器；并发场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器；常驻场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器；韧性场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器 |
| playwright-chrome | 不适用 | 启动场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器；吞吐场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器；并发场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器；常驻场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器；韧性场景：该软件包目前在 Windows arm64 上仅提供 x64 浏览器 |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | shotium | 通过 | 否 | 79 | 82 | 12.797 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 噪声过大 | 否 | 46055.255 | 46304.156 | 不适用 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 99.682 | 133.972 | 9.470 | 不适用 |
| 预热截图 | 1 | shotium | 噪声过大 | 否 | 46609.837 | 47297.699 | 不适用 | 不适用 |
| 顺序批量 | 1 | shotium | 噪声过大 | 否 | 46673.108 | 47100.177 | 不适用 | 不适用 |
| 并发截图 | 1 | shotium | 噪声过大 | 否 | 46832.949 | 47015.363 | 不适用 | 不适用 |
| 并发截图 | 2 | shotium | 噪声过大 | 否 | 46566.402 | 46947.796 | 不适用 | 不适用 |
| 并发截图 | 4 | shotium | 噪声过大 | 否 | 46371.081 | 46748.461 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 噪声过大 | 否 | 23468.928 | 47004.738 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 噪声过大 | 否 | 45919.856 | 45919.856 | 不适用 | 不适用 |
| 持续压力 | 4 | shotium | 噪声过大 | 否 | 45996.694 | 45996.694 | 不适用 | 不适用 |

## darwin-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“失败”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 噪声过大 | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | 基础设施错误 | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | 失败 | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | 失败 | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | 失败 | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | shotium | 噪声过大 | 否 | 129.5 | 626 | 4.800 | 不适用 |
| 冷启动 | 1 | playwright-chrome | 噪声过大 | 否 | 3532 | 13982 | 0.193 | 不适用 |
| 冷启动 | 1 | puppeteer-shell | 噪声过大 | 否 | 1122.5 | 2219 | 0.736 | 不适用 |
| 冷启动 | 1 | puppeteer-chrome | 噪声过大 | 否 | 2859.5 | 5674 | 0.306 | 不适用 |
| 冷启动 | 1 | playwright-shell | 噪声过大 | 否 | 920 | 2236 | 0.872 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-shell | 噪声过大 | 否 | 46899.839 | 47513.046 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-chrome | 噪声过大 | 否 | 9570.887 | 12575.737 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 噪声过大 | 否 | 49334.307 | 50415.856 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 噪声过大 | 否 | 3596.295 | 45570.794 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 噪声过大 | 否 | 6582.852 | 47010.516 | 不适用 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 193.911 | 446.878 | 4.655 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-chrome | 通过 | 否 | 3579.117 | 5841.001 | 0.266 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-shell | 通过 | 否 | 1366.161 | 1681.9 | 0.744 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 1052.71 | 1397.548 | 0.928 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 3232.154 | 3828.663 | 0.307 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 噪声过大 | 否 | 48731.132 | 49620.889 | 不适用 | 不适用 |
| 预热截图 | 1 | shotium | 噪声过大 | 否 | 3708.278 | 3998.977 | 不适用 | 不适用 |
| 预热截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 10014.418 | 10825.817 | 不适用 | 不适用 |
| 预热截图 | 1 | playwright-shell | 噪声过大 | 否 | 9233.235 | 47174.626 | 不适用 | 不适用 |
| 预热截图 | 1 | puppeteer-shell | 噪声过大 | 否 | 46903.63 | 47571.156 | 不适用 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 噪声过大 | 否 | 7645.495 | 11860.297 | 不适用 | 不适用 |
| 顺序批量 | 1 | puppeteer-shell | 噪声过大 | 否 | 46734.746 | 47742.664 | 不适用 | 不适用 |
| 顺序批量 | 1 | shotium | 噪声过大 | 否 | 45375.223 | 46300.017 | 不适用 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 噪声过大 | 否 | 49042.518 | 50736.82 | 不适用 | 不适用 |
| 顺序批量 | 1 | puppeteer-chrome | 噪声过大 | 否 | 10940.815 | 17139.131 | 不适用 | 不适用 |
| 并发截图 | 1 | playwright-shell | 噪声过大 | 否 | 8920.629 | 46805.434 | 不适用 | 不适用 |
| 并发截图 | 1 | playwright-chrome | 噪声过大 | 否 | 50293.024 | 50887.955 | 不适用 | 不适用 |
| 并发截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 11244.632 | 13692.273 | 不适用 | 不适用 |
| 并发截图 | 1 | puppeteer-shell | 噪声过大 | 否 | 47072.241 | 47880.038 | 不适用 | 不适用 |
| 并发截图 | 1 | shotium | 噪声过大 | 否 | 4540.187 | 46130.881 | 不适用 | 不适用 |
| 并发截图 | 2 | puppeteer-chrome | 噪声过大 | 否 | 10705.087 | 12844.592 | 不适用 | 不适用 |
| 并发截图 | 2 | puppeteer-shell | 噪声过大 | 否 | 47030.523 | 47408.321 | 不适用 | 不适用 |
| 并发截图 | 2 | shotium | 噪声过大 | 否 | 3726.215 | 45623.119 | 不适用 | 不适用 |
| 并发截图 | 2 | playwright-shell | 噪声过大 | 否 | 7762.797 | 47344.336 | 不适用 | 不适用 |
| 并发截图 | 2 | playwright-chrome | 噪声过大 | 否 | 49614.134 | 50104.739 | 不适用 | 不适用 |
| 并发截图 | 4 | shotium | 噪声过大 | 否 | 3703.045 | 45481.291 | 不适用 | 不适用 |
| 并发截图 | 4 | playwright-shell | 噪声过大 | 否 | 7823.593 | 10007.359 | 不适用 | 不适用 |
| 并发截图 | 4 | playwright-chrome | 噪声过大 | 否 | 48968.439 | 49792.317 | 不适用 | 不适用 |
| 并发截图 | 4 | puppeteer-chrome | 噪声过大 | 否 | 10432.539 | 12120.893 | 不适用 | 不适用 |
| 并发截图 | 4 | puppeteer-shell | 噪声过大 | 否 | 46916.093 | 48256.237 | 不适用 | 不适用 |
| 复用页面 | 1 | playwright-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | puppeteer-shell | 噪声过大 | 否 | 47405.64 | 47405.64 | 不适用 | 不适用 |
| 复用页面 | 1 | puppeteer-chrome | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 噪声过大 | 否 | 8017.343 | 8017.343 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 噪声过大 | 否 | 21583.207 | 30510.351 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-chrome | 失败 | 否 | 48620.149 | 50216.508 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 失败 | 否 | 47264.169 | 48082.726 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-shell | 基础设施错误 | 否 | 47209.772 | 57526.101 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 失败 | 否 | 49222.586 | 50003.737 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-chrome | 噪声过大 | 否 | 13479.421 | 13479.421 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 噪声过大 | 否 | 49583.068 | 49583.068 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 噪声过大 | 否 | 3004.617 | 3004.617 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 噪声过大 | 否 | 13703.034 | 13703.034 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-shell | 噪声过大 | 否 | 6462.818 | 6462.818 | 不适用 | 不适用 |
| 持续压力 | 4 | puppeteer-shell | 噪声过大 | 否 | 47202.558 | 47202.558 | 不适用 | 不适用 |
| 持续压力 | 4 | shotium | 噪声过大 | 否 | 3081.477 | 3081.477 | 不适用 | 不适用 |
| 持续压力 | 4 | puppeteer-chrome | 噪声过大 | 否 | 8556.743 | 8556.743 | 不适用 | 不适用 |

## darwin-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

### 平台内综合排名

结论：本平台质量状态为“噪声过大”；测量数据仅保留用于诊断，不生成正式排名或第一名。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | 噪声过大 | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | 噪声过大 | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | 噪声过大 | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | 噪声过大 | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | 噪声过大 | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| 冷启动 | 1 | shotium | 噪声过大 | 否 | 95 | 232 | 9.160 | 不适用 |
| 冷启动 | 1 | playwright-chrome | 噪声过大 | 否 | 1959.5 | 3793 | 0.439 | 不适用 |
| 冷启动 | 1 | puppeteer-shell | 通过 | 否 | 494 | 911 | 1.739 | 不适用 |
| 冷启动 | 1 | puppeteer-chrome | 通过 | 否 | 1722 | 4268 | 0.509 | 不适用 |
| 冷启动 | 1 | playwright-shell | 通过 | 否 | 399 | 744 | 2.155 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-shell | 噪声过大 | 否 | 46234.354 | 46403.629 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | puppeteer-chrome | 噪声过大 | 否 | 5298.085 | 6191.569 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-chrome | 噪声过大 | 否 | 47285.483 | 48086.451 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | shotium | 噪声过大 | 否 | 1421.053 | 45537.935 | 不适用 | 不适用 |
| 冷启动稳定后首张 | 1 | playwright-shell | 噪声过大 | 否 | 46171.791 | 46573.498 | 不适用 | 不适用 |
| 启动—截图—关闭循环 | 1 | shotium | 通过 | 否 | 98.453 | 166.222 | 9.790 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-chrome | 通过 | 否 | 1467.059 | 2206.518 | 0.649 | 不适用 |
| 启动—截图—关闭循环 | 1 | puppeteer-shell | 通过 | 否 | 470.985 | 821.312 | 2.011 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-shell | 通过 | 否 | 338.848 | 680.385 | 2.723 | 不适用 |
| 启动—截图—关闭循环 | 1 | playwright-chrome | 通过 | 否 | 1523.995 | 1986.194 | 0.666 | 不适用 |
| 预热截图 | 1 | playwright-chrome | 噪声过大 | 否 | 47191.451 | 47908.616 | 不适用 | 不适用 |
| 预热截图 | 1 | shotium | 噪声过大 | 否 | 1391.217 | 45228.503 | 不适用 | 不适用 |
| 预热截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 4941.614 | 10390.773 | 不适用 | 不适用 |
| 预热截图 | 1 | playwright-shell | 噪声过大 | 否 | 45890.194 | 46775.542 | 不适用 | 不适用 |
| 预热截图 | 1 | puppeteer-shell | 噪声过大 | 否 | 46140.407 | 46446.384 | 不适用 | 不适用 |
| 顺序批量 | 1 | playwright-shell | 噪声过大 | 否 | 46169.11 | 46564.594 | 不适用 | 不适用 |
| 顺序批量 | 1 | puppeteer-shell | 噪声过大 | 否 | 45993.615 | 46589.433 | 不适用 | 不适用 |
| 顺序批量 | 1 | shotium | 噪声过大 | 否 | 1316.909 | 1734.069 | 不适用 | 不适用 |
| 顺序批量 | 1 | playwright-chrome | 噪声过大 | 否 | 47150.353 | 47384.527 | 不适用 | 不适用 |
| 顺序批量 | 1 | puppeteer-chrome | 噪声过大 | 否 | 4904.649 | 6078.489 | 不适用 | 不适用 |
| 并发截图 | 1 | playwright-shell | 噪声过大 | 否 | 46297.956 | 46737.015 | 不适用 | 不适用 |
| 并发截图 | 1 | playwright-chrome | 噪声过大 | 否 | 47957.471 | 48448.498 | 不适用 | 不适用 |
| 并发截图 | 1 | puppeteer-chrome | 噪声过大 | 否 | 6957.269 | 7477.368 | 不适用 | 不适用 |
| 并发截图 | 1 | puppeteer-shell | 噪声过大 | 否 | 4135.02 | 46420.379 | 不适用 | 不适用 |
| 并发截图 | 1 | shotium | 噪声过大 | 否 | 1711.497 | 45897.569 | 不适用 | 不适用 |
| 并发截图 | 2 | puppeteer-chrome | 噪声过大 | 否 | 5424.86 | 7405.299 | 不适用 | 不适用 |
| 并发截图 | 2 | puppeteer-shell | 噪声过大 | 否 | 45799.001 | 46430.29 | 不适用 | 不适用 |
| 并发截图 | 2 | shotium | 噪声过大 | 否 | 2743.837 | 45723.436 | 不适用 | 不适用 |
| 并发截图 | 2 | playwright-shell | 噪声过大 | 否 | 3510.896 | 46451.385 | 不适用 | 不适用 |
| 并发截图 | 2 | playwright-chrome | 噪声过大 | 否 | 47378.786 | 47856.211 | 不适用 | 不适用 |
| 并发截图 | 4 | shotium | 噪声过大 | 否 | 1583.639 | 45788.124 | 不适用 | 不适用 |
| 并发截图 | 4 | playwright-shell | 噪声过大 | 否 | 3811.891 | 46888.599 | 不适用 | 不适用 |
| 并发截图 | 4 | playwright-chrome | 噪声过大 | 否 | 47125.247 | 48163.637 | 不适用 | 不适用 |
| 并发截图 | 4 | puppeteer-chrome | 噪声过大 | 否 | 5174.097 | 7731.804 | 不适用 | 不适用 |
| 并发截图 | 4 | puppeteer-shell | 噪声过大 | 否 | 46127.759 | 46522.798 | 不适用 | 不适用 |
| 复用页面 | 1 | playwright-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | puppeteer-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 复用页面 | 1 | puppeteer-chrome | 噪声过大 | 否 | 5442.441 | 5442.441 | 不适用 | 不适用 |
| 复用页面 | 1 | playwright-chrome | 噪声过大 | 否 | 48168.137 | 48168.137 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | shotium | 噪声过大 | 否 | 7354.842 | 10011.425 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-chrome | 噪声过大 | 否 | 15034.016 | 21954.708 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-shell | 噪声过大 | 否 | 46336.067 | 46835.282 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | puppeteer-shell | 噪声过大 | 否 | 16474.441 | 46462.682 | 不适用 | 不适用 |
| 常驻引擎新客户端 | 1 | playwright-chrome | 噪声过大 | 否 | 22686.729 | 48766.872 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-shell | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 异常与恢复 | 1 | puppeteer-chrome | 噪声过大 | 否 | 不适用 | 不适用 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-shell | 噪声过大 | 否 | 46608.46 | 46608.46 | 不适用 | 不适用 |
| 异常与恢复 | 1 | playwright-chrome | 噪声过大 | 否 | 48556.105 | 48556.105 | 不适用 | 不适用 |
| 异常与恢复 | 1 | shotium | 噪声过大 | 否 | 2621.155 | 2621.155 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-chrome | 噪声过大 | 否 | 47620.858 | 47620.858 | 不适用 | 不适用 |
| 持续压力 | 4 | playwright-shell | 噪声过大 | 否 | 3634.7 | 3634.7 | 不适用 | 不适用 |
| 持续压力 | 4 | puppeteer-shell | 噪声过大 | 否 | 4006.723 | 4006.723 | 不适用 | 不适用 |
| 持续压力 | 4 | shotium | 噪声过大 | 否 | 45297.893 | 45297.893 | 不适用 | 不适用 |
| 持续压力 | 4 | puppeteer-chrome | 噪声过大 | 否 | 5455.256 | 5455.256 | 不适用 | 不适用 |

