# shotium

Static screenshots from a stripped Chromium. DOM, CSS, layout, paint, fonts,
images, HTTP — everything a page needs to *look* right.

**There is no JavaScript engine.** V8 is not disabled, it is gone: deleted from
the tree, along with the browser layer that existed to host it. What is left is
Blink used directly, which turns out to be one self-contained executable —
41 MB on Windows, 12.8 MB compressed — that starts in under a second.

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

Built engines are on the [releases page], six archives per version:

| | x64 | arm64 |
|---|---|---|
| Windows | `shot-win-x64-v0.1.0.7z` | `shot-win-arm64-v0.1.0.7z` |
| macOS | `shot-mac-x64-v0.1.0.7z` | `shot-mac-arm64-v0.1.0.7z` |
| Linux | `shot-linux-x64-v0.1.0.7z` | `shot-linux-arm64-v0.1.0.7z` |

Each unpacks to one directory named for its platform and nothing else —
`shot-win-x64/`, `shot-mac-arm64/` — holding the executable and two `.pak`
files. The version is on the archive rather than inside it, so a newer build
unpacks over an older one in place.

The npm package is not published yet. Point it at an unpacked engine:

```bash
git clone https://github.com/sj817/shotium
cd shotium/shotium
npm link                      # or: require() it by path
export SHOTIUM_BINARY=/path/to/shot-win-x64/shot.exe    # `shot`, off Windows
```

With `SHOTIUM_BINARY` unset the package looks for `bin/shot.exe` — `bin/shot`
on macOS and Linux — beside `index.js`.

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
python tools/shot/serve_check.py   out/Shot/shot.exe   # protocol, geometry, encoders
python tools/shot/net_check.py     out/Shot/shot.exe   # http, redirects, cache, TLS
node   tools/shot/node_check.cjs   out/Shot/shot.exe   # pool, retry, crash isolation
python tools/shot/charset_check.py out/Shot/shot.exe   # legacy encodings vs the ICU cut
python tools/shot/demo_check.py    out/Shot/shot.exe   # 84 reftests
```

77 checks and 84 reftests. `node_check.cjs` is the only one that needs Node;
CI runs it on Windows and the rest everywhere. The ones worth knowing about:

- **The same document fetched over http and read off the disk must produce
  identical bytes.** The transport is not supposed to be visible in the picture.
- **`clip` and `selector` must find the same box, to the byte.** They are two
  ways of naming one rectangle; if they disagree, one of them is wrong.
- **A worker killed mid-request must come back on another one with the same
  image.** That is the claim the whole out-of-process design rests on.
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
