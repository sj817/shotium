# Engineering reference

Read the section relevant to the task after [CLAUDE.md](../../CLAUDE.md).
This file holds engineering details that do not need to load for every task.
Commands and implementation facts should be checked against their linked source;
historical timings and build counts belong in the design records.

## Engine contracts

The main implementation path is:

```text
CLI / --serve / C ABI / Node addon
    -> Capture (shot_capture.cc), with EngineService for native API callers
    -> ShotRuntime and ShotRenderer
    -> Blink layout and paint record
    -> ImageStream strip rasterisation and encoding
    -> Bytes returned to the caller
```

- [shot_engine.h](../../shot/shot_engine.h) and
  [shot_engine.cc](../../shot/shot_engine.cc) own the native engine service.
  Blink initialises once per process; its work is serialised on the engine
  thread. Preserve the non-nestable queue so a nested resource-loading loop
  cannot start another capture inside the current one. The Node addon uses
  TSFN completion rather than occupying a libuv worker while Blink runs.
- [shot_runtime.cc](../../shot/shot_runtime.cc) owns process setup, fonts and
  feature flags. Keep `RasterInducingScroll` disabled: its paint operations
  require compositor scroll state that this engine does not provide.
- Preserve grayscale antialiasing, fixed gamma and the isolated Windows
  DirectWrite factory. These remove host ClearType/cache variation; they do
  not supply identical fonts to different machines. Compare determinism in
  a controlled environment and use explicit fonts in portable fixtures.
- [shot_renderer.cc](../../shot/shot_renderer.cc) implements document setup,
  resource waiting and lifecycle updates. Blink's painted coordinates have a
  32767 CSS-pixel limit. Full-page and tile capture work around it with bounded
  viewport windows; changing the requested output size does not raise it.
- Keep the renderer's GC policy intact when changing memory behaviour:
  collection is normally suppressed during capture, has a bounded fallback
  while waiting, and runs between captures when needed. Dropping the latter
  can leave a resident engine holding garbage indefinitely. Check the actual
  `SHOT_WAIT_GC_MB` read site before tuning its threshold.
- [shot_image_stream.cc](../../shot/shot_image_stream.cc) handles strip
  rasterisation and encoding. PNG/JPEG and lossy WebP can stream row bands;
  lossless WebP needs the whole image. Preserve image/encoded-data lifetimes,
  tile coordinates and output ownership when changing the strip path.
- [shot_capture.cc](../../shot/shot_capture.cc) accepts a local path, `file:`,
  `http:` or `https:` as the main document. Main-document `data:` URLs are
  unsupported; this does not describe the separate subresource-loading rules.
  Tests should write their HTML to a temporary file.
- Network work is split among `shot_url_loader.*`, `shot_network.*`,
  `shot_fetch.*` and `shot_cache.*`. Changes to fetching must preserve body
  budgets, redirect handling, cancellation and the file-access gate.

## API changes

Read [the C ABI header](../../shot/shot_api.h),
[the C ABI guide](../c-abi/README.md), and
[the TypeScript types](../typescript/src/types.ts) for the current contract.

- The ABI uses opaque handles, integers and UTF-8 JSON. Allocations stay with
  their owner: callers use `shot_buffer_free`, not their own allocator.
  Preserve output-clearing rules on errors and the once-per-process engine
  lifetime. Destroying a handle does not make Blink restartable.
- Keep ABI changes aligned with `shot_api.exports`, `shot_api.map` and
  `shot_abi_version()`. The ELF symbol map hides allocator symbols deliberately;
  exporting them can make host libraries free pointers through the wrong heap.
- A public screenshot option has matching fields in `types.ts`,
  `shot/shot_request.h`, the TS `toRequest()` validator and shared C++ readers.
  Review the native conversion in `apps/typescript/native/binding.cc` and
  the JSON adapters; do not introduce independent names or default values.
- Update `apps/typescript/README.md` and `README.zh.md` with public API changes,
  and the two `apps/c-abi/README*` guides when the C contract changes. The latter
  are shipped as `C_ABI.md` and `C_ABI.zh.md` in engine archives.
- [binding.ts](../typescript/src/lib/binding.ts) tries the checkout's
  `out/Shot/shotium.node` before an installed platform package. A matching
  binding version does not prove that an old addon contains today's source.
  Rebuild the addon and identify the loaded binary before native tests or
  performance comparisons.
