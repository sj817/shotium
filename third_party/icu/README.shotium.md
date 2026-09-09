# ICU source slice

Based on Chromium ICU 6ebb40c594776cc2c21ea14df85a2a89a328b364.
This directory is maintained directly in the shotium repository.
It keeps common/i18n/stubdata sources, GN metadata, assembly generation
scripts, licenses and the cast/common prebuilt data sets. The Shot data set
is generated from cast by scripts/icu-repack.ts and is not checked in.
Chromium ICU build changes are already in BUILD.gn and config.gni; no patch
application or separate ICU checkout is required. Historical graph inputs
were used to check this initial selection. This baseline passed all six
platform builds for shotium 0.6.0 at 07ac95e4a85dad12bc7fd8029c5ca229e3e38d9f.
See docs/upstream-sync-0.6.0-validation.md for verification limits.
