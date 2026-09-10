# Shotium 0.3.4 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `6bf4cb2fbc8e69e59371281916ce6bf49a73ef24`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 3 | 7 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | pass | shotium | 3 | 10 |
| darwin-arm64 | noisy | shotium | 2 | 6 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 6.318× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 6.476× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 7.726× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 10.678× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.3.4/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 968 | 1059 | 1.038 | 16.69× |
| cold | 1 | playwright-shell | pass | yes | 735 | 816 | 1.342 | 12.67× |
| cold | 1 | puppeteer-chrome | pass | yes | 768 | 929 | 1.268 | 13.24× |
| cold | 1 | puppeteer-shell | pass | yes | 593 | 677 | 1.667 | 10.22× |
| cold | 1 | shotium | pass | yes | 58 | 58 | 17.766 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 167.887 | 181.528 | 5.961 | 11.01× |
| cold-settled | 1 | playwright-chrome | pass | yes | 146.079 | 155.363 | 6.799 | 9.58× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 140.415 | 143.694 | 7.230 | 9.21× |
| cold-settled | 1 | shotium | pass | yes | 15.25 | 15.316 | 65.278 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 118.39 | 130.116 | 8.370 | 7.76× |
| lifecycle | 1 | playwright-shell | pass | yes | 636.122 | 720.384 | 1.548 | 11.56× |
| lifecycle | 1 | playwright-chrome | pass | yes | 888.563 | 1547.626 | 1.075 | 16.14× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 604.424 | 836.983 | 1.616 | 10.98× |
| lifecycle | 1 | shotium | pass | yes | 55.041 | 98.141 | 15.595 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 960.459 | 1470.715 | 0.987 | 17.45× |
| warm | 1 | playwright-chrome | pass | yes | 134.055 | 199.45 | 7.143 | 12.32× |
| warm | 1 | shotium | pass | yes | 10.883 | 14.01 | 84.827 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 104.531 | 118.521 | 9.398 | 9.60× |
| warm | 1 | puppeteer-shell | pass | yes | 119.126 | 155.121 | 8.285 | 10.95× |
| warm | 1 | puppeteer-chrome | pass | yes | 149.585 | 161.64 | 6.808 | 13.74× |
| batch | 1 | shotium | pass | yes | 17.349 | 258.185 | 25.492 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 124.959 | 369.105 | 6.492 | 7.20× |
| batch | 1 | playwright-shell | pass | yes | 117.833 | 979.449 | 6.566 | 6.79× |
| batch | 1 | puppeteer-chrome | pass | yes | 160.886 | 390.74 | 5.415 | 9.27× |
| batch | 1 | playwright-chrome | pass | yes | 144.153 | 451.435 | 5.747 | 8.31× |
| parallel | 1 | playwright-chrome | pass | yes | 172.957 | 392.422 | 5.057 | 7.81× |
| parallel | 1 | puppeteer-shell | pass | yes | 151.342 | 396.18 | 5.605 | 6.83× |
| parallel | 1 | puppeteer-chrome | pass | yes | 183.806 | 404.991 | 4.689 | 8.30× |
| parallel | 1 | shotium | pass | yes | 22.154 | 260.222 | 22.858 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 142.549 | 382.12 | 5.928 | 6.43× |
| parallel | 2 | puppeteer-shell | pass | yes | 233.13 | 515.335 | 7.529 | 3.82× |
| parallel | 2 | puppeteer-chrome | fail | no | 273.718 | 887.056 | 3.403 | N/A |
| parallel | 2 | shotium | pass | yes | 61.011 | 290.558 | 22.540 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 234.4 | 458.809 | 7.760 | 3.84× |
| parallel | 2 | playwright-chrome | pass | yes | 261.85 | 563.31 | 6.724 | 4.29× |
| parallel | 4 | puppeteer-chrome | fail | no | 288.501 | 17177.086 | 5.705 | N/A |
| parallel | 4 | shotium | pass | yes | 129.457 | 369.309 | 22.596 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 404.348 | 703.97 | 9.148 | 3.12× |
| parallel | 4 | playwright-chrome | pass | yes | 461.597 | 813.668 | 7.693 | 3.57× |
| parallel | 4 | puppeteer-shell | pass | yes | 441.191 | 797.963 | 8.560 | 3.41× |
| reuse-page | 1 | playwright-shell | pass | no | 83.169 | 92.761 | 11.917 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 83.375 | 101.022 | 11.698 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.998 | 108.764 | 10.029 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.791 | 104.646 | 9.980 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 906 | 938 | 1.232 | 5.63× |
| resident | 1 | shotium | pass | yes | 161 | 239 | 6.903 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 993 | 1006 | 1.017 | 6.17× |
| resident | 1 | playwright-chrome | pass | yes | 1062 | 1131 | 0.990 | 6.60× |
| resident | 1 | puppeteer-chrome | pass | yes | 947 | 989 | 1.062 | 5.88× |
| faults | 1 | puppeteer-shell | pass | no | 10596.352 | 10596.352 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 11432.223 | 11432.223 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10600.512 | 10600.512 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15990.13 | 15990.13 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 18069.959 | 18069.959 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 392.555 | 803.371 | 9.215 | 2.87× |
| soak | 4 | puppeteer-chrome | fail | no | 281.904 | 16672.389 | 4.085 | N/A |
| soak | 4 | playwright-chrome | pass | yes | 497.892 | 991.252 | 7.661 | 3.64× |
| soak | 4 | puppeteer-shell | pass | yes | 435.006 | 890.946 | 8.673 | 3.18× |
| soak | 4 | shotium | pass | yes | 136.76 | 386.368 | 22.789 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.307× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.152× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-arm64@0.3.4/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 825 | 854 | 1.208 | 12.89× |
| cold | 1 | playwright-shell | pass | yes | 642 | 683 | 1.548 | 10.03× |
| cold | 1 | shotium | pass | yes | 64 | 67 | 15.695 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 129.203 | 131.142 | 8.152 | 3.74× |
| cold-settled | 1 | playwright-chrome | pass | yes | 152.322 | 180.166 | 6.424 | 4.41× |
| cold-settled | 1 | shotium | pass | yes | 34.544 | 38.919 | 27.562 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 568.552 | 643.339 | 1.729 | 7.76× |
| lifecycle | 1 | playwright-chrome | pass | yes | 736.373 | 821.402 | 1.339 | 10.05× |
| lifecycle | 1 | shotium | pass | yes | 73.306 | 100.994 | 12.887 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 149.214 | 161.934 | 6.912 | 4.86× |
| warm | 1 | playwright-shell | pass | yes | 116.697 | 142.91 | 8.442 | 3.80× |
| warm | 1 | shotium | pass | yes | 30.695 | 36.555 | 31.334 | 1.00× |
| batch | 1 | shotium | pass | yes | 39.785 | 262.974 | 17.308 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 127.498 | 371.249 | 6.503 | 3.20× |
| batch | 1 | playwright-chrome | pass | yes | 158.029 | 372.94 | 5.499 | 3.97× |
| parallel | 1 | playwright-chrome | pass | yes | 190.41 | 415.985 | 4.701 | 4.45× |
| parallel | 1 | shotium | pass | yes | 42.789 | 264.578 | 16.643 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 136.427 | 371.524 | 6.181 | 3.19× |
| parallel | 2 | shotium | pass | yes | 90.545 | 305.199 | 16.888 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 194.687 | 432.184 | 8.846 | 2.15× |
| parallel | 2 | playwright-chrome | pass | yes | 249.64 | 550.854 | 7.320 | 2.76× |
| parallel | 4 | playwright-shell | pass | yes | 343.873 | 566.271 | 11.079 | 1.80× |
| parallel | 4 | playwright-chrome | pass | yes | 442.561 | 710.259 | 8.531 | 2.32× |
| parallel | 4 | shotium | pass | yes | 190.731 | 418.073 | 16.744 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 83.262 | 84.071 | 12.126 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 84.561 | 206.044 | 9.113 | N/A |
| resident | 1 | playwright-shell | pass | yes | 913 | 956 | 1.152 | 2.22× |
| resident | 1 | playwright-chrome | pass | yes | 965 | 1051 | 1.128 | 2.34× |
| resident | 1 | shotium | pass | yes | 412 | 443 | 2.536 | 1.00× |
| faults | 1 | shotium | pass | no | 10196.866 | 10196.866 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 15048.746 | 15048.746 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14944.527 | 14944.527 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 383.771 | 689.889 | 9.979 | 2.07× |
| soak | 4 | playwright-shell | pass | yes | 298.078 | 613.475 | 12.348 | 1.61× |
| soak | 4 | shotium | pass | yes | 185.082 | 437.208 | 17.213 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 7 eligible cell(s), with 7 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 7 / 7 | 7 |
| 2 | playwright-shell | 6.549× | 6 | 7 / 7 | 0 |
| 3 | puppeteer-shell | 7.331× | 6 | 7 / 7 | 0 |
| not ranked (partial coverage) | playwright-chrome | 8.584× | 4 | 5 / 7 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 12.029× | 5 | 5 / 7 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `resident/c1`, `soak/c4`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `resident/c1`, `soak/c4`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `resident/c1`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `parallel/c1`, `parallel/c2`, `resident/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.3.4\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 1044 | 1058 | 0.957 | 14.70× |
| cold | 1 | playwright-shell | pass | yes | 811 | 845 | 1.222 | 11.42× |
| cold | 1 | puppeteer-chrome | pass | yes | 1090 | 1159 | 0.921 | 15.35× |
| cold | 1 | puppeteer-shell | pass | yes | 911 | 972 | 1.089 | 12.83× |
| cold | 1 | shotium | pass | yes | 71 | 74 | 13.944 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 210.7 | 285.226 | 0.541 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 164.612 | 217.761 | 0.537 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | no | 154.73 | 166.975 | 6.437 | N/A |
| cold-settled | 1 | shotium | noisy | no | 16.621 | 18.945 | 0.704 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 168.733 | 185.349 | 6.022 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 1091.345 | 2169.241 | 0.822 | 8.75× |
| lifecycle | 1 | playwright-chrome | fail | no | 2103.701 | 2379.563 | 0.271 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1434.476 | 4142.553 | 0.613 | 11.50× |
| lifecycle | 1 | shotium | pass | yes | 124.75 | 236.508 | 7.523 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2699.991 | 8236.974 | 0.332 | 21.64× |
| warm | 1 | playwright-chrome | noisy | no | 165.42 | 167.006 | 0.196 | N/A |
| warm | 1 | shotium | noisy | no | 14.393 | 16.056 | 0.713 | N/A |
| warm | 1 | playwright-shell | noisy | no | 162.441 | 197.436 | 1.109 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 157.394 | 190.576 | 0.431 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 179.416 | 200.762 | 0.220 | N/A |
| batch | 1 | shotium | pass | yes | 27.169 | 264.021 | 20.316 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 182.748 | 421.665 | 4.776 | 6.73× |
| batch | 1 | playwright-shell | pass | yes | 183.692 | 418.247 | 4.864 | 6.76× |
| batch | 1 | puppeteer-chrome | pass | yes | 204.949 | 441.015 | 4.325 | 7.54× |
| batch | 1 | playwright-chrome | pass | yes | 195.823 | 421.773 | 4.592 | 7.21× |
| parallel | 1 | playwright-chrome | pass | yes | 149.801 | 375.951 | 5.580 | 7.97× |
| parallel | 1 | puppeteer-shell | pass | yes | 162.688 | 401.209 | 5.500 | 8.66× |
| parallel | 1 | puppeteer-chrome | pass | yes | 171.901 | 404.11 | 5.047 | 9.15× |
| parallel | 1 | shotium | pass | yes | 18.795 | 271.82 | 24.069 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 147.27 | 376.852 | 5.760 | 7.84× |
| parallel | 2 | puppeteer-shell | pass | yes | 217.508 | 528.42 | 8.118 | 3.99× |
| parallel | 2 | puppeteer-chrome | fail | no | 221.404 | 30235.136 | 0.544 | N/A |
| parallel | 2 | shotium | pass | yes | 54.562 | 287.141 | 24.427 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 180.029 | 457.091 | 9.006 | 3.30× |
| parallel | 2 | playwright-chrome | pass | yes | 232.608 | 493.635 | 7.585 | 4.26× |
| parallel | 4 | puppeteer-chrome | fail | no | 267.261 | 15753.25 | 3.219 | N/A |
| parallel | 4 | shotium | noisy | no | 9615.83 | 9615.83 | N/A | N/A |
| parallel | 4 | playwright-shell | pass | no | 307.539 | 578.468 | 11.697 | N/A |
| parallel | 4 | playwright-chrome | pass | no | 422.612 | 1251.108 | 8.712 | N/A |
| parallel | 4 | puppeteer-shell | pass | no | 361.909 | 640.017 | 10.529 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 84.593 | 99.717 | 11.576 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 97.67 | 120.089 | 10.386 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.22 | 133.022 | 9.555 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 116.776 | 149.738 | 8.276 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 727 | 1008 | 1.399 | 11.02× |
| resident | 1 | shotium | pass | yes | 66 | 335 | 7.839 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 680 | 1103 | 1.286 | 10.30× |
| resident | 1 | playwright-chrome | pass | yes | 854 | 1439 | 1.034 | 12.94× |
| resident | 1 | puppeteer-chrome | pass | yes | 725 | 1015 | 1.386 | 10.98× |
| faults | 1 | puppeteer-shell | pass | no | 29395.603 | 29395.603 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 32420.789 | 32420.789 | N/A | N/A |
| faults | 1 | shotium | pass | no | 11276.998 | 11276.998 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 23269.916 | 23269.916 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 31625.424 | 31625.424 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 423.772 | 1259.413 | 8.913 | 2.87× |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 563.274 | 1727.28 | 6.838 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 445.005 | 920.766 | 8.638 | 3.02× |
| soak | 4 | shotium | pass | yes | 147.554 | 406.688 | 21.072 | 1.00× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-arm64@0.3.4\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 75 | 78 | 13.487 | N/A |
| cold-settled | 1 | shotium | noisy | no | 15.285 | 15.399 | 0.529 | N/A |
| lifecycle | 1 | shotium | pass | no | 99.836 | 194.205 | 9.120 | N/A |
| warm | 1 | shotium | pass | no | 14.311 | 15.111 | 67.311 | N/A |
| batch | 1 | shotium | pass | no | 25.561 | 273.566 | 21.208 | N/A |
| parallel | 1 | shotium | pass | no | 23.935 | 270.521 | 21.584 | N/A |
| parallel | 2 | shotium | pass | no | 63.019 | 291.215 | 22.201 | N/A |
| parallel | 4 | shotium | pass | no | 141.699 | 378.061 | 21.461 | N/A |
| resident | 1 | shotium | pass | no | 449 | 970 | 2.017 | N/A |
| faults | 1 | shotium | pass | no | 10905.686 | 10905.686 | N/A | N/A |
| soak | 4 | shotium | pass | no | 137.123 | 471.398 | 22.242 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 7.214× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 7.965× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 22.369× | 7 | 8 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 13.689× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.3.4/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | pass | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 3382 | 5754 | 0.264 | 30.20× |
| cold | 1 | playwright-shell | pass | yes | 1194 | 2499 | 0.704 | 10.66× |
| cold | 1 | puppeteer-chrome | pass | yes | 2585 | 7200 | 0.267 | 23.08× |
| cold | 1 | puppeteer-shell | pass | yes | 1204 | 2159 | 0.693 | 10.75× |
| cold | 1 | shotium | pass | yes | 112 | 332 | 7.114 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 389.329 | 517.756 | 2.602 | 18.76× |
| cold-settled | 1 | playwright-chrome | pass | yes | 871.483 | 941.641 | 1.163 | 42.00× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 358.109 | 385.7 | 2.884 | 17.26× |
| cold-settled | 1 | shotium | pass | yes | 20.75 | 27.18 | 47.521 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 298.129 | 429.544 | 3.258 | 14.37× |
| lifecycle | 1 | playwright-shell | pass | yes | 1639.733 | 2336.808 | 0.592 | 10.73× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3769.103 | 4492.27 | 0.266 | 24.66× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1884.733 | 2874.847 | 0.512 | 12.33× |
| lifecycle | 1 | shotium | pass | yes | 152.814 | 343.086 | 5.920 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3549.47 | 4506.189 | 0.276 | 23.23× |
| warm | 1 | playwright-chrome | pass | yes | 807.246 | 975.503 | 1.218 | 51.21× |
| warm | 1 | shotium | pass | yes | 15.764 | 30.61 | 56.514 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 257.824 | 833.508 | 3.260 | 16.36× |
| warm | 1 | puppeteer-shell | pass | yes | 329.438 | 387.211 | 3.180 | 20.90× |
| warm | 1 | puppeteer-chrome | pass | yes | 376.138 | 572.185 | 2.588 | 23.86× |
| batch | 1 | shotium | pass | yes | 24.675 | 275.257 | 18.346 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 343.907 | 631.945 | 2.808 | 13.94× |
| batch | 1 | playwright-shell | pass | yes | 313.46 | 693.539 | 3.012 | 12.70× |
| batch | 1 | puppeteer-chrome | pass | yes | 395.842 | 787.164 | 2.427 | 16.04× |
| batch | 1 | playwright-chrome | pass | yes | 985.076 | 2062.772 | 0.945 | 39.92× |
| parallel | 1 | playwright-chrome | pass | yes | 927.952 | 1387.398 | 1.052 | 24.95× |
| parallel | 1 | puppeteer-shell | pass | yes | 392.793 | 742.045 | 2.433 | 10.56× |
| parallel | 1 | puppeteer-chrome | pass | yes | 454.799 | 764.345 | 2.105 | 12.23× |
| parallel | 1 | shotium | pass | yes | 37.195 | 263.968 | 16.525 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 362.51 | 754.167 | 2.584 | 9.75× |
| parallel | 2 | puppeteer-shell | pass | yes | 666.64 | 1644.338 | 2.791 | 5.94× |
| parallel | 2 | puppeteer-chrome | fail | no | 912.248 | 6270.835 | 1.749 | N/A |
| parallel | 2 | shotium | pass | yes | 112.157 | 313.262 | 15.147 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 516.881 | 1543.666 | 3.604 | 4.61× |
| parallel | 2 | playwright-chrome | noisy | no | 15720.139 | 15720.139 | N/A | N/A |
| parallel | 4 | puppeteer-chrome | fail | no | 862.795 | 9710.825 | 1.044 | N/A |
| parallel | 4 | shotium | pass | yes | 243.801 | 473.138 | 14.976 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 957.453 | 2276.087 | 3.918 | 3.93× |
| parallel | 4 | playwright-chrome | pass | yes | 3277.955 | 5065.79 | 1.266 | 13.45× |
| parallel | 4 | puppeteer-shell | pass | yes | 866.728 | 1817.059 | 4.238 | 3.56× |
| reuse-page | 1 | playwright-shell | pass | no | 201.577 | 329.883 | 4.926 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 194.5 | 356.789 | 4.808 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 242.565 | 428.383 | 4.134 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 262.291 | 513.625 | 3.594 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 2120 | 2727 | 0.484 | 1.82× |
| resident | 1 | shotium | pass | yes | 1164 | 1299 | 0.830 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 1906 | 2500 | 0.494 | 1.64× |
| resident | 1 | playwright-chrome | pass | yes | 3402 | 3734 | 0.295 | 2.92× |
| resident | 1 | puppeteer-chrome | pass | yes | 2227 | 2649 | 0.445 | 1.91× |
| faults | 1 | puppeteer-shell | pass | no | 22424.018 | 22424.018 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 33811.841 | 33811.841 | N/A | N/A |
| faults | 1 | shotium | pass | no | 11149.065 | 11149.065 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 20206.027 | 20206.027 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 35181.748 | 35181.748 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 647.203 | 1374.942 | 6.010 | 3.87× |
| soak | 4 | puppeteer-chrome | fail | no | 575.29 | 17423.099 | 2.291 | N/A |
| soak | 4 | playwright-chrome | fail | no | 2817.534 | 5408.62 | 1.445 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 634.801 | 1365.476 | 6.083 | 3.80× |
| soak | 4 | shotium | pass | yes | 167.22 | 423.632 | 19.787 | 1.00× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 6 eligible cell(s), with 6 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 6 / 6 | 6 |
| 2 | puppeteer-shell | 8.538× | 6 | 6 / 6 | 0 |
| not ranked (partial coverage) | playwright-shell | 6.592× | 5 | 5 / 6 | 0 |
| not ranked (partial coverage) | playwright-chrome | 19.949× | 4 | 4 / 6 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 29.480× | 3 | 3 / 6 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.3.4/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | fail | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 1787 | 5321 | 0.458 | 36.47× |
| cold | 1 | playwright-shell | pass | yes | 574 | 716 | 1.673 | 11.71× |
| cold | 1 | puppeteer-chrome | pass | yes | 1568 | 4023 | 0.536 | 32.00× |
| cold | 1 | puppeteer-shell | pass | yes | 715 | 1013 | 1.384 | 14.59× |
| cold | 1 | shotium | pass | yes | 49 | 60 | 20.173 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 333.655 | 392.236 | 3.143 | 27.73× |
| cold-settled | 1 | playwright-chrome | pass | yes | 179.48 | 273.939 | 5.278 | 14.92× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 249.041 | 313.573 | 3.991 | 20.70× |
| cold-settled | 1 | shotium | pass | yes | 12.033 | 24.124 | 72.189 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 143.588 | 251.639 | 6.259 | 11.93× |
| lifecycle | 1 | playwright-shell | pass | yes | 508.993 | 805.163 | 1.845 | 10.05× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1444.713 | 2269.864 | 0.646 | 28.52× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 672.728 | 965.009 | 1.496 | 13.28× |
| lifecycle | 1 | shotium | pass | yes | 50.661 | 91.365 | 17.976 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1462.848 | 2303.407 | 0.660 | 28.88× |
| warm | 1 | playwright-chrome | noisy | no | 234.185 | 257.066 | 4.211 | N/A |
| warm | 1 | shotium | noisy | no | 13.656 | 17.164 | 67.585 | N/A |
| warm | 1 | playwright-shell | noisy | no | 183.771 | 220.583 | 5.315 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 8583.953 | 8583.953 | N/A | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 339.149 | 382.84 | 2.909 | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-shell | pass | no | 203.003 | 440.95 | 4.534 | N/A |
| batch | 1 | puppeteer-chrome | pass | no | 396.572 | 1940.31 | 2.096 | N/A |
| batch | 1 | playwright-chrome | pass | no | 250.759 | 1594.177 | 3.038 | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 219.523 | 1416.128 | 3.470 | 10.21× |
| parallel | 1 | puppeteer-shell | pass | yes | 233.17 | 488.917 | 4.186 | 10.84× |
| parallel | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | shotium | pass | yes | 21.503 | 269.099 | 23.187 | 1.00× |
| parallel | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 91.514 | 118.59 | 10.810 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 104.352 | 213.969 | 8.699 | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | 9073.436 | 9073.436 | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 176.988 | 265.169 | 5.400 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 717 | 1043 | 1.324 | 2.19× |
| resident | 1 | shotium | pass | yes | 328 | 491 | 2.877 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 764 | 907 | 1.270 | 2.33× |
| resident | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 12403.151 | 12403.151 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 27814.085 | 27814.085 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9846.204 | 9846.204 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16409.179 | 16409.179 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 26670.64 | 26670.64 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 418.025 | 1027.572 | 8.938 | 3.81× |
| soak | 4 | puppeteer-chrome | fail | no | 436.496 | 22887.189 | 4.023 | N/A |
| soak | 4 | playwright-chrome | fail | no | 599.531 | 3180.149 | 6.343 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 447.546 | 1010.044 | 8.654 | 4.07× |
| soak | 4 | shotium | pass | yes | 109.86 | 424.746 | 24.568 | 1.00× |

