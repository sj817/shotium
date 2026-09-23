# Shotium 0.12.1 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `f8dc420663e672c2892588f73f450fd805b53970`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | pass | shotium | 3 | 10 |
| win32-arm64 | pass | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 4 | 9 |
| darwin-arm64 | noisy | shotium | 4 | 7 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 8.340× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 8.931× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 11.891× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 10.553× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.12.1/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 811 | 827 | 1.235 | 10.96× |
| cold | 1 | shotium | pass | yes | 74 | 87 | 12.939 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 924 | 937 | 1.082 | 12.49× |
| cold | 1 | puppeteer-shell | pass | yes | 652 | 674 | 1.525 | 8.81× |
| cold | 1 | playwright-chrome | pass | yes | 1003 | 1032 | 0.998 | 13.55× |
| cold-settled | 1 | playwright-chrome | pass | yes | 165.58 | 200.772 | 5.692 | 13.09× |
| cold-settled | 1 | shotium | pass | yes | 12.651 | 15.216 | 75.298 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 187.369 | 194.652 | 5.335 | 14.81× |
| cold-settled | 1 | playwright-shell | pass | yes | 153.659 | 166.995 | 6.481 | 12.15× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 140.187 | 146.383 | 7.416 | 11.08× |
| lifecycle | 1 | playwright-chrome | fail | no | 2136.777 | 2226.878 | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 678.822 | 733.066 | 1.487 | 9.27× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 967.24 | 1038.813 | 1.027 | 13.21× |
| lifecycle | 1 | shotium | pass | yes | 73.235 | 119.249 | 12.499 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 702.823 | 769.805 | 1.410 | 9.60× |
| warm | 1 | puppeteer-chrome | pass | yes | 198.005 | 221.277 | 5.158 | 22.42× |
| warm | 1 | playwright-shell | pass | yes | 147.437 | 157.566 | 6.939 | 16.70× |
| warm | 1 | shotium | pass | yes | 8.83 | 14.229 | 95.660 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 179.654 | 213.302 | 5.562 | 20.35× |
| warm | 1 | puppeteer-shell | pass | yes | 133.328 | 152.771 | 7.268 | 15.10× |
| batch | 1 | playwright-chrome | pass | yes | 200.465 | 415.85 | 4.508 | 15.06× |
| batch | 1 | shotium | pass | yes | 13.309 | 277.157 | 28.002 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 228.847 | 442.85 | 3.964 | 17.19× |
| batch | 1 | puppeteer-shell | pass | yes | 152.633 | 385.269 | 5.529 | 11.47× |
| batch | 1 | playwright-shell | pass | yes | 160.998 | 391.059 | 5.289 | 12.10× |
| parallel | 1 | puppeteer-shell | pass | yes | 153.176 | 373.415 | 5.644 | 11.97× |
| parallel | 1 | playwright-chrome | pass | yes | 188.676 | 405.92 | 4.711 | 14.74× |
| parallel | 1 | playwright-shell | pass | yes | 154.71 | 389.418 | 5.496 | 12.09× |
| parallel | 1 | puppeteer-chrome | pass | yes | 216.37 | 455.796 | 4.103 | 16.90× |
| parallel | 1 | shotium | pass | yes | 12.8 | 259.468 | 27.920 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 306.532 | 595.35 | 5.926 | 7.85× |
| parallel | 2 | playwright-shell | pass | yes | 234.389 | 479.809 | 7.647 | 6.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 377.879 | 612.594 | 5.001 | 9.68× |
| parallel | 2 | shotium | pass | yes | 39.049 | 272.593 | 29.224 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 232.617 | 480.623 | 7.530 | 5.96× |
| parallel | 4 | playwright-shell | pass | yes | 461.22 | 741.205 | 8.499 | 5.33× |
| parallel | 4 | puppeteer-chrome | pass | yes | 660.168 | 992.665 | 5.836 | 7.63× |
| parallel | 4 | shotium | pass | yes | 86.515 | 337.551 | 29.295 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 436.575 | 729.811 | 8.810 | 5.05× |
| parallel | 4 | playwright-chrome | pass | yes | 525.278 | 868.728 | 7.237 | 6.07× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.045 | 134.051 | 9.636 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.633 | 103.309 | 10.114 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 100.536 | 896.678 | 6.711 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 101.087 | 120.453 | 9.399 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 1034 | 1155 | 1.034 | 7.03× |
| resident | 1 | puppeteer-shell | pass | yes | 850 | 962 | 1.222 | 5.78× |
| resident | 1 | shotium | pass | yes | 147 | 255 | 6.944 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 909 | 1009 | 1.077 | 6.18× |
| resident | 1 | playwright-shell | pass | yes | 949 | 956 | 1.150 | 6.46× |
| faults | 1 | playwright-shell | pass | no | 17212.708 | 17212.708 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 18033.557 | 18033.557 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4774.262 | 4774.262 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10117.263 | 10117.263 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 13964.987 | 13964.987 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 450.553 | 901.649 | 8.284 | 5.01× |
| soak | 4 | puppeteer-shell | pass | yes | 449.084 | 827.265 | 8.612 | 5.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 699.213 | 1207.88 | 5.537 | 7.78× |
| soak | 4 | playwright-chrome | pass | yes | 543.251 | 954.11 | 7.018 | 6.04× |
| soak | 4 | shotium | pass | yes | 89.895 | 360.902 | 29.324 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.608× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.642× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.12.1/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 814 | 832 | 1.229 | 12.52× |
| cold | 1 | playwright-shell | pass | yes | 645 | 661 | 1.562 | 9.92× |
| cold | 1 | shotium | pass | yes | 65 | 67 | 15.521 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 21.512 | 22.297 | 46.048 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 166.042 | 173.848 | 6.031 | 7.72× |
| cold-settled | 1 | playwright-shell | pass | yes | 140.785 | 145.322 | 7.279 | 6.54× |
| lifecycle | 1 | playwright-chrome | fail | no | 1527.546 | 1593.147 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 69.787 | 83.483 | 13.840 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 593.536 | 627.919 | 1.699 | 8.50× |
| warm | 1 | playwright-chrome | pass | yes | 170.856 | 198.374 | 5.722 | 8.99× |
| warm | 1 | playwright-shell | pass | yes | 133.578 | 154.027 | 7.208 | 7.03× |
| warm | 1 | shotium | pass | yes | 19.002 | 26.45 | 48.840 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 193.726 | 407.978 | 4.733 | 8.16× |
| batch | 1 | shotium | pass | yes | 23.745 | 262.935 | 23.391 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 151.312 | 392.711 | 5.738 | 6.37× |
| parallel | 1 | playwright-shell | pass | yes | 145.864 | 373.861 | 5.879 | 6.24× |
| parallel | 1 | playwright-chrome | pass | yes | 184.372 | 446.661 | 4.842 | 7.89× |
| parallel | 1 | shotium | pass | yes | 23.375 | 258.849 | 24.110 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 250.988 | 518.43 | 7.141 | 5.12× |
| parallel | 2 | shotium | pass | yes | 49.007 | 298.419 | 23.836 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 197.611 | 424.005 | 8.775 | 4.03× |
| parallel | 4 | shotium | pass | yes | 108.477 | 344.839 | 23.861 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 332.599 | 597.899 | 10.900 | 3.07× |
| parallel | 4 | playwright-chrome | pass | yes | 448.126 | 733.739 | 8.607 | 4.13× |
| reuse-page | 1 | playwright-shell | pass | no | 99.923 | 134.088 | 9.841 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 99.928 | 630.026 | 7.898 | N/A |
| resident | 1 | shotium | pass | yes | 181 | 288 | 5.534 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 895 | 925 | 1.117 | 4.94× |
| resident | 1 | playwright-chrome | pass | yes | 938 | 997 | 1.055 | 5.18× |
| faults | 1 | shotium | pass | no | 4579.71 | 4579.71 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16885.367 | 16885.367 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14546.842 | 14546.842 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 112.108 | 376.62 | 23.626 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 365.12 | 857.161 | 10.270 | 3.26× |
| soak | 4 | playwright-chrome | pass | yes | 459.699 | 876.932 | 8.409 | 4.10× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 10.560× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 10.969× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 16.255× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 15.180× | 5 | 6 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.12.1\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | pass | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 969 | 1021 | 1.040 | 17.00× |
| cold | 1 | shotium | pass | yes | 57 | 58 | 17.544 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 1177 | 1304 | 0.832 | 20.65× |
| cold | 1 | puppeteer-shell | pass | yes | 992 | 1027 | 1.002 | 17.40× |
| cold | 1 | playwright-chrome | pass | yes | 1228 | 1351 | 0.810 | 21.54× |
| cold-settled | 1 | playwright-chrome | pass | yes | 206.546 | 242.313 | 4.844 | 17.42× |
| cold-settled | 1 | shotium | pass | yes | 11.857 | 13.138 | 82.019 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 206.746 | 315.574 | 4.455 | 17.44× |
| cold-settled | 1 | playwright-shell | pass | yes | 185.402 | 196.91 | 5.483 | 15.64× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 188.747 | 214.867 | 5.250 | 15.92× |
| lifecycle | 1 | playwright-chrome | fail | no | 8403.099 | 10476.107 | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1732.554 | 2894.012 | 0.513 | 20.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3208.243 | 10478.729 | 0.280 | 37.03× |
| lifecycle | 1 | shotium | pass | yes | 86.649 | 191.137 | 10.019 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 1112.13 | 6268.062 | 0.613 | 12.83× |
| warm | 1 | puppeteer-chrome | pass | yes | 251.323 | 338.106 | 3.934 | 29.52× |
| warm | 1 | playwright-shell | pass | yes | 169.219 | 200.359 | 5.770 | 19.87× |
| warm | 1 | shotium | pass | yes | 8.515 | 11.428 | 103.983 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 178.213 | 233.198 | 5.620 | 20.93× |
| warm | 1 | puppeteer-shell | pass | yes | 165.092 | 188.494 | 5.930 | 19.39× |
| batch | 1 | playwright-chrome | pass | yes | 242.124 | 3945.343 | 2.918 | 16.67× |
| batch | 1 | shotium | pass | yes | 14.523 | 265.051 | 27.451 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 258.608 | 490.052 | 3.607 | 17.81× |
| batch | 1 | puppeteer-shell | pass | yes | 195.796 | 429.018 | 4.620 | 13.48× |
| batch | 1 | playwright-shell | pass | yes | 214.138 | 454.548 | 4.405 | 14.74× |
| parallel | 1 | puppeteer-shell | pass | yes | 167.514 | 474.031 | 4.790 | 10.62× |
| parallel | 1 | playwright-chrome | pass | yes | 198.713 | 410.327 | 4.605 | 12.60× |
| parallel | 1 | playwright-shell | pass | yes | 186.202 | 457.55 | 4.661 | 11.81× |
| parallel | 1 | puppeteer-chrome | pass | yes | 246.306 | 475.829 | 3.720 | 15.62× |
| parallel | 1 | shotium | pass | yes | 15.77 | 268.799 | 25.583 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 313.489 | 572.991 | 5.952 | 7.41× |
| parallel | 2 | playwright-shell | pass | yes | 290.073 | 665.802 | 6.181 | 6.86× |
| parallel | 2 | puppeteer-chrome | pass | yes | 463.466 | 788.109 | 4.131 | 10.96× |
| parallel | 2 | shotium | pass | yes | 42.281 | 276.446 | 27.787 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 270.742 | 484.618 | 6.879 | 6.40× |
| parallel | 4 | playwright-shell | pass | yes | 433.225 | 759.171 | 8.568 | 4.34× |
| parallel | 4 | puppeteer-chrome | pass | yes | 804.95 | 1273.297 | 4.820 | 8.07× |
| parallel | 4 | shotium | pass | yes | 99.777 | 352.45 | 27.224 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 448.751 | 762.372 | 8.321 | 4.50× |
| parallel | 4 | playwright-chrome | fail | no | 631.084 | 3242.679 | 5.720 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 116.281 | 134.945 | 8.625 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 102.378 | 118.249 | 9.643 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 113.715 | 211.135 | 8.355 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 118.062 | 133.51 | 8.304 | N/A |
| resident | 1 | playwright-chrome | noisy | no | 25821.758 | 25821.758 | N/A | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 762 | 1250 | 1.276 | 12.10× |
| resident | 1 | shotium | pass | yes | 63 | 580 | 4.391 | 1.00× |
| resident | 1 | puppeteer-chrome | fail | no | 34773.644 | 34773.644 | N/A | N/A |
| resident | 1 | playwright-shell | pass | yes | 727 | 972 | 1.310 | 11.54× |
| faults | 1 | playwright-shell | pass | no | 27460.207 | 27460.207 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 11567.149 | 11567.149 | N/A | N/A |
| faults | 1 | shotium | pass | no | 12007.855 | 12007.855 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 29044.761 | 29044.761 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 31616.493 | 31616.493 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 441.381 | 791.604 | 8.662 | 4.25× |
| soak | 4 | puppeteer-shell | pass | yes | 488.655 | 901.723 | 7.928 | 4.71× |
| soak | 4 | puppeteer-chrome | pass | yes | 850.237 | 1863.782 | 4.578 | 8.19× |
| soak | 4 | playwright-chrome | fail | no | 589.508 | 2569.405 | 3.975 | N/A |
| soak | 4 | shotium | pass | yes | 103.853 | 455.292 | 25.966 | 1.00× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.12.1\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 52 | 58 | 18.767 | N/A |
| cold-settled | 1 | shotium | pass | no | 8.9 | 12.282 | 104.009 | N/A |
| lifecycle | 1 | shotium | pass | no | 80.454 | 140.111 | 11.352 | N/A |
| warm | 1 | shotium | pass | no | 7.382 | 10.493 | 124.387 | N/A |
| batch | 1 | shotium | pass | no | 13.177 | 269.544 | 27.407 | N/A |
| parallel | 1 | shotium | pass | no | 12.029 | 266.483 | 29.310 | N/A |
| parallel | 2 | shotium | pass | no | 34.597 | 285.295 | 29.969 | N/A |
| parallel | 4 | shotium | pass | no | 75.611 | 323.971 | 30.147 | N/A |
| resident | 1 | shotium | pass | no | 242 | 772 | 2.954 | N/A |
| faults | 1 | shotium | pass | no | 7976.137 | 7976.137 | N/A | N/A |
| soak | 4 | shotium | pass | no | 57.412 | 423.767 | 34.119 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 10.320× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 12.056× | 7 | 9 / 9 | 0 |
| 4 | puppeteer-chrome | 18.031× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 27.532× | 6 | 8 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.12.1/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 1412 | 1697 | 0.710 | 12.72× |
| cold | 1 | shotium | pass | yes | 111 | 186 | 8.772 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 2457 | 2679 | 0.406 | 22.14× |
| cold | 1 | puppeteer-shell | pass | yes | 1407 | 1877 | 0.701 | 12.68× |
| cold | 1 | playwright-chrome | pass | yes | 3318 | 4256 | 0.298 | 29.89× |
| cold-settled | 1 | playwright-chrome | pass | yes | 777.418 | 1346.104 | 1.128 | 74.53× |
| cold-settled | 1 | shotium | pass | yes | 10.431 | 14.372 | 84.534 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 376.232 | 881.414 | 2.254 | 36.07× |
| cold-settled | 1 | playwright-shell | pass | yes | 221.341 | 381.1 | 3.890 | 21.22× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 287.494 | 359.588 | 3.249 | 27.56× |
| lifecycle | 1 | playwright-chrome | fail | no | 3827.492 | 5628.01 | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1382.893 | 1983.069 | 0.702 | 13.25× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2408.864 | 2940.988 | 0.403 | 23.07× |
| lifecycle | 1 | shotium | pass | yes | 104.405 | 180.807 | 9.125 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 1055.68 | 1358.434 | 0.922 | 10.11× |
| warm | 1 | puppeteer-chrome | pass | yes | 573.923 | 1724.261 | 1.481 | 49.40× |
| warm | 1 | playwright-shell | pass | yes | 346.754 | 438.713 | 2.944 | 29.85× |
| warm | 1 | shotium | pass | yes | 11.618 | 20.04 | 73.112 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 818.585 | 978.566 | 1.196 | 70.46× |
| warm | 1 | puppeteer-shell | pass | yes | 341.202 | 404.754 | 2.962 | 29.37× |
| batch | 1 | playwright-chrome | pass | yes | 876.929 | 1437.102 | 1.113 | 32.83× |
| batch | 1 | shotium | pass | yes | 26.709 | 265.549 | 19.341 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 607.154 | 1062.995 | 1.591 | 22.73× |
| batch | 1 | puppeteer-shell | pass | yes | 399.207 | 743.321 | 2.382 | 14.95× |
| batch | 1 | playwright-shell | pass | yes | 367.731 | 903.441 | 2.506 | 13.77× |
| parallel | 1 | puppeteer-shell | pass | yes | 347.498 | 731.133 | 2.799 | 17.26× |
| parallel | 1 | playwright-chrome | pass | yes | 875.937 | 1319.089 | 1.132 | 43.51× |
| parallel | 1 | playwright-shell | pass | yes | 345.206 | 806.953 | 2.699 | 17.15× |
| parallel | 1 | puppeteer-chrome | pass | yes | 557.66 | 1010.673 | 1.735 | 27.70× |
| parallel | 1 | shotium | pass | yes | 20.133 | 259.803 | 23.852 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 1588.372 | 3035.427 | 1.236 | 37.14× |
| parallel | 2 | playwright-shell | pass | yes | 409.885 | 620.949 | 4.783 | 9.58× |
| parallel | 2 | puppeteer-chrome | pass | yes | 748.923 | 3043.912 | 2.427 | 17.51× |
| parallel | 2 | shotium | pass | yes | 42.769 | 272.618 | 26.992 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 444.022 | 664.503 | 4.489 | 10.38× |
| parallel | 4 | playwright-shell | pass | yes | 668.943 | 1108.813 | 5.762 | 4.09× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1376.51 | 3537.053 | 2.606 | 8.42× |
| parallel | 4 | shotium | pass | yes | 163.387 | 455.068 | 19.321 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 1025.519 | 2591.459 | 3.503 | 6.28× |
| parallel | 4 | playwright-chrome | pass | yes | 3003.501 | 5349.455 | 1.393 | 18.38× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 237.617 | 289.168 | 4.186 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 242.148 | 340.656 | 3.886 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 238.017 | 324.776 | 4.384 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 237.718 | 310.293 | 4.382 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 2079 | 2362 | 0.474 | 2.16× |
| resident | 1 | puppeteer-shell | pass | yes | 2270 | 2488 | 0.459 | 2.35× |
| resident | 1 | shotium | pass | yes | 964 | 1249 | 1.008 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 2297 | 3366 | 0.387 | 2.38× |
| resident | 1 | playwright-shell | pass | yes | 1697 | 2504 | 0.544 | 1.76× |
| faults | 1 | playwright-shell | pass | no | 25677.274 | 25677.274 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 36675.853 | 36675.853 | N/A | N/A |
| faults | 1 | shotium | pass | no | 8233.627 | 8233.627 | N/A | N/A |
| faults | 1 | puppeteer-shell | noisy | no | 10856.43 | 10856.43 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | 14319.881 | 14319.881 | N/A | N/A |
| soak | 4 | playwright-shell | pass | no | 745.651 | 2081.693 | 5.241 | N/A |
| soak | 4 | puppeteer-shell | pass | no | 716.437 | 2156.277 | 5.424 | N/A |
| soak | 4 | puppeteer-chrome | pass | no | 1640.318 | 3774.312 | 2.345 | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 7 eligible cell(s), with 7 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 7 / 7 | 7 |
| 2 | playwright-shell | 9.567× | 7 | 7 / 7 | 0 |
| 3 | puppeteer-shell | 13.030× | 7 | 7 / 7 | 0 |
| 4 | puppeteer-chrome | 22.234× | 7 | 7 / 7 | 0 |
| not ranked (partial coverage) | playwright-chrome | 15.951× | 6 | 6 / 7 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.12.1/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 933 | 1323 | 1.009 | 12.78× |
| cold | 1 | shotium | pass | yes | 73 | 112 | 12.545 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 2814 | 5781 | 0.321 | 38.55× |
| cold | 1 | puppeteer-shell | pass | yes | 1028 | 1338 | 0.978 | 14.08× |
| cold | 1 | playwright-chrome | pass | yes | 2812 | 4234 | 0.344 | 38.52× |
| cold-settled | 1 | playwright-chrome | pass | yes | 218.351 | 360.067 | 3.907 | 15.55× |
| cold-settled | 1 | shotium | pass | yes | 14.038 | 21.009 | 73.252 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 371.094 | 681.042 | 2.306 | 26.43× |
| cold-settled | 1 | playwright-shell | pass | yes | 188.629 | 396.267 | 4.981 | 13.44× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 336.695 | 365.105 | 3.159 | 23.98× |
| lifecycle | 1 | playwright-chrome | fail | no | 3704.518 | 4010.144 | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 866.357 | 1379.403 | 1.116 | 9.95× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1912.975 | 3030.973 | 0.502 | 21.98× |
| lifecycle | 1 | shotium | pass | yes | 87.048 | 147.766 | 11.170 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 797.763 | 1414.713 | 1.235 | 9.16× |
| warm | 1 | puppeteer-chrome | pass | yes | 351.605 | 570.55 | 2.811 | 60.20× |
| warm | 1 | playwright-shell | pass | yes | 144.528 | 162.011 | 7.148 | 24.74× |
| warm | 1 | shotium | pass | yes | 5.841 | 9.728 | 148.370 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 181.578 | 300.44 | 5.258 | 31.09× |
| warm | 1 | puppeteer-shell | pass | yes | 241.317 | 434.744 | 3.938 | 41.31× |
| batch | 1 | playwright-chrome | pass | yes | 252.275 | 1799.32 | 2.903 | 23.22× |
| batch | 1 | shotium | pass | yes | 10.865 | 258.416 | 31.663 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 450.696 | 2208.054 | 1.968 | 41.48× |
| batch | 1 | puppeteer-shell | pass | yes | 297.281 | 663.821 | 3.269 | 27.36× |
| batch | 1 | playwright-shell | pass | yes | 162.279 | 414.929 | 5.403 | 14.94× |
| parallel | 1 | puppeteer-shell | pass | no | 263.836 | 2044.425 | 3.311 | N/A |
| parallel | 1 | playwright-chrome | pass | no | 249.557 | 1904.965 | 2.942 | N/A |
| parallel | 1 | playwright-shell | pass | no | 183.539 | 414.617 | 4.922 | N/A |
| parallel | 1 | puppeteer-chrome | pass | no | 446.882 | 1645.774 | 2.005 | N/A |
| parallel | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | pass | no | 240.426 | 448.791 | 7.472 | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | 12140.197 | 12140.197 | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 286.615 | 447.952 | 3.448 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 240.299 | 355.023 | 3.862 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 102.864 | 139.053 | 9.517 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 128.563 | 235.059 | 7.334 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 1707 | 2205 | 0.621 | 3.52× |
| resident | 1 | puppeteer-shell | pass | yes | 1208 | 1227 | 0.874 | 2.49× |
| resident | 1 | shotium | pass | yes | 485 | 554 | 2.070 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 1570 | 2225 | 0.608 | 3.24× |
| resident | 1 | playwright-shell | pass | yes | 1148 | 1436 | 0.879 | 2.37× |
| faults | 1 | playwright-shell | pass | no | 14751.826 | 14751.826 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 28653.967 | 28653.967 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5383.162 | 5383.162 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 11048.839 | 11048.839 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 22634.304 | 22634.304 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 359.135 | 992.274 | 10.531 | 5.33× |
| soak | 4 | puppeteer-shell | pass | yes | 454.272 | 918.865 | 8.479 | 6.74× |
| soak | 4 | puppeteer-chrome | pass | yes | 1000.15 | 4114.626 | 3.831 | 14.84× |
| soak | 4 | playwright-chrome | pass | yes | 729.465 | 4278.237 | 4.999 | 10.82× |
| soak | 4 | shotium | pass | yes | 67.409 | 394.099 | 31.354 | 1.00× |

