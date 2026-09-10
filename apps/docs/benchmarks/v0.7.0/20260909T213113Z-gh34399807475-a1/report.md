# Shotium 0.7.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `1a845157a6151f57d8aaea59435b302ef603a7cc`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 3 | 10 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | pass | shotium | 3 | 10 |
| darwin-arm64 | pass | shotium | 3 | 10 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 6.456× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 6.573× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 7.939× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 10.441× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.7.0/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 887 | 931 | 1.124 | 15.03× |
| cold | 1 | puppeteer-shell | pass | yes | 652 | 675 | 1.538 | 11.05× |
| cold | 1 | playwright-shell | pass | yes | 781 | 832 | 1.262 | 13.24× |
| cold | 1 | playwright-chrome | pass | yes | 971 | 984 | 1.033 | 16.46× |
| cold | 1 | shotium | pass | yes | 59 | 64 | 16.667 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 14.249 | 15.383 | 68.226 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 163.062 | 195.121 | 6.005 | 11.44× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 171.84 | 173.257 | 5.998 | 12.06× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 135.361 | 143.027 | 7.458 | 9.50× |
| cold-settled | 1 | playwright-shell | pass | yes | 131.924 | 142.348 | 7.619 | 9.26× |
| lifecycle | 1 | shotium | pass | yes | 59.079 | 112.408 | 14.529 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 650.319 | 718.774 | 1.513 | 11.01× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 943.376 | 1001.089 | 1.057 | 15.97× |
| lifecycle | 1 | playwright-chrome | pass | yes | 886.598 | 1375.516 | 1.094 | 15.01× |
| lifecycle | 1 | playwright-shell | pass | yes | 676.892 | 737.385 | 1.474 | 11.46× |
| warm | 1 | playwright-chrome | pass | yes | 154.665 | 181.943 | 6.345 | 11.54× |
| warm | 1 | shotium | pass | yes | 13.402 | 23.984 | 66.632 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 133.183 | 150.004 | 7.559 | 9.94× |
| warm | 1 | puppeteer-chrome | pass | yes | 165.252 | 173.617 | 6.079 | 12.33× |
| warm | 1 | playwright-shell | pass | yes | 128.435 | 147.543 | 7.818 | 9.58× |
| batch | 1 | playwright-chrome | pass | yes | 179.762 | 414.369 | 4.876 | 9.34× |
| batch | 1 | playwright-shell | pass | yes | 136.593 | 373.257 | 5.888 | 7.10× |
| batch | 1 | puppeteer-chrome | pass | yes | 189.151 | 426.598 | 4.600 | 9.83× |
| batch | 1 | puppeteer-shell | pass | yes | 150.933 | 386.907 | 5.559 | 7.84× |
| batch | 1 | shotium | pass | yes | 19.251 | 261.137 | 24.337 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 169.407 | 670.612 | 4.969 | 9.13× |
| parallel | 1 | puppeteer-shell | pass | yes | 151.716 | 390.467 | 5.637 | 8.17× |
| parallel | 1 | playwright-shell | pass | yes | 142.632 | 386.883 | 5.905 | 7.69× |
| parallel | 1 | puppeteer-chrome | pass | yes | 185.758 | 405.752 | 4.747 | 10.01× |
| parallel | 1 | shotium | pass | yes | 18.559 | 260.857 | 24.144 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 231.122 | 509.555 | 7.585 | 4.30× |
| parallel | 2 | playwright-shell | pass | yes | 217.91 | 437.915 | 8.110 | 4.06× |
| parallel | 2 | puppeteer-chrome | fail | no | 276.142 | 894.822 | 5.549 | N/A |
| parallel | 2 | shotium | pass | yes | 53.707 | 285.179 | 23.932 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 264.994 | 550.996 | 6.745 | 4.93× |
| parallel | 4 | playwright-shell | pass | yes | 386.809 | 663.383 | 9.443 | 3.32× |
| parallel | 4 | puppeteer-chrome | fail | no | 273.394 | 16170.367 | 6.060 | N/A |
| parallel | 4 | shotium | pass | yes | 116.405 | 357.062 | 24.251 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 494.9 | 837.95 | 7.647 | 4.25× |
| parallel | 4 | puppeteer-shell | pass | yes | 435.804 | 749.224 | 8.738 | 3.74× |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.988 | 116.435 | 9.893 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.083 | 131.983 | 9.724 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 83.563 | 100.772 | 11.524 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.424 | 96.025 | 11.897 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 971 | 1027 | 1.093 | 3.85× |
| resident | 1 | playwright-chrome | pass | yes | 1051 | 1084 | 0.952 | 4.17× |
| resident | 1 | playwright-shell | pass | yes | 975 | 998 | 1.030 | 3.87× |
| resident | 1 | shotium | pass | yes | 252 | 421 | 4.086 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 888 | 958 | 1.156 | 3.52× |
| faults | 1 | playwright-shell | pass | no | 16710.475 | 16710.475 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5373.494 | 5373.494 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10667.375 | 10667.375 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 17895.569 | 17895.569 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 12596.022 | 12596.022 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 400.91 | 847.487 | 9.093 | 3.28× |
| soak | 4 | puppeteer-shell | pass | yes | 440.006 | 872.407 | 8.613 | 3.60× |
| soak | 4 | shotium | pass | yes | 122.074 | 434.089 | 24.225 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 499.232 | 1049.128 | 7.593 | 4.09× |
| soak | 4 | puppeteer-chrome | fail | no | 283.886 | 13981.486 | 4.054 | N/A |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.741× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.768× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-arm64@0.7.0/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 630 | 873 | 1.472 | 10.50× |
| cold | 1 | playwright-chrome | pass | yes | 813 | 1749 | 1.060 | 13.55× |
| cold | 1 | shotium | pass | yes | 60 | 64 | 16.548 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 30.795 | 33.635 | 31.720 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 114.053 | 127.519 | 8.456 | 3.70× |
| cold-settled | 1 | playwright-chrome | pass | yes | 157.391 | 170.283 | 6.456 | 5.11× |
| lifecycle | 1 | playwright-shell | pass | yes | 569.82 | 638.918 | 1.734 | 7.87× |
| lifecycle | 1 | shotium | pass | yes | 72.387 | 89.193 | 13.618 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 746.768 | 816.307 | 1.320 | 10.32× |
| warm | 1 | playwright-shell | pass | yes | 121.997 | 150.583 | 8.001 | 3.98× |
| warm | 1 | shotium | pass | yes | 30.659 | 37.946 | 31.718 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 150.043 | 235.231 | 6.328 | 4.89× |
| batch | 1 | playwright-shell | pass | yes | 127.234 | 357.212 | 6.526 | 3.54× |
| batch | 1 | shotium | pass | yes | 35.973 | 260.873 | 18.686 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 165.638 | 390.083 | 5.288 | 4.60× |
| parallel | 1 | playwright-shell | pass | yes | 135.314 | 1258.975 | 5.857 | 3.55× |
| parallel | 1 | playwright-chrome | pass | yes | 171.086 | 388.754 | 5.153 | 4.49× |
| parallel | 1 | shotium | pass | yes | 38.116 | 269.349 | 17.902 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 255.773 | 532.76 | 7.412 | 2.99× |
| parallel | 2 | shotium | pass | yes | 85.605 | 306.511 | 17.794 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 187.344 | 396.541 | 9.309 | 2.19× |
| parallel | 4 | shotium | pass | yes | 162.645 | 420.414 | 18.315 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 335.755 | 572.525 | 11.393 | 2.06× |
| parallel | 4 | playwright-chrome | pass | yes | 433.057 | 730.919 | 8.757 | 2.66× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.431 | 243.792 | 9.960 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.246 | 86.759 | 12.109 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 879 | 919 | 1.131 | 4.33× |
| resident | 1 | playwright-shell | pass | yes | 854 | 910 | 1.205 | 4.21× |
| resident | 1 | shotium | pass | yes | 203 | 259 | 4.723 | 1.00× |
| faults | 1 | playwright-shell | pass | no | 16285.694 | 16285.694 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16522.063 | 16522.063 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4567.479 | 4567.479 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 167.026 | 432.703 | 18.243 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 308.745 | 589.648 | 11.958 | 1.85× |
| soak | 4 | playwright-chrome | pass | yes | 407.351 | 736.099 | 9.448 | 2.44× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 7.309× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 7.909× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 8.443× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 13.500× | 5 | 5 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.7.0\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | pass | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 1297 | 1373 | 0.790 | 18.01× |
| cold | 1 | puppeteer-shell | pass | yes | 998 | 1356 | 0.956 | 13.86× |
| cold | 1 | playwright-shell | pass | yes | 891 | 1000 | 1.097 | 12.38× |
| cold | 1 | playwright-chrome | pass | yes | 1186 | 1222 | 0.861 | 16.47× |
| cold | 1 | shotium | pass | yes | 72 | 76 | 13.807 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 14.357 | 17.762 | 64.697 | 1.00× |
| cold-settled | 1 | playwright-chrome | noisy | no | 177.263 | 209.702 | 0.063 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 193.075 | 193.075 | 0.014 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | yes | 172.048 | 200.948 | 5.718 | 11.98× |
| cold-settled | 1 | playwright-shell | pass | yes | 151.657 | 225.171 | 5.981 | 10.56× |
| lifecycle | 1 | shotium | pass | yes | 107.181 | 245.854 | 8.549 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1283.162 | 1844.208 | 0.774 | 11.97× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2142.6 | 2403.709 | 0.465 | 19.99× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1790.883 | 12069.963 | 0.428 | 16.71× |
| lifecycle | 1 | playwright-shell | pass | yes | 1030.155 | 1951.192 | 0.838 | 9.61× |
| warm | 1 | playwright-chrome | pass | yes | 168.207 | 209.989 | 5.898 | 11.95× |
| warm | 1 | shotium | pass | yes | 14.072 | 16.869 | 68.318 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 150.526 | 184.465 | 6.300 | 10.70× |
| warm | 1 | puppeteer-chrome | noisy | no | 181.969 | 226.304 | 0.906 | N/A |
| warm | 1 | playwright-shell | pass | yes | 161.734 | 188.003 | 6.224 | 11.49× |
| batch | 1 | playwright-chrome | pass | yes | 195.063 | 435.087 | 4.594 | 8.35× |
| batch | 1 | playwright-shell | pass | yes | 178.448 | 408.059 | 4.950 | 7.64× |
| batch | 1 | puppeteer-chrome | pass | yes | 199.373 | 431.401 | 4.260 | 8.53× |
| batch | 1 | puppeteer-shell | pass | yes | 177.307 | 407.707 | 4.823 | 7.59× |
| batch | 1 | shotium | pass | yes | 23.365 | 267.504 | 22.321 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 182.323 | 398.391 | 4.823 | 7.84× |
| parallel | 1 | puppeteer-shell | pass | yes | 188.568 | 406.612 | 4.783 | 8.11× |
| parallel | 1 | playwright-shell | pass | yes | 181.618 | 427.454 | 4.882 | 7.81× |
| parallel | 1 | puppeteer-chrome | pass | yes | 198.986 | 460.264 | 4.516 | 8.56× |
| parallel | 1 | shotium | pass | yes | 23.244 | 272.346 | 22.456 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 290.392 | 626.822 | 6.299 | 5.15× |
| parallel | 2 | playwright-shell | pass | yes | 224.019 | 489.169 | 7.575 | 3.98× |
| parallel | 2 | puppeteer-chrome | fail | no | 301.296 | 30273.129 | 0.192 | N/A |
| parallel | 2 | shotium | pass | yes | 56.345 | 300.948 | 22.968 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 293.128 | 661.564 | 6.125 | 5.20× |
| parallel | 4 | playwright-shell | pass | yes | 410.158 | 801.571 | 8.937 | 3.16× |
| parallel | 4 | puppeteer-chrome | fail | no | 403.519 | 2309.051 | 0.899 | N/A |
| parallel | 4 | shotium | pass | yes | 129.672 | 373.831 | 22.357 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 515.243 | 1163.224 | 7.156 | 3.97× |
| parallel | 4 | puppeteer-shell | pass | yes | 480.327 | 871.567 | 8.192 | 3.70× |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.745 | 103.931 | 9.984 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 116.84 | 139.198 | 8.384 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 99.986 | 110.461 | 9.910 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 87.238 | 119.38 | 10.767 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 1159 | 1667 | 0.934 | 17.04× |
| resident | 1 | playwright-chrome | pass | yes | 866 | 1307 | 1.038 | 12.74× |
| resident | 1 | playwright-shell | pass | yes | 801 | 1164 | 1.172 | 11.78× |
| resident | 1 | shotium | pass | yes | 68 | 596 | 5.072 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 802 | 1476 | 1.001 | 11.79× |
| faults | 1 | playwright-shell | pass | no | 21912.003 | 21912.003 | N/A | N/A |
| faults | 1 | shotium | pass | no | 8655.902 | 8655.902 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 25316.65 | 25316.65 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 24922.61 | 24922.61 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 33740.292 | 33740.292 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 388.03 | 1140.566 | 9.686 | 3.41× |
| soak | 4 | puppeteer-shell | pass | yes | 369.344 | 694.982 | 10.298 | 3.25× |
| soak | 4 | shotium | pass | yes | 113.77 | 358.856 | 25.237 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 437.171 | 1035.431 | 8.805 | 3.84× |
| soak | 4 | puppeteer-chrome | fail | no | 281.302 | 30265.732 | 2.755 | N/A |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-arm64@0.7.0\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 64 | 80 | 15.152 | N/A |
| cold-settled | 1 | shotium | noisy | no | 12.215 | 12.468 | 0.594 | N/A |
| lifecycle | 1 | shotium | pass | no | 90.396 | 114.478 | 10.601 | N/A |
| warm | 1 | shotium | pass | no | 12.419 | 13.726 | 77.261 | N/A |
| batch | 1 | shotium | pass | no | 21.757 | 331.632 | 21.998 | N/A |
| parallel | 1 | shotium | pass | no | 21.987 | 274.354 | 22.215 | N/A |
| parallel | 2 | shotium | pass | no | 52.944 | 285.918 | 23.811 | N/A |
| parallel | 4 | shotium | pass | no | 108.428 | 349.435 | 24.452 | N/A |
| resident | 1 | shotium | pass | no | 543 | 1117 | 1.499 | N/A |
| faults | 1 | shotium | pass | no | 11257.462 | 11257.462 | N/A | N/A |
| soak | 4 | shotium | pass | no | 115.143 | 403.589 | 24.002 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 7.894× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 8.361× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 21.833× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 12.485× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.7.0/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | pass | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 2135 | 2759 | 0.437 | 25.42× |
| cold | 1 | puppeteer-shell | pass | yes | 1094 | 1720 | 0.822 | 13.02× |
| cold | 1 | playwright-shell | pass | yes | 1003 | 1148 | 0.963 | 11.94× |
| cold | 1 | playwright-chrome | pass | yes | 3628 | 4229 | 0.280 | 43.19× |
| cold | 1 | shotium | pass | yes | 84 | 110 | 11.382 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 14.504 | 19.051 | 65.867 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 782.871 | 961.984 | 1.218 | 53.98× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 337.41 | 468.211 | 2.781 | 23.26× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 358.406 | 625.662 | 2.707 | 24.71× |
| cold-settled | 1 | playwright-shell | pass | yes | 271.723 | 374.558 | 3.655 | 18.73× |
| lifecycle | 1 | shotium | pass | yes | 127.672 | 228.248 | 7.347 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1695.926 | 2130.315 | 0.599 | 13.28× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3023.275 | 3840.778 | 0.324 | 23.68× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3166.165 | 5614.217 | 0.300 | 24.80× |
| lifecycle | 1 | playwright-shell | pass | yes | 1242.035 | 3327.173 | 0.735 | 9.73× |
| warm | 1 | playwright-chrome | pass | yes | 912.014 | 1275.3 | 1.066 | 37.14× |
| warm | 1 | shotium | pass | yes | 24.553 | 47.909 | 37.847 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 358.099 | 960.502 | 2.427 | 14.58× |
| warm | 1 | puppeteer-chrome | pass | yes | 449.194 | 1497.139 | 1.900 | 18.29× |
| warm | 1 | playwright-shell | pass | yes | 347.124 | 645.382 | 2.703 | 14.14× |
| batch | 1 | playwright-chrome | pass | yes | 953.428 | 1687.205 | 1.016 | 28.31× |
| batch | 1 | playwright-shell | pass | yes | 363.899 | 760.976 | 2.594 | 10.81× |
| batch | 1 | puppeteer-chrome | pass | yes | 448.251 | 2009.234 | 1.994 | 13.31× |
| batch | 1 | puppeteer-shell | pass | yes | 356.091 | 726.433 | 2.655 | 10.57× |
| batch | 1 | shotium | pass | yes | 33.677 | 270.577 | 15.862 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 940.175 | 1676.42 | 1.038 | 14.88× |
| parallel | 1 | puppeteer-shell | pass | yes | 433.808 | 1443.656 | 1.970 | 6.86× |
| parallel | 1 | playwright-shell | pass | yes | 377.536 | 785.448 | 2.542 | 5.97× |
| parallel | 1 | puppeteer-chrome | pass | yes | 482.786 | 1120.391 | 1.968 | 7.64× |
| parallel | 1 | shotium | pass | yes | 63.192 | 485.262 | 9.887 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 594.268 | 1313.594 | 3.012 | 8.02× |
| parallel | 2 | playwright-shell | pass | yes | 474.593 | 1165.715 | 4.023 | 6.40× |
| parallel | 2 | puppeteer-chrome | fail | no | 658.652 | 6372.215 | 2.136 | N/A |
| parallel | 2 | shotium | pass | yes | 74.139 | 322.297 | 20.075 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 1429.549 | 3374.436 | 1.335 | 19.28× |
| parallel | 4 | playwright-shell | pass | yes | 637.206 | 1153.913 | 6.004 | 4.23× |
| parallel | 4 | puppeteer-chrome | fail | no | 710.598 | 5424.24 | 1.443 | N/A |
| parallel | 4 | shotium | pass | yes | 150.618 | 429.209 | 19.970 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 3039.735 | 5108.082 | 1.307 | 20.18× |
| parallel | 4 | puppeteer-shell | pass | yes | 877.135 | 1654.596 | 4.504 | 5.82× |
| reuse-page | 1 | puppeteer-shell | pass | no | 241.874 | 348.73 | 4.463 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 250.342 | 539.328 | 3.552 | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 225.846 | 382.407 | 4.393 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 1565 | 2304 | 0.589 | 1.82× |
| resident | 1 | playwright-chrome | pass | yes | 2760 | 3008 | 0.357 | 3.20× |
| resident | 1 | playwright-shell | pass | yes | 3314 | 5453 | 0.318 | 3.84× |
| resident | 1 | shotium | pass | yes | 862 | 1602 | 1.025 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 1905 | 2508 | 0.520 | 2.21× |
| faults | 1 | playwright-shell | pass | no | 55515.677 | 55515.677 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9440.528 | 9440.528 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 30950.043 | 30950.043 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 56925.068 | 56925.068 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 48956.448 | 48956.448 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 1142.616 | 3192.553 | 3.272 | 4.54× |
| soak | 4 | puppeteer-shell | pass | yes | 899.186 | 3328.182 | 4.191 | 3.57× |
| soak | 4 | shotium | pass | yes | 251.599 | 693.032 | 14.120 | 1.00× |
| soak | 4 | playwright-chrome | fail | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 8.118× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 10.654× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 15.367× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 18.082× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.7.0/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | pass | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | fail | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 1892 | 5478 | 0.423 | 27.42× |
| cold | 1 | puppeteer-shell | pass | yes | 758 | 1023 | 1.273 | 10.99× |
| cold | 1 | playwright-shell | pass | yes | 771 | 964 | 1.328 | 11.17× |
| cold | 1 | playwright-chrome | pass | yes | 1991 | 4249 | 0.424 | 28.86× |
| cold | 1 | shotium | pass | yes | 69 | 98 | 14.228 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 10.924 | 28.648 | 67.084 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 194.41 | 263.258 | 5.004 | 17.80× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 379.11 | 492.553 | 2.616 | 34.70× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 266.561 | 371.917 | 3.622 | 24.40× |
| cold-settled | 1 | playwright-shell | pass | yes | 139.324 | 173.097 | 6.998 | 12.75× |
| lifecycle | 1 | shotium | pass | yes | 48.566 | 110.633 | 18.519 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 672.444 | 1172.504 | 1.385 | 13.85× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1455.467 | 2248.527 | 0.654 | 29.97× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1842.794 | 2543.012 | 0.562 | 37.94× |
| lifecycle | 1 | playwright-shell | pass | yes | 544.479 | 1092.108 | 1.696 | 11.21× |
| warm | 1 | playwright-chrome | pass | yes | 280.528 | 639.086 | 3.198 | 25.08× |
| warm | 1 | shotium | pass | yes | 11.185 | 27.115 | 69.993 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 264.782 | 486.834 | 3.342 | 23.67× |
| warm | 1 | puppeteer-chrome | pass | yes | 407.757 | 681.428 | 2.323 | 36.46× |
| warm | 1 | playwright-shell | pass | yes | 184.616 | 349.883 | 4.887 | 16.51× |
| batch | 1 | playwright-chrome | pass | yes | 246.188 | 1844.173 | 3.123 | 20.08× |
| batch | 1 | playwright-shell | pass | yes | 188.139 | 433.076 | 4.758 | 15.34× |
| batch | 1 | puppeteer-chrome | pass | yes | 359.024 | 1590.11 | 2.450 | 29.28× |
| batch | 1 | puppeteer-shell | pass | yes | 308.547 | 645.348 | 3.109 | 25.16× |
| batch | 1 | shotium | pass | yes | 12.262 | 263.125 | 31.426 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 222.56 | 1585.362 | 3.460 | 13.45× |
| parallel | 1 | puppeteer-shell | pass | yes | 215.208 | 538.816 | 4.362 | 13.01× |
| parallel | 1 | playwright-shell | pass | yes | 174.811 | 425.073 | 5.065 | 10.56× |
| parallel | 1 | puppeteer-chrome | pass | yes | 351.951 | 1599.396 | 2.433 | 21.27× |
| parallel | 1 | shotium | pass | yes | 16.547 | 275.322 | 26.897 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 322.804 | 652.922 | 5.743 | 8.52× |
| parallel | 2 | playwright-shell | pass | yes | 195.905 | 442.88 | 9.003 | 5.17× |
| parallel | 2 | puppeteer-chrome | pass | yes | 491.278 | 1739.03 | 3.474 | 12.97× |
| parallel | 2 | shotium | pass | yes | 37.88 | 285.242 | 28.620 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 349.678 | 1021.954 | 5.010 | 9.23× |
| parallel | 4 | playwright-shell | pass | yes | 368.977 | 775.84 | 9.562 | 4.94× |
| parallel | 4 | puppeteer-chrome | pass | yes | 535.954 | 30117.263 | 3.254 | 7.18× |
| parallel | 4 | shotium | pass | yes | 74.663 | 321.126 | 29.786 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 680.893 | 3534.967 | 5.313 | 9.12× |
| parallel | 4 | puppeteer-shell | pass | yes | 400.595 | 683.124 | 9.518 | 5.37× |
| reuse-page | 1 | puppeteer-shell | pass | no | 194.95 | 293.398 | 4.922 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 232.425 | 302.889 | 4.322 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 101.756 | 137.637 | 9.686 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 89.909 | 132.831 | 10.421 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 1097 | 1640 | 0.827 | 3.43× |
| resident | 1 | playwright-chrome | pass | yes | 1376 | 1928 | 0.767 | 4.30× |
| resident | 1 | playwright-shell | pass | yes | 753 | 900 | 1.287 | 2.35× |
| resident | 1 | shotium | pass | yes | 320 | 352 | 3.069 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 825 | 1147 | 1.113 | 2.58× |
| faults | 1 | playwright-shell | pass | no | 16532.578 | 16532.578 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6097.493 | 6097.493 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 12908 | 12908 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 31340.846 | 31340.846 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 23814.672 | 23814.672 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 424.094 | 1083.367 | 9.012 | 4.84× |
| soak | 4 | puppeteer-shell | pass | yes | 487.574 | 925.901 | 7.944 | 5.56× |
| soak | 4 | shotium | pass | yes | 87.678 | 584.17 | 27.312 | 1.00× |
| soak | 4 | playwright-chrome | fail | no | 571.244 | 6879.116 | 6.431 | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 476.46 | 24533.61 | 3.790 | N/A |

