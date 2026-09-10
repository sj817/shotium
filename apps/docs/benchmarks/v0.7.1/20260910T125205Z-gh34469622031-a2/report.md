# Shotium 0.7.1 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `0f28166c4485fd2e7c80f40b0fcd8daddcabac06`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 4 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 8 |
| darwin-arm64 | noisy | shotium | 5 | 7 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 6.286× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 6.496× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 7.881× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 9.951× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.7.1/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 756 | 811 | 1.310 | 12.60× |
| cold | 1 | shotium | pass | yes | 60 | 64 | 16.787 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 621 | 635 | 1.609 | 10.35× |
| cold | 1 | playwright-chrome | pass | yes | 944 | 992 | 1.054 | 15.73× |
| cold | 1 | puppeteer-chrome | pass | yes | 862 | 886 | 1.159 | 14.37× |
| cold-settled | 1 | shotium | pass | yes | 14.138 | 15.448 | 71.287 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 125.248 | 131.282 | 8.025 | 8.86× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 128.819 | 137.799 | 7.716 | 9.11× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 149.85 | 176.562 | 6.369 | 10.60× |
| cold-settled | 1 | playwright-chrome | pass | yes | 159.784 | 177.735 | 6.229 | 11.30× |
| lifecycle | 1 | playwright-shell | pass | yes | 646.555 | 718.436 | 1.514 | 11.22× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 635.968 | 698.269 | 1.558 | 11.04× |
| lifecycle | 1 | playwright-chrome | pass | yes | 852.433 | 908.633 | 1.174 | 14.80× |
| lifecycle | 1 | shotium | pass | yes | 57.604 | 102.822 | 14.933 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 906.325 | 970.688 | 1.103 | 15.73× |
| warm | 1 | playwright-shell | pass | yes | 131.401 | 155.046 | 7.586 | 9.51× |
| warm | 1 | puppeteer-shell | pass | yes | 133.2 | 135.372 | 7.555 | 9.64× |
| warm | 1 | shotium | pass | yes | 13.811 | 20.599 | 66.083 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 166.974 | 179.896 | 5.995 | 12.09× |
| warm | 1 | playwright-chrome | pass | yes | 154.024 | 178.821 | 6.334 | 11.15× |
| batch | 1 | playwright-chrome | pass | yes | 182.225 | 387.339 | 4.979 | 9.50× |
| batch | 1 | puppeteer-shell | pass | yes | 151.622 | 373.146 | 5.585 | 7.91× |
| batch | 1 | puppeteer-chrome | pass | yes | 191.9 | 420.396 | 4.529 | 10.01× |
| batch | 1 | playwright-shell | pass | yes | 141.719 | 397.327 | 5.820 | 7.39× |
| batch | 1 | shotium | pass | yes | 19.172 | 266.644 | 24.128 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 124.816 | 371.675 | 6.534 | 8.34× |
| parallel | 1 | shotium | pass | yes | 14.963 | 259.93 | 28.150 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 143.119 | 374.409 | 6.163 | 9.56× |
| parallel | 1 | puppeteer-chrome | pass | yes | 151.776 | 391.348 | 5.633 | 10.14× |
| parallel | 1 | playwright-shell | pass | yes | 116.048 | 342.668 | 7.078 | 7.76× |
| parallel | 2 | shotium | pass | yes | 38.536 | 278.099 | 28.316 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 197.634 | 445.509 | 8.779 | 5.13× |
| parallel | 2 | puppeteer-chrome | fail | no | 213.32 | 794.622 | 4.114 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 153.632 | 407.211 | 10.873 | 3.99× |
| parallel | 2 | puppeteer-shell | pass | yes | 189.34 | 415.153 | 9.316 | 4.91× |
| parallel | 4 | playwright-chrome | pass | yes | 379.469 | 642.209 | 9.985 | 4.21× |
| parallel | 4 | puppeteer-chrome | fail | no | 233.585 | 14201.887 | 6.900 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 290.86 | 547.855 | 12.278 | 3.23× |
| parallel | 4 | puppeteer-shell | pass | yes | 327.27 | 607.627 | 11.243 | 3.63× |
| parallel | 4 | shotium | pass | yes | 90.055 | 323.516 | 28.336 | 1.00× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.092 | 116.604 | 9.971 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 83.233 | 97.915 | 12.078 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 83.704 | 100.913 | 11.203 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.332 | 87.536 | 11.957 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 697 | 799 | 1.504 | 3.27× |
| resident | 1 | shotium | pass | yes | 213 | 342 | 5.295 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 700 | 728 | 1.497 | 3.29× |
| resident | 1 | playwright-chrome | pass | yes | 844 | 881 | 1.321 | 3.96× |
| resident | 1 | playwright-shell | pass | yes | 766 | 832 | 1.361 | 3.60× |
| faults | 1 | puppeteer-shell | pass | no | 9347.793 | 9347.793 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15020.62 | 15020.62 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 11757.157 | 11757.157 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5190.284 | 5190.284 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16145.47 | 16145.47 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 268.723 | 28856.46 | 4.210 | N/A |
| soak | 4 | playwright-shell | pass | yes | 360.597 | 867.771 | 9.939 | 3.05× |
| soak | 4 | puppeteer-shell | pass | yes | 409.173 | 833.59 | 9.227 | 3.46× |
| soak | 4 | shotium | pass | yes | 118.417 | 372.13 | 24.565 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 478.906 | 882.838 | 7.908 | 4.04× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.843× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.751× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-arm64@0.7.1/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 612 | 680 | 1.620 | 10.20× |
| cold | 1 | shotium | pass | yes | 60 | 65 | 16.548 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 785 | 825 | 1.258 | 13.08× |
| cold-settled | 1 | playwright-chrome | pass | yes | 141.723 | 161.836 | 6.887 | 4.17× |
| cold-settled | 1 | playwright-shell | pass | yes | 126.255 | 140.209 | 7.885 | 3.72× |
| cold-settled | 1 | shotium | pass | yes | 33.974 | 39.353 | 28.837 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 552.678 | 592.759 | 1.797 | 7.62× |
| lifecycle | 1 | playwright-chrome | pass | yes | 719.373 | 780.683 | 1.384 | 9.92× |
| lifecycle | 1 | shotium | pass | yes | 72.524 | 84.787 | 13.835 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 116.321 | 131.726 | 8.489 | 4.02× |
| warm | 1 | shotium | pass | yes | 28.907 | 32.86 | 33.715 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 145.779 | 166.404 | 7.152 | 5.04× |
| batch | 1 | shotium | pass | yes | 34.68 | 261.965 | 18.872 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 155.839 | 373.68 | 5.526 | 4.49× |
| batch | 1 | playwright-shell | pass | yes | 128.274 | 357.587 | 6.475 | 3.70× |
| parallel | 1 | playwright-shell | pass | yes | 135.222 | 358.174 | 6.267 | 3.76× |
| parallel | 1 | shotium | pass | yes | 35.916 | 263.209 | 18.426 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 175.024 | 408.197 | 5.153 | 4.87× |
| parallel | 2 | shotium | pass | yes | 75.452 | 294.697 | 18.597 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 237.82 | 513.002 | 7.644 | 3.15× |
| parallel | 2 | playwright-shell | pass | yes | 187.699 | 447.52 | 9.393 | 2.49× |
| parallel | 4 | playwright-chrome | pass | yes | 393.853 | 645.873 | 9.178 | 2.42× |
| parallel | 4 | playwright-shell | pass | yes | 309.386 | 560.818 | 11.486 | 1.90× |
| parallel | 4 | shotium | pass | yes | 163.051 | 403.318 | 18.568 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.252 | 83.889 | 12.003 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.165 | 85.314 | 12.222 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 914 | 934 | 1.178 | 5.25× |
| resident | 1 | shotium | pass | yes | 174 | 213 | 6.610 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 839 | 892 | 1.187 | 4.82× |
| faults | 1 | playwright-shell | pass | no | 15081.421 | 15081.421 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16181.704 | 16181.704 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5021.984 | 5021.984 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 316.136 | 600.593 | 11.637 | 1.91× |
| soak | 4 | shotium | pass | yes | 165.596 | 447.586 | 18.425 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 405.996 | 723.539 | 9.443 | 2.45× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 6.534× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 7.251× | 7 | 9 / 9 | 0 |
| 4 | playwright-chrome | 8.090× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 10.469× | 5 | 5 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.7.1\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 845 | 1224 | 1.113 | 11.58× |
| cold | 1 | shotium | pass | yes | 73 | 74 | 13.780 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 930 | 966 | 1.071 | 12.74× |
| cold | 1 | playwright-chrome | pass | yes | 1086 | 1195 | 0.916 | 14.88× |
| cold | 1 | puppeteer-chrome | pass | yes | 1118 | 1288 | 0.878 | 15.32× |
| cold-settled | 1 | shotium | pass | yes | 14.246 | 17.259 | 65.915 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 166.785 | 183.097 | 6.270 | 11.71× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 155.961 | 173.375 | 6.303 | 10.95× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 181.225 | 238.571 | 0.380 | N/A |
| cold-settled | 1 | playwright-chrome | pass | yes | 152.46 | 158.827 | 6.562 | 10.70× |
| lifecycle | 1 | playwright-shell | pass | yes | 819.518 | 1033.353 | 1.166 | 7.68× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1087.883 | 1383.737 | 0.894 | 10.19× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1654.144 | 1800.829 | 0.602 | 15.49× |
| lifecycle | 1 | shotium | pass | yes | 106.77 | 186.284 | 8.914 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2021.362 | 2231.026 | 0.492 | 18.93× |
| warm | 1 | playwright-shell | pass | yes | 155.306 | 184.89 | 6.252 | 11.43× |
| warm | 1 | puppeteer-shell | pass | yes | 164.803 | 218.963 | 5.948 | 12.13× |
| warm | 1 | shotium | pass | yes | 13.584 | 16.085 | 69.099 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 204.202 | 232.635 | 4.934 | 15.03× |
| warm | 1 | playwright-chrome | pass | yes | 179.726 | 205.917 | 5.560 | 13.23× |
| batch | 1 | playwright-chrome | pass | yes | 205.31 | 409.394 | 4.444 | 7.81× |
| batch | 1 | puppeteer-shell | pass | yes | 197.565 | 421.668 | 4.573 | 7.52× |
| batch | 1 | puppeteer-chrome | pass | yes | 213.374 | 460.264 | 4.206 | 8.12× |
| batch | 1 | playwright-shell | pass | yes | 193.652 | 446.325 | 4.648 | 7.37× |
| batch | 1 | shotium | pass | yes | 26.281 | 275.291 | 21.439 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 196.172 | 472.042 | 4.528 | 7.60× |
| parallel | 1 | shotium | pass | yes | 25.821 | 276.302 | 21.322 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 193.224 | 417.535 | 4.523 | 7.48× |
| parallel | 1 | puppeteer-chrome | noisy | no | 9497.424 | 9497.424 | N/A | N/A |
| parallel | 1 | playwright-shell | pass | yes | 183.761 | 408.843 | 4.755 | 7.12× |
| parallel | 2 | shotium | pass | yes | 61.195 | 314.306 | 21.903 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 312.95 | 607.496 | 5.847 | 5.11× |
| parallel | 2 | puppeteer-chrome | fail | no | 318.332 | 854.136 | 5.205 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 240.969 | 506.526 | 7.288 | 3.94× |
| parallel | 2 | puppeteer-shell | pass | yes | 260.575 | 496.745 | 6.796 | 4.26× |
| parallel | 4 | playwright-chrome | pass | yes | 574.456 | 1158.523 | 6.701 | 4.32× |
| parallel | 4 | puppeteer-chrome | fail | no | 390.825 | 2526.822 | 0.524 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 404.87 | 691.777 | 8.926 | 3.04× |
| parallel | 4 | puppeteer-shell | pass | yes | 435.985 | 727.522 | 8.423 | 3.28× |
| parallel | 4 | shotium | pass | yes | 133.092 | 389.072 | 22.250 | 1.00× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.151 | 118.067 | 9.656 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 84.809 | 109.02 | 11.095 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 85.967 | 119.817 | 10.773 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.314 | 98.48 | 11.783 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 714 | 1113 | 1.383 | 4.03× |
| resident | 1 | shotium | pass | yes | 177 | 514 | 5.319 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 629 | 896 | 1.447 | 3.55× |
| resident | 1 | playwright-chrome | pass | yes | 624 | 1073 | 1.393 | 3.53× |
| resident | 1 | playwright-shell | pass | yes | 514 | 827 | 1.730 | 2.90× |
| faults | 1 | puppeteer-shell | pass | no | 27509.4 | 27509.4 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 24504.366 | 24504.366 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 31771.851 | 31771.851 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9417.718 | 9417.718 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 28132.139 | 28132.139 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 362.131 | 31347.085 | 1.120 | N/A |
| soak | 4 | playwright-shell | pass | no | 407.35 | 976.897 | 9.309 | N/A |
| soak | 4 | puppeteer-shell | noisy | no | 11240.961 | 11240.961 | N/A | N/A |
| soak | 4 | shotium | noisy | no | 10144.37 | 10144.37 | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 555.02 | 1878.899 | 6.615 | N/A |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-arm64@0.7.1\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 66 | 76 | 14.737 | N/A |
| cold-settled | 1 | shotium | pass | no | 12.562 | 15.157 | 76.397 | N/A |
| lifecycle | 1 | shotium | pass | no | 96.316 | 123.655 | 10.090 | N/A |
| warm | 1 | shotium | noisy | no | 12.06 | 12.57 | 1.795 | N/A |
| batch | 1 | shotium | pass | no | 23.535 | 274.966 | 22.471 | N/A |
| parallel | 1 | shotium | pass | no | 22.829 | 271.383 | 22.739 | N/A |
| parallel | 2 | shotium | pass | no | 51.506 | 290.615 | 24.221 | N/A |
| parallel | 4 | shotium | pass | no | 114.958 | 373.382 | 23.939 | N/A |
| resident | 1 | shotium | pass | no | 600 | 2016 | 1.242 | N/A |
| faults | 1 | shotium | pass | no | 10401.93 | 10401.93 | N/A | N/A |
| soak | 4 | shotium | pass | no | 112.072 | 586.131 | 23.822 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 8 / 8 | 8 |
| 2 | playwright-shell | 8.749× | 7 | 8 / 8 | 0 |
| 3 | puppeteer-shell | 9.764× | 7 | 8 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 34.910× | 6 | 6 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 21.617× | 5 | 5 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.7.1/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 1279 | 1847 | 0.721 | 13.75× |
| cold | 1 | shotium | pass | yes | 93 | 138 | 9.550 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 1201 | 1659 | 0.778 | 12.91× |
| cold | 1 | playwright-chrome | pass | yes | 3656 | 5414 | 0.255 | 39.31× |
| cold | 1 | puppeteer-chrome | pass | yes | 2450 | 2884 | 0.410 | 26.34× |
| cold-settled | 1 | shotium | pass | yes | 13.897 | 19.448 | 67.538 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 260.319 | 347.847 | 3.707 | 18.73× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 278.455 | 1468.952 | 2.180 | 20.04× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 358.403 | 391.685 | 3.035 | 25.79× |
| cold-settled | 1 | playwright-chrome | pass | yes | 908.798 | 929.252 | 1.200 | 65.40× |
| lifecycle | 1 | playwright-shell | pass | yes | 1097.563 | 1349.417 | 0.896 | 10.87× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1353.543 | 1660.703 | 0.728 | 13.41× |
| lifecycle | 1 | playwright-chrome | pass | yes | 2859.099 | 3298.201 | 0.349 | 28.33× |
| lifecycle | 1 | shotium | pass | yes | 100.939 | 159.964 | 9.130 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2390.953 | 2966.79 | 0.403 | 23.69× |
| warm | 1 | playwright-shell | pass | yes | 269.145 | 567.915 | 3.385 | 16.80× |
| warm | 1 | puppeteer-shell | pass | yes | 328.763 | 456.464 | 3.119 | 20.52× |
| warm | 1 | shotium | pass | yes | 16.021 | 26.569 | 54.108 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 416.713 | 1544.65 | 1.824 | 26.01× |
| warm | 1 | playwright-chrome | pass | yes | 871.661 | 1154.584 | 1.120 | 54.41× |
| batch | 1 | playwright-chrome | pass | yes | 906.445 | 1344.236 | 1.091 | 24.84× |
| batch | 1 | puppeteer-shell | pass | yes | 352.156 | 740.087 | 2.675 | 9.65× |
| batch | 1 | puppeteer-chrome | pass | yes | 411.584 | 2486.324 | 1.988 | 11.28× |
| batch | 1 | playwright-shell | pass | yes | 330.391 | 713.919 | 2.790 | 9.05× |
| batch | 1 | shotium | pass | yes | 36.496 | 265.91 | 16.992 | 1.00× |
| parallel | 1 | puppeteer-shell | noisy | no | 10035.102 | 10035.102 | N/A | N/A |
| parallel | 1 | shotium | noisy | no | 11239.373 | 11239.373 | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 16628.66 | 16628.66 | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | no | 12208.258 | 12208.258 | N/A | N/A |
| parallel | 2 | shotium | pass | yes | 99.323 | 401.826 | 15.092 | 1.00× |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | fail | no | 585.719 | 1852.422 | 2.886 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 368.441 | 715.874 | 4.994 | 3.71× |
| parallel | 2 | puppeteer-shell | pass | yes | 453.208 | 796.86 | 4.244 | 4.56× |
| parallel | 4 | playwright-chrome | pass | yes | 2897.704 | 5172.321 | 1.385 | 18.39× |
| parallel | 4 | puppeteer-chrome | fail | no | 653.421 | 7228.363 | 1.790 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 697.039 | 1131.768 | 5.565 | 4.42× |
| parallel | 4 | puppeteer-shell | pass | yes | 788.649 | 3100.64 | 3.854 | 5.01× |
| parallel | 4 | shotium | pass | yes | 157.538 | 425.029 | 19.338 | 1.00× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 151.436 | 239.026 | 6.118 | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 21005.956 | 21005.956 | N/A | N/A |
| resident | 1 | shotium | noisy | no | 12529.998 | 12529.998 | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-shell | pass | no | 2719 | 3931 | 0.343 | N/A |
| faults | 1 | puppeteer-shell | pass | no | 17037.531 | 17037.531 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 24645.899 | 24645.899 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 35028.323 | 35028.323 | N/A | N/A |
| faults | 1 | shotium | pass | no | 7800.115 | 7800.115 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 37665.628 | 37665.628 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 616.176 | 26996.119 | 2.191 | N/A |
| soak | 4 | playwright-shell | pass | yes | 549.087 | 1093.46 | 7.171 | 4.91× |
| soak | 4 | puppeteer-shell | pass | yes | 588.674 | 1265.984 | 6.559 | 5.26× |
| soak | 4 | shotium | pass | yes | 111.822 | 393.667 | 24.495 | 1.00× |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 7 eligible cell(s), with 7 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 5 | 7 / 7 | 7 |
| 2 | playwright-shell | 7.007× | 5 | 7 / 7 | 0 |
| 3 | playwright-chrome | 11.206× | 5 | 7 / 7 | 0 |
| 4 | puppeteer-shell | 11.494× | 5 | 7 / 7 | 0 |
| 5 | puppeteer-chrome | 14.712× | 5 | 7 / 7 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.7.1/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | fail | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | noisy | no | 659 | 1368 | 1.282 | N/A |
| cold | 1 | shotium | noisy | no | 64.5 | 127 | 13.393 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 792.5 | 956 | 1.221 | N/A |
| cold | 1 | playwright-chrome | noisy | no | 2318.5 | 6358 | 0.352 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 1930 | 4732 | 0.417 | N/A |
| cold-settled | 1 | shotium | noisy | no | 21.138 | 30.887 | 46.651 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 215.786 | 251.707 | 4.536 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 349.861 | 376.004 | 3.198 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 381.249 | 413.668 | 2.606 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 256.734 | 289.176 | 3.869 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 693.213 | 1514.533 | 1.192 | 8.92× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 969.27 | 1446.442 | 0.991 | 12.48× |
| lifecycle | 1 | playwright-chrome | pass | yes | 2189.245 | 3361.738 | 0.436 | 28.18× |
| lifecycle | 1 | shotium | pass | yes | 77.69 | 131.254 | 11.978 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2082.106 | 3455.286 | 0.446 | 26.80× |
| warm | 1 | playwright-shell | pass | yes | 179.62 | 457.372 | 4.990 | 17.86× |
| warm | 1 | puppeteer-shell | pass | yes | 230.354 | 351.718 | 4.231 | 22.90× |
| warm | 1 | shotium | pass | yes | 10.058 | 15.132 | 91.459 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 295.933 | 384.311 | 3.356 | 29.42× |
| warm | 1 | playwright-chrome | pass | yes | 204.333 | 268.338 | 4.785 | 20.32× |
| batch | 1 | playwright-chrome | pass | yes | 217.291 | 1387.243 | 3.622 | 13.37× |
| batch | 1 | puppeteer-shell | pass | yes | 310.37 | 733.44 | 3.301 | 19.10× |
| batch | 1 | puppeteer-chrome | pass | yes | 411.145 | 1653.952 | 2.191 | 25.30× |
| batch | 1 | playwright-shell | pass | yes | 152.97 | 422.669 | 5.713 | 9.41× |
| batch | 1 | shotium | pass | yes | 16.253 | 268.329 | 27.584 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 300.871 | 666.586 | 3.144 | 26.66× |
| parallel | 1 | shotium | pass | yes | 11.285 | 273.517 | 32.011 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 177.068 | 1381.325 | 4.320 | 15.69× |
| parallel | 1 | puppeteer-chrome | pass | yes | 319.246 | 1212.056 | 2.778 | 28.29× |
| parallel | 1 | playwright-shell | pass | yes | 139.461 | 405.35 | 6.241 | 12.36× |
| parallel | 2 | shotium | pass | yes | 28.784 | 296.666 | 31.805 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 250.031 | 745.723 | 6.870 | 8.69× |
| parallel | 2 | puppeteer-chrome | pass | yes | 398.857 | 1442.529 | 3.898 | 13.86× |
| parallel | 2 | playwright-shell | pass | yes | 159.341 | 409.438 | 10.389 | 5.54× |
| parallel | 2 | puppeteer-shell | pass | yes | 337.192 | 617.374 | 5.686 | 11.71× |
| parallel | 4 | playwright-chrome | pass | yes | 444.898 | 1320.343 | 8.000 | 6.98× |
| parallel | 4 | puppeteer-chrome | pass | yes | 420.884 | 24343.332 | 4.026 | 6.60× |
| parallel | 4 | playwright-shell | pass | yes | 250.337 | 534.65 | 13.902 | 3.93× |
| parallel | 4 | puppeteer-shell | pass | yes | 371.794 | 797.251 | 9.502 | 5.83× |
| parallel | 4 | shotium | pass | yes | 63.738 | 359.293 | 30.391 | 1.00× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 265.883 | 396.661 | 3.503 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 123.96 | 231.228 | 7.731 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 278.362 | 454.256 | 3.353 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 103.862 | 151.681 | 9.388 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 1136 | 1346 | 0.924 | 2.67× |
| resident | 1 | shotium | pass | yes | 426 | 558 | 2.326 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 1231 | 1837 | 0.764 | 2.89× |
| resident | 1 | playwright-chrome | pass | yes | 1298 | 1914 | 0.721 | 3.05× |
| resident | 1 | playwright-shell | pass | yes | 877 | 1146 | 1.064 | 2.06× |
| faults | 1 | puppeteer-shell | pass | no | 10903.432 | 10903.432 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16851.815 | 16851.815 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 24892.034 | 24892.034 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6850.647 | 6850.647 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 28619.576 | 28619.576 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 647.94 | 29833.986 | 2.691 | N/A |
| soak | 4 | playwright-shell | pass | no | 404.954 | 867.367 | 9.518 | N/A |
| soak | 4 | puppeteer-shell | pass | no | 449.869 | 961.22 | 8.709 | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | 12555.622 | 12555.622 | N/A | N/A |

