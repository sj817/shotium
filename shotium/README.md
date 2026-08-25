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

## The resident daemon

The pool above lives and dies with the process holding it. A command line, a CI
step, a queue worker and a `node -e` all pay to start workers and then throw
them away, which for a one-shot process is most of what the screenshot costs.

`daemon` is the same pool behind a socket — a named pipe on Windows, a unix
socket elsewhere — so the next process finds it already warm:

```js
const client = await shotium.daemon.connect({workers: 4});
const png = await client.screenshot({file: 'https://example.com'});
client.close();
```

`connect()` starts a daemon if none is listening, and several processes racing
to do that is fine: the losers exit and everyone ends up on the winner. One
connection can carry several requests at once — each is tagged, and the pool
spreads them across workers — which the worker protocol underneath cannot do.

```js
await shotium.daemon.start({workers: 4});   // start one and leave it running
await shotium.daemon.status();              // {running, pid, workers, served, warm, ...}
await shotium.daemon.stop();                // ask it to go away
await shotium.daemon.screenshot({file});    // connect, render, disconnect
```

A daemon is identified by its configuration — binary, workers, cache root,
extra flags — and the endpoint is a hash of those, so a client never silently
attaches to a pool that renders with something other than what it asked for.
Pass `name` to address one by name instead.

Who may talk to it matters, because a request may set `allowFileAccess` and get
this machine's files back as a picture. On POSIX the socket is chmod 0600, so
only its owner can connect. On Windows it is a named pipe and Node exposes no
way to give one an ACL — the default lets any account on the machine open it, so
a daemon on a shared Windows host is as trusted as that host's users are.

It prewarms: one throwaway document per worker at startup, so the first real
request does not pay for whatever a worker initialises lazily. It exits after
`idleTimeoutMs` with nothing connected and nothing rendering (default five
minutes, `0` to stay forever), and its workers are resident for as long as it
is — about 30 MB each, which is the price of not paying for startup again.

## The command line

```bash
npx shotium https://example.com -o out.png --width 1280 --height 720
npx shotium daemon start --workers 4
npx shotium daemon status
npx shotium daemon stop
```

The command line is a daemon client: the first invocation starts one, and the
rest of them are a socket connect. `--no-daemon` renders in the invoking process
instead, for a caller who would rather pay the startup than leave a process
behind. Pointing it at a local path is what allows that document to read the
filesystem, exactly as `shot`'s own command line does; a URL is not that
statement and does not get the permission.

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