- Importing the package must not initialise Blink. Invalid options must reject
  before loading/starting the native engine. Rebuild `dist/` before tests that
  load the published entry point; generated declarations still need typechecking.
- [protocol.ts](../typescript/src/lib/protocol.ts) owns the worker/daemon wire
  format. Daemon wire generation, native binding version, C ABI version and npm
  version are separate compatibility mechanisms. Update the one affected and
  test negotiation rather than assuming a package-version bump handles it.
- Daemon endpoints depend on wire generation and configuration. Different
  configurations must not silently attach to the same incompatible daemon.

## Tooling conventions

Repository automation lives in `scripts/`, grouped as `build`, `verify`, `ci`,
`perf`, `package`, `tree` or `docs`. [The root package](../../package.json) maps
each command to one kebab-case `.ts` file; use that `pnpm` alias in documentation
and CI. Keep reusable code in `scripts/lib/` and tests next to the module.

| Need | Existing building block |
|---|---|
| Processes and retries | `execa`, `p-retry` |
| File discovery | `tinyglobby` |
| Argument parsing | `cac`, with the `pnpm` alias as the CLI name |
| Image decoding and comparisons | `sharp`, `pngjs`, `pixelmatch` |
| Paths, hashing, files | `node:path`, `node:crypto`, `node:fs/promises` |
| Output and tests | `picocolors`, `console.table`, `node:test` |

- Prefer these libraries over custom process wrappers, parsers, glob matchers
  or image decoders. New repository commands are TypeScript, not new `.py`,
  `.ps1` or `.cjs` utilities. Existing upstream build tools keep their own
  conventions; this rule is for shotium's repository tooling.
- Include a short purpose/rationale comment. Use a `cac` command action so
  unknown options are rejected rather than silently starting the default
  operation. Keep process exit handling at the command boundary.
- Resolve repository and user-supplied relative paths through
  [scripts/lib/repo.ts](../../scripts/lib/repo.ts). Root aliases launch another
  pnpm in `scripts/`, so `INIT_CWD` cannot reliably recover the user's original
  directory. State the repository-relative path rule in `--help`.
- Keep syntax compatible with `scripts/tsconfig.json` (`erasableSyntaxOnly`).
  `pnpm scripts:check` checks types and `pnpm scripts:test` runs the registered
  tests. Inspect `scripts/package.json` when adding a test in a new command
  group: its test globs are explicit.
- Install `scripts/` with its committed lockfile. `apps/typescript/` uses
  `pnpm install --no-lockfile` in CI because its optional platform packages may
  be pinned to an unpublished version; do not use `--no-optional` to hide that
  issue, since the package build also needs a native bundler dependency.
- There is no root pnpm workspace. Consult
  [the tooling migration record](scripts-to-typescript.md) before changing
  project boundaries or install semantics.

## Build and CI

The [build skill](../../.claude/skills/build-engine/SKILL.md) owns the local
build loop. Read the affected workflow and source action for CI changes.

- `build/args/shot.gn` holds shared defaults; Linux/macOS overlays carry
  platform-specific flags. Do not copy an overlay's flags into all platforms.
  Linux's `use_allocator_shim = false` avoids interposing the engine's allocator
  over host libraries.
- CHECK diagnostics can be a bare crash in the optimised configuration.
  Inspect the native stack when the normal build/error summary has no useful
  message; `CHECK(...) << message` is not a reliable logging channel here.
- Jumbo units group source files by directory before splitting them. A change
  in one platform's source list can expose anonymous-namespace, macro or include
  ordering collisions on that platform alone. Verify exclusions in freshly
  generated units on the owning target. Old generated units can remain on disk.
- Set `CHROMIUM_WIN_SDK_VERSION` and `win_ntddi_version` consistently. The
  Windows workflow computes them together; pinning only the SDK directory can
  silently disable declarations guarded by `NTDDI_VERSION`.
- ICU, Skia and Perfetto are tracked source slices; their `README.shotium.md`
  files describe the provenance. ICU data is generated from the tracked cast
  data. Do not reintroduce a separate checkout or patch replay for these trees.
