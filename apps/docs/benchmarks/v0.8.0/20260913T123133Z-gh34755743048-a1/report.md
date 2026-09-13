# Shotium 0.8.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `5f374c834ffcc696504fb1b1d56a0b63b380e082`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 4 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 1 | 10 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 10 |
| darwin-arm64 | noisy | shotium | 4 | 3 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 6.040× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 6.301× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 8.619× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 7.450× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.8.0/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 56 | 57 | 18.088 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 584 | 607 | 1.717 | 10.43× |
| cold | 1 | playwright-chrome | pass | yes | 813 | 1013 | 1.218 | 14.52× |
| cold | 1 | puppeteer-shell | pass | yes | 466 | 479 | 2.153 | 8.32× |
| cold | 1 | puppeteer-chrome | pass | yes | 704 | 1677 | 1.183 | 12.57× |
| cold-settled | 1 | playwright-shell | pass | yes | 116.178 | 136.551 | 8.475 | 9.87× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 125.898 | 142.074 | 7.727 | 10.69× |
| cold-settled | 1 | shotium | pass | yes | 11.776 | 12.452 | 84.113 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 151.708 | 202.161 | 6.566 | 12.88× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 141.975 | 187.801 | 6.544 | 12.06× |
| lifecycle | 1 | playwright-chrome | fail | no | 1762.921 | 2309.859 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 53.979 | 82.469 | 16.826 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 800.998 | 1635.998 | 1.080 | 14.84× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 547.168 | 756.712 | 1.780 | 10.14× |
| lifecycle | 1 | playwright-shell | pass | yes | 520.991 | 935.976 | 1.785 | 9.65× |
| warm | 1 | puppeteer-shell | pass | yes | 133.349 | 150.706 | 7.369 | 10.19× |
| warm | 1 | puppeteer-chrome | pass | yes | 199.494 | 211.785 | 5.062 | 15.25× |
| warm | 1 | playwright-chrome | pass | yes | 184.314 | 228.074 | 5.406 | 14.09× |
| warm | 1 | shotium | pass | yes | 13.082 | 18.73 | 70.891 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 147.756 | 165.827 | 6.788 | 11.29× |
| batch | 1 | puppeteer-chrome | pass | yes | 233.237 | 437.107 | 3.924 | 11.60× |
| batch | 1 | puppeteer-shell | pass | yes | 152.883 | 410.206 | 5.485 | 7.61× |
| batch | 1 | playwright-shell | pass | yes | 165.758 | 390.804 | 5.303 | 8.25× |
| batch | 1 | playwright-chrome | pass | yes | 198.776 | 403.726 | 4.462 | 9.89× |
| batch | 1 | shotium | pass | yes | 20.102 | 258.978 | 23.843 | 1.00× |
| parallel | 1 | shotium | pass | yes | 20.425 | 259.137 | 23.581 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 152.121 | 395.641 | 5.558 | 7.45× |
| parallel | 1 | playwright-shell | pass | yes | 155.257 | 389.272 | 5.312 | 7.60× |
| parallel | 1 | puppeteer-chrome | pass | yes | 227.969 | 441.992 | 4.005 | 11.16× |
| parallel | 1 | playwright-chrome | pass | yes | 199.835 | 407.952 | 4.427 | 9.78× |
| parallel | 2 | puppeteer-shell | pass | yes | 239.455 | 533.708 | 7.437 | 4.52× |
| parallel | 2 | playwright-shell | pass | yes | 245.061 | 474.627 | 7.428 | 4.62× |
| parallel | 2 | puppeteer-chrome | pass | yes | 399.187 | 647.304 | 4.817 | 7.53× |
| parallel | 2 | playwright-chrome | pass | yes | 296.242 | 591.706 | 6.179 | 5.59× |
| parallel | 2 | shotium | pass | yes | 53.022 | 294.143 | 23.773 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 464.715 | 725.646 | 8.261 | 3.90× |
| parallel | 4 | puppeteer-chrome | pass | yes | 668.133 | 1017.822 | 5.591 | 5.60× |
| parallel | 4 | playwright-chrome | pass | yes | 547.652 | 848.214 | 7.057 | 4.59× |
| parallel | 4 | shotium | pass | yes | 119.285 | 356.707 | 23.741 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 432.126 | 744.298 | 8.621 | 3.62× |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.885 | 100.743 | 10.212 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.046 | 131.775 | 9.667 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 100.081 | 131.777 | 9.668 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 100.152 | 129.415 | 9.849 | N/A |
| resident | 1 | shotium | pass | yes | 447 | 491 | 2.236 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 966 | 1027 | 1.024 | 2.16× |
| resident | 1 | puppeteer-shell | pass | yes | 912 | 976 | 1.153 | 2.04× |
| resident | 1 | playwright-chrome | pass | yes | 1047 | 1107 | 1.034 | 2.34× |
| resident | 1 | puppeteer-chrome | pass | yes | 941 | 990 | 1.183 | 2.11× |
| faults | 1 | puppeteer-chrome | pass | no | 11543.871 | 11543.871 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4799.842 | 4799.842 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16776.844 | 16776.844 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14107.718 | 14107.718 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 9734.676 | 9734.676 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 94.536 | 336.81 | 27.511 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 351.222 | 690.17 | 10.788 | 3.72× |
| soak | 4 | playwright-shell | pass | yes | 340.691 | 742.802 | 10.517 | 3.60× |
| soak | 4 | playwright-chrome | pass | yes | 436.109 | 777.515 | 8.859 | 4.61× |
| soak | 4 | puppeteer-chrome | pass | yes | 542.493 | 946.368 | 7.138 | 5.74× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.965× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 4.554× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.8.0/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 796 | 832 | 1.250 | 11.88× |
| cold | 1 | playwright-shell | pass | yes | 624 | 655 | 1.598 | 9.31× |
| cold | 1 | shotium | pass | yes | 67 | 72 | 14.675 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 131.094 | 139.809 | 7.582 | 4.26× |
| cold-settled | 1 | shotium | pass | yes | 30.77 | 32.142 | 32.257 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 157.04 | 168.008 | 6.345 | 5.10× |
| lifecycle | 1 | shotium | pass | yes | 77.97 | 93.871 | 12.438 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 1512.895 | 2252.316 | N/A | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 577.555 | 627.768 | 1.708 | 7.41× |
| warm | 1 | playwright-shell | pass | yes | 121.892 | 134.102 | 8.028 | 4.36× |
| warm | 1 | playwright-chrome | pass | yes | 156.11 | 168.835 | 6.365 | 5.58× |
| warm | 1 | shotium | pass | yes | 27.965 | 31.318 | 34.511 | 1.00× |
| batch | 1 | shotium | pass | yes | 34.797 | 260.015 | 19.027 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 143.182 | 374.167 | 6.030 | 4.11× |
| batch | 1 | playwright-chrome | pass | yes | 171.719 | 412.968 | 5.108 | 4.93× |
| parallel | 1 | playwright-shell | pass | yes | 146.307 | 385.168 | 5.819 | 4.01× |
| parallel | 1 | shotium | pass | yes | 36.521 | 266.641 | 18.197 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 186.239 | 419.186 | 4.801 | 5.10× |
| parallel | 2 | shotium | pass | yes | 79.928 | 334.664 | 18.248 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 263.863 | 528.39 | 6.931 | 3.30× |
| parallel | 2 | playwright-shell | pass | yes | 209.813 | 451.34 | 8.549 | 2.63× |
| parallel | 4 | playwright-chrome | pass | yes | 457.744 | 724.199 | 8.368 | 2.74× |
| parallel | 4 | playwright-shell | pass | yes | 343.689 | 614.251 | 10.733 | 2.06× |
| parallel | 4 | shotium | pass | yes | 166.896 | 406.881 | 18.156 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 100.13 | 233.458 | 8.740 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 99.774 | 116.774 | 10.281 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 920 | 970 | 1.074 | 4.16× |
| resident | 1 | playwright-shell | pass | yes | 898 | 911 | 1.130 | 4.06× |
| resident | 1 | shotium | pass | yes | 221 | 254 | 5.564 | 1.00× |
| faults | 1 | shotium | pass | no | 5007.001 | 5007.001 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 17678.551 | 17678.551 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14069.212 | 14069.212 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 434.242 | 792.896 | 8.879 | 2.62× |
| soak | 4 | playwright-shell | pass | yes | 342.864 | 695.914 | 10.808 | 2.07× |
| soak | 4 | shotium | pass | yes | 165.522 | 439.597 | 18.443 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| not ranked (partial coverage) | puppeteer-shell | 7.644× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-shell | 7.881× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 12.741× | 6 | 7 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 8.500× | 5 | 6 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c4`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.8.0\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 79 | 101 | 12.238 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 1120 | 1165 | 0.915 | 14.18× |
| cold | 1 | playwright-chrome | pass | yes | 1443 | 4507 | 0.531 | 18.27× |
| cold | 1 | puppeteer-shell | pass | yes | 1265 | 1324 | 0.837 | 16.01× |
| cold | 1 | puppeteer-chrome | pass | yes | 1451 | 1598 | 0.715 | 18.37× |
| cold-settled | 1 | playwright-shell | pass | yes | 169.911 | 174.368 | 5.951 | 12.31× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 164.7 | 188.802 | 5.905 | 11.94× |
| cold-settled | 1 | shotium | pass | yes | 13.798 | 15.253 | 70.761 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 176.408 | 244.504 | 5.357 | 12.79× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 194.889 | 215.761 | 5.076 | 14.12× |
| lifecycle | 1 | playwright-chrome | fail | no | 6734.335 | 6768.737 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 96.12 | 228.254 | 8.953 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2591.538 | 2844.561 | 0.383 | 26.96× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1224.239 | 1482.493 | 0.817 | 12.74× |
| lifecycle | 1 | playwright-shell | pass | yes | 972.613 | 1215.298 | 1.035 | 10.12× |
| warm | 1 | puppeteer-shell | noisy | no | 146.315 | 199.693 | 1.203 | N/A |
| warm | 1 | puppeteer-chrome | pass | yes | 175.448 | 240.318 | 5.511 | 16.03× |
| warm | 1 | playwright-chrome | noisy | no | 146.071 | 190.565 | 1.262 | N/A |
| warm | 1 | shotium | pass | yes | 10.942 | 12.847 | 88.636 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 152.361 | 199.551 | 6.346 | 13.92× |
| batch | 1 | puppeteer-chrome | pass | yes | 210.662 | 449.126 | 4.205 | 11.48× |
| batch | 1 | puppeteer-shell | pass | yes | 162.374 | 407.591 | 5.413 | 8.85× |
| batch | 1 | playwright-shell | pass | yes | 170.637 | 408.231 | 5.193 | 9.30× |
| batch | 1 | playwright-chrome | pass | yes | 158.724 | 391.675 | 5.331 | 8.65× |
| batch | 1 | shotium | pass | yes | 18.343 | 268.86 | 24.677 | 1.00× |
| parallel | 1 | shotium | pass | yes | 24.273 | 270.326 | 20.732 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 187.239 | 434.985 | 4.612 | 7.71× |
| parallel | 1 | playwright-shell | pass | yes | 198.901 | 431.414 | 4.469 | 8.19× |
| parallel | 1 | puppeteer-chrome | fail | no | 270.992 | 344.57 | 0.695 | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 211.758 | 464.086 | 4.312 | 8.72× |
| parallel | 2 | puppeteer-shell | pass | yes | 281.025 | 555.425 | 6.577 | 4.21× |
| parallel | 2 | playwright-shell | pass | yes | 257.942 | 548.356 | 6.929 | 3.87× |
| parallel | 2 | puppeteer-chrome | pass | yes | 439.176 | 731.59 | 4.284 | 6.59× |
| parallel | 2 | playwright-chrome | fail | no | 346.97 | 667.805 | 5.226 | N/A |
| parallel | 2 | shotium | pass | yes | 66.691 | 295.775 | 20.186 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 607.32 | 989.88 | 6.268 | 4.53× |
| parallel | 4 | puppeteer-chrome | pass | yes | 860.802 | 1398.644 | 4.589 | 6.43× |
| parallel | 4 | playwright-chrome | pass | yes | 608.085 | 1057.678 | 6.317 | 4.54× |
| parallel | 4 | shotium | pass | yes | 133.945 | 376.704 | 22.014 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 464.286 | 806.068 | 8.223 | 3.47× |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.356 | 122.145 | 9.707 | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 9071.151 | 9071.151 | N/A | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 116.871 | 143.68 | 8.250 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 103.965 | 119.861 | 9.472 | N/A |
| resident | 1 | shotium | pass | yes | 56 | 663 | 6.087 | 1.00× |
| resident | 1 | playwright-shell | noisy | no | 22462.035 | 22462.035 | N/A | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 574 | 1058 | 1.423 | 10.25× |
| resident | 1 | playwright-chrome | fail | no | 1055 | 1442 | 0.105 | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 32372.724 | 32372.724 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 29017.329 | 29017.329 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9499.871 | 9499.871 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 27223.16 | 27223.16 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 25339.385 | 25339.385 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 28982.167 | 28982.167 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 125.553 | 401.884 | 22.991 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 449.574 | 981.089 | 8.462 | 3.58× |
| soak | 4 | playwright-shell | pass | yes | 447.559 | 799.977 | 8.496 | 3.56× |
| soak | 4 | playwright-chrome | pass | yes | 591.506 | 2388.308 | 6.253 | 4.71× |
| soak | 4 | puppeteer-chrome | noisy | no | 9708.341 | 9708.341 | N/A | N/A |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.8.0\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 70 | 74 | 14.228 | N/A |
| cold-settled | 1 | shotium | noisy | no | 12.102 | 12.305 | 0.093 | N/A |
| lifecycle | 1 | shotium | pass | no | 92.752 | 163.587 | 10.406 | N/A |
| warm | 1 | shotium | pass | no | 11.903 | 12.411 | 80.613 | N/A |
| batch | 1 | shotium | pass | no | 19.401 | 274.31 | 23.431 | N/A |
| parallel | 1 | shotium | pass | no | 19.764 | 276.801 | 23.808 | N/A |
| parallel | 2 | shotium | pass | no | 49.214 | 288.825 | 24.935 | N/A |
| parallel | 4 | shotium | pass | no | 105.137 | 340.909 | 25.006 | N/A |
| resident | 1 | shotium | pass | no | 291 | 820 | 2.652 | N/A |
| faults | 1 | shotium | pass | no | 9554.108 | 9554.108 | N/A | N/A |
| soak | 4 | shotium | pass | no | 106.974 | 377.175 | 25.332 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 8.883× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 9.964× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 16.277× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 35.464× | 6 | 8 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.8.0/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | pass | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 86 | 98 | 11.647 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 1087 | 1461 | 0.891 | 12.64× |
| cold | 1 | playwright-chrome | pass | yes | 2741 | 2957 | 0.360 | 31.87× |
| cold | 1 | puppeteer-shell | pass | yes | 1109 | 1560 | 0.866 | 12.90× |
| cold | 1 | puppeteer-chrome | pass | yes | 2094 | 2438 | 0.472 | 24.35× |
| cold-settled | 1 | playwright-shell | pass | yes | 214.5 | 303.697 | 4.370 | 17.51× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 343.051 | 449.638 | 2.975 | 28.00× |
| cold-settled | 1 | shotium | pass | yes | 12.251 | 15.881 | 77.485 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 747.377 | 911.955 | 1.262 | 61.01× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 413.468 | 450.274 | 2.570 | 33.75× |
| lifecycle | 1 | playwright-chrome | fail | no | 4750.909 | 5183.255 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 107.671 | 202.467 | 8.672 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2548.493 | 3040.951 | 0.389 | 23.67× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1517.015 | 1796.172 | 0.659 | 14.09× |
| lifecycle | 1 | playwright-shell | pass | yes | 1129.835 | 1646.787 | 0.865 | 10.49× |
| warm | 1 | puppeteer-shell | pass | yes | 317.735 | 371.605 | 3.322 | 25.74× |
| warm | 1 | puppeteer-chrome | pass | yes | 380.945 | 514.783 | 2.531 | 30.86× |
| warm | 1 | playwright-chrome | pass | yes | 810.734 | 965.337 | 1.237 | 65.67× |
| warm | 1 | shotium | pass | yes | 12.345 | 14.239 | 77.169 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 287.537 | 377.355 | 3.443 | 23.29× |
| batch | 1 | puppeteer-chrome | pass | yes | 419.551 | 739.283 | 2.277 | 19.24× |
| batch | 1 | puppeteer-shell | pass | yes | 337.951 | 827.324 | 2.856 | 15.50× |
| batch | 1 | playwright-shell | pass | yes | 311.579 | 692.426 | 3.083 | 14.29× |
| batch | 1 | playwright-chrome | pass | yes | 862.642 | 1224.567 | 1.144 | 39.55× |
| batch | 1 | shotium | pass | yes | 21.809 | 260.996 | 23.208 | 1.00× |
| parallel | 1 | shotium | pass | yes | 26.285 | 372.671 | 17.266 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 349.732 | 653.098 | 2.728 | 13.31× |
| parallel | 1 | playwright-shell | pass | yes | 311.535 | 750.832 | 3.006 | 11.85× |
| parallel | 1 | puppeteer-chrome | pass | yes | 515.34 | 1248.605 | 1.891 | 19.61× |
| parallel | 1 | playwright-chrome | pass | yes | 871.552 | 1398.344 | 1.109 | 33.16× |
| parallel | 2 | puppeteer-shell | pass | yes | 409.524 | 661.99 | 4.745 | 7.62× |
| parallel | 2 | playwright-shell | pass | yes | 349.072 | 780.261 | 5.380 | 6.49× |
| parallel | 2 | puppeteer-chrome | pass | yes | 586.385 | 1952.53 | 3.051 | 10.90× |
| parallel | 2 | playwright-chrome | pass | yes | 1466.931 | 2862.215 | 1.376 | 27.28× |
| parallel | 2 | shotium | pass | yes | 53.775 | 294.229 | 23.694 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 607.703 | 1130.199 | 6.284 | 5.17× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1060.209 | 4783.01 | 3.332 | 9.03× |
| parallel | 4 | playwright-chrome | pass | yes | 2781.697 | 4739.973 | 1.474 | 23.69× |
| parallel | 4 | shotium | pass | yes | 117.438 | 360.926 | 23.213 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 584.512 | 904.627 | 6.822 | 4.98× |
| reuse-page | 1 | puppeteer-shell | pass | no | 238.388 | 395.086 | 3.983 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 249.968 | 363.164 | 3.870 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 225.041 | 249.523 | 4.838 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 227.25 | 244.058 | 4.487 | N/A |
| resident | 1 | shotium | pass | yes | 806 | 1261 | 1.160 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 1588 | 1826 | 0.617 | 1.97× |
| resident | 1 | puppeteer-shell | pass | yes | 1529 | 2140 | 0.595 | 1.90× |
| resident | 1 | playwright-chrome | noisy | no | 24247.364 | 24247.364 | N/A | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 2901 | 3678 | 0.371 | 3.60× |
| faults | 1 | puppeteer-chrome | pass | no | 22439.43 | 22439.43 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6475.441 | 6475.441 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 31152.023 | 31152.023 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 20385.985 | 20385.985 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 18515.098 | 18515.098 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 124.195 | 489.822 | 22.798 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 617.063 | 1263.625 | 6.326 | 4.97× |
| soak | 4 | playwright-shell | pass | yes | 627.113 | 1137.522 | 6.187 | 5.05× |
| soak | 4 | playwright-chrome | pass | yes | 2871.824 | 5604.821 | 1.393 | 23.12× |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 3 eligible cell(s), with 3 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 3 | 3 / 3 | 3 |
| 2 | playwright-shell | 7.483× | 3 | 3 / 3 | 0 |
| 3 | puppeteer-shell | 8.856× | 3 | 3 / 3 | 0 |
| 4 | puppeteer-chrome | 12.463× | 3 | 3 / 3 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.308× | 2 | 2 / 3 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `lifecycle/c1`, `parallel/c1`, `resident/c1`
- playwright-shell: `lifecycle/c1`, `parallel/c1`, `resident/c1`
- puppeteer-shell: `lifecycle/c1`, `parallel/c1`, `resident/c1`
- puppeteer-chrome: `lifecycle/c1`, `parallel/c1`, `resident/c1`
- playwright-chrome: `parallel/c1`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.8.0/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | noisy | no | 50.5 | 93 | 16.529 | N/A |
| cold | 1 | playwright-shell | noisy | no | 577.5 | 815 | 1.600 | N/A |
| cold | 1 | playwright-chrome | noisy | no | 1808 | 3007 | 0.523 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 569 | 1691 | 1.298 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 1436.5 | 3860 | 0.501 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 151.82 | 258.955 | 6.065 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 269.408 | 456.868 | 3.479 | N/A |
| cold-settled | 1 | shotium | noisy | no | 9.678 | 11.117 | 101.731 | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 240.491 | 253.841 | 4.493 | N/A |
| cold-settled | 1 | puppeteer-chrome | pass | no | 413.119 | 519.361 | 2.458 | N/A |
| lifecycle | 1 | playwright-chrome | fail | no | 2451.099 | 3286.892 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 51.129 | 72.689 | 19.524 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1542.894 | 1982.484 | 0.646 | 30.18× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 754.751 | 1104.354 | 1.293 | 14.76× |
| lifecycle | 1 | playwright-shell | pass | yes | 503.977 | 920.943 | 1.798 | 9.86× |
| warm | 1 | puppeteer-shell | noisy | no | 210.163 | 242.971 | 4.871 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 331.679 | 429.601 | 2.865 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 224.599 | 256.089 | 0.620 | N/A |
| warm | 1 | shotium | noisy | no | 10.348 | 12.145 | 95.988 | N/A |
| warm | 1 | playwright-shell | noisy | no | 172.31 | 209.389 | 5.766 | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | shotium | pass | yes | 11.882 | 280.998 | 29.776 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 224.906 | 550.758 | 4.182 | 18.93× |
| parallel | 1 | playwright-shell | pass | yes | 139.638 | 376.814 | 6.297 | 11.75× |
| parallel | 1 | puppeteer-chrome | pass | yes | 274.322 | 1447.55 | 3.103 | 23.09× |
| parallel | 1 | playwright-chrome | pass | yes | 172.111 | 1371.603 | 4.263 | 14.49× |
| parallel | 2 | puppeteer-shell | pass | no | 238.724 | 508.138 | 7.766 | N/A |
| parallel | 2 | playwright-shell | pass | no | 184.261 | 412.652 | 9.659 | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 276.13 | 379.946 | 3.325 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 261.986 | 488.514 | 3.700 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 137.125 | 171.145 | 7.192 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 113.079 | 176.785 | 8.649 | N/A |
| resident | 1 | shotium | pass | yes | 352 | 559 | 2.569 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 1273 | 1619 | 0.761 | 3.62× |
| resident | 1 | puppeteer-shell | pass | yes | 875 | 948 | 1.188 | 2.49× |
| resident | 1 | playwright-chrome | pass | yes | 967 | 1476 | 0.952 | 2.75× |
| resident | 1 | puppeteer-chrome | pass | yes | 978 | 1358 | 0.955 | 2.78× |
| faults | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 28361.24 | 28361.24 | N/A | N/A |
| faults | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10674.095 | 10674.095 | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | 8391.203 | 8391.203 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | no | 729.147 | 5433.487 | 4.794 | N/A |
| soak | 4 | puppeteer-chrome | pass | no | 816.267 | 3131.158 | 4.688 | N/A |

