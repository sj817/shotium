# Shotium 0.3.4 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `6bf4cb2fbc8e69e59371281916ce6bf49a73ef24`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | noisy | shotium | 3 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 2 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 2 | 10 |
| darwin-arm64 | noisy | shotium | 2 | 7 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.491× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 5.731× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 7.025× | 7 | 7 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 9.918× | 6 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.3.4/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | noisy | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 968 | 1036 | 1.020 | 15.61× |
| cold | 1 | playwright-shell | pass | yes | 794 | 851 | 1.263 | 12.81× |
| cold | 1 | puppeteer-chrome | pass | yes | 909 | 969 | 1.095 | 14.66× |
| cold | 1 | puppeteer-shell | pass | yes | 672 | 750 | 1.477 | 10.84× |
| cold | 1 | shotium | pass | yes | 62 | 63 | 16.279 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 172.684 | 185.702 | 5.971 | 11.11× |
| cold-settled | 1 | playwright-chrome | noisy | no | 142.719 | 166.006 | 1.206 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | yes | 131.473 | 142.623 | 7.478 | 8.46× |
| cold-settled | 1 | shotium | pass | yes | 15.539 | 15.881 | 63.974 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 107.712 | 120.698 | 8.967 | 6.93× |
| lifecycle | 1 | playwright-shell | pass | yes | 708.004 | 790.594 | 1.409 | 11.45× |
| lifecycle | 1 | playwright-chrome | pass | yes | 911.034 | 984.587 | 1.097 | 14.74× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 690.552 | 746.291 | 1.450 | 11.17× |
| lifecycle | 1 | shotium | pass | yes | 61.82 | 115.938 | 13.948 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1005.257 | 1049.929 | 1.007 | 16.26× |
| warm | 1 | playwright-chrome | pass | yes | 133.579 | 151.39 | 7.442 | 9.65× |
| warm | 1 | shotium | pass | yes | 13.845 | 16.466 | 68.832 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 109.235 | 150.073 | 8.925 | 7.89× |
| warm | 1 | puppeteer-shell | pass | yes | 117.239 | 134.677 | 8.190 | 8.47× |
| warm | 1 | puppeteer-chrome | pass | yes | 151.044 | 175.816 | 6.522 | 10.91× |
| batch | 1 | shotium | pass | yes | 21.697 | 263.206 | 22.604 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 138.271 | 353.329 | 6.013 | 6.37× |
| batch | 1 | playwright-shell | pass | yes | 128.469 | 348.021 | 6.466 | 5.92× |
| batch | 1 | puppeteer-chrome | pass | yes | 185.889 | 372.559 | 4.959 | 8.57× |
| batch | 1 | playwright-chrome | pass | yes | 158.695 | 378.683 | 5.396 | 7.31× |
| parallel | 1 | playwright-chrome | noisy | no | 4189.392 | 4189.392 | N/A | N/A |
| parallel | 1 | puppeteer-shell | pass | yes | 147.899 | 366.559 | 5.711 | 5.53× |
| parallel | 1 | puppeteer-chrome | pass | yes | 208.991 | 413.412 | 4.442 | 7.81× |
| parallel | 1 | shotium | pass | yes | 26.75 | 276.244 | 20.482 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 139.799 | 357.667 | 6.103 | 5.23× |
| parallel | 2 | puppeteer-shell | pass | yes | 236.142 | 504.485 | 7.341 | 3.64× |
| parallel | 2 | puppeteer-chrome | pass | yes | 316.404 | 623.419 | 5.849 | 4.88× |
| parallel | 2 | shotium | pass | yes | 64.846 | 302.519 | 21.841 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 232.411 | 464.795 | 7.769 | 3.58× |
| parallel | 2 | playwright-chrome | noisy | no | 4264.263 | 4264.263 | N/A | N/A |
| parallel | 4 | puppeteer-chrome | fail | no | 593.878 | 1050.863 | 6.388 | N/A |
| parallel | 4 | shotium | pass | yes | 138.513 | 395.673 | 21.310 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 411.003 | 831.16 | 8.622 | 2.97× |
| parallel | 4 | playwright-chrome | pass | yes | 544.743 | 898.582 | 6.948 | 3.93× |
| parallel | 4 | puppeteer-shell | pass | yes | 462.198 | 726.825 | 8.210 | 3.34× |
| reuse-page | 1 | playwright-shell | pass | no | 65.349 | 83.366 | 16.063 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 67.988 | 93.244 | 13.742 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 67.087 | 84.127 | 13.852 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 83.3 | 99.106 | 11.902 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 914 | 949 | 1.094 | 3.39× |
| resident | 1 | shotium | pass | yes | 270 | 456 | 3.848 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 995 | 1036 | 1.005 | 3.69× |
| resident | 1 | playwright-chrome | pass | yes | 1059 | 1077 | 1.044 | 3.92× |
| resident | 1 | puppeteer-chrome | noisy | no | 19433.159 | 19433.159 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10628.441 | 10628.441 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 15589.48 | 15589.48 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9188.008 | 9188.008 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13698.483 | 13698.483 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 15492.227 | 15492.227 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 279.597 | 634.612 | 12.661 | 2.56× |
| soak | 4 | puppeteer-chrome | fail | no | 448.264 | 875.429 | 5.756 | N/A |
| soak | 4 | playwright-chrome | pass | yes | 368.09 | 703.979 | 10.127 | 3.37× |
| soak | 4 | puppeteer-shell | pass | yes | 331.751 | 658.67 | 11.245 | 3.04× |
| soak | 4 | shotium | pass | yes | 109.155 | 354.266 | 25.988 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.051× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 3.742× | 8 | 10 / 10 | 0 |

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
| cold | 1 | playwright-chrome | pass | yes | 809 | 878 | 1.219 | 12.64× |
| cold | 1 | playwright-shell | pass | yes | 629 | 660 | 1.592 | 9.83× |
| cold | 1 | shotium | pass | yes | 64 | 66 | 15.590 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 102.346 | 114.671 | 9.792 | 3.02× |
| cold-settled | 1 | playwright-chrome | pass | yes | 124.565 | 137.03 | 8.247 | 3.68× |
| cold-settled | 1 | shotium | pass | yes | 33.835 | 34.295 | 29.369 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 551.892 | 602.393 | 1.795 | 7.70× |
| lifecycle | 1 | playwright-chrome | pass | yes | 707.491 | 813.344 | 1.388 | 9.87× |
| lifecycle | 1 | shotium | pass | yes | 71.667 | 97.458 | 13.180 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 122.415 | 152.03 | 7.856 | 3.86× |
| warm | 1 | playwright-shell | pass | yes | 100.337 | 116.966 | 9.565 | 3.16× |
| warm | 1 | shotium | pass | yes | 31.713 | 34.55 | 30.534 | 1.00× |
| batch | 1 | shotium | pass | yes | 38.871 | 271.242 | 17.219 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 123.533 | 325.507 | 7.021 | 3.18× |
| batch | 1 | playwright-chrome | pass | yes | 145.796 | 355.259 | 5.969 | 3.75× |
| parallel | 1 | playwright-chrome | pass | yes | 144.787 | 399.275 | 5.850 | 3.72× |
| parallel | 1 | shotium | pass | yes | 38.932 | 262.003 | 17.595 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 119.428 | 323.616 | 7.194 | 3.07× |
| parallel | 2 | shotium | pass | yes | 84.419 | 298.727 | 17.813 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 165.027 | 383.756 | 10.481 | 1.95× |
| parallel | 2 | playwright-chrome | pass | yes | 210.864 | 453.134 | 8.635 | 2.50× |
| parallel | 4 | playwright-shell | pass | yes | 289.572 | 559.458 | 12.636 | 1.62× |
| parallel | 4 | playwright-chrome | pass | yes | 358.808 | 643.78 | 10.184 | 2.01× |
| parallel | 4 | shotium | pass | yes | 178.602 | 406.879 | 17.756 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 49.855 | 65.97 | 19.734 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 65.48 | 75.114 | 15.637 | N/A |
| resident | 1 | playwright-shell | pass | yes | 852 | 889 | 1.170 | 2.02× |
| resident | 1 | playwright-chrome | pass | yes | 882 | 945 | 1.185 | 2.10× |
| resident | 1 | shotium | pass | yes | 421 | 464 | 2.347 | 1.00× |
| faults | 1 | shotium | pass | no | 9706.891 | 9706.891 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16295.717 | 16295.717 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13203.769 | 13203.769 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 387.181 | 707.503 | 9.994 | 2.07× |
| soak | 4 | playwright-shell | pass | yes | 288.665 | 563.402 | 12.863 | 1.54× |
| soak | 4 | shotium | pass | yes | 186.93 | 457.482 | 17.158 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 6.005× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 7.532× | 6 | 8 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 8.082× | 4 | 5 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 13.416× | 3 | 3 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- puppeteer-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `warm/c1`
- puppeteer-chrome: `cold/c1`, `lifecycle/c1`, `parallel/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.3.4\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 1053 | 1588 | 0.885 | 15.26× |
| cold | 1 | playwright-shell | pass | yes | 839 | 926 | 1.177 | 12.16× |
| cold | 1 | puppeteer-chrome | pass | yes | 1078 | 1292 | 0.903 | 15.62× |
| cold | 1 | puppeteer-shell | pass | yes | 952 | 987 | 1.052 | 13.80× |
| cold | 1 | shotium | pass | yes | 69 | 71 | 14.403 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 159.636 | 202.475 | 0.680 | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 155.871 | 187.174 | 6.251 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 139.028 | 142.417 | 0.252 | N/A |
| cold-settled | 1 | shotium | noisy | no | 14.998 | 19.433 | 0.714 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 133.417 | 181.316 | 7.170 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 1006.62 | 1488.561 | 0.936 | 9.85× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1781.234 | 5072.738 | 0.482 | 17.43× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1287.509 | 5071.52 | 0.665 | 12.60× |
| lifecycle | 1 | shotium | pass | yes | 102.217 | 217.029 | 8.351 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2264.522 | 2878.358 | 0.436 | 22.15× |
| warm | 1 | playwright-chrome | pass | yes | 165.085 | 289.095 | 5.663 | 10.73× |
| warm | 1 | shotium | pass | yes | 15.389 | 17.098 | 62.137 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 147.496 | 175.76 | 6.745 | 9.58× |
| warm | 1 | puppeteer-shell | pass | yes | 166.182 | 244.811 | 5.684 | 10.80× |
| warm | 1 | puppeteer-chrome | noisy | no | 191.941 | 253.548 | 1.380 | N/A |
| batch | 1 | shotium | pass | yes | 27.927 | 323.931 | 18.280 | 1.00× |
| batch | 1 | puppeteer-shell | noisy | no | 5500.292 | 5500.292 | N/A | N/A |
| batch | 1 | playwright-shell | pass | yes | 182.115 | 407.74 | 4.849 | 6.52× |
| batch | 1 | puppeteer-chrome | noisy | no | 8714.042 | 8714.042 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | yes | 222.682 | 433.691 | 4.060 | 7.97× |
| parallel | 1 | playwright-chrome | pass | yes | 184.145 | 452.303 | 4.689 | 6.23× |
| parallel | 1 | puppeteer-shell | pass | yes | 177.114 | 389.066 | 4.927 | 5.99× |
| parallel | 1 | puppeteer-chrome | pass | yes | 206.181 | 465.408 | 4.280 | 6.98× |
| parallel | 1 | shotium | pass | yes | 29.554 | 278.655 | 20.433 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 167.543 | 421.987 | 5.128 | 5.67× |
| parallel | 2 | puppeteer-shell | noisy | no | 5600.089 | 5600.089 | N/A | N/A |
| parallel | 2 | puppeteer-chrome | fail | no | 364.73 | 30272.139 | 0.224 | N/A |
| parallel | 2 | shotium | pass | yes | 68.152 | 295.027 | 20.877 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 245.844 | 508.34 | 7.317 | 3.61× |
| parallel | 2 | playwright-chrome | pass | yes | 349.335 | 831.806 | 5.096 | 5.13× |
| parallel | 4 | puppeteer-chrome | noisy | no | 6804.47 | 6804.47 | N/A | N/A |
| parallel | 4 | shotium | pass | yes | 146.529 | 389.789 | 20.645 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 423.206 | 666.12 | 9.094 | 2.89× |
| parallel | 4 | playwright-chrome | pass | yes | 595.147 | 1301.249 | 6.249 | 4.06× |
| parallel | 4 | puppeteer-shell | pass | yes | 449.375 | 750.266 | 8.546 | 3.07× |
| reuse-page | 1 | playwright-shell | pass | no | 50.286 | 68.646 | 19.407 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 66.074 | 91.403 | 15.121 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 66.676 | 86.312 | 14.365 | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 10127.463 | 10127.463 | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 24381.463 | 24381.463 | N/A | N/A |
| resident | 1 | shotium | pass | yes | 64 | 378 | 6.428 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 552 | 907 | 1.526 | 8.63× |
| resident | 1 | playwright-chrome | noisy | no | 41068.421 | 41068.421 | N/A | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 31265.19 | 31265.19 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 31592.484 | 31592.484 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 30870.389 | 30870.389 | N/A | N/A |
| faults | 1 | shotium | noisy | no | 3633.982 | 3633.982 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 27326.853 | 27326.853 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 31764.519 | 31764.519 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 423.459 | 789.112 | 9.111 | 2.66× |
| soak | 4 | puppeteer-chrome | fail | no | 752.409 | 60405.6 | 0.435 | N/A |
| soak | 4 | playwright-chrome | pass | yes | 557.802 | 1953.862 | 6.737 | 3.51× |
| soak | 4 | puppeteer-shell | fail | no | 484.112 | 1037.543 | 8.047 | N/A |
| soak | 4 | shotium | pass | yes | 158.922 | 436.927 | 19.880 | 1.00× |

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
| cold | 1 | shotium | pass | no | 69 | 69 | 14.583 | N/A |
| cold-settled | 1 | shotium | pass | no | 14.632 | 14.997 | 66.887 | N/A |
| lifecycle | 1 | shotium | pass | no | 97.692 | 132.626 | 9.946 | N/A |
| warm | 1 | shotium | pass | no | 15.332 | 17.746 | 62.202 | N/A |
| batch | 1 | shotium | pass | no | 21.589 | 277.787 | 20.590 | N/A |
| parallel | 1 | shotium | pass | no | 23.118 | 286.009 | 20.626 | N/A |
| parallel | 2 | shotium | pass | no | 62.357 | 333.549 | 22.000 | N/A |
| parallel | 4 | shotium | pass | no | 163.826 | 1273.036 | 15.848 | N/A |
| resident | 1 | shotium | noisy | no | 24866.131 | 24866.131 | N/A | N/A |
| faults | 1 | shotium | pass | no | 11964.008 | 11964.008 | N/A | N/A |
| soak | 4 | shotium | pass | no | 135.303 | 372.713 | 22.512 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 6.818× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-shell | 4.657× | 5 | 6 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 19.657× | 6 | 6 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 22.382× | 2 | 2 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `cold/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `cold/c1`, `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.3.4/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | pass | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 4674 | 6294 | 0.205 | 29.58× |
| cold | 1 | playwright-shell | pass | yes | 1908 | 2023 | 0.546 | 12.08× |
| cold | 1 | puppeteer-chrome | pass | yes | 3662 | 4061 | 0.278 | 23.18× |
| cold | 1 | puppeteer-shell | pass | yes | 1681 | 1872 | 0.576 | 10.64× |
| cold | 1 | shotium | pass | yes | 158 | 172 | 6.428 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 12592.096 | 14004.075 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 892.451 | 977.081 | 0.127 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | yes | 331.929 | 483.721 | 2.810 | 13.30× |
| cold-settled | 1 | shotium | pass | yes | 24.956 | 28.589 | 39.131 | 1.00× |
| cold-settled | 1 | playwright-shell | noisy | no | 10207.842 | 10843.99 | N/A | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 1719.188 | 2923.724 | 0.540 | 9.52× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3984.371 | 4622.425 | 0.250 | 22.07× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 2197.801 | 3495.388 | 0.443 | 12.18× |
| lifecycle | 1 | shotium | pass | yes | 180.511 | 401.773 | 4.797 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 3901.476 | 4468.07 | 0.252 | 21.61× |
| warm | 1 | playwright-chrome | pass | yes | 840.986 | 1073.792 | 1.158 | 55.81× |
| warm | 1 | shotium | pass | yes | 15.068 | 18.998 | 63.424 | 1.00× |
| warm | 1 | playwright-shell | noisy | no | 233.064 | 234.82 | 0.162 | N/A |
| warm | 1 | puppeteer-shell | pass | yes | 242.028 | 344.974 | 3.932 | 16.06× |
| warm | 1 | puppeteer-chrome | noisy | no | 9259.021 | 12800.162 | N/A | N/A |
| batch | 1 | shotium | pass | yes | 28.428 | 277.648 | 18.209 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 283.069 | 624.026 | 3.329 | 9.96× |
| batch | 1 | playwright-shell | noisy | no | 6659.582 | 6659.582 | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 9565.388 | 9565.388 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | yes | 877.375 | 1142.843 | 1.132 | 30.86× |
| parallel | 1 | playwright-chrome | noisy | no | 16430.196 | 16430.196 | N/A | N/A |
| parallel | 1 | puppeteer-shell | pass | yes | 376.102 | 778.17 | 2.531 | 9.28× |
| parallel | 1 | puppeteer-chrome | noisy | no | 10933.937 | 10933.937 | N/A | N/A |
| parallel | 1 | shotium | pass | yes | 40.537 | 265.895 | 15.331 | 1.00× |
| parallel | 1 | playwright-shell | noisy | no | 9371.501 | 9371.501 | N/A | N/A |
| parallel | 2 | puppeteer-shell | pass | yes | 469.25 | 808.392 | 3.972 | 4.21× |
| parallel | 2 | puppeteer-chrome | noisy | no | 11205.804 | 11205.804 | N/A | N/A |
| parallel | 2 | shotium | pass | yes | 111.481 | 310.355 | 15.706 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 370.771 | 632.153 | 5.134 | 3.33× |
| parallel | 2 | playwright-chrome | noisy | no | 15707.244 | 15707.244 | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | 11949.177 | 11949.177 | N/A | N/A |
| parallel | 4 | shotium | pass | yes | 245.403 | 740.75 | 14.600 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 799.302 | 1894.958 | 4.620 | 3.26× |
| parallel | 4 | playwright-chrome | pass | yes | 3083.615 | 6381.075 | 1.293 | 12.57× |
| parallel | 4 | puppeteer-shell | pass | yes | 742.224 | 1936.904 | 5.023 | 3.02× |
| reuse-page | 1 | playwright-shell | pass | no | 70.117 | 168.904 | 12.019 | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | 8241.285 | 8241.285 | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 140.3 | 239.379 | 7.124 | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 7096.992 | 7096.992 | N/A | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 1509 | 1729 | 0.661 | 1.99× |
| resident | 1 | shotium | pass | yes | 760 | 1024 | 1.243 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 1511 | 1942 | 0.630 | 1.99× |
| resident | 1 | playwright-chrome | pass | yes | 3102 | 3138 | 0.336 | 4.08× |
| resident | 1 | puppeteer-chrome | noisy | no | 28181.362 | 28181.362 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 26022.514 | 26022.514 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | 12788.094 | 12788.094 | N/A | N/A |
| faults | 1 | shotium | pass | no | 14630.154 | 14630.154 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 29752.253 | 29752.253 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 52263.773 | 52263.773 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 670.945 | 2764.262 | 5.546 | 4.12× |
| soak | 4 | puppeteer-chrome | noisy | no | 9310.738 | 9310.738 | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 2778.008 | 6199.324 | 1.224 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 546.921 | 1064.201 | 7.117 | 3.36× |
| soak | 4 | shotium | pass | yes | 162.936 | 474.775 | 19.928 | 1.00× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 7 eligible cell(s), with 7 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 5 | 7 / 7 | 7 |
| 2 | playwright-shell | 5.003× | 5 | 7 / 7 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 6.514× | 5 | 6 / 7 | 0 |
| not ranked (partial coverage) | playwright-chrome | 15.120× | 4 | 4 / 7 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 23.388× | 2 | 2 / 7 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`
- puppeteer-chrome: `cold/c1`, `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.3.4/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 2836 | 7022 | 0.311 | 25.10× |
| cold | 1 | playwright-shell | pass | yes | 931 | 1014 | 1.077 | 8.24× |
| cold | 1 | puppeteer-chrome | pass | yes | 2831 | 6088 | 0.327 | 25.05× |
| cold | 1 | puppeteer-shell | pass | yes | 853 | 1163 | 1.121 | 7.55× |
| cold | 1 | shotium | pass | yes | 113 | 205 | 8.197 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 6077.967 | 8879.962 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 209.71 | 504.6 | 0.609 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 228.157 | 349.442 | 1.079 | N/A |
| cold-settled | 1 | shotium | noisy | no | 10.343 | 26.616 | 4.514 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 170.7 | 253.228 | 6.078 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 490.937 | 768.787 | 1.847 | 7.59× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1636.463 | 2475.872 | 0.589 | 25.32× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 610.434 | 1008.995 | 1.551 | 9.44× |
| lifecycle | 1 | shotium | pass | yes | 64.64 | 107.572 | 15.275 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1411.361 | 2465.332 | 0.659 | 21.83× |
| warm | 1 | playwright-chrome | noisy | no | 226.771 | 360.872 | 4.170 | N/A |
| warm | 1 | shotium | noisy | no | 13.651 | 14.728 | 0.654 | N/A |
| warm | 1 | playwright-shell | noisy | no | 172.178 | 229.733 | 5.490 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 272.352 | 342.133 | 0.876 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 7623.834 | 12657.092 | N/A | N/A |
| batch | 1 | shotium | pass | yes | 25.761 | 270.275 | 19.871 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 266.312 | 558.665 | 3.500 | 10.34× |
| batch | 1 | playwright-shell | pass | yes | 169.346 | 387.714 | 5.183 | 6.57× |
| batch | 1 | puppeteer-chrome | noisy | no | 7592.708 | 7592.708 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | yes | 181.563 | 1578.159 | 3.914 | 7.05× |
| parallel | 1 | playwright-chrome | pass | yes | 235.798 | 1745.839 | 3.171 | 11.67× |
| parallel | 1 | puppeteer-shell | pass | yes | 249.052 | 634.809 | 3.773 | 12.33× |
| parallel | 1 | puppeteer-chrome | noisy | no | 6947.979 | 6947.979 | N/A | N/A |
| parallel | 1 | shotium | pass | yes | 20.206 | 274.265 | 24.668 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 157.853 | 379.945 | 5.731 | 7.81× |
| parallel | 2 | puppeteer-shell | noisy | no | 4863.848 | 4863.848 | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | 7102.638 | 7102.638 | N/A | N/A |
| parallel | 2 | shotium | pass | yes | 58.194 | 293.96 | 23.181 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 224.809 | 478.514 | 8.146 | 3.86× |
| parallel | 2 | playwright-chrome | fail | no | 316.203 | 2069.352 | 4.028 | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | 5647.334 | 5647.334 | N/A | N/A |
| parallel | 4 | shotium | pass | yes | 196.582 | 530.091 | 17.158 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 401.549 | 718.249 | 9.164 | 2.04× |
| parallel | 4 | playwright-chrome | fail | no | 544.111 | 6084.042 | 4.777 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 385.384 | 748.389 | 10.054 | 1.96× |
| reuse-page | 1 | playwright-shell | noisy | no | 1502.843 | 1502.843 | N/A | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 56.609 | 75.105 | 17.074 | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | 2764.586 | 2764.586 | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 155.018 | 202.984 | 6.891 | N/A |
| resident | 1 | puppeteer-shell | pass | no | 641 | 850 | 1.532 | N/A |
| resident | 1 | shotium | noisy | no | 6997.074 | 6997.074 | N/A | N/A |
| resident | 1 | playwright-shell | pass | no | 648 | 744 | 1.523 | N/A |
| resident | 1 | playwright-chrome | pass | no | 979 | 1431 | 1.022 | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 11974.248 | 11974.248 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 15796.717 | 15796.717 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 295.4 | 671.859 | 12.574 | 3.09× |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 480.197 | 4555.956 | 7.779 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 409.736 | 789.883 | 9.501 | 4.29× |
| soak | 4 | shotium | pass | yes | 95.492 | 405.403 | 26.059 | 1.00× |