- Preserve the `LASTCHANGE` pin and content-faithful mtimes when changing warm
  builds. Otherwise a source-identical checkout can rebuild many dependencies.
  Relevant code is in DEPS, `scripts/ci/stamp-mtimes.ts` and
  `scripts/build/icu-repack.ts`.
- Sharded builds merge outputs, Ninja logs and generated inputs. Read
  `scripts/build/shards.ts`, `scripts/lib/ninja-state.ts` and
  `scripts/ci/await-shards.ts` before changing that protocol. The final job also
  compiles a shard; assuming it only links changes the scheduling semantics.
- Cache visibility depends on the ref that created it. Diagnose misses from
  the actual run/cache keys; do not infer a compiler regression from an old
  warm-versus-cold timing. Keep cache population within the requested CI work.
- [checks.yml](../../.github/workflows/checks.yml) runs package/harness/tooling
  checks without an engine. Its filters include `apps/**`,
  `shot/testdata/bilibili/**`, `scripts/**` and its own workflow path.
  It runs `verify:daemon-protocol` and `verify:bilibili --fixtures-only`.
- `engine-*.yml` build native artifacts and run engine checks according to
  their conditions; `check-ffi.yml` validates native delivery. Inspect the
  architecture, mode, `run_checks`, step outcomes and artifact SHA before
  calling a platform verified. A probe or skipped step is not a runtime pass.
- Read failures with `gh run view -R sj817/shotium <id> --log-failed`.
  Publishing is covered by the [release skill](../../.claude/skills/release/SKILL.md).

## Tests and measurements

- A rendering feature normally gets `shot/testdata/demos/NAME.html` and
  `NAME-ref.html`. Express expected pixels through a path unaffected by the
  feature, such as positioned coloured blocks or equivalent text layout.
- If an exact reference is not possible, declare a justified WPT-style `fuzzy`
  tolerance or use a smoke fixture and report that weaker evidence explicitly.
  Smoke success does not establish pixel equivalence.
- Protocol, resource, cache, budget and lifecycle behaviour belongs in the
  relevant `scripts/verify/*.ts` suite. Keep fixtures offline; the Bilibili
  fixture-only check establishes completeness/locality, not rendering.
- `apps/test/render/baselines/` is gitignored. Generate baselines from a
  pre-change binary. Never accept the candidate's own images as evidence that
  it preserved behaviour; without a prior baseline, use reftests and say so.
- `pnpm accept --skip-build` compares the corpus to the existing Chrome oracle
  by region. Check the oracle's provenance when interpreting differences;
  omitting `--skip-build` starts a build.
- `apps/demo-card/card.html` is the source for the five language demo copies.
  Change that source and run `pnpm demo:sync`; `--check` verifies the copies.
  C ABI and packaging changes also need `verify:ffi`/`verify:delivery` as
  described in the verification skill.
- Use [performance.md](performance.md) and the performance skill for A/B work.
  Keep engine comparisons on local fixtures and control cache configuration.
  A remote URL includes network/cache costs that can dominate rendering.
- Report peak private memory separately from working set. The process sampler
  itself costs CPU; do not run builds or another benchmark beside a measurement.
  Use `pnpm size:report` for linked binary contribution, not object-file size.
- Competitor failures, noisy samples and harness/shotium failures have different
  meanings in `apps/benchmark`. Read its README and recorded engine list before
  comparing platforms or publishing a result.

## Sources of changing facts

| Fact | Check here |
|---|---|
| Public version and platform pins | `apps/typescript/package.json` |
| Commands, dependencies and install boundaries | Root and project `package.json` files; current workflow steps |
| Upstream baseline and replay decisions | [upstream-sync.md](upstream-sync.md) and its linked state files |
| Current benchmark evidence | [benchmarks/LATEST.md](benchmarks/LATEST.md) and the identified run directory |
| Build parallelism and sharding defaults | `scripts/build/engine.ts`, `scripts/ci/select-shards.ts`, workflow inputs |
| Runtime tuning variables | Actual read sites in `shot/`, especially `shot_renderer.cc`, `shot_fetch.cc`, `shot_image_stream.cc` |
| SDK paths and machine capacity | Current host inspection and gitignored `CLAUDE.local.md` |

Design records explain past decisions; source, manifests and measured artifacts
establish current behaviour. Keep unsupported historical numbers out of the
main agent instructions.
