# Shotium 0.12.4 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `de3f9d21ab60cf909b5af15a5f5f796323a47847`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 2 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 2 | 10 |
| darwin-arm64 | noisy | shotium | 2 | 3 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 10.025× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 10.489× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 14.372× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 13.142× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.12.4/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 70 | 80 | 13.834 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 648 | 651 | 1.567 | 9.26× |
| cold | 1 | puppeteer-chrome | pass | yes | 899 | 2006 | 0.945 | 12.84× |
| cold | 1 | playwright-shell | pass | yes | 791 | 811 | 1.262 | 11.30× |
| cold | 1 | playwright-chrome | pass | yes | 984 | 993 | 1.019 | 14.06× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 130.183 | 139.951 | 7.556 | 10.61× |
| cold-settled | 1 | shotium | pass | yes | 12.269 | 13.117 | 79.692 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 175.094 | 186.806 | 5.734 | 14.27× |
| cold-settled | 1 | playwright-shell | pass | yes | 145.687 | 168.704 | 6.733 | 11.87× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 178.769 | 193.669 | 5.522 | 14.57× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 666.641 | 719.848 | 1.503 | 9.53× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 949.548 | 1081.754 | 1.038 | 13.58× |
| lifecycle | 1 | playwright-shell | pass | yes | 665.024 | 744.522 | 1.467 | 9.51× |
| lifecycle | 1 | playwright-chrome | fail | no | 2066.142 | 2096.146 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 69.943 | 115.686 | 12.617 | 1.00× |
| warm | 1 | shotium | pass | yes | 8.244 | 12.988 | 101.896 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 148.742 | 168.974 | 6.629 | 18.04× |
| warm | 1 | playwright-chrome | pass | yes | 179.856 | 212.481 | 5.526 | 21.82× |
| warm | 1 | puppeteer-shell | pass | yes | 133.106 | 149.595 | 7.372 | 16.15× |
| warm | 1 | puppeteer-chrome | pass | yes | 197.614 | 201.749 | 5.141 | 23.97× |
| batch | 1 | puppeteer-shell | pass | yes | 158.103 | 397.533 | 5.491 | 11.19× |
| batch | 1 | playwright-shell | pass | yes | 161.398 | 400.865 | 5.270 | 11.42× |
| batch | 1 | shotium | pass | yes | 14.128 | 257.551 | 28.388 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 230.144 | 451.345 | 3.975 | 16.29× |
| batch | 1 | playwright-chrome | pass | yes | 203.231 | 418.745 | 4.454 | 14.38× |
| parallel | 1 | shotium | pass | yes | 9.407 | 255.549 | 34.020 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 126.279 | 458.954 | 6.647 | 13.42× |
| parallel | 1 | puppeteer-shell | pass | yes | 125.986 | 360.836 | 6.667 | 13.39× |
| parallel | 1 | playwright-chrome | pass | yes | 154.568 | 375.913 | 5.741 | 16.43× |
| parallel | 1 | puppeteer-chrome | pass | yes | 177.487 | 406.198 | 5.137 | 18.87× |
| parallel | 2 | playwright-shell | pass | yes | 163.677 | 392.921 | 10.638 | 7.25× |
| parallel | 2 | puppeteer-shell | pass | yes | 182.976 | 397.582 | 10.002 | 8.11× |
| parallel | 2 | playwright-chrome | pass | yes | 213.948 | 485.12 | 8.597 | 9.48× |
| parallel | 2 | puppeteer-chrome | pass | yes | 296.895 | 560.5 | 6.557 | 13.15× |
| parallel | 2 | shotium | pass | yes | 22.575 | 275.06 | 35.092 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 296.21 | 532.029 | 12.486 | 5.48× |
| parallel | 4 | playwright-chrome | pass | yes | 382.034 | 660.337 | 9.697 | 7.07× |
| parallel | 4 | puppeteer-chrome | pass | yes | 498.445 | 799.632 | 7.767 | 9.22× |
| parallel | 4 | shotium | pass | yes | 54.04 | 307.031 | 35.743 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 287.108 | 599.603 | 12.201 | 5.31× |
| reuse-page | 1 | playwright-chrome | pass | no | 85.822 | 116.533 | 10.964 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.94 | 101.633 | 10.116 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 116.502 | 233.745 | 7.816 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.463 | 104.89 | 11.330 | N/A |
| resident | 1 | shotium | pass | yes | 34 | 170 | 13.035 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 864 | 898 | 1.330 | 25.41× |
| resident | 1 | puppeteer-chrome | pass | yes | 727 | 763 | 1.456 | 21.38× |
| resident | 1 | playwright-shell | pass | yes | 793 | 843 | 1.310 | 23.32× |
| resident | 1 | puppeteer-shell | pass | yes | 685 | 739 | 1.452 | 20.15× |
| faults | 1 | playwright-chrome | pass | no | 19970.012 | 19970.012 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 14634.831 | 14634.831 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 17238.778 | 17238.778 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4827.729 | 4827.729 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 12910.31 | 12910.31 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 91.106 | 360.368 | 28.837 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 463.202 | 955.725 | 8.148 | 5.08× |
| soak | 4 | playwright-chrome | pass | yes | 604.956 | 967.022 | 6.377 | 6.64× |
| soak | 4 | puppeteer-shell | pass | yes | 460.492 | 897.399 | 8.480 | 5.05× |
| soak | 4 | puppeteer-chrome | pass | yes | 705.742 | 1272.62 | 5.530 | 7.75× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.246× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.182× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.12.4/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 64 | 66 | 15.660 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 628 | 665 | 1.584 | 9.81× |
| cold | 1 | playwright-chrome | pass | yes | 809 | 903 | 1.220 | 12.64× |
| cold-settled | 1 | shotium | pass | yes | 21.156 | 22.61 | 46.462 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 162.754 | 178.857 | 6.180 | 7.69× |
| cold-settled | 1 | playwright-shell | pass | yes | 130.26 | 146.245 | 7.513 | 6.16× |
| lifecycle | 1 | shotium | pass | yes | 68.81 | 86.652 | 13.991 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 564.575 | 629.73 | 1.748 | 8.20× |
| lifecycle | 1 | playwright-chrome | fail | no | 1469.861 | 1853.017 | N/A | N/A |
| warm | 1 | shotium | pass | yes | 18.157 | 22.535 | 51.127 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 166.066 | 1563.82 | 3.448 | 9.15× |
| warm | 1 | playwright-shell | pass | yes | 133.116 | 150.304 | 7.298 | 7.33× |
| batch | 1 | playwright-shell | pass | yes | 151.049 | 388.698 | 5.781 | 6.40× |
| batch | 1 | playwright-chrome | pass | yes | 192.281 | 405.083 | 4.691 | 8.15× |
| batch | 1 | shotium | pass | yes | 23.602 | 261.191 | 23.564 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 149.554 | 389.049 | 5.702 | 6.25× |
| parallel | 1 | playwright-chrome | pass | yes | 190.351 | 420.838 | 4.734 | 7.96× |
| parallel | 1 | shotium | pass | yes | 23.917 | 261.905 | 23.416 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 260.983 | 537.543 | 6.905 | 5.07× |
| parallel | 2 | shotium | pass | yes | 51.516 | 297.252 | 23.202 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 204.182 | 417.284 | 8.560 | 3.96× |
| parallel | 4 | shotium | pass | yes | 113.846 | 355.607 | 23.259 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 395.687 | 659.552 | 10.002 | 3.48× |
| parallel | 4 | playwright-chrome | pass | yes | 447.607 | 709.717 | 8.434 | 3.93× |
| reuse-page | 1 | playwright-chrome | pass | no | 99.982 | 116.184 | 9.888 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 99.9 | 106.683 | 10.011 | N/A |
| resident | 1 | shotium | pass | yes | 343 | 440 | 3.166 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 889 | 936 | 1.122 | 2.59× |
| resident | 1 | playwright-chrome | pass | yes | 976 | 998 | 1.043 | 2.85× |
| faults | 1 | playwright-shell | pass | no | 15769.961 | 15769.961 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4641.444 | 4641.444 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 14867.787 | 14867.787 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 328.081 | 636.646 | 11.128 | 3.04× |
| soak | 4 | shotium | pass | yes | 107.941 | 370.268 | 24.144 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 435.272 | 770.627 | 8.894 | 4.03× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 9 / 9 | 9 |
| 2 | playwright-shell | 9.318× | 8 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 9.192× | 7 | 8 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 13.087× | 7 | 8 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 8.743× | 5 | 6 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.12.4\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 55 | 65 | 17.903 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 985 | 1391 | 0.959 | 17.91× |
| cold | 1 | puppeteer-chrome | pass | yes | 1257 | 1594 | 0.787 | 22.85× |
| cold | 1 | playwright-shell | pass | yes | 854 | 1666 | 0.992 | 15.53× |
| cold | 1 | playwright-chrome | pass | yes | 1105 | 1211 | 0.894 | 20.09× |
| cold-settled | 1 | puppeteer-shell | noisy | no | 159.913 | 164.128 | 0.443 | N/A |
| cold-settled | 1 | shotium | pass | yes | 12.419 | 14.024 | 78.158 | 1.00× |
| cold-settled | 1 | playwright-chrome | noisy | no | 177.821 | 183.145 | 0.167 | N/A |
| cold-settled | 1 | playwright-shell | pass | yes | 171.865 | 183.403 | 5.901 | 13.84× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 204.364 | 229.045 | 0.209 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1373.465 | 2238.945 | 0.701 | 14.56× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2395.077 | 7386.353 | 0.331 | 25.40× |
| lifecycle | 1 | playwright-shell | pass | yes | 1018.521 | 1581.1 | 0.946 | 10.80× |
| lifecycle | 1 | playwright-chrome | fail | no | 7632.285 | 8012.644 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 94.311 | 172.983 | 9.991 | 1.00× |
| warm | 1 | shotium | pass | yes | 8.423 | 11.188 | 105.314 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 175.564 | 187.623 | 5.648 | 20.84× |
| warm | 1 | playwright-chrome | noisy | no | 167.369 | 196.126 | 0.872 | N/A |
| warm | 1 | puppeteer-shell | pass | yes | 165.838 | 187.612 | 5.961 | 19.69× |
| warm | 1 | puppeteer-chrome | pass | yes | 233.978 | 2650.056 | 2.767 | 27.78× |
| batch | 1 | puppeteer-shell | pass | yes | 182.241 | 1288.745 | 4.479 | 12.19× |
| batch | 1 | playwright-shell | pass | yes | 186.181 | 427.641 | 4.618 | 12.45× |
| batch | 1 | shotium | pass | yes | 14.956 | 272.427 | 26.774 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 256.393 | 470.256 | 3.635 | 17.14× |
| batch | 1 | playwright-chrome | pass | yes | 198.661 | 446.547 | 4.442 | 13.28× |
| parallel | 1 | shotium | pass | yes | 15.464 | 358.397 | 23.468 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 198.282 | 436.006 | 4.491 | 12.82× |
| parallel | 1 | puppeteer-shell | pass | yes | 205.513 | 529.342 | 4.276 | 13.29× |
| parallel | 1 | playwright-chrome | pass | yes | 200.033 | 420.116 | 4.443 | 12.94× |
| parallel | 1 | puppeteer-chrome | pass | yes | 249.128 | 457.337 | 3.643 | 16.11× |
| parallel | 2 | playwright-shell | pass | no | 260.235 | 539.021 | 6.863 | N/A |
| parallel | 2 | puppeteer-shell | pass | no | 276.359 | 477.125 | 6.691 | N/A |
| parallel | 2 | playwright-chrome | pass | no | 319.78 | 698.002 | 5.723 | N/A |
| parallel | 2 | puppeteer-chrome | pass | no | 441.367 | 770.031 | 4.248 | N/A |
| parallel | 2 | shotium | noisy | no | 8719.111 | 8719.111 | N/A | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 453.557 | 849.355 | 8.560 | 4.40× |
| parallel | 4 | playwright-chrome | pass | yes | 597.417 | 1413.368 | 6.388 | 5.79× |
| parallel | 4 | puppeteer-chrome | pass | yes | 842.178 | 1361.169 | 4.568 | 8.17× |
| parallel | 4 | shotium | pass | yes | 103.12 | 362.587 | 26.077 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 459.532 | 687.271 | 8.330 | 4.46× |
| reuse-page | 1 | playwright-chrome | pass | no | 116.738 | 148.907 | 8.640 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 116.775 | 140.973 | 8.544 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 101.112 | 129.845 | 9.423 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 100.463 | 120.021 | 9.650 | N/A |
| resident | 1 | shotium | pass | yes | 226 | 655 | 3.640 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 897 | 1425 | 1.009 | 3.97× |
| resident | 1 | puppeteer-chrome | pass | yes | 649 | 1289 | 1.327 | 2.87× |
| resident | 1 | playwright-shell | pass | yes | 789 | 1123 | 1.138 | 3.49× |
| resident | 1 | puppeteer-shell | pass | yes | 746 | 1113 | 1.236 | 3.30× |
| faults | 1 | playwright-chrome | pass | no | 28679.156 | 28679.156 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 29022.588 | 29022.588 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 29407.034 | 29407.034 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9645.403 | 9645.403 | N/A | N/A |
| faults | 1 | puppeteer-shell | fail | no | 30763.458 | 30763.458 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 102.987 | 4285.604 | 22.975 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 454.246 | 905.445 | 8.458 | 4.41× |
| soak | 4 | playwright-chrome | pass | yes | 579.396 | 1690.845 | 6.468 | 5.63× |
| soak | 4 | puppeteer-shell | pass | yes | 434.667 | 806.041 | 8.866 | 4.22× |
| soak | 4 | puppeteer-chrome | pass | yes | 848.787 | 1414.93 | 4.682 | 8.24× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.12.4\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 57 | 61 | 17.544 | N/A |
| cold-settled | 1 | shotium | pass | no | 9.317 | 10.669 | 103.434 | N/A |
| lifecycle | 1 | shotium | pass | no | 79.642 | 135.264 | 11.656 | N/A |
| warm | 1 | shotium | pass | no | 6.279 | 8.589 | 137.273 | N/A |
| batch | 1 | shotium | noisy | no | 8477.758 | 8477.758 | N/A | N/A |
| parallel | 1 | shotium | pass | no | 12.792 | 449.857 | 24.268 | N/A |
| parallel | 2 | shotium | pass | no | 31.792 | 283.487 | 29.813 | N/A |
| parallel | 4 | shotium | pass | no | 78.441 | 323.428 | 29.340 | N/A |
| resident | 1 | shotium | pass | no | 285 | 1835 | 1.832 | N/A |
| faults | 1 | shotium | pass | no | 10104.215 | 10104.215 | N/A | N/A |
| soak | 4 | shotium | pass | no | 70.414 | 611.173 | 30.160 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-chrome | 16.893× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-shell | 8.907× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 11.530× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 28.703× | 6 | 8 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.12.4/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 200 | 228 | 5.076 | 1.00× |
| cold | 1 | puppeteer-shell | pass | yes | 2585 | 2721 | 0.411 | 12.93× |
| cold | 1 | puppeteer-chrome | pass | yes | 4594 | 4831 | 0.231 | 22.97× |
| cold | 1 | playwright-shell | pass | yes | 2269 | 3007 | 0.414 | 11.35× |
| cold | 1 | playwright-chrome | pass | yes | 6217 | 13366 | 0.141 | 31.09× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 340.794 | 501.736 | 2.824 | 17.99× |
| cold-settled | 1 | shotium | pass | yes | 18.943 | 21.532 | 51.152 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 968.632 | 1118.325 | 1.047 | 51.13× |
| cold-settled | 1 | playwright-shell | pass | yes | 400.782 | 838.749 | 2.206 | 21.16× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 579.507 | 2220.316 | 1.233 | 30.59× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 2315.448 | 2924.461 | 0.421 | 12.45× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 4106.665 | 5205.411 | 0.236 | 22.07× |
| lifecycle | 1 | playwright-shell | pass | yes | 1944.47 | 2779.56 | 0.491 | 10.45× |
| lifecycle | 1 | playwright-chrome | fail | no | 7303.703 | 9886.808 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 186.053 | 342.444 | 4.927 | 1.00× |
| warm | 1 | shotium | pass | yes | 7.747 | 10.608 | 116.496 | 1.00× |
| warm | 1 | playwright-shell | noisy | no | 290.451 | 520.306 | 3.332 | N/A |
| warm | 1 | playwright-chrome | pass | yes | 834.866 | 1045.767 | 1.163 | 107.77× |
| warm | 1 | puppeteer-shell | pass | yes | 339.088 | 473.309 | 2.969 | 43.77× |
| warm | 1 | puppeteer-chrome | pass | yes | 460.155 | 1121.896 | 1.882 | 59.40× |
| batch | 1 | puppeteer-shell | pass | yes | 340.973 | 762.889 | 2.772 | 18.34× |
| batch | 1 | playwright-shell | pass | yes | 338.22 | 1238.04 | 2.601 | 18.19× |
| batch | 1 | shotium | pass | yes | 18.591 | 290.892 | 23.944 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 444.08 | 850.757 | 2.114 | 23.89× |
| batch | 1 | playwright-chrome | pass | yes | 924.648 | 1836.12 | 1.018 | 49.74× |
| parallel | 1 | shotium | pass | yes | 32.66 | 375.265 | 14.952 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 434.384 | 1526.501 | 1.917 | 13.30× |
| parallel | 1 | puppeteer-shell | pass | yes | 431.032 | 1432.694 | 2.098 | 13.20× |
| parallel | 1 | playwright-chrome | pass | yes | 977.294 | 2141.625 | 0.961 | 29.92× |
| parallel | 1 | puppeteer-chrome | pass | yes | 580.963 | 1186.971 | 1.606 | 17.79× |
| parallel | 2 | playwright-shell | pass | yes | 522.072 | 1385.076 | 3.640 | 8.58× |
| parallel | 2 | puppeteer-shell | pass | yes | 512.042 | 897.628 | 3.775 | 8.41× |
| parallel | 2 | playwright-chrome | pass | yes | 1608.369 | 2911.007 | 1.195 | 26.43× |
| parallel | 2 | puppeteer-chrome | pass | yes | 906.536 | 1725.288 | 2.099 | 14.89× |
| parallel | 2 | shotium | pass | yes | 60.864 | 295.098 | 21.753 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 886.792 | 1543.225 | 4.341 | 6.04× |
| parallel | 4 | playwright-chrome | pass | yes | 3439.753 | 5924.731 | 1.208 | 23.42× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1861.273 | 3500.595 | 1.967 | 12.67× |
| parallel | 4 | shotium | pass | yes | 146.856 | 398.009 | 21.370 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 859.18 | 1298.817 | 4.550 | 5.85× |
| reuse-page | 1 | playwright-chrome | pass | no | 216.616 | 376.951 | 4.668 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 238.186 | 342.437 | 4.041 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 240.804 | 342.598 | 3.888 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 233.034 | 326.723 | 4.462 | N/A |
| resident | 1 | shotium | pass | yes | 685 | 828 | 1.444 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 2000 | 2151 | 0.496 | 2.92× |
| resident | 1 | puppeteer-chrome | pass | yes | 1473 | 2011 | 0.650 | 2.15× |
| resident | 1 | playwright-shell | pass | yes | 1466 | 1574 | 0.695 | 2.14× |
| resident | 1 | puppeteer-shell | pass | yes | 1584 | 2151 | 0.590 | 2.31× |
| faults | 1 | playwright-chrome | pass | no | 32826.128 | 32826.128 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 33441.268 | 33441.268 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 35884.318 | 35884.318 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9937.769 | 9937.769 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 25428.594 | 25428.594 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 156.292 | 1517.787 | 18.546 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 845.5 | 3359.67 | 4.384 | 5.41× |
| soak | 4 | playwright-chrome | fail | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | pass | yes | 1861.996 | 5453.737 | 2.055 | 11.91× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 3 eligible cell(s), with 3 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 3 | 3 / 3 | 3 |
| 2 | puppeteer-chrome | 13.286× | 3 | 3 / 3 | 0 |
| not ranked (partial coverage) | playwright-chrome | 7.053× | 2 | 2 / 3 | 0 |
| not ranked (partial coverage) | playwright-shell | 11.645× | 2 | 2 / 3 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 12.700× | 2 | 2 / 3 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `lifecycle/c1`, `parallel/c1`, `resident/c1`
- puppeteer-chrome: `lifecycle/c1`, `parallel/c1`, `resident/c1`
- playwright-chrome: `parallel/c1`, `resident/c1`
- playwright-shell: `lifecycle/c1`, `parallel/c1`
- puppeteer-shell: `lifecycle/c1`, `parallel/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.12.4/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | noisy | no | 58.5 | 73 | 16.760 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 748 | 901 | 1.333 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 1808 | 5230 | 0.430 | N/A |
| cold | 1 | playwright-shell | noisy | no | 712 | 933 | 1.351 | N/A |
| cold | 1 | playwright-chrome | noisy | no | 1850 | 3347 | 0.488 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 266.877 | 454.859 | 3.294 | N/A |
| cold-settled | 1 | shotium | noisy | no | 7.184 | 15.364 | 110.205 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 264.143 | 285.432 | 3.865 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 206.971 | 301.301 | 4.635 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 404.883 | 679.581 | 2.162 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 700.783 | 1175.221 | 1.347 | 9.77× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1739.981 | 2314.369 | 0.568 | 24.25× |
| lifecycle | 1 | playwright-shell | pass | yes | 626.829 | 1233.207 | 1.405 | 8.73× |
| lifecycle | 1 | playwright-chrome | fail | no | 3817.778 | 4461.227 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 71.764 | 171.799 | 12.142 | 1.00× |
| warm | 1 | shotium | noisy | no | 4.776 | 6.709 | 184.434 | N/A |
| warm | 1 | playwright-shell | noisy | no | 154.638 | 235.253 | 6.276 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 276.448 | 405.323 | 3.926 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 193.609 | 275.966 | 4.793 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 327.84 | 386.98 | 0.577 | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | shotium | pass | yes | 13.523 | 279.684 | 26.885 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 209.933 | 507.017 | 4.320 | 15.52× |
| parallel | 1 | puppeteer-shell | pass | yes | 223.345 | 533.165 | 4.155 | 16.52× |
| parallel | 1 | playwright-chrome | pass | yes | 246.125 | 1763.809 | 3.013 | 18.20× |
| parallel | 1 | puppeteer-chrome | pass | yes | 456.092 | 2165.568 | 1.925 | 33.73× |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | pass | no | 352.71 | 722.673 | 5.585 | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | pass | no | 461.317 | 886.861 | 7.904 | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | pass | no | 1264.616 | 2420.304 | 2.977 | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | pass | no | 505.495 | 936.083 | 7.751 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 129.552 | 190.106 | 7.140 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 184.621 | 249.794 | 5.314 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 141.299 | 225.503 | 6.581 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 114.632 | 141.63 | 8.738 | N/A |
| resident | 1 | shotium | pass | yes | 499 | 593 | 1.938 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 1364 | 2591 | 0.677 | 2.73× |
| resident | 1 | puppeteer-chrome | pass | yes | 1431 | 1853 | 0.669 | 2.87× |
| resident | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 36225.942 | 36225.942 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 42255.329 | 42255.329 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 22313.546 | 22313.546 | N/A | N/A |
| faults | 1 | shotium | pass | no | 7455.795 | 7455.795 | N/A | N/A |
| faults | 1 | puppeteer-shell | noisy | no | 9906.041 | 9906.041 | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | no | 643.215 | 1339.321 | 6.061 | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |

