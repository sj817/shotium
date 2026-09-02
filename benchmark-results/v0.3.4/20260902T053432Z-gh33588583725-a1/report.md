# Shotium 0.3.4 benchmark

[Interactive benchmark explorer](https://sj817.github.io/shotium/)

Result: **incomplete**; quality: **fail**; evidence: **incomplete**. Profile **full**, seed `6bf4cb2fbc8e69e59371281916ce6bf49a73ef24`.

Conclusion: this run is incomplete. 0 platform(s) contain valid within-platform comparisons; missing outputs are never inferred.

Every ratio is computed only when Shotium and the compared engine both pass and are ranking-eligible on the same platform, scenario and concurrency. No cross-platform ranking is produced.

## Six-platform overview

| platform | quality status | formal winner | formally ranked engines | comparable cells |
|:--|:--|:--|--:|--:|
| linux-x64 | fail | no valid ranking | 0 | 9 |
| linux-arm64 | noisy | no valid ranking | 0 | 9 |
| win32-x64 | infra-error | no valid ranking | 0 | 0 |
| win32-arm64 | infra-error | no valid ranking | 0 | 0 |
| darwin-x64 | fail | no valid ranking | 0 | 3 |
| darwin-arm64 | fail | no valid ranking | 0 | 2 |

## linux-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: platform quality is fail; its measurements remain available for diagnosis, but no formal ranking or winner is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-x64/shotium.node |
| puppeteer-shell | pass | x64; /home/runner/.cache/puppeteer/chrome-headless-shell/linux-152.0.7977.42/chrome-headless-shell-linux64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /home/runner/.cache/puppeteer/chrome/linux-152.0.7977.42/chrome-linux64/chrome |
| playwright-shell | pass | x64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-linux64/chrome-headless-shell |
| playwright-chrome | infra-error | x64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux64/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | no | 536 | 600 | 1.810 | N/A |
| cold | 1 | playwright-shell | pass | no | 332 | 405 | 2.989 | N/A |
| cold | 1 | puppeteer-chrome | pass | no | 583 | 610 | 1.722 | N/A |
| cold | 1 | puppeteer-shell | pass | no | 343 | 355 | 2.950 | N/A |
| cold | 1 | shotium | pass | no | 65 | 72 | 15.054 | N/A |
| cold-settled | 1 | puppeteer-chrome | pass | no | 168.92 | 192.91 | 5.983 | N/A |
| cold-settled | 1 | playwright-chrome | noisy | no | 184.866 | 184.866 | 0.048 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | no | 130.708 | 141.063 | 7.737 | N/A |
| cold-settled | 1 | shotium | pass | no | 16.158 | 22.212 | 58.739 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 117.893 | 127.631 | 8.387 | N/A |
| lifecycle | 1 | playwright-shell | pass | no | 279.571 | 330.136 | 3.501 | N/A |
| lifecycle | 1 | playwright-chrome | pass | no | 607.464 | 652.171 | 1.696 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | no | 363.373 | 458.215 | 2.679 | N/A |
| lifecycle | 1 | shotium | pass | no | 64.013 | 129.632 | 13.455 | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | no | 757.717 | 799.481 | 1.352 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 156.293 | 188.066 | 2.731 | N/A |
| warm | 1 | shotium | pass | no | 12.889 | 19.721 | 69.256 | N/A |
| warm | 1 | playwright-shell | pass | no | 117.402 | 136.254 | 8.403 | N/A |
| warm | 1 | puppeteer-shell | pass | no | 130.303 | 180.449 | 7.717 | N/A |
| warm | 1 | puppeteer-chrome | pass | no | 161.309 | 204.583 | 6.147 | N/A |
| batch | 1 | shotium | pass | no | 28.024 | 259.312 | 20.359 | N/A |
| batch | 1 | puppeteer-shell | pass | no | 147.279 | 364.756 | 5.854 | N/A |
| batch | 1 | playwright-shell | pass | no | 134.622 | 355.192 | 6.324 | N/A |
| batch | 1 | puppeteer-chrome | pass | no | 183.902 | 397.02 | 4.865 | N/A |
| batch | 1 | playwright-chrome | noisy | no | 168.929 | 374.159 | 4.347 | N/A |
| parallel | 1 | playwright-chrome | pass | no | 173.234 | 377.585 | 5.140 | N/A |
| parallel | 1 | puppeteer-shell | pass | no | 147.893 | 479.555 | 5.761 | N/A |
| parallel | 1 | puppeteer-chrome | pass | no | 181.746 | 399.016 | 4.908 | N/A |
| parallel | 1 | shotium | pass | no | 27.599 | 261.072 | 20.651 | N/A |
| parallel | 1 | playwright-shell | pass | no | 137.752 | 352.973 | 6.216 | N/A |
| parallel | 2 | puppeteer-chrome | pass | no | 316.638 | 460.433 | 5.744 | N/A |
| parallel | 2 | shotium | pass | no | 62.573 | 276.373 | 20.539 | N/A |
| parallel | 2 | playwright-shell | pass | no | 247.68 | 457.936 | 7.171 | N/A |
| parallel | 2 | playwright-chrome | noisy | no | 319.515 | 483.075 | 3.799 | N/A |
| parallel | 2 | puppeteer-shell | pass | no | 255.298 | 393.974 | 6.752 | N/A |
| parallel | 4 | playwright-shell | pass | no | 426.332 | 786.175 | 7.814 | N/A |
| parallel | 4 | playwright-chrome | noisy | no | 558.014 | 724.559 | 2.978 | N/A |
| parallel | 4 | puppeteer-shell | pass | no | 502.642 | 699.073 | 7.197 | N/A |
| parallel | 4 | puppeteer-chrome | fail | no | 595.886 | 76540.607 | 1.067 | N/A |
| parallel | 4 | shotium | pass | no | 131.628 | 328.4 | 20.264 | N/A |
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
| soak | 4 | playwright-shell | pass | no | 434.917 | 878.386 | 8.707 | N/A |
| soak | 4 | puppeteer-chrome | fail | no | 614.702 | 180941.964 | 1.958 | N/A |
| soak | 4 | playwright-chrome | infra-error | no | 549.083 | 993.122 | 6.996 | N/A |
| soak | 4 | puppeteer-shell | pass | no | 483.301 | 905.498 | 8.080 | N/A |
| soak | 4 | shotium | pass | no | 141.308 | 392.196 | 21.703 | N/A |

## linux-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: platform quality is noisy; its measurements remain available for diagnosis, but no formal ranking or winner is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | arm64; /home/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-linux-arm64/shotium.node |
| puppeteer-shell | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| puppeteer-chrome | n/a | startup: the package has no native browser for this platform architecture; throughput: the package has no native browser for this platform architecture; parallel: the package has no native browser for this platform architecture; resident: the package has no native browser for this platform architecture; resilience: the package has no native browser for this platform architecture |
| playwright-shell | pass | arm64; /home/runner/.cache/ms-playwright/chromium_headless_shell-1234/chrome-linux/headless_shell |
| playwright-chrome | noisy | arm64; /home/runner/.cache/ms-playwright/chromium-1234/chrome-linux/chrome |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | no | 387 | 436 | 2.568 | N/A |
| cold | 1 | playwright-shell | pass | no | 234 | 437 | 3.806 | N/A |
| cold | 1 | shotium | pass | no | 61 | 65 | 16.204 | N/A |
| cold-settled | 1 | playwright-shell | pass | no | 100.514 | 117.535 | 9.825 | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 139.2 | 151.137 | 7.348 | N/A |
| cold-settled | 1 | shotium | pass | no | 34.135 | 37.535 | 28.873 | N/A |
| lifecycle | 1 | playwright-shell | pass | no | 223.327 | 285.695 | 4.295 | N/A |
| lifecycle | 1 | playwright-chrome | pass | no | 414.74 | 459.802 | 2.399 | N/A |
| lifecycle | 1 | shotium | pass | no | 69.436 | 97.919 | 13.550 | N/A |
| warm | 1 | playwright-chrome | pass | no | 132.457 | 170.762 | 7.421 | N/A |
| warm | 1 | playwright-shell | pass | no | 103.057 | 131.859 | 9.281 | N/A |
| warm | 1 | shotium | pass | no | 31.115 | 36.981 | 30.619 | N/A |
| batch | 1 | shotium | pass | no | 41.573 | 263.164 | 17.258 | N/A |
| batch | 1 | playwright-shell | pass | no | 127.508 | 324.973 | 7.033 | N/A |
| batch | 1 | playwright-chrome | pass | no | 137.543 | 352.136 | 6.225 | N/A |
| parallel | 1 | playwright-chrome | noisy | no | 167.68 | 371.411 | 4.467 | N/A |
| parallel | 1 | shotium | pass | no | 44.512 | 264.778 | 16.510 | N/A |
| parallel | 1 | playwright-shell | pass | no | 132.379 | 352.231 | 6.451 | N/A |
| parallel | 2 | shotium | pass | no | 86.974 | 301.296 | 16.826 | N/A |
| parallel | 2 | playwright-shell | pass | no | 204.366 | 362.629 | 8.550 | N/A |
| parallel | 2 | playwright-chrome | pass | no | 266.92 | 394.053 | 7.000 | N/A |
| parallel | 4 | playwright-shell | pass | no | 356.323 | 576.342 | 9.585 | N/A |
| parallel | 4 | playwright-chrome | pass | no | 457.045 | 607.196 | 7.786 | N/A |
| parallel | 4 | shotium | pass | no | 174.22 | 369.477 | 16.852 | N/A |
| reuse-page | 1 | playwright-shell | pass | no | 50.213 | 69.855 | 18.773 | N/A |
| reuse-page | 1 | playwright-chrome | pass | no | 64.971 | 67.096 | 15.817 | N/A |
| resident | 1 | playwright-shell | pass | no | 836 | 867 | 1.232 | N/A |
| resident | 1 | playwright-chrome | pass | no | 890 | 931 | 1.144 | N/A |
| resident | 1 | shotium | noisy | no | 338 | 415 | 0.333 | N/A |
| faults | 1 | shotium | pass | no | 9137.552 | 9137.552 | N/A | N/A |
| faults | 1 | playwright-chrome | pass | no | 17250.492 | 17250.492 | N/A | N/A |
| faults | 1 | playwright-shell | pass | no | 13145.578 | 13145.578 | N/A | N/A |
| soak | 4 | playwright-chrome | pass | no | 375.958 | 673.265 | 10.135 | N/A |
| soak | 4 | playwright-shell | pass | no | 285.434 | 543.489 | 12.844 | N/A |
| soak | 4 | shotium | pass | no | 179.177 | 424.282 | 17.776 | N/A |

## win32-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: platform quality is infra-error; its measurements remain available for diagnosis, but no formal ranking or winner is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|

## win32-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: platform quality is infra-error; its measurements remain available for diagnosis, but no formal ranking or winner is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|

## darwin-x64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: platform quality is fail; its measurements remain available for diagnosis, but no formal ranking or winner is produced.

| engine | availability | reason / binary architecture |
|:--|:--|:--|
| shotium | noisy | x64; /Users/runner/work/shotium/shotium/apps/benchmark/node_modules/@shotkit/shotium-darwin-x64/shotium.node |
| puppeteer-shell | noisy | x64; /Users/runner/.cache/puppeteer/chrome-headless-shell/mac-152.0.7977.42/chrome-headless-shell-mac-x64/chrome-headless-shell |
| puppeteer-chrome | fail | x64; /Users/runner/.cache/puppeteer/chrome/mac-152.0.7977.42/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |
| playwright-shell | noisy | x64; /Users/runner/Library/Caches/ms-playwright/chromium_headless_shell-1234/chrome-headless-shell-mac-x64/chrome-headless-shell |
| playwright-chrome | fail | x64; /Users/runner/Library/Caches/ms-playwright/chromium-1234/chrome-mac-x64/Google Chrome for Testing.app/Contents/MacOS/Google Chrome for Testing |

| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |
|:--|--:|:--|:--|:--|--:|--:|--:|--:|
| cold | 1 | playwright-chrome | pass | no | 4623 | 15502 | 0.167 | N/A |
| cold | 1 | playwright-shell | pass | no | 1118 | 2172 | 0.769 | N/A |
| cold | 1 | puppeteer-chrome | pass | no | 3273 | 7469 | 0.248 | N/A |
| cold | 1 | puppeteer-shell | pass | no | 1301 | 1805 | 0.764 | N/A |
| cold | 1 | shotium | pass | no | 172 | 529 | 4.375 | N/A |
| cold-settled | 1 | puppeteer-chrome | noisy | no | 11176.351 | 13033.562 | N/A | N/A |
| cold-settled | 1 | playwright-chrome | pass | no | 817.63 | 901.08 | 1.209 | N/A |
| cold-settled | 1 | puppeteer-shell | pass | no | 312.791 | 418.888 | 2.976 | N/A |
| cold-settled | 1 | shotium | noisy | no | 20.469 | 26.325 | 0.412 | N/A |
| cold-settled | 1 | playwright-shell | noisy | no | 319.144 | 339.822 | 0.092 | N/A |
| lifecycle | 1 | playwright-shell | pass | no | 1234.88 | 2082.948 | 0.749 | N/A |
| lifecycle | 1 | playwright-chrome | pass | no | 3379.47 | 4631.868 | 0.281 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | no | 1586.12 | 1942.095 | 0.660 | N/A |
| lifecycle | 1 | shotium | pass | no | 209.276 | 555.73 | 4.293 | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | no | 4188.048 | 5142.899 | 0.243 | N/A |
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
| soak | 4 | puppeteer-shell | pass | no | 808.473 | 1768.91 | 4.783 | N/A |
| soak | 4 | shotium | pass | no | 176.215 | 870.766 | 18.436 | N/A |

## darwin-arm64

Scenario groups ran on separate native runners; every engine comparison remains within one shard and one runner.

### Within-platform ranking

Conclusion: platform quality is fail; its measurements remain available for diagnosis, but no formal ranking or winner is produced.

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
| lifecycle | 1 | playwright-shell | pass | no | 493.453 | 906.426 | 1.885 | N/A |
| lifecycle | 1 | playwright-chrome | pass | no | 1977.412 | 3194.205 | 0.476 | N/A |
| lifecycle | 1 | puppeteer-shell | pass | no | 638.561 | 1166.601 | 1.419 | N/A |
| lifecycle | 1 | shotium | pass | no | 95.997 | 131.49 | 10.833 | N/A |
| lifecycle | 1 | puppeteer-chrome | pass | no | 2180.04 | 3540.398 | 0.431 | N/A |
| warm | 1 | playwright-chrome | noisy | no | 258.445 | 421.296 | 3.839 | N/A |
| warm | 1 | shotium | pass | no | 11 | 61.271 | 64.737 | N/A |
| warm | 1 | playwright-shell | pass | no | 135.149 | 232.58 | 7.155 | N/A |
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

