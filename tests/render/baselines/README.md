# Render baselines

`update-baselines.ps1 -Accept` writes Chromium reference PNGs and
`manifest.json` here. Generate them from one pinned `headless_shell` build (or
explicitly select an external system Chrome) and review the image changes before
committing them.

The regression runner never updates this directory.
