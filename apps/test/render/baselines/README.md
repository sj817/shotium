# Render baselines

`pnpm render update-baselines --accept` writes Chromium reference PNGs and
`manifest.json` to this directory. Generate them from a pinned `headless_shell` build
(or explicitly select an external system Chrome) and review the image diffs before
committing them.

The regression runner (`pnpm render run`) only reads from this directory and never modifies it.
