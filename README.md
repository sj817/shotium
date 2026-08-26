**English** · [简体中文](README.zh.md)

# shotium

Static screenshots from a stripped Chromium. DOM, CSS, layout, paint, fonts,
images, HTTP — everything a page needs to *look* right.

**There is no JavaScript engine.** V8 is not disabled, it is gone: deleted from
the tree, along with the browser layer that existed to host it. What is left is
Blink used directly, which turns out to be one self-contained executable —
41 MB on Windows, 12.8 MB compressed — that starts in under a second.

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

---

## What this repository is

A fork of Chromium with most of it removed, plus two things that are ours:

| | |
|---|---|
| `shot/` | the engine. A ~1,800-line C++ program that drives Blink directly |
| `shotium/` | the npm package. Process pool, queue, retry, types — plain JavaScript, plus a native addon |
| `tools/shot/` | the checks: protocol, geometry, network, and the JS layer |
| `bench/` | the benchmarks: this engine against Chromium, and against puppeteer and playwright |
| `docs/cut-progress.md` | how the tree was cut, and what broke on the way |
| `docs/shotium-plan.md` | the current design and what is deliberately not done |

Everything else is Chromium with 1.4 GB of things the build never reads taken
out, so cloning it is 0.57 GB rather than 2 GB.

## What it is not

A browser. There is no `//content`, no render process, no GPU process, no
compositor, no sandbox, no extensions, no devtools, no V8.

The consequences are worth stating plainly, because they are the product
boundary rather than a list of bugs:

- **A page that builds itself with script photographs as an empty page.** No
  `<script>`, no framework hydration, no client-side routing. If your target is
  a React app with no server rendering, this tool is the wrong tool.
- `selector` is resolved with `Document::querySelector` inside the renderer.
  Nothing is injected into the page, because there is nothing to inject it with.
- **Which fonts exist is the host's business. How they are rasterised is
  not.** Text is drawn with grayscale antialiasing at a fixed gamma, with no
  subpixel geometry and no read of the host's ClearType settings, so the same
  page renders byte-identically on every process on a platform. Two *different*
  platforms still differ by a hair at glyph edges — CoreText and DirectWrite
  round an advance differently — and a font the host does not have is a font
  the page does not get.
- **A page that declares a legacy encoding renders as mojibake.**
  `ForceSynchronousDocumentInstall` installs the document with the encoding
  hardcoded to UTF-8, and an explicit encoding outranks `<meta charset>`, so a
  `shift_jis` or `gbk` declaration is never consulted.
  `tools/shot/charset_check.py` measures this every run rather than assuming it
  is fine; whoever fixes it should re-run that script.
- There is no SSRF protection and no resource ceiling beyond a body-size cap.
  Deciding which URLs may be fetched is the caller's job.

## Using it

```bash
npm install @shotkit/shotium
```

It is an ES module, and only that. `import` works anywhere; `require()` of it
works on Node 20.19 and 22.12 and later, and on anything older a CommonJS
caller reaches it with `await import('@shotkit/shotium')`.

Shipping one format rather than two is deliberate. Everything this package
holds is a process-wide singleton — `runtime` is one pool, the daemon is
addressed by a hash so that two callers find one process, and blink refuses
outright to start a second engine in a process that has had one. A package
built both ways hands a caller who reaches it both ways two of each, and the
failure that produces is a second pool nobody asked for and an engine that
will not start, some distance from the `require` that caused it.

Two entry points, and nothing else is reachable:

| | |
|---|---|
| `@shotkit/shotium` | `runtime`, `screenshot`, `daemon`, `Runtime` |
| `@shotkit/shotium/native` | `native`, `screenshot`, `NativeRuntime` |

The engine is 41 MB of Chromium and there is a different one per platform and
architecture, so it is not in that package. It is in six of its own —
`@shotkit/shotium-win32-x64`, `@shotkit/shotium-darwin-arm64`, and the other
four — declared as `optionalDependencies` with `os` and `cpu` set. npm installs
the one that matches the machine and skips the rest. They are named for
`process.platform`, which is what npm matches `os` against; the archives below
spell those two platforms `win` and `mac`, because those are read by people. No postinstall script, no
download outside the registry: what the lockfile pins is what arrives.

The same engines are on the [releases page] as six archives per version, for a
caller who is not installing from npm at all:

| | x64 | arm64 |
|---|---|---|
| Windows | `shotium-win-x64-v0.1.0.7z` | `shotium-win-arm64-v0.1.0.7z` |
| macOS | `shotium-mac-x64-v0.1.0.7z` | `shotium-mac-arm64-v0.1.0.7z` |
| Linux | `shotium-linux-x64-v0.1.0.7z` | `shotium-linux-arm64-v0.1.0.7z` |

