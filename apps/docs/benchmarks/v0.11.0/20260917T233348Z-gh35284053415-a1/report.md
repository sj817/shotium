# Shotium 0.11.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `f258ede499a685f00854290f0deb0d225000ac0f`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 2 | 8 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 4 | 8 |
| darwin-arm64 | noisy | shotium | 4 | 4 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 8.673× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 9.060× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 12.326× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 10.998× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.11.0/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 56 | 59 | 17.857 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 492 | 521 | 2.028 | 8.79× |
| cold | 1 | puppeteer-chrome | pass | yes | 686 | 716 | 1.447 | 12.25× |
| cold | 1 | playwright-shell | pass | yes | 653 | 672 | 1.540 | 11.66× |
| cold | 1 | playwright-chrome | pass | yes | 811 | 969 | 1.206 | 14.48× |
| cold-settled | 1 | playwright-shell | pass | yes | 131.649 | 139.592 | 7.576 | 12.25× |
| cold-settled | 1 | playwright-chrome | pass | yes | 154.285 | 160.056 | 6.770 | 14.36× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 165.997 | 186.513 | 6.009 | 15.45× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 124.329 | 140.063 | 7.845 | 11.57× |
| cold-settled | 1 | shotium | pass | yes | 10.744 | 13.062 | 88.193 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 756.449 | 820.753 | 1.319 | 13.54× |
| lifecycle | 1 | shotium | pass | yes | 55.855 | 75.645 | 16.975 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 583.887 | 636.03 | 1.703 | 10.45× |
| lifecycle | 1 | playwright-chrome | fail | no | 1657.731 | 1836.293 | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 531.698 | 628.244 | 1.871 | 9.52× |
| warm | 1 | puppeteer-shell | pass | yes | 133.142 | 138.76 | 7.554 | 16.13× |
| warm | 1 | shotium | pass | yes | 8.254 | 15.406 | 99.183 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 141.025 | 150.497 | 7.068 | 17.09× |
| warm | 1 | playwright-chrome | pass | yes | 172.385 | 184.428 | 5.749 | 20.89× |
| warm | 1 | puppeteer-chrome | pass | yes | 182.564 | 203.453 | 5.608 | 22.12× |
| batch | 1 | shotium | pass | yes | 12.495 | 255.978 | 29.362 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 153.969 | 389.101 | 5.411 | 12.32× |
| batch | 1 | puppeteer-shell | pass | yes | 148.284 | 385.045 | 5.618 | 11.87× |
| batch | 1 | playwright-chrome | pass | yes | 184.658 | 425.243 | 4.648 | 14.78× |
| batch | 1 | puppeteer-chrome | pass | yes | 215.473 | 443.287 | 4.121 | 17.24× |
| parallel | 1 | playwright-chrome | pass | yes | 152.752 | 376.775 | 5.693 | 18.15× |
| parallel | 1 | playwright-shell | pass | yes | 123.622 | 342.946 | 7.034 | 14.69× |
| parallel | 1 | puppeteer-shell | pass | yes | 119.674 | 359.3 | 6.843 | 14.22× |
| parallel | 1 | puppeteer-chrome | pass | yes | 170.57 | 420.758 | 5.211 | 20.27× |
| parallel | 1 | shotium | pass | yes | 8.416 | 257.391 | 34.563 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 167.914 | 397.373 | 10.247 | 8.21× |
| parallel | 2 | puppeteer-shell | pass | yes | 192.899 | 420.532 | 9.554 | 9.44× |
| parallel | 2 | puppeteer-chrome | pass | yes | 327.393 | 602.817 | 5.774 | 16.01× |
| parallel | 2 | shotium | pass | yes | 20.444 | 267.846 | 36.025 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 226.371 | 566.573 | 7.841 | 11.07× |
| parallel | 4 | puppeteer-shell | pass | yes | 324.01 | 579.46 | 11.711 | 5.69× |
| parallel | 4 | puppeteer-chrome | pass | yes | 508.328 | 783.17 | 7.521 | 8.92× |
| parallel | 4 | shotium | pass | yes | 56.984 | 292.252 | 35.147 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 398.769 | 692 | 9.182 | 7.00× |
| parallel | 4 | playwright-shell | pass | yes | 293.771 | 540.383 | 11.811 | 5.16× |
| reuse-page | 1 | playwright-chrome | pass | no | 99.974 | 115.121 | 9.848 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.015 | 101.786 | 10.114 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.874 | 107.662 | 9.993 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 99.923 | 101.486 | 10.010 | N/A |
| resident | 1 | playwright-shell | pass | yes | 983 | 1041 | 1.019 | 3.82× |
| resident | 1 | playwright-chrome | pass | yes | 1040 | 1048 | 1.029 | 4.05× |
| resident | 1 | puppeteer-chrome | pass | yes | 912 | 959 | 1.099 | 3.55× |
| resident | 1 | shotium | pass | yes | 257 | 322 | 4.318 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 857 | 889 | 1.253 | 3.33× |
| faults | 1 | puppeteer-shell | pass | no | 9239.121 | 9239.121 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14357.075 | 14357.075 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 9701.302 | 9701.302 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 14546.201 | 14546.201 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4355.934 | 4355.934 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 430.952 | 920.597 | 8.811 | 6.45× |
| soak | 4 | shotium | pass | yes | 66.863 | 347.104 | 33.351 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 538.697 | 908.145 | 7.199 | 8.06× |
| soak | 4 | playwright-shell | pass | yes | 332.896 | 661.379 | 11.054 | 4.98× |
| soak | 4 | puppeteer-shell | pass | yes | 341.604 | 647.619 | 11.230 | 5.11× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.530× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.622× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.11.0/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 833 | 2304 | 0.958 | 15.43× |
| cold | 1 | playwright-shell | pass | yes | 642 | 659 | 1.558 | 11.89× |
| cold | 1 | shotium | pass | yes | 54 | 59 | 18.088 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 21.381 | 25.758 | 44.708 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 129.03 | 143.876 | 7.403 | 6.03× |
| cold-settled | 1 | playwright-chrome | pass | yes | 168.779 | 181.836 | 5.886 | 7.89× |
| lifecycle | 1 | playwright-shell | pass | yes | 575.643 | 658.622 | 1.704 | 9.15× |
| lifecycle | 1 | shotium | pass | yes | 62.878 | 77.182 | 15.841 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 1473.271 | 1817.467 | N/A | N/A |
| warm | 1 | playwright-chrome | pass | yes | 166.196 | 185.43 | 5.999 | 9.29× |
| warm | 1 | shotium | pass | yes | 17.896 | 22.373 | 52.015 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 132.859 | 146.685 | 7.567 | 7.42× |
| batch | 1 | playwright-shell | pass | yes | 142.104 | 384.335 | 5.939 | 5.84× |
| batch | 1 | playwright-chrome | pass | yes | 176.426 | 404.548 | 4.973 | 7.25× |
| batch | 1 | shotium | pass | yes | 24.345 | 257.101 | 23.728 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 139.879 | 388.364 | 5.997 | 5.81× |
| parallel | 1 | playwright-chrome | pass | yes | 179.594 | 425.699 | 4.854 | 7.46× |
| parallel | 1 | shotium | pass | yes | 24.071 | 257.296 | 23.878 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 255.706 | 546.312 | 7.263 | 5.01× |
| parallel | 2 | shotium | pass | yes | 51.089 | 284.776 | 23.931 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 197.2 | 427.4 | 9.003 | 3.86× |
| parallel | 4 | shotium | pass | yes | 107.833 | 348.884 | 23.890 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 326.457 | 547.017 | 11.155 | 3.03× |
| parallel | 4 | playwright-chrome | pass | yes | 420.801 | 634.194 | 9.028 | 3.90× |
| reuse-page | 1 | playwright-chrome | pass | no | 88.198 | 116.386 | 10.837 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.514 | 112.751 | 11.306 | N/A |
| resident | 1 | shotium | pass | yes | 187 | 312 | 4.892 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 859 | 915 | 1.163 | 4.59× |
| resident | 1 | playwright-chrome | pass | yes | 942 | 971 | 1.117 | 5.04× |
| faults | 1 | playwright-shell | pass | no | 14815.926 | 14815.926 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 15425.709 | 15425.709 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4708.606 | 4708.606 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 112.164 | 373.69 | 23.649 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 456.53 | 798.522 | 8.439 | 4.07× |
| soak | 4 | playwright-shell | pass | yes | 337.968 | 651.225 | 10.994 | 3.01× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 8 / 8 | 8 |
| 2 | playwright-shell | 11.206× | 6 | 8 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 10.772× | 5 | 7 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 13.291× | 5 | 7 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 16.410× | 4 | 6 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.11.0\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 42 | 48 | 23.569 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 765 | 925 | 1.283 | 18.21× |
| cold | 1 | puppeteer-chrome | pass | yes | 935 | 1092 | 1.058 | 22.26× |
| cold | 1 | playwright-shell | pass | yes | 710 | 787 | 1.393 | 16.90× |
| cold | 1 | playwright-chrome | pass | yes | 838 | 1078 | 1.146 | 19.95× |
| cold-settled | 1 | playwright-shell | pass | no | 149.867 | 175.116 | 6.407 | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 143.302 | 152.262 | 6.930 | N/A |
| cold-settled | 1 | puppeteer-chrome | pass | no | 174.116 | 185.862 | 5.708 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | no | 138.339 | 153.624 | 7.200 | N/A |
| cold-settled | 1 | shotium | noisy | no | 10.617 | 11.874 | 0.703 | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2016.543 | 3566.172 | 0.482 | 27.78× |
| lifecycle | 1 | shotium | pass | yes | 72.586 | 151.653 | 11.935 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 773.447 | 1430.674 | 1.144 | 10.66× |
| lifecycle | 1 | playwright-chrome | fail | no | 7338.628 | 8319.238 | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1057.497 | 3415.556 | 0.712 | 14.57× |
| warm | 1 | puppeteer-shell | noisy | no | 138.225 | 152.912 | 1.458 | N/A |
| warm | 1 | shotium | pass | yes | 6.33 | 8.284 | 144.195 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 121.845 | 166.82 | 7.629 | 19.25× |
| warm | 1 | playwright-chrome | pass | yes | 153.442 | 212.882 | 6.375 | 24.24× |
| warm | 1 | puppeteer-chrome | noisy | no | 178.185 | 241.295 | 1.254 | N/A |
| batch | 1 | shotium | pass | yes | 11.69 | 268.336 | 28.251 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 152.008 | 414.781 | 5.873 | 13.00× |
| batch | 1 | puppeteer-shell | pass | yes | 146.192 | 395.108 | 5.997 | 12.51× |
| batch | 1 | playwright-chrome | pass | yes | 167.354 | 413.014 | 5.339 | 14.32× |
| batch | 1 | puppeteer-chrome | pass | yes | 206.989 | 482.536 | 4.401 | 17.71× |
| parallel | 1 | playwright-chrome | pass | yes | 212.348 | 425.261 | 4.347 | 14.77× |
| parallel | 1 | playwright-shell | pass | yes | 200.864 | 461.603 | 4.505 | 13.97× |
| parallel | 1 | puppeteer-shell | pass | yes | 182.301 | 486.544 | 4.756 | 12.68× |
| parallel | 1 | puppeteer-chrome | pass | yes | 276.811 | 536.004 | 3.370 | 19.25× |
| parallel | 1 | shotium | pass | yes | 14.38 | 270.545 | 27.503 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 254.581 | 467.104 | 7.013 | 5.76× |
| parallel | 2 | puppeteer-shell | pass | yes | 272.03 | 536.845 | 6.625 | 6.16× |
| parallel | 2 | puppeteer-chrome | pass | yes | 464.871 | 937.112 | 4.174 | 10.52× |
| parallel | 2 | shotium | pass | yes | 44.169 | 275.749 | 27.253 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 334.575 | 809.281 | 5.576 | 7.57× |
| parallel | 4 | puppeteer-shell | pass | yes | 568.505 | 839.104 | 6.954 | 5.81× |
| parallel | 4 | puppeteer-chrome | pass | yes | 860.698 | 1418.709 | 4.612 | 8.80× |
| parallel | 4 | shotium | pass | yes | 97.793 | 340.927 | 26.786 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 662.779 | 1647.536 | 5.710 | 6.78× |
| parallel | 4 | playwright-shell | pass | yes | 465.381 | 674.209 | 8.336 | 4.76× |
| reuse-page | 1 | playwright-chrome | pass | no | 100.332 | 115.109 | 9.890 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 98.049 | 130.686 | 10.503 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.999 | 116.771 | 9.741 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 96.374 | 118.629 | 11.015 | N/A |
| resident | 1 | playwright-shell | pass | yes | 662 | 1533 | 1.267 | 14.39× |
| resident | 1 | playwright-chrome | pass | yes | 642 | 929 | 1.546 | 13.96× |
| resident | 1 | puppeteer-chrome | fail | no | 587 | 1126 | 0.141 | N/A |
| resident | 1 | shotium | pass | yes | 46 | 381 | 7.752 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 514 | 801 | 1.832 | 11.17× |
| faults | 1 | puppeteer-shell | pass | no | 28755.339 | 28755.339 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 25161.131 | 25161.131 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 23082.685 | 23082.685 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 32695.686 | 32695.686 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9023.243 | 9023.243 | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 546.571 | 1550.654 | 4.097 | N/A |
| soak | 4 | shotium | noisy | no | 7798.486 | 7798.486 | N/A | N/A |
| soak | 4 | puppeteer-chrome | pass | no | 759.872 | 1355.357 | 5.163 | N/A |
| soak | 4 | playwright-shell | pass | no | 474.311 | 1014.475 | 8.181 | N/A |
| soak | 4 | puppeteer-shell | pass | no | 441.184 | 965.724 | 8.778 | N/A |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.11.0\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 49 | 51 | 20.772 | N/A |
| cold-settled | 1 | shotium | noisy | no | 8.829 | 9.11 | 0.156 | N/A |
| lifecycle | 1 | shotium | pass | no | 75.655 | 131.119 | 12.591 | N/A |
| warm | 1 | shotium | noisy | no | 6.433 | 8.848 | 0.297 | N/A |
| batch | 1 | shotium | pass | no | 12.653 | 267.588 | 28.339 | N/A |
| parallel | 1 | shotium | pass | no | 12.92 | 271.341 | 26.580 | N/A |
| parallel | 2 | shotium | pass | no | 28.996 | 278.57 | 31.793 | N/A |
| parallel | 4 | shotium | pass | no | 69.943 | 326.116 | 31.022 | N/A |
| resident | 1 | shotium | pass | no | 274 | 1039 | 2.373 | N/A |
| faults | 1 | shotium | pass | no | 11764.488 | 11764.488 | N/A | N/A |
| soak | 4 | shotium | pass | no | 80.092 | 344.497 | 29.678 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 8 / 8 | 8 |
| 2 | puppeteer-shell | 11.904× | 8 | 8 / 8 | 0 |
| 3 | playwright-shell | 11.983× | 8 | 8 / 8 | 0 |
| 4 | puppeteer-chrome | 19.334× | 8 | 8 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 29.758× | 6 | 6 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.11.0/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 113 | 141 | 8.454 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 1900 | 2292 | 0.516 | 16.81× |
| cold | 1 | puppeteer-chrome | pass | yes | 3524 | 5982 | 0.259 | 31.19× |
| cold | 1 | playwright-shell | pass | yes | 1890 | 2041 | 0.528 | 16.73× |
| cold | 1 | playwright-chrome | pass | yes | 4482 | 5265 | 0.222 | 39.66× |
| cold-settled | 1 | playwright-shell | pass | yes | 426.637 | 456.234 | 2.526 | 24.47× |
| cold-settled | 1 | playwright-chrome | pass | yes | 899.252 | 1018.103 | 1.102 | 51.58× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 495.88 | 785.438 | 1.931 | 28.44× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 364.102 | 444.074 | 2.806 | 20.89× |
| cold-settled | 1 | shotium | pass | yes | 17.433 | 18.44 | 58.048 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3844.985 | 4661.922 | 0.251 | 27.41× |
| lifecycle | 1 | shotium | pass | yes | 140.289 | 285.286 | 6.628 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 1860.942 | 4886.703 | 0.494 | 13.27× |
| lifecycle | 1 | playwright-chrome | fail | no | 7158.299 | 8559.806 | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 2146.019 | 2642.744 | 0.455 | 15.30× |
| warm | 1 | puppeteer-shell | pass | yes | 363.323 | 462.894 | 2.804 | 22.82× |
| warm | 1 | shotium | pass | yes | 15.923 | 26.348 | 55.836 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 419.751 | 651.467 | 2.315 | 26.36× |
| warm | 1 | playwright-chrome | pass | yes | 911.155 | 1137.749 | 1.075 | 57.22× |
| warm | 1 | puppeteer-chrome | pass | yes | 659.081 | 1819.075 | 1.348 | 41.39× |
| batch | 1 | shotium | pass | yes | 26.975 | 373.219 | 19.159 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 354.055 | 751.189 | 2.663 | 13.13× |
| batch | 1 | puppeteer-shell | pass | yes | 393.483 | 740.801 | 2.368 | 14.59× |
| batch | 1 | playwright-chrome | pass | yes | 928.901 | 1281.643 | 1.066 | 34.44× |
| batch | 1 | puppeteer-chrome | pass | yes | 565.946 | 2553.47 | 1.521 | 20.98× |
| parallel | 1 | playwright-chrome | pass | yes | 932.708 | 1594.665 | 1.043 | 59.33× |
| parallel | 1 | playwright-shell | pass | yes | 344.799 | 747.901 | 2.661 | 21.93× |
| parallel | 1 | puppeteer-shell | pass | yes | 365.822 | 833.843 | 2.575 | 23.27× |
| parallel | 1 | puppeteer-chrome | pass | yes | 469.147 | 2214.804 | 1.773 | 29.84× |
| parallel | 1 | shotium | pass | yes | 15.722 | 257.386 | 27.002 | 1.00× |
| parallel | 2 | playwright-shell | pass | no | 357.126 | 904.004 | 5.106 | N/A |
| parallel | 2 | puppeteer-shell | pass | no | 395.201 | 729.018 | 4.746 | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | pass | no | 616.407 | 930.004 | 6.436 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 237.506 | 315.208 | 4.402 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 248.629 | 348.19 | 3.758 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 245.815 | 348.964 | 3.886 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 215.946 | 280.809 | 4.554 | N/A |
| resident | 1 | playwright-shell | pass | yes | 1712 | 2110 | 0.563 | 2.04× |
| resident | 1 | playwright-chrome | pass | yes | 2433 | 3077 | 0.398 | 2.90× |
| resident | 1 | puppeteer-chrome | pass | yes | 1924 | 3229 | 0.476 | 2.30× |
| resident | 1 | shotium | pass | yes | 838 | 1000 | 1.179 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 1785 | 3349 | 0.513 | 2.13× |
| faults | 1 | puppeteer-shell | pass | no | 18185.457 | 18185.457 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 20231.965 | 20231.965 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 25194.419 | 25194.419 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 30773.846 | 30773.846 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6088.973 | 6088.973 | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | N/A | N/A | N/A | N/A |
| soak | 4 | shotium | pass | yes | 141.094 | 587.292 | 21.126 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 1904.284 | 6076.373 | 1.971 | 13.50× |
| soak | 4 | playwright-shell | pass | yes | 712.649 | 1784.63 | 5.441 | 5.05× |
| soak | 4 | puppeteer-shell | pass | yes | 641.988 | 1587.334 | 6.058 | 4.55× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 4 eligible cell(s), with 4 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 4 | 4 / 4 | 4 |
| 2 | playwright-shell | 9.040× | 4 | 4 / 4 | 0 |
| 3 | puppeteer-shell | 11.473× | 4 | 4 / 4 | 0 |
| 4 | puppeteer-chrome | 18.897× | 4 | 4 / 4 | 0 |
| not ranked (partial coverage) | playwright-chrome | 13.625× | 3 | 3 / 4 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.11.0/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 47 | 59 | 21.944 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 739 | 1061 | 1.347 | 15.72× |
| cold | 1 | puppeteer-chrome | pass | yes | 1477 | 5259 | 0.488 | 31.43× |
| cold | 1 | playwright-shell | pass | yes | 649 | 741 | 1.577 | 13.81× |
| cold | 1 | playwright-chrome | pass | yes | 1508 | 3335 | 0.581 | 32.09× |
| cold-settled | 1 | playwright-shell | pass | yes | 140.967 | 218.278 | 6.131 | 17.63× |
| cold-settled | 1 | playwright-chrome | pass | yes | 200.489 | 242.741 | 4.703 | 25.08× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 327.529 | 486.713 | 2.842 | 40.97× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 224.774 | 307.273 | 4.152 | 28.11× |
| cold-settled | 1 | shotium | pass | yes | 7.995 | 17.195 | 102.581 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1922.862 | 2752.756 | 0.506 | 32.88× |
| lifecycle | 1 | shotium | pass | yes | 58.486 | 131.553 | 15.284 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 680.789 | 1177.675 | 1.354 | 11.64× |
| lifecycle | 1 | playwright-chrome | fail | no | 4045.758 | 4383.283 | N/A | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 876.649 | 2234.326 | 1.036 | 14.99× |
| warm | 1 | puppeteer-shell | noisy | no | 198.713 | 256.197 | 5.005 | N/A |
| warm | 1 | shotium | noisy | no | 5.281 | 7.013 | 165.625 | N/A |
| warm | 1 | playwright-shell | noisy | no | 154.776 | 276.86 | 5.888 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 229.261 | 320.196 | 4.217 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 299.791 | 372.765 | 3.272 | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-chrome | pass | no | 240.433 | 1986.4 | 3.101 | N/A |
| batch | 1 | puppeteer-chrome | pass | no | 436.352 | 2019.794 | 1.954 | N/A |
| parallel | 1 | playwright-chrome | pass | no | 223.375 | 1537.795 | 3.384 | N/A |
| parallel | 1 | playwright-shell | pass | no | 168.066 | 429.378 | 5.147 | N/A |
| parallel | 1 | puppeteer-shell | pass | no | 214.949 | 443.783 | 4.368 | N/A |
| parallel | 1 | puppeteer-chrome | pass | no | 420.967 | 2168.042 | 1.914 | N/A |
| parallel | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | pass | no | 346.64 | 709.493 | 10.690 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 103.434 | 296.023 | 9.006 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 148.787 | 229.622 | 6.640 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 211.187 | 274.835 | 4.676 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.006 | 106.75 | 12.267 | N/A |
| resident | 1 | playwright-shell | pass | yes | 740 | 840 | 1.387 | 2.36× |
| resident | 1 | playwright-chrome | pass | yes | 987 | 1639 | 0.964 | 3.14× |
| resident | 1 | puppeteer-chrome | pass | yes | 946 | 1467 | 1.001 | 3.01× |
| resident | 1 | shotium | pass | yes | 314 | 355 | 3.140 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 821 | 1552 | 1.082 | 2.61× |
| faults | 1 | puppeteer-shell | pass | no | 11747.56 | 11747.56 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 22130.842 | 22130.842 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 26344.928 | 26344.928 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 22533.096 | 22533.096 | N/A | N/A |
| faults | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |

