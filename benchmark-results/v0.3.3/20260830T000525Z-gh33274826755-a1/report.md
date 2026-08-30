# Shotium 0.3.3 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **incomplete**; quality: **fail**; evidence: **incomplete**. Profile **full**, seed `bac114e9f8c57fb04875ee8ceec7e8e7c11905d4`.

Conclusion: this run is incomplete. 5 platform(s) contain valid within-platform comparisons; missing outputs are never inferred.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | fail | shotium | 2 | 10 |
| linux-arm64 | noisy | shotium | 1 | 10 |
| win32-x64 | infra-error | shotium | 5 | 1 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | fail | shotium | 5 | 1 |
| darwin-arm64 | noisy | shotium | 5 | 1 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.617× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 4.230× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 4.758× | 5 | 7 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 6.334× | 6 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | noisy | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | noisy | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 53 | 54 | 18.919 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 410 | 464 | 2.396 | 7.74× |
| cold | 1 | puppeteer-shell | pass | yes | 282 | 375 | 3.235 | 5.32× |
| cold | 1 | puppeteer-chrome | pass | yes | 471 | 642 | 2.044 | 8.89× |
| cold | 1 | playwright-shell | pass | yes | 256 | 281 | 3.857 | 4.83× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 104.061 | 124.113 | 9.287 | 4.95× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 114.146 | 142.731 | 8.569 | 5.43× |
| cold-settled | 1 | playwright-chrome | pass | yes | 118.888 | 170.457 | 7.536 | 5.65× |
| cold-settled | 1 | shotium | pass | yes | 21.028 | 21.967 | 46.435 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 95.619 | 105.772 | 10.494 | 4.55× |
| lifecycle | 1 | shotium | pass | yes | 53.989 | 80.148 | 16.839 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 565.139 | 1648.594 | 1.548 | 10.47× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 289.344 | 409.664 | 3.402 | 5.36× |
| lifecycle | 1 | playwright-shell | pass | yes | 262.976 | 301.556 | 3.901 | 4.87× |
| lifecycle | 1 | playwright-chrome | pass | yes | 488.753 | 723.828 | 1.978 | 9.05× |
| warm | 1 | playwright-chrome | noisy | no | 145.381 | 179.195 | 2.815 | N/A |
| warm | 1 | shotium | pass | yes | 24.905 | 29.44 | 38.201 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 157.438 | 201.988 | 6.217 | 6.32× |
| warm | 1 | playwright-shell | pass | yes | 122.864 | 149.809 | 8.026 | 4.93× |
| warm | 1 | puppeteer-shell | pass | yes | 131.601 | 157.137 | 7.542 | 5.28× |
| batch | 1 | playwright-shell | pass | yes | 138.668 | 360.009 | 6.215 | 4.31× |
| batch | 1 | puppeteer-shell | pass | yes | 147.616 | 349.613 | 5.845 | 4.59× |
| batch | 1 | shotium | pass | yes | 32.151 | 267.234 | 18.545 | 1.00× |
| batch | 1 | playwright-chrome | noisy | no | 165.397 | 374.801 | 4.373 | N/A |
| batch | 1 | puppeteer-chrome | pass | yes | 187.216 | 401.322 | 4.816 | 5.82× |
| parallel | 1 | playwright-shell | pass | yes | 107.387 | 1160.162 | 6.843 | 3.65× |
| parallel | 1 | playwright-chrome | pass | yes | 137.004 | 358.306 | 6.273 | 4.65× |
| parallel | 1 | puppeteer-chrome | pass | yes | 149.739 | 375.189 | 5.844 | 5.09× |
| parallel | 1 | puppeteer-shell | pass | yes | 120.234 | 330.217 | 6.806 | 4.08× |
| parallel | 1 | shotium | pass | yes | 29.44 | 258.629 | 20.560 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 255.832 | 427.23 | 6.888 | 4.33× |
| parallel | 2 | puppeteer-shell | pass | yes | 214.92 | 624.344 | 7.957 | 3.63× |
| parallel | 2 | shotium | pass | yes | 59.133 | 290.96 | 20.488 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 188.534 | 348.728 | 8.873 | 3.19× |
| parallel | 2 | playwright-chrome | pass | yes | 238.892 | 588.97 | 7.556 | 4.04× |
| parallel | 4 | shotium | pass | yes | 128.032 | 347.711 | 20.665 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 355.746 | 600.237 | 9.743 | 2.78× |
| parallel | 4 | playwright-chrome | pass | yes | 406.883 | 595.639 | 8.416 | 3.18× |
| parallel | 4 | puppeteer-chrome | fail | no | 472.409 | 920.936 | 7.578 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 391.442 | 609.159 | 9.002 | 3.06× |
| reuse-page | 1 | playwright-shell | pass | no | 65.7 | 102.291 | 15.320 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 66.998 | 86.19 | 14.172 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 82.989 | 124.292 | 11.755 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 68.45 | 96.443 | 13.499 | N/A |
| resident | 1 | shotium | pass | yes | 455 | 465 | 2.222 | 1.00× |
| resident | 1 | puppeteer-chrome | noisy | no | 15305.456 | 15810.345 | N/A | N/A |
| resident | 1 | playwright-shell | pass | yes | 971 | 1042 | 1.038 | 2.13× |
| resident | 1 | puppeteer-shell | noisy | no | 767.5 | 825 | 0.094 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 1061 | 1095 | 0.957 | 2.33× |
| faults | 1 | puppeteer-shell | pass | no | 10025.711 | 10025.711 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 13297.764 | 13297.764 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16319.469 | 16319.469 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 17197.933 | 17197.933 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10491.485 | 10491.485 | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | 3332.398 | 3332.398 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 403.487 | 885.848 | 9.084 | 2.44× |
| soak | 4 | puppeteer-shell | pass | yes | 460.5 | 856.388 | 8.358 | 2.79× |
| soak | 4 | shotium | pass | yes | 165.14 | 387.537 | 19.653 | 1.00× |
| soak | 4 | puppeteer-chrome | infra-error | no | 591.049 | 180896.151 | 1.404 | N/A |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| not ranked (partial coverage) | playwright-shell | 2.634× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 3.749× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | noisy | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | noisy | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 230 | 238 | 4.345 | 3.65× |
| cold | 1 | shotium | pass | yes | 63 | 67 | 15.730 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 406 | 1069 | 2.020 | 6.44× |
| cold-settled | 1 | playwright-chrome | noisy | no | 120.816 | 132.088 | 1.845 | N/A |
| cold-settled | 1 | shotium | pass | yes | 34.088 | 34.76 | 29.329 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 110.935 | 123.177 | 9.125 | 3.25× |
| lifecycle | 1 | shotium | pass | yes | 73.365 | 101.532 | 12.932 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 238.236 | 312.762 | 4.085 | 3.25× |
| lifecycle | 1 | playwright-chrome | pass | yes | 418.73 | 494.885 | 2.362 | 5.71× |
| warm | 1 | shotium | pass | yes | 32.049 | 34.055 | 30.984 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 117.495 | 158.248 | 8.267 | 3.67× |
| warm | 1 | playwright-shell | pass | yes | 103.611 | 132.446 | 9.290 | 3.23× |
| batch | 1 | playwright-shell | pass | yes | 124.002 | 323.849 | 7.079 | 2.89× |
| batch | 1 | playwright-chrome | pass | yes | 142.813 | 356.542 | 6.137 | 3.33× |
| batch | 1 | shotium | pass | yes | 42.861 | 260.261 | 16.716 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 152.331 | 356.975 | 5.805 | 3.45× |
| parallel | 1 | playwright-shell | pass | yes | 126.921 | 348.32 | 6.861 | 2.87× |
| parallel | 1 | shotium | pass | yes | 44.16 | 262.453 | 16.439 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 184.844 | 343.981 | 8.918 | 2.05× |
| parallel | 2 | shotium | pass | yes | 90.209 | 295.828 | 16.317 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 251.896 | 413.124 | 7.179 | 2.79× |
| parallel | 4 | shotium | pass | yes | 184.187 | 379.566 | 16.228 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 438.408 | 608.686 | 8.080 | 2.38× |
| parallel | 4 | playwright-shell | pass | yes | 343.492 | 544.654 | 9.785 | 1.86× |
| reuse-page | 1 | playwright-shell | pass | no | 50.118 | 60.441 | 19.224 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 58.17 | 317.889 | 12.928 | N/A |
| resident | 1 | playwright-shell | noisy | no | 715.5 | 884 | 0.369 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 629 | 876 | 1.542 | 6.84× |
| resident | 1 | shotium | pass | yes | 92 | 249 | 8.526 | 1.00× |
| faults | 1 | playwright-chrome | pass | no | 15569.272 | 15569.272 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10383.181 | 10383.181 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14505.632 | 14505.632 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 308.776 | 597.304 | 11.949 | 1.54× |
| soak | 4 | playwright-chrome | pass | yes | 417.275 | 753.151 | 9.330 | 2.08× |
| soak | 4 | shotium | pass | yes | 200.7 | 451.49 | 16.216 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 1 eligible cell(s), with 1 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 1 | 1 / 1 | 1 |
| 2 | playwright-shell | 5.415× | 1 | 1 / 1 | 0 |
| 3 | puppeteer-shell | 8.582× | 1 | 1 / 1 | 0 |
| 4 | playwright-chrome | 11.359× | 1 | 1 / 1 | 0 |
| 5 | puppeteer-chrome | 16.961× | 1 | 1 / 1 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `lifecycle/c1`
- playwright-shell: `lifecycle/c1`
- puppeteer-shell: `lifecycle/c1`
- playwright-chrome: `lifecycle/c1`
- puppeteer-chrome: `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | noisy | no | 84 | 99 | 11.628 | N/A |
| cold | 1 | playwright-chrome | pass | no | 701 | 821 | 1.383 | N/A |
| cold | 1 | puppeteer-shell | pass | no | 560 | 605 | 1.751 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 794.5 | 815 | 1.263 | N/A |
| cold | 1 | playwright-shell | noisy | no | 397 | 423 | 2.524 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 46420.41 | 47329.032 | N/A | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 6183.054 | 47633.995 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 46821.762 | 47631.277 | N/A | N/A |
| cold-settled | 1 | shotium | noisy | no | 45767.839 | 46756.035 | N/A | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 46789.648 | 47227.197 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 112.285 | 238.905 | 8.263 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1904.429 | 2179.802 | 0.534 | 16.96× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 963.584 | 1077.043 | 1.032 | 8.58× |
| lifecycle | 1 | playwright-shell | pass | yes | 607.973 | 759.499 | 1.634 | 5.41× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1275.404 | 1419.992 | 0.777 | 11.36× |
| warm | 1 | playwright-chrome | noisy | no | 47053.403 | 47438.84 | N/A | N/A |
| warm | 1 | shotium | noisy | no | 46803.022 | 47044.192 | N/A | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 6236.182 | 47466.327 | N/A | N/A |
| warm | 1 | playwright-shell | noisy | no | 46779.769 | 47023.534 | N/A | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 46625.748 | 47179.071 | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | 46600.013 | 46744.045 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 47137.276 | 47557.484 | N/A | N/A |
| batch | 1 | shotium | noisy | no | 46709.656 | 47046.573 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | 5836.161 | 47849.154 | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 5841.33 | 47693.235 | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | 46756.185 | 46756.185 | N/A | N/A |
| resident | 1 | shotium | noisy | no | 22673.875 | 46230.858 | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 25002.277 | 29852.297 | N/A | N/A |
| resident | 1 | playwright-shell | noisy | no | 47052.666 | 47378.883 | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 46932.998 | 47286.106 | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | no | 47856.224 | 48019.072 | N/A | N/A |
| faults | 1 | puppeteer-shell | noisy | no | 5315.461 | 5315.461 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | 6092.129 | 6092.129 | N/A | N/A |
| faults | 1 | playwright-shell | noisy | no | 47240.803 | 47240.803 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 47674.012 | 47674.012 | N/A | N/A |
| faults | 1 | shotium | noisy | no | 46275.446 | 46275.446 | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | 6495.607 | 6495.607 | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | 47337.462 | 47337.462 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | 47154.786 | 47154.786 | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 79 | 82 | 12.797 | N/A |
| cold-settled | 1 | shotium | noisy | no | 46055.255 | 46304.156 | N/A | N/A |
| lifecycle | 1 | shotium | pass | no | 99.682 | 133.972 | 9.470 | N/A |
| warm | 1 | shotium | noisy | no | 46609.837 | 47297.699 | N/A | N/A |
| batch | 1 | shotium | noisy | no | 46673.108 | 47100.177 | N/A | N/A |
| parallel | 1 | shotium | noisy | no | 46832.949 | 47015.363 | N/A | N/A |
| parallel | 2 | shotium | noisy | no | 46566.402 | 46947.796 | N/A | N/A |
| parallel | 4 | shotium | noisy | no | 46371.081 | 46748.461 | N/A | N/A |
| resident | 1 | shotium | noisy | no | 23468.928 | 47004.738 | N/A | N/A |
| faults | 1 | shotium | noisy | no | 45919.856 | 45919.856 | N/A | N/A |
| soak | 4 | shotium | noisy | no | 45996.694 | 45996.694 | N/A | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 1 eligible cell(s), with 1 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 1 | 1 / 1 | 1 |
| 2 | playwright-shell | 5.429× | 1 | 1 / 1 | 0 |
| 3 | puppeteer-shell | 7.045× | 1 | 1 / 1 | 0 |
| 4 | playwright-chrome | 16.668× | 1 | 1 / 1 | 0 |
| 5 | puppeteer-chrome | 18.458× | 1 | 1 / 1 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `lifecycle/c1`
- playwright-shell: `lifecycle/c1`
- puppeteer-shell: `lifecycle/c1`
- playwright-chrome: `lifecycle/c1`
- puppeteer-chrome: `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | infra-error | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | noisy | no | 129.5 | 626 | 4.800 | N/A |
| cold | 1 | playwright-chrome | noisy | no | 3532 | 13982 | 0.193 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 1122.5 | 2219 | 0.736 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 2859.5 | 5674 | 0.306 | N/A |
| cold | 1 | playwright-shell | noisy | no | 920 | 2236 | 0.872 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 46899.839 | 47513.046 | N/A | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 9570.887 | 12575.737 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 49334.307 | 50415.856 | N/A | N/A |
| cold-settled | 1 | shotium | noisy | no | 3596.295 | 45570.794 | N/A | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 6582.852 | 47010.516 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 193.911 | 446.878 | 4.655 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3579.117 | 5841.001 | 0.266 | 18.46× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1366.161 | 1681.9 | 0.744 | 7.05× |
| lifecycle | 1 | playwright-shell | pass | yes | 1052.71 | 1397.548 | 0.928 | 5.43× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3232.154 | 3828.663 | 0.307 | 16.67× |
| warm | 1 | playwright-chrome | noisy | no | 48731.132 | 49620.889 | N/A | N/A |
| warm | 1 | shotium | noisy | no | 3708.278 | 3998.977 | N/A | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 10014.418 | 10825.817 | N/A | N/A |
| warm | 1 | playwright-shell | noisy | no | 9233.235 | 47174.626 | N/A | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 46903.63 | 47571.156 | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | 7645.495 | 11860.297 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 46734.746 | 47742.664 | N/A | N/A |
| batch | 1 | shotium | noisy | no | 45375.223 | 46300.017 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | 49042.518 | 50736.82 | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 10940.815 | 17139.131 | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | no | 8920.629 | 46805.434 | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 50293.024 | 50887.955 | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | 11244.632 | 13692.273 | N/A | N/A |
| parallel | 1 | puppeteer-shell | noisy | no | 47072.241 | 47880.038 | N/A | N/A |
| parallel | 1 | shotium | noisy | no | 4540.187 | 46130.881 | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | 10705.087 | 12844.592 | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | 47030.523 | 47408.321 | N/A | N/A |
| parallel | 2 | shotium | noisy | no | 3726.215 | 45623.119 | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | 7762.797 | 47344.336 | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | 49614.134 | 50104.739 | N/A | N/A |
| parallel | 4 | shotium | noisy | no | 3703.045 | 45481.291 | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | 7823.593 | 10007.359 | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | 48968.439 | 49792.317 | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | 10432.539 | 12120.893 | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | 46916.093 | 48256.237 | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | 47405.64 | 47405.64 | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | 8017.343 | 8017.343 | N/A | N/A |
| resident | 1 | shotium | noisy | no | 21583.207 | 30510.351 | N/A | N/A |
| resident | 1 | puppeteer-chrome | fail | no | 48620.149 | 50216.508 | N/A | N/A |
| resident | 1 | playwright-shell | fail | no | 47264.169 | 48082.726 | N/A | N/A |
| resident | 1 | puppeteer-shell | infra-error | no | 47209.772 | 57526.101 | N/A | N/A |
| resident | 1 | playwright-chrome | fail | no | 49222.586 | 50003.737 | N/A | N/A |
| faults | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | 13479.421 | 13479.421 | N/A | N/A |
| faults | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 49583.068 | 49583.068 | N/A | N/A |
| faults | 1 | shotium | noisy | no | 3004.617 | 3004.617 | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | 13703.034 | 13703.034 | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | 6462.818 | 6462.818 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | 47202.558 | 47202.558 | N/A | N/A |
| soak | 4 | shotium | noisy | no | 3081.477 | 3081.477 | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | 8556.743 | 8556.743 | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 1 eligible cell(s), with 1 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 1 | 1 / 1 | 1 |
| 2 | playwright-shell | 3.442× | 1 | 1 / 1 | 0 |
| 3 | puppeteer-shell | 4.784× | 1 | 1 / 1 | 0 |
| 4 | puppeteer-chrome | 14.901× | 1 | 1 / 1 | 0 |
| 5 | playwright-chrome | 15.479× | 1 | 1 / 1 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `lifecycle/c1`
- playwright-shell: `lifecycle/c1`
- puppeteer-shell: `lifecycle/c1`
- puppeteer-chrome: `lifecycle/c1`
- playwright-chrome: `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | noisy | no | 95 | 232 | 9.160 | N/A |
| cold | 1 | playwright-chrome | noisy | no | 1959.5 | 3793 | 0.439 | N/A |
| cold | 1 | puppeteer-shell | pass | no | 494 | 911 | 1.739 | N/A |
| cold | 1 | puppeteer-chrome | pass | no | 1722 | 4268 | 0.509 | N/A |
| cold | 1 | playwright-shell | pass | no | 399 | 744 | 2.155 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 46234.354 | 46403.629 | N/A | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 5298.085 | 6191.569 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 47285.483 | 48086.451 | N/A | N/A |
| cold-settled | 1 | shotium | noisy | no | 1421.053 | 45537.935 | N/A | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 46171.791 | 46573.498 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 98.453 | 166.222 | 9.790 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1467.059 | 2206.518 | 0.649 | 14.90× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 470.985 | 821.312 | 2.011 | 4.78× |
| lifecycle | 1 | playwright-shell | pass | yes | 338.848 | 680.385 | 2.723 | 3.44× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1523.995 | 1986.194 | 0.666 | 15.48× |
| warm | 1 | playwright-chrome | noisy | no | 47191.451 | 47908.616 | N/A | N/A |
| warm | 1 | shotium | noisy | no | 1391.217 | 45228.503 | N/A | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 4941.614 | 10390.773 | N/A | N/A |
| warm | 1 | playwright-shell | noisy | no | 45890.194 | 46775.542 | N/A | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 46140.407 | 46446.384 | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | 46169.11 | 46564.594 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 45993.615 | 46589.433 | N/A | N/A |
| batch | 1 | shotium | noisy | no | 1316.909 | 1734.069 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | 47150.353 | 47384.527 | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 4904.649 | 6078.489 | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | no | 46297.956 | 46737.015 | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 47957.471 | 48448.498 | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | 6957.269 | 7477.368 | N/A | N/A |
| parallel | 1 | puppeteer-shell | noisy | no | 4135.02 | 46420.379 | N/A | N/A |
| parallel | 1 | shotium | noisy | no | 1711.497 | 45897.569 | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | 5424.86 | 7405.299 | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | 45799.001 | 46430.29 | N/A | N/A |
| parallel | 2 | shotium | noisy | no | 2743.837 | 45723.436 | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | 3510.896 | 46451.385 | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | 47378.786 | 47856.211 | N/A | N/A |
| parallel | 4 | shotium | noisy | no | 1583.639 | 45788.124 | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | 3811.891 | 46888.599 | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | 47125.247 | 48163.637 | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | 5174.097 | 7731.804 | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | 46127.759 | 46522.798 | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 5442.441 | 5442.441 | N/A | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | 48168.137 | 48168.137 | N/A | N/A |
| resident | 1 | shotium | noisy | no | 7354.842 | 10011.425 | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 15034.016 | 21954.708 | N/A | N/A |
| resident | 1 | playwright-shell | noisy | no | 46336.067 | 46835.282 | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 16474.441 | 46462.682 | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | no | 22686.729 | 48766.872 | N/A | N/A |
| faults | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-shell | noisy | no | 46608.46 | 46608.46 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 48556.105 | 48556.105 | N/A | N/A |
| faults | 1 | shotium | noisy | no | 2621.155 | 2621.155 | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | 47620.858 | 47620.858 | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | 3634.7 | 3634.7 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | 4006.723 | 4006.723 | N/A | N/A |
| soak | 4 | shotium | noisy | no | 45297.893 | 45297.893 | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | 5455.256 | 5455.256 | N/A | N/A |

