# ICU source slice

Based on Chromium ICU 8cc91d9b6ab9991802fd208ee03a69714fd0251c.
This directory is maintained directly in the shotium repository.
It keeps common/i18n/stubdata sources, GN metadata, assembly generation
scripts, licenses and the cast/common prebuilt data sets. The Shot data set
is generated from cast by scripts/icu-repack.ts and is not checked in.
Chromium ICU build changes are already in BUILD.gn and config.gni; no patch
application or separate ICU checkout is required. Historical graph inputs
were used to check this initial selection; current cross-platform builds
still need verification after the complete screenshot cut batch.
