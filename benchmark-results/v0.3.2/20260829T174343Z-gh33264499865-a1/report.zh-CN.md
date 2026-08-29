# Shotium 0.3.2 基准报告

结果：**complete**；质量：**fail**；证据：**complete**。配置 `smoke`，种子 `01059e287ce7`。

性能比仅以同一原生运行器、同一场景和同一并发度下的 Shotium 为基准计算。不同操作系统或处理器架构之间的绝对耗时不参与排名。

## linux-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | noisy | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | 是 | 322 | 322 | 3.106 | 4.81x |
| cold | 1 | puppeteer-chrome | pass | 是 | 580 | 580 | 1.724 | 8.66x |
| cold | 1 | puppeteer-shell | pass | 是 | 326 | 326 | 3.067 | 4.87x |
| cold | 1 | shotium | pass | 是 | 67 | 67 | 14.925 | 1.00x |
| cold | 1 | playwright-chrome | pass | 是 | 519 | 519 | 1.927 | 7.75x |
| cold-settled | 1 | playwright-shell | pass | 是 | 140.16 | 140.16 | 7.119 | 4.77x |
| cold-settled | 1 | shotium | pass | 是 | 29.38 | 29.38 | 33.634 | 1.00x |
| cold-settled | 1 | playwright-chrome | noisy | 否 | 3554.511 | 3554.511 | N/A | N/A |
| cold-settled | 1 | puppeteer-chrome | pass | 是 | 167.5 | 167.5 | 5.960 | 5.70x |
| cold-settled | 1 | puppeteer-shell | pass | 是 | 134.061 | 134.061 | 7.444 | 4.56x |
| lifecycle | 1 | shotium | pass | 是 | 64.07 | 98.876 | 13.372 | 1.00x |
| lifecycle | 1 | puppeteer-chrome | pass | 是 | 727.888 | 731.298 | 1.411 | 11.36x |
| lifecycle | 1 | playwright-shell | pass | 是 | 263.912 | 303.138 | 3.623 | 4.12x |
| lifecycle | 1 | playwright-chrome | pass | 是 | 608.625 | 637.166 | 1.732 | 9.50x |
| lifecycle | 1 | puppeteer-shell | pass | 是 | 355.626 | 442.241 | 2.647 | 5.55x |
| warm | 1 | playwright-chrome | noisy | 否 | 3382.287 | 3382.287 | N/A | N/A |
| warm | 1 | playwright-shell | pass | 是 | 115.45 | 133.168 | 8.692 | 4.51x |
| warm | 1 | puppeteer-shell | pass | 是 | 132.638 | 147.157 | 7.685 | 5.18x |
| warm | 1 | puppeteer-chrome | pass | 是 | 166.411 | 177.777 | 6.009 | 6.50x |
| warm | 1 | shotium | pass | 是 | 25.621 | 29.4 | 37.253 | 1.00x |
| batch | 1 | playwright-shell | pass | 是 | 152.277 | 212.555 | 6.219 | 4.87x |
| batch | 1 | puppeteer-chrome | pass | 是 | 183.854 | 215.827 | 5.349 | 5.88x |
| batch | 1 | puppeteer-shell | pass | 是 | 132.987 | 169.508 | 6.930 | 4.26x |
| batch | 1 | playwright-chrome | noisy | 否 | 3304.705 | 3304.705 | N/A | N/A |
| batch | 1 | shotium | pass | 是 | 31.251 | 65.409 | 24.773 | 1.00x |
| parallel | 1 | playwright-shell | pass | 是 | 130.795 | 149.604 | 7.353 | 4.50x |
| parallel | 1 | puppeteer-chrome | pass | 是 | 152.006 | 158.117 | 6.488 | 5.23x |
| parallel | 1 | shotium | pass | 是 | 29.088 | 66.289 | 25.037 | 1.00x |
| parallel | 1 | playwright-chrome | pass | 是 | 160.408 | 215.737 | 5.745 | 5.51x |
| parallel | 1 | puppeteer-shell | pass | 是 | 145.745 | 174.794 | 6.599 | 5.01x |
| parallel | 2 | puppeteer-chrome | pass | 是 | 295.585 | 301.683 | 6.463 | 3.02x |
| parallel | 2 | shotium | pass | 是 | 98.033 | 102.67 | 23.316 | 1.00x |
| parallel | 2 | playwright-chrome | pass | 是 | 310.504 | 327.319 | 6.408 | 3.17x |
| parallel | 2 | puppeteer-shell | pass | 是 | 251.397 | 253.816 | 7.486 | 2.56x |
| parallel | 2 | playwright-shell | pass | 是 | 243.632 | 255.198 | 8.183 | 2.49x |
| reuse-page | 1 | playwright-shell | pass | 否 | 68.952 | 85.189 | 14.173 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | 否 | 74.22 | 91.873 | 12.750 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | 否 | 84.317 | 95.281 | 11.411 | N/A |
| reuse-page | 1 | playwright-chrome | pass | 否 | 81.184 | 83.168 | 12.916 | N/A |
| resident | 1 | puppeteer-shell | pass | 是 | 1152 | 1152 | 0.868 | 1.91x |
| resident | 1 | puppeteer-chrome | noisy | 否 | 13821.059 | 13821.059 | N/A | N/A |
| resident | 1 | playwright-chrome | pass | 是 | 1394 | 1394 | 0.717 | 2.32x |
| resident | 1 | shotium | pass | 是 | 602 | 602 | 1.661 | 1.00x |
| resident | 1 | playwright-shell | pass | 是 | 1267 | 1267 | 0.789 | 2.10x |
| faults | 1 | puppeteer-chrome | pass | 否 | 11555.137 | 11555.137 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | 否 | 9553.92 | 9553.92 | N/A | N/A |
| faults | 1 | playwright-shell | pass | 否 | 15758.61 | 15758.61 | N/A | N/A |
| faults | 1 | shotium | pass | 否 | 10376.101 | 10376.101 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | 否 | 17829.373 | 17829.373 | N/A | N/A |
| soak | 4 | shotium | pass | 是 | 134.846 | 198.003 | 27.433 | 1.00x |
| soak | 4 | puppeteer-chrome | pass | 是 | 550.809 | 797.368 | 7.184 | 4.08x |
| soak | 4 | playwright-shell | pass | 是 | 368.133 | 510.632 | 10.337 | 2.73x |
| soak | 4 | playwright-chrome | pass | 是 | 475.225 | 584.556 | 8.442 | 3.52x |
| soak | 4 | puppeteer-shell | pass | 是 | 409.817 | 591.12 | 9.466 | 3.04x |

