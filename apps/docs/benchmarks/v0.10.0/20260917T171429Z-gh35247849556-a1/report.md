# Shotium 0.10.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `a5189426dfcbf1ccf7baff05010689eb3998761c`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 3 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 9 |
| darwin-arm64 | noisy | shotium | 4 | 8 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 7.865× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 8.089× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 11.503× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 10.104× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.10.0/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 722 | 866 | 1.333 | 13.13× |
| cold | 1 | puppeteer-shell | pass | yes | 495 | 598 | 1.955 | 9.00× |
| cold | 1 | playwright-shell | pass | yes | 639 | 700 | 1.549 | 11.62× |
| cold | 1 | playwright-chrome | pass | yes | 842 | 1074 | 1.102 | 15.31× |
| cold | 1 | shotium | pass | yes | 55 | 58 | 18.229 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 11.598 | 13.325 | 83.895 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 163.764 | 278.774 | 5.885 | 14.12× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 119.589 | 157.533 | 7.874 | 10.31× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 168.906 | 201.613 | 5.707 | 14.56× |
| cold-settled | 1 | playwright-shell | pass | yes | 119.402 | 125.334 | 8.287 | 10.30× |
| lifecycle | 1 | playwright-shell | pass | yes | 577.386 | 715.25 | 1.701 | 10.17× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 566.558 | 898.537 | 1.671 | 9.98× |
| lifecycle | 1 | playwright-chrome | fail | no | 2295.878 | 2340.443 | N/A | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 832.329 | 985.56 | 1.184 | 14.65× |
| lifecycle | 1 | shotium | pass | yes | 56.796 | 72.062 | 16.872 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 181.431 | 235.698 | 5.556 | 16.83× |
| warm | 1 | puppeteer-chrome | pass | yes | 192.258 | 216.775 | 5.237 | 17.84× |
| warm | 1 | playwright-shell | pass | yes | 147.287 | 159.515 | 6.876 | 13.66× |
| warm | 1 | puppeteer-shell | pass | yes | 133.539 | 144.379 | 7.339 | 12.39× |
| warm | 1 | shotium | pass | yes | 10.779 | 20.441 | 83.507 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 208.105 | 419.545 | 4.365 | 15.58× |
| batch | 1 | puppeteer-chrome | pass | yes | 237.237 | 462.292 | 3.824 | 17.77× |
| batch | 1 | shotium | pass | yes | 13.354 | 260.173 | 27.611 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 154.545 | 392.677 | 5.422 | 11.57× |
| batch | 1 | playwright-shell | pass | yes | 161.318 | 394.006 | 5.209 | 12.08× |
| parallel | 1 | shotium | pass | yes | 8.856 | 254.157 | 35.635 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 147.065 | 444.201 | 5.878 | 16.61× |
| parallel | 1 | puppeteer-shell | pass | yes | 119.799 | 360.431 | 6.884 | 13.53× |
| parallel | 1 | playwright-shell | pass | yes | 120.463 | 342.197 | 7.172 | 13.60× |
| parallel | 1 | puppeteer-chrome | pass | yes | 171.019 | 408.959 | 5.200 | 19.31× |
| parallel | 2 | playwright-chrome | pass | yes | 221.25 | 493.551 | 8.386 | 10.27× |
| parallel | 2 | puppeteer-shell | pass | yes | 177.626 | 399.425 | 9.869 | 8.24× |
| parallel | 2 | playwright-shell | pass | yes | 158.288 | 425.31 | 10.534 | 7.35× |
| parallel | 2 | puppeteer-chrome | pass | yes | 299.286 | 550.864 | 6.518 | 13.89× |
| parallel | 2 | shotium | pass | yes | 21.55 | 263.296 | 36.265 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 314.916 | 580.818 | 12.048 | 5.85× |
| parallel | 4 | playwright-shell | pass | yes | 287.17 | 535.905 | 12.179 | 5.34× |
| parallel | 4 | puppeteer-chrome | pass | yes | 539.265 | 813.011 | 7.232 | 10.02× |
| parallel | 4 | shotium | pass | yes | 53.812 | 305.487 | 34.874 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 401.538 | 844.799 | 9.199 | 7.46× |
| reuse-page | 1 | playwright-shell | pass | no | 99.956 | 110.868 | 9.911 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 100.54 | 117.723 | 9.569 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.729 | 101.998 | 10.089 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.935 | 116.771 | 9.801 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 985 | 1016 | 1.069 | 2.22× |
| resident | 1 | shotium | pass | yes | 444 | 472 | 2.326 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 923 | 979 | 1.079 | 2.08× |
| resident | 1 | playwright-shell | pass | yes | 998 | 1074 | 1.074 | 2.25× |
| resident | 1 | playwright-chrome | pass | yes | 1091 | 1186 | 0.935 | 2.46× |
| faults | 1 | puppeteer-chrome | pass | no | 14365.748 | 14365.748 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 18634.059 | 18634.059 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16366.1 | 16366.1 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10041.92 | 10041.92 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5068.248 | 5068.248 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 564.407 | 1133.141 | 6.837 | 6.19× |
| soak | 4 | shotium | pass | yes | 91.139 | 352.904 | 29.211 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 698.5 | 1203.036 | 5.578 | 7.66× |
| soak | 4 | puppeteer-shell | pass | yes | 458.561 | 911.734 | 8.435 | 5.03× |
| soak | 4 | playwright-shell | pass | yes | 454.64 | 974.121 | 8.124 | 4.99× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.488× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.358× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.10.0/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 802 | 811 | 1.252 | 15.13× |
| cold | 1 | shotium | pass | yes | 53 | 60 | 18.470 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 641 | 643 | 1.574 | 12.09× |
| cold-settled | 1 | playwright-shell | pass | yes | 129.44 | 130.958 | 7.704 | 5.76× |
| cold-settled | 1 | playwright-chrome | pass | yes | 152.467 | 161.984 | 6.644 | 6.79× |
| cold-settled | 1 | shotium | pass | yes | 22.469 | 29.344 | 42.214 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 1496.996 | 1540.212 | N/A | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 582.217 | 651.575 | 1.713 | 9.66× |
| lifecycle | 1 | shotium | pass | yes | 60.251 | 72.746 | 16.128 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 163.913 | 183.681 | 6.191 | 9.35× |
| warm | 1 | playwright-shell | pass | yes | 132.916 | 153.33 | 7.493 | 7.58× |
| warm | 1 | shotium | pass | yes | 17.538 | 23.61 | 51.940 | 1.00× |
| batch | 1 | shotium | pass | yes | 22.818 | 258.394 | 24.329 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 173.787 | 406.693 | 5.050 | 7.62× |
| batch | 1 | playwright-shell | pass | yes | 145.278 | 375.286 | 5.891 | 6.37× |
| parallel | 1 | shotium | pass | yes | 23.525 | 258.57 | 24.097 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 142.817 | 376.249 | 5.920 | 6.07× |
| parallel | 1 | playwright-chrome | pass | yes | 175.451 | 406.566 | 5.014 | 7.46× |
| parallel | 2 | playwright-shell | pass | yes | 192.755 | 443.871 | 8.976 | 3.79× |
| parallel | 2 | playwright-chrome | pass | yes | 249.423 | 506.78 | 7.430 | 4.91× |
| parallel | 2 | shotium | pass | yes | 50.81 | 295.315 | 24.086 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 429.424 | 692.127 | 8.841 | 3.98× |
| parallel | 4 | shotium | pass | yes | 107.87 | 349.582 | 23.980 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 322.137 | 582.785 | 11.049 | 2.99× |
| reuse-page | 1 | playwright-shell | pass | no | 99.84 | 100.406 | 10.001 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 99.965 | 117.289 | 9.790 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 969 | 981 | 1.036 | 4.07× |
| resident | 1 | playwright-shell | pass | yes | 876 | 891 | 1.144 | 3.68× |
| resident | 1 | shotium | pass | yes | 238 | 308 | 4.888 | 1.00× |
| faults | 1 | playwright-chrome | pass | no | 14538.309 | 14538.309 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14565.49 | 14565.49 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5026.103 | 5026.103 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 107.774 | 377.911 | 24.209 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 421.917 | 1428.334 | 9.073 | 3.91× |
| soak | 4 | playwright-shell | pass | yes | 324.944 | 634.253 | 11.277 | 3.02× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | puppeteer-shell | 10.667× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-chrome | 16.378× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-shell | 11.064× | 6 | 7 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 11.773× | 4 | 6 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `resident/c1`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.10.0\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | pass | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | pass | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 928 | 965 | 1.087 | 23.20× |
| cold | 1 | puppeteer-shell | pass | yes | 762 | 890 | 1.295 | 19.05× |
| cold | 1 | playwright-shell | pass | yes | 675 | 700 | 1.487 | 16.88× |
| cold | 1 | playwright-chrome | pass | yes | 859 | 1166 | 1.098 | 21.48× |
| cold | 1 | shotium | pass | yes | 40 | 44 | 24.648 | 1.00× |
| cold-settled | 1 | shotium | noisy | no | 9.687 | 10.093 | 0.669 | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 151.63 | 198.986 | 6.260 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | no | 137.428 | 146.179 | 7.228 | N/A |
| cold-settled | 1 | puppeteer-chrome | pass | no | 178.388 | 203.158 | 5.476 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 155.295 | 179.855 | 6.517 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 992.533 | 3757.866 | 0.858 | 14.48× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1338.121 | 4518.902 | 0.578 | 19.52× |
| lifecycle | 1 | playwright-chrome | fail | no | 6410.749 | 11160.402 | N/A | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2870.465 | 3895.836 | 0.338 | 41.88× |
| lifecycle | 1 | shotium | pass | yes | 68.536 | 126.198 | 13.360 | 1.00× |
| warm | 1 | playwright-chrome | noisy | no | 189.382 | 194.283 | 0.095 | N/A |
| warm | 1 | puppeteer-chrome | pass | yes | 260.074 | 349.386 | 3.876 | 27.47× |
| warm | 1 | playwright-shell | noisy | no | 178.151 | 198.491 | 0.997 | N/A |
| warm | 1 | puppeteer-shell | pass | yes | 162.014 | 191.233 | 6.029 | 17.11× |
| warm | 1 | shotium | pass | yes | 9.467 | 13.868 | 97.378 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 216.967 | 546.738 | 4.018 | 14.12× |
| batch | 1 | puppeteer-chrome | pass | yes | 264.236 | 500.365 | 3.524 | 17.20× |
| batch | 1 | shotium | pass | yes | 15.366 | 273.597 | 26.197 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 198.751 | 426.14 | 4.500 | 12.93× |
| batch | 1 | playwright-shell | pass | yes | 205.866 | 437.755 | 4.430 | 13.40× |
| parallel | 1 | shotium | pass | yes | 12.082 | 262.012 | 28.795 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 161.888 | 403.492 | 5.473 | 13.40× |
| parallel | 1 | puppeteer-shell | pass | yes | 157.15 | 404.965 | 5.525 | 13.01× |
| parallel | 1 | playwright-shell | pass | yes | 163.623 | 389.301 | 5.597 | 13.54× |
| parallel | 1 | puppeteer-chrome | pass | yes | 203.719 | 442.56 | 4.348 | 16.86× |
| parallel | 2 | playwright-chrome | pass | yes | 249.022 | 870.101 | 6.984 | 7.47× |
| parallel | 2 | puppeteer-shell | pass | yes | 212.64 | 467.939 | 8.138 | 6.38× |
| parallel | 2 | playwright-shell | pass | yes | 202.269 | 454.726 | 8.461 | 6.07× |
| parallel | 2 | puppeteer-chrome | pass | yes | 374.176 | 636.802 | 5.036 | 11.23× |
| parallel | 2 | shotium | pass | yes | 33.321 | 286.129 | 30.590 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 359.947 | 635.366 | 10.443 | 4.75× |
| parallel | 4 | playwright-shell | noisy | no | 10145.65 | 10145.65 | N/A | N/A |
| parallel | 4 | puppeteer-chrome | pass | yes | 628.036 | 996.681 | 6.154 | 8.29× |
| parallel | 4 | shotium | pass | yes | 75.789 | 356.539 | 30.249 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 422.692 | 981.466 | 8.595 | 5.58× |
| reuse-page | 1 | playwright-shell | pass | no | 100.619 | 118.37 | 9.823 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 116.32 | 138.454 | 8.515 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.273 | 121.059 | 9.606 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.725 | 193.279 | 8.565 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 719 | 1433 | 1.152 | 13.31× |
| resident | 1 | shotium | pass | yes | 54 | 571 | 5.521 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 639 | 903 | 1.496 | 11.83× |
| resident | 1 | playwright-shell | pass | yes | 794 | 1245 | 1.113 | 14.70× |
| resident | 1 | playwright-chrome | pass | yes | 849 | 1060 | 1.157 | 15.72× |
| faults | 1 | puppeteer-chrome | pass | no | 33397.345 | 33397.345 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 12195.111 | 12195.111 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 27829.981 | 27829.981 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 31212.215 | 31212.215 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10306.136 | 10306.136 | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | 12964.839 | 12964.839 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 100.108 | 605.641 | 26.060 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 885.293 | 2967.402 | 4.417 | 8.84× |
| soak | 4 | puppeteer-shell | pass | yes | 465.917 | 924.304 | 8.261 | 4.65× |
| soak | 4 | playwright-shell | pass | yes | 513.36 | 1087.961 | 7.523 | 5.13× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.10.0\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 49 | 55 | 19.830 | N/A |
| cold-settled | 1 | shotium | pass | no | 9.007 | 9.671 | 107.117 | N/A |
| lifecycle | 1 | shotium | pass | no | 77.417 | 396.175 | 10.799 | N/A |
| warm | 1 | shotium | pass | no | 6.224 | 9.132 | 137.472 | N/A |
| batch | 1 | shotium | pass | no | 11.189 | 269.605 | 29.766 | N/A |
| parallel | 1 | shotium | pass | no | 11.208 | 266.723 | 29.174 | N/A |
| parallel | 2 | shotium | noisy | no | 8257.533 | 8257.533 | N/A | N/A |
| parallel | 4 | shotium | pass | no | 74.712 | 345.026 | 30.275 | N/A |
| resident | 1 | shotium | pass | no | 583 | 1540 | 1.483 | N/A |
| faults | 1 | shotium | pass | no | 11269.836 | 11269.836 | N/A | N/A |
| soak | 4 | shotium | pass | no | 86.059 | 396.309 | 28.847 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | puppeteer-shell | 10.517× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-chrome | 16.805× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-shell | 10.250× | 6 | 8 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 27.875× | 6 | 8 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.10.0/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 2301 | 4493 | 0.387 | 33.84× |
| cold | 1 | puppeteer-shell | pass | yes | 1220 | 2111 | 0.749 | 17.94× |
| cold | 1 | playwright-shell | pass | yes | 1072 | 1333 | 0.908 | 15.76× |
| cold | 1 | playwright-chrome | pass | yes | 2764 | 3040 | 0.356 | 40.65× |
| cold | 1 | shotium | pass | yes | 68 | 113 | 13.060 | 1.00× |
| cold-settled | 1 | shotium | noisy | no | 9.353 | 11.454 | 100.102 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 698.407 | 900.238 | 1.329 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 352.112 | 447.03 | 2.801 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 822.936 | 1209.374 | 1.198 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 231.29 | 379.512 | 4.022 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 1411.238 | 2258.129 | 0.706 | 14.06× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1784.509 | 2371.839 | 0.566 | 17.78× |
| lifecycle | 1 | playwright-chrome | fail | no | 5221.513 | 6215.314 | N/A | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3080.32 | 3906.946 | 0.326 | 30.69× |
| lifecycle | 1 | shotium | pass | yes | 100.358 | 142.408 | 10.259 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 877.399 | 1020.741 | 1.112 | 59.86× |
| warm | 1 | puppeteer-chrome | pass | yes | 582.156 | 974.801 | 1.569 | 39.72× |
| warm | 1 | playwright-shell | pass | yes | 335.078 | 701.818 | 2.756 | 22.86× |
| warm | 1 | puppeteer-shell | pass | yes | 364.209 | 681.158 | 2.549 | 24.85× |
| warm | 1 | shotium | pass | yes | 14.657 | 49.239 | 53.536 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 965.828 | 1376.24 | 1.025 | 44.92× |
| batch | 1 | puppeteer-chrome | pass | yes | 599.921 | 927.867 | 1.611 | 27.90× |
| batch | 1 | shotium | pass | yes | 21.502 | 259.081 | 21.944 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 391.703 | 763.889 | 2.413 | 18.22× |
| batch | 1 | playwright-shell | pass | yes | 388.092 | 738.786 | 2.433 | 18.05× |
| parallel | 1 | shotium | pass | yes | 29.474 | 518.349 | 16.006 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 912.525 | 1670.42 | 1.065 | 30.96× |
| parallel | 1 | puppeteer-shell | pass | yes | 354.222 | 1046.936 | 2.565 | 12.02× |
| parallel | 1 | playwright-shell | pass | yes | 338.033 | 663.029 | 2.784 | 11.47× |
| parallel | 1 | puppeteer-chrome | pass | yes | 443.871 | 979.699 | 2.085 | 15.06× |
| parallel | 2 | playwright-chrome | pass | yes | 1475.498 | 3240.038 | 1.323 | 40.88× |
| parallel | 2 | puppeteer-shell | pass | yes | 400.892 | 735.297 | 4.689 | 11.11× |
| parallel | 2 | playwright-shell | pass | yes | 330.112 | 734.567 | 5.522 | 9.15× |
| parallel | 2 | puppeteer-chrome | pass | yes | 638.938 | 1215.172 | 2.973 | 17.70× |
| parallel | 2 | shotium | pass | yes | 36.093 | 274.31 | 28.682 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 596.528 | 998.176 | 6.531 | 5.94× |
| parallel | 4 | playwright-shell | pass | yes | 629.575 | 1125.218 | 6.215 | 6.27× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1140.385 | 3162.141 | 3.216 | 11.36× |
| parallel | 4 | shotium | pass | yes | 100.426 | 362.626 | 26.399 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 2821.394 | 4700.797 | 1.474 | 28.09× |
| reuse-page | 1 | playwright-shell | pass | no | 243.522 | 293.073 | 4.427 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 216.282 | 266.551 | 4.848 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 258.346 | 376.858 | 3.713 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 236.601 | 338.675 | 4.162 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 2095 | 2233 | 0.491 | 2.29× |
| resident | 1 | shotium | pass | yes | 913 | 1152 | 1.078 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 1452 | 2595 | 0.596 | 1.59× |
| resident | 1 | playwright-shell | pass | yes | 1850 | 2020 | 0.551 | 2.03× |
| resident | 1 | playwright-chrome | pass | yes | 2362 | 2733 | 0.428 | 2.59× |
| faults | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 38213.981 | 38213.981 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 25721.889 | 25721.889 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 25264.064 | 25264.064 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9045.209 | 9045.209 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 3119.517 | 7222.701 | 1.284 | 36.25× |
| soak | 4 | shotium | pass | yes | 86.05 | 401.803 | 29.148 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 1150.44 | 4121.67 | 3.311 | 13.37× |
| soak | 4 | puppeteer-shell | pass | yes | 743.564 | 1993.909 | 5.182 | 8.64× |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 8 / 8 | 8 |
| 2 | playwright-shell | 13.059× | 6 | 8 / 8 | 0 |
| 3 | puppeteer-shell | 17.996× | 6 | 8 / 8 | 0 |
| 4 | puppeteer-chrome | 32.135× | 6 | 8 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 20.349× | 5 | 7 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.10.0/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 2711 | 5852 | 0.370 | 41.71× |
| cold | 1 | puppeteer-shell | pass | yes | 997 | 1272 | 0.987 | 15.34× |
| cold | 1 | playwright-shell | pass | yes | 795 | 1082 | 1.241 | 12.23× |
| cold | 1 | playwright-chrome | pass | yes | 2186 | 3770 | 0.431 | 33.63× |
| cold | 1 | shotium | pass | yes | 65 | 121 | 14.583 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 12.177 | 14.254 | 84.322 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 302.932 | 356.768 | 3.324 | 24.88× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 293.509 | 347.497 | 3.464 | 24.10× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 471.311 | 549.96 | 2.292 | 38.71× |
| cold-settled | 1 | playwright-shell | pass | yes | 225.245 | 252.013 | 4.753 | 18.50× |
| lifecycle | 1 | playwright-shell | pass | yes | 562.393 | 992.148 | 1.646 | 12.33× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 731.405 | 1122.689 | 1.285 | 16.03× |
| lifecycle | 1 | playwright-chrome | fail | no | 2905.498 | 3787.196 | N/A | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1644.851 | 2761.195 | 0.579 | 36.06× |
| lifecycle | 1 | shotium | pass | yes | 45.619 | 66.895 | 21.684 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 226.758 | 375.283 | 4.334 | 27.19× |
| warm | 1 | puppeteer-chrome | pass | yes | 398.914 | 589.736 | 2.480 | 47.83× |
| warm | 1 | playwright-shell | pass | yes | 194.06 | 290.659 | 5.237 | 23.27× |
| warm | 1 | puppeteer-shell | pass | yes | 245.232 | 385.395 | 4.030 | 29.40× |
| warm | 1 | shotium | pass | yes | 8.341 | 16.752 | 103.339 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 268.968 | 1698.124 | 2.893 | 19.29× |
| batch | 1 | puppeteer-chrome | pass | yes | 468.599 | 1781.625 | 1.917 | 33.60× |
| batch | 1 | shotium | pass | yes | 13.947 | 267.584 | 28.245 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 316.314 | 636.993 | 2.984 | 22.68× |
| batch | 1 | playwright-shell | pass | yes | 200.099 | 476.706 | 4.537 | 14.35× |
| parallel | 1 | shotium | pass | yes | 13.125 | 352.089 | 26.610 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 259.38 | 1811.52 | 2.819 | 19.76× |
| parallel | 1 | puppeteer-shell | pass | yes | 318.905 | 589.86 | 3.127 | 24.30× |
| parallel | 1 | playwright-shell | pass | yes | 192.841 | 425.379 | 4.634 | 14.69× |
| parallel | 1 | puppeteer-chrome | pass | yes | 463.654 | 2070.978 | 1.872 | 35.33× |
| parallel | 2 | playwright-chrome | pass | yes | 407.862 | 1648.899 | 3.673 | 14.58× |
| parallel | 2 | puppeteer-shell | pass | yes | 396.207 | 574.843 | 5.168 | 14.17× |
| parallel | 2 | playwright-shell | pass | yes | 244.301 | 537.797 | 7.128 | 8.74× |
| parallel | 2 | puppeteer-chrome | pass | yes | 683.636 | 1713.123 | 2.767 | 24.44× |
| parallel | 2 | shotium | pass | yes | 27.967 | 306.621 | 31.455 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 535.049 | 925.618 | 7.380 | 8.08× |
| parallel | 4 | playwright-shell | pass | yes | 468.377 | 808.503 | 8.215 | 7.08× |
| parallel | 4 | puppeteer-chrome | pass | yes | 931.827 | 1770.267 | 4.019 | 14.08× |
| parallel | 4 | shotium | pass | yes | 66.182 | 330.375 | 31.798 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 756.447 | 2045.72 | 4.892 | 11.43× |
| reuse-page | 1 | playwright-shell | pass | no | 100.702 | 127.385 | 9.761 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 122.571 | 229.036 | 7.502 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 164.892 | 228.897 | 6.322 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 174.703 | 273.59 | 5.447 | N/A |
| resident | 1 | puppeteer-chrome | pass | no | 1828 | 2132 | 0.610 | N/A |
| resident | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 34995.086 | 34995.086 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 31744.851 | 31744.851 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16796.246 | 16796.246 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 13394.125 | 13394.125 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5495.829 | 5495.829 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | no | 694.098 | 9988.989 | 4.079 | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | 11255.349 | 11255.349 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |

