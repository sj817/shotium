# Shotium 0.12.0 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **complete**; quality: **noisy**; evidence: **complete**. Profile **full**, seed `2b868b0eb4dfe358be8c6671e77b08f981b72689`.

Conclusion: all platform outputs exist, but quality is noisy. Only rows marked pass and ranking-eligible are used; 5 platform(s) contain valid comparisons.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | pass | shotium | 4 | 10 |
| linux-arm64 | pass | shotium | 2 | 10 |
| win32-x64 | noisy | shotium | 3 | 10 |
| win32-arm64 | pass | no valid ranking | 0 | 0 |
| darwin-x64 | noisy | shotium | 3 | 9 |
| darwin-arm64 | noisy | shotium | 2 | 8 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | puppeteer-shell | 7.771× | 8 | 10 / 10 | 0 |
| 3 | playwright-shell | 8.343× | 8 | 10 / 10 | 0 |
| 4 | puppeteer-chrome | 11.077× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 9.963× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-x64@0.12.0/node_modules/@pixel.js/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | pass | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | fail | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-shell | pass | yes | 474 | 485 | 2.123 | 8.32× |
| cold | 1 | playwright-chrome | pass | yes | 788 | 797 | 1.274 | 13.82× |
| cold | 1 | playwright-shell | pass | yes | 627 | 1110 | 1.452 | 11.00× |
| cold | 1 | shotium | pass | yes | 57 | 63 | 17.413 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 672 | 677 | 1.496 | 11.79× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 120.004 | 132.423 | 8.313 | 10.22× |
| cold-settled | 1 | playwright-chrome | pass | yes | 164.88 | 168.831 | 6.211 | 14.04× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 159.125 | 173.951 | 6.169 | 13.55× |
| cold-settled | 1 | shotium | pass | yes | 11.745 | 12.701 | 85.947 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 120.948 | 124.492 | 8.418 | 10.30× |
| lifecycle | 1 | playwright-chrome | fail | no | 2377.399 | 2827.5 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 57.926 | 76.558 | 16.419 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 545.455 | 635.342 | 1.767 | 9.42× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 760.494 | 864.17 | 1.311 | 13.13× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 516.55 | 586.573 | 1.923 | 8.92× |
| warm | 1 | puppeteer-chrome | pass | yes | 184.16 | 199.953 | 5.390 | 20.12× |
| warm | 1 | playwright-chrome | pass | yes | 168.762 | 212.534 | 5.647 | 18.43× |
| warm | 1 | puppeteer-shell | pass | yes | 133.228 | 149.601 | 7.469 | 14.55× |
| warm | 1 | shotium | pass | yes | 9.155 | 12.89 | 97.718 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 148.459 | 173.339 | 6.802 | 16.22× |
| batch | 1 | playwright-shell | pass | yes | 162.14 | 388.759 | 5.410 | 12.59× |
| batch | 1 | playwright-chrome | pass | yes | 191.93 | 421.304 | 4.669 | 14.91× |
| batch | 1 | puppeteer-shell | pass | yes | 149.986 | 388.493 | 5.643 | 11.65× |
| batch | 1 | puppeteer-chrome | pass | yes | 227.974 | 452.665 | 4.066 | 17.71× |
| batch | 1 | shotium | pass | yes | 12.875 | 262.486 | 27.812 | 1.00× |
| parallel | 1 | shotium | pass | yes | 13.486 | 257.729 | 28.222 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 152.667 | 390.288 | 5.533 | 11.32× |
| parallel | 1 | playwright-shell | pass | yes | 166.878 | 389.229 | 5.275 | 12.37× |
| parallel | 1 | playwright-chrome | pass | yes | 200.566 | 441.426 | 4.436 | 14.87× |
| parallel | 1 | puppeteer-chrome | pass | yes | 225.802 | 452.233 | 3.982 | 16.74× |
| parallel | 2 | puppeteer-shell | pass | yes | 245.761 | 499.331 | 7.317 | 6.04× |
| parallel | 2 | playwright-shell | pass | yes | 235.445 | 477.737 | 7.444 | 5.78× |
| parallel | 2 | playwright-chrome | pass | yes | 299.412 | 596.854 | 6.031 | 7.35× |
| parallel | 2 | puppeteer-chrome | pass | yes | 383.779 | 628.18 | 4.880 | 9.42× |
| parallel | 2 | shotium | pass | yes | 40.722 | 275.264 | 28.026 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 452.147 | 769.463 | 8.380 | 4.80× |
| parallel | 4 | playwright-chrome | pass | yes | 545.044 | 896.644 | 6.954 | 5.79× |
| parallel | 4 | puppeteer-chrome | pass | yes | 696.169 | 1050.807 | 5.581 | 7.39× |
| parallel | 4 | shotium | pass | yes | 94.207 | 330.433 | 28.232 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 435.414 | 791.468 | 8.545 | 4.62× |
| reuse-page | 1 | playwright-chrome | pass | no | 101.355 | 118.34 | 9.468 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 100.28 | 127.123 | 9.703 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.036 | 115.295 | 9.904 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 100.027 | 125.553 | 9.636 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 903 | 930 | 1.173 | 4.20× |
| resident | 1 | playwright-chrome | pass | yes | 1087 | 1129 | 0.933 | 5.06× |
| resident | 1 | shotium | pass | yes | 215 | 353 | 4.881 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 930 | 974 | 1.100 | 4.33× |
| resident | 1 | playwright-shell | pass | yes | 974 | 1054 | 1.114 | 4.53× |
| faults | 1 | playwright-chrome | pass | no | 15448.677 | 15448.677 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 10810.441 | 10810.441 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14896.767 | 14896.767 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 9690.665 | 9690.665 | N/A | N/A |
| faults | 1 | shotium | pass | no | 5000.508 | 5000.508 | N/A | N/A |
| soak | 4 | puppeteer-chrome | pass | yes | 618.449 | 1146.328 | 6.210 | 7.38× |
| soak | 4 | shotium | pass | yes | 83.806 | 336.85 | 30.086 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 474.973 | 892.947 | 7.875 | 5.67× |
| soak | 4 | playwright-shell | pass | yes | 404.008 | 784.398 | 9.228 | 4.82× |
| soak | 4 | puppeteer-shell | pass | yes | 394.861 | 807.54 | 9.567 | 4.71× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 5.058× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 6.012× | 7 | 9 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-linux-arm64@0.12.0/node_modules/@pixel.js/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | fail | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-shell | pass | yes | 595 | 618 | 1.675 | 9.60× |
| cold | 1 | shotium | pass | yes | 62 | 66 | 16.055 | 1.00× |
| cold | 1 | playwright-chrome | pass | yes | 769 | 786 | 1.305 | 12.40× |
| cold-settled | 1 | shotium | pass | yes | 21.173 | 21.542 | 46.991 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 133.438 | 145.659 | 7.339 | 6.30× |
| cold-settled | 1 | playwright-chrome | pass | yes | 154.204 | 162.258 | 6.453 | 7.28× |
| lifecycle | 1 | playwright-shell | pass | yes | 559.183 | 616.553 | 1.762 | 8.08× |
| lifecycle | 1 | shotium | pass | yes | 69.178 | 82.297 | 14.190 | 1.00× |
| lifecycle | 1 | playwright-chrome | fail | no | 1591.415 | 1822.386 | N/A | N/A |
| warm | 1 | playwright-shell | pass | yes | 132.933 | 147.997 | 7.419 | 7.41× |
| warm | 1 | playwright-chrome | pass | yes | 166.574 | 186.825 | 5.840 | 9.28× |
| warm | 1 | shotium | pass | yes | 17.949 | 21.974 | 52.213 | 1.00× |
| batch | 1 | shotium | pass | yes | 23.816 | 263.516 | 23.870 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 145.492 | 389.567 | 5.862 | 6.11× |
| batch | 1 | playwright-chrome | pass | yes | 187.898 | 406.262 | 4.757 | 7.89× |
| parallel | 1 | playwright-shell | pass | yes | 143.329 | 383.814 | 5.955 | 6.01× |
| parallel | 1 | playwright-chrome | pass | yes | 183.488 | 407.204 | 4.863 | 7.69× |
| parallel | 1 | shotium | pass | yes | 23.849 | 259.974 | 23.673 | 1.00× |
| parallel | 2 | playwright-chrome | pass | yes | 267.839 | 532.082 | 6.855 | 5.48× |
| parallel | 2 | shotium | pass | yes | 48.895 | 286.189 | 23.905 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 200.98 | 455.877 | 8.698 | 4.11× |
| parallel | 4 | shotium | pass | yes | 109.768 | 356.572 | 23.537 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 354.23 | 569.727 | 10.576 | 3.23× |
| parallel | 4 | playwright-chrome | pass | yes | 428.953 | 704.07 | 8.768 | 3.91× |
| reuse-page | 1 | playwright-shell | pass | no | 99.848 | 104.173 | 10.003 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 100.129 | 232.154 | 8.015 | N/A |
| resident | 1 | shotium | pass | yes | 440 | 453 | 2.283 | 1.00× |
| resident | 1 | playwright-shell | pass | yes | 935 | 1016 | 1.059 | 2.13× |
| resident | 1 | playwright-chrome | pass | yes | 988 | 1046 | 1.000 | 2.25× |
| faults | 1 | playwright-chrome | pass | no | 15480.721 | 15480.721 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15830.112 | 15830.112 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4989.304 | 4989.304 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 456.828 | 821.668 | 8.422 | 4.20× |
| soak | 4 | playwright-shell | pass | yes | 318.704 | 688.531 | 11.362 | 2.93× |
| soak | 4 | shotium | pass | yes | 108.898 | 374.213 | 24.161 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 10 eligible cell(s), with 10 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 10 / 10 | 10 |
| 2 | playwright-shell | 10.739× | 8 | 10 / 10 | 0 |
| 3 | puppeteer-chrome | 15.858× | 8 | 10 / 10 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 11.056× | 7 | 9 / 10 | 0 |
| not ranked (partial coverage) | playwright-chrome | 13.576× | 6 | 8 / 10 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | x64; D:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-x64@0.12.0\node_modules\@pixel.js\shotium-win32-x64\shotium.node |
| puppeteer-shell | noisy | x64; C:\Users\runneradmin\.cache\puppeteer\chrome-headless-shell\win64-152.0.7977.42\chrome-headless-shell-win64\chrome-headless-shell.exe |
| puppeteer-chrome | pass | x64; C:\Users\runneradmin\.cache\puppeteer\chrome\win64-152.0.7977.42\chrome-win64\chrome.exe |
| playwright-shell | pass | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium_headless_shell-1234\chrome-headless-shell-win64\chrome-headless-shell.exe |
| playwright-chrome | fail | x64; C:\Users\runneradmin\AppData\Local\ms-playwright\chromium-1234\chrome-win64\chrome.exe |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-shell | pass | yes | 1044 | 1193 | 0.947 | 17.69× |
| cold | 1 | playwright-chrome | pass | yes | 1283 | 1427 | 0.761 | 21.75× |
| cold | 1 | playwright-shell | pass | yes | 1010 | 1215 | 0.949 | 17.12× |
| cold | 1 | shotium | pass | yes | 59 | 97 | 15.453 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 1267 | 1614 | 0.767 | 21.47× |
| cold-settled | 1 | puppeteer-shell | noisy | no | 169.433 | 181.314 | 0.179 | N/A |
| cold-settled | 1 | playwright-chrome | pass | yes | 211.636 | 296.809 | 4.309 | 17.48× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 214.94 | 381.172 | 4.122 | 17.75× |
| cold-settled | 1 | shotium | pass | yes | 12.108 | 22.756 | 68.335 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 180.968 | 201.057 | 5.496 | 14.95× |
| lifecycle | 1 | playwright-chrome | fail | no | 6905.245 | 7286.974 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 79.934 | 173.805 | 11.106 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 942.81 | 1263.849 | 1.055 | 11.79× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2307.469 | 2584.581 | 0.427 | 28.87× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1199.537 | 1434.99 | 0.829 | 15.01× |
| warm | 1 | puppeteer-chrome | pass | yes | 214.913 | 247.018 | 4.730 | 24.88× |
| warm | 1 | playwright-chrome | pass | yes | 168.105 | 201.228 | 5.780 | 19.46× |
| warm | 1 | puppeteer-shell | pass | yes | 161.475 | 187.519 | 6.102 | 18.69× |
| warm | 1 | shotium | pass | yes | 8.638 | 12.675 | 99.779 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 167.163 | 203.853 | 5.866 | 19.35× |
| batch | 1 | playwright-shell | pass | yes | 202.596 | 450.139 | 4.395 | 12.85× |
| batch | 1 | playwright-chrome | pass | yes | 198.953 | 429.859 | 4.473 | 12.62× |
| batch | 1 | puppeteer-shell | pass | yes | 184.972 | 425.228 | 4.727 | 11.73× |
| batch | 1 | puppeteer-chrome | pass | yes | 248.655 | 479.556 | 3.679 | 15.77× |
| batch | 1 | shotium | pass | yes | 15.767 | 284.535 | 26.623 | 1.00× |
| parallel | 1 | shotium | pass | yes | 15.158 | 260.694 | 26.330 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 187.363 | 429 | 4.583 | 12.36× |
| parallel | 1 | playwright-shell | pass | yes | 196.594 | 418.049 | 4.437 | 12.97× |
| parallel | 1 | playwright-chrome | pass | yes | 215.868 | 409.971 | 4.228 | 14.24× |
| parallel | 1 | puppeteer-chrome | pass | yes | 260.207 | 509.244 | 3.550 | 17.17× |
| parallel | 2 | puppeteer-shell | pass | yes | 310.314 | 565.134 | 5.878 | 7.06× |
| parallel | 2 | playwright-shell | pass | yes | 259.26 | 480.973 | 6.987 | 5.90× |
| parallel | 2 | playwright-chrome | pass | yes | 332.766 | 627.089 | 5.684 | 7.57× |
| parallel | 2 | puppeteer-chrome | pass | yes | 464.198 | 779.988 | 4.097 | 10.57× |
| parallel | 2 | shotium | pass | yes | 43.933 | 274.477 | 27.329 | 1.00× |
| parallel | 4 | playwright-shell | pass | yes | 471.72 | 782.616 | 8.200 | 4.40× |
| parallel | 4 | playwright-chrome | pass | yes | 612.251 | 1233.263 | 6.011 | 5.71× |
| parallel | 4 | puppeteer-chrome | pass | yes | 863.233 | 1588.559 | 4.389 | 8.04× |
| parallel | 4 | shotium | pass | yes | 107.316 | 361.644 | 25.757 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 475.051 | 834.124 | 8.004 | 4.43× |
| reuse-page | 1 | playwright-chrome | pass | no | 116.456 | 171.403 | 8.352 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 112.215 | 134.046 | 8.683 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 100.443 | 118.808 | 9.575 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 108.036 | 145.881 | 8.982 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 1001 | 1314 | 1.064 | 22.24× |
| resident | 1 | playwright-chrome | pass | yes | 904 | 1184 | 1.129 | 20.09× |
| resident | 1 | shotium | pass | yes | 45 | 383 | 10.955 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 825 | 1241 | 1.113 | 18.33× |
| resident | 1 | playwright-shell | pass | yes | 790 | 1570 | 1.119 | 17.56× |
| faults | 1 | playwright-chrome | pass | no | 29745.741 | 29745.741 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 32458.568 | 32458.568 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 24659.925 | 24659.925 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 27287.852 | 27287.852 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9678.744 | 9678.744 | N/A | N/A |
| soak | 4 | puppeteer-chrome | pass | yes | 852.778 | 1499.594 | 4.649 | 8.71× |
| soak | 4 | shotium | pass | yes | 97.953 | 374.748 | 27.234 | 1.00× |
| soak | 4 | playwright-chrome | fail | no | 600.191 | 3176.223 | 6.156 | N/A |
| soak | 4 | playwright-shell | pass | yes | 450.958 | 972.911 | 8.422 | 4.60× |
| soak | 4 | puppeteer-shell | pass | yes | 482.968 | 992.166 | 8.026 | 4.93× |

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | pass | arm64; C:\a\shotium\shotium\apps\benchmark\node_modules\.pnpm\@pixel.js+shotium-win32-arm64@0.12.0\node_modules\@pixel.js\shotium-win32-arm64\shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |
| playwright-chrome | n/a | startup: the package currently supplies an x64 browser on Windows arm64; throughput: the package currently supplies an x64 browser on Windows arm64; parallel: the package currently supplies an x64 browser on Windows arm64; resident: the package currently supplies an x64 browser on Windows arm64; resilience: the package currently supplies an x64 browser on Windows arm64 |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | shotium | pass | no | 55 | 60 | 17.857 | N/A |
| cold-settled | 1 | shotium | pass | no | 8.782 | 9.681 | 109.618 | N/A |
| lifecycle | 1 | shotium | pass | no | 79.733 | 171.036 | 11.681 | N/A |
| warm | 1 | shotium | pass | no | 6.327 | 9.508 | 134.331 | N/A |
| batch | 1 | shotium | pass | no | 12.01 | 268.767 | 29.689 | N/A |
| parallel | 1 | shotium | pass | no | 11.843 | 266.028 | 29.074 | N/A |
| parallel | 2 | shotium | pass | no | 29.505 | 281.038 | 31.871 | N/A |
| parallel | 4 | shotium | pass | no | 70.408 | 316.781 | 31.772 | N/A |
| resident | 1 | shotium | pass | no | 314 | 537 | 3.420 | N/A |
| faults | 1 | shotium | pass | no | 9947.53 | 9947.53 | N/A | N/A |
| soak | 4 | shotium | pass | no | 73.057 | 363.297 | 30.643 | N/A |

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 9 / 9 | 9 |
| 2 | playwright-shell | 10.377× | 8 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 11.591× | 8 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 17.250× | 7 | 7 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 31.643× | 6 | 7 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c4`, `resident/c1`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `parallel/c1`, `parallel/c4`, `resident/c1`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-x64@0.12.0/node_modules/@pixel.js/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-shell | pass | yes | 1120 | 1192 | 0.886 | 13.49× |
| cold | 1 | playwright-chrome | pass | yes | 2762 | 3223 | 0.360 | 33.28× |
| cold | 1 | playwright-shell | pass | yes | 1010 | 1132 | 0.981 | 12.17× |
| cold | 1 | shotium | pass | yes | 83 | 109 | 11.345 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 2085 | 3525 | 0.432 | 25.12× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 317.03 | 360.182 | 3.314 | 26.85× |
| cold-settled | 1 | playwright-chrome | pass | yes | 791.779 | 1576.656 | 1.095 | 67.05× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 466.643 | 706.258 | 2.190 | 39.52× |
| cold-settled | 1 | shotium | pass | yes | 11.808 | 16.173 | 83.276 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 304.433 | 381.724 | 3.272 | 25.78× |
| lifecycle | 1 | playwright-chrome | fail | no | 3964.102 | 5680.149 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 97.629 | 159.688 | 9.766 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 1042.619 | 2135.753 | 0.927 | 10.68× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2251.52 | 2545.705 | 0.441 | 23.06× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1358.247 | 1745.038 | 0.736 | 13.91× |
| warm | 1 | puppeteer-chrome | pass | yes | 501.195 | 1601.688 | 1.459 | 44.82× |
| warm | 1 | playwright-chrome | pass | yes | 860.222 | 1033.527 | 1.153 | 76.92× |
| warm | 1 | puppeteer-shell | pass | yes | 346.217 | 454.208 | 3.019 | 30.96× |
| warm | 1 | shotium | pass | yes | 11.183 | 18.969 | 82.995 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 301.272 | 630.502 | 3.095 | 26.94× |
| batch | 1 | playwright-shell | pass | yes | 308.548 | 717.2 | 2.953 | 14.43× |
| batch | 1 | playwright-chrome | pass | yes | 865.352 | 1237.99 | 1.140 | 40.47× |
| batch | 1 | puppeteer-shell | pass | yes | 349.975 | 728.841 | 2.742 | 16.37× |
| batch | 1 | puppeteer-chrome | pass | yes | 476.924 | 1679.981 | 1.907 | 22.31× |
| batch | 1 | shotium | pass | yes | 21.38 | 261.122 | 23.054 | 1.00× |
| parallel | 1 | shotium | pass | yes | 15.498 | 258.878 | 25.143 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 344.237 | 666.887 | 2.821 | 22.21× |
| parallel | 1 | playwright-shell | pass | yes | 313.433 | 687.591 | 3.103 | 20.22× |
| parallel | 1 | playwright-chrome | pass | yes | 882.33 | 1378.727 | 1.121 | 56.93× |
| parallel | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | pass | yes | 637.212 | 1657.235 | 5.867 | 5.35× |
| parallel | 4 | playwright-chrome | pass | yes | 2849.149 | 4833.125 | 1.426 | 23.94× |
| parallel | 4 | puppeteer-chrome | pass | yes | 1083.36 | 2957.119 | 3.230 | 9.10× |
| parallel | 4 | shotium | pass | yes | 119.03 | 356.765 | 23.722 | 1.00× |
| parallel | 4 | puppeteer-shell | pass | yes | 717.926 | 1619.792 | 5.056 | 6.03× |
| reuse-page | 1 | playwright-chrome | pass | no | 206.231 | 315.884 | 4.631 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 236.594 | 398.948 | 3.850 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 246.453 | 433.626 | 3.529 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 244.616 | 362.282 | 4.013 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 1731 | 2046 | 0.572 | 2.61× |
| resident | 1 | playwright-chrome | pass | yes | 2221 | 2514 | 0.450 | 3.35× |
| resident | 1 | shotium | pass | yes | 662 | 815 | 1.475 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 1444 | 2525 | 0.623 | 2.18× |
| resident | 1 | playwright-shell | pass | yes | 1362 | 1558 | 0.704 | 2.06× |
| faults | 1 | playwright-chrome | pass | no | 30825.008 | 30825.008 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 19539.392 | 19539.392 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 19337.881 | 19337.881 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 15971.277 | 15971.277 | N/A | N/A |
| faults | 1 | shotium | pass | no | 6385.75 | 6385.75 | N/A | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 1007.269 | 1802.067 | 3.800 | N/A |
| soak | 4 | shotium | pass | yes | 127.09 | 529.872 | 22.790 | 1.00× |
| soak | 4 | playwright-chrome | fail | no | 3042.258 | 6390.271 | 1.318 | N/A |
| soak | 4 | playwright-shell | pass | yes | 611.219 | 1755.779 | 6.207 | 4.81× |
| soak | 4 | puppeteer-shell | pass | yes | 536.197 | 1073.255 | 7.198 | 4.22× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 8 eligible cell(s), with 8 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 8 | 8 / 8 | 8 |
| 2 | puppeteer-shell | 15.717× | 8 | 8 / 8 | 0 |
| not ranked (partial coverage) | playwright-shell | 11.240× | 7 | 7 / 8 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 22.944× | 7 | 7 / 8 | 0 |
| not ranked (partial coverage) | playwright-chrome | 16.270× | 6 | 6 / 8 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `resident/c1`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `resident/c1`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/.pnpm/@pixel.js+shotium-darwin-arm64@0.12.0/node_modules/@pixel.js/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | puppeteer-shell | pass | yes | 778 | 972 | 1.292 | 14.15× |
| cold | 1 | playwright-chrome | pass | yes | 1681 | 3622 | 0.505 | 30.56× |
| cold | 1 | playwright-shell | pass | yes | 643 | 1059 | 1.512 | 11.69× |
| cold | 1 | shotium | pass | yes | 55 | 76 | 18.325 | 1.00× |
| cold | 1 | puppeteer-chrome | pass | yes | 1724 | 4711 | 0.481 | 31.35× |
| cold-settled | 1 | puppeteer-shell | pass | yes | 296.699 | 370.163 | 3.441 | 35.59× |
| cold-settled | 1 | playwright-chrome | pass | yes | 179.325 | 217.551 | 5.401 | 21.51× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 355.996 | 402.285 | 3.042 | 42.71× |
| cold-settled | 1 | shotium | pass | yes | 8.336 | 10.485 | 116.626 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 151.028 | 171.35 | 6.676 | 18.12× |
| lifecycle | 1 | playwright-chrome | fail | no | 2425.543 | 3358.399 | N/A | N/A |
| lifecycle | 1 | shotium | pass | yes | 60.744 | 91.526 | 16.071 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 573.859 | 1047.931 | 1.546 | 9.45× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 1675.629 | 2368.094 | 0.575 | 27.59× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 736.074 | 1241.178 | 1.333 | 12.12× |
| warm | 1 | puppeteer-chrome | pass | yes | 358.724 | 624.736 | 2.474 | 44.79× |
| warm | 1 | playwright-chrome | pass | yes | 205.912 | 299.402 | 4.696 | 25.71× |
| warm | 1 | puppeteer-shell | pass | yes | 253.94 | 379.559 | 3.691 | 31.71× |
| warm | 1 | shotium | pass | yes | 8.009 | 13.708 | 110.534 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 160.621 | 221.919 | 6.072 | 20.06× |
| batch | 1 | playwright-shell | pass | yes | 146.215 | 421.392 | 5.700 | 16.78× |
| batch | 1 | playwright-chrome | pass | yes | 189.388 | 1306.059 | 3.962 | 21.74× |
| batch | 1 | puppeteer-shell | pass | yes | 295.498 | 658.515 | 3.175 | 33.92× |
| batch | 1 | puppeteer-chrome | pass | yes | 360.33 | 1435.257 | 2.474 | 41.36× |
| batch | 1 | shotium | pass | yes | 8.712 | 273.135 | 34.265 | 1.00× |
| parallel | 1 | shotium | pass | yes | 15.686 | 267.567 | 24.917 | 1.00× |
| parallel | 1 | puppeteer-shell | pass | yes | 336.073 | 649.533 | 2.946 | 21.43× |
| parallel | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 2 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 141.726 | 208.61 | 6.967 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 113.754 | 131.216 | 9.057 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 281.537 | 449.313 | 3.383 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 276.987 | 484.932 | 3.293 | N/A |
| resident | 1 | puppeteer-shell | pass | yes | 941 | 1494 | 0.938 | 2.75× |
| resident | 1 | playwright-chrome | pass | yes | 1233 | 1982 | 0.742 | 3.61× |
| resident | 1 | shotium | pass | yes | 342 | 392 | 2.854 | 1.00× |
| resident | 1 | puppeteer-chrome | pass | yes | 1211 | 1822 | 0.802 | 3.54× |
| resident | 1 | playwright-shell | pass | yes | 1312 | 1667 | 0.780 | 3.84× |
| faults | 1 | playwright-chrome | pass | no | 18836.305 | 18836.305 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 17134.473 | 17134.473 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13255.44 | 13255.44 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 8863.423 | 8863.423 | N/A | N/A |
| faults | 1 | shotium | pass | no | 4323.827 | 4323.827 | N/A | N/A |
| soak | 4 | puppeteer-chrome | pass | yes | 661.435 | 2654.468 | 5.813 | 13.82× |
| soak | 4 | shotium | pass | yes | 47.868 | 350.192 | 34.781 | 1.00× |
| soak | 4 | playwright-chrome | pass | yes | 670.124 | 4731.677 | 5.724 | 14.00× |
| soak | 4 | playwright-shell | pass | yes | 419.947 | 983.236 | 9.147 | 8.77× |
| soak | 4 | puppeteer-shell | pass | yes | 460.727 | 1234.514 | 8.360 | 9.62× |

