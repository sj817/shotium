# Shotium 0.11.1 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `8a4c6cf601d293d6e269dccca6c514e8f31e376f`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 3 | 6 |
| win32-arm64 | pass | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 4 | 9 |
| darwin-arm64 | noisy | shotium | 4 | 8 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 7.459× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 8.058× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 10.793× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 9.336× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.11.1/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 67 | 71 | 14.894 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 910 | 926 | 1.098 | 13.58× |
| cold | 1 | playwright-shell | pass | yes | 787 | 804 | 1.266 | 11.75× |
| cold | 1 | playwright-chrome | pass | yes | 980 | 1013 | 1.020 | 14.63× |
| cold | 1 | puppeteer-shell | pass | yes | 647 | 672 | 1.537 | 9.66× |
| cold-settled | 1 | playwright-shell | pass | yes | 146.793 | 162.351 | 6.764 | 9.44× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 177.752 | 191.183 | 5.698 | 11.43× |
| cold-settled | 1 | playwright-chrome | pass | yes | 170.862 | 187.904 | 5.840 | 10.99× |
| cold-settled | 1 | shotium | pass | yes | 15.547 | 16.561 | 64.445 | 1.00× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 127.653 | 143.977 | 7.528 | 8.21× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 673.671 | 754.017 | 1.478 | 10.22× |
| lifecycle | 1 | shotium | pass | yes | 65.923 | 118.366 | 13.177 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 981.575 | 1070.636 | 1.014 | 14.89× |
| lifecycle | 1 | playwright-shell | pass | yes | 703.938 | 770.497 | 1.407 | 10.68× |
| lifecycle | 1 | playwright-chrome | fail | no | 2182.51 | 2911.519 | N/A | N/A |
| warm | 1 | puppeteer-chrome | pass | yes | 186.505 | 204.187 | 5.279 | 18.30× |
| warm | 1 | playwright-chrome | pass | yes | 171.421 | 198.794 | 5.683 | 16.82× |
| warm | 1 | puppeteer-shell | pass | yes | 133.272 | 151.729 | 7.246 | 13.08× |
| warm | 1 | playwright-shell | pass | yes | 147.021 | 159.308 | 6.864 | 14.43× |
| warm | 1 | shotium | pass | yes | 10.191 | 19.666 | 84.248 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 156.313 | 389.445 | 5.395 | 11.96× |
| batch | 1 | shotium | pass | yes | 13.07 | 256.089 | 28.466 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 151.448 | 386.866 | 5.590 | 11.59× |
| batch | 1 | puppeteer-chrome | pass | yes | 227.904 | 438.326 | 4.003 | 17.44× |
| batch | 1 | playwright-chrome | pass | yes | 202.411 | 404.536 | 4.360 | 15.49× |
| parallel | 1 | puppeteer-chrome | pass | yes | 245.184 | 471.057 | 3.809 | 17.39× |
| parallel | 1 | playwright-shell | pass | yes | 164.294 | 390.879 | 5.206 | 11.65× |
| parallel | 1 | puppeteer-shell | pass | yes | 151.919 | 388.468 | 5.516 | 10.78× |
| parallel | 1 | shotium | pass | yes | 14.099 | 258.31 | 27.506 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 209.354 | 420.036 | 4.375 | 14.85× |
| parallel | 2 | playwright-shell | pass | yes | 257.154 | 457.208 | 7.218 | 6.89× |
| parallel | 2 | puppeteer-shell | pass | yes | 240.378 | 500.207 | 7.254 | 6.44× |
| parallel | 2 | shotium | pass | yes | 37.317 | 285.784 | 28.407 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 302.916 | 592.652 | 6.106 | 8.12× |
| parallel | 2 | puppeteer-chrome | pass | yes | 376.23 | 647.107 | 4.913 | 10.08× |
| parallel | 4 | puppeteer-shell | pass | yes | 447.852 | 747.31 | 8.573 | 4.68× |
| parallel | 4 | shotium | pass | yes | 95.669 | 317.128 | 28.102 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 527.017 | 860.335 | 7.183 | 5.51× |
| parallel | 4 | puppeteer-chrome | pass | yes | 706.94 | 1111.679 | 5.585 | 7.39× |
| parallel | 4 | playwright-shell | pass | yes | 454.631 | 753.383 | 8.170 | 4.75× |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.105 | 110.863 | 9.857 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 100.129 | 110.567 | 9.852 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.992 | 101.32 | 10.069 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 100.203 | 118.396 | 9.464 | N/A |
| resident | 1 | playwright-shell | pass | yes | 987 | 1055 | 1.103 | 2.96× |
| resident | 1 | playwright-chrome | pass | yes | 1051 | 1094 | 1.066 | 3.15× |
| resident | 1 | puppeteer-shell | pass | yes | 900 | 959 | 1.113 | 2.69× |
| resident | 1 | shotium | pass | yes | 334 | 432 | 3.125 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 938 | 1023 | 1.119 | 2.81× |
| faults | 1 | shotium | pass | no | 5241.928 | 5241.928 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16567.761 | 16567.761 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 17371.405 | 17371.405 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10911.433 | 10911.433 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 12240.921 | 12240.921 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 88.816 | 358.542 | 29.358 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 440.615 | 839.652 | 8.736 | 4.96× |
| soak | 4 | playwright-chrome | pass | yes | 547.056 | 978.373 | 7.062 | 6.16× |
| soak | 4 | playwright-shell | pass | yes | 444.797 | 943.87 | 8.394 | 5.01× |
| soak | 4 | puppeteer-chrome | pass | yes | 709.41 | 1233.738 | 5.502 | 7.99× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.195× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.059× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.11.1/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 56 | 58 | 17.949 | 1.00× |
| cold | 1 | playwright-shell | pass | yes | 649 | 661 | 1.548 | 11.59× |
| cold | 1 | playwright-chrome | pass | yes | 812 | 836 | 1.224 | 14.50× |
| cold-settled | 1 | shotium | pass | yes | 21.569 | 22.558 | 45.742 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 173.118 | 189.313 | 5.836 | 8.03× |
| cold-settled | 1 | playwright-shell | pass | yes | 129.379 | 147.144 | 7.365 | 6.00× |
| lifecycle | 1 | shotium | pass | yes | 62.245 | 77.231 | 15.678 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 1529.429 | 1746.478 | N/A | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 583.257 | 671.709 | 1.679 | 9.37× |
| warm | 1 | playwright-chrome | pass | yes | 159.856 | 169.435 | 6.344 | 8.18× |
| warm | 1 | playwright-shell | pass | yes | 132.509 | 145.47 | 7.715 | 6.78× |
| warm | 1 | shotium | pass | yes | 19.545 | 23.526 | 49.610 | 1.00× |
| batch | 1 | shotium | pass | yes | 22.093 | 257.771 | 24.361 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 143.085 | 401.786 | 5.961 | 6.48× |
| batch | 1 | playwright-chrome | pass | yes | 171.96 | 405.817 | 5.025 | 7.78× |
| parallel | 1 | playwright-shell | pass | yes | 145.634 | 383.284 | 5.740 | 5.95× |
| parallel | 1 | playwright-chrome | pass | yes | 189.459 | 438.635 | 4.708 | 7.75× |
| parallel | 1 | shotium | pass | yes | 24.458 | 264.088 | 23.488 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 261.852 | 525.634 | 6.859 | 5.17× |
| parallel | 2 | shotium | pass | yes | 50.689 | 291.298 | 23.418 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 203.66 | 446.902 | 8.536 | 4.02× |
| parallel | 4 | shotium | pass | yes | 111.057 | 365.949 | 23.470 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 363.381 | 579.82 | 10.465 | 3.27× |
| parallel | 4 | playwright-chrome | pass | yes | 455.996 | 741.733 | 8.406 | 4.11× |
| reuse-page | 1 | playwright-shell | pass | no | 99.933 | 105.63 | 10.001 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 99.967 | 115.928 | 9.884 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 983 | 1030 | 1.020 | 2.26× |
| resident | 1 | playwright-shell | pass | yes | 900 | 921 | 1.110 | 2.07× |
| resident | 1 | shotium | pass | yes | 434 | 460 | 2.288 | 1.00× |
| faults | 1 | shotium | pass | no | 4982.739 | 4982.739 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 17336 | 17336 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 14576.279 | 14576.279 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 440.63 | 779.609 | 8.703 | 3.99× |
| soak | 4 | playwright-shell | pass | yes | 340.652 | 674.167 | 10.835 | 3.09× |
| soak | 4 | shotium | pass | yes | 110.411 | 375.651 | 23.874 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 6 eligible cell(s), with 6 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 6 / 6 | 6 |
| 2 | playwright-shell | 12.800× | 6 | 6 / 6 | 0 |
| 3 | playwright-chrome | 14.172× | 6 | 6 / 6 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 14.942× | 5 | 5 / 6 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 16.806× | 5 | 5 / 6 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `parallel/c1`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `cold/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.11.1\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 47 | 51 | 21.084 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 1166 | 1750 | 0.790 | 24.81× |
| cold | 1 | playwright-shell | pass | yes | 859 | 1105 | 1.105 | 18.28× |
| cold | 1 | playwright-chrome | pass | yes | 1136 | 1370 | 0.845 | 24.17× |
| cold | 1 | puppeteer-shell | pass | yes | 979 | 1392 | 0.929 | 20.83× |
| cold-settled | 1 | playwright-shell | noisy | no | 164.47 | 177.174 | 6.177 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 225.139 | 225.139 | 0.022 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 206.712 | 214.891 | 0.199 | N/A |
| cold-settled | 1 | shotium | noisy | no | 11.611 | 12.157 | 0.166 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 158.982 | 169.598 | 0.376 | N/A |
| lifecycle | 1 | puppeteer-shell | noisy | no | 1135.648 | 1322.15 | 0.877 | N/A |
| lifecycle | 1 | shotium | noisy | no | 77.657 | 115.683 | 11.878 | N/A |
| lifecycle | 1 | puppeteer-chrome | noisy | no | 2386.716 | 3109.042 | 0.408 | N/A |
| lifecycle | 1 | playwright-shell | pass | no | 943.156 | 1526.157 | 1.055 | N/A |
| lifecycle | 1 | playwright-chrome | fail | no | 6717.429 | 6902.891 | N/A | N/A |
| warm | 1 | puppeteer-chrome | pass | yes | 167.486 | 232.377 | 5.674 | 25.77× |
| warm | 1 | playwright-chrome | pass | yes | 135.118 | 151.312 | 7.201 | 20.79× |
| warm | 1 | puppeteer-shell | pass | yes | 134.182 | 163.444 | 7.163 | 20.65× |
| warm | 1 | playwright-shell | pass | yes | 140.894 | 173.748 | 7.047 | 21.68× |
| warm | 1 | shotium | pass | yes | 6.499 | 9.652 | 131.386 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 149.062 | 3179.461 | 4.239 | 13.23× |
| batch | 1 | shotium | pass | yes | 11.271 | 270.97 | 30.455 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 153.122 | 392.9 | 5.522 | 13.59× |
| batch | 1 | puppeteer-chrome | noisy | no | 9953.177 | 9953.177 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | yes | 157.495 | 394.067 | 5.462 | 13.97× |
| parallel | 1 | puppeteer-chrome | pass | yes | 261.252 | 484.382 | 3.555 | 17.86× |
| parallel | 1 | playwright-shell | pass | yes | 193.167 | 438.393 | 4.512 | 13.21× |
| parallel | 1 | puppeteer-shell | pass | yes | 182.45 | 410.596 | 4.815 | 12.47× |
| parallel | 1 | shotium | pass | yes | 14.626 | 288.197 | 26.461 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 197.744 | 428.88 | 4.515 | 13.52× |
| parallel | 2 | playwright-shell | pass | no | 255.381 | 524.477 | 7.095 | N/A |
| parallel | 2 | puppeteer-shell | pass | no | 267.598 | 463.947 | 6.690 | N/A |
| parallel | 2 | shotium | noisy | no | 7209.865 | 7209.865 | N/A | N/A |
| parallel | 2 | playwright-chrome | pass | no | 333.154 | 845.728 | 5.361 | N/A |
| parallel | 2 | puppeteer-chrome | pass | no | 455.917 | 744.603 | 4.161 | N/A |
| parallel | 4 | puppeteer-shell | pass | no | 459.191 | 799.769 | 8.199 | N/A |
| parallel | 4 | shotium | noisy | no | 7792.652 | 7792.652 | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | 9030.629 | 9030.629 | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | 15185.857 | 15185.857 | N/A | N/A |
| parallel | 4 | playwright-shell | pass | no | 529.383 | 1008.718 | 7.285 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.856 | 116.642 | 9.549 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 100.441 | 127.037 | 9.652 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 117.881 | 147.545 | 7.994 | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | 10984.233 | 10984.233 | N/A | N/A |
| resident | 1 | playwright-shell | pass | yes | 801 | 1154 | 1.249 | 12.52× |
| resident | 1 | playwright-chrome | pass | yes | 829 | 1181 | 1.116 | 12.95× |
| resident | 1 | puppeteer-shell | pass | yes | 654 | 1259 | 1.230 | 10.22× |
| resident | 1 | shotium | pass | yes | 64 | 498 | 5.848 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 804 | 1192 | 1.148 | 12.56× |
| faults | 1 | shotium | pass | no | 10701.044 | 10701.044 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 27574.886 | 27574.886 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 29939.55 | 29939.55 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 30854.605 | 30854.605 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 30884.139 | 30884.139 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 97.821 | 377.513 | 27.231 | 1.00× |
| soak | 4 | puppeteer-shell | noisy | no | 9402.797 | 9402.797 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 644.358 | 2919.003 | 5.706 | 6.59× |
| soak | 4 | playwright-shell | pass | yes | 496.696 | 1176.688 | 7.742 | 5.08× |
| soak | 4 | puppeteer-chrome | pass | yes | 914.271 | 1611.102 | 4.314 | 9.35× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.11.1\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 47 | 53 | 20.958 | N/A |
| cold-settled | 1 | shotium | pass | no | 8.644 | 9.368 | 111.980 | N/A |
| lifecycle | 1 | shotium | pass | no | 68.42 | 85.37 | 13.952 | N/A |
| warm | 1 | shotium | pass | no | 7.335 | 11.193 | 117.027 | N/A |
| batch | 1 | shotium | pass | no | 14.343 | 267.836 | 26.120 | N/A |
| parallel | 1 | shotium | pass | no | 14.634 | 274.885 | 26.296 | N/A |
| parallel | 2 | shotium | pass | no | 38.189 | 287.324 | 28.148 | N/A |
| parallel | 4 | shotium | pass | no | 92.889 | 337.788 | 27.836 | N/A |
| resident | 1 | shotium | pass | no | 310 | 891 | 2.366 | N/A |
| faults | 1 | shotium | pass | no | 10841.891 | 10841.891 | N/A | N/A |
| soak | 4 | shotium | pass | no | 75.931 | 369.73 | 30.396 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 10.611× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 10.724× | 7 | 9 / 9 | 0 |
| 4 | puppeteer-chrome | 16.832× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 30.104× | 5 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.11.1/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | yes | 72 | 246 | 10.189 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 2044 | 2597 | 0.468 | 28.39× |
| cold | 1 | playwright-shell | pass | yes | 1112 | 1598 | 0.831 | 15.44× |
| cold | 1 | playwright-chrome | pass | yes | 2986 | 3331 | 0.333 | 41.47× |
| cold | 1 | puppeteer-shell | pass | yes | 1129 | 1286 | 0.879 | 15.68× |
| cold-settled | 1 | playwright-shell | pass | yes | 483.921 | 585.481 | 2.280 | 27.80× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 466.736 | 561.646 | 2.199 | 26.81× |
| cold-settled | 1 | playwright-chrome | pass | yes | 899.186 | 1705.969 | 0.922 | 51.65× |
| cold-settled | 1 | shotium | pass | yes | 17.408 | 28.284 | 54.684 | 1.00× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 351.944 | 790.702 | 2.347 | 20.22× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1875.654 | 2367.073 | 0.530 | 15.61× |
| lifecycle | 1 | shotium | pass | yes | 120.142 | 207.238 | 7.981 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3517.775 | 4653.14 | 0.281 | 29.28× |
| lifecycle | 1 | playwright-shell | pass | yes | 1475.072 | 2969.249 | 0.622 | 12.28× |
| lifecycle | 1 | playwright-chrome | fail | no | 6880.155 | 7753.519 | N/A | N/A |
| warm | 1 | puppeteer-chrome | pass | no | 439.965 | 739.643 | 2.286 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 783.058 | 940.765 | 0.443 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 349.548 | 415.574 | 0.854 | N/A |
| warm | 1 | playwright-shell | noisy | no | 284.854 | 717.916 | 1.049 | N/A |
| warm | 1 | shotium | noisy | no | 7.823 | 13.596 | 1.419 | N/A |
| batch | 1 | playwright-shell | pass | yes | 316.151 | 686.493 | 2.976 | 20.25× |
| batch | 1 | shotium | pass | yes | 15.614 | 257.681 | 25.092 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 339.833 | 631.716 | 2.908 | 21.76× |
| batch | 1 | puppeteer-chrome | pass | yes | 437.921 | 875.333 | 2.219 | 28.05× |
| batch | 1 | playwright-chrome | pass | yes | 863.292 | 1246.361 | 1.143 | 55.29× |
| parallel | 1 | puppeteer-chrome | pass | yes | 441.347 | 1180.903 | 2.070 | 30.02× |
| parallel | 1 | playwright-shell | pass | yes | 311.686 | 705.761 | 3.080 | 21.20× |
| parallel | 1 | puppeteer-shell | pass | yes | 343.547 | 691.623 | 2.803 | 23.37× |
| parallel | 1 | shotium | pass | yes | 14.7 | 257.84 | 28.366 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 865.735 | 1184.114 | 1.150 | 58.89× |
| parallel | 2 | playwright-shell | pass | yes | 307.645 | 612.007 | 5.810 | 7.78× |
| parallel | 2 | puppeteer-shell | pass | yes | 388.044 | 639.562 | 5.018 | 9.81× |
| parallel | 2 | shotium | pass | yes | 39.563 | 272.181 | 27.120 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 1513.766 | 2346.627 | 1.350 | 38.26× |
| parallel | 2 | puppeteer-chrome | pass | yes | 613.504 | 2184.374 | 2.956 | 15.51× |
| parallel | 4 | puppeteer-shell | pass | yes | 584.788 | 1335.787 | 6.181 | 5.83× |
| parallel | 4 | shotium | pass | yes | 100.331 | 337.443 | 26.649 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 2940.65 | 5163.259 | 1.381 | 29.31× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1121.26 | 2834.987 | 3.367 | 11.18× |
| parallel | 4 | playwright-shell | pass | yes | 616.42 | 1101.264 | 6.160 | 6.14× |
| reuse-page | 1 | puppeteer-shell | noisy | no | 12169.862 | 12169.862 | N/A | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 200.409 | 254.301 | 5.415 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 251.323 | 349.944 | 3.753 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 253.155 | 439.098 | 3.796 | N/A |
| resident | 1 | playwright-shell | pass | yes | 2410 | 2725 | 0.424 | 2.46× |
| resident | 1 | playwright-chrome | pass | yes | 2810 | 2945 | 0.361 | 2.86× |
| resident | 1 | puppeteer-shell | pass | yes | 2037 | 2487 | 0.481 | 2.08× |
| resident | 1 | shotium | pass | yes | 981 | 1078 | 1.018 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 2216 | 2655 | 0.441 | 2.26× |
| faults | 1 | shotium | pass | no | 9608.686 | 9608.686 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 33328.053 | 33328.053 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 42262.957 | 42262.957 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 28891.764 | 28891.764 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 32278.968 | 32278.968 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 107.873 | 514.568 | 25.245 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 677.108 | 2173.945 | 5.607 | 6.28× |
| soak | 4 | playwright-chrome | fail | no | 2992.048 | 6312.057 | 1.337 | N/A |
| soak | 4 | playwright-shell | pass | yes | 692.378 | 1276.904 | 5.643 | 6.42× |
| soak | 4 | puppeteer-chrome | pass | yes | 1592.187 | 4102.803 | 2.442 | 14.76× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 8 / 8 | 8 |
| 2 | playwright-shell | 10.432× | 6 | 8 / 8 | 0 |
| 3 | puppeteer-shell | 13.784× | 6 | 8 / 8 | 0 |
| 4 | puppeteer-chrome | 22.246× | 6 | 8 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 13.679× | 5 | 7 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.11.1/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | noisy | no | 76 | 108 | 12.618 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 4192 | 6299 | 0.239 | N/A |
| cold | 1 | playwright-shell | noisy | no | 1159 | 1489 | 0.818 | N/A |
| cold | 1 | playwright-chrome | noisy | no | 3768 | 4409 | 0.265 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 1730 | 1864 | 0.631 | N/A |
| cold-settled | 1 | playwright-shell | pass | yes | 149.639 | 200.021 | 6.285 | 19.89× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 327.657 | 382.001 | 3.037 | 43.56× |
| cold-settled | 1 | playwright-chrome | pass | yes | 207.578 | 276.544 | 4.408 | 27.60× |
| cold-settled | 1 | shotium | pass | yes | 7.522 | 11.197 | 122.092 | 1.00× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 296.505 | 375.871 | 3.394 | 39.42× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 640.264 | 1075.594 | 1.451 | 13.86× |
| lifecycle | 1 | shotium | pass | yes | 46.21 | 85.052 | 21.145 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1592.261 | 2423.322 | 0.605 | 34.46× |
| lifecycle | 1 | playwright-shell | pass | yes | 665.896 | 1280.878 | 1.401 | 14.41× |
| lifecycle | 1 | playwright-chrome | fail | no | 3340.518 | 4953.555 | N/A | N/A |
| warm | 1 | puppeteer-chrome | pass | yes | 391.462 | 689.554 | 2.310 | 50.94× |
| warm | 1 | playwright-chrome | pass | yes | 208.783 | 296.188 | 4.788 | 27.17× |
| warm | 1 | puppeteer-shell | pass | yes | 230.016 | 340.94 | 4.362 | 29.93× |
| warm | 1 | playwright-shell | pass | yes | 167.214 | 214.655 | 5.961 | 21.76× |
| warm | 1 | shotium | pass | yes | 7.685 | 12.314 | 118.643 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 177.061 | 427.118 | 4.877 | 9.65× |
| batch | 1 | shotium | pass | yes | 18.346 | 353.497 | 22.273 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 310.737 | 665.477 | 3.109 | 16.94× |
| batch | 1 | puppeteer-chrome | pass | yes | 397.143 | 1509.808 | 2.222 | 21.65× |
| batch | 1 | playwright-chrome | pass | yes | 208.836 | 1458.911 | 3.652 | 11.38× |
| parallel | 1 | puppeteer-chrome | pass | yes | 439.75 | 2089.844 | 1.965 | 38.90× |
| parallel | 1 | playwright-shell | pass | yes | 185.719 | 422.034 | 4.783 | 16.43× |
| parallel | 1 | puppeteer-shell | pass | yes | 294.739 | 727.684 | 3.247 | 26.07× |
| parallel | 1 | shotium | pass | yes | 11.304 | 266.638 | 31.180 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 253.868 | 1735.909 | 3.091 | 22.46× |
| parallel | 2 | playwright-shell | pass | yes | 222.154 | 522.999 | 7.984 | 6.18× |
| parallel | 2 | puppeteer-shell | pass | yes | 379.193 | 625.515 | 5.196 | 10.55× |
| parallel | 2 | shotium | pass | yes | 35.941 | 274.149 | 29.040 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 424.915 | 2014.672 | 3.328 | 11.82× |
| parallel | 2 | puppeteer-chrome | pass | yes | 688.3 | 1895.333 | 2.711 | 19.15× |
| parallel | 4 | puppeteer-shell | pass | yes | 553.241 | 1198.802 | 6.877 | 6.62× |
| parallel | 4 | shotium | pass | yes | 83.558 | 326.874 | 28.881 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 1039.019 | 3390.915 | 3.485 | 12.43× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1317.457 | 2718.191 | 2.793 | 15.77× |
| parallel | 4 | playwright-shell | pass | yes | 570.788 | 1208.157 | 6.698 | 6.83× |
| reuse-page | 1 | puppeteer-shell | pass | no | 272.562 | 367.738 | 3.598 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 103.037 | 133.487 | 9.330 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 277.919 | 396.12 | 3.446 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 117.151 | 128.982 | 8.686 | N/A |
| resident | 1 | playwright-shell | pass | yes | 1149 | 1389 | 0.856 | 3.36× |
| resident | 1 | playwright-chrome | pass | yes | 1088 | 1713 | 0.857 | 3.18× |
| resident | 1 | puppeteer-shell | pass | yes | 884 | 1054 | 1.131 | 2.58× |
| resident | 1 | shotium | pass | yes | 342 | 351 | 2.971 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 1055 | 1493 | 0.928 | 3.08× |
| faults | 1 | shotium | pass | no | 6296.501 | 6296.501 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 20850.318 | 20850.318 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 26057.155 | 26057.155 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 11428.316 | 11428.316 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 29825.657 | 29825.657 | N/A | N/A |
| soak | 4 | shotium | pass | no | 75.757 | 388.274 | 30.020 | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |

