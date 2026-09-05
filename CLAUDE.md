# shotium Development Guide

Guidance for coding agents working in this repository. It covers what the tree
is, how it is built and verified, the rules that exist because something went
wrong once, and where the longer documents are. Multi-step procedures live in
`.claude/skills/` and are referenced as `/build-engine`, `/verify-engine`,
`/cut-component`, `/perf-compare` and `/release`.

## Critical rules

1. **Never restore V8, `//content`, the GPU process or DevTools.** Their
   absence is the product, not a gap. A page that needs JavaScript renders
   empty by design. See `docs/shotium-plan.md` section 5 for the full list of
   things this project deliberately does not do.
2. **Build only through `pnpm build:engine` (`scripts/build-engine.ts`),
   and only into `out/Shot`.** Do not hand-write `gn gen` or `autoninja` for
   the engine. Every other directory under `out/` is a stale experiment; a
   binary from one of them proves nothing.
3. **"Success" means a binary plus green checks.** Report status on three
   levels and never promote one: graph passes (`gn gen` + `ninja -n`, the CI
   "probe" mode, zero lines compiled) -> compiles (a binary exists) -> binary
   plus checks pass. `checks.yml` being green covers only the half of the
   project that needs no engine.
4. **Stage explicit paths.** `git add <path>`, never `-A`, `-u` or `.`. Other
   processes edit this working tree concurrently; `git add -A` has pushed
   someone else's half-finished work to the public repository before. Run
   `git --no-optional-locks status --short` before committing and leave any
   path you do not recognise alone.
5. **`git status` here holds `.git/index.lock` for tens of seconds.** Always
   pass `--no-optional-locks` to read-only git commands. For a write, delete a
   stale lock and run the command in the same shell invocation; never loop on
   the lock from a tool that gets killed mid-retry.
6. **Never `git merge upstream/main`, never push `codex/shot-engine-round1`.**
   The published history is a squashed root with no common ancestor; a merge
   is half a million conflicts. The old branch carries Chromium's 200k-commit
   ancestry and would push 10 GB. Upstream sync is a replay, documented in
   `docs/upstream-sync.md`.
7. **grep is not evidence that something is unused.** Whole-tree ripgrep
   times out silently and returns a partial result that looks complete. Use
   `git grep`, then `gn path out/Shot <target> <dep>`, and treat the compiler
   and linker as the judge.
8. **After deleting files, run `python tools/shot/missing_inputs.py out/Shot`
   before pushing.** GN copies input paths into `build.ninja` without checking
   they exist, and a warm build directory hides the gap until the next cold CI
   build, an hour later.
9. **Do not change font rasterisation.** Grayscale antialiasing, fixed gamma,
   `kUnknown_SkPixelGeometry`, and an isolated DirectWrite factory
   (`shot/shot_runtime.cc`). These are what make the same HTML render to the
   same bytes on every platform and in every process.
