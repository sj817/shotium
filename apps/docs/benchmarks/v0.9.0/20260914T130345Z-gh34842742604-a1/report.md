# Shotium 0.9.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `f6ae8e198044efbfdd8c7a991f544add637774c7`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 4 | 9 |
| win32-arm64 | noisy | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 8 |
| darwin-arm64 | noisy | shotium | 4 | 3 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 9.094× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 9.727× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 13.029× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 11.676× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.9.0/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 807 | 842 | 1.235 | 11.70× |
| cold | 1 | puppeteer-shell | pass | yes | 611 | 649 | 1.616 | 8.86× |
| cold | 1 | puppeteer-chrome | pass | yes | 902 | 923 | 1.113 | 13.07× |
| cold | 1 | playwright-chrome | pass | yes | 991 | 1029 | 1.006 | 14.36× |
| cold | 1 | shotium | pass | yes | 69 | 74 | 14.706 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 10.217 | 12.255 | 93.033 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 189.152 | 233.901 | 5.141 | 18.51× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 188.585 | 204.743 | 5.241 | 18.46× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 137.457 | 139.947 | 7.421 | 13.45× |
| cold-settled | 1 | playwright-shell | pass | yes | 143.284 | 171.367 | 6.824 | 14.02× |
| lifecycle | 1 | playwright-chrome | fail | no | 2177.332 | 2367.423 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 62.897 | 103.359 | 14.143 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 643.388 | 687.946 | 1.557 | 10.23× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 936.004 | 985.67 | 1.062 | 14.88× |
| lifecycle | 1 | playwright-shell | pass | yes | 683.655 | 780.488 | 1.449 | 10.87× |
| warm | 1 | puppeteer-chrome | pass | yes | 198.09 | 223.978 | 5.051 | 21.53× |
| warm | 1 | playwright-shell | pass | yes | 149.986 | 174.906 | 6.528 | 16.30× |
| warm | 1 | shotium | pass | yes | 9.199 | 11.302 | 101.042 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 133.486 | 155.557 | 7.243 | 14.51× |
| warm | 1 | playwright-chrome | pass | yes | 180.7 | 210.259 | 5.500 | 19.64× |
| batch | 1 | shotium | pass | yes | 12.472 | 256.289 | 28.674 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 154.026 | 389.389 | 5.421 | 12.35× |
| batch | 1 | puppeteer-chrome | pass | yes | 227.861 | 446.388 | 4.004 | 18.27× |
| batch | 1 | playwright-chrome | pass | yes | 195.58 | 406.044 | 4.599 | 15.68× |
| batch | 1 | puppeteer-shell | pass | yes | 153.092 | 378.623 | 5.580 | 12.27× |
| parallel | 1 | playwright-shell | pass | yes | 161.523 | 398.644 | 5.297 | 13.15× |
| parallel | 1 | shotium | pass | yes | 12.279 | 261.504 | 29.068 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 152.589 | 392.326 | 5.525 | 12.43× |
| parallel | 1 | puppeteer-chrome | pass | yes | 236.572 | 442.611 | 3.939 | 19.27× |
| parallel | 1 | playwright-chrome | pass | yes | 192.84 | 416.166 | 4.587 | 15.70× |
| parallel | 2 | shotium | pass | yes | 39.144 | 272.803 | 28.849 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 243.808 | 478.34 | 7.326 | 6.23× |
| parallel | 2 | puppeteer-chrome | pass | yes | 365.177 | 627.857 | 4.963 | 9.33× |
| parallel | 2 | playwright-chrome | pass | yes | 313.672 | 628.805 | 6.033 | 8.01× |
| parallel | 2 | playwright-shell | pass | yes | 245.663 | 470.999 | 7.389 | 6.28× |
| parallel | 4 | puppeteer-shell | pass | yes | 466.283 | 804.041 | 8.369 | 5.32× |
| parallel | 4 | puppeteer-chrome | pass | yes | 689.341 | 1043.6 | 5.717 | 7.87× |
| parallel | 4 | playwright-chrome | pass | yes | 533.064 | 864.656 | 6.881 | 6.08× |
| parallel | 4 | playwright-shell | pass | yes | 468.565 | 782.595 | 8.277 | 5.35× |
| parallel | 4 | shotium | pass | yes | 87.618 | 338.069 | 28.689 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 100.846 | 122.314 | 9.501 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.121 | 101.047 | 10.066 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 100.584 | 114.303 | 9.705 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.134 | 118.237 | 9.735 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 921 | 944 | 1.166 | 8.69× |
| resident | 1 | playwright-chrome | pass | yes | 1081 | 1141 | 0.969 | 10.20× |
| resident | 1 | puppeteer-chrome | pass | yes | 945 | 983 | 1.191 | 8.92× |
| resident | 1 | playwright-shell | pass | yes | 1000 | 1071 | 0.987 | 9.43× |
| resident | 1 | shotium | pass | yes | 106 | 237 | 7.910 | 1.00× |
| faults | 1 | shotium | pass | no | 5365.347 | 5365.347 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 14867.764 | 14867.764 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 18309.104 | 18309.104 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 10341.183 | 10341.183 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 16550.149 | 16550.149 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 438.773 | 900.213 | 8.727 | 4.98× |
| soak | 4 | shotium | pass | yes | 88.133 | 331.842 | 29.481 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 446.751 | 837.372 | 8.413 | 5.07× |
| soak | 4 | playwright-chrome | pass | yes | 555.671 | 1035.867 | 6.915 | 6.30× |
| soak | 4 | puppeteer-chrome | pass | yes | 697.501 | 1190.148 | 5.579 | 7.91× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.611× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.507× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.9.0/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 664 | 685 | 1.514 | 11.45× |
| cold | 1 | shotium | pass | yes | 58 | 60 | 17.157 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 836 | 863 | 1.191 | 14.41× |
| cold-settled | 1 | shotium | pass | yes | 20.152 | 20.562 | 49.456 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 158.562 | 187.516 | 6.157 | 7.87× |
| cold-settled | 1 | playwright-shell | pass | yes | 143.563 | 146.311 | 7.065 | 7.12× |
| lifecycle | 1 | shotium | pass | yes | 60.711 | 83.004 | 15.658 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 1545.143 | 1583.655 | N/A | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 588.517 | 670.445 | 1.662 | 9.69× |
| warm | 1 | playwright-chrome | pass | yes | 166.777 | 185.415 | 5.866 | 9.04× |
| warm | 1 | playwright-shell | pass | yes | 133.108 | 159.802 | 7.372 | 7.21× |
| warm | 1 | shotium | pass | yes | 18.45 | 21.132 | 51.834 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 146.792 | 387.592 | 5.765 | 5.95× |
| batch | 1 | shotium | pass | yes | 24.684 | 260.769 | 23.368 | 1.00× |
| batch | 1 | playwright-chrome | pass | yes | 188.4 | 409.267 | 4.763 | 7.63× |
| parallel | 1 | playwright-shell | pass | yes | 142.196 | 522.199 | 5.877 | 6.01× |
| parallel | 1 | shotium | pass | yes | 23.645 | 255.937 | 23.890 | 1.00× |
| parallel | 1 | playwright-chrome | pass | yes | 178.443 | 438.28 | 4.906 | 7.55× |
| parallel | 2 | shotium | pass | yes | 49.826 | 293.085 | 23.855 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 257.474 | 518.351 | 7.213 | 5.17× |
| parallel | 2 | playwright-shell | pass | yes | 197.825 | 442.181 | 8.775 | 3.97× |
| parallel | 4 | playwright-chrome | pass | yes | 421.558 | 705.496 | 8.903 | 3.84× |
| parallel | 4 | playwright-shell | pass | yes | 348.357 | 559.328 | 10.950 | 3.17× |
| parallel | 4 | shotium | pass | yes | 109.848 | 368.714 | 23.711 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 99.99 | 119.683 | 9.974 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 99.924 | 100.853 | 10.013 | N/A |
| resident | 1 | playwright-chrome | pass | yes | 976 | 1033 | 1.017 | 4.30× |
| resident | 1 | shotium | pass | yes | 227 | 375 | 4.554 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 884 | 923 | 1.121 | 3.89× |
| faults | 1 | shotium | pass | no | 4578.235 | 4578.235 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 14954.652 | 14954.652 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14501.659 | 14501.659 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 329.305 | 645.687 | 11.089 | 3.09× |
| soak | 4 | playwright-chrome | pass | yes | 442.514 | 1517.22 | 8.766 | 4.15× |
| soak | 4 | shotium | pass | yes | 106.569 | 362.328 | 24.334 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 10.148× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 10.232× | 7 | 9 / 9 | 0 |
| 4 | puppeteer-chrome | 14.905× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 11.988× | 5 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.9.0\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | pass | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | pass | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 1138 | 1232 | 0.926 | 16.74× |
| cold | 1 | puppeteer-shell | pass | yes | 1280 | 1353 | 0.802 | 18.82× |
| cold | 1 | puppeteer-chrome | pass | yes | 1535 | 1623 | 0.665 | 22.57× |
| cold | 1 | playwright-chrome | pass | yes | 1450 | 1593 | 0.723 | 21.32× |
| cold | 1 | shotium | pass | yes | 68 | 75 | 16.317 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 9.503 | 9.933 | 103.698 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 188.251 | 197.744 | 5.405 | 19.81× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 212.67 | 240.399 | 4.669 | 22.38× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 163.679 | 222.292 | 5.702 | 17.22× |
| cold-settled | 1 | playwright-shell | pass | yes | 169.475 | 184.062 | 5.932 | 17.83× |
| lifecycle | 1 | playwright-chrome | fail | no | 6503.409 | 6845.51 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 70.102 | 171.682 | 12.301 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1150.933 | 1404.586 | 0.854 | 16.42× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2167.022 | 2321.282 | 0.461 | 30.91× |
| lifecycle | 1 | playwright-shell | pass | yes | 878.037 | 1174.731 | 1.111 | 12.53× |
| warm | 1 | puppeteer-chrome | pass | yes | 182.949 | 211.117 | 5.416 | 25.07× |
| warm | 1 | playwright-shell | pass | yes | 157.482 | 185.269 | 6.215 | 21.58× |
| warm | 1 | shotium | pass | yes | 7.297 | 9.014 | 126.810 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 133.664 | 150.534 | 7.410 | 18.32× |
| warm | 1 | playwright-chrome | pass | yes | 149.731 | 177.134 | 6.726 | 20.52× |
| batch | 1 | shotium | noisy | no | 8758.048 | 8758.048 | N/A | N/A |
| batch | 1 | playwright-shell | pass | no | 163.087 | 391.121 | 5.406 | N/A |
| batch | 1 | puppeteer-chrome | pass | no | 213.954 | 454.721 | 4.149 | N/A |
| batch | 1 | playwright-chrome | pass | no | 172.639 | 408.641 | 5.167 | N/A |
| batch | 1 | puppeteer-shell | pass | no | 148.873 | 405.412 | 5.620 | N/A |
| parallel | 1 | playwright-shell | pass | yes | 196.801 | 435.191 | 4.477 | 13.03× |
| parallel | 1 | shotium | pass | yes | 15.103 | 273.98 | 26.710 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 180.849 | 409.651 | 4.814 | 11.97× |
| parallel | 1 | puppeteer-chrome | pass | yes | 248.098 | 512.593 | 3.719 | 16.43× |
| parallel | 1 | playwright-chrome | pass | yes | 187.554 | 413.162 | 4.520 | 12.42× |
| parallel | 2 | shotium | pass | yes | 46.633 | 292.543 | 25.108 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 270.565 | 528.831 | 6.728 | 5.80× |
| parallel | 2 | puppeteer-chrome | pass | yes | 411.944 | 698.484 | 4.470 | 8.83× |
| parallel | 2 | playwright-chrome | pass | yes | 312.814 | 617.269 | 5.689 | 6.71× |
| parallel | 2 | playwright-shell | pass | yes | 307.349 | 557.993 | 5.974 | 6.59× |
| parallel | 4 | puppeteer-shell | pass | yes | 459.956 | 866.585 | 8.227 | 5.06× |
| parallel | 4 | puppeteer-chrome | pass | yes | 770.125 | 1178.79 | 5.047 | 8.48× |
| parallel | 4 | playwright-chrome | pass | yes | 567.125 | 1341.994 | 6.908 | 6.24× |
| parallel | 4 | playwright-shell | pass | yes | 443.561 | 734.014 | 8.536 | 4.88× |
| parallel | 4 | shotium | pass | yes | 90.828 | 325.043 | 28.093 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 116.822 | 136.649 | 8.377 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 121.289 | 201.413 | 7.845 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 104.171 | 118.812 | 9.344 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 110.845 | 120.325 | 9.138 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 923 | 1195 | 1.061 | 7.69× |
| resident | 1 | playwright-chrome | pass | yes | 947 | 1327 | 1.015 | 7.89× |
| resident | 1 | puppeteer-chrome | pass | yes | 906 | 1584 | 0.959 | 7.55× |
| resident | 1 | playwright-shell | pass | yes | 833 | 1103 | 1.150 | 6.94× |
| resident | 1 | shotium | pass | yes | 120 | 782 | 4.028 | 1.00× |
| faults | 1 | shotium | pass | no | 10112.813 | 10112.813 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 30621.987 | 30621.987 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 11056.129 | 11056.129 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 29344.668 | 29344.668 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 29000.602 | 29000.602 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 433.759 | 838.459 | 8.724 | 4.66× |
| soak | 4 | shotium | pass | yes | 93.081 | 361.5 | 27.979 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 452.197 | 1192.751 | 8.426 | 4.86× |
| soak | 4 | playwright-chrome | fail | no | 633.677 | 1946.335 | 5.911 | N/A |
| soak | 4 | puppeteer-chrome | pass | yes | 929.403 | 1621.586 | 4.258 | 9.98× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.9.0\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 50 | 91 | 18.041 | N/A |
| cold-settled | 1 | shotium | noisy | no | 8879.133 | 9413.408 | N/A | N/A |
| lifecycle | 1 | shotium | pass | no | 72.963 | 89.836 | 13.330 | N/A |
| warm | 1 | shotium | pass | no | 6.29 | 7.262 | 143.764 | N/A |
| batch | 1 | shotium | pass | no | 11.875 | 267.578 | 28.979 | N/A |
| parallel | 1 | shotium | pass | no | 11.729 | 269.144 | 30.130 | N/A |
| parallel | 2 | shotium | pass | no | 29.374 | 280.848 | 31.453 | N/A |
| parallel | 4 | shotium | pass | no | 68.014 | 317.434 | 31.856 | N/A |
| resident | 1 | shotium | pass | no | 102 | 857 | 4.359 | N/A |
| faults | 1 | shotium | pass | no | 15633.818 | 15633.818 | N/A | N/A |
| soak | 4 | shotium | pass | no | 90.948 | 360.856 | 28.386 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 6 | 8 / 8 | 8 |
| 2 | playwright-shell | 10.146× | 6 | 8 / 8 | 0 |
| 3 | puppeteer-shell | 10.369× | 6 | 8 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 18.151× | 5 | 7 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 26.622× | 4 | 6 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- playwright-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`
- puppeteer-chrome: `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`
- playwright-chrome: `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.9.0/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | noisy | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | pass | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 1688 | 3478 | 0.575 | 17.77× |
| cold | 1 | puppeteer-shell | pass | yes | 1450 | 1731 | 0.693 | 15.26× |
| cold | 1 | puppeteer-chrome | pass | yes | 2952 | 3730 | 0.324 | 31.07× |
| cold | 1 | playwright-chrome | pass | yes | 3222 | 5109 | 0.287 | 33.92× |
| cold | 1 | shotium | pass | yes | 95 | 108 | 11.182 | 1.00× |
| cold-settled | 1 | shotium | pass | yes | 11.971 | 17.682 | 76.255 | 1.00× |
| cold-settled | 1 | playwright-chrome | pass | yes | 836.346 | 1144.172 | 1.148 | 69.86× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 560.57 | 985.152 | 1.526 | 46.83× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 317.95 | 360.893 | 3.148 | 26.56× |
| cold-settled | 1 | playwright-shell | pass | yes | 316.109 | 374.34 | 3.176 | 26.41× |
| lifecycle | 1 | playwright-chrome | fail | no | 4785.524 | 5137.205 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 83.216 | 136.653 | 11.291 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1484.743 | 2182.938 | 0.662 | 17.84× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2500.899 | 3066.569 | 0.394 | 30.05× |
| lifecycle | 1 | playwright-shell | pass | yes | 1055.382 | 1297.914 | 0.920 | 12.68× |
| warm | 1 | puppeteer-chrome | pass | no | 544.291 | 800.197 | 1.875 | N/A |
| warm | 1 | playwright-shell | pass | no | 348.346 | 578.042 | 2.705 | N/A |
| warm | 1 | shotium | noisy | no | 9.872 | 17.893 | 0.681 | N/A |
| warm | 1 | puppeteer-shell | pass | no | 343.613 | 669.682 | 2.805 | N/A |
| warm | 1 | playwright-chrome | pass | no | 881.119 | 1056.734 | 1.118 | N/A |
| batch | 1 | shotium | noisy | no | 8368.589 | 8368.589 | N/A | N/A |
| batch | 1 | playwright-shell | pass | no | 343.321 | 1019.208 | 2.610 | N/A |
| batch | 1 | puppeteer-chrome | pass | no | 499.524 | 1370.88 | 1.881 | N/A |
| batch | 1 | playwright-chrome | pass | no | 950.709 | 1337.513 | 1.030 | N/A |
| batch | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-shell | pass | yes | 314.694 | 718.151 | 2.967 | 17.47× |
| parallel | 1 | shotium | pass | yes | 18.009 | 263.568 | 25.064 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 343.661 | 657.677 | 2.807 | 19.08× |
| parallel | 1 | puppeteer-chrome | pass | yes | 485.89 | 899.084 | 1.918 | 26.98× |
| parallel | 1 | playwright-chrome | pass | yes | 894.788 | 1294.985 | 1.109 | 49.69× |
| parallel | 2 | shotium | pass | yes | 48.988 | 293.018 | 24.630 | 1.00× |
| parallel | 2 | puppeteer-shell | pass | yes | 412.926 | 765.68 | 4.575 | 8.43× |
| parallel | 2 | puppeteer-chrome | pass | yes | 739.394 | 1630.329 | 2.535 | 15.09× |
| parallel | 2 | playwright-chrome | pass | yes | 1504.744 | 3452.994 | 1.296 | 30.72× |
| parallel | 2 | playwright-shell | pass | yes | 394.124 | 686.397 | 4.784 | 8.05× |
| parallel | 4 | puppeteer-shell | pass | yes | 694.322 | 1386.269 | 5.413 | 7.76× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1495.6 | 3068.433 | 2.576 | 16.72× |
| parallel | 4 | playwright-chrome | pass | yes | 2926.494 | 6563.327 | 1.361 | 32.71× |
| parallel | 4 | playwright-shell | pass | yes | 735.745 | 1169.68 | 5.283 | 8.22× |
| parallel | 4 | shotium | pass | yes | 89.462 | 354.725 | 27.043 | 1.00× |
| reuse-page | 1 | playwright-chrome | pass | no | 228.342 | 357.986 | 4.423 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 239.056 | 351.906 | 4.236 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 189.118 | 246.44 | 5.169 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 243.481 | 444.844 | 3.860 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 1484 | 1798 | 0.664 | 2.03× |
| resident | 1 | playwright-chrome | pass | yes | 2200 | 2432 | 0.455 | 3.01× |
| resident | 1 | puppeteer-chrome | pass | yes | 1594 | 2218 | 0.578 | 2.18× |
| resident | 1 | playwright-shell | pass | yes | 1543 | 1666 | 0.642 | 2.11× |
| resident | 1 | shotium | pass | yes | 731 | 943 | 1.346 | 1.00× |
| faults | 1 | shotium | pass | no | 6777.157 | 6777.157 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 37091.108 | 37091.108 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 54195.867 | 54195.867 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 23702.48 | 23702.48 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 22206.17 | 22206.17 | N/A | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 741.677 | 1519.346 | 5.220 | 7.29× |
| soak | 4 | shotium | pass | yes | 101.717 | 384.773 | 26.611 | 1.00× |
| soak | 4 | playwright-shell | pass | yes | 786.837 | 1729.069 | 4.950 | 7.74× |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 3 eligible cell(s), with 3 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 3 | 3 / 3 | 3 |
| 2 | playwright-shell | 15.526× | 3 | 3 / 3 | 0 |
| 3 | puppeteer-shell | 25.077× | 3 | 3 / 3 | 0 |
| 4 | puppeteer-chrome | 43.548× | 3 | 3 / 3 | 0 |
| not ranked (partial coverage) | playwright-chrome | 26.764× | 2 | 2 / 3 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `lifecycle/c1`, `warm/c1`
- playwright-shell: `batch/c1`, `lifecycle/c1`, `warm/c1`
- puppeteer-shell: `batch/c1`, `lifecycle/c1`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `lifecycle/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.9.0/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | noisy | no | 1052 | 1541 | 0.906 | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 1066.5 | 1680 | 0.841 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | 2503.5 | 6403 | 0.303 | N/A |
| cold | 1 | playwright-chrome | noisy | no | 2433 | 5184 | 0.336 | N/A |
| cold | 1 | shotium | noisy | no | 71 | 86 | 14.245 | N/A |
| cold-settled | 1 | shotium | noisy | no | 7.866 | 34.633 | 75.257 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 281.942 | 455.878 | 3.243 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 559.154 | 684.086 | 1.810 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 380.955 | 407.634 | 2.612 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 210.253 | 311.398 | 4.488 | N/A |
| lifecycle | 1 | playwright-chrome | fail | no | 4804.694 | 5312.375 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 85.62 | 167.214 | 10.937 | 1.00× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1122.664 | 1532.327 | 0.855 | 13.11× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2770.94 | 3767.044 | 0.356 | 32.36× |
| lifecycle | 1 | playwright-shell | pass | yes | 886.625 | 1848.577 | 1.020 | 10.36× |
| warm | 1 | puppeteer-chrome | pass | yes | 396.55 | 573.893 | 2.435 | 64.28× |
| warm | 1 | playwright-shell | pass | yes | 136.618 | 309.003 | 6.654 | 22.15× |
| warm | 1 | shotium | pass | yes | 6.169 | 15.07 | 129.462 | 1.00× |
| warm | 1 | puppeteer-shell | pass | yes | 230.479 | 377.463 | 4.093 | 37.36× |
| warm | 1 | playwright-chrome | pass | yes | 216.313 | 583.635 | 4.065 | 35.06× |
| batch | 1 | shotium | pass | yes | 8.656 | 267.807 | 32.412 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 141.277 | 408.733 | 6.051 | 16.32× |
| batch | 1 | puppeteer-chrome | pass | yes | 343.632 | 1656.517 | 2.483 | 39.70× |
| batch | 1 | playwright-chrome | pass | yes | 176.829 | 1250.391 | 4.082 | 20.43× |
| batch | 1 | puppeteer-shell | pass | yes | 278.636 | 642.01 | 3.349 | 32.19× |
| parallel | 1 | playwright-shell | pass | no | 142.041 | 391.194 | 6.103 | N/A |
| parallel | 1 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 135.432 | 314.327 | 6.706 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 254.249 | 368.949 | 3.892 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 104.337 | 153.613 | 9.405 | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | 8562.182 | 8562.182 | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| resident | 1 | puppeteer-chrome | pass | no | 1477 | 2246 | 0.672 | N/A |
| resident | 1 | playwright-shell | noisy | no | 12974.665 | 12974.665 | N/A | N/A |
| resident | 1 | shotium | noisy | no | 10391.745 | 10391.745 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4704.091 | 4704.091 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 21755.849 | 21755.849 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 11282.693 | 11282.693 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 9654.451 | 9654.451 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14259.799 | 14259.799 | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |

