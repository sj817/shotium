---
name: build-engine
description: Build the shotium engine on the Windows development host (targets shot and shot_c into out/Shot), read the build log by error class, and iterate with the syntax-only checker instead of full rebuilds. Use for "build the engine", "the build failed", "does this compile", or after any change under shot/, third_party/blink, cc/, skia/ or the GN files. Does not cover Linux or macOS builds; those are dispatched through engine-linux.yml and engine-macos.yml.
---

# Build the engine

The engine is built by one script into one directory. Everything else about
building here is about not being fooled by a stale binary, a leftover
process, or a jumbo grouping that only fails on another platform.

## Before starting

1. **Kill leftover build processes, twice.** Stopping a background task ends
   the shell, not `ninja` or the `clang-cl` tree it spawned. Three ninjas
   writing one output directory once put 112 compilers on the machine and
   froze it.

   ```powershell
   Get-Process -Name ninja,clang-cl,rustc,lld-link -EA SilentlyContinue | Stop-Process -Force
   Get-Process -Name ninja,clang-cl,rustc,lld-link -EA SilentlyContinue | Stop-Process -Force
   ```

   The second pass catches compilers that were still starting when the first
   ran.

2. **The output directory is `out/Shot`.** The script hard-codes it. The
   twenty-odd sibling directories under `out/` are experiments from earlier
   phases; their binaries are old (some still carry the pre-rename `shot.exe`)
   and a check run against one proves nothing.

3. **Check for a lock or another build.** `git --no-optional-locks status --short`
   shows whether someone else is mid-edit in the shared tree; a running
   `ninja` from another session shows up in step 1.

## Build

```bash
pnpm -C tools/shot install                                              # once per checkout
pnpm build:engine --jobs 16 --log out/Shot/build.log                    # shotium.exe + .pak files
pnpm build:engine --target shot_c --jobs 16 --log out/Shot/build.log    # shotium.dll, which the Node addon links
```

The entry point is `tools/shot/build_engine.ts` (TypeScript, `execa`,
`p-retry`, `cac`); `pnpm build:engine` forwards to it from the repository
root, and a relative `--log` is relative to where you typed the command.
Run it in the background and read the log. Expectations:

| Situation | Duration |
|---|---|
| First build into `out/Shot` (~5,800 steps) | ~50 minutes at `-Jobs 16` |
| Incremental, only `shot/` touched | a few minutes, mostly link |
| Incremental touching Blink core headers | tens of minutes (jumbo TUs recompile) |

What the script does that you must not duplicate by hand:

- Runs `gn gen out/Shot` first, so ninja rarely triggers its own regen.
- Retries `PermissionError: [Errno 13] Permission denied: 'environment.x64'`.
  That is several toolchain variants racing to write the same file, not a
  configuration error; the retry always wins.
- Retries the ninja invocation once for the case where ninja regenerated
  anyway and hit the same race.
- Regenerates `third_party/icu/shot/icudtl.dat` from the cast data set
  (`tools/shot/icu_repack.py`), because `third_party/icu` is a DEPS checkout
  that gclient can reset.

The log's first `ninja: Entering directory` line must say `out/Shot`.

### Parallelism

- `--jobs` defaults to 12 in the script. 16 is the last value that completed a
  full build on this host. 24 hit `LLVM ERROR: out of memory` in the
  `blink/renderer/core` jumbo TUs (template-heavy `Vector<>` instantiations),
  and doing so while a `gn gen` ran in parallel made it worse.
- Measure the phase you are about to run, not one you measured earlier:
  `Get-Process clang-cl | Measure-Object WorkingSet64 -Sum`, then choose `j`
  so that peak-per-compiler x `j` stays under half of free memory. ninja is
  incremental; stopping to change `--jobs` loses nothing already compiled.
- ThinLTO link memory is governed by `/opt:lldltojobs=N` in the linker
  flags, not by `-Jobs`.

## Reading failures

Never read the raw log; every `FAILED:` is followed by ~6 KB of flags.

```powershell
python tools/shot/errors.py out/Shot/build.log              # errors grouped by target
python tools/shot/errors.py out/Shot/build.log --top 30     # most frequent diagnostics first
python tools/shot/errors.py out/Shot/build.log --full       # every message
python tools/shot/build_errors.py out/Shot/build.log --files  # grouped by normalised diagnostic text, with file lists
```

Recognise these before debugging:

