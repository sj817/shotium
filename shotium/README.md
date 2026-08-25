# shotium

Static screenshots from a stripped Chromium. DOM, CSS, layout, paint, fonts,
images, HTTP — everything a page needs to *look* right. No JavaScript engine.

```js
const shotium = require('shotium');

shotium.runtime.start({workers: 4});
const png = await shotium.screenshot({
  file: 'https://example.com',
  viewport: {width: 1280, height: 720},
  fullPage: true,
});
await shotium.runtime.stop();
```

## What it is

`shot` — `shot.exe` on Windows — is Chromium with V8 removed and the browser
layer removed with it: no `//content`, no render process, no GPU process, no
compositor, no sandbox.
What is left is Blink used directly — `Page` → `LocalFrame` → `Document` →
lifecycle → `cc::PaintRecord` → CPU raster — which is the same shape Blink
already uses internally for SVG images.

This package is the supervisor for those processes. It is plain JavaScript: no
native addon, no node-gyp, no ABI matrix. The worker talks a length-prefixed
protocol over stdio.

## What it is not

**There is no JavaScript engine.** A page that builds itself with script
photographs as an empty page. That is the product boundary, not a missing
feature: removing V8 is what makes the binary small enough to ship and fast
enough to start.

Consequences worth knowing:

- No `<script>`, no framework hydration, no `document.fonts.ready`.
- `selector` is resolved with `Document::querySelector` inside the renderer.
  Nothing is injected into the page.
- Which fonts exist is the host's business; how they are rasterised is not.
  Grayscale antialiasing at a fixed gamma, no subpixel geometry, no read of the
  host's ClearType settings — so one platform renders a page the same way every
  time. Two different platforms still differ by a hair at glyph edges.
- A page that declares a legacy encoding renders as mojibake. The document is
  installed with the encoding hardcoded to UTF-8, and that outranks
  `<meta charset>`, so `shift_jis` and `gbk` declarations are never consulted.

## The engine binary

The engine is a separate download, one archive per platform and architecture —
`shot-win-x64`, `shot-mac-arm64`, `shot-linux-x64` and the other three — on the
[releases page]. Unpack one and point the package at it:

```bash
export SHOTIUM_BINARY=/path/to/shot-mac-arm64/shot
```

Or pass `binary` to `runtime.start()`. With neither set, the package looks for
`bin/shot` (`bin/shot.exe` on Windows) beside `index.js`.

## Processes

Blink is a process-wide singleton — `Platform::InitializeBlink()`,
`cppgc::InitializeProcess()` and the main-thread scheduler are all
once-per-process. So one worker renders one document at a time, and concurrency
means more processes. That is also what makes a crash survivable: a worker takes
its own request down and nothing else, and the pool refills the slot.

`retry` re-sends a request after a crash or a timeout. A worker that dies
mid-request looks exactly like one that never answered, which is deliberate —
the retry path does not have to tell them apart.

## Network

The worker links `//net` directly: HTTPS, HTTP/2, redirects, an in-memory cookie
jar, and an optional disk cache. No HTTP/3, no proxy support, system DNS.

`cacheDir` is a *root*; each worker slot gets its own subdirectory, because the
cache backend takes an exclusive lock on its directory. A restarted worker
inherits the warm cache of the slot it replaced.

There is no SSRF protection and no resource ceiling beyond a body-size cap.
shotium is the bottom half of something; deciding which URLs may be fetched is
the caller's job.

## Events

```js
shotium.runtime.on('crash', ({worker}) => …);          // died owing an answer
shotium.runtime.on('timeout', ({worker, timeout}) => …);
shotium.runtime.on('worker-restart', ({worker}) => …);
shotium.runtime.on('stderr', ({worker, line}) => …);   // console + load log
```

`stderr` carries the worker's own diagnostics, including the page's console
messages and, with `args: ['--verbose']`, every subresource request and its
outcome. It is the first place to look when a screenshot comes out wrong.

[releases page]: https://github.com/sj817/shotium/releases