## linux-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | noisy | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | 是 | 471 | 471 | 2.123 | 7.03x |
| cold | 1 | shotium | pass | 是 | 67 | 67 | 14.925 | 1.00x |
| cold | 1 | playwright-shell | pass | 是 | 281 | 281 | 3.559 | 4.19x |
| cold-settled | 1 | shotium | pass | 是 | 34.25 | 34.25 | 29.007 | 1.00x |
| cold-settled | 1 | playwright-chrome | pass | 是 | 151.35 | 151.35 | 6.595 | 4.42x |
| cold-settled | 1 | playwright-shell | pass | 是 | 98.587 | 98.587 | 10.117 | 2.88x |
| lifecycle | 1 | shotium | pass | 是 | 79.849 | 99.217 | 12.170 | 1.00x |
| lifecycle | 1 | playwright-shell | pass | 是 | 254.252 | 297.781 | 3.910 | 3.18x |
| lifecycle | 1 | playwright-chrome | pass | 是 | 416.375 | 422.579 | 2.438 | 5.21x |
| warm | 1 | playwright-shell | pass | 是 | 101.202 | 117.306 | 9.473 | 3.03x |
| warm | 1 | shotium | pass | 是 | 33.443 | 34.548 | 29.438 | 1.00x |
| warm | 1 | playwright-chrome | pass | 是 | 134.612 | 150.121 | 7.288 | 4.03x |
| batch | 1 | playwright-chrome | noisy | 否 | 2949.848 | 2949.848 | N/A | N/A |
| batch | 1 | playwright-shell | pass | 是 | 123.597 | 160.583 | 7.672 | 2.61x |
| batch | 1 | shotium | pass | 是 | 47.312 | 70.772 | 19.632 | 1.00x |
| parallel | 1 | shotium | pass | 是 | 46.333 | 69.504 | 20.028 | 1.00x |
| parallel | 1 | playwright-shell | pass | 是 | 121.218 | 144.904 | 8.044 | 2.62x |
| parallel | 1 | playwright-chrome | pass | 是 | 166.454 | 171.753 | 6.342 | 3.59x |
| parallel | 2 | playwright-shell | pass | 是 | 157.714 | 211.016 | 9.703 | 1.46x |
| parallel | 2 | playwright-chrome | pass | 是 | 244.393 | 251.07 | 7.765 | 2.27x |
| parallel | 2 | shotium | pass | 是 | 107.744 | 119.891 | 19.269 | 1.00x |
| reuse-page | 1 | playwright-shell | pass | 否 | 49.85 | 50.117 | 20.001 | N/A |
| reuse-page | 1 | playwright-chrome | pass | 否 | 66.615 | 93.763 | 13.846 | N/A |
| resident | 1 | shotium | pass | 是 | 569 | 569 | 1.757 | 1.00x |
| resident | 1 | playwright-shell | pass | 是 | 1007 | 1007 | 0.993 | 1.77x |
| resident | 1 | playwright-chrome | pass | 是 | 1080 | 1080 | 0.926 | 1.90x |
| faults | 1 | playwright-shell | pass | 否 | 16294.245 | 16294.245 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | 否 | 15780.917 | 15780.917 | N/A | N/A |
| faults | 1 | shotium | pass | 否 | 10261.911 | 10261.911 | N/A | N/A |
| soak | 4 | playwright-shell | pass | 是 | 292.028 | 368.373 | 13.586 | 1.60x |
| soak | 4 | playwright-chrome | pass | 是 | 401.839 | 503.527 | 10.002 | 2.20x |
| soak | 4 | shotium | pass | 是 | 182.659 | 214.44 | 21.220 | 1.00x |

