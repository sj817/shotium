# Shotium 0.6.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `07ac95e4a85dad12bc7fd8029c5ca229e3e38d9f`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 3 | 9 |
| win32-arm64 | pass | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 6 |
| darwin-arm64 | pass | shotium | 3 | 10 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 6.358× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 6.609× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 7.803× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 10.261× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.6.0/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 784 | 792 | 1.281 | 13.52× |
| cold | 1 | puppeteer-chrome | pass | yes | 900 | 938 | 1.102 | 15.52× |
| cold | 1 | puppeteer-shell | pass | yes | 656 | 666 | 1.522 | 11.31× |
| cold | 1 | playwright-chrome | pass | yes | 972 | 987 | 1.032 | 16.76× |
| cold | 1 | shotium | pass | yes | 58 | 64 | 16.908 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 170.369 | 176.306 | 5.915 | 10.85× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 137.124 | 143.309 | 7.339 | 8.74× |
| cold-settled | 1 | playwright-shell | pass | yes | 125.216 | 126.643 | 8.024 | 7.98× |
| cold-settled | 1 | shotium | pass | yes | 15.695 | 19.502 | 61.432 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 162.935 | 197.419 | 5.967 | 10.38× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 676.445 | 737.884 | 1.465 | 11.50× |
| lifecycle | 1 | playwright-shell | pass | yes | 663.95 | 737.347 | 1.466 | 11.29× |
| lifecycle | 1 | playwright-chrome | pass | yes | 885.762 | 980.708 | 1.122 | 15.06× |
| lifecycle | 1 | shotium | pass | yes | 58.834 | 112.207 | 14.178 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 945.073 | 1005.067 | 1.051 | 16.06× |
| warm | 1 | puppeteer-shell | pass | yes | 132.648 | 147.078 | 7.536 | 11.36× |
| warm | 1 | playwright-shell | pass | yes | 116.55 | 146.747 | 8.291 | 9.98× |
| warm | 1 | playwright-chrome | pass | yes | 149.088 | 316.79 | 6.399 | 12.77× |
| warm | 1 | shotium | pass | yes | 11.678 | 14.428 | 81.453 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 162.294 | 181.897 | 6.267 | 13.90× |
| batch | 1 | shotium | pass | yes | 17.997 | 260.882 | 25.120 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 179.886 | 405.638 | 4.887 | 10.00× |
| batch | 1 | playwright-chrome | pass | yes | 166.961 | 390.387 | 5.194 | 9.28× |
| batch | 1 | playwright-shell | pass | yes | 133.142 | 365.575 | 6.234 | 7.40× |
| batch | 1 | puppeteer-shell | pass | yes | 142.715 | 373.717 | 5.767 | 7.93× |
| parallel | 1 | puppeteer-chrome | pass | yes | 185.047 | 418.968 | 4.742 | 9.40× |
| parallel | 1 | puppeteer-shell | pass | yes | 149.957 | 387.074 | 5.670 | 7.62× |
| parallel | 1 | shotium | pass | yes | 19.678 | 263.544 | 24.259 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 168.671 | 399.597 | 5.105 | 8.57× |
| parallel | 1 | playwright-shell | pass | yes | 139.986 | 374.955 | 5.932 | 7.11× |
| parallel | 2 | puppeteer-shell | pass | yes | 232.242 | 529.923 | 7.628 | 4.41× |
| parallel | 2 | shotium | pass | yes | 52.638 | 300.136 | 23.956 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 258.189 | 545.155 | 6.806 | 4.90× |
| parallel | 2 | playwright-shell | pass | yes | 220.321 | 456.421 | 8.145 | 4.19× |
| parallel | 2 | puppeteer-chrome | fail | no | 262.645 | 934.132 | 3.483 | N/A |
| parallel | 4 | shotium | pass | yes | 117.505 | 387.393 | 23.939 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 476.592 | 735.165 | 7.851 | 4.06× |
| parallel | 4 | playwright-shell | pass | yes | 382.556 | 695.577 | 9.365 | 3.26× |
| parallel | 4 | puppeteer-chrome | fail | no | 294.688 | 16741.83 | 3.475 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 427.691 | 697.306 | 8.864 | 3.64× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.945 | 117.086 | 9.818 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.259 | 90.319 | 11.946 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 83.611 | 100.733 | 11.666 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.077 | 116.055 | 9.845 | N/A |
| resident | 1 | playwright-shell | pass | yes | 947 | 1001 | 1.113 | 3.79× |
| resident | 1 | puppeteer-chrome | pass | yes | 847 | 946 | 1.266 | 3.39× |
| resident | 1 | puppeteer-shell | pass | yes | 864 | 923 | 1.152 | 3.46× |
| resident | 1 | playwright-chrome | pass | yes | 959 | 1074 | 1.098 | 3.84× |
| resident | 1 | shotium | pass | yes | 250 | 306 | 4.551 | 1.00× |
| faults | 1 | puppeteer-chrome | pass | no | 13331.826 | 13331.826 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10962.269 | 10962.269 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 17593.582 | 17593.582 | N/A | N/A |
| faults | 1 | shotium | pass | no | 11315.143 | 11315.143 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16474.783 | 16474.783 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 482.106 | 884.53 | 7.958 | 4.13× |
| soak | 4 | puppeteer-shell | pass | yes | 428.937 | 886.393 | 8.934 | 3.67× |
| soak | 4 | puppeteer-chrome | fail | no | 270.32 | 29583.086 | 4.177 | N/A |
| soak | 4 | playwright-shell | pass | yes | 382.072 | 798.212 | 9.603 | 3.27× |
| soak | 4 | shotium | pass | yes | 116.85 | 392.962 | 24.334 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.549× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.398× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-arm64@0.6.0/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 821 | 896 | 1.199 | 13.46× |
| cold | 1 | shotium | pass | yes | 61 | 64 | 16.432 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 645 | 897 | 1.471 | 10.57× |
| cold-settled | 1 | playwright-shell | pass | yes | 126.589 | 141.731 | 7.872 | 3.92× |
| cold-settled | 1 | playwright-chrome | pass | yes | 151.134 | 161.065 | 6.565 | 4.68× |
| cold-settled | 1 | shotium | pass | yes | 32.32 | 41.282 | 29.058 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 570.844 | 634.845 | 1.729 | 8.18× |
| lifecycle | 1 | playwright-chrome | pass | yes | 726.621 | 811.308 | 1.354 | 10.42× |
| lifecycle | 1 | shotium | pass | yes | 69.766 | 93.37 | 13.755 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 147.6 | 154.945 | 6.976 | 5.02× |
| warm | 1 | shotium | pass | yes | 29.428 | 32.857 | 33.534 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 116.497 | 132.265 | 8.414 | 3.96× |
| batch | 1 | playwright-chrome | pass | yes | 155.236 | 372.768 | 5.522 | 4.58× |
| batch | 1 | shotium | pass | yes | 33.914 | 260.078 | 19.244 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 125.858 | 356.725 | 6.650 | 3.71× |
| parallel | 1 | shotium | pass | yes | 34.705 | 261.094 | 18.952 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 129.422 | 371.5 | 6.554 | 3.73× |
| parallel | 1 | playwright-chrome | pass | yes | 155.003 | 387.132 | 5.539 | 4.47× |
| parallel | 2 | playwright-shell | pass | yes | 180.885 | 430.163 | 9.678 | 2.51× |
| parallel | 2 | playwright-chrome | pass | yes | 223.697 | 525.38 | 7.984 | 3.10× |
| parallel | 2 | shotium | pass | yes | 72.084 | 311.866 | 19.219 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 375.385 | 656.02 | 10.041 | 2.42× |
| parallel | 4 | shotium | pass | yes | 155.319 | 408.013 | 19.221 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 279.95 | 526.269 | 12.323 | 1.80× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.297 | 92.159 | 11.851 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.266 | 86.716 | 12.001 | N/A |
| resident | 1 | playwright-shell | pass | yes | 860 | 899 | 1.186 | 2.01× |
| resident | 1 | shotium | pass | yes | 428 | 442 | 2.513 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 924 | 954 | 1.077 | 2.16× |
| faults | 1 | playwright-shell | pass | no | 18087.771 | 18087.771 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10368.126 | 10368.126 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16051.03 | 16051.03 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 164.005 | 424.151 | 18.588 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 408.379 | 726.104 | 9.372 | 2.49× |
| soak | 4 | playwright-shell | pass | yes | 307.96 | 622.772 | 11.846 | 1.88× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 6.144× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 6.590× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 10.921× | 6 | 6 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 9.455× | 5 | 5 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`
- playwright-chrome: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.6.0\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | pass | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 625 | 929 | 1.499 | 11.16× |
| cold | 1 | puppeteer-chrome | pass | yes | 844 | 950 | 1.159 | 15.07× |
| cold | 1 | puppeteer-shell | pass | yes | 726 | 744 | 1.383 | 12.96× |
| cold | 1 | playwright-chrome | pass | yes | 783 | 843 | 1.261 | 13.98× |
| cold | 1 | shotium | pass | yes | 56 | 63 | 17.544 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | no | 155.924 | 159.006 | 6.522 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | no | 144.767 | 148.537 | 6.959 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 132.79 | 167.854 | 7.251 | N/A |
| cold-settled | 1 | shotium | noisy | no | 11.331 | 11.622 | 0.745 | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 136.811 | 147.383 | 7.406 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1433.023 | 3532.27 | 0.648 | 16.87× |
| lifecycle | 1 | playwright-shell | pass | yes | 956.783 | 1857.135 | 0.968 | 11.26× |
| lifecycle | 1 | playwright-chrome | pass | yes | 2513.692 | 3557.501 | 0.378 | 29.58× |
| lifecycle | 1 | shotium | pass | yes | 84.969 | 3640.934 | 3.669 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3377.724 | 5808.946 | 0.277 | 39.75× |
| warm | 1 | puppeteer-shell | pass | yes | 166.551 | 191.06 | 5.929 | 12.90× |
| warm | 1 | playwright-shell | pass | yes | 164.35 | 196.006 | 6.084 | 12.73× |
| warm | 1 | playwright-chrome | noisy | no | 163.979 | 186.039 | 0.990 | N/A |
| warm | 1 | shotium | pass | yes | 12.913 | 14.002 | 73.496 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 182.449 | 221.521 | 5.429 | 14.13× |
| batch | 1 | shotium | pass | yes | 36.71 | 6769.637 | 5.642 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 204.304 | 465.409 | 4.342 | 5.57× |
| batch | 1 | playwright-chrome | noisy | no | 11331.638 | 11331.638 | N/A | N/A |
| batch | 1 | playwright-shell | pass | yes | 180.878 | 449.009 | 4.851 | 4.93× |
| batch | 1 | puppeteer-shell | pass | yes | 200.143 | 433.084 | 4.516 | 5.45× |
| parallel | 1 | puppeteer-chrome | pass | yes | 204.154 | 982.085 | 4.111 | 8.45× |
| parallel | 1 | puppeteer-shell | pass | yes | 190.249 | 418.53 | 4.635 | 7.87× |
| parallel | 1 | shotium | pass | yes | 24.173 | 272.684 | 21.999 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 197.783 | 433.091 | 4.451 | 8.18× |
| parallel | 1 | playwright-shell | pass | yes | 208.521 | 426.838 | 4.369 | 8.63× |
| parallel | 2 | puppeteer-shell | pass | yes | 279.059 | 471.929 | 6.574 | 4.15× |
| parallel | 2 | shotium | pass | yes | 67.184 | 303.295 | 21.251 | 1.00× |
| parallel | 2 | playwright-chrome | fail | no | 311.794 | 803.16 | 5.803 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 246.074 | 533.765 | 7.080 | 3.66× |
| parallel | 2 | puppeteer-chrome | noisy | no | 9791.506 | 9791.506 | N/A | N/A |
| parallel | 4 | shotium | pass | yes | 136.52 | 436.635 | 21.163 | 1.00× |
| parallel | 4 | playwright-chrome | noisy | no | 14572.334 | 14572.334 | N/A | N/A |
| parallel | 4 | playwright-shell | pass | yes | 436.466 | 730.293 | 8.958 | 3.20× |
| parallel | 4 | puppeteer-chrome | fail | no | 396.279 | 19456.632 | 2.819 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 448.774 | 809.542 | 8.361 | 3.29× |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 12218.218 | 12218.218 | N/A | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.736 | 93.98 | 11.662 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 99.682 | 109.799 | 10.029 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 102.338 | 136.828 | 9.403 | N/A |
| resident | 1 | playwright-shell | pass | yes | 754 | 1050 | 1.237 | 4.74× |
| resident | 1 | puppeteer-chrome | pass | yes | 678 | 1451 | 1.057 | 4.26× |
| resident | 1 | puppeteer-shell | pass | yes | 647 | 966 | 1.439 | 4.07× |
| resident | 1 | playwright-chrome | pass | yes | 917 | 1317 | 1.062 | 5.77× |
| resident | 1 | shotium | pass | yes | 159 | 1562 | 2.481 | 1.00× |
| faults | 1 | puppeteer-chrome | pass | no | 35555.031 | 35555.031 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 24477.466 | 24477.466 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 29509.111 | 29509.111 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9596.453 | 9596.453 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 26602.487 | 26602.487 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 555.717 | 1278.084 | 6.885 | 3.87× |
| soak | 4 | puppeteer-shell | pass | yes | 500.158 | 1119.273 | 7.689 | 3.48× |
| soak | 4 | puppeteer-chrome | noisy | no | 9614.892 | 9614.892 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 474.687 | 1059.327 | 8.115 | 3.31× |
| soak | 4 | shotium | pass | yes | 143.547 | 413.045 | 21.709 | 1.00× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-arm64@0.6.0\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 72 | 75 | 13.917 | N/A |
| cold-settled | 1 | shotium | pass | no | 12.662 | 12.935 | 77.713 | N/A |
| lifecycle | 1 | shotium | pass | no | 96.774 | 122.534 | 10.122 | N/A |
| warm | 1 | shotium | pass | no | 12.109 | 14.718 | 77.692 | N/A |
| batch | 1 | shotium | pass | no | 21.417 | 271.965 | 22.835 | N/A |
| parallel | 1 | shotium | pass | no | 25.181 | 277.926 | 21.005 | N/A |
| parallel | 2 | shotium | pass | no | 52.523 | 289.726 | 23.766 | N/A |
| parallel | 4 | shotium | pass | no | 112.724 | 349.734 | 23.644 | N/A |
| resident | 1 | shotium | pass | no | 287 | 807 | 2.481 | N/A |
| faults | 1 | shotium | pass | no | 11080.503 | 11080.503 | N/A | N/A |
| soak | 4 | shotium | pass | no | 109.584 | 405.209 | 24.569 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 6 eligible cell(s), with 6 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 6 / 6 | 6 |
| 2 | playwright-shell | 7.937× | 6 | 6 / 6 | 0 |
| 3 | puppeteer-shell | 8.241× | 6 | 6 / 6 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 13.713× | 5 | 5 / 6 | 0 |
| not ranked (partial coverage) | playwright-chrome | 21.446× | 5 | 5 / 6 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.6.0/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 1485 | 3094 | 0.558 | 12.80× |
| cold | 1 | puppeteer-chrome | pass | yes | 3724 | 7060 | 0.243 | 32.10× |
| cold | 1 | puppeteer-shell | pass | yes | 1667 | 3156 | 0.512 | 14.37× |
| cold | 1 | playwright-chrome | pass | yes | 4126 | 5334 | 0.227 | 35.57× |
| cold | 1 | shotium | pass | yes | 116 | 156 | 8.235 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 425.31 | 893.37 | 2.223 | 23.90× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 358.245 | 460.022 | 2.714 | 20.13× |
| cold-settled | 1 | playwright-shell | pass | yes | 317.333 | 540.77 | 3.017 | 17.83× |
| cold-settled | 1 | shotium | pass | yes | 17.796 | 22.152 | 54.260 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 901.001 | 1041.639 | 1.113 | 50.63× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1998.561 | 2315.297 | 0.501 | 14.74× |
| lifecycle | 1 | playwright-shell | pass | yes | 1446.099 | 1778.68 | 0.679 | 10.67× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3557.196 | 4058.558 | 0.279 | 26.24× |
| lifecycle | 1 | shotium | pass | yes | 135.575 | 219.71 | 6.429 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3338.511 | 3802.103 | 0.296 | 24.62× |
| warm | 1 | puppeteer-shell | pass | no | 305.775 | 439.427 | 3.259 | N/A |
| warm | 1 | playwright-shell | noisy | no | 325.075 | 428.786 | 0.971 | N/A |
| warm | 1 | playwright-chrome | pass | no | 864.761 | 1157.349 | 1.162 | N/A |
| warm | 1 | shotium | noisy | no | 19.386 | 26.793 | 1.950 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 433.275 | 670.004 | 0.581 | N/A |
| batch | 1 | shotium | pass | yes | 35.317 | 387.445 | 16.493 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 455.338 | 1135.082 | 2.130 | 12.89× |
| batch | 1 | playwright-chrome | pass | yes | 937.472 | 1377.199 | 1.053 | 26.54× |
| batch | 1 | playwright-shell | pass | yes | 337.42 | 733.562 | 2.754 | 9.55× |
| batch | 1 | puppeteer-shell | pass | yes | 375.42 | 751.44 | 2.472 | 10.63× |
| parallel | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | fail | no | 3015.168 | 6029.944 | 1.306 | N/A |
| parallel | 4 | playwright-shell | pass | no | 741.59 | 1150.868 | 5.281 | N/A |
| parallel | 4 | puppeteer-chrome | fail | no | 725.673 | 13618.329 | 1.224 | N/A |
| parallel | 4 | puppeteer-shell | pass | no | 827.854 | 1583.513 | 4.739 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 271.274 | 368.015 | 3.601 | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 243.124 | 279.641 | 4.339 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 242.124 | 279.138 | 4.385 | N/A |
| resident | 1 | playwright-shell | pass | yes | 2085 | 2342 | 0.482 | 1.98× |
| resident | 1 | puppeteer-chrome | pass | yes | 2096 | 3326 | 0.433 | 1.99× |
| resident | 1 | puppeteer-shell | pass | yes | 1930 | 2033 | 0.523 | 1.83× |
| resident | 1 | playwright-chrome | pass | yes | 3809 | 4008 | 0.278 | 3.62× |
| resident | 1 | shotium | pass | yes | 1053 | 1521 | 0.869 | 1.00× |
| faults | 1 | puppeteer-chrome | pass | no | 32781.836 | 32781.836 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 19556.872 | 19556.872 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 44344.182 | 44344.182 | N/A | N/A |
| faults | 1 | shotium | pass | no | 13089.738 | 13089.738 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 23356.749 | 23356.749 | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 2822.537 | 6650.893 | 1.427 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 710.804 | 3058.373 | 5.164 | 3.77× |
| soak | 4 | puppeteer-chrome | fail | no | 674.597 | 9310.993 | 2.769 | N/A |
| soak | 4 | playwright-shell | pass | yes | 1023.298 | 3058.07 | 3.612 | 5.43× |
| soak | 4 | shotium | pass | yes | 188.579 | 683.326 | 17.196 | 1.00× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 7.601× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 10.207× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 16.575× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 15.257× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.6.0/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | pass | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | fail | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 966 | 1168 | 1.002 | 9.56× |
| cold | 1 | puppeteer-chrome | pass | yes | 2589 | 8618 | 0.285 | 25.63× |
| cold | 1 | puppeteer-shell | pass | yes | 1248 | 1559 | 0.809 | 12.36× |
| cold | 1 | playwright-chrome | pass | yes | 3017 | 7082 | 0.290 | 29.87× |
| cold | 1 | shotium | pass | yes | 101 | 110 | 10.057 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 377.957 | 453.659 | 2.692 | 39.15× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 277.039 | 468.174 | 3.401 | 28.69× |
| cold-settled | 1 | playwright-shell | pass | yes | 170.594 | 205.938 | 5.792 | 17.67× |
| cold-settled | 1 | shotium | pass | yes | 9.655 | 27.525 | 80.130 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 199.998 | 290.483 | 4.509 | 20.71× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 924.433 | 1512.828 | 1.014 | 10.26× |
| lifecycle | 1 | playwright-shell | pass | yes | 772.396 | 1372.272 | 1.169 | 8.58× |
| lifecycle | 1 | playwright-chrome | pass | yes | 2055.005 | 2912.444 | 0.475 | 22.82× |
| lifecycle | 1 | shotium | pass | yes | 90.061 | 133.096 | 11.426 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2208.392 | 3850.978 | 0.432 | 24.52× |
| warm | 1 | puppeteer-shell | pass | yes | 238.809 | 373.864 | 4.083 | 25.48× |
| warm | 1 | playwright-shell | pass | yes | 168.251 | 343.163 | 5.571 | 17.95× |
| warm | 1 | playwright-chrome | pass | yes | 201.43 | 254.338 | 4.959 | 21.50× |
| warm | 1 | shotium | pass | yes | 9.371 | 18.733 | 94.951 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 339.831 | 537.874 | 2.951 | 36.26× |
| batch | 1 | shotium | pass | yes | 11.99 | 267.955 | 28.850 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 332.947 | 1474.974 | 2.597 | 27.77× |
| batch | 1 | playwright-chrome | pass | yes | 197.309 | 1660.814 | 3.732 | 16.46× |
| batch | 1 | playwright-shell | pass | yes | 142.88 | 403.038 | 6.076 | 11.92× |
| batch | 1 | puppeteer-shell | pass | yes | 287.256 | 627.121 | 3.353 | 23.96× |
| parallel | 1 | puppeteer-chrome | pass | yes | 263.305 | 1550.967 | 3.114 | 14.34× |
| parallel | 1 | puppeteer-shell | pass | yes | 231.141 | 572.311 | 4.028 | 12.58× |
| parallel | 1 | shotium | pass | yes | 18.367 | 265.956 | 26.138 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 224.491 | 1411.11 | 3.452 | 12.22× |
| parallel | 1 | playwright-shell | pass | yes | 165.678 | 424.513 | 5.294 | 9.02× |
| parallel | 2 | puppeteer-shell | pass | yes | 320.603 | 646.344 | 5.992 | 7.17× |
| parallel | 2 | shotium | pass | yes | 44.724 | 312.969 | 26.039 | 1.00× |
| parallel | 2 | playwright-chrome | fail | no | 344.475 | 1171.018 | 4.921 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 239.132 | 472.661 | 7.828 | 5.35× |
| parallel | 2 | puppeteer-chrome | pass | yes | 514.862 | 1672.034 | 3.207 | 11.51× |
| parallel | 4 | shotium | pass | yes | 90.001 | 380.253 | 26.067 | 1.00× |
| parallel | 4 | playwright-chrome | fail | no | 703.048 | 3076.52 | 5.132 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 418.947 | 869.87 | 8.707 | 4.65× |
| parallel | 4 | puppeteer-chrome | pass | yes | 524.943 | 27342.748 | 3.584 | 5.83× |
| parallel | 4 | puppeteer-shell | pass | yes | 454.187 | 732.349 | 8.366 | 5.05× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 223.059 | 264.028 | 4.531 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 88.969 | 157.513 | 10.378 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 132.761 | 221.784 | 7.159 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 180.544 | 241.887 | 5.747 | N/A |
| resident | 1 | playwright-shell | pass | yes | 1138 | 1453 | 0.891 | 2.14× |
| resident | 1 | puppeteer-chrome | pass | yes | 2102 | 2558 | 0.486 | 3.96× |
| resident | 1 | puppeteer-shell | pass | yes | 1092 | 1209 | 0.908 | 2.06× |
| resident | 1 | playwright-chrome | pass | yes | 1674 | 2271 | 0.608 | 3.15× |
| resident | 1 | shotium | pass | yes | 531 | 738 | 1.797 | 1.00× |
| faults | 1 | puppeteer-chrome | pass | no | 23139.515 | 23139.515 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10606.628 | 10606.628 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 22906.065 | 22906.065 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9557.88 | 9557.88 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13381.143 | 13381.143 | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 541.876 | 4281.187 | 6.853 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 384.333 | 808.644 | 10.160 | 5.90× |
| soak | 4 | puppeteer-chrome | fail | no | 425.601 | 28127.829 | 4.243 | N/A |
| soak | 4 | playwright-shell | pass | yes | 281.009 | 825.423 | 13.128 | 4.31× |
| soak | 4 | shotium | pass | yes | 65.132 | 358.95 | 31.023 | 1.00× |

