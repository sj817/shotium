#!/bin/bash
# Peak memory of one shotium.exe run per scenario. SHOT_PROFILE=1 + --verbose
# make the engine log "shot: mem <stage> ..." lines, printed with PEAK_VERBOSE=1.
# Output directory for the images and the fixture; the two bilibili pages are
# the user's test cases (see docs/handoff-memory-2026-09-04.md).
W="${MEASURE_DIR:-/tmp/shot-measure}"; mkdir -p "$W"
PEAK="$(dirname "$0")/peak_memory.ps1"
[ -f "$W/tiny.html" ] || printf '<!doctype html><body style="margin:0;background:#fff"><p style="font:16px sans-serif">hello</p></body>' > "$W/tiny.html"
EXE="${EXE:-D:/Github/chromium/out/Shot/shotium.exe}"
B1="D:/Downloads/bilibili_dynamic_DYNAMIC_TYPE_ARTICLE_1788403871415.html"
B2="D:/Downloads/bilibili_dynamic_DYNAMIC_TYPE_ARTICLE_1788434008828.html"
export SHOT_PROFILE=1
run() { pwsh -NoProfile -File "$PEAK" "$@"; }
only="${1:-all}"
case "$only" in
  all|tiny)  run "tiny viewport"     "$EXE" --file "$W/tiny.html" --width 1440 --verbose --output "$W/m_tiny.png" ;;&
  all|b1v)   run "bili1 viewport"    "$EXE" --file "$B1" --width 1440 --verbose --output "$W/m_b1v.png" ;;&
  all|b1)    run "bili1 fullPage png" "$EXE" --file "$B1" --width 1440 --full-page --verbose --output "$W/m_b1.png" ;;&
  all|b1j)   run "bili1 fullPage jpeg" "$EXE" --file "$B1" --width 1440 --full-page --type jpeg --verbose --output "$W/m_b1.jpg" ;;&
  all|b2)    run "bili2 fullPage png" "$EXE" --file "$B2" --width 1440 --full-page --verbose --output "$W/m_b2.png" ;;&
  all|b2t)   run "bili2 tiles 8000"  "$EXE" --file "$B2" --width 1440 --full-page --tile-height 8000 --verbose --output "$W/m_b2t.png" ;;&
  all|b2j)   run "bili2 fullPage jpeg" "$EXE" --file "$B2" --width 1440 --full-page --type jpeg --verbose --output "$W/m_b2.jpg" ;;&
  all|b2w)   run "bili2 webp"        "$EXE" --file "$B2" --width 1440 --full-page --type webp --scale 0.35 --verbose --output "$W/m_b2.webp" ;;&
esac
