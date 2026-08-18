# Shot baseline

`//shot:shot` is the first extraction baseline for a static HTML/CSS screenshot
engine. It uses Headless only to bootstrap Content. The screenshot itself is a
direct path:

```text
content::WebContents
  -> RenderFrameHost visual-state barrier
  -> RenderWidgetHostView::CopyFromSurface
  -> SkBitmap
  -> gfx::PNGCodec
  -> base::WriteFile
```

It does not invoke CDP or `Page.captureScreenshot`.

## Usage

```text
shot URL_OR_PATH --width 1280 --height 720 --output page.png
shot --file page.html -o page.png
shot --stdin -o page.png
```

`--timeout-ms` (or `--timeout`) is measured in milliseconds and defaults to
30000. The first version fixes device scale to 1 and captures only the viewport.
HTML supplied on stdin is stored in a temporary file; relative resource URLs in
that mode are not yet a stable interface.

## Build

For the smallest available baseline configuration, generate an output directory
with Chromium's existing Headless args and disable printing:

```gn
import("//build/args/headless.gn")
is_debug = false
enable_printing = false
```

Then build `//shot:shot`.

This is still scaffolding, not the final dependency closure. In particular,
`//headless:headless_shell_lib` currently retains Content's multiprocess model,
V8, Mojo, Viz, sandbox/crash infrastructure, and Headless DevTools code even
though Shot's capture path does not call DevTools.
