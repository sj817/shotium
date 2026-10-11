# Shotium 0.12.2 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `b4678c135a652cbd52843e2c73289f40d284a8d9`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 2 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 4 | 8 |
| darwin-arm64 | noisy | shotium | 4 | 8 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 7.659× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 8.244× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 10.912× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 9.739× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.12.2/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 891 | 960 | 1.108 | 12.04× |
| cold | 1 | puppeteer-shell | pass | yes | 634 | 666 | 1.568 | 8.57× |
| cold | 1 | playwright-shell | pass | yes | 783 | 816 | 1.264 | 10.58× |
| cold | 1 | shotium | pass | yes | 74 | 83 | 13.410 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 981 | 990 | 1.024 | 13.26× |
| cold-settled | 1 | shotium | pass | yes | 13.067 | 18.695 | 71.554 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 177.218 | 193.317 | 5.663 | 13.56× |
| cold-settled | 1 | playwright-shell | pass | yes | 149.592 | 161.829 | 6.591 | 11.45× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 128.124 | 141.194 | 7.767 | 9.81× |
| cold-settled | 1 | playwright-chrome | pass | yes | 180.357 | 204.37 | 5.534 | 13.80× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 943.476 | 988.142 | 1.056 | 13.08× |
| lifecycle | 1 | shotium | pass | yes | 72.144 | 113.723 | 12.556 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 686.966 | 763.368 | 1.433 | 9.52× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 659.419 | 723.732 | 1.512 | 9.14× |
| lifecycle | 1 | playwright-chrome | fail | no | 2120.243 | 2146.847 | N/A | N/A |
| warm | 1 | playwright-shell | pass | yes | 146.779 | 168.291 | 6.820 | 15.94× |
| warm | 1 | puppeteer-chrome | pass | yes | 183.281 | 203.688 | 5.390 | 19.91× |
| warm | 1 | puppeteer-shell | pass | yes | 133.259 | 148.981 | 7.429 | 14.47× |
| warm | 1 | playwright-chrome | pass | yes | 166.755 | 213.607 | 5.773 | 18.11× |
| warm | 1 | shotium | pass | yes | 9.207 | 14.577 | 98.068 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 151.47 | 374.979 | 5.611 | 11.50× |
| batch | 1 | puppeteer-chrome | pass | yes | 225.828 | 450.927 | 4.014 | 17.14× |
| batch | 1 | playwright-shell | pass | yes | 158.765 | 427.889 | 5.435 | 12.05× |
| batch | 1 | playwright-chrome | pass | yes | 199.501 | 416.907 | 4.618 | 15.14× |
| batch | 1 | shotium | pass | yes | 13.175 | 261.596 | 28.139 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 153.207 | 647.06 | 5.332 | 11.56× |
| parallel | 1 | shotium | pass | yes | 13.25 | 262.113 | 28.363 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 228.92 | 472.736 | 3.950 | 17.28× |
| parallel | 1 | playwright-shell | pass | yes | 165.378 | 389.759 | 5.279 | 12.48× |
| parallel | 1 | playwright-chrome | pass | yes | 205.86 | 418.2 | 4.410 | 15.54× |
| parallel | 2 | shotium | pass | yes | 37.766 | 273.343 | 28.330 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 377.253 | 635.44 | 4.877 | 9.99× |
| parallel | 2 | playwright-shell | pass | yes | 241.439 | 499.806 | 7.362 | 6.39× |
| parallel | 2 | playwright-chrome | pass | yes | 299.909 | 602.554 | 6.095 | 7.94× |
| parallel | 2 | puppeteer-shell | pass | yes | 243.402 | 498.635 | 7.262 | 6.45× |
| parallel | 4 | puppeteer-chrome | pass | yes | 706.881 | 1042.179 | 5.538 | 7.92× |
| parallel | 4 | playwright-shell | pass | yes | 453.491 | 800.826 | 8.148 | 5.08× |
| parallel | 4 | playwright-chrome | pass | yes | 536.338 | 878.502 | 6.974 | 6.01× |
| parallel | 4 | puppeteer-shell | pass | yes | 449.202 | 763.524 | 8.427 | 5.03× |
| parallel | 4 | shotium | pass | yes | 89.233 | 328.964 | 28.304 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 100.035 | 117.543 | 9.837 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 99.923 | 101.91 | 10.005 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.8 | 101.683 | 10.267 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.964 | 141.32 | 9.759 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 908 | 992 | 1.165 | 3.21× |
| resident | 1 | puppeteer-shell | pass | yes | 868 | 941 | 1.149 | 3.07× |
| resident | 1 | shotium | pass | yes | 283 | 327 | 4.351 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 1059 | 1080 | 0.965 | 3.74× |
| resident | 1 | playwright-shell | pass | yes | 980 | 1000 | 1.026 | 3.46× |
| faults | 1 | playwright-chrome | pass | no | 15752.827 | 15752.827 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15816.31 | 15816.31 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 9445.943 | 9445.943 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4945.624 | 4945.624 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 11072.087 | 11072.087 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 396.529 | 805.669 | 9.624 | 4.72× |
| soak | 4 | puppeteer-chrome | pass | yes | 628.908 | 1096.627 | 6.171 | 7.49× |
| soak | 4 | shotium | pass | yes | 83.974 | 348.319 | 29.910 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 391.155 | 771.919 | 9.491 | 4.66× |
| soak | 4 | playwright-chrome | pass | yes | 475.177 | 861.332 | 7.985 | 5.66× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.135× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.071× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.12.2/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 608 | 617 | 1.662 | 9.97× |
| cold | 1 | shotium | pass | yes | 61 | 63 | 16.432 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 775 | 795 | 1.294 | 12.70× |
| cold-settled | 1 | shotium | pass | yes | 20.8 | 21.014 | 47.716 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 131.484 | 147.301 | 7.424 | 6.32× |
| cold-settled | 1 | playwright-chrome | pass | yes | 157.241 | 164.105 | 6.399 | 7.56× |
| lifecycle | 1 | playwright-shell | pass | yes | 551.012 | 597.325 | 1.795 | 8.03× |
| lifecycle | 1 | playwright-chrome | fail | no | 1705.906 | 1823.026 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 68.648 | 79.532 | 14.387 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 166.039 | 178.738 | 6.014 | 9.27× |
| warm | 1 | shotium | pass | yes | 17.92 | 22.244 | 51.608 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 133.692 | 787.436 | 5.176 | 7.46× |
| batch | 1 | shotium | pass | yes | 23.909 | 258.49 | 23.626 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 143.296 | 389.991 | 5.839 | 5.99× |
| batch | 1 | playwright-chrome | pass | yes | 185.09 | 401.562 | 4.854 | 7.74× |
| parallel | 1 | playwright-chrome | pass | yes | 181.657 | 669.934 | 4.515 | 7.70× |
| parallel | 1 | playwright-shell | pass | yes | 142.151 | 373.616 | 5.902 | 6.03× |
| parallel | 1 | shotium | pass | yes | 23.583 | 257.989 | 23.843 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 197.333 | 429.161 | 8.880 | 4.10× |
| parallel | 2 | shotium | pass | yes | 48.153 | 285.614 | 24.256 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 255.645 | 548.82 | 7.073 | 5.31× |
| parallel | 4 | shotium | pass | yes | 103.191 | 340.035 | 24.386 | 1.00× |
| parallel | 4 | playwright-chrome | pass | yes | 412.141 | 688.59 | 9.007 | 3.99× |
| parallel | 4 | playwright-shell | pass | yes | 335.838 | 549.228 | 10.892 | 3.25× |
| reuse-page | 1 | playwright-shell | pass | no | 99.866 | 104.23 | 10.000 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 100.352 | 518.503 | 6.260 | N/A |
| resident | 1 | shotium | pass | yes | 412 | 441 | 2.460 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 929 | 955 | 1.075 | 2.25× |
| resident | 1 | playwright-chrome | pass | yes | 1018 | 1102 | 0.970 | 2.47× |
| faults | 1 | playwright-shell | pass | no | 14510.266 | 14510.266 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 18416.748 | 18416.748 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4742.253 | 4742.253 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 456.534 | 739.576 | 8.460 | 4.03× |
| soak | 4 | shotium | pass | yes | 113.318 | 378.802 | 23.501 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 352.357 | 688.117 | 10.522 | 3.11× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | puppeteer-chrome | 16.305× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-shell | 10.155× | 6 | 8 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 11.702× | 7 | 8 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 10.397× | 4 | 6 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.12.2\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 1269 | 1502 | 0.769 | 22.26× |
| cold | 1 | puppeteer-shell | pass | yes | 1005 | 1038 | 0.994 | 17.63× |
| cold | 1 | playwright-shell | pass | yes | 952 | 1069 | 1.035 | 16.70× |
| cold | 1 | shotium | pass | yes | 57 | 71 | 16.867 | 1.00× |
| cold | 1 | playwright-chrome | noisy | no | 1133 | 1333 | 0.848 | N/A |
| cold-settled | 1 | shotium | noisy | no | 12.434 | 16.012 | 0.742 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 211.049 | 260.81 | 0.098 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 166.45 | 221.417 | 0.198 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 162.48 | 185.118 | 0.437 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 193.543 | 222.971 | 0.126 | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2391.816 | 2794.859 | 0.413 | 28.87× |
| lifecycle | 1 | shotium | pass | yes | 82.838 | 138.968 | 10.847 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 951.486 | 1403.229 | 1.038 | 11.49× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1151.815 | 1384.736 | 0.842 | 13.90× |
| lifecycle | 1 | playwright-chrome | fail | no | 6747.858 | 9152.042 | N/A | N/A |
| warm | 1 | playwright-shell | pass | yes | 176.627 | 206.181 | 5.587 | 19.61× |
| warm | 1 | puppeteer-chrome | pass | yes | 259.972 | 379.781 | 3.667 | 28.87× |
| warm | 1 | puppeteer-shell | pass | yes | 166.571 | 234.305 | 5.769 | 18.50× |
| warm | 1 | playwright-chrome | noisy | no | 181.301 | 251.565 | 0.839 | N/A |
| warm | 1 | shotium | pass | yes | 9.005 | 13.264 | 95.006 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 199.548 | 457.219 | 4.294 | 13.02× |
| batch | 1 | puppeteer-chrome | pass | yes | 280.44 | 543.461 | 3.313 | 18.30× |
| batch | 1 | playwright-shell | noisy | no | 13108.92 | 13108.92 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | yes | 237.507 | 1522.921 | 3.680 | 15.49× |
| batch | 1 | shotium | pass | yes | 15.328 | 257.466 | 26.452 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 157.387 | 404.736 | 5.510 | 13.04× |
| parallel | 1 | shotium | pass | yes | 12.066 | 262.141 | 30.065 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 210.748 | 457.881 | 4.273 | 17.47× |
| parallel | 1 | playwright-shell | pass | yes | 177.324 | 421.514 | 5.022 | 14.70× |
| parallel | 1 | playwright-chrome | pass | yes | 162.877 | 396.5 | 5.324 | 13.50× |
| parallel | 2 | shotium | pass | yes | 32.615 | 278.841 | 30.834 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 356.97 | 573.526 | 5.364 | 10.94× |
| parallel | 2 | playwright-shell | pass | yes | 200.003 | 492.648 | 8.522 | 6.13× |
| parallel | 2 | playwright-chrome | pass | yes | 249.774 | 572.401 | 7.309 | 7.66× |
| parallel | 2 | puppeteer-shell | pass | yes | 218.244 | 472.37 | 8.050 | 6.69× |
| parallel | 4 | puppeteer-chrome | pass | yes | 635.771 | 1008.86 | 6.100 | 7.54× |
| parallel | 4 | playwright-shell | pass | yes | 373.717 | 640.388 | 10.443 | 4.43× |
| parallel | 4 | playwright-chrome | pass | yes | 439.111 | 751.823 | 8.649 | 5.21× |
| parallel | 4 | puppeteer-shell | noisy | no | 11244.617 | 11244.617 | N/A | N/A |
| parallel | 4 | shotium | pass | yes | 84.284 | 331.755 | 27.914 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 117.531 | 142.729 | 8.123 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 102.613 | 130.096 | 9.238 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.219 | 116.688 | 9.796 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 117.054 | 183.127 | 8.173 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 891 | 1422 | 0.986 | 17.82× |
| resident | 1 | puppeteer-shell | pass | yes | 741 | 1421 | 1.154 | 14.82× |
| resident | 1 | shotium | pass | yes | 50 | 411 | 8.197 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 1206 | 1657 | 0.831 | 24.12× |
| resident | 1 | playwright-shell | pass | yes | 748 | 1113 | 1.260 | 14.96× |
| faults | 1 | playwright-chrome | pass | no | 27965.378 | 27965.378 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 28781.578 | 28781.578 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 30646.442 | 30646.442 | N/A | N/A |
| faults | 1 | shotium | pass | no | 10327.504 | 10327.504 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 27876.259 | 27876.259 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 453.788 | 998.783 | 8.458 | 4.60× |
| soak | 4 | puppeteer-chrome | pass | yes | 919.926 | 2196.238 | 4.279 | 9.33× |
| soak | 4 | shotium | pass | yes | 98.569 | 378.044 | 27.087 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 495.439 | 916.493 | 7.800 | 5.03× |
| soak | 4 | playwright-chrome | pass | yes | 618.634 | 1918.662 | 6.094 | 6.28× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.12.2\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 56 | 57 | 17.903 | N/A |
| cold-settled | 1 | shotium | pass | no | 9.638 | 13.034 | 98.028 | N/A |
| lifecycle | 1 | shotium | pass | no | 81.536 | 135.927 | 11.579 | N/A |
| warm | 1 | shotium | noisy | no | 8.05 | 11.831 | 0.120 | N/A |
| batch | 1 | shotium | pass | no | 11.856 | 269.374 | 29.641 | N/A |
| parallel | 1 | shotium | pass | no | 11.441 | 268.308 | 29.658 | N/A |
| parallel | 2 | shotium | pass | no | 32.39 | 279.985 | 30.504 | N/A |
| parallel | 4 | shotium | pass | no | 67.32 | 318.439 | 31.972 | N/A |
| resident | 1 | shotium | pass | no | 245 | 577 | 3.138 | N/A |
| faults | 1 | shotium | pass | no | 10192.303 | 10192.303 | N/A | N/A |
| soak | 4 | shotium | pass | no | 71.726 | 368.819 | 30.928 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 8 / 8 | 8 |
| 2 | playwright-shell | 8.685× | 6 | 8 / 8 | 0 |
| 3 | puppeteer-shell | 9.813× | 6 | 8 / 8 | 0 |
| 4 | puppeteer-chrome | 15.953× | 6 | 8 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 27.625× | 5 | 7 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.12.2/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 2161 | 3599 | 0.406 | 24.84× |
| cold | 1 | puppeteer-shell | pass | yes | 1032 | 1357 | 0.935 | 11.86× |
| cold | 1 | playwright-shell | pass | yes | 1025 | 1505 | 0.910 | 11.78× |
| cold | 1 | shotium | pass | yes | 87 | 123 | 10.671 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 2816 | 3282 | 0.350 | 32.37× |
| cold-settled | 1 | shotium | noisy | no | 11.871 | 12.585 | 87.917 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 722.62 | 983.532 | 1.430 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 321.474 | 379.053 | 0.366 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 341.307 | 358.111 | 3.063 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 746.075 | 797.404 | 1.331 | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2325.005 | 2817.898 | 0.430 | 23.33× |
| lifecycle | 1 | shotium | pass | yes | 99.678 | 145.705 | 9.604 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 1009.393 | 1233.117 | 0.983 | 10.13× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1294.97 | 1670.049 | 0.749 | 12.99× |
| lifecycle | 1 | playwright-chrome | fail | no | 3961.846 | 4083.397 | N/A | N/A |
| warm | 1 | playwright-shell | noisy | no | 258.535 | 384.129 | 3.604 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 531.053 | 614.005 | 0.303 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 342.096 | 423.164 | 3.036 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 802.479 | 930.122 | 1.244 | N/A |
| warm | 1 | shotium | noisy | no | 10.135 | 17.15 | 84.837 | N/A |
| batch | 1 | puppeteer-shell | pass | yes | 353.011 | 1034.934 | 2.679 | 21.87× |
| batch | 1 | puppeteer-chrome | pass | yes | 444.732 | 1252.59 | 2.044 | 27.55× |
| batch | 1 | playwright-shell | pass | yes | 299.304 | 903.43 | 3.049 | 18.54× |
| batch | 1 | playwright-chrome | pass | yes | 855.355 | 1255.534 | 1.144 | 52.99× |
| batch | 1 | shotium | pass | yes | 16.141 | 258.062 | 27.345 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 341.759 | 726.964 | 2.808 | 24.74× |
| parallel | 1 | shotium | pass | yes | 13.814 | 256.755 | 28.798 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 424.578 | 832.509 | 2.281 | 30.74× |
| parallel | 1 | playwright-shell | pass | yes | 296.363 | 686.389 | 3.102 | 21.45× |
| parallel | 1 | playwright-chrome | pass | yes | 859.751 | 1160.295 | 1.150 | 62.24× |
| parallel | 2 | shotium | pass | yes | 35.167 | 274.95 | 28.904 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 640.764 | 1659.314 | 2.960 | 18.22× |
| parallel | 2 | playwright-shell | pass | yes | 316.5 | 797.695 | 5.575 | 9.00× |
| parallel | 2 | playwright-chrome | pass | yes | 1472.271 | 2884.352 | 1.357 | 41.87× |
| parallel | 2 | puppeteer-shell | pass | yes | 394.913 | 731.293 | 4.975 | 11.23× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1079.079 | 1622.355 | 3.541 | 12.73× |
| parallel | 4 | playwright-shell | pass | yes | 609.629 | 997.355 | 6.384 | 7.19× |
| parallel | 4 | playwright-chrome | pass | yes | 2714.818 | 4936.96 | 1.467 | 32.02× |
| parallel | 4 | puppeteer-shell | pass | yes | 558.598 | 1072.206 | 6.829 | 6.59× |
| parallel | 4 | shotium | pass | yes | 84.779 | 325.573 | 28.115 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 245.058 | 306.755 | 4.117 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 236.655 | 314.054 | 4.513 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 248.205 | 350.117 | 3.801 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 240.42 | 344.096 | 4.445 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 1834 | 3680 | 0.455 | 2.76× |
| resident | 1 | puppeteer-shell | pass | yes | 1363 | 1664 | 0.704 | 2.05× |
| resident | 1 | shotium | pass | yes | 664 | 716 | 1.541 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 2047 | 2201 | 0.484 | 3.08× |
| resident | 1 | playwright-shell | pass | yes | 1366 | 1484 | 0.728 | 2.06× |
| faults | 1 | playwright-chrome | pass | no | 38448.211 | 38448.211 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 23857.926 | 23857.926 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 18860.476 | 18860.476 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6810.337 | 6810.337 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 30808.183 | 30808.183 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 699.562 | 1829.141 | 5.476 | 6.79× |
| soak | 4 | puppeteer-chrome | pass | yes | 1375.947 | 5118.018 | 2.817 | 13.35× |
| soak | 4 | shotium | pass | yes | 103.061 | 480.999 | 26.331 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 527.819 | 1911.163 | 7.204 | 5.12× |
| soak | 4 | playwright-chrome | pass | yes | 2867.668 | 5573.775 | 1.422 | 27.82× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 8 / 8 | 8 |
| 2 | playwright-shell | 8.766× | 6 | 8 / 8 | 0 |
| 3 | puppeteer-shell | 11.222× | 6 | 8 / 8 | 0 |
| 4 | puppeteer-chrome | 19.746× | 6 | 8 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 14.099× | 5 | 7 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.12.2/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 3182 | 9417 | 0.251 | 27.91× |
| cold | 1 | puppeteer-shell | pass | yes | 1675 | 1828 | 0.612 | 14.69× |
| cold | 1 | playwright-shell | pass | yes | 1277 | 1687 | 0.729 | 11.20× |
| cold | 1 | shotium | pass | yes | 114 | 163 | 8.140 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 3012 | 6000 | 0.296 | 26.42× |
| cold-settled | 1 | shotium | pass | yes | 14.441 | 21.364 | 67.082 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 438.272 | 541.429 | 2.166 | 30.35× |
| cold-settled | 1 | playwright-shell | pass | yes | 220.884 | 266.33 | 4.433 | 15.30× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 264.729 | 454.702 | 3.289 | 18.33× |
| cold-settled | 1 | playwright-chrome | pass | yes | 326.271 | 540.039 | 2.761 | 22.59× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2658.412 | 4499.564 | 0.359 | 24.17× |
| lifecycle | 1 | shotium | pass | yes | 109.967 | 182.248 | 9.509 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 864.18 | 2004.575 | 0.998 | 7.86× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 993.998 | 1784.52 | 0.950 | 9.04× |
| lifecycle | 1 | playwright-chrome | fail | no | 5574.722 | 6468.912 | N/A | N/A |
| warm | 1 | playwright-shell | noisy | no | 148.208 | 217.321 | 6.280 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 347.817 | 435.57 | 2.774 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 182.328 | 259.756 | 5.264 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 158.851 | 222.393 | 5.953 | N/A |
| warm | 1 | shotium | noisy | no | 7608.557 | 7608.557 | N/A | N/A |
| batch | 1 | puppeteer-shell | pass | yes | 289.274 | 665.768 | 3.258 | 30.61× |
| batch | 1 | puppeteer-chrome | pass | yes | 433.429 | 1429.222 | 2.090 | 45.87× |
| batch | 1 | playwright-shell | pass | yes | 184.398 | 439.09 | 4.877 | 19.51× |
| batch | 1 | playwright-chrome | pass | yes | 236.967 | 1735.518 | 3.279 | 25.08× |
| batch | 1 | shotium | pass | yes | 9.45 | 266.908 | 33.837 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 328.671 | 589.118 | 3.076 | 22.00× |
| parallel | 1 | shotium | pass | yes | 14.938 | 304.631 | 28.098 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 461.694 | 2152.571 | 1.855 | 30.91× |
| parallel | 1 | playwright-shell | pass | yes | 190.286 | 470.792 | 4.675 | 12.74× |
| parallel | 1 | playwright-chrome | pass | yes | 248.651 | 1751.477 | 2.997 | 16.65× |
| parallel | 2 | shotium | pass | yes | 35.67 | 313.165 | 28.202 | 1.00× |
| parallel | 2 | puppeteer-chrome | pass | yes | 675.77 | 2321.403 | 2.663 | 18.95× |
| parallel | 2 | playwright-shell | pass | yes | 263.032 | 491.243 | 6.934 | 7.37× |
| parallel | 2 | playwright-chrome | pass | yes | 434.174 | 1219.966 | 4.050 | 12.17× |
| parallel | 2 | puppeteer-shell | pass | yes | 362.935 | 668.566 | 5.168 | 10.17× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1224.038 | 2197.212 | 3.129 | 15.90× |
| parallel | 4 | playwright-shell | pass | yes | 512.165 | 948.932 | 7.395 | 6.65× |
| parallel | 4 | playwright-chrome | pass | yes | 838.558 | 2803.827 | 4.421 | 10.89× |
| parallel | 4 | puppeteer-shell | pass | yes | 548.172 | 960.676 | 7.290 | 7.12× |
| parallel | 4 | shotium | pass | yes | 76.995 | 340.289 | 29.877 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 123.873 | 245.752 | 7.674 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 92.545 | 127.891 | 10.623 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 233.427 | 374.151 | 3.871 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 269.217 | 386.974 | 3.532 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 1171 | 1998 | 0.750 | 2.64× |
| resident | 1 | puppeteer-shell | pass | yes | 938 | 987 | 1.095 | 2.12× |
| resident | 1 | shotium | pass | yes | 443 | 585 | 2.244 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 1485 | 2037 | 0.723 | 3.35× |
| resident | 1 | playwright-shell | pass | yes | 941 | 1181 | 1.028 | 2.12× |
| faults | 1 | playwright-chrome | pass | no | 28433.208 | 28433.208 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15862.7 | 15862.7 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 11439.143 | 11439.143 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5421.415 | 5421.415 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 25391.54 | 25391.54 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | 9482.622 | 9482.622 | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |

