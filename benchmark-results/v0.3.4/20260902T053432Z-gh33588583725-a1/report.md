# Shotium 0.3.4 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **incomplete**; quality: **fail**; evidence: **incomplete**. Profile **full**, seed `6bf4cb2fbc8e69e59371281916ce6bf49a73ef24`.

Conclusion: this run is incomplete. 4 platform(s) contain valid within-platform comparisons; missing outputs are never inferred.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | fail | shotium | 3 | 9 |
| linux-arm64 | noisy | shotium | 2 | 9 |
| win32-x64 | infra-error | no valid ranking | 0 | 0 |
| win32-arm64 | infra-error | no valid ranking | 0 | 0 |
| darwin-x64 | fail | shotium | 2 | 3 |
| darwin-arm64 | fail | shotium | 2 | 2 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 4.819× | 7 | 9 / 9 | 0 |
| 3 | puppeteer-shell | 5.364× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 8.435× | 6 | 7 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 7.890× | 3 | 3 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- puppeteer-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- puppeteer-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `warm/c1`
- playwright-chrome: `cold/c1`, `lifecycle/c1`, `parallel/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | infra-error | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 536 | 600 | 1.810 | 8.25× |
| cold | 1 | playwright-shell | pass | yes | 332 | 405 | 2.989 | 5.11× |
| cold | 1 | puppeteer-chrome | pass | yes | 583 | 610 | 1.722 | 8.97× |
| cold | 1 | puppeteer-shell | pass | yes | 343 | 355 | 2.950 | 5.28× |
| cold | 1 | shotium | pass | yes | 65 | 72 | 15.054 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | pass | yes | 168.92 | 192.91 | 5.983 | 10.45× |
| cold-settled | 1 | playwright-chrome | noisy | no | 184.866 | 184.866 | 0.048 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | yes | 130.708 | 141.063 | 7.737 | 8.09× |
| cold-settled | 1 | shotium | pass | yes | 16.158 | 22.212 | 58.739 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 117.893 | 127.631 | 8.387 | 7.30× |
| lifecycle | 1 | playwright-shell | pass | yes | 279.571 | 330.136 | 3.501 | 4.37× |
| lifecycle | 1 | playwright-chrome | pass | yes | 607.464 | 652.171 | 1.696 | 9.49× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 363.373 | 458.215 | 2.679 | 5.68× |
| lifecycle | 1 | shotium | pass | yes | 64.013 | 129.632 | 13.455 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 757.717 | 799.481 | 1.352 | 11.84× |
| warm | 1 | playwright-chrome | noisy | no | 156.293 | 188.066 | 2.731 | N/A |
| warm | 1 | shotium | pass | yes | 12.889 | 19.721 | 69.256 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 117.402 | 136.254 | 8.403 | 9.11× |
| warm | 1 | puppeteer-shell | pass | yes | 130.303 | 180.449 | 7.717 | 10.11× |
| warm | 1 | puppeteer-chrome | pass | yes | 161.309 | 204.583 | 6.147 | 12.52× |
| batch | 1 | shotium | pass | yes | 28.024 | 259.312 | 20.359 | 1.00× |
| batch | 1 | puppeteer-shell | pass | yes | 147.279 | 364.756 | 5.854 | 5.26× |
| batch | 1 | playwright-shell | pass | yes | 134.622 | 355.192 | 6.324 | 4.80× |
| batch | 1 | puppeteer-chrome | pass | yes | 183.902 | 397.02 | 4.865 | 6.56× |
| batch | 1 | playwright-chrome | noisy | no | 168.929 | 374.159 | 4.347 | N/A |
| parallel | 1 | playwright-chrome | pass | yes | 173.234 | 377.585 | 5.140 | 6.28× |
| parallel | 1 | puppeteer-shell | pass | yes | 147.893 | 479.555 | 5.761 | 5.36× |
| parallel | 1 | puppeteer-chrome | pass | yes | 181.746 | 399.016 | 4.908 | 6.59× |
| parallel | 1 | shotium | pass | yes | 27.599 | 261.072 | 20.651 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 137.752 | 352.973 | 6.216 | 4.99× |
| parallel | 2 | puppeteer-chrome | pass | yes | 316.638 | 460.433 | 5.744 | 5.06× |
| parallel | 2 | shotium | pass | yes | 62.573 | 276.373 | 20.539 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 247.68 | 457.936 | 7.171 | 3.96× |
| parallel | 2 | playwright-chrome | noisy | no | 319.515 | 483.075 | 3.799 | N/A |
| parallel | 2 | puppeteer-shell | pass | yes | 255.298 | 393.974 | 6.752 | 4.08× |
| parallel | 4 | playwright-shell | pass | yes | 426.332 | 786.175 | 7.814 | 3.24× |
| parallel | 4 | playwright-chrome | noisy | no | 558.014 | 724.559 | 2.978 | N/A |
| parallel | 4 | puppeteer-shell | pass | yes | 502.642 | 699.073 | 7.197 | 3.82× |
| parallel | 4 | puppeteer-chrome | fail | no | 595.886 | 76540.607 | 1.067 | N/A |
| parallel | 4 | shotium | pass | yes | 131.628 | 328.4 | 20.264 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 67.145 | 83.32 | 14.437 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 67.933 | 94.144 | 14.176 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 82.118 | 85.25 | 12.702 | N/A |
| reuse-page | 1 | puppeteer-chrome | pass | no | 85.815 | 115.92 | 10.923 | N/A |
| resident | 1 | puppeteer-shell | pass | no | 858 | 949 | 1.169 | N/A |
| resident | 1 | shotium | noisy | no | 190 | 466 | 0.290 | N/A |
| resident | 1 | playwright-shell | pass | no | 886 | 1065 | 1.090 | N/A |
| resident | 1 | playwright-chrome | pass | no | 1011 | 1133 | 0.962 | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 16399.376 | 16504.764 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 9660.592 | 9660.592 | N/A | N/A |
| faults | 1 | puppeteer-chrome | pass | no | 11308.981 | 11308.981 | N/A | N/A |
| faults | 1 | shotium | pass | no | 9781.608 | 9781.608 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 14911.922 | 14911.922 | N/A | N/A |
| faults | 1 | playwright-chrome | noisy | no | 3463.032 | 3463.032 | N/A | N/A |
| soak | 4 | playwright-shell | pass | yes | 434.917 | 878.386 | 8.707 | 3.08× |
| soak | 4 | puppeteer-chrome | fail | no | 614.702 | 180941.964 | 1.958 | N/A |
| soak | 4 | playwright-chrome | infra-error | no | 549.083 | 993.122 | 6.996 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 483.301 | 905.498 | 8.080 | 3.42× |
| soak | 4 | shotium | pass | yes | 141.308 | 392.196 | 21.703 | 1.00× |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 9 eligible cell(s), with 9 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 7 | 9 / 9 | 9 |
| 2 | playwright-shell | 2.729× | 7 | 9 / 9 | 0 |
| not ranked (partial coverage) | playwright-chrome | 3.721× | 7 | 8 / 9 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-shell: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`
- playwright-chrome: `batch/c1`, `cold/c1`, `cold-settled/c1`, `lifecycle/c1`, `parallel/c2`, `parallel/c4`, `soak/c4`, `warm/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | noisy | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 387 | 436 | 2.568 | 6.34× |
| cold | 1 | playwright-shell | pass | yes | 234 | 437 | 3.806 | 3.84× |
| cold | 1 | shotium | pass | yes | 61 | 65 | 16.204 | 1.00× |
| cold-settled | 1 | playwright-shell | pass | yes | 100.514 | 117.535 | 9.825 | 2.94× |
| cold-settled | 1 | playwright-chrome | pass | yes | 139.2 | 151.137 | 7.348 | 4.08× |
| cold-settled | 1 | shotium | pass | yes | 34.135 | 37.535 | 28.873 | 1.00× |
| lifecycle | 1 | playwright-shell | pass | yes | 223.327 | 285.695 | 4.295 | 3.22× |
| lifecycle | 1 | playwright-chrome | pass | yes | 414.74 | 459.802 | 2.399 | 5.97× |
| lifecycle | 1 | shotium | pass | yes | 69.436 | 97.919 | 13.550 | 1.00× |
| warm | 1 | playwright-chrome | pass | yes | 132.457 | 170.762 | 7.421 | 4.26× |
| warm | 1 | playwright-shell | pass | yes | 103.057 | 131.859 | 9.281 | 3.31× |
| warm | 1 | shotium | pass | yes | 31.115 | 36.981 | 30.619 | 1.00× |
| batch | 1 | shotium | pass | yes | 41.573 | 263.164 | 17.258 | 1.00× |
| batch | 1 | playwright-shell | pass | yes | 127.508 | 324.973 | 7.033 | 3.07× |
| batch | 1 | playwright-chrome | pass | yes | 137.543 | 352.136 | 6.225 | 3.31× |
| parallel | 1 | playwright-chrome | noisy | no | 167.68 | 371.411 | 4.467 | N/A |
| parallel | 1 | shotium | pass | yes | 44.512 | 264.778 | 16.510 | 1.00× |
| parallel | 1 | playwright-shell | pass | yes | 132.379 | 352.231 | 6.451 | 2.97× |
| parallel | 2 | shotium | pass | yes | 86.974 | 301.296 | 16.826 | 1.00× |
| parallel | 2 | playwright-shell | pass | yes | 204.366 | 362.629 | 8.550 | 2.35× |
| parallel | 2 | playwright-chrome | pass | yes | 266.92 | 394.053 | 7.000 | 3.07× |
| parallel | 4 | playwright-shell | pass | yes | 356.323 | 576.342 | 9.585 | 2.05× |
| parallel | 4 | playwright-chrome | pass | yes | 457.045 | 607.196 | 7.786 | 2.62× |
| parallel | 4 | shotium | pass | yes | 174.22 | 369.477 | 16.852 | 1.00× |
| reuse-page | 1 | playwright-shell | pass | no | 50.213 | 69.855 | 18.773 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 64.971 | 67.096 | 15.817 | N/A |
| resident | 1 | playwright-shell | pass | no | 836 | 867 | 1.232 | N/A |
| resident | 1 | playwright-chrome | pass | no | 890 | 931 | 1.144 | N/A |
| resident | 1 | shotium | noisy | no | 338 | 415 | 0.333 | N/A |
| faults | 1 | shotium | pass | no | 9137.552 | 9137.552 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 17250.492 | 17250.492 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13145.578 | 13145.578 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | yes | 375.958 | 673.265 | 10.135 | 2.10× |
| soak | 4 | playwright-shell | pass | yes | 285.434 | 543.489 | 12.844 | 1.59× |
| soak | 4 | shotium | pass | yes | 179.177 | 424.282 | 17.776 | 1.00× |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: no scenario/concurrency pair has both an eligible Shotium result and an eligible competitor result, so no ranking is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 3 eligible cell(s), with 3 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 3 | 3 / 3 | 3 |
| 2 | puppeteer-shell | 6.407× | 3 | 3 / 3 | 0 |
| not ranked (partial coverage) | playwright-shell | 6.193× | 2 | 2 / 3 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 19.514× | 2 | 2 / 3 | 0 |
| not ranked (partial coverage) | playwright-chrome | 20.834× | 2 | 2 / 3 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `cold/c1`, `lifecycle/c1`, `soak/c4`
- puppeteer-shell: `cold/c1`, `lifecycle/c1`, `soak/c4`
- playwright-shell: `cold/c1`, `lifecycle/c1`
- puppeteer-chrome: `cold/c1`, `lifecycle/c1`
- playwright-chrome: `cold/c1`, `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | yes | 4623 | 15502 | 0.167 | 26.88× |
| cold | 1 | playwright-shell | pass | yes | 1118 | 2172 | 0.769 | 6.50× |
| cold | 1 | puppeteer-chrome | pass | yes | 3273 | 7469 | 0.248 | 19.03× |
| cold | 1 | puppeteer-shell | pass | yes | 1301 | 1805 | 0.764 | 7.56× |
| cold | 1 | shotium | pass | yes | 172 | 529 | 4.375 | 1.00× |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 11176.351 | 13033.562 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 817.63 | 901.08 | 1.209 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | no | 312.791 | 418.888 | 2.976 | N/A |
| cold-settled | 1 | shotium | noisy | no | 20.469 | 26.325 | 0.412 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 319.144 | 339.822 | 0.092 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 1234.88 | 2082.948 | 0.749 | 5.90× |
| lifecycle | 1 | playwright-chrome | pass | yes | 3379.47 | 4631.868 | 0.281 | 16.15× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 1586.12 | 1942.095 | 0.660 | 7.58× |
| lifecycle | 1 | shotium | pass | yes | 209.276 | 555.73 | 4.293 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 4188.048 | 5142.899 | 0.243 | 20.01× |
| warm | 1 | playwright-chrome | noisy | no | 882.866 | 1220.821 | 1.098 | N/A |
| warm | 1 | shotium | noisy | no | 24.167 | 25.145 | 0.368 | N/A |
| warm | 1 | playwright-shell | noisy | no | 8723.494 | 9590.523 | N/A | N/A |
| warm | 1 | puppeteer-shell | pass | no | 350.143 | 446.201 | 2.934 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 11966.652 | 12461.073 | N/A | N/A |
| batch | 1 | shotium | noisy | no | 59.423 | 385.714 | 6.034 | N/A |
| batch | 1 | puppeteer-shell | pass | no | 369.255 | 662.248 | 2.544 | N/A |
| batch | 1 | playwright-shell | noisy | no | 324.869 | 532.382 | 0.586 | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 11228.043 | 11675.331 | N/A | N/A |
| batch | 1 | playwright-chrome | pass | no | 903.096 | 1296.935 | 1.082 | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 906.564 | 1410.417 | 0.900 | N/A |
| parallel | 1 | puppeteer-shell | noisy | no | 341.134 | 660.43 | 1.853 | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | 9977.01 | 11744.662 | N/A | N/A |
| parallel | 1 | shotium | noisy | no | 63.165 | 329.298 | 8.532 | N/A |
| parallel | 1 | playwright-shell | noisy | no | 341.323 | 590.311 | 0.592 | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | 8579.806 | 11607.587 | N/A | N/A |
| parallel | 2 | shotium | noisy | no | 85.199 | 288.071 | 6.311 | N/A |
| parallel | 2 | playwright-shell | noisy | no | 449.869 | 680.605 | 0.740 | N/A |
| parallel | 2 | playwright-chrome | fail | no | 1415.253 | 3636.407 | 1.031 | N/A |
| parallel | 2 | puppeteer-shell | pass | no | 464.775 | 809.432 | 4.052 | N/A |
| parallel | 4 | playwright-shell | noisy | no | 741.027 | 1451.816 | 1.207 | N/A |
| parallel | 4 | playwright-chrome | fail | no | 2682.982 | 6731.402 | 1.347 | N/A |
| parallel | 4 | puppeteer-shell | pass | no | 769.684 | 1959.266 | 4.472 | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | 9199.031 | 10794.529 | N/A | N/A |
| parallel | 4 | shotium | noisy | no | 186.087 | 526.292 | 4.476 | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | 5259.146 | 5259.146 | N/A | N/A |
| reuse-page | 1 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 7021.205 | 7021.205 | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 3363 | 4197 | 0.101 | N/A |
| resident | 1 | shotium | noisy | no | 21916.349 | 27706.591 | N/A | N/A |
| resident | 1 | playwright-shell | noisy | no | 2592 | 4148 | 0.055 | N/A |
| resident | 1 | playwright-chrome | fail | no | 49293.081 | 61064.171 | N/A | N/A |
| resident | 1 | puppeteer-chrome | fail | no | 2157 | 2281 | 0.012 | N/A |
| faults | 1 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | 11490.006 | 11490.006 | N/A | N/A |
| faults | 1 | shotium | noisy | no | 6030.664 | 6030.664 | N/A | N/A |
| faults | 1 | playwright-shell | noisy | no | 9267.233 | 9267.233 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 42488.346 | 42488.346 | N/A | N/A |
| soak | 4 | playwright-shell | noisy | no | 6376.069 | 6376.069 | N/A | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | 9222.694 | 9222.694 | N/A | N/A |
| soak | 4 | playwright-chrome | fail | no | 2954.813 | 7149.026 | 1.093 | N/A |
| soak | 4 | puppeteer-shell | pass | yes | 808.473 | 1768.91 | 4.783 | 4.59× |
| soak | 4 | shotium | pass | yes | 176.215 | 870.766 | 18.436 | 1.00× |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Relative elapsed time is normalized to Shotium = 1.000 in each eligible cell; **lower is better**. The score is the geometric mean within this platform only. A formal rank is awarded only to engines covering every comparable platform cell; partial coverage remains visible but is not ranked. Tied cell winners each receive one win.