Each unpacks to one directory named for its platform and nothing else —
`shotium-win-x64/`, `shotium-mac-arm64/` — holding the executable and two `.pak`
files. The version is on the archive rather than inside it, so a newer build
unpacks over an older one in place.

Point the package at one with `SHOTIUM_BINARY`, which outranks the platform
package if both are there:

```bash
export SHOTIUM_BINARY=/path/to/shotium-win-x64/shotium.exe    # `shotium`, off Windows
```

With that unset and no platform package installed, it looks for
`bin/shotium.exe` — `bin/shotium` on macOS and Linux — beside `index.js`.

```js
import {runtime, screenshot} from '@shotkit/shotium';

runtime.on('crash',   ({worker})          => console.warn('worker', worker, 'died'));
runtime.on('timeout', ({worker, timeout}) => console.warn('worker', worker, 'hung'));
runtime.on('stderr',  ({line})            => console.error(line));

runtime.start({
  workers: 4,                              // default: half the cores
  cacheDir: '/var/tmp/shotium-cache',       // null disables the HTTP cache
});

await screenshot({file: 'https://example.com', path: 'out.png'});
```

`screenshot()` resolves to a `Buffer`, or to `null` when `path` was given and
the worker wrote the file itself.

### The resident daemon

`runtime` holds its workers for as long as the Node process holding it. A
command line, a CI step or a queue worker is a process that exists for one
screenshot, and for that process starting the workers *is* the cost of the
picture.

`daemon` is the same pool behind a socket — a named pipe on Windows, a unix
socket elsewhere — so the next process finds it already warm and already
prewarmed:

```js
import {daemon} from '@shotkit/shotium';

const client = await daemon.connect({workers: 4});   // starts one if none is up
const png = await client.screenshot({file: 'https://example.com'});
client.close();

await daemon.status();    // {running, pid, workers, served, warm, ...}
await daemon.stop();
```

```bash
npx @shotkit/shotium https://example.com -o out.png    # first call starts the daemon
npx @shotkit/shotium daemon status
```

A daemon is addressed by a hash of its configuration — binary, workers, cache
root, flags — so a client never silently attaches to a pool that renders with
something other than what it asked for. It exits five minutes after the last
client leaves, and one connection can carry several requests at once.

### In this process, instead

`runtime` and `daemon` both put blink in a worker process. The native engine
puts it here:

```js
import {native} from '@shotkit/shotium/native';

const png = await native.screenshot({file: 'https://example.com'});
native.purge({releaseWorkingSet: true});   // when the batch is over
await native.stop();
```

Same options and the same bytes — `tools/shot/native_check.cjs` compares them
against what the executable produces for the same document, because two paths
to blink that could disagree are two engines. What differs is the shape: one
process instead of five, about **31 ms** a screenshot instead of 47, and about
**81 MiB** of working set instead of 190.

What it gives up is what a separate process was providing for free. **One
renderer** — blink is a process-wide singleton and `worker_threads` share the
process, so requests are serialised however many callers there are, and the
pool's four-at-once has no equivalent here. And **no crash isolation**: a
renderer that dies takes the host program with it, where the pool would have
retried on another worker.

Underneath is a shared library with a C ABI, [`shot/shot_api.h`](shot/shot_api.h)
— eight functions, opaque pointers, JSON in and bytes out, nothing owning
memory across the seam. The addon is a thin piece of Node-API over it that does
not read what it carries. The header is not node-specific; ctypes, cgo and
libloading take it as-is.

Both ship prebuilt in the platform package, beside the engine they belong to,
because the library underneath is a Chromium build and `npm install` is not
going to do that. `@shotkit/shotium` itself needs neither.

### Options

```ts
interface ScreenshotOptions {
  file: string                        // http/https/file URL, or a local path
  type?: 'png' | 'jpeg' | 'webp'      // default png
  fullPage?: boolean                  // whole document, not just the viewport
  selector?: string                   // capture one element's box
  quality?: number                    // 1-100, jpeg and webp only, default 90
  scale?: number                      // device scale factor, 0.01-8, default 1
  omitBackground?: boolean            // keep alpha instead of painting white
  path?: string                       // write here instead of returning bytes
  viewport?: { width?: number; height?: number }        // default 1280x720
  pageGotoParams?: {
    timeout?: number                  // ms, default 30000
    waitUntil?: 'load' | 'networkidle'
  }
  clip?: { x: number; y: number; width: number; height: number }
  allowFileAccess?: boolean           // let the document read file: subresources
  retry?: number                      // re-sends after a crash or a timeout
}
```

