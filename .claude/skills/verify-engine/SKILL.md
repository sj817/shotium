---
name: verify-engine
description: Run every check that needs a built engine and report on the three-level status ladder: serve_check.py, net_check.py, node_check.cjs, daemon_check.cjs, daemon_protocol_check.cjs, demo_check.py reftests, bilibili_check.py, tests/render. Use for "verify", "did the checks pass", "is it ready to release", and as the last step after any change to shot/, shotium/src, shotium/native or the Blink/Skia/cc code they depend on. CI's checks.yml does not run any of these.
---

# Verify the engine

`checks.yml` never touches an engine, and it is path-filtered: a change
confined to `shot/`, Blink, `docs/` or `.claude/` produces no run at all, so
neither its green nor its absence says anything here. The checks below are what
actually establish that the engine works, and they run in CI only in
the manually dispatched `engine-*.yml` workflows. The public JS API has
changed shape before with CI fully green and every one of these scripts
broken; that is why this is a skill and not a footnote.

## 0. Confirm the binary is the one you think it is

```powershell
Get-Item out/Shot/shotium.exe, out/Shot/shotium.dll | Select-Object Name, Length, LastWriteTime
git --no-optional-locks log -1 --format='%h %ci' -- shot/
```

The binary's timestamp must be later than the last source change you intend
to verify. Results from `out/ShotWip` or any other directory are void.

## 1. Engine-side checks (Python, `shotium.exe` only)

```powershell
python tools/shot/serve_check.py out/Shot/shotium.exe
python tools/shot/net_check.py   out/Shot/shotium.exe
python tools/shot/demo_check.py  out/Shot/shotium.exe
```

| Script | Sections | What a failure means |
|---|---|---|
| `serve_check.py` | `--serve` framing; two renders on one process are byte-identical; worker output equals CLI output; `allowFileAccess` actually gates subresources; refused requests are reported, not silently empty | The resident-worker contract is broken, or rendering is nondeterministic within a process |
| `net_check.py` | http fetch, redirect following and limits, disk cache shared across two worker processes, `networkidle`, and the strongest one: the same document over http and from disk renders to identical bytes | `//net` integration or the loader changed what reaches Blink |
| `demo_check.py` | 84 reftest pairs in `shot/testdata/demos`: each `NAME.html` must render byte-identical to `NAME-ref.html`; pages without a `-ref` are smoke tests (renders, more than one colour, identical on a second run); WPT-style `fuzzy` meta allows a declared tolerance | A layout or paint feature regressed. Expected: 62 pass, 1 fuzzy, 21 smoke |

`demo_check.py` needs Pillow. All three print their own pass/fail counts.

## 2. Node-side checks (need the addon)

Three preconditions; missing any one produces a convincing false failure:

1. **Rebuild the JS if `shotium/src` changed.** The scripts `require()`
   `shotium/dist`, which is tsdown output:

   ```bash
   cd shotium && pnpm run build && pnpm run check:types
   ```

2. **Rebuild the addon if the C ABI or `shotium.dll` changed.** `binding.gyp`
   reads `SHOT_INCLUDE_DIR` (default `../../shot`) and `SHOT_LIB_DIR`
   (default `../../out/Shot`):

   ```bash
   cd shotium/native && SHOT_INCLUDE_DIR=$PWD/../../shot SHOT_LIB_DIR=$PWD/../../out/Shot pnpm dlx node-gyp@13 rebuild
   cp ../../out/Shot/shotium.dll ../../out/Shot/shotium_data.pak ../../out/Shot/shotium_strings.pak build/Release/
   ```

3. **Check the addon you just built is newer than the change.**
   `shotium/src/lib/binding.ts` resolves `native/build/Release/shotium.node`
   *first* and falls back to `@shotkit/shotium-win32-x64` (or the platform's
   equivalent) only when the local build is absent. So the platform package
   does not need moving -- and moving it fixes nothing. What does go wrong is
   the reverse: a *stale* local addon silently wins, and any new ABI call
   fails with `native.xxx is not a function`. If you see that after changing
   `shot_api.h`, rebuild the addon (step 2); do not go looking for a package
   in the way.

   ```powershell
   Get-Item shotium/native/build/Release/shotium.node, out/Shot/shotium.dll |
       Select-Object Name, Length, LastWriteTime
   ```

Then:

```bash
PATH="$PWD/out/Shot:$PATH" node tools/shot/node_check.cjs   out/Shot/shotium.exe
PATH="$PWD/out/Shot:$PATH" node tools/shot/daemon_check.cjs out/Shot/shotium.exe
node tools/shot/daemon_protocol_check.cjs
```

`PATH` must include `out/Shot`: the addon links `shotium.dll` and the loader
reports `ERR_DLOPEN_FAILED` otherwise. `node_check.cjs` exercises
`require()` of the ESM package (needs Node 22.12+ or 20.19+), `screenshot()`
returning `{image, stats}`, tiles, options validation, and `start()`/`stop()`
semantics. `daemon_check.cjs` spawns a detached daemon, connects, pipelines
requests, and stops it. `daemon_protocol_check.cjs` needs no engine and checks
that wire generations are isolated and negotiated.

## 3. Whole-page fixtures

When tiles, full-page rendering, the strip rasteriser or image decoding
changed:

```bash
python tools/shot/bilibili_check.py --package shotium
```

Two real articles (41k and 46k CSS px tall), every tile, every photo and both
footer QR codes, entirely offline. `--fixtures-only` (what CI runs) only
proves the fixtures are complete.

## 4. Pixel regression (optional, needs baselines)

```powershell
pwsh tests/render/run.ps1 -ShotExecutable out/Shot/shotium.exe
```

`tests/render/baselines/` is gitignored. Without a manifest the run fails with
`Missing baseline manifest`; baselines must be generated with a *pre-change*
binary via `update-baselines.ps1 -Accept`. If you did not do that before
building, skip this step, use `demo_check.py` as the pixel evidence, and say
which one you used. Thresholds default to exact decoded-pixel equality; a
changed SHA-256 with identical pixels is an encoder difference, not a
regression. Relax a per-case threshold only after looking at the generated
red-on-black diff and recording the reason in `cases.json`.

## 5. Acceptance run

```powershell
pwsh tools/shot/accept.ps1 -SkipBuild
```

Reports the binary size, renders `shot/testdata/render_corpus.html` at
1248x1320, pixel-diffs it against the Chrome oracle in
`shot/testdata/out/oracle.png`, and then breaks the difference down region by
region -- a whole-image percentage cannot tell "antialiasing is a shade
different everywhere" from "one element is missing". A region that moved is
either an intended rendering change (document it in `docs/cut-progress.md`
section 8.6, where the known differences are listed) or a regression.

`-SkipBuild` is not optional here: without it the script's first act is
`pnpm build:engine`, which is the ~50-minute build you have just finished.

## 6. Report

Three levels, and only the third is "success":

1. **Graph passes**: `gn gen` + `ninja -n` (CI probe). Nothing compiled.
2. **Compiles**: a binary exists.
3. **Binary + checks pass**: everything above is green against a fresh binary.

Report in this shape, with the script's own wording for any failure rather
than a paraphrase:

| Item | Result |
|---|---|
| Binary | `out/Shot/shotium.exe`, size, timestamp |
| serve_check | N passed / M failed |
| net_check | N passed / M failed |
| demo_check | pass / fuzzy / smoke counts, failures by name |
| node_check | N passed / M failed |
| daemon_check | N passed / M failed |
| bilibili_check | ran / skipped (why) |
| tests/render | ran / skipped (no baselines) |

If a check was skipped, say so in the table; do not fold it into "all green".
