# Shotium 0.11.2 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `d253e2583e022b10454258c6dd4a7d16546538b0`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 4 | 7 |
| win32-arm64 | pass | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 2 | 8 |
| darwin-arm64 | noisy | shotium | 4 | 6 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 7.486× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 8.087× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 10.829× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 9.411× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.11.2/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 66 | 70 | 15.152 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 779 | 791 | 1.281 | 11.80× |
| cold | 1 | puppeteer-chrome | pass | yes | 896 | 928 | 1.123 | 13.58× |
| cold | 1 | playwright-chrome | pass | yes | 968 | 987 | 1.033 | 14.67× |
| cold | 1 | puppeteer-shell | pass | yes | 622 | 670 | 1.582 | 9.42× |
| cold-settled | 1 | playwright-shell | pass | yes | 140 | 144.686 | 7.295 | 9.58× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 126.953 | 140.628 | 7.784 | 8.69× |
| cold-settled | 1 | shotium | pass | yes | 14.616 | 15.817 | 67.358 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 168.168 | 211.845 | 5.624 | 11.51× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 175.231 | 193.223 | 5.813 | 11.99× |
| lifecycle | 1 | playwright-chrome | fail | no | 2122.405 | 2193.231 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 64.409 | 109.478 | 13.747 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 695.451 | 748.375 | 1.441 | 10.80× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 955.568 | 1028.135 | 1.052 | 14.84× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 640.706 | 697.763 | 1.537 | 9.95× |
| warm | 1 | puppeteer-chrome | pass | yes | 199.041 | 213.68 | 5.184 | 19.22× |
| warm | 1 | playwright-shell | pass | yes | 147.844 | 175.649 | 6.667 | 14.28× |
| warm | 1 | shotium | pass | yes | 10.355 | 21.552 | 84.255 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 133.09 | 135.109 | 7.517 | 12.85× |
| warm | 1 | playwright-chrome | pass | yes | 170.25 | 209.158 | 5.735 | 16.44× |
| batch | 1 | puppeteer-chrome | pass | yes | 221.921 | 427.403 | 4.064 | 17.47× |
| batch | 1 | shotium | pass | yes | 12.703 | 260.709 | 29.182 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 196.896 | 430.152 | 4.590 | 15.50× |
| batch | 1 | playwright-shell | pass | yes | 156.466 | 395.334 | 5.390 | 12.32× |
| batch | 1 | puppeteer-shell | pass | yes | 150.841 | 389.764 | 5.643 | 11.87× |
| parallel | 1 | playwright-chrome | pass | yes | 200.145 | 438.36 | 4.468 | 13.71× |
| parallel | 1 | puppeteer-shell | pass | yes | 153.688 | 387.178 | 5.579 | 10.53× |
| parallel | 1 | puppeteer-chrome | pass | yes | 233.484 | 461.354 | 3.968 | 16.00× |
| parallel | 1 | playwright-shell | pass | yes | 159.896 | 401.545 | 5.289 | 10.95× |
| parallel | 1 | shotium | pass | yes | 14.597 | 262.259 | 28.323 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 236.089 | 511.802 | 7.500 | 6.20× |
| parallel | 2 | puppeteer-chrome | pass | yes | 379.062 | 604.806 | 5.009 | 9.96× |
| parallel | 2 | playwright-shell | pass | yes | 252.591 | 467.473 | 7.297 | 6.64× |
| parallel | 2 | shotium | pass | yes | 38.052 | 273.33 | 28.232 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 296.319 | 544.807 | 6.273 | 7.79× |
| parallel | 4 | puppeteer-chrome | pass | yes | 644.915 | 1007.668 | 5.824 | 7.03× |
| parallel | 4 | playwright-shell | pass | yes | 423.585 | 794.441 | 8.427 | 4.61× |
| parallel | 4 | shotium | pass | yes | 91.793 | 329.224 | 28.061 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 547.425 | 894.312 | 6.964 | 5.96× |
| parallel | 4 | puppeteer-shell | pass | yes | 447.217 | 790.941 | 8.576 | 4.87× |
| reuse-page | 1 | puppeteer-shell | pass | no | 83.46 | 99.955 | 11.695 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 95.483 | 116.024 | 10.744 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 68.397 | 102.101 | 13.190 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 66.834 | 93.377 | 14.188 | N/A |
| resident | 1 | playwright-shell | pass | yes | 810 | 841 | 1.233 | 3.36× |
| resident | 1 | playwright-chrome | pass | yes | 864 | 891 | 1.230 | 3.59× |
| resident | 1 | shotium | pass | yes | 241 | 318 | 4.875 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 744 | 801 | 1.329 | 3.09× |
| resident | 1 | puppeteer-shell | pass | yes | 689 | 761 | 1.525 | 2.86× |
| faults | 1 | playwright-chrome | pass | no | 18223.267 | 18223.267 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4890.635 | 4890.635 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 11585.998 | 11585.998 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 14194.862 | 14194.862 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15553.014 | 15553.014 | N/A | N/A |
| soak | 4 | puppeteer-chrome | pass | yes | 729.755 | 1226.549 | 5.377 | 7.92× |
| soak | 4 | shotium | pass | yes | 92.182 | 346.044 | 28.622 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 450.356 | 860.881 | 8.513 | 4.89× |
| soak | 4 | playwright-shell | pass | yes | 455.465 | 969.99 | 8.247 | 4.94× |
| soak | 4 | playwright-chrome | pass | yes | 543.61 | 1463.021 | 7.004 | 5.90× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.252× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.088× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.11.2/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 850 | 871 | 1.176 | 14.17× |
| cold | 1 | playwright-shell | pass | yes | 674 | 707 | 1.485 | 11.23× |
| cold | 1 | shotium | pass | yes | 60 | 64 | 16.746 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 142.988 | 156.154 | 7.053 | 6.43× |
| cold-settled | 1 | shotium | pass | yes | 22.227 | 22.53 | 44.669 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 167.041 | 182.243 | 5.905 | 7.52× |
| lifecycle | 1 | shotium | pass | yes | 62.356 | 83.848 | 15.391 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 1594.03 | 1747.074 | N/A | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 595.648 | 664.998 | 1.661 | 9.55× |
| warm | 1 | shotium | pass | yes | 18.052 | 24.742 | 50.575 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 166.351 | 184.086 | 5.964 | 9.22× |
| warm | 1 | playwright-shell | pass | yes | 133.437 | 150.266 | 7.289 | 7.39× |
| batch | 1 | playwright-shell | pass | yes | 143.272 | 383.989 | 5.832 | 5.94× |
| batch | 1 | shotium | pass | yes | 24.138 | 261.171 | 23.599 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 179.552 | 406.732 | 4.948 | 7.44× |
| parallel | 1 | shotium | pass | yes | 24.039 | 263.8 | 23.427 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 145.828 | 388.849 | 5.826 | 6.07× |
| parallel | 1 | playwright-chrome | pass | yes | 195.147 | 432.11 | 4.622 | 8.12× |
| parallel | 2 | playwright-shell | pass | yes | 210.574 | 423.773 | 8.473 | 4.23× |
| parallel | 2 | playwright-chrome | pass | yes | 257.975 | 524.479 | 7.114 | 5.18× |
| parallel | 2 | shotium | pass | yes | 49.768 | 287.93 | 23.746 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 429.955 | 645.323 | 8.747 | 4.04× |
| parallel | 4 | shotium | pass | yes | 106.426 | 352.35 | 23.940 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 332.506 | 580.357 | 10.770 | 3.12× |
| reuse-page | 1 | playwright-chrome | pass | no | 99.924 | 108.388 | 9.916 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 99.871 | 103.142 | 10.006 | N/A |
| resident | 1 | shotium | pass | yes | 404 | 439 | 2.448 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 863 | 894 | 1.158 | 2.14× |
| resident | 1 | playwright-chrome | pass | yes | 931 | 984 | 1.135 | 2.30× |
| faults | 1 | playwright-chrome | pass | no | 15972.171 | 15972.171 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4577.777 | 4577.777 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14159.207 | 14159.207 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 436.525 | 748.524 | 8.804 | 4.02× |
| soak | 4 | playwright-shell | pass | yes | 334.377 | 671.918 | 10.994 | 3.08× |
| soak | 4 | shotium | pass | yes | 108.607 | 373.924 | 23.824 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 7 eligible cell(s), with 7 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 5 | 7 / 7 | 7 |
| 2 | puppeteer-shell | 9.543× | 5 | 7 / 7 | 0 |
| 3 | playwright-shell | 9.694× | 5 | 7 / 7 | 0 |
| 4 | puppeteer-chrome | 15.235× | 5 | 7 / 7 | 0 |
| not ranked (partial coverage) | playwright-chrome | 12.004× | 3 | 5 / 7 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-chrome: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-chrome: `cold/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.11.2\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 50 | 56 | 19.886 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 858 | 1096 | 1.110 | 17.16× |
| cold | 1 | puppeteer-chrome | pass | yes | 1239 | 1549 | 0.807 | 24.78× |
| cold | 1 | playwright-chrome | pass | yes | 1240 | 1438 | 0.797 | 24.80× |
| cold | 1 | puppeteer-shell | pass | yes | 938 | 1092 | 1.049 | 18.76× |
| cold-settled | 1 | playwright-shell | pass | no | 172.682 | 187.602 | 5.873 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 154.584 | 159.183 | 0.243 | N/A |
| cold-settled | 1 | shotium | noisy | no | 12.425 | 13.481 | 0.132 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 175.07 | 278.963 | 0.438 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 225.124 | 422.596 | 0.202 | N/A |
| lifecycle | 1 | playwright-chrome | fail | no | 6214.238 | 6720.362 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 80.045 | 162.708 | 11.500 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 878.452 | 1070.367 | 1.108 | 10.97× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2961.041 | 3389.389 | 0.346 | 36.99× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1222.437 | 1393.726 | 0.816 | 15.27× |
| warm | 1 | puppeteer-chrome | noisy | no | 252.171 | 408.784 | 0.209 | N/A |
| warm | 1 | playwright-shell | noisy | no | 177.497 | 221.123 | 0.212 | N/A |
| warm | 1 | shotium | noisy | no | 10.042 | 12.747 | 0.722 | N/A |
| warm | 1 | puppeteer-shell | pass | no | 166.243 | 191.719 | 6.012 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 186.18 | 191.271 | 0.075 | N/A |
| batch | 1 | puppeteer-chrome | pass | no | 303.007 | 4274.867 | 2.391 | N/A |
| batch | 1 | shotium | noisy | no | 7940.441 | 7940.441 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | 15191.616 | 15191.616 | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | 12422.728 | 12422.728 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 12817.33 | 12817.33 | N/A | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 202.004 | 406.027 | 4.354 | 14.32× |
| parallel | 1 | puppeteer-shell | pass | yes | 184.376 | 410.324 | 4.685 | 13.07× |
| parallel | 1 | puppeteer-chrome | pass | yes | 249.439 | 469.288 | 3.660 | 17.69× |
| parallel | 1 | playwright-shell | pass | yes | 199.496 | 441.638 | 4.533 | 14.15× |
| parallel | 1 | shotium | pass | yes | 14.102 | 268.053 | 28.001 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 257.171 | 469.288 | 6.873 | 6.04× |
| parallel | 2 | puppeteer-chrome | pass | yes | 448.291 | 771.516 | 4.223 | 10.53× |
| parallel | 2 | playwright-shell | pass | yes | 251.794 | 495.21 | 6.911 | 5.91× |
| parallel | 2 | shotium | pass | yes | 42.58 | 290.373 | 27.664 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 321.134 | 657.532 | 5.586 | 7.54× |
| parallel | 4 | puppeteer-chrome | pass | yes | 936.591 | 1530.162 | 4.203 | 9.88× |
| parallel | 4 | playwright-shell | pass | yes | 446.922 | 785.974 | 8.406 | 4.72× |
| parallel | 4 | shotium | pass | yes | 94.777 | 349.949 | 27.139 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 552.501 | 2620.916 | 6.363 | 5.83× |
| parallel | 4 | puppeteer-shell | pass | yes | 475.448 | 852.051 | 8.368 | 5.02× |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.891 | 121.704 | 9.677 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 101.36 | 138.115 | 9.038 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 115.414 | 128.578 | 8.686 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 100.871 | 110.996 | 9.719 | N/A |
| resident | 1 | playwright-shell | pass | yes | 1065 | 1228 | 1.020 | 22.19× |
| resident | 1 | playwright-chrome | pass | yes | 766 | 1416 | 1.177 | 15.96× |
| resident | 1 | shotium | pass | yes | 48 | 570 | 8.353 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 619 | 1093 | 1.418 | 12.90× |
| resident | 1 | puppeteer-shell | pass | yes | 647 | 1166 | 1.307 | 13.48× |
| faults | 1 | playwright-chrome | pass | no | 27494.45 | 27494.45 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9126.033 | 9126.033 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 34062.735 | 34062.735 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 32036.042 | 32036.042 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 25403.386 | 25403.386 | N/A | N/A |
| soak | 4 | puppeteer-chrome | pass | yes | 841.114 | 1528.916 | 4.678 | 8.76× |
| soak | 4 | shotium | pass | yes | 96.05 | 417.557 | 27.764 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 452.598 | 938.114 | 8.500 | 4.71× |
| soak | 4 | playwright-shell | pass | yes | 468.736 | 872.537 | 8.230 | 4.88× |
| soak | 4 | playwright-chrome | fail | no | 566.318 | 2405.596 | 3.987 | N/A |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.11.2\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 49 | 53 | 20.057 | N/A |
| cold-settled | 1 | shotium | pass | no | 8.756 | 9.493 | 108.921 | N/A |
| lifecycle | 1 | shotium | pass | no | 72.284 | 122.603 | 13.071 | N/A |
| warm | 1 | shotium | pass | no | 7.524 | 9.977 | 118.521 | N/A |
| batch | 1 | shotium | pass | no | 14.502 | 269.26 | 27.319 | N/A |
| parallel | 1 | shotium | pass | no | 12.499 | 265.897 | 29.630 | N/A |
| parallel | 2 | shotium | pass | no | 35.521 | 279.75 | 28.774 | N/A |
| parallel | 4 | shotium | pass | no | 86.779 | 322.205 | 28.745 | N/A |
| resident | 1 | shotium | pass | no | 514 | 967 | 1.960 | N/A |
| faults | 1 | shotium | pass | no | 10672.672 | 10672.672 | N/A | N/A |
| soak | 4 | shotium | pass | no | 73.623 | 352.449 | 30.635 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 8 / 8 | 8 |
| 2 | playwright-shell | 12.091× | 7 | 8 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 15.241× | 7 | 7 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 21.258× | 7 | 7 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 29.925× | 6 | 7 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.11.2/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 99 | 278 | 8.464 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 1461 | 2583 | 0.655 | 14.76× |
| cold | 1 | puppeteer-chrome | pass | yes | 2658 | 3544 | 0.353 | 26.85× |
| cold | 1 | playwright-chrome | pass | yes | 3276 | 5347 | 0.264 | 33.09× |
| cold | 1 | puppeteer-shell | pass | yes | 1565 | 2024 | 0.669 | 15.81× |
| cold-settled | 1 | playwright-shell | pass | yes | 290.082 | 413.328 | 3.153 | 22.60× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 347.805 | 365.174 | 2.957 | 27.09× |
| cold-settled | 1 | shotium | pass | yes | 12.838 | 20.547 | 72.039 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 840.923 | 988.125 | 1.168 | 65.50× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 454.481 | 560.202 | 2.226 | 35.40× |
| lifecycle | 1 | playwright-chrome | fail | no | 4936.889 | 6950.587 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 106.133 | 177.746 | 9.081 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 1377.57 | 1781.013 | 0.726 | 12.98× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2881.174 | 3764.943 | 0.328 | 27.15× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1705.722 | 2311.508 | 0.585 | 16.07× |
| warm | 1 | puppeteer-chrome | pass | yes | 379.145 | 552.758 | 2.484 | 53.91× |
| warm | 1 | playwright-shell | pass | yes | 285.756 | 368.292 | 3.581 | 40.63× |
| warm | 1 | shotium | pass | yes | 7.033 | 11.888 | 119.439 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 342.98 | 437.187 | 3.036 | 48.77× |
| warm | 1 | playwright-chrome | pass | yes | 801.857 | 930.592 | 1.229 | 114.01× |
| batch | 1 | puppeteer-chrome | pass | yes | 411.299 | 1046.226 | 2.264 | 30.17× |
| batch | 1 | shotium | pass | yes | 13.633 | 258.273 | 28.570 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 875.287 | 1213.852 | 1.129 | 64.20× |
| batch | 1 | playwright-shell | pass | yes | 318.645 | 682.796 | 2.951 | 23.37× |
| batch | 1 | puppeteer-shell | pass | yes | 351.643 | 651.249 | 2.719 | 25.79× |
| parallel | 1 | playwright-chrome | pass | no | 916.008 | 2168.462 | 1.036 | N/A |
| parallel | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | 20098.848 | 20098.848 | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | no | 24158.779 | 24158.779 | N/A | N/A |
| parallel | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | pass | yes | 605.365 | 2167.981 | 2.795 | 5.86× |
| parallel | 2 | shotium | pass | yes | 103.319 | 693.592 | 12.656 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 1613.119 | 3591.445 | 1.152 | 15.61× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1773.023 | 2982.072 | 2.192 | 16.26× |
| parallel | 4 | playwright-shell | pass | yes | 921.577 | 1500.482 | 4.266 | 8.45× |
| parallel | 4 | shotium | pass | yes | 109.01 | 347.892 | 24.880 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 3111.69 | 6277.22 | 1.307 | 28.54× |
| parallel | 4 | puppeteer-shell | pass | yes | 892.617 | 2143.754 | 4.085 | 8.19× |
| reuse-page | 1 | puppeteer-shell | pass | no | 237.502 | 369.584 | 4.069 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 246.718 | 1073.045 | 3.515 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 238.666 | 379.972 | 4.289 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 233.752 | 314.691 | 4.554 | N/A |
| resident | 1 | playwright-shell | pass | yes | 1552 | 1872 | 0.630 | 2.24× |
| resident | 1 | playwright-chrome | pass | yes | 2103 | 2637 | 0.453 | 3.04× |
| resident | 1 | shotium | pass | yes | 692 | 717 | 1.443 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 1989 | 2180 | 0.506 | 2.87× |
| resident | 1 | puppeteer-shell | pass | yes | 1865 | 2161 | 0.529 | 2.70× |
| faults | 1 | playwright-chrome | pass | no | 29491.269 | 29491.269 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5655.669 | 5655.669 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 15801.311 | 15801.311 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 19742.456 | 19742.456 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 22349.847 | 22349.847 | N/A | N/A |
| soak | 4 | puppeteer-chrome | pass | no | 1005.234 | 1922.244 | 3.875 | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 6 eligible cell(s), with 6 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 4 | 6 / 6 | 6 |
| 2 | playwright-shell | 12.968× | 4 | 6 / 6 | 0 |
| 3 | puppeteer-shell | 17.141× | 4 | 6 / 6 | 0 |
| 4 | puppeteer-chrome | 32.130× | 4 | 6 / 6 | 0 |
| not ranked (partial coverage) | playwright-chrome | 18.964× | 3 | 5 / 6 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.11.2/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 62 | 86 | 16.092 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 911 | 1111 | 1.120 | 14.69× |
| cold | 1 | puppeteer-chrome | pass | yes | 2261 | 4425 | 0.409 | 36.47× |
| cold | 1 | playwright-chrome | pass | yes | 2302 | 3949 | 0.364 | 37.13× |
| cold | 1 | puppeteer-shell | pass | yes | 921 | 1608 | 0.930 | 14.85× |
| cold-settled | 1 | playwright-shell | pass | yes | 168.129 | 400.201 | 4.756 | 21.31× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 258.889 | 360.433 | 3.628 | 32.82× |
| cold-settled | 1 | shotium | pass | yes | 7.889 | 12.92 | 113.055 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 212.432 | 274.334 | 4.650 | 26.93× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 394.31 | 596.034 | 2.408 | 49.98× |
| lifecycle | 1 | playwright-chrome | fail | no | 4007.881 | 6012.021 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 48.392 | 113.499 | 17.073 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 659.195 | 1250.757 | 1.386 | 13.62× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1887.419 | 2807.312 | 0.530 | 39.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 794.657 | 1678.446 | 1.198 | 16.42× |
| warm | 1 | puppeteer-chrome | noisy | no | 285.333 | 348.593 | 3.650 | N/A |
| warm | 1 | playwright-shell | noisy | no | 130.83 | 143.565 | 7.852 | N/A |
| warm | 1 | shotium | noisy | no | 4.956 | 10.065 | 166.456 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 193.361 | 326.851 | 4.749 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 165.983 | 190.559 | 6.019 | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 10523.108 | 10523.108 | N/A | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 205.114 | 1373.801 | 3.566 | 20.89× |
| parallel | 1 | puppeteer-shell | pass | yes | 290.358 | 633.925 | 3.192 | 29.57× |
| parallel | 1 | puppeteer-chrome | pass | yes | 416.815 | 1436.989 | 2.216 | 42.44× |
| parallel | 1 | playwright-shell | pass | yes | 184.737 | 455.728 | 5.006 | 18.81× |
| parallel | 1 | shotium | pass | yes | 9.821 | 267.901 | 32.850 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 347.337 | 695.862 | 5.455 | 13.29× |
| parallel | 2 | puppeteer-chrome | pass | yes | 592.875 | 1281.082 | 3.362 | 22.69× |
| parallel | 2 | playwright-shell | pass | yes | 234.286 | 477.682 | 7.706 | 8.97× |
| parallel | 2 | shotium | pass | yes | 26.133 | 281.833 | 32.747 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 281.743 | 1061.109 | 6.366 | 10.78× |
| parallel | 4 | puppeteer-chrome | pass | yes | 796.011 | 1597.562 | 4.839 | 16.07× |
| parallel | 4 | playwright-shell | pass | yes | 327.502 | 819.351 | 10.710 | 6.61× |
| parallel | 4 | shotium | pass | yes | 49.53 | 328.76 | 34.974 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 539.667 | 1442.413 | 6.787 | 10.90× |
| parallel | 4 | puppeteer-shell | pass | yes | 399.421 | 769.358 | 9.321 | 8.06× |
| reuse-page | 1 | puppeteer-shell | pass | no | 158.437 | 212.849 | 6.209 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 174.647 | 215.721 | 5.686 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 96.39 | 133.836 | 10.228 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 90.922 | 154.157 | 10.065 | N/A |
| resident | 1 | playwright-shell | pass | no | 823 | 1055 | 1.177 | N/A |
| resident | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 24152.287 | 24152.287 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4153.299 | 4153.299 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 8842.604 | 8842.604 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 19609.103 | 19609.103 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 18740.699 | 18740.699 | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |

