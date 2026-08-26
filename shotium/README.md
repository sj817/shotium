# shotium

Static screenshots from a stripped Chromium. DOM, CSS, layout, paint, fonts,
images, HTTP — everything a page needs to *look* right. No JavaScript engine.

```js
import shotium from '@shotkit/shotium';

shotium.runtime.start({workers: 4});
const png = await shotium.screenshot({
  file: 'https://example.com',
  viewport: {width: 1280, height: 720},
  fullPage: true,
});
await shotium.runtime.stop();
```

## What it is

`shotium` — `shotium.exe` on Windows — is Chromium with V8 removed and the browser
layer removed with it: no `//content`, no render process, no GPU process, no
compositor, no sandbox.
What is left is Blink used directly — `Page` → `LocalFrame` → `Document` →
lifecycle → `cc::PaintRecord` → CPU raster — which is the same shape Blink
already uses internally for SVG images.

This package is the supervisor for those processes: no native addon on this
side, no node-gyp, no ABI matrix. The worker talks a length-prefixed protocol
over stdio.

It is written in TypeScript and ships the types generated from that source, so
the declarations and the code cannot disagree — there is no second description
of the API to keep in step. What is published is the bundle, plus the sources
and sourcemaps it was built from, so a stack trace out of a failed screenshot
names a line rather than a column.

It is an ES module, and only that. `import` works anywhere; `require()` of it
works on Node 20.19 and 22.12 and later, and a CommonJS caller on anything
older reaches it with `await import('@shotkit/shotium')`. One format rather
than two because everything here is a process-wide singleton — one pool, one
daemon per configuration, and an in-process engine blink will not start twice
— and a package built both ways gives a caller who reaches it both ways two of
each.

Two entry points, and nothing else is reachable:

| | |
|---|---|
| `@shotkit/shotium` | `runtime`, `screenshot`, `daemon`, `Runtime` |
| `@shotkit/shotium/native` | `native`, `screenshot`, `NativeRuntime` |

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

The engine is 41 MB of Chromium and there is a different one for every platform
and architecture, so it does not ship inside this package. It ships in six of
its own:

```
@shotkit/shotium-win32-x64    @shotkit/shotium-darwin-x64    @shotkit/shotium-linux-x64
@shotkit/shotium-win32-arm64  @shotkit/shotium-darwin-arm64  @shotkit/shotium-linux-arm64
```

All six are `optionalDependencies` of this package with `os` and `cpu` set, so
`npm install @shotkit/shotium` fetches the one that matches the machine and
skips the other five. There is no postinstall script and nothing is downloaded
outside the registry — what the lockfile pins is what you get.

Two ways to override that, in the order they win:

```bash
export SHOTIUM_BINARY=/path/to/shotium          # or shotium.exe
```

…or pass `binary` to `runtime.start()`. With neither set and no platform
package installed — a machine nobody builds for, or a checkout — the package
looks for `bin/shotium` (`bin/shotium.exe` on Windows) beside `index.js`, which
is where an archive from the [releases page] unpacks to.

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

## In this process, instead

`@shotkit/shotium/native` is the same engine loaded into this process rather
than started beside it. There is no worker, no pipe and no supervisor: a
screenshot costs about a third less, and the whole thing is one process.

```js
import {native} from '@shotkit/shotium/native';

const png = await native.screenshot({file: 'https://example.com'});
native.purge({releaseWorkingSet: true});   // when the batch is over
await native.stop();
```

Same options, and the same bytes: `tools/shot/native_check.cjs` compares the
addon's output against the executable's for the same document, because two
paths to blink that could disagree are two engines rather than one.

Two things it does not have, both of them things a separate process was
providing for free. It is **one renderer** -- the singleton above applies to
this process too, and `worker_threads` share it, so requests are serialised
however many callers there are. And there is **no crash isolation**: a renderer
that dies takes the host with it, where the pool would have refilled the slot
and retried.

So: this for a program that takes screenshots one at a time and would rather
not run a pool; `runtime` or `daemon` for a service.

Under it is a shared library with a C ABI -- `shot/shot_api.h`, eight
functions, JSON in and bytes out -- and a thin Node-API addon over that. Both
ship in the platform package beside the engine, because the library is a
Chromium build and `npm install` is not going to produce one. Nothing else in
this package needs them.

## No command line

There is no `bin` here, and `npx @shotkit/shotium` does nothing. A command line
that renders a page is a program, not a library entry point: it wants to start
fast, and paying node's startup and this package's import to reach an engine
that is already resident is most of what a one-shot screenshot costs.

So the command line ships as a binary of its own, from the [releases page],
alongside the engine archives. This package is for programs that are already
running node, and it stops there.

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
