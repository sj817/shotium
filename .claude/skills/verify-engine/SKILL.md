---
name: verify-engine
description: >-
  Verify shotium rendering, native APIs, daemon behaviour and delivery against
  freshly built artifacts. Use for engine verification, release readiness or
  changes to the runtime and its bindings. Distinguish graph checks, successful
  compilation and runtime evidence; package-only CI does not verify the engine.
---

# Verify the engine

`checks.yml` does not use an engine binary. It runs package/tooling checks,
`verify:daemon-protocol` and `verify:bilibili --fixtures-only`; its path filters
include apps, tooling, release workflows, root READMEs and agent instructions;
inspect the workflow for the complete list. It also runs packaging unit tests
and actual example 7z round trips. An engine-only change may not trigger it. Neither
a green package check nor an absent run establishes engine correctness.
Runtime checks run in `engine-*.yml`, with delivery checks in `check-ffi.yml`;
inspect their conditions and skipped steps before reporting platform coverage.

## 0. Confirm the binary is the one you think it is

```powershell
Get-Item out/Shot/shotium.exe | Select-Object Name, Length, LastWriteTime
git --no-optional-locks status --short
```

Record the successful build command/result and the intended source state,
including uncommitted changes. Inspect the addon and/or shared library too
when the checks need them. Timestamps and hashes identify artifacts but do
not by themselves prove the source they contain. Use the intended artifacts
in `out/Shot`, not an experimental sibling directory.

## 1. Engine-side checks (TypeScript, `shotium.exe` only)

```powershell
pnpm verify:serve out/Shot/shotium.exe
pnpm verify:net   out/Shot/shotium.exe
pnpm verify:demos out/Shot/shotium.exe
pnpm verify:charset out/Shot/shotium.exe
```

| Script | Sections | What a failure means |
|---|---|---|
| `pnpm verify:serve` | `--serve` framing; two renders on one process are byte-identical; worker output equals CLI output; `allowFileAccess` actually gates subresources; refused requests are reported, not silently empty | The resident-worker contract is broken, or rendering is nondeterministic within a process |
| `pnpm verify:net` | http fetch, redirect following and limits, disk cache shared across two worker processes, `networkidle`, and the strongest one: the same document over http and from disk renders to identical bytes | `//net` integration or the loader changed what reaches Blink |
| `pnpm verify:demos` | Reftests in `shot/testdata/demos`: each `NAME.html` is compared with `NAME-ref.html`; pages without a reference are smoke tests; WPT-style `fuzzy` meta allows a declared tolerance | A layout or paint feature regressed; report the current pass/fuzzy/smoke counts rather than a historical expected count |
| `pnpm verify:charset` | Legacy CJK and single-byte encodings must render consistently with their UTF-8 equivalents | Encoding conversion or ICU data changes altered the document |

The scripts print their own pass/fail counts.

## 2. Node-side checks (need the addon)

Three preconditions; missing any one produces a convincing false failure:

1. **Rebuild the JS if `apps/typescript/src` changed.** The scripts `require()`
   `apps/typescript/dist`, which is tsdown output:

   ```bash
   pnpm -C apps/typescript build
   pnpm -C apps/typescript check:types
   ```

2. **Build the GN addon with the current core.**

   ```bash
   pnpm build:engine --target shot_node --log out/Shot/node-build.log
   ```

   It uses the pinned SDK from `scripts/build/node-sdk.ts`; no node-gyp or engine DLL
   is involved. `binding.ts` prefers `out/Shot/shotium.node` in a checkout,
   checks its internal binding version, then uses the installed platform package.
   Verify the addon timestamp and hash before attributing results to a source change.

3. **Run the native and daemon checks.**

   ```bash
   pnpm verify:node out/Shot/shotium.exe
   pnpm verify:node-entry
   pnpm verify:daemon out/Shot/shotium.exe
   pnpm verify:daemon-protocol
   ```

`scripts/verify/node-entry.ts` covers FIFO, failed requests, destroy with work in flight,
Buffer lifetime, Worker termination, natural exit and `UV_THREADPOOL_SIZE=1`.
`scripts/verify/node.ts` exercises `require()` of the ESM package (needs Node 22.12+ or 20.19+), `screenshot()`
returning `{image, stats}`, tiles, options validation, and `start()`/`stop()`
semantics. `scripts/verify/daemon.ts` spawns a detached daemon, connects, pipelines
requests, and stops it. `scripts/verify/daemon-protocol.ts` needs no engine and checks
that wire generations are isolated and negotiated.

