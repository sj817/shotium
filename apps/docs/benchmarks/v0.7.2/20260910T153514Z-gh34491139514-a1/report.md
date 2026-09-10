# Shotium 0.7.2 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `46ffa147ca87be7dc7b40461ef8d3ad517e3dc52`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 5 | 10 |
| linux-arm64 | pass | shotium | 3 | 10 |
| win32-x64 | noisy | shotium | 3 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 9 |
| darwin-arm64 | noisy | shotium | 4 | 6 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 7.028× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-shell | 7.083× | 8 | 10 / 10 | 0 |
| 4 | playwright-chrome | 8.988× | 8 | 10 / 10 | 0 |
| 5 | puppeteer-chrome | 10.325× | 8 | 10 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-linux-x64@0.7.2/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | pass | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 577 | 784 | 1.610 | 14.07× |
| cold | 1 | puppeteer-shell | pass | yes | 408 | 486 | 2.378 | 9.95× |
| cold | 1 | shotium | pass | yes | 41 | 48 | 24.138 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 653 | 890 | 1.468 | 15.93× |
| cold | 1 | playwright-shell | pass | yes | 487 | 506 | 2.044 | 11.88× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 107.227 | 110.96 | 9.268 | 12.17× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 160.091 | 176.285 | 6.265 | 18.17× |
| cold-settled | 1 | shotium | pass | yes | 8.809 | 10.354 | 108.120 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 92.004 | 137.854 | 9.455 | 10.44× |
| cold-settled | 1 | playwright-chrome | pass | yes | 146.266 | 174.519 | 6.917 | 16.60× |
| lifecycle | 1 | playwright-shell | pass | yes | 472.913 | 640.235 | 2.080 | 10.06× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 420.761 | 561.349 | 2.309 | 8.95× |
| lifecycle | 1 | shotium | pass | yes | 47.013 | 70.885 | 20.131 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 654.277 | 826.482 | 1.483 | 13.92× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 708.939 | 941.845 | 1.377 | 15.08× |
| warm | 1 | puppeteer-chrome | pass | yes | 183.983 | 205.931 | 5.373 | 15.56× |
| warm | 1 | playwright-chrome | pass | yes | 159.584 | 195.378 | 6.223 | 13.50× |
| warm | 1 | shotium | pass | yes | 11.821 | 15.172 | 76.907 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 131.142 | 145.727 | 7.771 | 11.09× |
| warm | 1 | puppeteer-shell | pass | yes | 133.26 | 139.215 | 7.482 | 11.27× |
| batch | 1 | playwright-shell | pass | yes | 145.792 | 373.291 | 5.778 | 7.66× |
| batch | 1 | shotium | pass | yes | 19.031 | 258.203 | 24.020 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 223.022 | 444.874 | 3.982 | 11.72× |
| batch | 1 | playwright-chrome | pass | yes | 182.232 | 404.163 | 4.810 | 9.58× |
| batch | 1 | puppeteer-shell | pass | yes | 152.827 | 389.642 | 5.492 | 8.03× |
| parallel | 1 | shotium | pass | yes | 19.492 | 258.483 | 23.990 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 145.577 | 372.469 | 5.738 | 7.47× |
| parallel | 1 | puppeteer-shell | pass | yes | 153.249 | 373.743 | 5.481 | 7.86× |
| parallel | 1 | puppeteer-chrome | pass | yes | 231.573 | 451.101 | 3.939 | 11.88× |
| parallel | 1 | playwright-chrome | pass | yes | 184.507 | 404.19 | 4.858 | 9.47× |
| parallel | 2 | playwright-shell | pass | yes | 233.134 | 457.542 | 7.680 | 4.27× |
| parallel | 2 | puppeteer-shell | pass | yes | 242.687 | 491.864 | 7.203 | 4.44× |
| parallel | 2 | puppeteer-chrome | pass | yes | 395.942 | 632.557 | 4.658 | 7.25× |
| parallel | 2 | playwright-chrome | pass | yes | 302.953 | 559.55 | 6.105 | 5.55× |
| parallel | 2 | shotium | pass | yes | 54.604 | 285.641 | 23.741 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 451.843 | 790.636 | 8.308 | 3.70× |
| parallel | 4 | puppeteer-chrome | pass | yes | 693.789 | 1234.736 | 5.456 | 5.68× |
| parallel | 4 | playwright-chrome | pass | yes | 499.196 | 906.954 | 7.404 | 4.08× |
| parallel | 4 | shotium | pass | yes | 122.219 | 366.193 | 23.762 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 420.604 | 729.283 | 8.740 | 3.44× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.314 | 100.477 | 11.887 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 83.425 | 101.294 | 11.520 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 99.904 | 105.375 | 10.027 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 81.626 | 84.321 | 13.017 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 656 | 768 | 1.544 | 6.69× |
| resident | 1 | playwright-chrome | pass | yes | 768 | 819 | 1.479 | 7.84× |
| resident | 1 | playwright-shell | pass | yes | 747 | 786 | 1.338 | 7.62× |
| resident | 1 | shotium | pass | yes | 98 | 330 | 7.056 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 662 | 696 | 1.508 | 6.76× |
| faults | 1 | shotium | pass | no | 5934.612 | 5934.612 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 11031.37 | 11031.37 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 17535.334 | 17535.334 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16386.996 | 16386.996 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 13342.323 | 13342.323 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 392.531 | 813.033 | 9.359 | 3.31× |
| soak | 4 | puppeteer-shell | pass | yes | 439.33 | 816.731 | 8.744 | 3.71× |
| soak | 4 | shotium | pass | yes | 118.447 | 400.662 | 24.095 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 708.274 | 1197.348 | 5.561 | 5.98× |
| soak | 4 | playwright-chrome | pass | yes | 509.791 | 962.109 | 7.542 | 4.30× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 3.823× | 8 | 10 / 10 | 0 |
| 3 | playwright-chrome | 4.886× | 8 | 10 / 10 | 0 |

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
| cold | 1 | shotium | pass | yes | 62 | 66 | 16.055 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 827 | 925 | 1.183 | 13.34× |
| cold | 1 | playwright-shell | pass | yes | 661 | 693 | 1.511 | 10.66× |
| cold-settled | 1 | shotium | pass | yes | 32.186 | 35.871 | 30.594 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 125.074 | 141.929 | 7.972 | 3.89× |
| cold-settled | 1 | playwright-chrome | pass | yes | 152.003 | 164.417 | 6.548 | 4.72× |
| lifecycle | 1 | shotium | pass | yes | 71.072 | 95.952 | 13.396 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 565.576 | 642.549 | 1.752 | 7.96× |
| lifecycle | 1 | playwright-chrome | pass | yes | 735.405 | 810.597 | 1.342 | 10.35× |
| warm | 1 | shotium | pass | yes | 30.726 | 36.018 | 31.615 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 149.882 | 168.745 | 6.593 | 4.88× |
| warm | 1 | playwright-shell | pass | yes | 116.205 | 133.458 | 8.402 | 3.78× |
| batch | 1 | playwright-chrome | pass | yes | 158.201 | 391.168 | 5.392 | 4.47× |
| batch | 1 | shotium | pass | yes | 35.369 | 268.862 | 18.428 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 134.99 | 358.151 | 6.363 | 3.82× |
| parallel | 1 | playwright-chrome | pass | yes | 166.653 | 391.54 | 5.302 | 4.66× |
| parallel | 1 | shotium | pass | yes | 35.795 | 270.346 | 18.387 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 129.098 | 358.381 | 6.316 | 3.61× |
| parallel | 2 | shotium | pass | yes | 79.536 | 303.509 | 18.561 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 179.339 | 427.849 | 9.334 | 2.25× |
| parallel | 2 | playwright-chrome | pass | yes | 258.644 | 485.369 | 7.316 | 3.25× |
| parallel | 4 | playwright-shell | pass | yes | 303.601 | 547.234 | 11.762 | 1.87× |
| parallel | 4 | playwright-chrome | pass | yes | 408.843 | 665.749 | 9.303 | 2.52× |
| parallel | 4 | shotium | pass | yes | 161.952 | 410.444 | 18.573 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 83.208 | 100.726 | 11.883 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 83.275 | 83.838 | 12.001 | N/A |
| resident | 1 | playwright-shell | pass | yes | 856 | 925 | 1.166 | 4.86× |
| resident | 1 | playwright-chrome | pass | yes | 956 | 981 | 1.049 | 5.43× |
| resident | 1 | shotium | pass | yes | 176 | 293 | 5.560 | 1.00× |
| faults | 1 | shotium | pass | no | 4597.98 | 4597.98 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16323.934 | 16323.934 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 15375.269 | 15375.269 | N/A | N/A |
| soak | 4 | shotium | pass | yes | 162.889 | 431.649 | 18.611 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 427.649 | 787.521 | 9.002 | 2.63× |
| soak | 4 | playwright-shell | pass | yes | 307.972 | 626.719 | 11.959 | 1.89× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 6.242× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-chrome | 9.885× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 6.148× | 6 | 8 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 7.686× | 5 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-chrome: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-x64@0.7.2\node_modules\@shotkit\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | noisy | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 1123 | 1298 | 0.866 | 15.82× |
| cold | 1 | puppeteer-shell | pass | yes | 933 | 987 | 1.061 | 13.14× |
| cold | 1 | shotium | pass | yes | 71 | 74 | 13.972 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 1053 | 1111 | 0.945 | 14.83× |
| cold | 1 | playwright-shell | pass | yes | 813 | 822 | 1.232 | 11.45× |
| cold-settled | 1 | puppeteer-shell | pass | no | 159.625 | 203.232 | 6.058 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 222.265 | 279.115 | 0.484 | N/A |
| cold-settled | 1 | shotium | noisy | no | 16.529 | 17.335 | 60.577 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 162.129 | 168.806 | 6.097 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 157.863 | 174.229 | 0.223 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 855.236 | 1090.014 | 1.138 | 8.11× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1069.26 | 1274.096 | 0.919 | 10.14× |
| lifecycle | 1 | shotium | pass | yes | 105.435 | 176.413 | 8.874 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1657.452 | 1825.216 | 0.597 | 15.72× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2041.023 | 2261.577 | 0.489 | 19.36× |
| warm | 1 | puppeteer-chrome | pass | yes | 255.994 | 355.796 | 3.819 | 18.31× |
| warm | 1 | playwright-chrome | noisy | no | 172.711 | 215.628 | 0.424 | N/A |
| warm | 1 | shotium | pass | yes | 13.98 | 16.56 | 67.573 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 171.186 | 206.284 | 5.770 | 12.25× |
| warm | 1 | puppeteer-shell | noisy | no | 171.431 | 205.77 | 1.299 | N/A |
| batch | 1 | playwright-shell | pass | yes | 186.549 | 417.455 | 4.624 | 7.42× |
| batch | 1 | shotium | pass | yes | 25.145 | 269.86 | 21.500 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 263.281 | 509.352 | 3.453 | 10.47× |
| batch | 1 | playwright-chrome | pass | yes | 197.166 | 412.315 | 4.488 | 7.84× |
| batch | 1 | puppeteer-shell | pass | yes | 184.02 | 482.234 | 4.626 | 7.32× |
| parallel | 1 | shotium | pass | yes | 22.879 | 269.169 | 20.770 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 175.519 | 411.016 | 4.968 | 7.67× |
| parallel | 1 | puppeteer-shell | pass | yes | 195.452 | 423.647 | 4.696 | 8.54× |
| parallel | 1 | puppeteer-chrome | pass | yes | 253.454 | 463.976 | 3.665 | 11.08× |
| parallel | 1 | playwright-chrome | pass | yes | 191.424 | 428.278 | 4.648 | 8.37× |
| parallel | 2 | playwright-shell | pass | yes | 260.008 | 548.727 | 6.798 | 4.35× |
| parallel | 2 | puppeteer-shell | pass | yes | 285.818 | 620.196 | 6.385 | 4.78× |
| parallel | 2 | puppeteer-chrome | pass | yes | 443.037 | 821.913 | 4.214 | 7.41× |
| parallel | 2 | playwright-chrome | pass | yes | 316.245 | 598.065 | 5.943 | 5.29× |
| parallel | 2 | shotium | pass | yes | 59.752 | 311.217 | 22.346 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 487.916 | 813.009 | 8.077 | 3.74× |
| parallel | 4 | puppeteer-chrome | pass | yes | 809.149 | 1267.009 | 4.875 | 6.20× |
| parallel | 4 | playwright-chrome | pass | yes | 531.303 | 1396.091 | 6.790 | 4.07× |
| parallel | 4 | shotium | pass | yes | 130.517 | 385.664 | 22.200 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 432.038 | 849.943 | 8.574 | 3.31× |
| reuse-page | 1 | playwright-chrome | pass | no | 101.219 | 133.649 | 9.494 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 103.271 | 118.485 | 9.525 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 117.284 | 148.779 | 8.440 | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | 9674.577 | 9674.577 | N/A | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 748 | 1182 | 1.191 | 4.67× |
| resident | 1 | playwright-chrome | pass | yes | 769 | 825 | 1.304 | 4.81× |
| resident | 1 | playwright-shell | pass | yes | 741 | 1140 | 1.307 | 4.63× |
| resident | 1 | shotium | pass | yes | 160 | 700 | 4.400 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 658 | 941 | 1.414 | 4.11× |
| faults | 1 | shotium | pass | no | 9632.772 | 9632.772 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 25562.871 | 25562.871 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 30254.812 | 30254.812 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 26731.654 | 26731.654 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 31182.386 | 31182.386 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 427.597 | 858.058 | 8.915 | 3.33× |
| soak | 4 | puppeteer-shell | pass | yes | 427.328 | 869.45 | 8.821 | 3.33× |
| soak | 4 | shotium | pass | yes | 128.282 | 403.162 | 22.767 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 827.128 | 1289.103 | 4.780 | 6.45× |
| soak | 4 | playwright-chrome | noisy | no | 9089.384 | 9089.384 | N/A | N/A |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@shotkit+shotium-win32-arm64@0.7.2\node_modules\@shotkit\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 67 | 73 | 14.768 | N/A |
| cold-settled | 1 | shotium | pass | no | 12.619 | 12.902 | 77.817 | N/A |
| lifecycle | 1 | shotium | pass | no | 95.779 | 124.441 | 10.175 | N/A |
| warm | 1 | shotium | noisy | no | 12.406 | 12.793 | 0.117 | N/A |
| batch | 1 | shotium | pass | no | 20.743 | 561.837 | 21.742 | N/A |
| parallel | 1 | shotium | pass | no | 20.888 | 383.745 | 22.551 | N/A |
| parallel | 2 | shotium | pass | no | 48.585 | 288.887 | 24.470 | N/A |
| parallel | 4 | shotium | pass | no | 117.827 | 365.332 | 23.390 | N/A |
| resident | 1 | shotium | pass | no | 419 | 845 | 2.545 | N/A |
| faults | 1 | shotium | noisy | no | 9694.995 | 9694.995 | N/A | N/A |
| soak | 4 | shotium | noisy | no | 8194.428 | 8194.428 | N/A | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 6.798× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-chrome | 11.557× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 6.601× | 6 | 8 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 18.754× | 5 | 5 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold-settled/c1`, `parallel/c1`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-x64@0.7.2/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 3386 | 10325 | 0.216 | 22.88× |
| cold | 1 | puppeteer-shell | pass | yes | 1580 | 1832 | 0.626 | 10.68× |
| cold | 1 | shotium | pass | yes | 148 | 270 | 6.381 | 1.00× |
| cold | 1 | playwright-chrome | noisy | no | 4486 | 5005 | 0.224 | N/A |
| cold | 1 | playwright-shell | pass | yes | 1578 | 1894 | 0.616 | 10.66× |
| cold-settled | 1 | puppeteer-shell | noisy | no | 353.539 | 964.168 | 0.371 | N/A |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 478.413 | 803.506 | 1.826 | 23.59× |
| cold-settled | 1 | shotium | pass | yes | 20.277 | 43.811 | 42.763 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 318.881 | 495.711 | 2.986 | 15.73× |
| cold-settled | 1 | playwright-chrome | pass | yes | 799.331 | 1257.818 | 1.143 | 39.42× |
| lifecycle | 1 | playwright-shell | noisy | no | 1482.639 | 2214.478 | 0.646 | N/A |
| lifecycle | 1 | puppeteer-shell | noisy | no | 1697.841 | 2862.81 | 0.534 | N/A |
| lifecycle | 1 | shotium | noisy | no | 138.763 | 248.339 | 6.462 | N/A |
| lifecycle | 1 | playwright-chrome | noisy | no | 3731.731 | 4518.767 | 0.265 | N/A |
| lifecycle | 1 | puppeteer-chrome | noisy | no | 3447.094 | 6938.646 | 0.266 | N/A |
| warm | 1 | puppeteer-chrome | pass | yes | 540.19 | 2057.992 | 1.507 | 26.68× |
| warm | 1 | playwright-chrome | pass | yes | 917.468 | 1617.628 | 1.044 | 45.32× |
| warm | 1 | shotium | pass | yes | 20.244 | 33.493 | 44.039 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 294.279 | 496.976 | 3.268 | 14.54× |
| warm | 1 | puppeteer-shell | pass | yes | 355.306 | 496.444 | 2.830 | 17.55× |
| batch | 1 | playwright-shell | pass | yes | 331.506 | 680.183 | 2.765 | 10.06× |
| batch | 1 | shotium | pass | yes | 32.946 | 307.121 | 15.690 | 1.00× |
| batch | 1 | puppeteer-chrome | pass | yes | 495.311 | 813.557 | 1.900 | 15.03× |
| batch | 1 | playwright-chrome | pass | yes | 895.757 | 1696.401 | 1.094 | 27.19× |
| batch | 1 | puppeteer-shell | pass | yes | 353.023 | 727.984 | 2.616 | 10.72× |
| parallel | 1 | shotium | pass | yes | 32.493 | 461.75 | 16.277 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 309.975 | 704.046 | 2.985 | 9.54× |
| parallel | 1 | puppeteer-shell | pass | yes | 344.583 | 628.3 | 2.812 | 10.60× |
| parallel | 1 | puppeteer-chrome | pass | yes | 422.729 | 2246.139 | 2.047 | 13.01× |
| parallel | 1 | playwright-chrome | pass | yes | 898.402 | 1707.913 | 1.098 | 27.65× |
| parallel | 2 | playwright-shell | pass | yes | 448.349 | 748.756 | 4.232 | 6.27× |
| parallel | 2 | puppeteer-shell | pass | yes | 480.934 | 946.005 | 3.920 | 6.73× |
| parallel | 2 | puppeteer-chrome | pass | yes | 887.702 | 1951.23 | 2.108 | 12.41× |
| parallel | 2 | playwright-chrome | fail | no | 1572.957 | 3043.128 | 1.240 | N/A |
| parallel | 2 | shotium | pass | yes | 71.506 | 303.365 | 20.289 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 761.039 | 1405.073 | 4.885 | 4.65× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1769.009 | 3172.194 | 2.185 | 10.81× |
| parallel | 4 | playwright-chrome | fail | no | 3109.688 | 5584.346 | 1.311 | N/A |
| parallel | 4 | shotium | pass | yes | 163.651 | 431.188 | 18.725 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 814.254 | 1720.536 | 4.581 | 4.98× |
| reuse-page | 1 | playwright-chrome | pass | no | 136.661 | 181.5 | 7.303 | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 11097.787 | 11097.787 | N/A | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | 10521.339 | 10521.339 | N/A | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 1558 | 1808 | 0.651 | 1.04× |
| resident | 1 | playwright-chrome | pass | yes | 2591 | 3836 | 0.352 | 1.73× |
| resident | 1 | playwright-shell | pass | yes | 1749 | 2881 | 0.469 | 1.17× |
| resident | 1 | shotium | pass | yes | 1500 | 2874 | 0.639 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 1981 | 2499 | 0.482 | 1.32× |
| faults | 1 | shotium | pass | no | 8583.367 | 8583.367 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 36988.626 | 36988.626 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 40598.448 | 40598.448 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 27337.927 | 27337.927 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 37530.653 | 37530.653 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 581.132 | 1532.298 | 6.599 | 3.64× |
| soak | 4 | puppeteer-shell | pass | yes | 653.944 | 2757.663 | 5.660 | 4.10× |
| soak | 4 | shotium | pass | yes | 159.497 | 465.118 | 19.686 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 1493.612 | 4056.842 | 2.565 | 9.36× |
| soak | 4 | playwright-chrome | fail | no | 2947.761 | 6588.213 | 1.385 | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 6 eligible cell(s), with 6 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 6 / 6 | 6 |
| 2 | playwright-shell | 7.187× | 6 | 6 / 6 | 0 |
| 3 | puppeteer-shell | 9.186× | 6 | 6 / 6 | 0 |
| 4 | puppeteer-chrome | 17.496× | 6 | 6 / 6 | 0 |
| not ranked (partial coverage) | playwright-chrome | 14.586× | 4 | 4 / 6 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@shotkit+shotium-darwin-arm64@0.7.2/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-chrome | pass | yes | 2582 | 8926 | 0.295 | 36.37× |
| cold | 1 | puppeteer-shell | pass | yes | 815 | 1103 | 1.195 | 11.48× |
| cold | 1 | shotium | pass | yes | 71 | 106 | 12.367 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 2154 | 6896 | 0.346 | 30.34× |
| cold | 1 | playwright-shell | pass | yes | 871 | 960 | 1.184 | 12.27× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 247.138 | 278.197 | 4.364 | 25.32× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 454.641 | 704.442 | 2.117 | 46.58× |
| cold-settled | 1 | shotium | pass | yes | 9.761 | 12.184 | 97.279 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 157.687 | 284.152 | 5.632 | 16.15× |
| cold-settled | 1 | playwright-chrome | pass | yes | 219.719 | 342.768 | 4.162 | 22.51× |
| lifecycle | 1 | playwright-shell | pass | yes | 682.775 | 1419.136 | 1.361 | 9.32× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 890.752 | 1226.474 | 1.105 | 12.16× |
| lifecycle | 1 | shotium | pass | yes | 73.248 | 151.132 | 12.211 | 1.00× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1768.203 | 2504.295 | 0.542 | 24.14× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2132.712 | 2979.389 | 0.455 | 29.12× |
| warm | 1 | puppeteer-chrome | pass | no | 340.284 | 481.649 | 2.901 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 207.579 | 346.869 | 4.537 | N/A |
| warm | 1 | shotium | noisy | no | 9.467 | 15.847 | 95.988 | N/A |
| warm | 1 | playwright-shell | pass | no | 166.787 | 554.377 | 4.911 | N/A |
| warm | 1 | puppeteer-shell | pass | no | 211.51 | 331.049 | 4.581 | N/A |
| batch | 1 | playwright-shell | noisy | no | 8769.707 | 8769.707 | N/A | N/A |
| batch | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| batch | 1 | puppeteer-chrome | pass | no | 419.391 | 1791.502 | 2.066 | N/A |
| batch | 1 | playwright-chrome | pass | no | 253.924 | 1697.505 | 3.074 | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | shotium | pass | yes | 15.853 | 263.684 | 25.492 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 168.017 | 408.182 | 5.276 | 10.60× |
| parallel | 1 | puppeteer-shell | pass | yes | 225.26 | 479.64 | 4.251 | 14.21× |
| parallel | 1 | puppeteer-chrome | pass | yes | 342.71 | 1606.649 | 2.480 | 21.62× |
| parallel | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 107.423 | 258.759 | 8.998 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 187.482 | 298.171 | 5.116 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 238.697 | 316.57 | 4.289 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 84.27 | 108.085 | 11.415 | N/A |
| resident | 1 | puppeteer-chrome | pass | yes | 1040 | 1639 | 0.917 | 2.70× |
| resident | 1 | playwright-chrome | pass | yes | 1057 | 1408 | 0.931 | 2.75× |
| resident | 1 | playwright-shell | pass | yes | 696 | 842 | 1.375 | 1.81× |
| resident | 1 | shotium | pass | yes | 385 | 467 | 2.474 | 1.00× |
| resident | 1 | puppeteer-shell | pass | yes | 982 | 1257 | 0.976 | 2.55× |
| faults | 1 | shotium | pass | no | 4889.281 | 4889.281 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 9268.832 | 9268.832 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 26137.999 | 26137.999 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13158.866 | 13158.866 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 17671.914 | 17671.914 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 394.824 | 1196.087 | 9.644 | 3.89× |
| soak | 4 | puppeteer-shell | pass | yes | 475.714 | 1092.458 | 8.057 | 4.69× |
| soak | 4 | shotium | pass | yes | 101.408 | 398.391 | 25.651 | 1.00× |
| soak | 4 | puppeteer-chrome | pass | yes | 1009.893 | 4167.432 | 3.786 | 9.96× |
| soak | 4 | playwright-chrome | fail | no | 606.614 | 3802.592 | 6.187 | N/A |