## win32-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | infra-error | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | 是 | 466 | 466 | 2.146 | 5.68x |
| cold | 1 | puppeteer-chrome | pass | 是 | 737 | 737 | 1.357 | 8.99x |
| cold | 1 | puppeteer-shell | pass | 是 | 637 | 637 | 1.570 | 7.77x |
| cold | 1 | shotium | pass | 是 | 82 | 82 | 12.195 | 1.00x |
| cold | 1 | playwright-chrome | pass | 是 | 720 | 720 | 1.389 | 8.78x |
| cold-settled | 1 | playwright-shell | noisy | 否 | 46583.62 | 46583.62 | N/A | N/A |
| cold-settled | 1 | shotium | noisy | 否 | 46807.084 | 46807.084 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | 否 | 47186.194 | 47186.194 | N/A | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | 否 | 8332.22 | 8332.22 | N/A | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | 否 | 6107.943 | 6107.943 | N/A | N/A |
| lifecycle | 1 | shotium | pass | 是 | 147.437 | 272.991 | 5.613 | 1.00x |
| lifecycle | 1 | puppeteer-chrome | noisy | 否 | 2745.746 | 7768.449 | 0.231 | N/A |
| lifecycle | 1 | playwright-shell | pass | 是 | 1032.899 | 1359.394 | 0.976 | 7.01x |
| lifecycle | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | playwright-chrome | noisy | 否 | 47279.217 | 47279.217 | N/A | N/A |
| warm | 1 | playwright-shell | noisy | 否 | 46773.643 | 46773.643 | N/A | N/A |
| warm | 1 | puppeteer-shell | noisy | 否 | 46125.229 | 46125.229 | N/A | N/A |
| warm | 1 | puppeteer-chrome | noisy | 否 | 47000.197 | 47000.197 | N/A | N/A |
| warm | 1 | shotium | noisy | 否 | 1357.238 | 1357.238 | N/A | N/A |
| batch | 1 | playwright-shell | noisy | 否 | 47004.794 | 47004.794 | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | 否 | 47064.724 | 47064.724 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | 否 | 47286.871 | 47286.871 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | 否 | 4369.206 | 4369.206 | N/A | N/A |
| batch | 1 | shotium | noisy | 否 | 45809.62 | 45809.62 | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | 否 | 46890.279 | 46890.279 | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| parallel | 1 | shotium | noisy | 否 | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-shell | noisy | 否 | 47484.789 | 47484.789 | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | 否 | 46617.689 | 46617.689 | N/A | N/A |
| parallel | 2 | shotium | noisy | 否 | 46493.968 | 46493.968 | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | 否 | 46624.198 | 46624.198 | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | 否 | 46871.24 | 46871.24 | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | 否 | 46686.922 | 46686.922 | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | 否 | 46626.562 | 46626.562 | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | 否 | 46187.34 | 46187.34 | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | 否 | 47179.53 | 47179.53 | N/A | N/A |
| reuse-page | 1 | playwright-chrome | noisy | 否 | 47159.664 | 47159.664 | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | 否 | 47624.674 | 47624.674 | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | 否 | 29750.722 | 29750.722 | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | 否 | 47888.484 | 47888.484 | N/A | N/A |
| resident | 1 | shotium | infra-error | 否 | 6515.496 | 6515.496 | N/A | N/A |
| resident | 1 | playwright-shell | noisy | 否 | 47899.93 | 47899.93 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | 否 | 6286.748 | 6286.748 | N/A | N/A |
| faults | 1 | puppeteer-shell | noisy | 否 | 5632.643 | 5632.643 | N/A | N/A |
| faults | 1 | playwright-shell | noisy | 否 | 47435.691 | 47435.691 | N/A | N/A |
| faults | 1 | shotium | noisy | 否 | 47110.041 | 47110.041 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | 否 | 47631.345 | 47631.345 | N/A | N/A |
| soak | 4 | shotium | noisy | 否 | 46214.307 | 46214.307 | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | 否 | 8165.33 | 8165.33 | N/A | N/A |
| soak | 4 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | 否 | 7937.846 | 7937.846 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |

