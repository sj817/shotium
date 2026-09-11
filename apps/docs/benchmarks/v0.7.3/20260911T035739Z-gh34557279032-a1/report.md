# Shotium 0.7.3 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `391bb42b2072e117ca8e665b3e955fecf16a6f75`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 5 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 3 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 8 |
| darwin-arm64 | noisy | shotium | 2 | 3 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 6.197× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 6.246× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 7.801× | 8 | 10 / 10 | 0 |
| 5 | puppeteer-chrome | 9.133× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.7.3/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 934 | 1073 | 1.061 | 16.68× |
| cold | 1 | shotium | pass | yes | 56 | 58 | 17.995 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 698 | 762 | 1.417 | 12.46× |
| cold | 1 | puppeteer-chrome | pass | yes | 813 | 911 | 1.213 | 14.52× |
| cold | 1 | puppeteer-shell | pass | yes | 567 | 579 | 1.795 | 10.13× |
| cold-settled | 1 | shotium | pass | yes | 13.055 | 14.04 | 75.407 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 112.063 | 132.003 | 8.518 | 8.58× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 128.174 | 143.002 | 7.708 | 9.82× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 181.453 | 196.237 | 5.460 | 13.90× |
| cold-settled | 1 | playwright-chrome | pass | yes | 160.801 | 201.202 | 6.263 | 12.32× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 599.898 | 813.25 | 1.643 | 11.33× |
| lifecycle | 1 | shotium | pass | yes | 52.962 | 89.016 | 16.630 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 966.436 | 1382.155 | 1.020 | 18.25× |
| lifecycle | 1 | playwright-shell | pass | yes | 621.333 | 765.179 | 1.586 | 11.73× |
| lifecycle | 1 | playwright-chrome | pass | yes | 865.748 | 1076.148 | 1.121 | 16.35× |
| warm | 1 | shotium | pass | yes | 13.655 | 22.831 | 64.807 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 133.112 | 150.117 | 7.442 | 9.75× |
| warm | 1 | playwright-chrome | pass | yes | 153.002 | 171.083 | 6.470 | 11.20× |
| warm | 1 | playwright-shell | pass | yes | 128.806 | 138.326 | 7.842 | 9.43× |
| warm | 1 | puppeteer-chrome | pass | yes | 183.109 | 200.891 | 5.414 | 13.41× |
| batch | 1 | puppeteer-chrome | pass | yes | 225.291 | 444.419 | 3.995 | 11.56× |
| batch | 1 | playwright-shell | pass | yes | 147.054 | 374.446 | 5.818 | 7.55× |
| batch | 1 | puppeteer-shell | pass | yes | 151.397 | 372.465 | 5.548 | 7.77× |
| batch | 1 | playwright-chrome | pass | yes | 172.282 | 402.569 | 5.019 | 8.84× |
| batch | 1 | shotium | pass | yes | 19.486 | 265.322 | 24.496 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 177.797 | 526.987 | 4.913 | 9.12× |
| parallel | 1 | shotium | pass | yes | 19.487 | 258.327 | 24.102 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 151.234 | 389.237 | 5.649 | 7.76× |
| parallel | 1 | puppeteer-chrome | pass | yes | 223.629 | 439.203 | 4.085 | 11.48× |
| parallel | 1 | playwright-shell | pass | yes | 144.49 | 372.527 | 5.915 | 7.41× |
| parallel | 2 | shotium | pass | yes | 43.213 | 282.373 | 24.230 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 236.161 | 508.946 | 7.520 | 5.47× |
| parallel | 2 | puppeteer-chrome | pass | yes | 378.694 | 606.482 | 5.028 | 8.76× |
| parallel | 2 | playwright-shell | pass | yes | 217.766 | 457.672 | 7.977 | 5.04× |
| parallel | 2 | playwright-chrome | pass | yes | 297.923 | 598.041 | 6.241 | 6.89× |
| parallel | 4 | puppeteer-shell | pass | yes | 433.839 | 713.402 | 8.713 | 3.71× |
| parallel | 4 | puppeteer-chrome | pass | yes | 676.517 | 1078.54 | 5.715 | 5.78× |
| parallel | 4 | playwright-shell | pass | yes | 411.634 | 655.28 | 9.336 | 3.52× |
| parallel | 4 | playwright-chrome | pass | yes | 491.929 | 790.937 | 7.828 | 4.20× |
| parallel | 4 | shotium | pass | yes | 117.011 | 395.569 | 23.831 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 83.222 | 85.257 | 12.113 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 83.389 | 101.272 | 11.824 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.946 | 107.318 | 9.947 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.782 | 101.848 | 10.186 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 1044 | 1088 | 0.963 | 2.31× |
| resident | 1 | shotium | pass | yes | 451 | 473 | 2.227 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 963 | 993 | 1.036 | 2.14× |
| resident | 1 | puppeteer-shell | pass | yes | 830 | 933 | 1.434 | 1.84× |
| resident | 1 | puppeteer-chrome | pass | yes | 947 | 966 | 1.070 | 2.10× |
| faults | 1 | playwright-chrome | pass | no | 18652.704 | 18652.704 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16245.844 | 16245.844 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 12489.426 | 12489.426 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5890.748 | 5890.748 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10823.574 | 10823.574 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 118.26 | 368.147 | 24.092 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 393.961 | 845.309 | 9.350 | 3.33× |
| soak | 4 | puppeteer-chrome | pass | yes | 685.224 | 1334.46 | 5.702 | 5.79× |
| soak | 4 | playwright-chrome | pass | yes | 484.803 | 883.935 | 7.870 | 4.10× |
| soak | 4 | puppeteer-shell | pass | yes | 432.9 | 836.183 | 8.865 | 3.66× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.649× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.582× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.7.3/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 618 | 635 | 1.620 | 10.30× |
| cold | 1 | playwright-chrome | pass | yes | 785 | 914 | 1.248 | 13.08× |
| cold | 1 | shotium | pass | yes | 60 | 61 | 16.706 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 121.107 | 129.61 | 8.221 | 4.04× |
| cold-settled | 1 | shotium | pass | yes | 30.013 | 34.321 | 32.130 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 147.714 | 152.722 | 7.037 | 4.92× |
| lifecycle | 1 | playwright-shell | pass | yes | 546.371 | 584.857 | 1.819 | 8.00× |
| lifecycle | 1 | shotium | pass | yes | 68.261 | 85.907 | 14.023 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 711.782 | 759.335 | 1.404 | 10.43× |
| warm | 1 | shotium | pass | yes | 31.458 | 36.834 | 31.439 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 117.138 | 132.682 | 8.209 | 3.72× |
| warm | 1 | playwright-chrome | pass | yes | 150.344 | 188.387 | 6.504 | 4.78× |
| batch | 1 | shotium | pass | yes | 34.686 | 260.077 | 18.649 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 128.726 | 358.56 | 6.425 | 3.71× |
| batch | 1 | playwright-chrome | pass | yes | 159.828 | 372.739 | 5.491 | 4.61× |
| parallel | 1 | playwright-shell | pass | yes | 135.239 | 357.49 | 6.279 | 3.69× |
| parallel | 1 | shotium | pass | yes | 36.61 | 259.592 | 18.391 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 169.487 | 402.646 | 5.321 | 4.63× |
| parallel | 2 | shotium | pass | yes | 75.885 | 314.021 | 18.436 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 245.759 | 503.617 | 7.440 | 3.24× |
| parallel | 2 | playwright-shell | pass | yes | 187.78 | 424.76 | 9.204 | 2.47× |
| parallel | 4 | playwright-chrome | pass | yes | 415.595 | 723.456 | 9.023 | 2.48× |
| parallel | 4 | playwright-shell | pass | yes | 327.097 | 585.923 | 11.446 | 1.95× |
| parallel | 4 | shotium | pass | yes | 167.545 | 400.571 | 18.301 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.244 | 129.026 | 11.542 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.31 | 92.267 | 12.078 | N/A |
| resident | 1 | playwright-shell | pass | yes | 827 | 1482 | 1.079 | 2.75× |
| resident | 1 | shotium | pass | yes | 301 | 408 | 3.926 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 882 | 970 | 1.174 | 2.93× |
| faults | 1 | playwright-chrome | pass | no | 14592.262 | 14592.262 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5140.315 | 5140.315 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14509.93 | 14509.93 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 167.061 | 435.521 | 18.372 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 309.956 | 622.258 | 11.805 | 1.86× |
| soak | 4 | playwright-chrome | pass | yes | 423.322 | 753.734 | 9.084 | 2.53× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | puppeteer-shell | 7.912× | 7 | 9 / 9 | 0 |
| 3 | playwright-chrome | 9.150× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 12.811× | 7 | 8 / 9 | 0 |
| not ranked (partial coverage) | playwright-shell | 7.231× | 6 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.7.3\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 1002 | 1034 | 0.998 | 14.96× |
| cold | 1 | shotium | pass | yes | 67 | 70 | 14.768 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 816 | 1052 | 1.178 | 12.18× |
| cold | 1 | puppeteer-chrome | pass | yes | 1070 | 1209 | 0.909 | 15.97× |
| cold | 1 | puppeteer-shell | pass | yes | 945 | 1267 | 1.019 | 14.10× |
| cold-settled | 1 | shotium | pass | yes | 13.154 | 13.425 | 75.036 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 150.614 | 172.672 | 6.392 | 11.45× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 165.925 | 180.194 | 6.057 | 12.61× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 200.585 | 256.493 | 4.844 | 15.25× |
| cold-settled | 1 | playwright-chrome | pass | yes | 154.587 | 207.816 | 6.257 | 11.75× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1587.865 | 5700.24 | 0.490 | 12.77× |
| lifecycle | 1 | shotium | pass | yes | 124.304 | 212.154 | 7.681 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3412.663 | 8265.565 | 0.265 | 27.45× |
| lifecycle | 1 | playwright-shell | pass | yes | 1124.962 | 7097.756 | 0.653 | 9.05× |
| lifecycle | 1 | playwright-chrome | pass | yes | 2341.601 | 9341.957 | 0.342 | 18.84× |
| warm | 1 | shotium | pass | yes | 9.395 | 10.111 | 103.332 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 133.201 | 182.121 | 7.133 | 14.18× |
| warm | 1 | playwright-chrome | pass | yes | 139.991 | 214.34 | 6.923 | 14.90× |
| warm | 1 | playwright-shell | noisy | no | 117.223 | 144.85 | 1.473 | N/A |
| warm | 1 | puppeteer-chrome | pass | yes | 197.858 | 247.942 | 5.056 | 21.06× |
| batch | 1 | puppeteer-chrome | pass | no | 212.818 | 461.487 | 4.354 | N/A |
| batch | 1 | playwright-shell | pass | no | 129.141 | 361.029 | 6.319 | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 9678.89 | 9678.89 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | no | 154.445 | 389.104 | 5.714 | N/A |
| batch | 1 | shotium | noisy | no | 7987.747 | 7987.747 | N/A | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 181.815 | 462.913 | 4.830 | 7.68× |
| parallel | 1 | shotium | pass | yes | 23.659 | 276.363 | 22.408 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 181.103 | 422.799 | 4.768 | 7.65× |
| parallel | 1 | puppeteer-chrome | pass | yes | 248.55 | 476.496 | 3.610 | 10.51× |
| parallel | 1 | playwright-shell | pass | yes | 182.247 | 429.67 | 4.818 | 7.70× |
| parallel | 2 | shotium | pass | yes | 64.136 | 293.591 | 22.010 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 261.741 | 502.992 | 6.961 | 4.08× |
| parallel | 2 | puppeteer-chrome | noisy | no | 10259.082 | 10259.082 | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | 12572.253 | 12572.253 | N/A | N/A |
| parallel | 2 | playwright-chrome | pass | yes | 302.99 | 595.62 | 6.048 | 4.72× |
| parallel | 4 | puppeteer-shell | pass | yes | 426.078 | 826.27 | 8.785 | 3.34× |
| parallel | 4 | puppeteer-chrome | pass | yes | 786.009 | 1267.269 | 4.870 | 6.15× |
| parallel | 4 | playwright-shell | pass | yes | 425.783 | 741.71 | 8.872 | 3.33× |
| parallel | 4 | playwright-chrome | pass | yes | 528.951 | 1278.646 | 6.689 | 4.14× |
| parallel | 4 | shotium | pass | yes | 127.732 | 364.813 | 22.379 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 75.17 | 94.401 | 13.151 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 83.504 | 113.426 | 11.314 | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 8825.124 | 8825.124 | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 83.586 | 125.539 | 11.189 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 699 | 1218 | 1.312 | 13.71× |
| resident | 1 | shotium | pass | yes | 51 | 278 | 8.739 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 493 | 1070 | 1.514 | 9.67× |
| resident | 1 | puppeteer-shell | pass | yes | 539 | 766 | 1.805 | 10.57× |
| resident | 1 | puppeteer-chrome | pass | yes | 620 | 884 | 1.566 | 12.16× |
| faults | 1 | playwright-chrome | pass | no | 29796.282 | 29796.282 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 25444.848 | 25444.848 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 29608.461 | 29608.461 | N/A | N/A |
| faults | 1 | shotium | pass | no | 11707.044 | 11707.044 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 25476.836 | 25476.836 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 127.416 | 1401.59 | 22.458 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 420.539 | 796.372 | 9.098 | 3.30× |
| soak | 4 | puppeteer-chrome | pass | yes | 835.466 | 1470.729 | 4.713 | 6.56× |
| soak | 4 | playwright-chrome | pass | yes | 563.51 | 1895.43 | 6.809 | 4.42× |
| soak | 4 | puppeteer-shell | pass | yes | 436.09 | 889.16 | 8.796 | 3.42× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.7.3\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 68 | 95 | 13.889 | N/A |
| cold-settled | 1 | shotium | noisy | no | 12.059 | 12.196 | 0.046 | N/A |
| lifecycle | 1 | shotium | pass | no | 95.536 | 130.803 | 10.159 | N/A |
| warm | 1 | shotium | noisy | no | 11.679 | 13.509 | 1.879 | N/A |
| batch | 1 | shotium | noisy | no | 10293.847 | 10293.847 | N/A | N/A |
| parallel | 1 | shotium | pass | no | 20.475 | 272.543 | 24.231 | N/A |
| parallel | 2 | shotium | pass | no | 52.272 | 290.18 | 23.831 | N/A |
| parallel | 4 | shotium | pass | no | 119.948 | 356.185 | 23.537 | N/A |
| resident | 1 | shotium | pass | no | 282 | 960 | 2.816 | N/A |
| faults | 1 | shotium | pass | no | 9564.701 | 9564.701 | N/A | N/A |
| soak | 4 | shotium | pass | no | 103.459 | 1371.349 | 24.159 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 8 / 8 | 8 |
| 2 | playwright-shell | 7.420× | 6 | 8 / 8 | 0 |
| 3 | puppeteer-chrome | 13.248× | 6 | 8 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 8.982× | 5 | 7 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 22.853× | 5 | 5 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.7.3/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 3403 | 13524 | 0.207 | 35.45× |
| cold | 1 | shotium | pass | yes | 96 | 329 | 7.786 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 1116 | 1870 | 0.819 | 11.63× |
| cold | 1 | puppeteer-chrome | pass | yes | 2395 | 2976 | 0.424 | 24.95× |
| cold | 1 | puppeteer-shell | pass | yes | 1209 | 1587 | 0.792 | 12.59× |
| cold-settled | 1 | shotium | noisy | no | 20.489 | 22.191 | 54.633 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 270.529 | 378.26 | 3.499 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | no | 299.843 | 359.563 | 3.332 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 459.927 | 771.494 | 1.964 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 866.805 | 1026.008 | 1.118 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1483.332 | 2017.892 | 0.653 | 13.78× |
| lifecycle | 1 | shotium | pass | yes | 107.647 | 183.976 | 8.713 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2659.609 | 3439.112 | 0.370 | 24.71× |
| lifecycle | 1 | playwright-shell | pass | yes | 1205.753 | 1603.919 | 0.814 | 11.20× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3076.952 | 3750.783 | 0.319 | 28.58× |
| warm | 1 | shotium | pass | yes | 13.227 | 15.262 | 71.942 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 338.157 | 462.657 | 3.041 | 25.57× |
| warm | 1 | playwright-chrome | pass | yes | 777.921 | 941.111 | 1.249 | 58.81× |
| warm | 1 | playwright-shell | pass | yes | 285.963 | 435.767 | 3.347 | 21.62× |
| warm | 1 | puppeteer-chrome | pass | yes | 422.53 | 843.113 | 2.334 | 31.94× |
| batch | 1 | puppeteer-chrome | pass | no | 472.306 | 812.623 | 1.978 | N/A |
| batch | 1 | playwright-shell | pass | no | 351.462 | 765.407 | 2.759 | N/A |
| batch | 1 | puppeteer-shell | pass | no | 396.299 | 1123.279 | 2.346 | N/A |
| batch | 1 | playwright-chrome | pass | no | 933.299 | 1554.138 | 1.043 | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 833.652 | 1248.578 | 1.151 | 30.17× |
| parallel | 1 | shotium | pass | yes | 27.633 | 263.602 | 20.362 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 377.969 | 736.75 | 2.385 | 13.68× |
| parallel | 1 | puppeteer-chrome | pass | yes | 500.514 | 1402.037 | 1.751 | 18.11× |
| parallel | 1 | playwright-shell | pass | yes | 331.653 | 805.811 | 2.638 | 12.00× |
| parallel | 2 | shotium | pass | yes | 68.416 | 384.439 | 18.830 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 485.109 | 1269.864 | 3.807 | 7.09× |
| parallel | 2 | puppeteer-chrome | pass | yes | 799.247 | 1934.784 | 2.222 | 11.68× |
| parallel | 2 | playwright-shell | pass | yes | 332.868 | 880.058 | 5.404 | 4.87× |
| parallel | 2 | playwright-chrome | fail | no | 1488.51 | 3400.568 | 1.289 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 680.121 | 1231.213 | 5.478 | 5.52× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1554.527 | 5273.388 | 2.316 | 12.62× |
| parallel | 4 | playwright-shell | pass | yes | 675.728 | 1116.615 | 5.593 | 5.49× |
| parallel | 4 | playwright-chrome | fail | no | 2830.187 | 4553.565 | 1.434 | N/A |
| parallel | 4 | shotium | pass | yes | 123.167 | 355.562 | 22.727 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 232.508 | 425.887 | 4.493 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 238.966 | 389.783 | 4.310 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 241.308 | 358.758 | 3.725 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 251.767 | 340.852 | 4.061 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 2878 | 3299 | 0.339 | 3.47× |
| resident | 1 | shotium | pass | yes | 830 | 1670 | 1.073 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 1604 | 2032 | 0.583 | 1.93× |
| resident | 1 | puppeteer-shell | pass | yes | 1647 | 2131 | 0.593 | 1.98× |
| resident | 1 | puppeteer-chrome | pass | yes | 1649 | 2349 | 0.566 | 1.99× |
| faults | 1 | playwright-chrome | pass | no | 39785.572 | 39785.572 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 23587.135 | 23587.135 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | shotium | pass | no | 7990.357 | 7990.357 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 31670.483 | 31670.483 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 153.48 | 491.319 | 19.992 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 809.087 | 3113.039 | 4.448 | 5.27× |
| soak | 4 | puppeteer-chrome | pass | yes | 1393.954 | 6011.573 | 2.720 | 9.08× |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 3 eligible cell(s), with 3 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 3 | 3 / 3 | 3 |
| 2 | puppeteer-chrome | 20.001× | 3 | 3 / 3 | 0 |
| not ranked (partial coverage) | playwright-shell | 6.486× | 2 | 2 / 3 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 8.253× | 2 | 2 / 3 | 0 |
| not ranked (partial coverage) | playwright-chrome | 18.696× | 2 | 2 / 3 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `lifecycle/c1`, `parallel/c1`, `soak/c4`
- puppeteer-chrome: `lifecycle/c1`, `parallel/c1`, `soak/c4`
- playwright-shell: `lifecycle/c1`, `soak/c4`
- puppeteer-shell: `lifecycle/c1`, `soak/c4`
- playwright-chrome: `lifecycle/c1`, `parallel/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.7.3/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | noisy | no | 2312 | 9193 | 0.258 | N/A |
| cold | 1 | shotium | noisy | no | 71 | 110 | 12.766 | N/A |
| cold | 1 | playwright-shell | noisy | no | 995 | 1673 | 0.865 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 2186 | 7123 | 0.307 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 824.5 | 1283 | 1.124 | N/A |
| cold-settled | 1 | shotium | noisy | no | 10.582 | 12.104 | 93.769 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 154.706 | 202.644 | 6.316 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 226.063 | 355.059 | 4.192 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 357.849 | 417.875 | 2.804 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 218.422 | 424.919 | 4.280 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 846.42 | 1343.527 | 1.098 | 13.54× |
| lifecycle | 1 | shotium | pass | yes | 62.533 | 133.514 | 15.344 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1689.986 | 2814.157 | 0.550 | 27.03× |
| lifecycle | 1 | playwright-shell | pass | yes | 600.201 | 983.544 | 1.502 | 9.60× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1737.444 | 2612.639 | 0.538 | 27.78× |
| warm | 1 | shotium | noisy | no | 13.74 | 23.592 | 63.058 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 219.981 | 242.864 | 4.668 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 244.507 | 318.497 | 4.157 | N/A |
| warm | 1 | playwright-shell | noisy | no | 172.913 | 278.844 | 0.422 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 407.429 | 482.91 | 2.453 | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 259.828 | 2192.232 | 2.892 | 12.58× |
| parallel | 1 | shotium | pass | yes | 20.653 | 269.368 | 23.523 | 1.00× |
| parallel | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-chrome | pass | yes | 510.721 | 2372.376 | 1.743 | 24.73× |
| parallel | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | 8047.218 | 8047.218 | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | pass | no | 745.542 | 1623.018 | 2.621 | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | fail | no | 371.463 | 1301.967 | 4.262 | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | pass | no | 1253.016 | 2292.263 | 3.066 | N/A |
| parallel | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | 7777.378 | 7777.378 | N/A | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 96.768 | 120.724 | 10.655 | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 32180.625 | 32180.625 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15994.811 | 15994.811 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 26121.917 | 26121.917 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5005.036 | 5005.036 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 11643.319 | 11643.319 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 100.562 | 398.494 | 25.413 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 440.729 | 891.268 | 8.808 | 4.38× |
| soak | 4 | puppeteer-chrome | pass | yes | 1203.992 | 4705.369 | 3.203 | 11.97× |
| soak | 4 | playwright-chrome | fail | no | 438.233 | 8232.617 | 4.008 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 506.046 | 1125.162 | 7.523 | 5.03× |