`selector`, `clip` and `fullPage` each name the region to capture, and they name
different ones, so asking for two is an error rather than a guess. Likewise
`quality` on a png, and `omitBackground` on a jpeg — jpeg has no alpha channel.
A field that parsed but could not be honoured is refused, never silently
dropped.

### Or just the binary

```bash
shotium https://example.com --width 1280 --height 720 -o out.png
shotium --file page.html --full-page --type webp --quality 85 -o out.webp
shotium --serve --cache-dir /var/tmp/shotium-cache    # resident worker, see shot/shot_server.h
shotium --help
```

---

## Numbers

Against puppeteer and playwright, on ten static local documents at 1280x720,
PNG, viewport only, `waitUntil: 'load'`, a fresh page per screenshot, one fresh
process tree per sample, seven repeats, medians. The method and every caveat
are in [`bench/cross/`](bench/cross/README.md); the full report — every column,
every scenario, and the samples that failed — is
[`bench/cross/RESULTS.md`](bench/cross/RESULTS.md). A 32-core Windows desktop
that was doing other things at the time.

| engine | cold start | per shot | per shot, page reused | ten pages, 4 at a time | memory there |
|---|--:|--:|--:|--:|--:|
| **shotium** | **352 ms** | **47 ms** | — | **237 ms** (42/s) | **256 MiB** / **73 private** |
| puppeteer, chrome-headless-shell | 946 ms | 133 ms | 61 ms | 905 ms (11/s) | 647 MiB / 180 private |
| puppeteer, headless Chrome | 1559 ms | 132 ms | 50 ms | 1890 ms (5/s) | 1287 MiB / 379 private |
| playwright, chrome-headless-shell | 962 ms | 150 ms | **33 ms** | 1171 ms (9/s) | 652 MiB / 215 private |
| playwright, headless Chrome | 1385 ms | 146 ms | 45 ms | 1276 ms (8/s) | 789 MiB / 282 private |

*Cold start* is a whole process: node, `require`, launching the engine, one
screenshot. *Per shot* is the marginal cost on an engine already running, with
startup entirely out of it.

*Memory* is two numbers because a tree of processes does not have one. The
first is the sum of working sets, which is what task manager adds up and which
charges every process separately for pages it shares with its siblings — four
shotium workers each mapping the same 43 MiB of `shotium.exe`, twenty-one chrome
processes each mapping the same `chrome.dll`. The second is the sum of private
working sets: the pages belonging to exactly one process, nothing counted
twice. The real cost is between them, and the two bracket it.

The one column shotium loses is the interesting one. Navigating **one page** from
document to document is faster than making a new one, and Chrome lets you do
that; shotium builds and tears down a `Page` per request whether you want it or
not, so it has no equivalent. Where that stops helping is concurrency: four
pages driven at once cost Chrome four renderer processes and most of a
gigabyte, and the four workers answering four requests at once are what shotium is
shaped for.

With the engine already up — a CLI invocation, a queue worker, a request
handler — what a **fresh process** pays for one screenshot:

| engine | client, end to end | connect | screenshot | resident | engine only | resident processes |
|---|--:|--:|--:|--:|--:|--:|
| **shotium daemon** | **250 ms** | **2.3 ms** | **57 ms** | **58 MiB** | **2.8 MiB** | 5 |
| puppeteer, chrome-headless-shell | 512 ms | 17 ms | 170 ms | 355 MiB | 287 MiB | 8 |
| puppeteer, headless Chrome | 588 ms | 19 ms | 204 ms | 587 MiB | 519 MiB | 12 |
| playwright, chrome-headless-shell | 764 ms | 38 ms | 189 ms | 272 MiB | 154 MiB | 5 |
| playwright, headless Chrome | 680 ms | 34 ms | 228 ms | 400 MiB | 299 MiB | 7 |

Both sides are attaching to something they did not start: shotium over its
named pipe, puppeteer over `browserWSEndpoint`, playwright over a
`launchServer()`. The resident columns are what each costs while nothing at all
is happening, sampled after every engine has been left alone for fifteen
seconds. *Engine only* takes the node processes out, and for shotium that is
almost all of it: a worker whose queue has been quiet for ten seconds collects
blink's heap, drops the caches, and hands its pages back to the OS, so four
resident renderers cost 2.8 MiB between them and the node supervising them
costs the rest. The next request pays about 8 ms of soft page faults to get
them back — see `PurgeMemory` and `ReleaseWorkingSet` in
[`shot/shot_runtime.h`](shot/shot_runtime.h). The last step of that, handing
the pages back, is a Windows working-set trim; the collection and the caches
are not, and on Linux PartitionAlloc's reclaimer has already returned the heap
by the time it would run. These are Windows numbers either way — the table was
measured on one host, and no equivalent run exists for Linux yet.