## 3. Whole-page fixtures

When tiles, full-page rendering, the strip rasteriser or image decoding
changed:

```bash
pnpm verify:bilibili --package apps/typescript
```

Two real articles (41k and 46k CSS px tall), every tile, every photo and both
footer QR codes, entirely offline. `--fixtures-only` (what CI runs) only
proves the fixtures are complete.

## 4. Pixel regression (optional, needs baselines)

```powershell
pnpm render run --shot out/Shot/shotium.exe
```

`apps/test/render/baselines/` is gitignored. Without a manifest the run fails with
`Missing baseline manifest`; baselines must be generated with a *pre-change*
binary via `pnpm render update-baselines --accept`. If you did not do that before
building, skip this step, use `scripts/verify/demos.ts` as the pixel evidence, and say
which one you used. Thresholds default to exact decoded-pixel equality; a
changed SHA-256 with identical pixels is an encoder difference, not a
regression. Relax a per-case threshold only after looking at the generated
red-on-black diff and recording the reason in `cases.json`.

## 5. Acceptance run

```powershell
pnpm accept --skip-build
```

Reports the binary size, renders `shot/testdata/render_corpus.html` at
1248x1320, pixel-diffs it against the Chrome oracle in
`shot/testdata/out/oracle.png`, and then breaks the difference down region by
region -- a whole-image percentage cannot tell "antialiasing is a shade
different everywhere" from "one element is missing". A region that moved is
either an intended rendering change (document it in `apps/docs/cut-progress.md`
section 8.6, where the known differences are listed) or a regression.

Use `--skip-build` to check the artifacts just built; without it the script
starts `pnpm build:engine` again.

## 6. C ABI and package delivery

When the ABI, language demos or packaging changes, validate the relevant
delivery as well:

```powershell
pnpm verify:ffi --cli out/Shot/shotium.exe --library-dir out/Shot
pnpm verify:delivery --platform-dir <directory-containing-platform-tarball>
```

The FFI check needs the shared library, resource packs, a separate CLI and the
language tools. `--cli` defaults to the local `out/Shot` executable; delivery CI
must pass the extracted CLI explicitly, never assume it is inside the C ABI
package. For example on Windows:

```powershell
pnpm package:cli --os win --dest out/ffi-native/shotium-cli-windows-amd64 --check
pnpm package:c-abi --os win --dest out/ffi-native/shotium-c-abi-windows-amd64 --check
pnpm verify:ffi --cli out/ffi-native/shotium-cli-windows-amd64/shotium.exe --library-dir out/ffi-native/shotium-c-abi-windows-amd64 --source-dir out/language-sources/shotium-example-python --languages python
```

Repeat the source delivery check for go, python, rust, csharp and java after
extracting their `shotium-example-<language>.7z` files. On Linux/macOS use
`shotium` without `.exe` and the matching platform directories. Keep the ABI
version check. Before release, validate all six platforms and run
`pnpm package:checksums --dir dist/release --check` for the complete 17-archive
set and single `SHA256SUMS`; see the engineering reference for the contract.
The delivery check needs the staged platform tarball; it checks a clean npm
installation without relying on the checkout's addon or shared library.
Report missing prerequisites or omitted languages rather than implying full
delivery coverage. See `check-ffi.yml` for native platform conditions.

## 7. Report

Three levels, and only the third is "success":

1. **Graph passes**: `gn gen` + `ninja -n` (CI probe). Nothing compiled.
2. **Compiles**: the affected targets built successfully from the intended source.
3. **Binary + checks pass**: the required checks passed against those artifacts.

Report in this shape, with the script's own wording for any failure rather
than a paraphrase:

| Item | Result |
|---|---|
| Artifacts | Source state, successful build result, relevant artifact paths and identities |
| verify:serve | N passed / M failed |
| verify:net | N passed / M failed |
| verify:demos | pass / fuzzy / smoke counts, failures by name |
| verify:charset | N passed / M failed |
| verify:node / verify:node-entry | Results for each command |
| verify:daemon / verify:daemon-protocol | Results for each command |
| verify:bilibili | ran / skipped (why) |
| apps/test/render | ran / skipped (no baselines) |
| verify:ffi / verify:delivery | Coverage and results, or skipped (why) |

If a check was skipped, say so in the table; do not fold it into "all green".
