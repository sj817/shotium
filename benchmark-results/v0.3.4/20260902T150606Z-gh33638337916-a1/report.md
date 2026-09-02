# Shotium 0.3.4 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **fail**; evidence: **complete**. Profile **full**, seed `6bf4cb2fbc8e69e59371281916ce6bf49a73ef24`.

Conclusion: all platform outputs exist, but quality is fail. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | fail | shotium | 2 | 10 |
| linux-arm64 | noisy | shotium | 2 | 10 |
| win32-x64 | fail | shotium | 2 | 5 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | fail | shotium | 2 | 8 |
| darwin-arm64 | fail | shotium | 2 | 3 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 4.888× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 5.435× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 8.417× | 6 | 7 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 7.947× | 4 | 4 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `warm/c1`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`

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
| cold | 1 | playwright-chrome | pass | yes | 513 | 532 | 1.939 | 8.41× |
| cold | 1 | playwright-shell | pass | yes | 337 | 360 | 2.979 | 5.52× |
| cold | 1 | puppeteer-chrome | pass | yes | 597 | 665 | 1.660 | 9.79× |
| cold | 1 | puppeteer-shell | pass | yes | 350 | 370 | 2.834 | 5.74× |
| cold | 1 | shotium | pass | yes | 61 | 65 | 16.355 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 158.335 | 167.384 | 6.552 | 10.75× |
| cold-settled | 1 | playwright-chrome | pass | yes | 138.669 | 162.356 | 7.099 | 9.42× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 129.234 | 142.627 | 7.614 | 8.78× |
| cold-settled | 1 | shotium | pass | yes | 14.723 | 14.985 | 67.228 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 107.712 | 136.985 | 8.829 | 7.32× |
| lifecycle | 1 | playwright-shell | pass | yes | 272.291 | 317.966 | 3.609 | 4.81× |
| lifecycle | 1 | playwright-chrome | pass | yes | 469.79 | 518.274 | 2.130 | 8.29× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 326.528 | 387.251 | 2.982 | 5.76× |
| lifecycle | 1 | shotium | pass | yes | 56.645 | 114.154 | 14.625 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 603.646 | 676.787 | 1.646 | 10.66× |
| warm | 1 | playwright-chrome | noisy | no | 142.208 | 157.012 | 1.149 | N/A |
| warm | 1 | shotium | pass | yes | 14.121 | 17.889 | 65.854 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 117.338 | 161.968 | 8.158 | 8.31× |
| warm | 1 | puppeteer-shell | pass | yes | 131.817 | 147.548 | 7.584 | 9.33× |
| warm | 1 | puppeteer-chrome | pass | yes | 167.539 | 202.927 | 5.747 | 11.86× |
| batch | 1 | shotium | pass | yes | 28.237 | 261.698 | 20.533 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 146.489 | 356.743 | 5.688 | 5.19× |
| batch | 1 | playwright-shell | pass | yes | 135.288 | 353.33 | 6.176 | 4.79× |
| batch | 1 | puppeteer-chrome | pass | yes | 185.165 | 388.57 | 4.767 | 6.56× |
| batch | 1 | playwright-chrome | noisy | no | 162.474 | 373.272 | 1.706 | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 162.514 | 371.5 | 2.364 | N/A |
| parallel | 1 | puppeteer-shell | pass | yes | 148.671 | 355.842 | 5.758 | 5.31× |
| parallel | 1 | puppeteer-chrome | pass | yes | 189.385 | 396.283 | 4.813 | 6.77× |
| parallel | 1 | shotium | pass | yes | 27.977 | 270.428 | 20.819 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 136.857 | 353.71 | 6.207 | 4.89× |
| parallel | 2 | puppeteer-chrome | pass | yes | 303.856 | 473.541 | 5.839 | 5.07× |
| parallel | 2 | shotium | pass | yes | 59.991 | 288.205 | 21.012 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 228.385 | 390.346 | 7.452 | 3.81× |
| parallel | 2 | playwright-chrome | noisy | no | 279.773 | 409.224 | 1.113 | N/A |
| parallel | 2 | puppeteer-shell | pass | yes | 257.893 | 386.186 | 6.821 | 4.30× |
| parallel | 4 | playwright-shell | pass | yes | 409.463 | 655.562 | 8.079 | 3.33× |
| parallel | 4 | playwright-chrome | noisy | no | 568.026 | 662.647 | 1.816 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 471.181 | 751.626 | 7.303 | 3.83× |
| parallel | 4 | puppeteer-chrome | fail | no | 592.834 | 180737.721 | 0.468 | N/A |
| parallel | 4 | shotium | pass | yes | 122.911 | 359.719 | 20.932 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 65.558 | 88.674 | 15.704 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 67.843 | 85.252 | 13.743 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 66.741 | 83.266 | 14.644 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 83.211 | 120.89 | 11.493 | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 873.5 | 943 | 0.271 | N/A |
| resident | 1 | shotium | pass | yes | 171 | 466 | 4.189 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 966 | 989 | 1.046 | 5.65× |
| resident | 1 | playwright-chrome | pass | yes | 1038 | 1053 | 1.031 | 6.07× |
| resident | 1 | puppeteer-chrome | noisy | no | 19666.625 | 23424.157 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10795.426 | 10795.426 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 12240.601 | 12240.601 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10718.344 | 10718.344 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 17620.127 | 17620.127 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 4092.955 | 4092.955 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 392.826 | 857.654 | 9.358 | 2.87× |
| soak | 4 | puppeteer-chrome | fail | no | 568.652 | 1119.718 | 4.578 | N/A |
| soak | 4 | playwright-chrome | noisy | no | 4008.91 | 4008.91 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 459.613 | 997.019 | 8.543 | 3.36× |
| soak | 4 | shotium | pass | yes | 136.738 | 404.68 | 22.611 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 2.570× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 3.295× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | noisy | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 441 | 479 | 2.269 | 6.89× |
| cold | 1 | playwright-shell | pass | yes | 262 | 291 | 3.784 | 4.09× |
| cold | 1 | shotium | pass | yes | 64 | 68 | 15.317 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 103.065 | 117.022 | 9.732 | 2.97× |
| cold-settled | 1 | playwright-chrome | pass | yes | 125.576 | 140.428 | 8.029 | 3.62× |
| cold-settled | 1 | shotium | pass | yes | 34.715 | 34.874 | 28.690 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 241.83 | 285.198 | 4.138 | 3.27× |
| lifecycle | 1 | playwright-chrome | pass | yes | 395.975 | 458.064 | 2.479 | 5.35× |
| lifecycle | 1 | shotium | pass | yes | 74.035 | 99.936 | 12.831 | 1.00× |
| warm | 1 | playwright-chrome | noisy | no | 133.117 | 148.201 | 1.362 | N/A |
| warm | 1 | playwright-shell | pass | yes | 100.339 | 117.628 | 9.517 | 3.10× |
| warm | 1 | shotium | pass | yes | 32.399 | 37.117 | 29.754 | 1.00× |
| batch | 1 | shotium | pass | yes | 44.488 | 263.666 | 16.560 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 122.723 | 326.861 | 6.882 | 2.76× |
| batch | 1 | playwright-chrome | pass | yes | 148.88 | 372.275 | 5.822 | 3.35× |
| parallel | 1 | playwright-chrome | pass | yes | 139.729 | 356.619 | 6.177 | 3.27× |
| parallel | 1 | shotium | pass | yes | 42.751 | 261.275 | 16.958 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 119.308 | 333.297 | 7.116 | 2.79× |
| parallel | 2 | shotium | pass | yes | 86.139 | 299.085 | 17.077 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 176.131 | 351.005 | 9.287 | 2.04× |
| parallel | 2 | playwright-chrome | pass | yes | 224.707 | 387.702 | 7.650 | 2.61× |
| parallel | 4 | playwright-shell | pass | yes | 325.587 | 502.321 | 10.471 | 1.92× |
| parallel | 4 | playwright-chrome | pass | yes | 417.041 | 619.276 | 8.594 | 2.45× |
| parallel | 4 | shotium | pass | yes | 169.947 | 370.399 | 17.136 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 49.99 | 65.321 | 19.676 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 63.618 | 323.2 | 13.318 | N/A |
| resident | 1 | playwright-shell | pass | yes | 867 | 892 | 1.182 | 2.15× |
| resident | 1 | playwright-chrome | pass | yes | 893 | 916 | 1.178 | 2.22× |
| resident | 1 | shotium | pass | yes | 403 | 431 | 3.024 | 1.00× |
| faults | 1 | shotium | pass | no | 9726.344 | 9726.344 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 15862.235 | 15862.235 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14658.03 | 14658.03 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 422.069 | 716.005 | 9.166 | 2.21× |
| soak | 4 | playwright-shell | pass | yes | 300.886 | 598.958 | 12.220 | 1.58× |
| soak | 4 | shotium | pass | yes | 190.807 | 458.751 | 16.903 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 5 eligible cell(s), with 5 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 5 | 5 / 5 | 5 |
| 2 | playwright-shell | 5.073× | 5 | 5 / 5 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 5.648× | 4 | 4 / 5 | 0 |
| not ranked (partial coverage) | playwright-chrome | 10.900× | 3 | 3 / 5 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 12.759× | 3 | 3 / 5 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `soak/c4`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `soak/c4`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 608 | 683 | 1.625 | 8.81× |
| cold | 1 | playwright-shell | pass | yes | 353 | 366 | 2.835 | 5.12× |
| cold | 1 | puppeteer-chrome | pass | yes | 739 | 828 | 1.419 | 10.71× |
| cold | 1 | puppeteer-shell | pass | yes | 502 | 548 | 1.963 | 7.28× |
| cold | 1 | shotium | pass | yes | 69 | 71 | 14.493 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 170.425 | 241.214 | 5.538 | 11.58× |
| cold-settled | 1 | playwright-chrome | pass | yes | 153.355 | 164.862 | 6.479 | 10.42× |
| cold-settled | 1 | puppeteer-shell | noisy | no | 167.008 | 242.429 | 0.370 | N/A |
| cold-settled | 1 | shotium | pass | yes | 14.72 | 16.207 | 66.071 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 147.668 | 192.393 | 6.475 | 10.03× |
| lifecycle | 1 | playwright-shell | pass | yes | 515.957 | 735.511 | 1.966 | 4.86× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1498.762 | 12126.791 | 0.419 | 14.11× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 863.072 | 2239.865 | 1.068 | 8.12× |
| lifecycle | 1 | shotium | pass | yes | 106.245 | 245.086 | 8.168 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1779.764 | 2245.271 | 0.549 | 16.75× |
| warm | 1 | playwright-chrome | noisy | no | 179.092 | 246.476 | 0.854 | N/A |
| warm | 1 | shotium | noisy | no | 15.265 | 20.77 | 4.048 | N/A |
| warm | 1 | playwright-shell | pass | no | 137.381 | 182.499 | 6.802 | N/A |
| warm | 1 | puppeteer-shell | pass | no | 151.586 | 183.38 | 6.344 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 167.51 | 213.588 | 1.449 | N/A |
| batch | 1 | shotium | noisy | no | 36.182 | 269.483 | 4.331 | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 186.604 | 393.111 | 1.751 | N/A |
| batch | 1 | playwright-shell | noisy | no | 175.665 | 389.126 | 2.141 | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 197.626 | 417.143 | 1.818 | N/A |
| batch | 1 | playwright-chrome | noisy | no | 187.711 | 399.798 | 2.128 | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 194.547 | 3298.889 | 3.037 | N/A |
| parallel | 1 | puppeteer-shell | pass | yes | 182.4 | 415.574 | 4.748 | 5.05× |
| parallel | 1 | puppeteer-chrome | noisy | no | 199.756 | 465.17 | 1.460 | N/A |
| parallel | 1 | shotium | pass | yes | 36.109 | 272.676 | 18.794 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 179.288 | 422.838 | 5.108 | 4.97× |
| parallel | 2 | puppeteer-chrome | fail | no | 370.301 | 154242.603 | 0.185 | N/A |
| parallel | 2 | shotium | noisy | no | 71.22 | 281.619 | 3.098 | N/A |
| parallel | 2 | playwright-shell | pass | no | 249.305 | 451.072 | 6.820 | N/A |
| parallel | 2 | playwright-chrome | fail | no | 328.657 | 608.793 | 3.088 | N/A |
| parallel | 2 | puppeteer-shell | pass | no | 296.733 | 516.232 | 6.186 | N/A |
| parallel | 4 | playwright-shell | pass | no | 442.746 | 800.577 | 7.620 | N/A |
| parallel | 4 | playwright-chrome | pass | no | 582.819 | 1386.7 | 6.106 | N/A |
| parallel | 4 | puppeteer-shell | pass | no | 491.527 | 685.978 | 7.162 | N/A |
| parallel | 4 | puppeteer-chrome | fail | no | 676.318 | 180512.368 | 0.067 | N/A |
| parallel | 4 | shotium | noisy | no | 143.852 | 350.233 | 10.357 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 66.101 | 111.294 | 14.596 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 79.208 | 120.67 | 12.053 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 72.195 | 107.49 | 13.200 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 84.693 | 113.878 | 11.202 | N/A |
| resident | 1 | puppeteer-shell | fail | no | 1021 | 1066 | 0.050 | N/A |
| resident | 1 | shotium | noisy | no | 352 | 408 | 0.031 | N/A |
| resident | 1 | playwright-shell | fail | no | 1117 | 1240 | 0.028 | N/A |
| resident | 1 | playwright-chrome | noisy | no | 1121 | 1266 | 0.046 | N/A |
| resident | 1 | puppeteer-chrome | fail | no | 1046 | 1097 | 0.008 | N/A |
| faults | 1 | puppeteer-shell | pass | no | 27822.443 | 27822.443 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 24277.087 | 24277.087 | N/A | N/A |
| faults | 1 | shotium | pass | no | 8054.112 | 8054.112 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 19465.011 | 19465.011 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 21286.729 | 21286.729 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 231.807 | 515.329 | 15.507 | 2.72× |
| soak | 4 | puppeteer-chrome | fail | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 329.717 | 649.973 | 11.431 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 291.064 | 637.054 | 12.887 | 3.41× |
| soak | 4 | shotium | pass | yes | 85.376 | 361.597 | 28.533 | 1.00× |

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
| cold | 1 | shotium | pass | no | 69 | 73 | 14.374 | N/A |
| cold-settled | 1 | shotium | pass | no | 14.81 | 15.011 | 66.336 | N/A |
| lifecycle | 1 | shotium | pass | no | 94.36 | 129.617 | 10.077 | N/A |
| warm | 1 | shotium | pass | no | 14.532 | 15.095 | 66.677 | N/A |
| batch | 1 | shotium | pass | no | 32.382 | 272.283 | 19.443 | N/A |
| parallel | 1 | shotium | pass | no | 31.685 | 450.316 | 18.785 | N/A |
| parallel | 2 | shotium | pass | no | 66.743 | 298.375 | 20.601 | N/A |
| parallel | 4 | shotium | pass | no | 124.605 | 354.788 | 20.440 | N/A |
| resident | 1 | shotium | noisy | no | 556 | 556 | 0.007 | N/A |
| faults | 1 | shotium | pass | no | 11019.329 | 11019.329 | N/A | N/A |
| soak | 4 | shotium | pass | no | 136.955 | 368.991 | 22.493 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 8 / 8 | 8 |
| 2 | puppeteer-shell | 6.541× | 6 | 8 / 8 | 0 |
| not ranked (partial coverage) | playwright-shell | 4.339× | 3 | 3 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 19.810× | 3 | 3 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 16.850× | 2 | 2 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`
- playwright-shell: `cold/c1`, `lifecycle/c1`, `soak/c4`
- playwright-chrome: `cold/c1`, `lifecycle/c1`, `parallel/c4`
- puppeteer-chrome: `cold/c1`, `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 3078 | 6947 | 0.262 | 25.02× |
| cold | 1 | playwright-shell | pass | yes | 604 | 952 | 1.471 | 4.91× |
| cold | 1 | puppeteer-chrome | pass | yes | 2038 | 3261 | 0.454 | 16.57× |
| cold | 1 | puppeteer-shell | pass | yes | 759 | 1341 | 1.140 | 6.17× |
| cold | 1 | shotium | pass | yes | 123 | 177 | 7.813 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 8213.746 | 9447.987 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 803.785 | 869.345 | 0.326 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | yes | 295.636 | 345.316 | 3.394 | 19.92× |
| cold-settled | 1 | shotium | pass | yes | 14.842 | 18.937 | 61.927 | 1.00× |
| cold-settled | 1 | playwright-shell | noisy | no | 254.838 | 323.783 | 0.219 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 536.843 | 670.257 | 1.842 | 5.21× |
| lifecycle | 1 | playwright-chrome | pass | yes | 2345.173 | 2961.917 | 0.423 | 22.74× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 866.874 | 1050.285 | 1.140 | 8.41× |
| lifecycle | 1 | shotium | pass | yes | 103.109 | 141.777 | 9.137 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1766.798 | 2166.659 | 0.556 | 17.14× |
| warm | 1 | playwright-chrome | noisy | no | 847.61 | 1270.736 | 0.485 | N/A |
| warm | 1 | shotium | noisy | no | 16.98 | 25.67 | 4.087 | N/A |
| warm | 1 | playwright-shell | noisy | no | 168.142 | 252.622 | 0.318 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 241.849 | 281.734 | 1.531 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 9868.736 | 12592.93 | N/A | N/A |
| batch | 1 | shotium | pass | yes | 42.497 | 406.584 | 14.939 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 281.317 | 911.807 | 3.160 | 6.62× |
| batch | 1 | playwright-shell | noisy | no | 292.178 | 568.642 | 0.664 | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 8884.993 | 12209.479 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | 868.368 | 1490.472 | 0.785 | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 922.823 | 1156.037 | 0.930 | N/A |
| parallel | 1 | puppeteer-shell | pass | yes | 326.671 | 776.175 | 2.893 | 6.82× |
| parallel | 1 | puppeteer-chrome | noisy | no | 9489.536 | 10864.244 | N/A | N/A |
| parallel | 1 | shotium | pass | yes | 47.905 | 411.661 | 14.981 | 1.00× |
| parallel | 1 | playwright-shell | noisy | no | 259.344 | 568.528 | 2.429 | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | 11626.054 | 14454.886 | N/A | N/A |
| parallel | 2 | shotium | pass | yes | 109.09 | 300.156 | 15.139 | 1.00× |
| parallel | 2 | playwright-shell | noisy | no | 425.769 | 768.129 | 2.146 | N/A |
| parallel | 2 | playwright-chrome | fail | no | 1521.539 | 2567.372 | 1.013 | N/A |
| parallel | 2 | puppeteer-shell | pass | yes | 488.976 | 815.361 | 3.944 | 4.48× |
| parallel | 4 | playwright-shell | noisy | no | 744.117 | 1158.713 | 1.647 | N/A |
| parallel | 4 | playwright-chrome | pass | yes | 3008.596 | 5830.935 | 1.334 | 13.66× |
| parallel | 4 | puppeteer-shell | pass | yes | 844.162 | 1638.433 | 4.201 | 3.83× |
| parallel | 4 | puppeteer-chrome | noisy | no | 11390.965 | 13870.628 | N/A | N/A |
| parallel | 4 | shotium | pass | yes | 220.262 | 447.182 | 14.595 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 98.832 | 162.093 | 9.444 | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | 9979.812 | 9979.812 | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | 8038.27 | 8038.27 | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 10114.126 | 10114.126 | N/A | N/A |
| resident | 1 | puppeteer-shell | pass | no | 1528 | 3341 | 0.535 | N/A |
| resident | 1 | shotium | noisy | no | 1494 | 1494 | 0.008 | N/A |
| resident | 1 | playwright-shell | pass | no | 1709 | 2456 | 0.553 | N/A |
| resident | 1 | playwright-chrome | fail | no | 2904.5 | 2992 | 0.087 | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 1646 | 2066 | 0.067 | N/A |
| faults | 1 | puppeteer-shell | pass | no | 23190.947 | 23190.947 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | 7340.269 | 7340.269 | N/A | N/A |
| faults | 1 | shotium | pass | no | 11324.347 | 11324.347 | N/A | N/A |
| faults | 1 | playwright-shell | noisy | no | 5355.371 | 5355.371 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 36752.244 | 36752.244 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 569.903 | 2447.621 | 6.681 | 3.20× |
| soak | 4 | puppeteer-chrome | noisy | no | 10874.568 | 10874.568 | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 745.739 | 1454.696 | 5.169 | 4.18× |
| soak | 4 | shotium | pass | yes | 178.346 | 516.419 | 18.572 | 1.00× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 3 eligible cell(s), with 3 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 3 | 3 / 3 | 3 |
| 2 | playwright-shell | 4.247× | 3 | 3 / 3 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 7.734× | 2 | 2 / 3 | 0 |
| not ranked (partial coverage) | playwright-chrome | 13.355× | 2 | 2 / 3 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 20.972× | 1 | 1 / 3 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `lifecycle/c1`, `soak/c4`
- playwright-shell: `batch/c1`, `lifecycle/c1`, `soak/c4`
- puppeteer-shell: `batch/c1`, `lifecycle/c1`
- playwright-chrome: `batch/c1`, `lifecycle/c1`
- puppeteer-chrome: `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | no | 1988 | 6421 | 0.353 | N/A |
| cold | 1 | playwright-shell | noisy | no | 482 | 776 | 1.882 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 1664.5 | 5009 | 0.457 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 523 | 1028 | 1.692 | N/A |
| cold | 1 | shotium | noisy | no | 70 | 80 | 14.245 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 5735.257 | 7444.909 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 221.643 | 270.432 | 4.675 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 242.127 | 304.505 | 0.449 | N/A |
| cold-settled | 1 | shotium | noisy | no | 9.898 | 11.069 | 3.484 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 149.194 | 244.301 | 1.467 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 296.34 | 416.475 | 3.271 | 4.55× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1332.375 | 2070.341 | 0.725 | 20.46× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 422.065 | 1062.203 | 2.068 | 6.48× |
| lifecycle | 1 | shotium | pass | yes | 65.12 | 295.381 | 11.538 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1365.69 | 2140.791 | 0.684 | 20.97× |
| warm | 1 | playwright-chrome | noisy | no | 224.201 | 314.806 | 4.099 | N/A |
| warm | 1 | shotium | noisy | no | 11.242 | 17.911 | 81.069 | N/A |
| warm | 1 | playwright-shell | noisy | no | 148.297 | 169.41 | 6.795 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 221.013 | 345.385 | 4.570 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 6297.684 | 7308.872 | N/A | N/A |
| batch | 1 | shotium | pass | yes | 25.074 | 270.702 | 21.637 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 231.412 | 557.545 | 4.084 | 9.23× |
| batch | 1 | playwright-shell | pass | yes | 133.159 | 380.706 | 6.609 | 5.31× |
| batch | 1 | puppeteer-chrome | noisy | no | 5450.567 | 6243.92 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | yes | 218.566 | 1715.019 | 3.048 | 8.72× |
| parallel | 1 | playwright-chrome | noisy | no | 209.632 | 1503.049 | 2.224 | N/A |
| parallel | 1 | puppeteer-shell | noisy | no | 237.457 | 524.216 | 2.699 | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | 5184.679 | 6818.47 | N/A | N/A |
| parallel | 1 | shotium | pass | no | 25.693 | 278.38 | 24.184 | N/A |
| parallel | 1 | playwright-shell | noisy | no | 128.569 | 370.374 | 5.234 | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | 5688.965 | 6563.626 | N/A | N/A |
| parallel | 2 | shotium | noisy | no | 53.461 | 283.424 | 14.952 | N/A |
| parallel | 2 | playwright-shell | noisy | no | 179.483 | 374.982 | 7.124 | N/A |
| parallel | 2 | playwright-chrome | fail | no | 425.963 | 1137.192 | 4.069 | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | 303.231 | 511.569 | 4.224 | N/A |
| parallel | 4 | playwright-shell | pass | no | 409.402 | 868.579 | 8.047 | N/A |
| parallel | 4 | playwright-chrome | fail | no | 772.929 | 2447.481 | 4.214 | N/A |
| parallel | 4 | puppeteer-shell | pass | no | 465.889 | 997.728 | 7.004 | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | 6578.83 | 7863.334 | N/A | N/A |
| parallel | 4 | shotium | noisy | no | 106.726 | 385.562 | 15.447 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 48.57 | 52.934 | 21.539 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 99.523 | 171.577 | 9.829 | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | 6099.149 | 6099.149 | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 5424.533 | 5424.533 | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 740 | 817 | 0.176 | N/A |
| resident | 1 | shotium | noisy | no | 317 | 318 | 0.150 | N/A |
| resident | 1 | playwright-shell | noisy | no | 731 | 941 | 1.323 | N/A |
| resident | 1 | playwright-chrome | noisy | no | 1019 | 1317 | 0.987 | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 16794.261 | 20782.937 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 9205.271 | 9205.271 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | 3832.892 | 3832.892 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10704.524 | 10704.524 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13458.676 | 13458.676 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 21837.656 | 21837.656 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 269.745 | 591 | 13.656 | 3.17× |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | 10409.123 | 10409.123 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 85.096 | 371.634 | 28.107 | 1.00× |