**What none of this measures is script.** The corpus is static documents,
because that is what shotium can photograph. On a page that builds itself in the
browser these numbers do not apply — that page comes out blank, and no
benchmark result changes it.

---

## Building the engine

This is a Chromium checkout, so it is built like one — **but it is a fork, which
changes one thing: `gclient` must not manage `src`.** Set `"managed": False`, or
the next `gclient sync` will reset the tree to upstream and undo everything this
repository is.

You need [depot_tools] on your `PATH`, about 40 GB of disk, and a host
toolchain:

| | |
|---|---|
| Windows | Visual Studio with the Windows SDK, plus the arm64 toolset for an arm64 target |
| macOS | Xcode. Leave `FORCE_MAC_TOOLCHAIN` unset — the hermetic Xcode needs access outside Google nobody has |
| Linux | `sudo ./build/install-build-deps.sh --no-prompt --no-nacl` |

The tree pins Windows SDK **10.0.28000**. The pin is a directory name rather
than a compatibility statement, and it lives in two files that must agree —
`build/vs_toolchain.py` and `build/toolchain/win/setup_toolchain.py`. If the
SDK you have installed is a different one, name it instead of editing both —
and name the NTDDI symbol out of that same SDK:

```bash
export CHROMIUM_WIN_SDK_VERSION=10.0.26100.0
# and in out/Shot/args.gn:  win_ntddi_version = "NTDDI_WIN11_GE"
```

Both halves matter. `win_ntddi_version` defaults to `NTDDI_WIN11_BR`, and an
NTDDI identifier the preprocessor has never heard of compares as 0 instead of
failing — so naming one your SDK does not define quietly switches off every
version-guarded declaration, and it surfaces thousands of lines later as
`unknown type name`. Take the highest `NTDDI_WIN*` that SDK's
`shared/sdkddkver.h` actually defines. That is what CI does, because Chromium's
own toolchain package is not downloadable outside Google and a hosted runner
has whatever SDK its image shipped with.

```bash
mkdir shotium-build && cd shotium-build

cat > .gclient <<'EOF'
solutions = [{
  "name": "src",
  "url": "https://github.com/sj817/shotium.git",
  "managed": False,
  "custom_deps": {},
  "custom_vars": {"checkout_configuration": "small"},
}]
target_os = ["win"]          # or ["mac"], or ["linux"]
EOF

gclient sync --nohooks --no-history
gclient runhooks

cd src

# The ICU data set is generated rather than checked in: third_party/icu is a
# DEPS checkout, so gclient would discard it. This has to run before gn gen.
python3 tools/shot/icu_repack.py \
  third_party/icu/cast/icudtl.dat \
  third_party/icu/shot/icudtl.dat --preset shot

mkdir -p out/Shot
echo 'import("//build/args/shot.gn")' > out/Shot/args.gn
gn gen out/Shot
ninja -C out/Shot shot
```

**`target_os` decides which DEPS entries exist at all.** The `.gclient` checked
into this tree says `["win"]`, so a sync that inherits it on another platform
omits every entry gated on that platform and fails much later — on a missing
include, rather than on anything that names the cause.

Use `build/args/shot-mac.gn` or `build/args/shot-linux.gn` in the `import` on
those platforms. Both import `shot.gn` and add what the platform needs, so
there is still one place where a configuration decision is written down.

The `import` rather than `--args="$(cat ...)"`: the file contains quoted values,
which do not survive being interpolated into a shell argument.

`build/args/shot.gn` is the release configuration: official build, ThinLTO,
`-Os`, DCHECKs off, and a list of features turned off with a comment on each
saying why. Every argument in it is a size or correctness decision that is
written down.

### Parallelism

Blink's layout code needs **1.1–1.3 GB per compiler process** at its heaviest —
`-j 20` peaks around 25 GB. Sizing `-j` from the light translation units and
extrapolating is how you get nine `LLVM ERROR: out of memory` two hours in.
Budget from the peak, not the average.

### Checks

```bash
python tools/shot/serve_check.py   out/Shot/shotium.exe   # protocol, geometry, encoders
python tools/shot/net_check.py     out/Shot/shotium.exe   # http, redirects, cache, TLS
node   tools/shot/node_check.cjs   out/Shot/shotium.exe   # pool, retry, crash isolation
node   tools/shot/daemon_check.cjs out/Shot/shotium.exe   # the resident daemon
node   tools/shot/native_check.cjs out/Shot/shotium.exe   # the in-process engine
python tools/shot/charset_check.py out/Shot/shotium.exe   # legacy encodings vs the ICU cut
python tools/shot/demo_check.py    out/Shot/shotium.exe   # 84 reftests
```

