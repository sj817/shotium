# Shotium 0.4.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `5ac86028f6a02c4540460fe2d95651a1501d276b`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 4 | 7 |
| win32-arm64 | pass | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 2 | 10 |
| darwin-arm64 | noisy | shotium | 3 | 10 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 6.034× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 6.083× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 7.390× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 9.228× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.4.0/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-shell | pass | yes | 562 | 577 | 1.783 | 10.22× |
| cold | 1 | playwright-shell | pass | yes | 742 | 757 | 1.350 | 13.49× |
| cold | 1 | shotium | pass | yes | 55 | 57 | 18.229 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 924 | 1991 | 0.931 | 16.80× |
| cold | 1 | puppeteer-chrome | pass | yes | 787 | 1114 | 1.201 | 14.31× |
| cold-settled | 1 | playwright-shell | pass | yes | 127.959 | 133.121 | 8.042 | 9.29× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 172.667 | 189.272 | 5.804 | 12.53× |
| cold-settled | 1 | playwright-chrome | pass | yes | 146.028 | 158.171 | 7.027 | 10.60× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 143.338 | 144.9 | 7.146 | 10.40× |
| cold-settled | 1 | shotium | pass | yes | 13.78 | 15.6 | 69.974 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 840.067 | 925.792 | 1.181 | 15.01× |
| lifecycle | 1 | playwright-shell | pass | yes | 627.49 | 698.557 | 1.566 | 11.21× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 850.702 | 939.898 | 1.166 | 15.20× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 585.216 | 673.737 | 1.670 | 10.46× |
| lifecycle | 1 | shotium | pass | yes | 55.962 | 97.882 | 15.955 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 133.132 | 150.047 | 7.417 | 9.50× |
| warm | 1 | puppeteer-chrome | pass | yes | 167.845 | 188.987 | 5.967 | 11.98× |
| warm | 1 | playwright-shell | pass | yes | 130.209 | 203.164 | 7.476 | 9.29× |
| warm | 1 | playwright-chrome | pass | yes | 162.711 | 204.659 | 6.034 | 11.61× |
| warm | 1 | shotium | pass | yes | 14.015 | 17.292 | 67.411 | 1.00× |
| batch | 1 | shotium | pass | yes | 22.202 | 262.088 | 23.175 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 181.395 | 416.037 | 4.793 | 8.17× |
| batch | 1 | playwright-chrome | pass | yes | 168.748 | 403.432 | 5.084 | 7.60× |
| batch | 1 | puppeteer-shell | pass | yes | 152.047 | 387.702 | 5.633 | 6.85× |
| batch | 1 | playwright-shell | pass | yes | 141.248 | 372.427 | 5.889 | 6.36× |
| parallel | 1 | puppeteer-chrome | pass | yes | 159.491 | 401.619 | 5.492 | 10.19× |
| parallel | 1 | puppeteer-shell | pass | yes | 124.538 | 359.465 | 6.550 | 7.96× |
| parallel | 1 | playwright-chrome | pass | yes | 149.902 | 404.969 | 5.660 | 9.58× |
| parallel | 1 | shotium | pass | yes | 15.655 | 264.696 | 27.998 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 115.413 | 362.541 | 7.036 | 7.37× |
| parallel | 2 | puppeteer-shell | pass | yes | 185.135 | 418.94 | 9.267 | 4.58× |
| parallel | 2 | playwright-chrome | pass | yes | 210.883 | 727.939 | 7.743 | 5.22× |
| parallel | 2 | shotium | pass | yes | 40.427 | 279.587 | 28.003 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 166.48 | 381.732 | 10.321 | 4.12× |
| parallel | 2 | puppeteer-chrome | fail | no | 194.715 | 776.343 | 4.239 | N/A |
| parallel | 4 | playwright-chrome | pass | yes | 368.966 | 635.845 | 10.158 | 4.22× |
| parallel | 4 | shotium | pass | yes | 87.363 | 317.495 | 28.576 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 293.205 | 569.959 | 12.128 | 3.36× |
| parallel | 4 | puppeteer-chrome | fail | no | 229.42 | 13606.74 | 4.424 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 328.018 | 564.719 | 11.393 | 3.75× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.483 | 117.369 | 11.363 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 99.896 | 100.924 | 10.125 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.002 | 112.48 | 9.936 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.167 | 85.293 | 12.051 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 860 | 917 | 1.201 | 2.10× |
| resident | 1 | puppeteer-shell | pass | yes | 813 | 873 | 1.319 | 1.98× |
| resident | 1 | playwright-shell | pass | yes | 963 | 975 | 1.187 | 2.35× |
| resident | 1 | shotium | pass | yes | 410 | 461 | 2.459 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 991 | 1091 | 0.989 | 2.42× |
| faults | 1 | playwright-chrome | pass | no | 17467.363 | 17467.363 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16775.719 | 16775.719 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10816.174 | 10816.174 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 12557.428 | 12557.428 | N/A | N/A |
| faults | 1 | shotium | pass | no | 11111.498 | 11111.498 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 393.177 | 848.126 | 9.304 | 3.22× |
| soak | 4 | puppeteer-shell | pass | yes | 431.418 | 897.055 | 8.819 | 3.54× |
| soak | 4 | puppeteer-chrome | fail | no | 279.526 | 11211.397 | 4.149 | N/A |
| soak | 4 | playwright-chrome | pass | yes | 492.615 | 874.816 | 7.727 | 4.04× |
| soak | 4 | shotium | pass | yes | 122.016 | 425.343 | 23.695 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.586× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.555× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-arm64@0.4.0/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | pass | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 684 | 706 | 1.461 | 10.69× |
| cold | 1 | playwright-chrome | pass | yes | 869 | 1289 | 1.075 | 13.58× |
| cold | 1 | shotium | pass | yes | 64 | 101 | 14.344 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 156.591 | 164.587 | 6.409 | 4.74× |
| cold-settled | 1 | shotium | pass | yes | 33.063 | 38.258 | 29.557 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 125.128 | 128.527 | 7.958 | 3.78× |
| lifecycle | 1 | playwright-chrome | pass | yes | 773.788 | 839.542 | 1.285 | 11.06× |
| lifecycle | 1 | playwright-shell | pass | yes | 579.501 | 629.307 | 1.709 | 8.29× |
| lifecycle | 1 | shotium | pass | yes | 69.942 | 92.298 | 13.630 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 116.9 | 134.384 | 8.211 | 3.82× |
| warm | 1 | shotium | pass | yes | 30.592 | 38.73 | 31.435 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 150.227 | 215.148 | 6.459 | 4.91× |
| batch | 1 | shotium | pass | yes | 35.806 | 265.821 | 18.563 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 179.211 | 400.522 | 5.052 | 5.01× |
| batch | 1 | playwright-shell | pass | yes | 135.125 | 371.055 | 6.371 | 3.77× |
| parallel | 1 | playwright-shell | pass | yes | 140.853 | 376.819 | 6.034 | 3.70× |
| parallel | 1 | playwright-chrome | pass | yes | 188.57 | 424.891 | 4.797 | 4.96× |
| parallel | 1 | shotium | pass | yes | 38.055 | 268.261 | 17.748 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 258.525 | 523.899 | 7.230 | 3.15× |
| parallel | 2 | shotium | pass | yes | 82.183 | 303.273 | 17.775 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 204.169 | 447.483 | 8.807 | 2.48× |
| parallel | 4 | shotium | pass | yes | 170.676 | 418.303 | 17.852 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 342.135 | 580.904 | 10.579 | 2.00× |
| parallel | 4 | playwright-chrome | pass | yes | 451.765 | 768.453 | 8.467 | 2.65× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.298 | 116.539 | 11.772 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.151 | 90.539 | 12.300 | N/A |
| resident | 1 | playwright-shell | pass | yes | 849 | 868 | 1.187 | 2.10× |
| resident | 1 | playwright-chrome | pass | yes | 909 | 939 | 1.102 | 2.24× |
| resident | 1 | shotium | pass | yes | 405 | 416 | 2.485 | 1.00× |
| faults | 1 | playwright-shell | pass | no | 17271.828 | 17271.828 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 15073.035 | 15073.035 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9968.479 | 9968.479 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 396.254 | 732.228 | 9.700 | 2.37× |
| soak | 4 | shotium | pass | yes | 166.865 | 423.278 | 18.293 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 313.872 | 615.408 | 11.602 | 1.88× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 7 eligible cell(s), with 7 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 7 / 7 | 7 |
| 2 | playwright-shell | 5.631× | 6 | 7 / 7 | 0 |
| 3 | puppeteer-shell | 6.330× | 6 | 7 / 7 | 0 |
| 4 | playwright-chrome | 7.348× | 6 | 7 / 7 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 8.243× | 5 | 5 / 7 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c4`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.4.0\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | fail | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-shell | pass | yes | 1265 | 1354 | 0.853 | 16.87× |
| cold | 1 | playwright-shell | pass | yes | 1028 | 1215 | 1.006 | 13.71× |
| cold | 1 | shotium | pass | yes | 75 | 90 | 13.158 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 1216 | 1699 | 0.781 | 16.21× |
| cold | 1 | puppeteer-chrome | pass | yes | 1258 | 1561 | 0.760 | 16.77× |
| cold-settled | 1 | playwright-shell | pass | yes | 160.017 | 178.777 | 6.250 | 11.42× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 186.356 | 226.156 | 5.120 | 13.30× |
| cold-settled | 1 | playwright-chrome | pass | yes | 157.102 | 180.423 | 6.277 | 11.21× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 169.667 | 193.312 | 5.796 | 12.11× |
| cold-settled | 1 | shotium | pass | yes | 14.014 | 16.863 | 68.448 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1693.433 | 1865.671 | 0.591 | 16.27× |
| lifecycle | 1 | playwright-shell | pass | yes | 887.53 | 1113.018 | 1.117 | 8.53× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2135.459 | 2340.427 | 0.467 | 20.52× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1117.16 | 1426.752 | 0.854 | 10.73× |
| lifecycle | 1 | shotium | pass | yes | 104.083 | 197.656 | 8.652 | 1.00× |
| warm | 1 | puppeteer-shell | pass | no | 170.597 | 188.386 | 5.879 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 206.469 | 218.444 | 0.095 | N/A |
| warm | 1 | playwright-shell | noisy | no | 152.255 | 242.771 | 0.477 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 164.657 | 189.912 | 0.816 | N/A |
| warm | 1 | shotium | noisy | no | 16.15 | 16.812 | 0.120 | N/A |
| batch | 1 | shotium | noisy | no | 7935.783 | 7935.783 | N/A | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 11225.68 | 11225.68 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | 12609.049 | 12609.049 | N/A | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 11806.367 | 11806.367 | N/A | N/A |
| batch | 1 | playwright-shell | pass | no | 193.482 | 471.016 | 4.541 | N/A |
| parallel | 1 | puppeteer-chrome | pass | no | 214.308 | 432.638 | 4.281 | N/A |
| parallel | 1 | puppeteer-shell | pass | no | 183.501 | 421.032 | 4.738 | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 9867.638 | 9867.638 | N/A | N/A |
| parallel | 1 | shotium | noisy | no | 8575.216 | 8575.216 | N/A | N/A |
| parallel | 1 | playwright-shell | pass | no | 180.73 | 405.91 | 4.815 | N/A |
| parallel | 2 | puppeteer-shell | pass | yes | 291.612 | 482.125 | 6.302 | 4.71× |
| parallel | 2 | playwright-chrome | pass | yes | 317.653 | 929.942 | 5.699 | 5.13× |
| parallel | 2 | shotium | pass | yes | 61.95 | 293.05 | 21.960 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 243.777 | 503.139 | 7.283 | 3.94× |
| parallel | 2 | puppeteer-chrome | fail | no | 338.178 | 30286.524 | 0.750 | N/A |
| parallel | 4 | playwright-chrome | pass | yes | 566.463 | 1114.613 | 6.690 | 4.24× |
| parallel | 4 | shotium | pass | yes | 133.566 | 374.435 | 22.169 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 457.41 | 772.585 | 8.895 | 3.42× |
| parallel | 4 | puppeteer-chrome | pass | yes | 331.904 | 17925.287 | 5.467 | 2.48× |
| parallel | 4 | puppeteer-shell | pass | yes | 474.238 | 781.796 | 8.484 | 3.55× |
| reuse-page | 1 | playwright-chrome | pass | no | 99.512 | 106.617 | 10.233 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.404 | 129.311 | 9.575 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 116.001 | 163.122 | 8.506 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 84.2 | 101.61 | 11.445 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 716 | 1259 | 1.246 | 3.35× |
| resident | 1 | puppeteer-shell | pass | yes | 642 | 992 | 1.467 | 3.00× |
| resident | 1 | playwright-shell | pass | yes | 720 | 1041 | 1.225 | 3.36× |
| resident | 1 | shotium | pass | yes | 214 | 737 | 2.995 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 831 | 1388 | 1.005 | 3.88× |
| faults | 1 | playwright-chrome | pass | no | 27535.132 | 27535.132 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 21140.117 | 21140.117 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 24984.357 | 24984.357 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 22187.327 | 22187.327 | N/A | N/A |
| faults | 1 | shotium | pass | no | 7612.113 | 7612.113 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 256.063 | 1763.705 | 13.699 | 2.97× |
| soak | 4 | puppeteer-shell | pass | yes | 320.038 | 632.655 | 11.809 | 3.71× |
| soak | 4 | puppeteer-chrome | fail | no | 251.338 | 30290.104 | 3.351 | N/A |
| soak | 4 | playwright-chrome | pass | yes | 399.774 | 911.34 | 9.604 | 4.63× |
| soak | 4 | shotium | pass | yes | 86.351 | 373.98 | 27.895 | 1.00× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-arm64@0.4.0\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 70 | 73 | 14.199 | N/A |
| cold-settled | 1 | shotium | pass | no | 12.638 | 12.909 | 77.597 | N/A |
| lifecycle | 1 | shotium | pass | no | 101.061 | 131.551 | 9.761 | N/A |
| warm | 1 | shotium | pass | no | 12.598 | 13.854 | 75.038 | N/A |
| batch | 1 | shotium | pass | no | 26.874 | 331.963 | 19.205 | N/A |
| parallel | 1 | shotium | pass | no | 21.455 | 269.826 | 22.780 | N/A |
| parallel | 2 | shotium | pass | no | 48.805 | 288.305 | 24.973 | N/A |
| parallel | 4 | shotium | pass | no | 110.297 | 354.65 | 24.114 | N/A |
| resident | 1 | shotium | pass | no | 364 | 1001 | 2.146 | N/A |
| faults | 1 | shotium | pass | no | 11072.575 | 11072.575 | N/A | N/A |
| soak | 4 | shotium | pass | no | 115.93 | 381.556 | 24.006 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 8.299× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 9.630× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 25.645× | 7 | 8 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 14.046× | 7 | 7 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.4.0/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-shell | pass | yes | 1379 | 1566 | 0.729 | 10.86× |
| cold | 1 | playwright-shell | pass | yes | 1245 | 2337 | 0.671 | 9.80× |
| cold | 1 | shotium | pass | yes | 127 | 313 | 6.585 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 4126 | 4956 | 0.236 | 32.49× |
| cold | 1 | puppeteer-chrome | pass | yes | 2478 | 3866 | 0.363 | 19.51× |
| cold-settled | 1 | playwright-shell | pass | yes | 322.627 | 430.198 | 3.044 | 14.69× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 427.351 | 768.378 | 2.023 | 19.46× |
| cold-settled | 1 | playwright-chrome | pass | yes | 850.751 | 935.514 | 1.170 | 38.75× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 348.23 | 363.898 | 2.907 | 15.86× |
| cold-settled | 1 | shotium | pass | yes | 21.955 | 25.176 | 48.986 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3245.859 | 4383.25 | 0.296 | 22.01× |
| lifecycle | 1 | playwright-shell | pass | yes | 1278.779 | 2358.11 | 0.733 | 8.67× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2940.996 | 4100.563 | 0.338 | 19.95× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1885.469 | 2245.438 | 0.544 | 12.79× |
| lifecycle | 1 | shotium | pass | yes | 147.44 | 262.962 | 6.341 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 267.005 | 389.1 | 3.534 | 22.79× |
| warm | 1 | puppeteer-chrome | pass | yes | 355.516 | 548.985 | 2.865 | 30.35× |
| warm | 1 | playwright-shell | pass | yes | 282.293 | 356.727 | 3.475 | 24.10× |
| warm | 1 | playwright-chrome | pass | yes | 768.753 | 888.406 | 1.300 | 65.62× |
| warm | 1 | shotium | pass | yes | 11.715 | 13.226 | 81.113 | 1.00× |
| batch | 1 | shotium | pass | yes | 22.138 | 267.795 | 21.562 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 353.864 | 662.301 | 2.713 | 15.98× |
| batch | 1 | playwright-chrome | pass | yes | 859.966 | 1281.783 | 1.157 | 38.85× |
| batch | 1 | puppeteer-shell | pass | yes | 343.892 | 729.871 | 2.800 | 15.53× |
| batch | 1 | playwright-shell | pass | yes | 315.712 | 669.469 | 3.078 | 14.26× |
| parallel | 1 | puppeteer-chrome | pass | yes | 382.521 | 867.059 | 2.471 | 13.44× |
| parallel | 1 | puppeteer-shell | pass | yes | 349.643 | 774.146 | 2.606 | 12.28× |
| parallel | 1 | playwright-chrome | pass | yes | 919.647 | 1357.089 | 1.073 | 32.31× |
| parallel | 1 | shotium | pass | yes | 28.462 | 301.536 | 19.114 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 333.26 | 791.646 | 2.813 | 11.71× |
| parallel | 2 | puppeteer-shell | pass | yes | 498.392 | 1131.213 | 3.826 | 7.03× |
| parallel | 2 | playwright-chrome | fail | no | 1518.44 | 2640.591 | 1.258 | N/A |
| parallel | 2 | shotium | pass | yes | 70.907 | 286.813 | 20.082 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 383.236 | 640.355 | 5.120 | 5.40× |
| parallel | 2 | puppeteer-chrome | fail | no | 467.101 | 1505.728 | 3.560 | N/A |
| parallel | 4 | playwright-chrome | pass | yes | 2915.054 | 4491.143 | 1.422 | 21.56× |
| parallel | 4 | shotium | pass | yes | 135.211 | 571.447 | 18.476 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 727.881 | 1211.951 | 5.338 | 5.38× |
| parallel | 4 | puppeteer-chrome | fail | no | 560.59 | 8538.677 | 1.805 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 627.308 | 1162.187 | 5.897 | 4.64× |
| reuse-page | 1 | playwright-chrome | pass | no | 213.525 | 338.35 | 4.671 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 260.054 | 486.729 | 3.437 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 239.844 | 435.14 | 3.967 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 235.894 | 329.511 | 4.322 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 1448 | 1603 | 0.698 | 2.18× |
| resident | 1 | puppeteer-shell | pass | yes | 1512 | 1557 | 0.696 | 2.28× |
| resident | 1 | playwright-shell | pass | yes | 1403 | 1735 | 0.688 | 2.12× |
| resident | 1 | shotium | pass | yes | 663 | 802 | 1.451 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 2520 | 2737 | 0.397 | 3.80× |
| faults | 1 | playwright-chrome | pass | no | 39011.341 | 39011.341 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 22830.454 | 22830.454 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 22305.291 | 22305.291 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 27289.448 | 27289.448 | N/A | N/A |
| faults | 1 | shotium | pass | no | 12110.532 | 12110.532 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 711.692 | 2083.575 | 5.356 | 5.01× |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 740.285 | 27489.847 | 1.804 | N/A |
| soak | 4 | playwright-chrome | fail | no | N/A | N/A | N/A | N/A |
| soak | 4 | shotium | pass | yes | 142.085 | 475.103 | 21.001 | 1.00× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 7.656× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 11.283× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 17.972× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 14.378× | 7 | 8 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.4.0/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | pass | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | fail | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-shell | pass | yes | 686 | 1374 | 1.255 | 12.47× |
| cold | 1 | playwright-shell | pass | yes | 514 | 959 | 1.539 | 9.35× |
| cold | 1 | shotium | pass | yes | 55 | 87 | 16.166 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 1479 | 6396 | 0.461 | 26.89× |
| cold | 1 | puppeteer-chrome | pass | yes | 1597 | 5403 | 0.478 | 29.04× |
| cold-settled | 1 | playwright-shell | pass | yes | 194.048 | 281.869 | 4.982 | 17.52× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 355.343 | 411.382 | 2.836 | 32.09× |
| cold-settled | 1 | playwright-chrome | pass | yes | 243.075 | 333.805 | 4.183 | 21.95× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 243.997 | 477.418 | 3.561 | 22.04× |
| cold-settled | 1 | shotium | pass | yes | 11.073 | 14.388 | 89.952 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 2377.904 | 3623.035 | 0.427 | 26.12× |
| lifecycle | 1 | playwright-shell | pass | yes | 692.248 | 1273.164 | 1.323 | 7.60× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2265.236 | 4169.503 | 0.430 | 24.88× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1051.456 | 1497.105 | 0.977 | 11.55× |
| lifecycle | 1 | shotium | pass | yes | 91.032 | 167.168 | 10.331 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 239.649 | 364.27 | 4.018 | 28.44× |
| warm | 1 | puppeteer-chrome | pass | yes | 355.545 | 457.521 | 2.831 | 42.20× |
| warm | 1 | playwright-shell | pass | yes | 144.876 | 176.988 | 6.710 | 17.20× |
| warm | 1 | playwright-chrome | pass | yes | 180.179 | 308.719 | 5.264 | 21.39× |
| warm | 1 | shotium | pass | yes | 8.425 | 12.425 | 105.853 | 1.00× |
| batch | 1 | shotium | pass | yes | 12.997 | 275.278 | 28.103 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 373.302 | 1393.398 | 2.405 | 28.72× |
| batch | 1 | playwright-chrome | pass | yes | 210.668 | 1308.394 | 3.668 | 16.21× |
| batch | 1 | puppeteer-shell | pass | yes | 281.599 | 665.757 | 3.276 | 21.67× |
| batch | 1 | playwright-shell | pass | yes | 165.534 | 405.147 | 5.382 | 12.74× |
| parallel | 1 | puppeteer-chrome | pass | yes | 343.709 | 1284.915 | 2.586 | 25.92× |
| parallel | 1 | puppeteer-shell | pass | yes | 285.798 | 651.776 | 3.341 | 21.56× |
| parallel | 1 | playwright-chrome | pass | yes | 184.974 | 1243.537 | 4.176 | 13.95× |
| parallel | 1 | shotium | pass | yes | 13.258 | 267.615 | 30.567 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 139.784 | 415.903 | 6.078 | 10.54× |
| parallel | 2 | puppeteer-shell | pass | yes | 324.947 | 677.256 | 5.998 | 10.01× |
| parallel | 2 | playwright-chrome | fail | no | 277.462 | 857.479 | 6.118 | N/A |
| parallel | 2 | shotium | pass | yes | 32.452 | 271.296 | 31.242 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 187.742 | 444.391 | 9.293 | 5.79× |
| parallel | 2 | puppeteer-chrome | pass | yes | 406.98 | 1568.419 | 3.953 | 12.54× |
| parallel | 4 | playwright-chrome | pass | yes | 488.263 | 1264.902 | 7.762 | 7.56× |
| parallel | 4 | shotium | pass | yes | 64.612 | 314.601 | 31.690 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 264.684 | 502.918 | 13.005 | 4.10× |
| parallel | 4 | puppeteer-chrome | pass | yes | 450.962 | 23825.973 | 4.113 | 6.98× |
| parallel | 4 | puppeteer-shell | pass | yes | 383.973 | 663.137 | 10.435 | 5.94× |
| reuse-page | 1 | playwright-chrome | pass | no | 100.959 | 289.007 | 8.887 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 336.535 | 454.999 | 3.134 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 330.514 | 470.476 | 3.118 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 94.885 | 122.162 | 10.207 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 908 | 1280 | 1.051 | 3.07× |
| resident | 1 | puppeteer-shell | pass | yes | 657 | 758 | 1.483 | 2.22× |
| resident | 1 | playwright-shell | pass | yes | 656 | 730 | 1.513 | 2.22× |
| resident | 1 | shotium | pass | yes | 296 | 304 | 3.431 | 1.00× |
| resident | 1 | playwright-chrome | pass | yes | 959 | 1419 | 1.011 | 3.24× |
| faults | 1 | playwright-chrome | pass | no | 25365.156 | 25365.156 | N/A | N/A |
| faults | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 8371.06 | 8371.06 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 16981.184 | 16981.184 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9226.055 | 9226.055 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 286.01 | 743.986 | 13.000 | 4.58× |
| soak | 4 | puppeteer-shell | pass | yes | 374.793 | 840.346 | 10.459 | 6.01× |
| soak | 4 | puppeteer-chrome | fail | no | 409.305 | 6593.48 | 4.305 | N/A |
| soak | 4 | playwright-chrome | fail | no | 429.74 | 3229.179 | 8.623 | N/A |
| soak | 4 | shotium | pass | yes | 62.41 | 366.878 | 31.122 | 1.00× |

