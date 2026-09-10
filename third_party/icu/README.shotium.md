# ICU Source Slice

Based on Chromium ICU `6ebb40c594776cc2c21ea14df85a2a89a328b364`.
This directory is maintained directly in the Shotium repository.
It retains `common`, `i18n`, and `stubdata` sources, GN metadata, assembly generation
scripts, licenses, and the `cast`/`common` prebuilt data sets. The Shotium data set
is generated from `cast` by `scripts/build/icu-repack.ts` (`pnpm icu:repack`) and is not committed.
Chromium ICU build modifications are maintained directly in `BUILD.gn` and `config.gni`;
no external patch application or separate ICU checkout is required.
This baseline passed all six platform builds for Shotium 0.6.0 at commit `07ac95e4a85dad12bc7fd8029c5ca229e3e38d9f`.
See `apps/docs/upstream-sync-0.6.0-validation.md` for verification limits.
