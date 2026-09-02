# Shotium 0.3.4 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **incomplete**; quality: **fail**; evidence: **incomplete**. Profile **full**, seed `6bf4cb2fbc8e69e59371281916ce6bf49a73ef24`.

Conclusion: this run is incomplete. 4 platform(s) contain valid within-platform comparisons; missing outputs are never inferred.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | fail | shotium | 4 | 9 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | infra-error | shotium | 2 | 2 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | infra-error | no valid ranking | 0 | 0 |
| darwin-arm64 | infra-error | shotium | 3 | 2 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 4.754× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 5.455× | 7 | 9 / 9 | 0 |
| 4 | playwright-chrome | 6.309× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 8.342× | 6 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | noisy | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 519 | 536 | 1.927 | 8.51× |
| cold | 1 | playwright-shell | pass | yes | 336 | 361 | 2.949 | 5.51× |
| cold | 1 | puppeteer-chrome | pass | yes | 577 | 594 | 1.748 | 9.46× |
| cold | 1 | puppeteer-shell | pass | yes | 359 | 372 | 2.787 | 5.89× |
| cold | 1 | shotium | pass | yes | 61 | 62 | 16.548 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 159.077 | 164.147 | 6.535 | 10.70× |
| cold-settled | 1 | playwright-chrome | pass | yes | 123.445 | 147.671 | 7.655 | 8.30× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 128.029 | 142.348 | 7.668 | 8.61× |
| cold-settled | 1 | shotium | pass | yes | 14.865 | 15.114 | 66.705 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 106.029 | 110.874 | 9.457 | 7.13× |
| lifecycle | 1 | playwright-shell | pass | yes | 273.37 | 318.447 | 3.667 | 4.74× |
| lifecycle | 1 | playwright-chrome | pass | yes | 478.219 | 527.881 | 2.084 | 8.29× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 323.41 | 373.778 | 3.057 | 5.60× |
| lifecycle | 1 | shotium | pass | yes | 57.71 | 110.964 | 14.627 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 630.545 | 714.769 | 1.593 | 10.93× |
| warm | 1 | playwright-chrome | pass | yes | 116.342 | 228.827 | 7.777 | 9.76× |
| warm | 1 | shotium | pass | yes | 11.917 | 15.214 | 78.526 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 99.481 | 106.803 | 10.329 | 8.35× |
| warm | 1 | puppeteer-shell | pass | yes | 115.146 | 117.722 | 8.952 | 9.66× |
| warm | 1 | puppeteer-chrome | pass | yes | 127.848 | 143.259 | 7.890 | 10.73× |
| batch | 1 | shotium | pass | yes | 22.018 | 267.071 | 23.980 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 121.057 | 341.736 | 6.596 | 5.50× |
| batch | 1 | playwright-shell | pass | yes | 107.483 | 466.269 | 7.118 | 4.88× |
| batch | 1 | puppeteer-chrome | pass | yes | 154.522 | 440.57 | 5.543 | 7.02× |
| batch | 1 | playwright-chrome | pass | yes | 137.067 | 354.897 | 6.107 | 6.23× |
| parallel | 1 | playwright-chrome | pass | yes | 160.183 | 366.453 | 5.457 | 5.98× |
| parallel | 1 | puppeteer-shell | pass | yes | 137.377 | 353.393 | 5.915 | 5.13× |
| parallel | 1 | puppeteer-chrome | pass | yes | 175.902 | 386.747 | 4.990 | 6.57× |
| parallel | 1 | shotium | pass | yes | 26.77 | 273.592 | 21.726 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 125.166 | 338.547 | 6.510 | 4.68× |
| parallel | 2 | puppeteer-chrome | pass | yes | 286.03 | 486.467 | 6.002 | 5.14× |
| parallel | 2 | shotium | pass | yes | 55.662 | 281.932 | 22.006 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 218.558 | 446.772 | 7.713 | 3.93× |
| parallel | 2 | playwright-chrome | pass | yes | 267.679 | 429.696 | 6.576 | 4.81× |
| parallel | 2 | puppeteer-shell | pass | yes | 236.995 | 402.335 | 7.083 | 4.26× |
| parallel | 4 | playwright-shell | pass | yes | 391.681 | 751.624 | 8.490 | 3.37× |
| parallel | 4 | playwright-chrome | pass | yes | 506.558 | 705.12 | 7.227 | 4.36× |
| parallel | 4 | puppeteer-shell | pass | yes | 474.744 | 718.788 | 7.611 | 4.08× |
| parallel | 4 | puppeteer-chrome | fail | no | 526.249 | 149484.84 | 0.579 | N/A |
| parallel | 4 | shotium | pass | yes | 116.286 | 324.496 | 22.083 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 65.926 | 84.37 | 15.520 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 67.981 | 95.569 | 13.574 | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | 3033.727 | 3033.727 | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 83.278 | 101.907 | 11.853 | N/A |
| resident | 1 | puppeteer-shell | pass | no | 886 | 933 | 1.191 | N/A |
| resident | 1 | shotium | noisy | no | 177 | 509 | 0.629 | N/A |
| resident | 1 | playwright-shell | pass | no | 982 | 1027 | 1.015 | N/A |
| resident | 1 | playwright-chrome | pass | no | 1016 | 1118 | 1.132 | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 20006.348 | 23471.947 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 9880.462 | 9880.462 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 11042.705 | 11042.705 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9539.372 | 9539.372 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13603.551 | 13603.551 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 15234.7 | 15234.7 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 287.883 | 644.444 | 12.513 | 2.64× |
| soak | 4 | puppeteer-chrome | fail | no | 440.828 | 892.298 | 5.800 | N/A |
| soak | 4 | playwright-chrome | pass | yes | 386.491 | 1050.254 | 9.662 | 3.55× |
| soak | 4 | puppeteer-shell | pass | yes | 345.946 | 726.112 | 10.958 | 3.18× |
| soak | 4 | shotium | pass | yes | 108.869 | 356.507 | 25.984 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 2.596× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 3.342× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 434 | 547 | 2.232 | 6.68× |
| cold | 1 | playwright-shell | pass | yes | 271 | 886 | 2.800 | 4.17× |
| cold | 1 | shotium | pass | yes | 65 | 68 | 15.217 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 101.217 | 115.978 | 9.840 | 2.92× |
| cold-settled | 1 | playwright-chrome | pass | yes | 127.492 | 142.144 | 7.737 | 3.68× |
| cold-settled | 1 | shotium | pass | yes | 34.657 | 42.539 | 27.368 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 237.54 | 294.593 | 4.134 | 3.28× |
| lifecycle | 1 | playwright-chrome | pass | yes | 396.603 | 472.947 | 2.465 | 5.47× |
| lifecycle | 1 | shotium | pass | yes | 72.459 | 99.108 | 12.924 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 129.439 | 153.448 | 7.773 | 3.98× |
| warm | 1 | playwright-shell | pass | yes | 105.338 | 126.478 | 9.201 | 3.24× |
| warm | 1 | shotium | pass | yes | 32.486 | 37.066 | 29.498 | 1.00× |
| batch | 1 | shotium | pass | yes | 44.465 | 263.428 | 16.431 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 126.647 | 339.498 | 6.742 | 2.85× |
| batch | 1 | playwright-chrome | pass | yes | 149.675 | 367.889 | 5.860 | 3.37× |
| parallel | 1 | playwright-chrome | pass | yes | 139.078 | 355.827 | 6.215 | 3.25× |
| parallel | 1 | shotium | pass | yes | 42.783 | 270.237 | 17.022 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 119.183 | 338.199 | 7.249 | 2.79× |
| parallel | 2 | shotium | pass | yes | 84.639 | 294.043 | 17.291 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 180.278 | 343.093 | 9.425 | 2.13× |
| parallel | 2 | playwright-chrome | pass | yes | 217.834 | 389.065 | 7.805 | 2.57× |
| parallel | 4 | playwright-shell | pass | yes | 329.95 | 459.167 | 10.691 | 1.95× |
| parallel | 4 | playwright-chrome | pass | yes | 405.632 | 544.362 | 8.547 | 2.39× |
| parallel | 4 | shotium | pass | yes | 169.544 | 368.191 | 17.177 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 49.958 | 50.873 | 20.055 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 58.438 | 83.241 | 16.892 | N/A |
| resident | 1 | playwright-shell | pass | yes | 810 | 875 | 1.374 | 2.02× |
| resident | 1 | playwright-chrome | pass | yes | 866 | 901 | 1.186 | 2.16× |
| resident | 1 | shotium | pass | yes | 401 | 441 | 2.803 | 1.00× |
| faults | 1 | shotium | pass | no | 10276.45 | 10276.45 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16200.035 | 16200.035 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15886.45 | 15886.45 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 432.943 | 752.568 | 8.950 | 2.23× |
| soak | 4 | playwright-shell | pass | yes | 314.417 | 588.146 | 11.754 | 1.62× |
| soak | 4 | shotium | pass | yes | 194.275 | 444.092 | 16.648 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 2 eligible cell(s), with 2 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 2 | 2 / 2 | 2 |
| 2 | playwright-shell | 5.328× | 2 | 2 / 2 | 0 |
| not ranked (partial coverage) | playwright-chrome | 3.530× | 1 | 1 / 2 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 10.021× | 1 | 1 / 2 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 10.777× | 1 | 1 / 2 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `soak/c4`, `warm/c1`
- playwright-shell: `soak/c4`, `warm/c1`
- playwright-chrome: `soak/c4`
- puppeteer-shell: `warm/c1`
- puppeteer-chrome: `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | infra-error | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| warm | 1 | playwright-chrome | noisy | no | 148.517 | 186.161 | 0.829 | N/A |
| warm | 1 | shotium | pass | yes | 15.589 | 17.801 | 61.362 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 135.781 | 163.754 | 7.060 | 8.71× |
| warm | 1 | puppeteer-shell | pass | yes | 156.213 | 202.837 | 6.090 | 10.02× |
| warm | 1 | puppeteer-chrome | pass | yes | 168.002 | 213.567 | 5.747 | 10.78× |
| batch | 1 | shotium | noisy | no | 38.813 | 878.464 | 3.849 | N/A |
| batch | 1 | puppeteer-shell | pass | no | 182.368 | 415.01 | 4.723 | N/A |
| batch | 1 | playwright-shell | pass | no | 168.13 | 395.681 | 5.114 | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 208.156 | 420.193 | 2.951 | N/A |
| batch | 1 | playwright-chrome | pass | no | 195.695 | 383.843 | 4.495 | N/A |
| faults | 1 | puppeteer-shell | pass | no | 18379.771 | 18379.771 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 29329.436 | 29329.436 | N/A | N/A |
| faults | 1 | shotium | infra-error | no | 10052.356 | 10052.356 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 24893.505 | 24893.505 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 5971.407 | 5971.407 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 368.351 | 753.262 | 10.395 | 3.26× |
| soak | 4 | puppeteer-chrome | fail | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 398.91 | 1014.797 | 9.592 | 3.53× |
| soak | 4 | puppeteer-shell | fail | no | 353.477 | 713.29 | 10.997 | N/A |
| soak | 4 | shotium | pass | yes | 113.013 | 386.039 | 25.066 | 1.00× |

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
| cold | 1 | shotium | pass | no | 69 | 71 | 14.463 | N/A |
| cold-settled | 1 | shotium | pass | no | 15.037 | 16.993 | 64.011 | N/A |
| lifecycle | 1 | shotium | pass | no | 98.125 | 130.092 | 9.881 | N/A |
| warm | 1 | shotium | pass | no | 14.279 | 15.256 | 66.902 | N/A |
| batch | 1 | shotium | pass | no | 32.334 | 274.538 | 19.433 | N/A |
| parallel | 1 | shotium | pass | no | 32.497 | 319.814 | 19.131 | N/A |
| parallel | 2 | shotium | pass | no | 67.922 | 291.74 | 20.221 | N/A |
| parallel | 4 | shotium | pass | no | 131.699 | 376.231 | 20.265 | N/A |
| resident | 1 | shotium | noisy | no | 23852.558 | 25087.051 | N/A | N/A |
| faults | 1 | shotium | pass | no | 13014.669 | 13014.669 | N/A | N/A |
| soak | 4 | shotium | pass | no | 131.949 | 375.061 | 23.085 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 2 eligible cell(s), with 2 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 2 | 2 / 2 | 2 |
| 2 | playwright-shell | 5.032× | 2 | 2 / 2 | 0 |
| 3 | playwright-chrome | 13.427× | 2 | 2 / 2 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 6.481× | 1 | 1 / 2 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 22.827× | 1 | 1 / 2 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `lifecycle/c1`
- playwright-shell: `batch/c1`, `lifecycle/c1`
- playwright-chrome: `batch/c1`, `lifecycle/c1`
- puppeteer-shell: `lifecycle/c1`
- puppeteer-chrome: `lifecycle/c1`

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
| cold | 1 | playwright-chrome | noisy | no | 2795.5 | 7596 | 0.293 | N/A |
| cold | 1 | playwright-shell | noisy | no | 871 | 1381 | 1.184 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 1980.5 | 6055 | 0.339 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 728 | 1145 | 1.342 | N/A |
| cold | 1 | shotium | noisy | no | 101.5 | 117 | 10.084 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 6102.849 | 7228.021 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 178.392 | 232.725 | 5.198 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 229.174 | 262.842 | 1.046 | N/A |
| cold-settled | 1 | shotium | noisy | no | 9.896 | 13.793 | 3.592 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 128.672 | 145.46 | 1.600 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 311.621 | 982.783 | 2.670 | 4.58× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1314.34 | 2061.324 | 0.725 | 19.30× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 441.45 | 838.023 | 2.029 | 6.48× |
| lifecycle | 1 | shotium | pass | yes | 68.113 | 180.537 | 13.686 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1554.836 | 2728.598 | 0.619 | 22.83× |
| warm | 1 | playwright-chrome | noisy | no | 193.474 | 241.877 | 0.642 | N/A |
| warm | 1 | shotium | noisy | no | 9.688 | 12.561 | 5.797 | N/A |
| warm | 1 | playwright-shell | noisy | no | 93.642 | 181.296 | 3.714 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 161.092 | 275.627 | 5.748 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 4772.609 | 8555.174 | N/A | N/A |
| batch | 1 | shotium | pass | yes | 26.624 | 277.756 | 20.808 | 1.00× |
| batch | 1 | puppeteer-shell | noisy | no | 231.969 | 543.791 | 3.389 | N/A |
| batch | 1 | playwright-shell | pass | yes | 147.38 | 371.237 | 6.038 | 5.54× |
| batch | 1 | puppeteer-chrome | noisy | no | 6409.507 | 6872.965 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | yes | 248.74 | 1287.494 | 2.949 | 9.34× |
| reuse-page | 1 | playwright-shell | pass | no | 47.824 | 118.542 | 19.800 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 96.78 | 222.717 | 8.777 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 127.67 | 174.675 | 7.742 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 166.824 | 251.24 | 5.782 | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 838 | 1338 | 1.089 | N/A |
| resident | 1 | shotium | noisy | no | 331 | 331 | 0.019 | N/A |
| resident | 1 | playwright-shell | noisy | no | 916 | 1005 | 1.103 | N/A |
| resident | 1 | playwright-chrome | noisy | no | 1149.5 | 1339 | 0.061 | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 16832.014 | 25077.653 | N/A | N/A |

