# Shotium 0.7.2 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `236879439c28b39f0754873ab48d0b5b562f3f10`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 3 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 1 | 9 |
| win32-arm64 | pass | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 9 |
| darwin-arm64 | noisy | shotium | 1 | 6 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 6.759× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 6.809× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 8.291× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 11.141× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.7.2/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 922 | 932 | 1.086 | 15.90× |
| cold | 1 | puppeteer-shell | pass | yes | 569 | 596 | 1.754 | 9.81× |
| cold | 1 | playwright-shell | pass | yes | 742 | 1092 | 1.267 | 12.79× |
| cold | 1 | puppeteer-chrome | pass | yes | 809 | 818 | 1.242 | 13.95× |
| cold | 1 | shotium | pass | yes | 58 | 61 | 17.241 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 13.798 | 15.18 | 70.625 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 131.73 | 133.918 | 7.690 | 9.55× |
| cold-settled | 1 | playwright-chrome | pass | yes | 151.159 | 157.941 | 6.541 | 10.96× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 163.984 | 179.116 | 6.053 | 11.88× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 140.436 | 147.445 | 7.204 | 10.18× |
| lifecycle | 1 | playwright-shell | pass | yes | 645.232 | 708.078 | 1.553 | 12.05× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 859.967 | 920.867 | 1.158 | 16.06× |
| lifecycle | 1 | shotium | pass | yes | 53.546 | 104.232 | 15.484 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 589.83 | 670.21 | 1.655 | 11.02× |
| lifecycle | 1 | playwright-chrome | pass | yes | 835.144 | 909.903 | 1.188 | 15.60× |
| warm | 1 | playwright-chrome | pass | yes | 164.51 | 204.016 | 5.982 | 11.97× |
| warm | 1 | playwright-shell | pass | yes | 132.892 | 169.067 | 7.412 | 9.67× |
| warm | 1 | puppeteer-shell | pass | yes | 133.303 | 149.203 | 7.435 | 9.70× |
| warm | 1 | shotium | pass | yes | 13.745 | 22.573 | 64.446 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 164.441 | 173.744 | 6.181 | 11.96× |
| batch | 1 | puppeteer-chrome | pass | yes | 191.339 | 421.25 | 4.656 | 9.71× |
| batch | 1 | playwright-chrome | pass | yes | 180.813 | 403.279 | 4.968 | 9.17× |
| batch | 1 | shotium | pass | yes | 19.708 | 261.777 | 24.113 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 145.691 | 371.22 | 5.853 | 7.39× |
| batch | 1 | puppeteer-shell | pass | yes | 151.308 | 387.915 | 5.594 | 7.68× |
| parallel | 1 | playwright-chrome | pass | yes | 169.83 | 672.034 | 4.958 | 8.90× |
| parallel | 1 | puppeteer-shell | pass | yes | 143.702 | 404.594 | 5.815 | 7.53× |
| parallel | 1 | shotium | pass | yes | 19.086 | 259.081 | 23.418 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 180.31 | 410.309 | 4.892 | 9.45× |
| parallel | 1 | playwright-shell | pass | yes | 128.158 | 371.526 | 6.277 | 6.71× |
| parallel | 2 | puppeteer-shell | pass | yes | 218.657 | 474.969 | 7.820 | 3.78× |
| parallel | 2 | shotium | pass | yes | 57.79 | 280.331 | 23.655 | 1.00× |
| parallel | 2 | puppeteer-chrome | fail | no | 249.695 | 877.063 | 3.431 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 205.411 | 426.664 | 8.557 | 3.55× |
| parallel | 2 | playwright-chrome | pass | yes | 255.289 | 544.967 | 7.067 | 4.42× |
| parallel | 4 | shotium | pass | yes | 122.29 | 405.601 | 23.650 | 1.00× |
| parallel | 4 | puppeteer-chrome | fail | no | 256.781 | 16086.085 | 3.492 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 375.12 | 637.682 | 9.816 | 3.07× |
| parallel | 4 | playwright-chrome | pass | yes | 468.649 | 779.729 | 8.025 | 3.83× |
| parallel | 4 | puppeteer-shell | pass | yes | 425.676 | 699.406 | 9.121 | 3.48× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.131 | 116.873 | 9.705 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.885 | 103.369 | 10.098 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 83.749 | 100.85 | 11.473 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.474 | 89.615 | 11.940 | N/A |
| resident | 1 | playwright-shell | pass | yes | 958 | 986 | 1.104 | 7.79× |
| resident | 1 | puppeteer-shell | pass | yes | 880 | 901 | 1.201 | 7.15× |
| resident | 1 | puppeteer-chrome | pass | yes | 897 | 938 | 1.186 | 7.29× |
| resident | 1 | playwright-chrome | pass | yes | 1010 | 1061 | 0.982 | 8.21× |
| resident | 1 | shotium | pass | yes | 123 | 205 | 8.009 | 1.00× |
| faults | 1 | playwright-chrome | pass | no | 17128.55 | 17128.55 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5611.323 | 5611.323 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 11858.768 | 11858.768 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15080.436 | 15080.436 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10662.352 | 10662.352 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 269.996 | 27125.749 | 4.146 | N/A |
| soak | 4 | playwright-shell | pass | yes | 388.673 | 765.419 | 9.454 | 3.32× |
| soak | 4 | shotium | pass | yes | 117.157 | 414.007 | 24.186 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 432.04 | 835.357 | 8.829 | 3.69× |
| soak | 4 | playwright-chrome | pass | yes | 487.568 | 881.928 | 7.821 | 4.16× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.516× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.473× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-arm64@0.7.2/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 642 | 669 | 1.546 | 10.52× |
| cold | 1 | playwright-chrome | pass | yes | 817 | 1054 | 1.173 | 13.39× |
| cold | 1 | shotium | pass | yes | 61 | 64 | 16.355 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 144.269 | 166.335 | 6.807 | 4.53× |
| cold-settled | 1 | playwright-shell | pass | yes | 112.254 | 127.766 | 8.518 | 3.53× |
| cold-settled | 1 | shotium | pass | yes | 31.838 | 36.442 | 30.647 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 742.601 | 799.827 | 1.345 | 10.47× |
| lifecycle | 1 | shotium | pass | yes | 70.926 | 93.904 | 13.699 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 572.584 | 625.06 | 1.728 | 8.07× |
| warm | 1 | playwright-shell | pass | yes | 121.096 | 148.916 | 7.955 | 4.09× |
| warm | 1 | shotium | pass | yes | 29.624 | 33.37 | 32.852 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 150.46 | 183.892 | 6.405 | 5.08× |
| batch | 1 | playwright-shell | pass | yes | 136.94 | 376.328 | 6.261 | 3.72× |
| batch | 1 | shotium | pass | yes | 36.835 | 260.171 | 18.172 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 177.53 | 392.768 | 5.021 | 4.82× |
| parallel | 1 | playwright-shell | pass | yes | 132.155 | 358.155 | 6.301 | 3.61× |
| parallel | 1 | playwright-chrome | pass | yes | 177.151 | 390.54 | 5.073 | 4.84× |
| parallel | 1 | shotium | pass | yes | 36.595 | 268.184 | 18.231 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 245.331 | 506.52 | 7.483 | 3.06× |
| parallel | 2 | shotium | pass | yes | 80.195 | 295.652 | 18.485 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 185.743 | 426.056 | 9.337 | 2.32× |
| parallel | 4 | shotium | pass | yes | 160.782 | 407.497 | 18.548 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 324.169 | 576.587 | 11.375 | 2.02× |
| parallel | 4 | playwright-chrome | pass | yes | 410.501 | 650.902 | 9.349 | 2.55× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.237 | 100.37 | 11.914 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.258 | 87.833 | 12.026 | N/A |
| resident | 1 | shotium | pass | yes | 437 | 455 | 2.332 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 877 | 985 | 1.130 | 2.01× |
| resident | 1 | playwright-chrome | pass | yes | 942 | 1017 | 1.052 | 2.16× |
| faults | 1 | shotium | pass | no | 5306.863 | 5306.863 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15834.07 | 15834.07 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 16119.049 | 16119.049 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 420.552 | 753.129 | 9.253 | 2.53× |
| soak | 4 | shotium | pass | yes | 166.205 | 428.426 | 18.423 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 311.17 | 596.97 | 11.697 | 1.87× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| not ranked (partial coverage) | playwright-shell | 6.741× | 5 | 7 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 8.535× | 5 | 7 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 12.300× | 5 | 6 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 16.168× | 4 | 4 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`
- puppeteer-chrome: `cold/c1`, `lifecycle/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.7.2\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 1073 | 1237 | 0.915 | 15.78× |
| cold | 1 | puppeteer-shell | pass | yes | 901 | 928 | 1.107 | 13.25× |
| cold | 1 | playwright-shell | pass | yes | 826 | 846 | 1.211 | 12.15× |
| cold | 1 | puppeteer-chrome | pass | yes | 1036 | 1175 | 0.947 | 15.24× |
| cold | 1 | shotium | pass | yes | 68 | 70 | 14.675 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 12.997 | 16.848 | 71.739 | 1.00× |
| cold-settled | 1 | playwright-shell | noisy | no | 157.653 | 179.506 | 0.348 | N/A |
| cold-settled | 1 | playwright-chrome | pass | yes | 172.122 | 204.351 | 5.634 | 13.24× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 177.006 | 201.235 | 0.247 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | yes | 161.578 | 194.09 | 6.028 | 12.43× |
| lifecycle | 1 | playwright-shell | pass | yes | 1085.804 | 1335.427 | 0.951 | 10.34× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2185.007 | 7659.24 | 0.400 | 20.81× |
| lifecycle | 1 | shotium | pass | yes | 104.981 | 212.642 | 8.547 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1435.866 | 2283.452 | 0.693 | 13.68× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1766.057 | 7480.921 | 0.364 | 16.82× |
| warm | 1 | playwright-chrome | noisy | no | 184.731 | 202.498 | 0.975 | N/A |
| warm | 1 | playwright-shell | noisy | no | 160.505 | 182.288 | 1.156 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 170.436 | 227.026 | 0.851 | N/A |
| warm | 1 | shotium | pass | yes | 13.82 | 15.551 | 68.421 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 201.922 | 240.378 | 4.881 | 14.61× |
| batch | 1 | puppeteer-chrome | pass | no | 216.231 | 444.09 | 4.046 | N/A |
| batch | 1 | playwright-chrome | pass | no | 217 | 475.788 | 4.076 | N/A |
| batch | 1 | shotium | noisy | no | 8987.214 | 8987.214 | N/A | N/A |
| batch | 1 | playwright-shell | pass | no | 182.582 | 438.809 | 4.645 | N/A |
| batch | 1 | puppeteer-shell | pass | no | 230.929 | 450.131 | 3.979 | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 189.355 | 457.403 | 4.653 | 8.07× |
| parallel | 1 | puppeteer-shell | pass | yes | 187.983 | 437.838 | 4.677 | 8.02× |
| parallel | 1 | shotium | pass | yes | 23.451 | 283.732 | 21.965 | 1.00× |
| parallel | 1 | puppeteer-chrome | fail | no | 200.837 | 280.314 | 0.724 | N/A |
| parallel | 1 | playwright-shell | pass | yes | 184.867 | 445.807 | 4.710 | 7.88× |
| parallel | 2 | puppeteer-shell | pass | yes | 281.159 | 621.577 | 6.450 | 4.31× |
| parallel | 2 | shotium | pass | yes | 65.295 | 289.46 | 20.792 | 1.00× |
| parallel | 2 | puppeteer-chrome | fail | no | 312.316 | 30187.76 | 0.421 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 251.715 | 563.833 | 6.928 | 3.86× |
| parallel | 2 | playwright-chrome | noisy | no | 16251.403 | 16251.403 | N/A | N/A |
| parallel | 4 | shotium | pass | yes | 132.282 | 375.16 | 22.384 | 1.00× |
| parallel | 4 | puppeteer-chrome | noisy | no | 9721.595 | 9721.595 | N/A | N/A |
| parallel | 4 | playwright-shell | pass | yes | 459.35 | 823.073 | 8.089 | 3.47× |
| parallel | 4 | playwright-chrome | pass | yes | 556.19 | 1209.722 | 6.615 | 4.20× |
| parallel | 4 | puppeteer-shell | pass | yes | 452.129 | 697.546 | 8.343 | 3.42× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 101.922 | 146.453 | 8.937 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.377 | 142.37 | 9.514 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 101.655 | 127.909 | 9.603 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.925 | 98.035 | 11.616 | N/A |
| resident | 1 | playwright-shell | pass | yes | 720 | 1052 | 1.316 | 15.00× |
| resident | 1 | puppeteer-shell | pass | yes | 596 | 1164 | 1.343 | 12.42× |
| resident | 1 | puppeteer-chrome | pass | yes | 708 | 1260 | 1.302 | 14.75× |
| resident | 1 | playwright-chrome | pass | yes | 1393 | 2286 | 0.703 | 29.02× |
| resident | 1 | shotium | pass | yes | 48 | 411 | 8.872 | 1.00× |
| faults | 1 | playwright-chrome | pass | no | 26292.615 | 26292.615 | N/A | N/A |
| faults | 1 | shotium | pass | no | 8337.191 | 8337.191 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 28648.225 | 28648.225 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 27384.454 | 27384.454 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 18412.613 | 18412.613 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 275.513 | 30362.621 | 1.563 | N/A |
| soak | 4 | playwright-shell | pass | yes | 330.284 | 714.269 | 11.211 | 3.18× |
| soak | 4 | shotium | pass | yes | 103.851 | 580.632 | 25.185 | 1.00× |
| soak | 4 | puppeteer-shell | noisy | no | 11109.209 | 11109.209 | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | 10862.087 | 10862.087 | N/A | N/A |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-arm64@0.7.2\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 66 | 68 | 15.251 | N/A |
| cold-settled | 1 | shotium | pass | no | 12.239 | 13.212 | 79.013 | N/A |
| lifecycle | 1 | shotium | pass | no | 93.486 | 115.475 | 10.547 | N/A |
| warm | 1 | shotium | pass | no | 12.902 | 16.398 | 73.125 | N/A |
| batch | 1 | shotium | pass | no | 23.546 | 272.615 | 22.125 | N/A |
| parallel | 1 | shotium | pass | no | 21.595 | 274.209 | 23.791 | N/A |
| parallel | 2 | shotium | pass | no | 53.939 | 289.372 | 23.701 | N/A |
| parallel | 4 | shotium | pass | no | 115.182 | 348.775 | 24.050 | N/A |
| resident | 1 | shotium | pass | no | 679 | 1301 | 1.541 | N/A |
| faults | 1 | shotium | pass | no | 10736.846 | 10736.846 | N/A | N/A |
| soak | 4 | shotium | pass | no | 116.866 | 384.708 | 23.636 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 8.817× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 9.572× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 26.089× | 7 | 8 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 14.940× | 7 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.7.2/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 3293 | 4604 | 0.278 | 39.20× |
| cold | 1 | puppeteer-shell | pass | yes | 1016 | 1218 | 0.951 | 12.10× |
| cold | 1 | playwright-shell | pass | yes | 1005 | 2775 | 0.797 | 11.96× |
| cold | 1 | puppeteer-chrome | pass | yes | 2123 | 2359 | 0.478 | 25.27× |
| cold | 1 | shotium | pass | yes | 84 | 324 | 8.547 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 11.68 | 13.702 | 81.278 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 263.896 | 309.813 | 3.813 | 22.59× |
| cold-settled | 1 | playwright-chrome | pass | yes | 708.809 | 847.646 | 1.367 | 60.69× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 358.567 | 479.062 | 2.877 | 30.70× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 335.583 | 446.418 | 2.992 | 28.73× |
| lifecycle | 1 | playwright-shell | pass | yes | 1019.807 | 1382.276 | 0.944 | 9.47× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2301.909 | 2920.148 | 0.427 | 21.38× |
| lifecycle | 1 | shotium | pass | yes | 107.68 | 250.743 | 8.489 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1331.656 | 1674.432 | 0.739 | 12.37× |
| lifecycle | 1 | playwright-chrome | pass | yes | 2750.333 | 3046.696 | 0.362 | 25.54× |
| warm | 1 | playwright-chrome | pass | yes | 808.245 | 1469.282 | 1.197 | 48.89× |
| warm | 1 | playwright-shell | pass | yes | 253.658 | 401.564 | 3.739 | 15.34× |
| warm | 1 | puppeteer-shell | pass | yes | 260.533 | 376.278 | 3.584 | 15.76× |
| warm | 1 | shotium | pass | yes | 16.533 | 76.978 | 31.075 | 1.00× |
| warm | 1 | puppeteer-chrome | pass | yes | 343.101 | 438.523 | 2.899 | 20.75× |
| batch | 1 | puppeteer-chrome | pass | yes | 355.285 | 849.186 | 2.718 | 16.61× |
| batch | 1 | playwright-chrome | pass | yes | 881.41 | 1259.161 | 1.114 | 41.21× |
| batch | 1 | shotium | pass | yes | 21.389 | 264.915 | 23.087 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 323.324 | 657.75 | 2.966 | 15.12× |
| batch | 1 | puppeteer-shell | pass | yes | 346.409 | 696.777 | 2.779 | 16.20× |
| parallel | 1 | playwright-chrome | pass | yes | 1007.242 | 1398.04 | 0.984 | 27.91× |
| parallel | 1 | puppeteer-shell | pass | yes | 422.588 | 1156.885 | 2.198 | 11.71× |
| parallel | 1 | shotium | pass | yes | 36.092 | 265.617 | 17.250 | 1.00× |
| parallel | 1 | puppeteer-chrome | pass | yes | 527.717 | 1076.887 | 1.834 | 14.62× |
| parallel | 1 | playwright-shell | pass | yes | 369.279 | 720.445 | 2.590 | 10.23× |
| parallel | 2 | puppeteer-shell | pass | yes | 489.682 | 993.93 | 3.760 | 5.47× |
| parallel | 2 | shotium | pass | yes | 89.455 | 322.297 | 16.788 | 1.00× |
| parallel | 2 | puppeteer-chrome | fail | no | 685.243 | 1795.711 | 2.531 | N/A |
| parallel | 2 | playwright-shell | pass | yes | 445.899 | 779.654 | 4.246 | 4.98× |
| parallel | 2 | playwright-chrome | fail | no | 1579.439 | 3405.233 | 1.227 | N/A |
| parallel | 4 | shotium | pass | yes | 174.959 | 439.892 | 17.898 | 1.00× |
| parallel | 4 | puppeteer-chrome | fail | no | 716.796 | 5327.134 | 1.513 | N/A |
| parallel | 4 | playwright-shell | pass | yes | 864.661 | 1427.483 | 4.423 | 4.94× |
| parallel | 4 | playwright-chrome | pass | yes | 3052.972 | 6850.632 | 1.313 | 17.45× |
| parallel | 4 | puppeteer-shell | pass | yes | 859.558 | 1750.025 | 4.276 | 4.91× |
| reuse-page | 1 | puppeteer-chrome | pass | no | 241.937 | 361.461 | 4.262 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 240.08 | 370.531 | 4.241 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 224.646 | 261.151 | 4.883 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 238.338 | 359.659 | 4.337 | N/A |
| resident | 1 | playwright-shell | pass | yes | 1545 | 1843 | 0.610 | 2.15× |
| resident | 1 | puppeteer-shell | pass | yes | 1403 | 1608 | 0.703 | 1.95× |
| resident | 1 | puppeteer-chrome | pass | yes | 1427 | 3354 | 0.532 | 1.99× |
| resident | 1 | playwright-chrome | pass | yes | 2585 | 2900 | 0.383 | 3.60× |
| resident | 1 | shotium | pass | yes | 718 | 1178 | 1.306 | 1.00× |
| faults | 1 | playwright-chrome | pass | no | 53393.527 | 53393.527 | N/A | N/A |
| faults | 1 | shotium | pass | no | 7846.3 | 7846.3 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 25920.741 | 25920.741 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 23189.486 | 23189.486 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 19525.026 | 19525.026 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 591.091 | 29333.912 | 3.146 | N/A |
| soak | 4 | playwright-shell | pass | no | 756.22 | 1755.718 | 5.038 | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 6 eligible cell(s), with 6 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 6 / 6 | 6 |
| not ranked (partial coverage) | playwright-shell | 8.993× | 5 | 5 / 6 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 9.369× | 5 | 5 / 6 | 0 |
| not ranked (partial coverage) | playwright-chrome | 17.375× | 5 | 5 / 6 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 21.752× | 5 | 5 / 6 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.7.2/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | fail | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 2446 | 6592 | 0.330 | 40.10× |
| cold | 1 | puppeteer-shell | pass | yes | 966 | 1385 | 0.940 | 15.84× |
| cold | 1 | playwright-shell | pass | yes | 768 | 1374 | 1.173 | 12.59× |
| cold | 1 | puppeteer-chrome | pass | yes | 2477 | 4604 | 0.381 | 40.61× |
| cold | 1 | shotium | pass | yes | 61 | 79 | 16.129 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 10.44 | 15.921 | 90.463 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 198.796 | 294.527 | 5.129 | 19.04× |
| cold-settled | 1 | playwright-chrome | pass | yes | 220.041 | 266.838 | 4.542 | 21.08× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 356.04 | 420.374 | 2.856 | 34.10× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 283.515 | 380.213 | 3.392 | 27.16× |
| lifecycle | 1 | playwright-shell | pass | yes | 672.054 | 1324.757 | 1.328 | 11.62× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1842.427 | 3531.596 | 0.503 | 31.87× |
| lifecycle | 1 | shotium | pass | yes | 57.813 | 144.418 | 15.087 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 750.547 | 1136.512 | 1.260 | 12.98× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1962.38 | 2791.177 | 0.524 | 33.94× |
| warm | 1 | playwright-chrome | noisy | no | 212.29 | 275.171 | 4.570 | N/A |
| warm | 1 | playwright-shell | noisy | no | 174.887 | 250.007 | 5.452 | N/A |
| warm | 1 | puppeteer-shell | noisy | no | 209.767 | 263.666 | 1.007 | N/A |
| warm | 1 | shotium | noisy | no | 11.172 | 20.188 | 73.932 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 335.999 | 501.332 | 2.911 | N/A |
| batch | 1 | puppeteer-chrome | pass | yes | 367.674 | 2118.823 | 2.232 | 23.43× |
| batch | 1 | playwright-chrome | pass | yes | 185.253 | 1384.673 | 3.971 | 11.80× |
| batch | 1 | shotium | pass | yes | 15.695 | 271.875 | 27.453 | 1.00× |
| batch | 1 | playwright-shell | noisy | no | 9152.855 | 9152.855 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 9502.124 | 9502.124 | N/A | N/A |
| parallel | 1 | playwright-chrome | pass | no | 198.759 | 1391.596 | 3.751 | N/A |
| parallel | 1 | puppeteer-shell | pass | no | 222.317 | 503.051 | 4.212 | N/A |
| parallel | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-shell | noisy | no | 10027.562 | 10027.562 | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | 10153.301 | 10153.301 | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 208.063 | 322.549 | 4.772 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 167.831 | 245.644 | 5.726 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 112.861 | 247.236 | 8.243 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 88.418 | 134.678 | 10.567 | N/A |
| resident | 1 | playwright-shell | pass | yes | 960 | 1267 | 1.033 | 2.99× |
| resident | 1 | puppeteer-shell | pass | yes | 689 | 895 | 1.383 | 2.15× |
| resident | 1 | puppeteer-chrome | pass | yes | 1512 | 2777 | 0.583 | 4.71× |
| resident | 1 | playwright-chrome | pass | yes | 1501 | 2162 | 0.656 | 4.68× |
| resident | 1 | shotium | pass | yes | 321 | 536 | 2.727 | 1.00× |
| faults | 1 | playwright-chrome | pass | no | 45196.892 | 45196.892 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6793.238 | 6793.238 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 36306.764 | 36306.764 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 19301.29 | 19301.29 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 13671.529 | 13671.529 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 753.854 | 25441.55 | 2.425 | N/A |
| soak | 4 | playwright-shell | pass | yes | 530.636 | 1451.736 | 7.242 | 7.06× |
| soak | 4 | shotium | pass | yes | 75.17 | 368.223 | 29.284 | 1.00× |
| soak | 4 | puppeteer-shell | pass | yes | 452.912 | 962.971 | 8.516 | 6.03× |
| soak | 4 | playwright-chrome | fail | no | 694.757 | 4393.609 | 5.402 | N/A |

