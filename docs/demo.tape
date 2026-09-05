# shotium terminal demo.
#
# Regenerate with `pnpm run docs:demo`, which warms npm's download cache and
# then prepares docs/.demo-run as a fresh copy of the docs/demo sources with no
# node_modules. The install below is real without recording network latency.
# The generator then runs
# `vhs docs/demo.tape` from the repository root -- Output is resolved there.
#
# Needs vhs (https://github.com/charmbracelet/vhs), plus ttyd and ffmpeg on
# PATH. The shell is pinned to bash rather than left at the VHS default
# (powershell on Windows) so that the recording looks the same everywhere;
# on Windows, Git for Windows supplies one.

# The terminal recording is composited with the rendered card by
# tools/shot/docs_demo.mjs.
Output docs/.demo-terminal.gif

Require node
Require npm

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
Type "export npm_config_audit=false npm_config_fund=false npm_config_update_notifier=false" Enter
Type "clear" Enter
Sleep 1s
Show

# 1. Install. Two packages: the JS wrapper and one prebuilt platform binary.
Type "npm install @shotkit/shotium" Enter
Sleep 800ms
Hide
Sleep 15s
Show
Wait+Screen@60s /added/
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
