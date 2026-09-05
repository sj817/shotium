# Bilibili large-image regressions

These are the two reported article HTML files, with their original layout and
content. Only resource URLs and line endings were changed. All images, logos,
and fonts are local; nothing needs Bilibili, a localhost font proxy, or the
reporter's filesystem at test time. Embedded fonts/images were extracted and
deduplicated. Image bytes, dimensions, and formats were preserved, including GIFs.

`manifest.json` records original HTML hashes and each asset's source, size, and
SHA256. The localhost font URLs proxy Huawei's `config/commonResource/font`.
The logo comes from `karin-plugin-kkk/packages/core/resources/image/frame-logo.png`.
The two pages share 443 assets (about 88.5 MiB), including 403 WOFF2 files. These
are reproduction inputs, not assets shipped in the npm package.

Run `python tools/shot/bilibili_check.py --fixtures-only` for resource integrity
and complete offline dependency checks. After building the local Node package
and addon, run `python tools/shot/bilibili_check.py --package shotium` to exercise
`screenshot()` and `screenshotTiles()` at 1440px, compare all tile pixels with the
full capture with bounded antialiasing tolerance, and check every article photo
and both footer QR codes against their original source pixels. Both
pages must retain their footer beyond the 32767px paint boundary.
Rendered evidence is left in `shot/testdata/out/bilibili-*` on failure or success.