## win32-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | 是 | 76 | 76 | 13.158 | 1.00x |
| cold-settled | 1 | shotium | noisy | 否 | 46168.317 | 46168.317 | N/A | N/A |
| lifecycle | 1 | shotium | pass | 是 | 104.772 | 136.914 | 8.829 | 1.00x |
| warm | 1 | shotium | noisy | 否 | 1873.202 | 1873.202 | N/A | N/A |
| batch | 1 | shotium | noisy | 否 | 46486.259 | 46486.259 | N/A | N/A |
| parallel | 1 | shotium | noisy | 否 | 46695.616 | 46695.616 | N/A | N/A |
| parallel | 2 | shotium | noisy | 否 | 46950.444 | 46950.444 | N/A | N/A |
| resident | 1 | shotium | noisy | 否 | 23117.552 | 23117.552 | N/A | N/A |
| faults | 1 | shotium | noisy | 否 | 46975.901 | 46975.901 | N/A | N/A |
| soak | 4 | shotium | noisy | 否 | 46432.043 | 46432.043 | N/A | N/A |

## darwin-x64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| cold | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| cold | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| cold | 1 | shotium | noisy | 否 | N/A | N/A | N/A | N/A |
| cold | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| cold-settled | 1 | playwright-shell | noisy | 否 | 7477.915 | 7477.915 | N/A | N/A |
| cold-settled | 1 | shotium | noisy | 否 | 5041.307 | 5041.307 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | 否 | 49879.092 | 49879.092 | N/A | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | 否 | 11958.017 | 11958.017 | N/A | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | 否 | 11687.414 | 11687.414 | N/A | N/A |
| lifecycle | 1 | shotium | pass | 是 | 326.472 | 713.06 | 2.406 | 1.00x |
| lifecycle | 1 | puppeteer-chrome | pass | 是 | 4295.798 | 5231.146 | 0.220 | 13.16x |
| lifecycle | 1 | playwright-shell | pass | 是 | 1339.014 | 2078.576 | 0.652 | 4.10x |
| lifecycle | 1 | playwright-chrome | pass | 是 | 5300.425 | 5334.671 | 0.205 | 16.24x |
| lifecycle | 1 | puppeteer-shell | pass | 是 | 1503.696 | 1754.042 | 0.648 | 4.61x |
| warm | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | shotium | noisy | 否 | 3447.161 | 3447.161 | N/A | N/A |
| batch | 1 | playwright-shell | noisy | 否 | 47366.907 | 47366.907 | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | 否 | 8598.249 | 8598.249 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | 否 | 46198.196 | 46198.196 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | 否 | 48280.142 | 48280.142 | N/A | N/A |
| batch | 1 | shotium | noisy | 否 | 2734.468 | 2734.468 | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | 否 | 6423.122 | 6423.122 | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | 否 | 8544.275 | 8544.275 | N/A | N/A |
| parallel | 1 | shotium | noisy | 否 | 45989.251 | 45989.251 | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | 否 | 48025.742 | 48025.742 | N/A | N/A |
| parallel | 1 | puppeteer-shell | noisy | 否 | 46965.068 | 46965.068 | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | 否 | 9343.895 | 9343.895 | N/A | N/A |
| parallel | 2 | shotium | noisy | 否 | 3140.291 | 3140.291 | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | 否 | 48153.381 | 48153.381 | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | 否 | 46736.067 | 46736.067 | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | 否 | 5535.844 | 5535.844 | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | 否 | 40431.105 | 40431.105 | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | 否 | 36738.756 | 36738.756 | N/A | N/A |
| resident | 1 | playwright-chrome | fail | 否 | 50076.56 | 50076.56 | N/A | N/A |
| resident | 1 | shotium | noisy | 否 | 25733.225 | 25733.225 | N/A | N/A |
| resident | 1 | playwright-shell | fail | 否 | 47418.403 | 47418.403 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| faults | 1 | shotium | noisy | 否 | 45521.705 | 45521.705 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | 否 | 49137.61 | 49137.61 | N/A | N/A |
| soak | 4 | shotium | noisy | 否 | 3764.19 | 3764.19 | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | 否 | 9321.07 | 9321.07 | N/A | N/A |
| soak | 4 | playwright-shell | noisy | 否 | 7742.904 | 7742.904 | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | 否 | 49473.614 | 49473.614 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | 否 | 46504.434 | 46504.434 | N/A | N/A |