10. **Commit only when asked.** When you do, one file at a time is fine;
    message format is in [Git conventions](#git-conventions).
11. **Scripts are TypeScript in `scripts/`, and a library beats a hand-written
    utility.** `scripts/` is the only place new tooling goes: one kebab-case
    `.ts` file per command, run with `tsx` through `pnpm -C scripts` (root
    `package.json` forwards the common ones). `tools/shot/`, `tests/render/`
    and `bootstrap/` hold legacy scripts that move into `scripts/` as they are
    rewritten and get deleted from where they were. Use a
    library for everything that is not this project's own logic: `execa` for
    processes, `tinyglobby` for globs, `cac` for argument parsing, `p-retry`
    for retries, `sharp` / `pngjs` / `pixelmatch` for pixels. Do not write a
    PNG decoder, a glob matcher, an argument parser or a process wrapper by
    hand, and do not add to the remaining `.py`, `.ps1` and `.cjs` scripts;
    they are migrated per `docs/scripts-to-typescript.md`. The effort budget
    is for `shot/`, not for tooling.

## Project overview

shotium is a static HTML/CSS screenshot engine cut out of Chromium. It keeps
Blink (DOM, CSS, layout, paint), Skia (raster and encoders), `//net` (fetching,
disk cache) and the ICU/font stack, and drops everything that exists to be a
browser: V8, `//content`, the multi-process model, the compositor, viz, the GPU
process, DevTools, WebUI, media, WebRTC, ML, extensions. What is left renders a
document in the calling process, on one thread, and hands back PNG, JPEG or
WebP bytes.

It ships as the npm package `@shotkit/shotium` (TypeScript, ESM) plus six
platform packages `@shotkit/shotium-{win32,darwin,linux}-{x64,arm64}` that
carry the native engine (~22 MB each). The same engine is also a standalone
executable (`shotium.exe`) and a C ABI (`shotium.dll` / `libshotium.so` /
`libshotium.dylib`) for Rust, Go, Python and C++ callers.

The tree is a *slice* of Chromium, not a fork: 64k tracked files out of
upstream's 505k, pinned at the baseline recorded in `docs/upstream-sync.md`.
Chromium files that remain are edited in place; there is no patch queue for
in-tree code. `patches/` holds the three patches applied to DEPS-fetched
checkouts (ICU, Skia) that are not in git.

## Repository layout

```text
chromium/                      # this repo; upstream Chromium layout with most of it removed
├── shot/                      # the engine: one source_set, an executable and a shared library
│   ├── shot_api.h             # the C ABI -- the only header with no Chromium includes
│   ├── main.cc                # CLI entry point; also the --serve resident worker
│   ├── shot_server.{h,cc}     # --serve: length-prefixed JSON/bytes framing over stdin/stdout
│   ├── shot_runtime.{h,cc}    # process-wide setup: blink::Platform, cppgc, fonts, feature flags
│   ├── shot_renderer.{h,cc}   # Page/Frame, document install, WaitForLoad, lifecycle, paint
│   ├── shot_capture.{h,cc}    # one request in, one encoded image out; every caller goes through it
│   ├── shot_image_stream.*    # strip raster + streaming PNG/JPEG/WebP encoders
│   ├── shot_url_loader.*      # subresource loading: file: from disk, http(s) through //net
│   ├── shot_network.*         # process-level //net configuration and the disk cache
│   ├── shot_fetch.*           # per-response body budget and fetch results
│   ├── shot_cache.*           # HTTP cache listing/clearing
│   ├── shot_request.*         # the wire shape of one request (mirrors ScreenshotOptions exactly)
│   ├── shot_capture_context.* # CaptureStats: timings, request counts, cache hits
│   ├── shot_bytes.*           # move-only output buffer, reserved once and committed as it fills
│   ├── shot_platform.*        # blink::Platform: packed resources + MimeRegistry
│   ├── shot_options.*         # command-line parsing
│   ├── BUILD.gn               # targets shot_core, shot (exe), shot_c (dll), shot_resources
│   ├── VERSION / BRANDING     # moved here from chrome/
│   └── testdata/              # render corpus, reftest demos, offline Bilibili fixtures
├── shotium/                   # npm package @shotkit/shotium
│   ├── src/index.ts           # public API: Runtime, screenshot(), screenshotTiles(), daemon, cache
│   ├── src/types.ts           # every published type; adding an option means adding it here
│   ├── src/daemon_main.ts     # entry point of the detached daemon process
│   ├── src/lib/engine.ts      # owns the one engine handle a process gets
│   ├── src/lib/binding.ts     # finds and loads the .node addon (platform package first)
│   ├── src/lib/platform.ts    # which of the six platform packages carries this machine's engine
│   ├── src/lib/client.ts      # daemon client: spawn, connect, request pipeline
│   ├── src/lib/daemon.ts      # daemon server side
│   ├── src/lib/endpoint.ts    # daemon address = hash(wire generation, configuration)
│   ├── src/lib/protocol.ts    # the --serve wire format and the daemon wire generation
│   ├── src/lib/request.ts     # ScreenshotOptions -> wire request (viewport flattened)
│   ├── src/lib/cache.ts       # cache API and the URL glob matcher
│   ├── src/lib/config.ts      # StartOptions with defaults filled in
│   ├── native/                # Node-API addon (binding.cc, binding.gyp); links shotium.dll
│   ├── test/consumer.ts       # compiles against the published .d.ts through the exports map
│   └── dist/                  # tsdown output; what npm publishes, never committed
├── apps/benchmark/            # six-platform harness vs Puppeteer/Playwright (own pnpm workspace)
├── apps/benchmark-site/       # the published benchmark pages
├── tests/render/              # PowerShell pixel regression (needs locally generated baselines)
├── scripts/                   # repository scripts (@shotkit/scripts, own pnpm project); the only home for new tooling
│   ├── build-engine.ts        # the engine build entry point (pnpm build:engine)
│   ├── link-agent-skills.ts   # expose .claude/skills to .agents/skills (pnpm skills:link)
│   └── package.json           # execa, cac, p-retry, tsx ...; `pnpm -C scripts install`
├── tools/shot/                # legacy .py/.cjs/.ps1 scripts, frozen; each moves to scripts/ as TypeScript and is deleted here
├── build/args/shot*.gn        # GN args: shot.gn (base), shot-linux.gn, shot-mac.gn (CI overlays), shot-official.gn
├── build/config/shot_build.gni# declares is_shot_build
├── patches/                   # patches for DEPS checkouts (icu, skia); applied by the engine-*.yml patch step
├── bootstrap/                 # fresh-checkout setup (bootstrap.ps1)
├── docs/                      # shotium-plan.md, upstream-sync.md, cut-progress.md, demo assets
├── benchmark-results/         # committed aggregated benchmark results
├── .github/workflows/         # checks, engine-{windows,linux,macos}, benchmark, perf gate, publish
└── third_party/, base/, net/, third_party/blink/, skia/, cc/, ui/, url/ ...   # Chromium, cut down
```

## Architecture

### One path from request to bytes

```text
CLI (main.cc)          --serve worker (shot_server.cc)        shot_c library (shot_api.cc)
        \                        |                                  /
         `----------------- shot::Capture() (shot_capture.cc) ----'
                                  |
                         ShotRuntime (once per process)
                blink::Platform, cppgc heap, fonts, //net, mojo core
                                  |
                         ShotRenderer::Render()
   Page::CreateNonOrdinary + EmptyChromeClient/EmptyLocalFrameClient
   ForceSynchronousDocumentInstall(main document)
   WaitForLoad: subresources via ShotURLLoader, lifecycle at most every 50 ms,
                GC disabled for the capture (NoGarbageCollectionScope)
   UpdateAllLifecyclePhases -> paint into a cc::DisplayItemList
                                  |
                       ImageStream (shot_image_stream.cc)
   raster 256-row strips into address space reserved once (PA AllocPages),
   decode images on first use at the mip level actually drawn (cc::ImageProvider),
   encode rows as they finish: PNG (SkPngRustEncoder rows), JPEG (libjpeg-turbo
   scanlines), lossy WebP (YUV import per row band); lossless WebP needs the whole image
                                  |
                          shot::Bytes -> caller
```

The shape comes from Blink's own `IsolatedSVGDocumentHost`: an SVG image is a
full document that must lay out and paint without a renderer process, and shot
drives the page the same way. There is no compositor and no screenshot API in
between; the paint record *is* the output. `docs/cut-progress.md` section 14
records the design, section 16 the first end-to-end run.

Things that follow from this and are easy to get wrong:

- **Blink is a process-wide singleton and cannot be restarted.** `Platform::InitializeBlink()`,
  `cppgc::InitializeProcess()` and the scheduler binding happen once. The C API
  refuses a second `shot_engine_create()` for the life of the process; the JS
  `Runtime.start()` adopts the existing engine instead of building a new one,
  and throws if asked for a *different* configuration. Concurrency means more
  processes, which is what the daemon is for.
- **Paint coordinates stop at 32767 CSS px per axis, silently, inside Blink.**
  Regions beyond that (and every `tile` request) are rendered by scrolling the
  layout viewport in windows of at most 32000 px and rasterising each window
  into the right rows. Do not try to raise the limit; it is not in shot.
- **`RasterInducingScroll` must stay disabled** (`shot_runtime.cc`). With it on,
  scroll containers become `DrawScrollingContentsOp` and replay CHECKs on a
  compositor scroll table this binary does not have.
- **GC is disabled during a capture and run between captures** when the heap is
  over the threshold. Both halves are required: with no allocation between
  captures the growth heuristic never fires, so skipping the explicit
  collection means never collecting. `SHOT_WAIT_GC_MB` tunes the in-wait
  fallback. Do not reopen "remove the GC": Blink's object model is Oilpan
  (7,885 `GarbageCollected` types in core+platform).
- **`data:` URLs are not supported** by the engine or the worker, whatever
  older README text says. Test fixtures write a temporary file.
- **Every capture is byte-identical to every other capture of the same input**
  on the same platform. `serve_check.py` asserts it; treat any diff between two
  renders of one document as a bug.

### The C ABI (`shot/shot_api.h`)

The seam is C because the engine is built with clang, its own libc++ and its
own allocator, and none of that survives a caller built by another toolchain.
Opaque pointers, plain integers, NUL-terminated UTF-8 JSON, and nothing
allocated on one side is freed on the other.

| Function | Purpose |
|---|---|
| `shot_abi_version()` | Check before `shot_engine_create`; the addon refuses a mismatch |
| `shot_engine_create(options_json, &engine, &error)` | Once per process. `SHOT_ERR_STATE` on a second call |
| `shot_engine_capture(engine, request_json, ...)` | One image. Request JSON is `ScreenshotOptions` with the viewport flattened |
| `shot_engine_capture_tiles(...)` / `shot_tile_list_*` | Horizontal slices, each encoded on its own, handed over one at a time |
| `shot_engine_status`, `shot_engine_purge` | Stats and memory release |
| `shot_cache_list`, `shot_cache_clear` | The HTTP disk cache |
| `shot_buffer_data/size/free` | Every byte the engine returns; freed only with `shot_buffer_free` |
| `shot_engine_destroy` | Tears down the handle, not Blink |

Status codes: `SHOT_OK`, `SHOT_ERR_USAGE`, `SHOT_ERR_CAPTURE`, `SHOT_ERR_STATE`.
`shot_api.h` must never include a Chromium header. Exported symbols are listed in
`shot_api.exports` (Windows) and `shot_api.map` (ELF); the map hides
PartitionAlloc's `malloc`/`free` on purpose, which is why Linux builds set
`use_allocator_shim = false` in `build/args/shot-linux.gn` (two allocators in
one process cannot tell each other's pointers apart; fontconfig's `free()` of a
`realpath()` string trapped in `FreeInUnknownRoot`).

### The npm package (`shotium/`)

- **Zero runtime dependencies.** The glob matcher in `cache.ts` is thirty lines
  for that reason.
- **The JS layer is a transport, not a translation.** Field names in
  `ScreenshotOptions` are the wire field names; `shot_request.h` mirrors them.
  The addon carries `CaptureStats` JSON through without parsing it. Anything
  the JS layer has to rename is somewhere a bug can hide.
- **Loading the package must not start the engine.** `checks.yml` asserts
  `runtime.running === false` after `require()`.
- **Options are validated before the engine is touched** (`toRequest()`), and
  a bad request rejects with a `TypeError` without starting anything.
- **Engine discovery order** (`binding.ts`): the platform package for this
  `os`/`cpu`, then `native/build/Release/shotium.node`. A locally installed
  platform package therefore shadows a freshly built addon; see
  `/verify-engine`.
- **The daemon** (`daemon.ts`, `client.ts`, `endpoint.ts`) is a detached
  `node dist/daemon_main.js <base64 config>` process speaking the `--serve`
  framing over a socket whose address is a hash of the wire generation and the
  configuration. Two configurations are two daemons. The wire generation in
  `protocol.ts` is independent of the package version and the C ABI version;
  `tools/shot/daemon_protocol_check.cjs` checks negotiation.
- **Public surface** (`index.ts`): `Runtime` (`start`, `stop`, `status`,
  `running`, `screenshot`, `screenshotTiles`, `releaseMemory`, `cache`),
  module-level `screenshot`/`screenshotTiles`, `daemon.{connect,start,status,stop,screenshot,screenshotTiles}`,
  `cache`. Every type is in `types.ts`, and the README's API reference is kept
  in step with it by hand.

### The build layer

- `build/args/shot.gn` is the platform-neutral args template; `out/Shot/args.gn`
  is one `import` line. It sets `is_shot_build`, `use_shot_jumbo_build`,
  `is_official_build = true`, `dcheck_always_on = false`, `symbol_level = 0`,
  embeds ICU, and turns off every media/GPU/ANGLE backend. `shot-linux.gn`
  (Ozone headless, `use_allocator_shim = false`) and `shot-mac.gn` (system
  Xcode, no Clang modules) import it and are what CI generates with; their
  settings are errors on Windows, which is why they are not in `shot.gn`.
  A CHECK failure in this configuration is a bare `ImmediateCrash()` with no
  message: on Linux a naked `Trace/breakpoint trap`, on Windows exit
  `0x80000003`. Read the stack with gdb (`.symtab` is still present) or
  `cdb -g -G -lines -c "g; kc 40; q"`.
- `is_shot_build` gates a handful of `BUILD.gn` files and two `.cc` files
  (`IS_SHOT_BUILD`). Most of the cut is deletion, not conditionals.
- Jumbo builds (`use_shot_jumbo_build`, `shot_jumbo_file_merge_limit = 8`)
  concatenate sources into one TU per group. Grouping follows the source list,
  so "compiles on Windows" says nothing about Linux. Known cross-file failure
  modes: a trailing `#undef` in a header, an unqualified name resolved by an
  earlier file in the chunk, colliding anonymous namespaces or macros.
  Exclusions in `shot_jumbo_excluded_sources` fail silently when written on the
  wrong target or in the wrong path form; verify by reading
  `out/Shot/gen/<path>/<target>_shot_jumbo_N.cc`.
- Three upstream lines are *behavioural disagreements* rather than cuts
  (`parkable_image.cc` feature default, `cc/paint/draw_looper` `MaxOutset()`,
  `paint_chunks_to_cc_layer` extra parameters). They are tabulated in
  `docs/upstream-sync.md` and must be replayed by meaning, not by diff.

## Building

Local builds are Windows-only; Linux and macOS are built by CI.

```bash
pnpm -C scripts install                                                 # once per checkout
pnpm build:engine --jobs 16 --log out/Shot/build.log                    # shotium.exe
pnpm build:engine --target shot_c --jobs 16 --log out/Shot/build.log    # shotium.dll (the addon links this)
```

Output: `out/Shot/shotium.exe`, `out/Shot/shotium.dll`, `out/Shot/shotium_data.pak`,
`out/Shot/shotium_strings.pak`. GN target names are still `shot` and `shot_c`.

- `scripts/build-engine.ts` applies the Skia patches, regenerates the ICU
  data set, runs `gn gen`, then ninja with the output in the log file. It
  retries two failures that are not build errors: parallel toolchain
  variants racing on `environment.x64` (`PermissionError`), and ninja
  re-running `gn gen` itself. A relative `--log` is relative to where you
  typed the command.
- `--jobs`: 16 is the last known-good for a full build on the development host;
  24 hit `LLVM ERROR: out of memory` in Blink core jumbo TUs. Measure the
  phase you are about to run (`Get-Process clang-cl | Measure-Object WorkingSet64 -Sum`)
  rather than reusing a number. ThinLTO link memory is governed by
  `/opt:lldltojobs=N`, not `-j`.
- Stopping a background build kills the shell, not `ninja` or its `clang-cl`
  children. Before starting a build, kill leftovers twice:
  `Get-Process -Name ninja,clang-cl,rustc,lld-link -EA SilentlyContinue | Stop-Process -Force`.
- First build into `out/Shot` is ~5,800 steps (about 50 minutes); incremental
  builds touching only `shot/` take minutes.
- Read failures with `python tools/shot/errors.py out/Shot/build.log` or
  `build_errors.py --files`, never the raw log (6 KB of flags per failure).
- Front-end errors do not need a full build:
  `python tools/shot/check.py <file.cc>` runs `-fsyntax-only` from the compdb
  (8 s for a core/ TU instead of 40 s); `--from-log out/Shot/build.log` re-checks
  only the TUs that failed. Loop that to empty, then build again.
- Linux GN errors reproduce locally in ~30 s: a second out dir importing
  `//build/args/shot-linux.gn` plus `use_sysroot = false` and
  `host_toolchain = "//build/toolchain/linux:clang_x64"`. macOS cannot be
  cross-generated (`BUILDCONFIG.gn` asserts), so batch mac fixes before
  dispatching `engine-macos.yml`.
- Windows SDK: set both `CHROMIUM_WIN_SDK_VERSION` and `win_ntddi_version`
  together. Setting only the SDK version leaves `NTDDI_VERSION` undefined,
  which the preprocessor treats as 0 and every `#if NTDDI_VERSION >=` guard
  silently disappears. `engine-windows.yml`'s `windows sdk` step shows how to
  derive the second from the first.
- Checkout layout is unconventional: `.gclient` is in the parent directory
  (`D:\Github\.gclient`), `D:\Github\src` is a symlink to this repo, depot_tools
  is pinned in `D:\Github\depot_tools`. Run `gclient` from `D:\Github` with
  `DEPOT_TOOLS_UPDATE=0` and `DEPOT_TOOLS_WIN_TOOLCHAIN=0`. DEPS and
  `.gitmodules` are separate; gclient reads only DEPS, so deleting a checkout
  means deleting its DEPS entry too.

## Verifying

| Check | Command | Needs | Asserts |
|---|---|---|---|
| Worker protocol | `python tools/shot/serve_check.py out/Shot/shotium.exe` | exe | `--serve` framing, two renders on one process byte-identical, worker == CLI bytes, `allowFileAccess` gate |
| Network stack | `python tools/shot/net_check.py out/Shot/shotium.exe` | exe | http fetch, redirects, disk cache shared across processes, `networkidle`, http bytes == file bytes |
| Node addon | `PATH="$PWD/out/Shot:$PATH" node tools/shot/node_check.cjs out/Shot/shotium.exe` | dll + addon | `require()` of the ESM package, `screenshot()`, `{image, stats}` shape, tiles |
| Daemon | `PATH="$PWD/out/Shot:$PATH" node tools/shot/daemon_check.cjs out/Shot/shotium.exe` | dll + addon | spawn, connect, pipeline, stop |
| Daemon wire | `node tools/shot/daemon_protocol_check.cjs` | none | generation isolation and negotiation |
| Reftests | `python tools/shot/demo_check.py out/Shot/shotium.exe` | exe | 84 pairs in `shot/testdata/demos`: page vs `-ref` page byte-identical (62 pass, 1 fuzzy, 21 smoke) |
| Bilibili fixtures | `python tools/shot/bilibili_check.py --package shotium` | addon | two whole articles, every tile, every photo, both QR codes |
| Pixel regression | `pwsh tests/render/run.ps1 -ShotExecutable out/Shot/shotium.exe` | baselines | decoded-pixel equality against a locally generated baseline |
| Corpus digest | `tools/shot/accept.ps1` | exe | `shot/testdata/render_corpus.html` SHA-256 unchanged |

- `PATH` must contain `out/Shot` for the Node checks: the addon links
  `shotium.dll`, and without it you get `ERR_DLOPEN_FAILED`.
- The reftest suite has no golden images on purpose: cutting Blink rarely
  fails to compile, it fails by laying out slightly differently, and a suite
  of stored PNGs would either need re-blessing after every cut or drown in
  diffs. A `-ref.html` states the expected pixels in CSS the cut cannot
  plausibly break. Pages without a `-ref` are smoke tests (renders, more than
  one colour, deterministic on a second run). Bounded differences use WPT's
  `<meta name="fuzzy" content="maxDifference=N;totalPixels=A-B">`.
- `tests/render/baselines/` is gitignored. `run.ps1` fails with
  `Missing baseline manifest` until `update-baselines.ps1 -Accept` has been
  run with a pre-change binary. If you did not generate baselines before
  changing the engine, use `demo_check.py` for pixel evidence and say so.
- CI's `checks.yml` runs on every push and pull request but never touches an
  engine. The four scripts above run only in the manually dispatched
  `engine-*.yml` workflows. Changing the public shape of `shotium/src`
  without running `node_check.cjs` and `daemon_check.cjs` locally has shipped
  broken checks before.

## Testing conventions

- A rendering feature gets a reftest pair `shot/testdata/demos/NAME.html` +
  `NAME-ref.html`. The reference uses only absolutely positioned blocks with
  background colours, or the same text through a path the feature does not
  touch.
- Anything with antialiasing or blur (no exact restatement possible) gets a
  smoke page without a `-ref`.
- Engine behaviour that is not pixels (protocol, cache, budget, tiles) goes
  into `serve_check.py` / `net_check.py` / `node_check.cjs` / `daemon_check.cjs`
  as a numbered section; each script prints `N passed / M failed`.
- Harness logic has unit tests: `tools/shot/node_perf_gate.test.cjs`
  (`node` test runner) and `apps/benchmark/test/*.test.ts` (`pnpm run check`).
  Both run in `checks.yml`.
- Fixtures must be offline. `bilibili_check.py --fixtures-only` verifies the
  Bilibili corpus references nothing on the network.
- Measure performance with local files, never remote URLs (see
  [Performance](#performance-and-memory)).

## Code conventions

**C++ (`shot/` and edited Chromium files)**

- Chromium style, `clang-format` with the root `.clang-format`. Match the
  surrounding file when editing upstream code.
- Comments explain *why*; the top of every `shot/*.h` is a short essay on what
  the file is for and which alternative was rejected. Keep that register. Do
  not add comments that restate the code.
- `-Wunsafe-buffer-usage` is an error. Pointer arithmetic goes through
  `UNSAFE_BUFFERS(base::span(p, n))` with a `// SAFETY:` comment. Assigning a
  `GUARDED_BY` field outside its lock is also an error.
- `wexit_time_destructors` is on for `shot_core`, `shot` and `shot_c`.
- `CHECK(...) << msg` produces no message in this build configuration; write
  the invariant so the function name in the stack is enough.
- Do not neutralise a check with `&& false`; delete the block and say why.
- Chromium files are edited in place. Cut, do not `#if 0`. If a header is
  needed only for its types and the upstream call sites already test the
  pointer, restore the header unchanged rather than rewriting the callers.
- Generated IDL enums/unions/dictionaries come from the in-tree generators in
  `tools/shot/gen_idl_*.py`. The enum and union generators have a `--check`
  mode that compares their output against identifiers found at call sites.
  Run it; inferred naming rules have produced code that looked right and
  broke hundreds of files.

**TypeScript (`shotium/src`, `apps/benchmark/src`, `tools/shot/*.cjs`)**

- ESM, strict TypeScript, no runtime dependencies in the package.
  `pnpm run build` (tsdown) then `pnpm run check:types`.
- New options go in `types.ts` with a doc comment, in `shot_request.h` under
  the same name, in `toRequest()` validation, and in the README API reference.
- Do not add a third opinion about a shape: the addon passes JSON through, and
  the C++ side parses `ScreenshotOptions` by the same field names.
- No `npx`; use `pnpm dlx` or `node_modules/.bin`. The package manager is
  pinned (`packageManager` in `package.json`).

**Scripts (`scripts/*.ts`)**

- One kebab-case file per command, a `// why` header, a `cac` CLI,
  `process.exitCode` rather than `process.exit()` in the middle.
  `tsconfig.json` has `erasableSyntaxOnly`, so the files also run under
  Node's own type stripping. `checks.yml` typechecks the directory on every
  push.
- The library-first rule, made concrete:

  | Need | Use | Not |
  |---|---|---|
  | Run a process, capture or tee output, retries | `execa` (+ `p-retry`) | `child_process` wrappers, PowerShell `&` |
  | Find files | `tinyglobby` | recursive `readdir`, `Get-ChildItem -Recurse` |
  | Parse arguments | `cac` | `process.argv` slicing, `param()` blocks |
  | Read, compare, resize images | `sharp`, `pngjs`, `pixelmatch` | hand-written PNG decoders, `System.Drawing`, Pillow |
  | Hash, temp files, paths | `node:crypto`, `node:fs/promises`, `node:path` | `Get-FileHash`, `certutil` |
  | Tables and colour in output | `picocolors`, `console.table` | manual padding |
  | Tests for a script | `node:test` (`*.test.ts`) | ad-hoc assertion scripts |

- pnpm runs a package script with `cwd = scripts`; resolve user-supplied
  relative paths against `process.env.INIT_CWD`, and the repository root as
  `path.resolve(import.meta.dirname, '..')`.
- Every script that CI calls has a root `package.json` alias so the workflow
  YAML stays a list of `pnpm <name>` lines rather than inline shell.

**Cutting (deleting Chromium code)**

- Delete anything with no live caller today. Do not keep files for a rebase
  that will never happen; this is a slice, and sync is a replay.
- Decide "restore or cut" by what the code actually depends on, not by its
  name or directory. The decision table is in `docs/cut-progress.md`
  section 11 and in `/cut-component`.
- Rust crates: the `features` list is the list of things other code uses.
  Read `lib.rs` before removing one.
- Delete DEPS entries with the directories they fetch, and `.gitmodules` with
  them, or CI's `gclient sync` puts the directory back.

## Git conventions

- Remotes: `origin` = `sj817/shotium` (push here), `upstream` =
  `chromium/chromium` (read-only, partial clone; `ls-tree` and `log` work
  offline). `gh` resolves to the upstream repo by default; always pass
  `-R sj817/shotium`.
- Branches: local `release` tracks remote `main`; `git push` is configured to
  do the right thing. Feature branches are normal branches off `release`. The
  local orphan `main` (`Initial commit`) is a leftover and nothing's ancestor.
- Commit messages: English, Conventional Commits, lower-case scope from
  `shot`, `shotium`, `blink`, `bench`, `ci`, `docs`. The body explains
  the mechanism and gives before/after numbers; release notes are written from
  bodies, and a body-less commit forces the next person to read the diff.
- Never rewrite published history on `main`. Never push tags except as part of
  `/release`.
- Auto memory for this repository (`~/.claude/projects/<repo>/memory/`) holds
  host-specific facts (paths, `-Jobs`, SDK versions). Prefer it over guessing
  when a rule here says "on the development host".

## CI

| Workflow | Trigger | What it does |
|---|---|---|
| `checks.yml` | push to `main`, every PR, dispatch | The engine-less half: package builds, types, `require()` does not start the engine, option validation, daemon wire, harness syntax + unit tests, six platform packages consistent, tarball contents, Python scripts compile, Bilibili fixtures offline, PNG decoder sanity. ~40 s, expected always green |
| `engine-windows.yml` | dispatch (`arch`, `jobs`, `run_checks`) | depot_tools, SDK, `gclient sync`, timestamp restore, cached `out/`, `gn gen`, `ninja`, package `.7z`, node platform package, run the check suites. Cold ~4 h, warm ~25 min |
| `engine-linux.yml`, `engine-macos.yml` | dispatch (`mode` = probe or build, `arch`, `jobs`, `run_checks`) | Same, plus `probe` = `gn gen` + `ninja -n` only. Probe green is level 1 of 3, not success |
| `benchmark.yml` | dispatch (`shotium_version`, `profile`, `commit_results`, `seed`) | 30-job `platform x shard` matrix against Puppeteer/Playwright; commits aggregated results to `benchmark-results/` |
| `benchmark-pages.yml` | push to `main` touching results or site | Publishes `apps/benchmark-site` |
| `performance-regression.yml` | dispatch (`baseline_version`, `build_runs` JSON) | Candidate vs published npm on six platforms through `node_perf_ci.cjs`, gated by `node_perf_gate.cjs` |
| `publish.yml` | tag `v*`, or dispatch with `dry_run` | Collects the six engine artifacts at the tag's commit, publishes seven npm packages, then creates the GitHub release |

- Read a failed run with `gh run view -R sj817/shotium <id> --log-failed`.
- A CI red in the benchmark is not the same as "the benchmark found
  something": competitor-engine failures are recorded in `summary.json` /
  `failures.json` and do not fail the run; only a shotium failure, a harness or
  host error, or an exhausted budget does. The rule lives in both `cli.ts` and
  `merge-shards.ts` and must change in both.
- Engine build caches are keyed by content-faithful mtimes
  (`tools/shot/ci_stamp_mtimes.py`); without that step every CI build is cold.

## Release

The order is enforced by `publish.yml`, which looks up engine artifacts by the
tag's commit:

1. Bump the version in `shotium/package.json`: `version` plus the six
   `optionalDependencies` pins (7 lines). Nothing else hard-codes it.
2. Push, wait for `checks.yml`.
3. Dispatch six engine builds on that exact commit (`mode=build` for Linux and
   macOS; probe produces no artifact).
4. `git tag -a vX.Y.Z && git push origin vX.Y.Z`. Any commit after the tag, or
   a build on an earlier commit, makes the collect step fail with
   `NOT FOUND at this commit`.
5. `publish.yml` publishes the six platform packages, then the main package,
   then creates a non-draft release with the six `.7z` archives. Do not use
   draft releases: they create no git tag and freeze `targetCommitish`.
6. Verify all seven packages on the registry. `@shotkit/shotium-win32-arm64`
   has lagged the CDN by ~15 minutes twice; the publish log line
   `Publishing to ...` without `(dry-run)` is the evidence, not the 404.
7. Write real release notes: English half on top, Chinese half below, each
   complete; material comes from commit bodies.

Full procedure: `/release`.

## Performance and memory

- Instrument with `SHOT_PROFILE=1` and `--verbose` (or `SHOT_VERBOSE=1` for
  the library); `LOG(INFO)` is filtered otherwise. It prints per-stage memory
  (`shot: mem <stage>`: working set, private, PA partitions, cppgc, Skia,
  fonts, discardable) and wait-loop timings. A `pump=` figure near a multiple
  of 15.6 ms means the wait loop is sleeping on the watchdog, not loading.
- Baselines on the development host: idle engine ~15 MB private; a simple
  page ~9 ms in `--serve`; the two Bilibili articles (41k/46k px tall) render
  full-page PNG within ~115 MB and ~240 MB peak private respectively.
- Measure with local files or written fixtures. A remote URL includes DNS,
  TCP and TLS (240-390 ms cold, 0.6 ms cached), and the cache default has
  flipped between releases (0.1 on, 0.2 off, 0.3 on), so cross-version
  comparisons of remote URLs are false regressions. `stats.fromCache` is
  `URLRequest::was_cached()`, which counts a 304 revalidation as a hit.
- Report peak private and working set separately (discardable shared
  segments only appear in the working set).
- The benchmark harness's process sampler is itself host load on Windows and
  is duty-cycled; never reintroduce `si.powerShellStart()`, which deadlocks
  `si.processes()`. macOS runners have no usable CPU gate and arm64 runners
  have no comparison engines; read `summary.json`'s engine list before
  comparing numbers across platforms.
- Binary size: `obj` size is not image contribution (ThinLTO removes the
  unreachable). Ask `tools/shot/size_report.py`, not the dependency graph.

Procedure: `/perf-compare`; methodology: `tools/shot/PERFORMANCE.md`.

## Environment variables

| Variable | Read by | Effect |
|---|---|---|
| `SHOT_PROFILE` | runtime, fetch, renderer | Per-stage memory and timing logs (needs `--verbose` / `SHOT_VERBOSE`) |
| `SHOT_VERBOSE` | `shot_api.cc` | Library-mode equivalent of `--verbose` |
| `SHOT_PARK_IMAGES` | renderer | Park compressed image bytes to a scratch file while loading; default on (47 MB off a 72 MB-photo page) |
| `SHOT_WAIT_GC_MB` | renderer | Heap growth that triggers a collection during `WaitForLoad` |
| `SHOT_FETCH_BUDGET_MB`, `SHOT_FETCH_CONCURRENCY` | `shot_fetch.cc` | Response-body memory budget and parallel fetch limit |
| `SHOT_STRIP_ROWS`, `SHOT_STRIP_BUDGET_MB`, `SHOT_STRIP_MARGIN`, `SHOT_SINGLE_STRIP` | image stream | Strip raster geometry; tuning knobs, not configuration |
| `SHOT_RASTER_THREADS`, `SHOT_STREAM_PNG`, `SHOT_DISCARD_ENCODED`, `SHOT_DUMP_OPS`, `SHOT_RECLAIM`, `SHOT_PROFILE_WAIT` | image stream / renderer | Experiments; read the `getenv` site before relying on one |
| `SHOT_INCLUDE_DIR`, `SHOT_LIB_DIR` | `shotium/native/binding.gyp` | Where the addon finds `shot_api.h` and `shotium.dll` (defaults: `../../shot`, `../../out/Shot`) |
| `CHROMIUM_WIN_SDK_VERSION` | `build/vs_toolchain.py` | Windows SDK directory; pair with `win_ntddi_version` in args |
| `DEPOT_TOOLS_WIN_TOOLCHAIN=0`, `DEPOT_TOOLS_UPDATE=0` | depot_tools | Use the local Visual Studio; do not self-update |

## Key documents

| File | Read it when |
|---|---|
| `README.md`, `README.zh.md` | You need the public API or the marketing numbers (their source is the 0.3.3 linux-x64 CI run) |
| `shotium/README.md` | The package's API reference; keep it in step with `types.ts` |
| `docs/shotium-plan.md` | Decisions (section 0), architecture (1), network stack choice (2), public interface contract (4), explicit non-goals (5) |
| `docs/upstream-sync.md` | Baseline hash, why merge is impossible, the four-bucket replay, the deliberate disagreements table, post-sync checks |
| `docs/cut-progress.md` | 1,800 lines of what was removed and why; sections 8 (V8 removal), 11 (restore vs cut), 14 (driving Blink directly), 17 (size composition), 20 (making it usable), 21 (CI) |
| `tools/shot/PERFORMANCE.md` | The candidate-vs-npm methodology and stopping rules |
| `docs/scripts-to-typescript.md` | The inventory of legacy `.py` / `.ps1` / `.cjs` scripts, which library replaces each hand-written part, and the migration order |
| `apps/benchmark/README.md` | The six-platform harness, shards, and what counts as a run failure |
| `tests/render/README.md` | Pixel regression and baseline generation |
| `shot/README.md` | Historical; the first extraction baseline (predates the direct-Blink design) |

## Other agents and skills

- `AGENTS.md` only points here, for Codex and other tools that read that name.
- Skills are canonical in `.claude/skills/`. Codex and Gemini CLI look in
  `.agents/skills/`; run `pnpm skills:link` once per checkout to link them
  (junctions on Windows, symlinks elsewhere; the directory is gitignored).
- Host-specific facts belong in auto memory or `CLAUDE.local.md` (gitignored),
  not here.