Conclusion: shotium ranks first on this platform at 1.000× normalized elapsed time across 2 eligible cell(s), with 2 win(s).

| rank | engine | geometric mean relative time | eligible scenarios | eligible cells / platform cells | wins |
|--:|:--|--:|--:|--:|--:|
| 1 | shotium | 1.000× | 2 | 2 / 2 | 2 |
| 2 | playwright-shell | 7.947× | 2 | 2 / 2 | 0 |
| not ranked (partial coverage) | puppeteer-shell | 6.652× | 1 | 1 / 2 | 0 |
| not ranked (partial coverage) | playwright-chrome | 20.599× | 1 | 1 / 2 | 0 |
| not ranked (partial coverage) | puppeteer-chrome | 22.709× | 1 | 1 / 2 | 0 |

<details><summary>Coverage audit</summary>

- shotium: `lifecycle/c1`, `warm/c1`
- playwright-shell: `lifecycle/c1`, `warm/c1`
- puppeteer-shell: `lifecycle/c1`
- playwright-chrome: `lifecycle/c1`
- puppeteer-chrome: `lifecycle/c1`

</details>

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-arm64/shotium.node |
| puppeteer-shell | noisy | arm64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac_arm-152.0.7977.42/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| puppeteer-chrome | noisy | arm64; /Users/runner/.cache/puppeteer/chrome/mac_arm-152.0.7977.42/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | arm64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-arm64/chrome-headless-shell |
| playwright-chrome | fail | arm64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-arm64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | noisy | no | 5536.5 | 7438 | 0.181 | N/A |
| cold | 1 | playwright-shell | noisy | no | 1363 | 1956 | 0.733 | N/A |
| cold | 1 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| cold | 1 | puppeteer-shell | noisy | no | 1341 | 1583 | 0.746 | N/A |
| cold | 1 | shotium | noisy | no | 107 | 174 | 8.065 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 6834.059 | 8000.398 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 216.014 | 271.225 | 4.328 | N/A |
| cold-settled | 1 | puppeteer-shell | noisy | no | 297.03 | 338.777 | 0.151 | N/A |
| cold-settled | 1 | shotium | noisy | no | 10.31 | 14.387 | 0.417 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 188.779 | 413.709 | 0.527 | N/A |
| lifecycle | 1 | playwright-shell | pass | yes | 493.453 | 906.426 | 1.885 | 5.14× |
| lifecycle | 1 | playwright-chrome | pass | yes | 1977.412 | 3194.205 | 0.476 | 20.60× |
| lifecycle | 1 | puppeteer-shell | pass | yes | 638.561 | 1166.601 | 1.419 | 6.65× |
| lifecycle | 1 | shotium | pass | yes | 95.997 | 131.49 | 10.833 | 1.00× |
| lifecycle | 1 | puppeteer-chrome | pass | yes | 2180.04 | 3540.398 | 0.431 | 22.71× |
| warm | 1 | playwright-chrome | noisy | no | 258.445 | 421.296 | 3.839 | N/A |
| warm | 1 | shotium | pass | yes | 11 | 61.271 | 64.737 | 1.00× |
| warm | 1 | playwright-shell | pass | yes | 135.149 | 232.58 | 7.155 | 12.29× |
| warm | 1 | puppeteer-shell | noisy | no | 185.069 | 264.455 | 2.134 | N/A |
| warm | 1 | puppeteer-chrome | noisy | no | 6288.231 | 7460.062 | N/A | N/A |
| batch | 1 | shotium | noisy | no | 24.017 | 268.205 | 21.985 | N/A |
| batch | 1 | puppeteer-shell | noisy | no | 205.189 | 531.302 | 3.831 | N/A |
| batch | 1 | playwright-shell | noisy | no | 146.666 | 369.336 | 5.702 | N/A |
| batch | 1 | puppeteer-chrome | noisy | no | 5940.092 | 6809.501 | N/A | N/A |
| batch | 1 | playwright-chrome | noisy | no | 321.694 | 1433.614 | 2.514 | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 366.408 | 1266.863 | 1.399 | N/A |
| parallel | 1 | puppeteer-shell | noisy | no | 225.159 | 603.233 | 4.082 | N/A |
| parallel | 1 | puppeteer-chrome | noisy | no | 6247.266 | 10181.269 | N/A | N/A |
| parallel | 1 | shotium | noisy | no | 37.506 | 264.318 | 11.532 | N/A |
| parallel | 1 | playwright-shell | noisy | no | 242.984 | 568.835 | 2.186 | N/A |
| parallel | 2 | puppeteer-chrome | noisy | no | 7287.501 | 9168.148 | N/A | N/A |
| parallel | 2 | shotium | noisy | no | 56.422 | 287.083 | 15.532 | N/A |
| parallel | 2 | playwright-shell | noisy | no | 304.852 | 695.425 | 4.097 | N/A |
| parallel | 2 | playwright-chrome | fail | no | 669.503 | 1868.575 | 2.596 | N/A |
| parallel | 2 | puppeteer-shell | noisy | no | 385.679 | 1121.22 | 3.956 | N/A |
| parallel | 4 | playwright-shell | noisy | no | 539.9 | 899.575 | 6.833 | N/A |
| parallel | 4 | playwright-chrome | fail | no | 915.103 | 3267.395 | 2.065 | N/A |
| parallel | 4 | puppeteer-shell | noisy | no | 536.122 | 1028.033 | 6.329 | N/A |
| parallel | 4 | puppeteer-chrome | noisy | no | 6908.631 | 7642.027 | N/A | N/A |
| parallel | 4 | shotium | noisy | no | 126.6 | 387.593 | 20.618 | N/A |
| reuse-page | 1 | playwright-shell | noisy | no | N/A | N/A | N/A | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 140.034 | 295.315 | 6.563 | N/A |
| reuse-page | 1 | puppeteer-shell | pass | no | 155.391 | 243.211 | 5.903 | N/A |
| reuse-page | 1 | puppeteer-chrome | noisy | no | 5501.616 | 5501.616 | N/A | N/A |
| resident | 1 | puppeteer-shell | noisy | no | 1899 | 2737 | 0.056 | N/A |
| resident | 1 | shotium | noisy | no | 12358.764 | 18277.514 | N/A | N/A |
| resident | 1 | playwright-shell | noisy | no | 1543 | 1888 | 0.079 | N/A |
| resident | 1 | playwright-chrome | noisy | no | 1948 | 2768 | 0.498 | N/A |
| resident | 1 | puppeteer-chrome | noisy | no | 20668.658 | 23370.146 | N/A | N/A |
| faults | 1 | puppeteer-shell | pass | no | 12629.116 | 12629.116 | N/A | N/A |
| faults | 1 | puppeteer-chrome | noisy | no | 4884.457 | 4884.457 | N/A | N/A |
| faults | 1 | shotium | pass | no | 12131.197 | 12131.197 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 15198.843 | 15198.843 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 30451.051 | 30451.051 | N/A | N/A |
| soak | 4 | playwright-shell | pass | no | 484.79 | 1439.729 | 7.995 | N/A |
| soak | 4 | puppeteer-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | playwright-chrome | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | puppeteer-shell | noisy | no | N/A | N/A | N/A | N/A |
| soak | 4 | shotium | noisy | no | N/A | N/A | N/A | N/A |

