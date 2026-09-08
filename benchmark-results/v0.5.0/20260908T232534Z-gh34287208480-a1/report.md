# Shotium 0.5.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `442edbe7a9c346e66b07dc49871d1b8def3b32c2`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 2 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 9 |
| darwin-arm64 | pass | shotium | 3 | 10 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 6.167× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 6.188× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 7.432× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 9.365× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.5.0/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 878 | 903 | 1.133 | 14.63× |
| cold | 1 | puppeteer-shell | pass | yes | 651 | 655 | 1.539 | 10.85× |
| cold | 1 | playwright-shell | pass | yes | 770 | 803 | 1.292 | 12.83× |
| cold | 1 | shotium | pass | yes | 60 | 62 | 16.627 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 953 | 984 | 1.044 | 15.88× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 127.859 | 142.547 | 7.603 | 8.68× |
| cold-settled | 1 | playwright-shell | pass | yes | 127.16 | 133.421 | 7.822 | 8.64× |
| cold-settled | 1 | shotium | pass | yes | 14.722 | 15.941 | 68.438 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 164.912 | 195.968 | 5.884 | 11.20× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 160.157 | 174.597 | 6.169 | 10.88× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 669.59 | 720.564 | 1.497 | 11.88× |
| lifecycle | 1 | playwright-shell | pass | yes | 684.574 | 739.283 | 1.473 | 12.15× |
| lifecycle | 1 | shotium | pass | yes | 56.361 | 102.644 | 14.993 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 935.956 | 1004.175 | 1.062 | 16.61× |
| lifecycle | 1 | playwright-chrome | pass | yes | 864.012 | 914.113 | 1.152 | 15.33× |
| warm | 1 | puppeteer-shell | pass | yes | 133.184 | 149.875 | 7.503 | 9.71× |
| warm | 1 | playwright-shell | pass | yes | 131.873 | 159.499 | 7.653 | 9.61× |
| warm | 1 | shotium | pass | yes | 13.722 | 15.764 | 68.437 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 166.604 | 196.769 | 5.931 | 12.14× |
| warm | 1 | playwright-chrome | pass | yes | 151.164 | 217.469 | 6.320 | 11.02× |
| batch | 1 | puppeteer-chrome | pass | yes | 187.905 | 419.833 | 4.627 | 9.37× |
| batch | 1 | puppeteer-shell | pass | yes | 152.234 | 387.671 | 5.503 | 7.59× |
| batch | 1 | playwright-chrome | pass | yes | 187.37 | 410.267 | 4.712 | 9.34× |
| batch | 1 | playwright-shell | pass | yes | 144.012 | 375.125 | 5.765 | 7.18× |
| batch | 1 | shotium | pass | yes | 20.055 | 269.308 | 23.639 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 171.56 | 463.026 | 5.020 | 8.86× |
| parallel | 1 | puppeteer-shell | pass | yes | 152.005 | 388.628 | 5.648 | 7.85× |
| parallel | 1 | shotium | pass | yes | 19.359 | 265.388 | 23.935 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 186.444 | 408.738 | 4.711 | 9.63× |
| parallel | 1 | playwright-shell | pass | yes | 144.913 | 377.57 | 5.910 | 7.49× |
| parallel | 2 | puppeteer-shell | pass | yes | 228.975 | 509.458 | 7.531 | 4.48× |
| parallel | 2 | shotium | pass | yes | 51.147 | 291.695 | 23.997 | 1.00× |
| parallel | 2 | puppeteer-chrome | fail | no | 275.67 | 931.142 | 5.744 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 225.658 | 453.459 | 7.980 | 4.41× |
| parallel | 2 | playwright-chrome | pass | yes | 272.977 | 542.755 | 6.700 | 5.34× |
| parallel | 4 | shotium | pass | yes | 120.453 | 366.483 | 23.976 | 1.00× |
| parallel | 4 | puppeteer-chrome | fail | no | 278.693 | 17105.689 | 5.729 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 402.821 | 732.296 | 9.146 | 3.34× |
| parallel | 4 | playwright-chrome | pass | yes | 452.804 | 828.804 | 7.843 | 3.76× |
| parallel | 4 | puppeteer-shell | pass | yes | 441.321 | 686.441 | 8.673 | 3.66× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.357 | 98.448 | 11.858 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.925 | 118.359 | 9.809 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.347 | 93.616 | 11.908 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.985 | 105.975 | 10.121 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 851 | 896 | 1.181 | 2.12× |
| resident | 1 | playwright-chrome | pass | yes | 1014 | 1097 | 1.037 | 2.53× |
| resident | 1 | shotium | pass | yes | 401 | 445 | 2.467 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 941 | 1013 | 1.130 | 2.35× |
| resident | 1 | puppeteer-chrome | pass | yes | 875 | 971 | 1.267 | 2.18× |
| faults | 1 | playwright-chrome | pass | no | 17393.058 | 17393.058 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15835.264 | 15835.264 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 12224.16 | 12224.16 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 11773.327 | 11773.327 | N/A | N/A |
| faults | 1 | shotium | pass | no | 11086.956 | 11086.956 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 119.356 | 406.567 | 24.117 | 1.00× |
| soak | 4 | puppeteer-chrome | fail | no | 283.902 | 24766.787 | 4.097 | N/A |
| soak | 4 | playwright-shell | pass | yes | 394.512 | 832.829 | 9.322 | 3.31× |
| soak | 4 | playwright-chrome | pass | yes | 485.916 | 1770.281 | 7.797 | 4.07× |
| soak | 4 | puppeteer-shell | pass | yes | 436.133 | 913.507 | 8.832 | 3.65× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.602× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.504× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-arm64@0.5.0/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 624 | 663 | 1.588 | 10.76× |
| cold | 1 | playwright-chrome | pass | yes | 800 | 846 | 1.237 | 13.79× |
| cold | 1 | shotium | pass | yes | 58 | 61 | 17.073 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 126.375 | 131.026 | 8.017 | 3.93× |
| cold-settled | 1 | shotium | pass | yes | 32.166 | 33.684 | 31.250 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 151.725 | 170.757 | 6.523 | 4.72× |
| lifecycle | 1 | shotium | pass | yes | 68.147 | 85.652 | 14.269 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 724.975 | 786.387 | 1.381 | 10.64× |
| lifecycle | 1 | playwright-shell | pass | yes | 553.271 | 616.806 | 1.779 | 8.12× |
| warm | 1 | playwright-chrome | pass | yes | 149.663 | 167.951 | 6.583 | 4.87× |
| warm | 1 | shotium | pass | yes | 30.725 | 34.969 | 31.954 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 116.718 | 143.923 | 8.263 | 3.80× |
| batch | 1 | playwright-chrome | pass | yes | 164.118 | 389.654 | 5.343 | 4.57× |
| batch | 1 | shotium | pass | yes | 35.942 | 260.819 | 18.772 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 128.897 | 357.377 | 6.398 | 3.59× |
| parallel | 1 | playwright-shell | pass | yes | 131.867 | 358.176 | 6.341 | 3.67× |
| parallel | 1 | playwright-chrome | pass | yes | 161.926 | 406.435 | 5.341 | 4.51× |
| parallel | 1 | shotium | pass | yes | 35.902 | 264.908 | 18.461 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 241.017 | 505.288 | 7.588 | 3.25× |
| parallel | 2 | shotium | pass | yes | 74.113 | 310.723 | 18.888 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 188.743 | 406.616 | 9.287 | 2.55× |
| parallel | 4 | shotium | pass | yes | 160.348 | 390.908 | 18.818 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 301.376 | 531.915 | 12.022 | 1.88× |
| parallel | 4 | playwright-chrome | pass | yes | 381.147 | 587.397 | 9.837 | 2.38× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.249 | 87.124 | 12.042 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.313 | 84.116 | 12.230 | N/A |
| resident | 1 | shotium | pass | yes | 347 | 438 | 2.865 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 898 | 938 | 1.102 | 2.59× |
| resident | 1 | playwright-shell | pass | yes | 847 | 882 | 1.189 | 2.44× |
| faults | 1 | shotium | pass | no | 9951.642 | 9951.642 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14775.412 | 14775.412 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 15251.731 | 15251.731 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 401.897 | 694.577 | 9.662 | 2.48× |
| soak | 4 | playwright-shell | pass | yes | 297.475 | 612.208 | 12.184 | 1.83× |
| soak | 4 | shotium | pass | yes | 162.342 | 412.849 | 18.821 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 6.302× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 6.312× | 6 | 8 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 7.178× | 5 | 7 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 13.388× | 4 | 4 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.5.0\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 1164 | 1470 | 0.823 | 16.39× |
| cold | 1 | puppeteer-shell | pass | yes | 1014 | 1250 | 0.946 | 14.28× |
| cold | 1 | playwright-shell | pass | yes | 1001 | 1130 | 1.007 | 14.10× |
| cold | 1 | shotium | pass | yes | 71 | 77 | 14.028 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 1166 | 1341 | 0.840 | 16.42× |
| cold-settled | 1 | puppeteer-shell | pass | no | 167.335 | 206.987 | 5.898 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 172.309 | 246.494 | 5.431 | N/A |
| cold-settled | 1 | shotium | noisy | no | 14.975 | 24.436 | 0.739 | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 183.291 | 209.929 | 5.454 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 184.707 | 202.345 | 0.396 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1088.786 | 1538.009 | 0.839 | 11.35× |
| lifecycle | 1 | playwright-shell | pass | yes | 872.644 | 1075.29 | 1.130 | 9.09× |
| lifecycle | 1 | shotium | pass | yes | 95.963 | 196.119 | 8.956 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2127.793 | 2256.694 | 0.471 | 22.17× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1650.424 | 1931.022 | 0.595 | 17.20× |
| warm | 1 | puppeteer-shell | pass | yes | 166.567 | 198.368 | 5.914 | 12.55× |
| warm | 1 | playwright-shell | pass | yes | 168.365 | 195.848 | 5.888 | 12.69× |
| warm | 1 | shotium | pass | yes | 13.272 | 13.982 | 72.099 | 1.00× |
| warm | 1 | puppeteer-chrome | noisy | no | 167.961 | 201.929 | 1.113 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 166.326 | 226.255 | 1.202 | N/A |
| batch | 1 | puppeteer-chrome | pass | yes | 221.942 | 1502.396 | 3.966 | 8.60× |
| batch | 1 | puppeteer-shell | noisy | no | 14166.958 | 14166.958 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | yes | 195.709 | 424.618 | 4.683 | 7.59× |
| batch | 1 | playwright-shell | pass | yes | 186.588 | 434.757 | 4.764 | 7.23× |
| batch | 1 | shotium | pass | yes | 25.793 | 408.18 | 19.722 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 140.676 | 360.036 | 5.973 | 8.83× |
| parallel | 1 | puppeteer-shell | pass | yes | 149.675 | 401.05 | 5.582 | 9.39× |
| parallel | 1 | shotium | pass | yes | 15.932 | 269.195 | 25.534 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 163.615 | 399.072 | 5.292 | 10.27× |
| parallel | 1 | playwright-shell | pass | yes | 150.448 | 392.384 | 5.663 | 9.44× |
| parallel | 2 | puppeteer-shell | pass | yes | 224.031 | 446.703 | 8.125 | 5.28× |
| parallel | 2 | shotium | pass | yes | 42.43 | 299.203 | 26.582 | 1.00× |
| parallel | 2 | puppeteer-chrome | fail | no | 241.065 | 30232.577 | 1.265 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 182.02 | 426.401 | 9.254 | 4.29× |
| parallel | 2 | playwright-chrome | pass | yes | 228.12 | 503.876 | 7.754 | 5.38× |
| parallel | 4 | shotium | pass | yes | 96.089 | 371.495 | 26.387 | 1.00× |
| parallel | 4 | puppeteer-chrome | fail | no | 303.489 | 2533.905 | 0.894 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 389.27 | 716.186 | 9.595 | 4.05× |
| parallel | 4 | playwright-chrome | pass | yes | 397.141 | 943.855 | 9.247 | 4.13× |
| parallel | 4 | puppeteer-shell | pass | yes | 348.73 | 616.24 | 10.907 | 3.63× |
| reuse-page | 1 | playwright-chrome | pass | no | 99.682 | 114.27 | 10.260 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 115.695 | 130.715 | 8.627 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 84.218 | 107.091 | 11.532 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.391 | 129.773 | 9.643 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 666 | 1554 | 1.101 | 1.97× |
| resident | 1 | playwright-chrome | pass | yes | 789 | 1366 | 1.093 | 2.33× |
| resident | 1 | shotium | pass | yes | 338 | 722 | 2.476 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 732 | 830 | 1.340 | 2.17× |
| resident | 1 | puppeteer-chrome | noisy | no | 28931.302 | 28931.302 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 28514.397 | 28514.397 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 24736.896 | 24736.896 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 33955.123 | 33955.123 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 27186.337 | 27186.337 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10703.832 | 10703.832 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 131.008 | 409.231 | 22.342 | 1.00× |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 490.927 | 1079.187 | 7.847 | 3.75× |
| soak | 4 | playwright-chrome | fail | no | 569.601 | 1859.461 | 6.674 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 457.538 | 842.264 | 8.381 | 3.49× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-arm64@0.5.0\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 75 | 308 | 9.309 | N/A |
| cold-settled | 1 | shotium | noisy | no | 12.463 | 12.994 | 0.087 | N/A |
| lifecycle | 1 | shotium | pass | no | 98.199 | 122.802 | 9.908 | N/A |
| warm | 1 | shotium | noisy | no | 12.963 | 13.389 | 0.111 | N/A |
| batch | 1 | shotium | noisy | no | 8352.021 | 8352.021 | N/A | N/A |
| parallel | 1 | shotium | pass | no | 21.611 | 428.939 | 21.719 | N/A |
| parallel | 2 | shotium | pass | no | 47.177 | 291.949 | 25.376 | N/A |
| parallel | 4 | shotium | pass | no | 114.054 | 354.284 | 24.147 | N/A |
| resident | 1 | shotium | noisy | no | 13276.646 | 13276.646 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10744.742 | 10744.742 | N/A | N/A |
| soak | 4 | shotium | pass | no | 111.724 | 409.751 | 24.043 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 9.394× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 10.588× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 15.737× | 7 | 7 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 39.449× | 6 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.5.0/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 2086 | 7169 | 0.353 | 25.13× |
| cold | 1 | puppeteer-shell | pass | yes | 1106 | 1441 | 0.845 | 13.33× |
| cold | 1 | playwright-shell | pass | yes | 1099 | 1299 | 0.911 | 13.24× |
| cold | 1 | shotium | pass | yes | 83 | 91 | 11.925 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 3361 | 3759 | 0.293 | 40.49× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 294.878 | 357.953 | 3.370 | 23.60× |
| cold-settled | 1 | playwright-shell | pass | yes | 267.385 | 315.355 | 3.818 | 21.40× |
| cold-settled | 1 | shotium | pass | yes | 12.493 | 13.879 | 78.495 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 793.294 | 926.316 | 1.273 | 63.50× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 271.295 | 747.752 | 2.686 | 21.72× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1626.766 | 3866.93 | 0.570 | 13.70× |
| lifecycle | 1 | playwright-shell | pass | yes | 1274.499 | 1781.508 | 0.770 | 10.74× |
| lifecycle | 1 | shotium | pass | yes | 118.716 | 216.837 | 8.303 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2683.737 | 3556.442 | 0.358 | 22.61× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3061.591 | 3731.156 | 0.323 | 25.79× |
| warm | 1 | puppeteer-shell | pass | yes | 344.301 | 678.984 | 2.718 | 24.78× |
| warm | 1 | playwright-shell | pass | yes | 256.597 | 684.834 | 3.203 | 18.47× |
| warm | 1 | shotium | pass | yes | 13.893 | 22.337 | 66.692 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 370.049 | 643.911 | 2.488 | 26.64× |
| warm | 1 | playwright-chrome | pass | yes | 870.687 | 1055.316 | 1.135 | 62.67× |
| batch | 1 | puppeteer-chrome | pass | yes | 436.15 | 1338.147 | 2.191 | 20.74× |
| batch | 1 | puppeteer-shell | pass | yes | 340.945 | 643.009 | 2.814 | 16.21× |
| batch | 1 | playwright-chrome | pass | yes | 840.497 | 1248.573 | 1.166 | 39.97× |
| batch | 1 | playwright-shell | pass | yes | 302.924 | 677.328 | 3.076 | 14.40× |
| batch | 1 | shotium | pass | yes | 21.03 | 266.946 | 22.882 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 893.166 | 1822.381 | 1.040 | 39.32× |
| parallel | 1 | puppeteer-shell | pass | yes | 345.179 | 754.792 | 2.741 | 15.19× |
| parallel | 1 | shotium | pass | yes | 22.718 | 261.727 | 22.955 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 374.182 | 1147.133 | 2.472 | 16.47× |
| parallel | 1 | playwright-shell | pass | yes | 314.246 | 697.805 | 2.883 | 13.83× |
| parallel | 2 | puppeteer-shell | pass | yes | 393.99 | 761.655 | 4.770 | 6.41× |
| parallel | 2 | shotium | pass | yes | 61.425 | 328.495 | 21.065 | 1.00× |
| parallel | 2 | puppeteer-chrome | fail | no | 504.095 | 1557.769 | 3.169 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 306.721 | 653.108 | 5.774 | 4.99× |
| parallel | 2 | playwright-chrome | fail | no | 1454.369 | 3100.231 | 1.368 | N/A |
| parallel | 4 | shotium | pass | yes | 120.953 | 350.327 | 23.118 | 1.00× |
| parallel | 4 | puppeteer-chrome | fail | no | 502.388 | 29298.587 | 3.345 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 607.003 | 921.357 | 6.563 | 5.02× |
| parallel | 4 | playwright-chrome | pass | yes | 2753.952 | 4790.048 | 1.490 | 22.77× |
| parallel | 4 | puppeteer-shell | pass | yes | 581.684 | 1156.747 | 6.450 | 4.81× |
| reuse-page | 1 | playwright-chrome | pass | no | 133.614 | 194.201 | 7.052 | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | 9118.849 | 9118.849 | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 1581 | 1672 | 0.628 | 2.06× |
| resident | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | shotium | pass | yes | 767 | 1152 | 1.182 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 1558 | 1861 | 0.628 | 2.03× |
| resident | 1 | puppeteer-chrome | pass | yes | 1633 | 2114 | 0.585 | 2.13× |
| faults | 1 | playwright-chrome | pass | no | 41277.717 | 41277.717 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 35851.576 | 35851.576 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 36383.019 | 36383.019 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 21924.198 | 21924.198 | N/A | N/A |
| faults | 1 | shotium | pass | no | 13651.126 | 13651.126 | N/A | N/A |
| soak | 4 | shotium | pass | no | 168.87 | 584.189 | 18.845 | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 654.574 | 25580.395 | 2.022 | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 7.993× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 11.351× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 17.719× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 14.614× | 7 | 8 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.5.0/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | pass | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | fail | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 1294 | 5171 | 0.531 | 33.18× |
| cold | 1 | puppeteer-shell | pass | yes | 615 | 722 | 1.629 | 15.77× |
| cold | 1 | playwright-shell | pass | yes | 482 | 594 | 2.011 | 12.36× |
| cold | 1 | shotium | pass | yes | 39 | 54 | 24.306 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 1272 | 3973 | 0.605 | 32.62× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 304.075 | 385.841 | 3.290 | 36.96× |
| cold-settled | 1 | playwright-shell | pass | yes | 130.599 | 147.939 | 7.755 | 15.87× |
| cold-settled | 1 | shotium | pass | yes | 8.228 | 9.573 | 121.530 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 171.392 | 197.927 | 5.929 | 20.83× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 286.417 | 472.438 | 3.332 | 34.81× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 580.62 | 888.512 | 1.609 | 15.07× |
| lifecycle | 1 | playwright-shell | pass | yes | 467.188 | 601.583 | 2.088 | 12.13× |
| lifecycle | 1 | shotium | pass | yes | 38.528 | 84.32 | 22.513 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1359.525 | 1731.224 | 0.739 | 35.29× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1313.095 | 1825.559 | 0.741 | 34.08× |
| warm | 1 | puppeteer-shell | pass | yes | 306.145 | 390.676 | 3.342 | 31.06× |
| warm | 1 | playwright-shell | pass | yes | 182.033 | 263.083 | 5.334 | 18.47× |
| warm | 1 | shotium | pass | yes | 9.856 | 31.062 | 83.211 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 378.838 | 578.212 | 2.592 | 38.44× |
| warm | 1 | playwright-chrome | pass | yes | 233.258 | 295.43 | 4.159 | 23.67× |
| batch | 1 | puppeteer-chrome | pass | yes | 371.232 | 1588.02 | 2.368 | 21.45× |
| batch | 1 | puppeteer-shell | pass | yes | 317.564 | 761.171 | 3.082 | 18.35× |
| batch | 1 | playwright-chrome | pass | yes | 239.155 | 1631.383 | 3.232 | 13.82× |
| batch | 1 | playwright-shell | pass | yes | 172.567 | 429.537 | 5.085 | 9.97× |
| batch | 1 | shotium | pass | yes | 17.305 | 269.401 | 26.806 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 204.858 | 1652.355 | 3.561 | 10.43× |
| parallel | 1 | puppeteer-shell | pass | yes | 300.525 | 731.45 | 3.027 | 15.30× |
| parallel | 1 | shotium | pass | yes | 19.646 | 271.89 | 23.661 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 380.375 | 1987.582 | 2.202 | 19.36× |
| parallel | 1 | playwright-shell | pass | yes | 189.502 | 454.126 | 4.779 | 9.65× |
| parallel | 2 | puppeteer-shell | pass | yes | 330.669 | 711.192 | 5.843 | 9.21× |
| parallel | 2 | shotium | pass | yes | 35.909 | 344.768 | 28.224 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 483.639 | 1813.936 | 3.239 | 13.47× |
| parallel | 2 | playwright-shell | pass | yes | 244.056 | 487.146 | 7.399 | 6.80× |
| parallel | 2 | playwright-chrome | pass | yes | 293.639 | 1617.06 | 5.598 | 8.18× |
| parallel | 4 | shotium | pass | yes | 85.398 | 360.344 | 27.703 | 1.00× |
| parallel | 4 | puppeteer-chrome | pass | yes | 527.623 | 27264.556 | 3.594 | 6.18× |
| parallel | 4 | playwright-shell | pass | yes | 370.178 | 724.6 | 10.057 | 4.33× |
| parallel | 4 | playwright-chrome | fail | no | 587.536 | 2380.661 | 5.963 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 508.244 | 896.014 | 7.475 | 5.95× |
| reuse-page | 1 | playwright-chrome | pass | no | 121.791 | 297.59 | 7.760 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 286.009 | 459.335 | 3.302 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 97.044 | 131.854 | 10.498 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 334.726 | 480.747 | 2.989 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 894 | 1060 | 1.102 | 1.87× |
| resident | 1 | playwright-chrome | pass | yes | 1537 | 2197 | 0.642 | 3.22× |
| resident | 1 | shotium | pass | yes | 477 | 826 | 1.795 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 1017 | 1334 | 0.951 | 2.13× |
| resident | 1 | puppeteer-chrome | pass | yes | 1517 | 2305 | 0.603 | 3.18× |
| faults | 1 | playwright-chrome | pass | no | 33723.896 | 33723.896 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 17368.202 | 17368.202 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 28199.649 | 28199.649 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 12682.691 | 12682.691 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10512.219 | 10512.219 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 99.44 | 447.042 | 25.234 | 1.00× |
| soak | 4 | puppeteer-chrome | fail | no | 529.056 | 26755.96 | 3.471 | N/A |
| soak | 4 | playwright-shell | pass | yes | 398.89 | 1294.439 | 9.406 | 4.01× |
| soak | 4 | playwright-chrome | fail | no | 676.733 | 4178.327 | 5.591 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 448.789 | 1046.038 | 8.516 | 4.51× |

