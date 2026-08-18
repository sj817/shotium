# shotium

Static screenshots from a stripped Chromium. DOM, CSS, layout, paint, fonts,
images, HTTP — everything a page needs to *look* right.

**There is no JavaScript engine.** V8 is not disabled, it is gone: deleted from
the tree, along with the browser layer that existed to host it. What is left is
Blink used directly, which turns out to be about 50 MB and starts in under a
second.

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

---

## What this repository is

A fork of Chromium with most of it removed, plus two things that are ours:

| | |
|---|---|
| `shot/` | the engine. A ~1,800-line C++ program that drives Blink directly |
| `shotium/` | the npm package. Process pool, queue, retry, types — plain JavaScript |
| `tools/shot/` | the checks: protocol, geometry, network, and the JS layer |
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
- **Fonts come from the host system.** The same HTML on two machines can differ
  by a pixel. That is accepted, not fixed.
- There is no SSRF protection and no resource ceiling beyond a body-size cap.
  Deciding which URLs may be fetched is the caller's job.
- **Windows only, today.** The tree was cut on Windows; Linux and macOS need
  their platform files restored, which is real work and has not been done.

## Using it

The npm package is not published yet. Point it at a built engine:

```bash
git clone https://github.com/sj817/shotium
cd shotium/shotium
npm link                      # or: require() it by path
export SHOTIUM_BINARY=/path/to/shot.exe
```

```js
const {runtime, screenshot} = require('shotium');

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
shot https://example.com --width 1280 --height 720 -o out.png
shot --file page.html --full-page --type webp --quality 85 -o out.webp
shot --serve --cache-dir /var/tmp/shot-cache    # resident worker, see shot/shot_server.h
shot --help
```

---

## Building the engine

This is a Chromium checkout, so it is built like one — **but it is a fork, which
changes one thing: `gclient` must not manage `src`.** Set `"managed": False`, or
the next `gclient sync` will reset the tree to upstream and undo everything this
repository is.

You need [depot_tools] on your `PATH`, Visual Studio with the Windows SDK
(**10.0.28000** is what the tree pins), and about 40 GB of disk.

The pin is a directory name rather than a compatibility statement, and it lives
in two files that must agree — `build/vs_toolchain.py` and
`build/toolchain/win/setup_toolchain.py`. If the SDK you have installed is a
different one, name it instead of editing both:

```bash
export CHROMIUM_WIN_SDK_VERSION=10.0.26100.0
```

That is what CI does, because Chromium's own toolchain package is not
downloadable outside Google and a hosted runner has whatever SDK its image
shipped with.

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
target_os = ["win"]
EOF

gclient sync --nohooks --no-history
gclient runhooks

cd src
mkdir -p out/Shot
echo 'import("//build/args/shot.gn")' > out/Shot/args.gn
gn gen out/Shot
ninja -C out/Shot shot
```

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
python tools/shot/serve_check.py out/Shot/shot.exe    # protocol, geometry, encoders
python tools/shot/net_check.py   out/Shot/shot.exe    # http, redirects, cache, TLS
node   tools/shot/node_check.cjs out/Shot/shot.exe    # pool, retry, crash isolation
```

76 checks. The ones worth knowing about:

- **The same document fetched over http and read off the disk must produce
  identical bytes.** The transport is not supposed to be visible in the picture.
- **`clip` and `selector` must find the same box, to the byte.** They are two
  ways of naming one rectangle; if they disagree, one of them is wrong.
- **A worker killed mid-request must come back on another one with the same
  image.** That is the claim the whole out-of-process design rests on.

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
shot.exe --serve                 resident worker, one Blink per process
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

| | raw | compressed (7z, BCJ+LZMA2) |
|---|---:|---:|
| `shot.exe` + `icudtl.dat` + two `.pak` files | 57.7 MB | **16.7 MB** |

Down from 336 MB. `docs/cut-progress.md` has the whole account, including the
measurement method — and where the next few megabytes are, measured rather than
guessed.

## Licence

Chromium's, unchanged: BSD-3-Clause, see [LICENSE](LICENSE).

[depot_tools]: https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up