## darwin-arm64

场景分组在不同的原生运行器上执行；每个引擎对比仍严格限定在同一分片、同一运行器内。

| 引擎 | 可用性 | 原因 / 二进制架构 |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| cold | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| cold | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| cold | 1 | shotium | noisy | 否 | N/A | N/A | N/A | N/A |
| cold | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| cold-settled | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| cold-settled | 1 | shotium | noisy | 否 | N/A | N/A | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| lifecycle | 1 | shotium | pass | 是 | 130.956 | 213.179 | 7.085 | 1.00x |
| lifecycle | 1 | puppeteer-chrome | pass | 是 | 4095.133 | 7863.627 | 0.212 | 31.27x |
| lifecycle | 1 | playwright-shell | pass | 是 | 602.799 | 1052.446 | 1.359 | 4.60x |
| lifecycle | 1 | playwright-chrome | pass | 是 | 3181.979 | 5529.558 | 0.277 | 24.30x |
| lifecycle | 1 | puppeteer-shell | pass | 是 | 714.261 | 1324.338 | 1.197 | 5.45x |
| warm | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| warm | 1 | shotium | noisy | 否 | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | 否 | 13112.364 | 13112.364 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | 否 | 4003.112 | 4003.112 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | 否 | 48041.714 | 48041.714 | N/A | N/A |
| batch | 1 | shotium | noisy | 否 | 45607.231 | 45607.231 | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | 否 | 46325.816 | 46325.816 | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | 否 | 9774.233 | 9774.233 | N/A | N/A |
| parallel | 1 | shotium | noisy | 否 | 2085.288 | 2085.288 | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | 否 | 48841.429 | 48841.429 | N/A | N/A |
| parallel | 1 | puppeteer-shell | noisy | 否 | 46659.014 | 46659.014 | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | 否 | 7657.82 | 7657.82 | N/A | N/A |
| parallel | 2 | shotium | noisy | 否 | 2125.842 | 2125.842 | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | 否 | 47870.754 | 47870.754 | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | 否 | 46594.842 | 46594.842 | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | 否 | 46848.1 | 46848.1 | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | 否 | 23213.126 | 23213.126 | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | 否 | 30811.697 | 30811.697 | N/A | N/A |
| resident | 1 | shotium | noisy | 否 | 45754.999 | 45754.999 | N/A | N/A |
| resident | 1 | playwright-shell | noisy | 否 | 45858.943 | 45858.943 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | 否 | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-shell | noisy | 否 | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-shell | noisy | 否 | 2822.909 | 2822.909 | N/A | N/A |
| faults | 1 | shotium | noisy | 否 | 1611.12 | 1611.12 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | 否 | 48005.824 | 48005.824 | N/A | N/A |
| soak | 4 | shotium | noisy | 否 | 45592.615 | 45592.615 | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | 否 | 6540.631 | 6540.631 | N/A | N/A |
| soak | 4 | playwright-shell | noisy | 否 | 3299.195 | 3299.195 | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | 否 | 47503.024 | 47503.024 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | 否 | 46100.187 | 46100.187 | N/A | N/A |

