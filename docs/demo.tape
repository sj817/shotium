# shotium terminal demo.
#
# Regenerate with `pnpm run docs:demo`, which prepares the sandbox in
# docs/.demo-run (a fresh copy of the docs/demo sources, with no node_modules,
# so that the install below is a real one) and then runs
# `vhs docs/demo.tape` from the repository root -- Output is resolved there.
#
# Needs vhs (https://github.com/charmbracelet/vhs), plus ttyd and ffmpeg on
# PATH. The shell is pinned to bash rather than left at the VHS default
# (powershell on Windows) so that the recording looks the same everywhere;
# on Windows, Git for Windows supplies one.

Output docs/assets/demo.gif

Require node
Require pnpm

Set Shell "bash"
Set FontSize 20
Set Width 1080
Set Height 830
Set Padding 22
Set Margin 18
Set MarginFill "#11131c"
Set BorderRadius 10
Set WindowBar Colorful
Set Theme "Catppuccin Mocha"
Set TypingSpeed 40ms
Set Framerate 20

# Enter the sandbox off camera; the demo starts on a clean screen.
Hide
Type "cd docs/.demo-run" Enter
Type "export PS1='$ '" Enter
Type "clear" Enter
Sleep 1s
Show

# 1. Install. Two packages: the JS wrapper and one prebuilt platform binary.
Type "pnpm add @shotkit/shotium" Enter
Wait+Screen@120s /Done in/
Sleep 2s

Hide
Type "clear" Enter
Sleep 500ms
Show

# 2. The whole program: start the engine, capture a file, stop.
Type "cat card.mjs" Enter
Sleep 5s

Hide
Type "clear" Enter
Sleep 500ms
Show

# 3. Run it. No browser is launched -- Blink renders inside this process.
Type "node card.mjs" Enter
Wait+Screen@120s /KB/
Sleep 4s
