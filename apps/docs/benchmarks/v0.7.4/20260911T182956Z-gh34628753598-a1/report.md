# Shotium 0.7.4 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `e9a684ec808512d5fe446c86949c66088362451b`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 2 | 9 |
| win32-arm64 | pass | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 10 |
| darwin-arm64 | noisy | shotium | 4 | 4 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 6.670× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 7.068× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 9.503× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 8.073× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.7.4/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 880 | 1066 | 1.107 | 14.43× |
| cold | 1 | playwright-chrome | pass | yes | 968 | 979 | 1.032 | 15.87× |
| cold | 1 | shotium | pass | yes | 61 | 63 | 16.548 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 780 | 849 | 1.268 | 12.79× |
| cold | 1 | puppeteer-shell | pass | yes | 615 | 623 | 1.630 | 10.08× |
| cold-settled | 1 | playwright-chrome | pass | yes | 175.855 | 211.148 | 5.455 | 11.30× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 136.429 | 141.954 | 7.418 | 8.76× |
| cold-settled | 1 | playwright-shell | pass | yes | 144.706 | 168.464 | 6.809 | 9.30× |
| cold-settled | 1 | shotium | pass | yes | 15.568 | 15.954 | 64.373 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 188.26 | 191.981 | 5.461 | 12.09× |
| lifecycle | 1 | shotium | pass | yes | 57.804 | 102.676 | 14.938 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 2148.589 | 2191.98 | N/A | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 915.814 | 977.287 | 1.088 | 15.84× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 632.748 | 695.984 | 1.575 | 10.95× |
| lifecycle | 1 | playwright-shell | pass | yes | 690.664 | 744.977 | 1.455 | 11.95× |
| warm | 1 | puppeteer-chrome | pass | yes | 190.145 | 215.52 | 5.167 | 13.97× |
| warm | 1 | puppeteer-shell | pass | yes | 133.272 | 143.352 | 7.495 | 9.79× |
| warm | 1 | shotium | pass | yes | 13.608 | 18.504 | 67.896 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 150.2 | 177.82 | 6.521 | 11.04× |
| warm | 1 | playwright-chrome | pass | yes | 176.694 | 240.265 | 5.543 | 12.98× |
| batch | 1 | shotium | pass | yes | 19.432 | 258.906 | 24.284 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 224.155 | 438.699 | 3.986 | 11.54× |
| batch | 1 | playwright-shell | pass | yes | 156.37 | 385.756 | 5.461 | 8.05× |
| batch | 1 | playwright-chrome | pass | yes | 189.833 | 416.947 | 4.619 | 9.77× |
| batch | 1 | puppeteer-shell | pass | yes | 153.497 | 388.08 | 5.591 | 7.90× |
| parallel | 1 | puppeteer-shell | pass | yes | 127.63 | 1246.422 | 6.032 | 8.46× |
| parallel | 1 | shotium | pass | yes | 15.093 | 256.772 | 27.588 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 178.761 | 558.406 | 4.857 | 11.84× |
| parallel | 1 | playwright-shell | pass | yes | 125.414 | 375.46 | 6.707 | 8.31× |
| parallel | 1 | playwright-chrome | pass | yes | 156.827 | 378.012 | 5.490 | 10.39× |
| parallel | 2 | shotium | pass | yes | 44.195 | 280.661 | 27.514 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 310.967 | 602.388 | 5.794 | 7.04× |
| parallel | 2 | playwright-shell | pass | yes | 176.226 | 424.278 | 9.487 | 3.99× |
| parallel | 2 | playwright-chrome | pass | yes | 239.178 | 561.45 | 7.394 | 5.41× |
| parallel | 2 | puppeteer-shell | pass | yes | 193.267 | 424.509 | 9.079 | 4.37× |
| parallel | 4 | puppeteer-chrome | pass | yes | 524.735 | 846.236 | 7.245 | 5.68× |
| parallel | 4 | playwright-shell | pass | yes | 337.259 | 562.344 | 10.975 | 3.65× |
| parallel | 4 | playwright-chrome | pass | yes | 393.707 | 685.733 | 9.325 | 4.26× |
| parallel | 4 | puppeteer-shell | pass | yes | 330.561 | 601.136 | 10.904 | 3.58× |
| parallel | 4 | shotium | pass | yes | 92.337 | 352.669 | 27.493 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 99.197 | 233.923 | 9.493 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.149 | 103.341 | 9.974 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 92.07 | 101.297 | 10.887 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 99.106 | 102.007 | 10.701 | N/A |
| resident | 1 | playwright-shell | pass | yes | 883 | 937 | 1.131 | 5.48× |
| resident | 1 | puppeteer-chrome | pass | yes | 800 | 908 | 1.276 | 4.97× |
| resident | 1 | puppeteer-shell | pass | yes | 773 | 847 | 1.280 | 4.80× |
| resident | 1 | playwright-chrome | pass | yes | 945 | 960 | 1.070 | 5.87× |
| resident | 1 | shotium | pass | yes | 161 | 352 | 4.933 | 1.00× |
| faults | 1 | playwright-shell | pass | no | 16640.665 | 16640.665 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 12924.737 | 12924.737 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 11662.094 | 11662.094 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5880.836 | 5880.836 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 19921.947 | 19921.947 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 440.93 | 928.838 | 8.768 | 3.67× |
| soak | 4 | playwright-chrome | pass | yes | 547.477 | 1480.301 | 6.967 | 4.55× |
| soak | 4 | playwright-shell | pass | yes | 447.143 | 866.734 | 8.257 | 3.72× |
| soak | 4 | puppeteer-chrome | pass | yes | 688.949 | 1175.693 | 5.621 | 5.73× |
| soak | 4 | shotium | pass | yes | 120.253 | 374.012 | 24.019 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 4.189× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 4.880× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.7.4/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 67 | 71 | 14.768 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 878 | 921 | 1.129 | 13.10× |
| cold | 1 | playwright-shell | pass | yes | 700 | 730 | 1.430 | 10.45× |
| cold-settled | 1 | playwright-chrome | pass | yes | 174.607 | 184.255 | 5.771 | 5.43× |
| cold-settled | 1 | playwright-shell | pass | yes | 142.28 | 159.064 | 6.861 | 4.43× |
| cold-settled | 1 | shotium | pass | yes | 32.151 | 33.299 | 30.805 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 1606.951 | 1725.449 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 74.009 | 102.108 | 12.607 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 586.784 | 668.928 | 1.661 | 7.93× |
| warm | 1 | shotium | pass | yes | 29.922 | 35.68 | 32.134 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 165.759 | 181.833 | 6.143 | 5.54× |
| warm | 1 | playwright-shell | pass | yes | 133.011 | 148.732 | 7.485 | 4.45× |
| batch | 1 | shotium | pass | yes | 35.025 | 262.493 | 18.734 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 178.219 | 402.644 | 4.953 | 5.09× |
| batch | 1 | playwright-shell | pass | yes | 145.445 | 373.431 | 5.890 | 4.15× |
| parallel | 1 | shotium | pass | yes | 34.38 | 263.047 | 18.967 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 178.711 | 437.424 | 4.976 | 5.20× |
| parallel | 1 | playwright-shell | pass | yes | 143.706 | 374.258 | 5.915 | 4.18× |
| parallel | 2 | playwright-chrome | pass | yes | 275.914 | 533.923 | 6.927 | 3.66× |
| parallel | 2 | playwright-shell | pass | yes | 194.565 | 428.209 | 8.907 | 2.58× |
| parallel | 2 | shotium | pass | yes | 75.416 | 304.179 | 18.785 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 321.757 | 608.855 | 11.218 | 2.04× |
| parallel | 4 | shotium | pass | yes | 157.344 | 400.884 | 18.962 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 420.354 | 657.589 | 8.895 | 2.67× |
| reuse-page | 1 | playwright-chrome | pass | no | 100.024 | 116.434 | 9.794 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 99.968 | 106.524 | 10.084 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 922 | 967 | 1.078 | 5.80× |
| resident | 1 | playwright-shell | pass | yes | 845 | 884 | 1.186 | 5.31× |
| resident | 1 | shotium | pass | yes | 159 | 248 | 6.200 | 1.00× |
| faults | 1 | playwright-shell | pass | no | 14683.124 | 14683.124 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5378.622 | 5378.622 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16194.012 | 16194.012 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 443.483 | 759.491 | 8.722 | 2.65× |
| soak | 4 | playwright-shell | pass | yes | 350.324 | 643.038 | 10.630 | 2.10× |
| soak | 4 | shotium | pass | yes | 167.087 | 422.727 | 18.390 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 6.167× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 8.788× | 6 | 8 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 5.271× | 5 | 7 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.290× | 6 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.7.4\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 630 | 668 | 1.577 | 14.32× |
| cold | 1 | playwright-chrome | pass | yes | 630 | 738 | 1.552 | 14.32× |
| cold | 1 | shotium | pass | yes | 44 | 46 | 22.876 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 468 | 517 | 2.117 | 10.64× |
| cold | 1 | puppeteer-shell | pass | yes | 509 | 548 | 1.942 | 11.57× |
| cold-settled | 1 | playwright-chrome | pass | yes | 111.103 | 115.985 | 8.963 | 12.83× |
| cold-settled | 1 | puppeteer-shell | noisy | no | 132.506 | 139.934 | 0.556 | N/A |
| cold-settled | 1 | playwright-shell | pass | yes | 105.314 | 109.851 | 9.536 | 12.16× |
| cold-settled | 1 | shotium | pass | yes | 8.663 | 8.952 | 113.102 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 157.348 | 166.136 | 0.579 | N/A |
| lifecycle | 1 | shotium | pass | yes | 74.328 | 506.083 | 9.975 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 6609.027 | 7624.113 | N/A | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2961.959 | 6412.2 | 0.309 | 39.85× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1015.699 | 9381.362 | 0.679 | 13.67× |
| lifecycle | 1 | playwright-shell | pass | yes | 951.973 | 1781.582 | 0.977 | 12.81× |
| warm | 1 | puppeteer-chrome | pass | no | 265.466 | 335.458 | 3.823 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 165.312 | 190.112 | 1.296 | N/A |
| warm | 1 | shotium | noisy | no | 15.459 | 17.529 | 1.853 | N/A |
| warm | 1 | playwright-shell | pass | no | 170.85 | 191.017 | 5.788 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 190.343 | 225.358 | 0.495 | N/A |
| batch | 1 | shotium | pass | yes | 24.501 | 271.444 | 21.244 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 251.605 | 489.872 | 3.612 | 10.27× |
| batch | 1 | playwright-shell | pass | yes | 198.258 | 428.864 | 4.514 | 8.09× |
| batch | 1 | playwright-chrome | pass | yes | 198.272 | 455.344 | 4.414 | 8.09× |
| batch | 1 | puppeteer-shell | fail | no | 186.295 | 428.111 | 2.561 | N/A |
| parallel | 1 | puppeteer-shell | pass | yes | 165.727 | 417.133 | 5.049 | 8.48× |
| parallel | 1 | shotium | pass | yes | 19.547 | 276.617 | 24.975 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 227.963 | 455.109 | 4.040 | 11.66× |
| parallel | 1 | playwright-shell | pass | yes | 181.741 | 424.216 | 4.882 | 9.30× |
| parallel | 1 | playwright-chrome | noisy | no | 10973.676 | 10973.676 | N/A | N/A |
| parallel | 2 | shotium | pass | yes | 49.489 | 289.673 | 25.388 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 378.131 | 646.063 | 4.840 | 7.64× |
| parallel | 2 | playwright-shell | pass | yes | 220 | 473.929 | 7.904 | 4.45× |
| parallel | 2 | playwright-chrome | pass | yes | 265.904 | 574.265 | 6.791 | 5.37× |
| parallel | 2 | puppeteer-shell | pass | yes | 231.625 | 480.779 | 7.553 | 4.68× |
| parallel | 4 | puppeteer-chrome | pass | yes | 686.569 | 1137.456 | 5.633 | 5.81× |
| parallel | 4 | playwright-shell | pass | yes | 378.015 | 654.435 | 9.691 | 3.20× |
| parallel | 4 | playwright-chrome | pass | yes | 483.128 | 1266.052 | 7.685 | 4.09× |
| parallel | 4 | puppeteer-shell | pass | yes | 404.721 | 732.694 | 9.397 | 3.43× |
| parallel | 4 | shotium | pass | yes | 118.091 | 362.141 | 24.078 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 115.658 | 186.823 | 8.443 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 105.718 | 134.458 | 8.983 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 101.991 | 129.631 | 9.484 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 102.156 | 131.787 | 9.405 | N/A |
| resident | 1 | playwright-shell | pass | yes | 803 | 1354 | 1.092 | 2.25× |
| resident | 1 | puppeteer-chrome | pass | yes | 705 | 1074 | 1.260 | 1.97× |
| resident | 1 | puppeteer-shell | pass | yes | 654 | 1166 | 1.239 | 1.83× |
| resident | 1 | playwright-chrome | pass | yes | 1014 | 1647 | 0.875 | 2.84× |
| resident | 1 | shotium | pass | yes | 357 | 1189 | 2.225 | 1.00× |
| faults | 1 | playwright-shell | pass | no | 25982.717 | 25982.717 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 30412.118 | 30412.118 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 26962.012 | 26962.012 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10911.146 | 10911.146 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 29958.533 | 29958.533 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 425.11 | 813 | 9.036 | 2.87× |
| soak | 4 | playwright-chrome | pass | yes | 622.353 | 3155.919 | 5.773 | 4.20× |
| soak | 4 | playwright-shell | pass | yes | 479.481 | 1063.486 | 7.904 | 3.24× |
| soak | 4 | puppeteer-chrome | pass | yes | 879.284 | 1584.855 | 4.460 | 5.94× |
| soak | 4 | shotium | pass | yes | 148.151 | 419.322 | 21.188 | 1.00× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.7.4\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 67 | 75 | 14.737 | N/A |
| cold-settled | 1 | shotium | pass | no | 12.094 | 12.435 | 80.721 | N/A |
| lifecycle | 1 | shotium | pass | no | 94.18 | 121.209 | 10.337 | N/A |
| warm | 1 | shotium | pass | no | 12.514 | 13.749 | 75.907 | N/A |
| batch | 1 | shotium | pass | no | 21.98 | 272.608 | 22.645 | N/A |
| parallel | 1 | shotium | pass | no | 20.727 | 273.582 | 23.667 | N/A |
| parallel | 2 | shotium | pass | no | 57.413 | 311.017 | 21.431 | N/A |
| parallel | 4 | shotium | pass | no | 116.639 | 354.496 | 24.024 | N/A |
| resident | 1 | shotium | pass | no | 394 | 898 | 2.145 | N/A |
| faults | 1 | shotium | pass | no | 11838.2 | 11838.2 | N/A | N/A |
| soak | 4 | shotium | pass | no | 112.175 | 370.017 | 24.090 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 9.014× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-chrome | 14.238× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-shell | 10.288× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 20.920× | 5 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.7.4/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | pass | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 1953 | 2346 | 0.492 | 22.98× |
| cold | 1 | playwright-chrome | noisy | no | 2685 | 3396 | 0.365 | N/A |
| cold | 1 | shotium | pass | yes | 85 | 684 | 5.499 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 1105 | 1766 | 0.817 | 13.00× |
| cold | 1 | puppeteer-shell | pass | yes | 1055 | 1713 | 0.879 | 12.41× |
| cold-settled | 1 | playwright-chrome | pass | yes | 754.362 | 860.646 | 1.307 | 54.31× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 358.537 | 388.089 | 2.958 | 25.81× |
| cold-settled | 1 | playwright-shell | pass | yes | 314.799 | 404.366 | 3.164 | 22.66× |
| cold-settled | 1 | shotium | pass | yes | 13.89 | 24.253 | 66.354 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 392.407 | 467.927 | 2.492 | 28.25× |
| lifecycle | 1 | shotium | pass | yes | 103.8 | 170.789 | 8.813 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 4725.056 | 5123.689 | N/A | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2509.946 | 3090.24 | 0.394 | 24.18× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1480.008 | 1967.927 | 0.649 | 14.26× |
| lifecycle | 1 | playwright-shell | pass | yes | 1214.269 | 1722.043 | 0.825 | 11.70× |
| warm | 1 | puppeteer-chrome | pass | yes | 404.433 | 1201.594 | 2.113 | 32.33× |
| warm | 1 | puppeteer-shell | pass | yes | 347.142 | 467.675 | 2.882 | 27.75× |
| warm | 1 | shotium | pass | yes | 12.508 | 17.517 | 72.696 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 274.746 | 423.544 | 3.506 | 21.97× |
| warm | 1 | playwright-chrome | pass | yes | 815.433 | 973.886 | 1.249 | 65.19× |
| batch | 1 | shotium | pass | yes | 23.366 | 278.252 | 21.801 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 447.687 | 1778.428 | 1.928 | 19.16× |
| batch | 1 | playwright-shell | pass | yes | 318.982 | 622.265 | 3.045 | 13.65× |
| batch | 1 | playwright-chrome | pass | yes | 871.592 | 1301.017 | 1.128 | 37.30× |
| batch | 1 | puppeteer-shell | pass | yes | 344.51 | 639.88 | 2.827 | 14.74× |
| parallel | 1 | puppeteer-shell | pass | yes | 453.988 | 2267.564 | 1.908 | 10.53× |
| parallel | 1 | shotium | pass | yes | 43.109 | 271.931 | 14.287 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 654.885 | 1895.709 | 1.419 | 15.19× |
| parallel | 1 | playwright-shell | pass | yes | 470.502 | 1473.794 | 1.921 | 10.91× |
| parallel | 1 | playwright-chrome | pass | yes | 961.23 | 1499.668 | 1.010 | 22.30× |
| parallel | 2 | shotium | pass | yes | 92.352 | 319.476 | 16.704 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 1022.174 | 2485.079 | 1.871 | 11.07× |
| parallel | 2 | playwright-shell | pass | yes | 483.943 | 830.593 | 3.921 | 5.24× |
| parallel | 2 | playwright-chrome | pass | yes | 1568.557 | 3012.427 | 1.259 | 16.98× |
| parallel | 2 | puppeteer-shell | pass | yes | 533.055 | 806.466 | 3.714 | 5.77× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1725.314 | 5560.9 | 2.088 | 8.02× |
| parallel | 4 | playwright-shell | pass | yes | 840.219 | 1435.712 | 4.553 | 3.90× |
| parallel | 4 | playwright-chrome | pass | yes | 3248.655 | 5525.984 | 1.257 | 15.10× |
| parallel | 4 | puppeteer-shell | pass | yes | 831.737 | 1393.363 | 4.616 | 3.87× |
| parallel | 4 | shotium | pass | yes | 215.171 | 421.922 | 16.183 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 244.621 | 965.413 | 3.588 | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 11980.777 | 11980.777 | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 261.009 | 583.24 | 3.289 | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-shell | noisy | no | 18267.771 | 18267.771 | N/A | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 2296 | 2430 | 0.478 | 2.08× |
| resident | 1 | puppeteer-shell | pass | yes | 1525 | 1854 | 0.646 | 1.38× |
| resident | 1 | playwright-chrome | pass | yes | 2564 | 2814 | 0.401 | 2.32× |
| resident | 1 | shotium | pass | yes | 1104 | 1214 | 0.905 | 1.00× |
| faults | 1 | playwright-shell | pass | no | 30552.253 | 30552.253 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 42534.861 | 42534.861 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 20097.273 | 20097.273 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6900.108 | 6900.108 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 66108.199 | 66108.199 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 818.95 | 1937.518 | 4.646 | 5.84× |
| soak | 4 | playwright-chrome | fail | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 784.591 | 6043.802 | 4.617 | 5.59× |
| soak | 4 | puppeteer-chrome | pass | yes | 1761.268 | 6627.295 | 2.133 | 12.56× |
| soak | 4 | shotium | pass | yes | 140.235 | 433.378 | 21.255 | 1.00× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 4 eligible cell(s), with 4 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 4 | 4 / 4 | 4 |
| 2 | playwright-shell | 10.000× | 4 | 4 / 4 | 0 |
| 3 | puppeteer-shell | 11.538× | 4 | 4 / 4 | 0 |
| 4 | puppeteer-chrome | 21.850× | 4 | 4 / 4 | 0 |
| not ranked (partial coverage) | playwright-chrome | 12.795× | 2 | 2 / 4 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `soak/c4`
- playwright-shell: `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `soak/c4`
- puppeteer-shell: `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `soak/c4`
- puppeteer-chrome: `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `soak/c4`
- playwright-chrome: `cold-settled/c1`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.7.4/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | noisy | no | 3450 | 4705 | 0.279 | N/A |
| cold | 1 | playwright-chrome | noisy | no | 2882 | 4061 | 0.359 | N/A |
| cold | 1 | shotium | noisy | no | 81 | 142 | 11.142 | N/A |
| cold | 1 | playwright-shell | noisy | no | 1052 | 1237 | 0.925 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 1157 | 1314 | 0.864 | N/A |
| cold-settled | 1 | playwright-chrome | pass | yes | 237.555 | 339.052 | 4.015 | 22.49× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 225.078 | 262.372 | 4.555 | 21.31× |
| cold-settled | 1 | playwright-shell | pass | yes | 175.619 | 196.019 | 5.723 | 16.63× |
| cold-settled | 1 | shotium | pass | yes | 10.561 | 17.902 | 83.520 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 331.403 | 375.998 | 3.016 | 31.38× |
| lifecycle | 1 | shotium | pass | yes | 65.98 | 96.579 | 14.143 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 3518.412 | 3999.267 | N/A | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1920.82 | 2671.435 | 0.524 | 29.11× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 832.214 | 1246.043 | 1.120 | 12.61× |
| lifecycle | 1 | playwright-shell | pass | yes | 644.865 | 1119.27 | 1.422 | 9.77× |
| warm | 1 | puppeteer-chrome | noisy | no | 311.831 | 383.511 | 0.643 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 278.897 | 303.042 | 0.419 | N/A |
| warm | 1 | shotium | noisy | no | 9.785 | 11.666 | 100.037 | N/A |
| warm | 1 | playwright-shell | noisy | no | 161.735 | 214.913 | 6.105 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 185.483 | 284.153 | 5.243 | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-shell | pass | yes | 237.623 | 466.29 | 4.013 | 13.51× |
| parallel | 1 | shotium | pass | yes | 17.588 | 261.593 | 26.368 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 376.028 | 1925.799 | 2.281 | 21.38× |
| parallel | 1 | playwright-shell | pass | yes | 223.473 | 495.152 | 4.095 | 12.71× |
| parallel | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 124.063 | 267.656 | 7.600 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 183.746 | 241.807 | 5.489 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 149.017 | 226.053 | 6.508 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 104.672 | 133.831 | 9.560 | N/A |
| resident | 1 | playwright-shell | pass | no | 1029 | 1214 | 0.932 | N/A |
| resident | 1 | puppeteer-chrome | pass | no | 1350 | 1709 | 0.764 | N/A |
| resident | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 18578.08 | 18578.08 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 28557.65 | 28557.65 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 12553.254 | 12553.254 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6040.397 | 6040.397 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 27479.882 | 27479.882 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 479.013 | 1016.258 | 8.062 | 4.88× |
| soak | 4 | playwright-chrome | pass | yes | 714.372 | 8741.678 | 3.949 | 7.28× |
| soak | 4 | playwright-shell | pass | yes | 475.245 | 909.247 | 8.175 | 4.84× |
| soak | 4 | puppeteer-chrome | pass | yes | 1145.563 | 4178.589 | 3.393 | 11.67× |
| soak | 4 | shotium | pass | yes | 98.157 | 387.761 | 25.836 | 1.00× |