104 checks and 84 reftests. The three `.cjs` suites are the ones that need
Node, and they run on Windows and Linux — the daemon listens on a named pipe on
one and a unix socket on the other, and a suite that ran on only one platform
would be checking half of it. The ones worth knowing about:

- **The same document fetched over http and read off the disk must produce
  identical bytes.** The transport is not supposed to be visible in the picture.
- **`clip` and `selector` must find the same box, to the byte.** They are two
  ways of naming one rectangle; if they disagree, one of them is wrong.
- **A worker killed mid-request must come back on another one with the same
  image.** That is the claim the whole out-of-process design rests on.
- **The addon and the executable must produce the same bytes for the same
  document.** They reach blink two different ways — one through a pipe, one
  through a C ABI — and the only thing making them one engine is that they
  agree.
- **A second process must find the daemon rather than start one, and get the
  same bytes the in-process pool produces.** Two entry points that could drift
  apart would be two renderers.
- **Every reftest states its expected result in CSS the cut cannot break** — a
  pair of pages that must render identically, rather than a stored image that
  would need re-blessing after every cut and would hide the regressions in the
  noise. A page with no reference is a smoke test instead: it must render, show
  more than one colour, and produce the same bytes twice.

`tools/shot/size_report.py` attributes every byte of the binary to the object
file it came from, via the PDB's section contributions — no `/MAP` relink and no
`symbol_level` bump required.

---

## How it works

```
shotium (npm, plain JS)          pool · lifecycle · on() events · retry
     │
     │  stdio: length-prefixed JSON request / length-prefixed binary response
     ▼
shotium.exe --serve                 resident worker, one Blink per process
     │
     ▼
Blink   DOM → CSS → layout → paint → raster → encode
```

`shot_renderer.cc` is the whole pipeline:

```
Page::CreateNonOrdinary + LocalFrame + LocalFrameView
  → LocalFrame::ForceSynchronousDocumentInstall("text/html", bytes, url)
  → LocalFrameView::UpdateAllLifecyclePhases()   style, layout, prepaint, paint
  → LocalFrameView::GetPaintRecord()             the paint phase's output
  → SkiaPaintCanvas over an SkSurface            CPU raster
  → PNG / JPEG / WebP
```

This is not a reimplementation of anything. It is the shape Blink already uses
internally for SVG images — a full document that cannot have a renderer process
of its own; see `core/svg/graphics/isolated_svg_document_host.cc`.

**Why rendering is out of process:** Blink is a process-wide singleton.
`Platform::InitializeBlink()` builds WTF and the partitions once,
`cppgc::InitializeProcess()` sits behind a `NoDestructor`, and the main-thread
scheduler binds to one thread. So one process renders one document at a time,
concurrency means more processes, and crash isolation falls out of the same
fact.

**Networking** is `//net` linked directly — `URLRequest`, the HTTP cache,
BoringSSL — with no `//services/network` above it. That service is the mojo
wrapper multi-process Chrome puts around exactly these objects so a sandboxed
renderer can reach them; a worker is already its own process, so the wrapper
would be a pipe to itself. HTTPS, HTTP/2, redirects, brotli, an in-memory cookie
jar and an optional disk cache are on; HTTP/3 and proxies are not.

## Size

`icu_use_data_file = false` links ICU's table into the executable, so there is
no `icudtl.dat` to ship. What ships is the binary and two `.pak` files, 118 KB
between them.

| | raw | .7z |
|---|---:|---:|
| Windows x64 | 41.5 MB | **12.80 MB** |
| Windows arm64 | | **10.56 MB** |
| macOS arm64 | 38.4 MB | **12.29 MB** |
| Linux x64 | 70.2 MB | **15.80 MB** |

Down from 336 MB. The macOS figure is from the `tar.xz` that job produced
before the archive formats were unified; the same LZMA2 either way, so expect
it to move by kilobytes rather than megabytes. The Linux raw size is the one
number here that is not yet accounted for — 70 MB of ELF against 41 MB of PE
from the same source is a gap worth measuring, not explaining away.

`docs/cut-progress.md` has the whole account, including the measurement
method — and where the next few megabytes are, measured rather than guessed.

## Licence

Chromium's, unchanged: BSD-3-Clause, see [LICENSE](LICENSE).

[depot_tools]: https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up
[releases page]: https://github.com/sj817/shotium/releases
