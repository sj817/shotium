# Bilibili large-image regressions

These fixtures represent two reported article HTML files with their original
layout and content preserved. Only resource URLs and line endings were adjusted.
All images, logos, and fonts are completely local and offline; no live network
requests, external CDN dependencies, or local proxies are needed during testing.
Embedded fonts and images were extracted and deduplicated while preserving exact
image dimensions, formats (including GIFs), and bytes.

`manifest.json` records original HTML hashes and each asset's origin, size, and
SHA-256 checksum. The localhost font URLs proxy Huawei's `config/commonResource/font`.
The logo originates from `karin-plugin-kkk/packages/core/resources/image/frame-logo.png`.
The two pages share 443 assets (approximately 88.5 MiB), including 403 WOFF2 font
files. These files serve solely as reproduction test inputs and are not packaged
into published npm artifacts.

Run `pnpm verify:bilibili --fixtures-only` to verify asset integrity and ensure
zero unmapped network references. After building the local Node SDK package and
native addon, run `pnpm verify:bilibili --package apps/typescript` to exercise
`screenshot()` and `screenshotTiles()` at 1440px width. This compares every rendered
tile against the full capture with bounded antialiasing tolerance, and verifies
every article image and footer QR code against its source pixels. Both pages
must render correctly beyond the 32,767px layout coordinate boundary.
Rendered test evidence is placed in `shot/testdata/out/bilibili-*`.