| Symptom | Meaning |
|---|---|
| `Permission denied: 'environment.x64'` | Toolchain race; the script retries. Only a problem if it persists after the retry |
| Same error in a file you did not touch, only on one platform | Jumbo grouping changed (see below) |
| `missing and no known rule to make it` | A deleted file is still an input in the graph; run `python tools/shot/missing_inputs.py out/Shot` |
| `unknown type name 'FILE_INFO_BY_HANDLE_CLASS'` and friends | `win_ntddi_version` does not match the SDK; set both `CHROMIUM_WIN_SDK_VERSION` and `win_ntddi_version` |
| Link: `undefined symbol` for a Rust `hb_*` or `cxxbridge` symbol | A crate feature or DEPS checkout was removed; see `/cut-component` |
| Link: `FreeInUnknownRoot` at runtime on Linux | Two allocators in one process; the `.so` must be built with `use_allocator_shim = false` (`build/args/shot-linux.gn`) |

## The fast loop

Front-end errors (missing declaration, dangling include, member that no
longer exists) are decided before the optimiser runs. Do not pay for a full
compile to find them.

```powershell
python tools/shot/check.py path/to/foo.cc other.cc            # -fsyntax-only from the compdb; ~8 s per core/ TU
python tools/shot/check.py --from-log out/Shot/build.log      # only the TUs that failed last time
python tools/shot/check.py --dir third_party/blink/renderer/core/frame
```

Cadence: one full build to collect the complete failure set, then
`check.py --from-log` until it is empty, then build again.

### Jumbo

`use_shot_jumbo_build` merges sources into one TU per group of
`shot_jumbo_file_merge_limit` (8) files. The grouping is decided by the
target's source list, so two files that collide only collide when they land
in the same chunk, and the chunk boundaries move whenever a platform-specific
file enters or leaves the list. "It compiles on Windows" is not evidence
about Linux.

Known failure modes, all seen in this tree:

1. **A header ends with `#undef X`** (self-cleaning in a single TU). In a
   jumbo TU every later file loses the macro and the include guard stops the
   header from redefining it. Exclude the file that does the `#undef`, not the
   victim.
2. **An unqualified name resolves differently** after an earlier file made
   another declaration visible (`std::unique_ptr<VirtualAddressSpace>` picking
   `v8::base::` instead of the base class's injected name).
3. **Anonymous-namespace or macro collisions** between files.

Excluding a file: add it to `shot_jumbo_excluded_sources` on the *target that
owns it*. Two silent failures: writing it on the wrong target (the obj path
`obj/.../core/animation/animation/x.obj` names `core/animation:animation`,
not `core:core`), and a path form that does not match the target's `sources`
(some targets use `rebase_path` and carry absolute paths). Verify by reading
`out/Shot/gen/<path>/<target>_shot_jumbo_N.cc` after `gn gen`, checking the
timestamp so you are not reading a chunk from an earlier merge limit. Both
path forms are tried in `build/config/BUILDCONFIG.gn`; if you change that
matcher, regress both Blink core and Skia.

## Other platforms

- **Linux GN configuration** reproduces locally in ~30 seconds, with errors
  identical to CI's. Use a separate out directory with:

  ```gn
  import("//build/args/shot-linux.gn")
  use_sysroot = false                                   # sysroot.gni asserts otherwise
  host_toolchain = "//build/toolchain/linux:clang_x64"  # or build/toolchain/win loads and asserts is_win
  ```

  `shot-linux.gn` imports `shot.gn` and sets `target_os`, Ozone headless and
  the allocator-shim setting; `tools/shot/probe_platform_graph.py` writes
  this same args file and additionally stubs every missing directory so one
  run lists every gap.

  When the only remaining complaints are `cxxbridge.exe` /
  `*_build_script.exe` "Input to targets not generated by a dependency", the
  graph is done; that suffix comes from the Windows host and is not a target
  problem. Run `missing_inputs.py` against that directory too; one out
  directory answers for one platform.
- **macOS** cannot be cross-generated (`BUILDCONFIG.gn` asserts the host is
  mac or linux). Batch several fixes before dispatching `engine-macos.yml`,
  and use `mode=probe` first: it reports graph errors in ~15 minutes without
  a build.
- **CI runners** build with `-j 4` and about 1.2 GB per Blink layout TU; the
  Windows job needs `CHROMIUM_WIN_SDK_VERSION` to follow the runner image.

## After it builds

A binary is level 2 of 3. Record the timestamp and size of
`out/Shot/shotium.exe` (and `shotium.dll` if `shot_c` was built), then run
`/verify-engine`. Do not report success before the checks pass.
