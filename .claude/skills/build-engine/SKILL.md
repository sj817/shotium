---
name: build-engine
description: >-
  Build or troubleshoot the Windows shotium engine in out/Shot. Use for
  compilation requests and C++ or GN changes affecting the engine. Covers
  log diagnosis and the syntax-check loop; Linux and macOS builds use CI.
---

# Build the engine

The engine is built by one script into one directory. `shot_node` prepares a pinned, checksum-verified Node SDK and uses Node-API 8. Everything else about
building here is about not being fooled by a stale binary, a leftover
process, or a jumbo grouping that only fails on another platform.

## Before starting

1. **Check who owns an existing build.** Inspect process IDs, parent processes,
   command lines and logs for `out/Shot`. Do not start a second writer there.
   If another session is building, coordinate with it or wait. Stopping a
   background shell may leave its children running; after cancelling your own
   build, recheck that specific process tree. Stop only processes owned by this
   task or confirmed abandoned within the user's authorised scope, never all
   `ninja`, compiler or linker processes by name.

2. **The output directory is `out/Shot`.** The script hard-codes it. Binaries
   in experimental sibling directories are not substitutes for the intended
   build's artifacts.

3. **Inspect the working tree.** Use `git --no-optional-locks status --short`
   and the relevant diffs to preserve concurrent edits. Git status does not
   identify a running build, and an index lock alone does not prove that a Git
   writer is abandoned.

## Build

```bash
pnpm -C scripts install --frozen-lockfile                          # once per checkout
pnpm build:engine --log out/Shot/build.log                          # shotium.exe + .pak files
pnpm build:engine --target shot_c --log out/Shot/c-build.log         # independent C ABI library
pnpm build:engine --target shot_node --log out/Shot/node-build.log    # Node addon
```

The entry point is `scripts/build/engine.ts` (TypeScript, `execa`,
`p-retry`, `cac`); `pnpm build:engine` forwards to it from the repository
root, and a relative `--log` is resolved against the repository root -- the
forward runs a second pnpm, which overwrites `INIT_CWD`, so the directory you
typed the command in is not recoverable.
Run it in the background and read the log. Use the current run's commands,
exit status and artifacts as evidence. Historical build counts and timings
depend on the source, cache, target and host; do not treat them as estimates
for an unmeasured checkout.

What the script does that you must not duplicate by hand:

- Runs `gn gen out/Shot` first, so ninja rarely triggers its own regen.
- Retries `PermissionError: [Errno 13] Permission denied: 'environment.x64'`.
  Concurrent toolchain variants can race to write that file. If retries still
  fail, inspect the final error rather than assuming the build recovered.
- Retries the ninja invocation once for the case where ninja regenerated
  anyway and hit the same race.
- Regenerates `third_party/icu/shot/icudtl.dat` from the tracked cast data set
  (`pnpm icu:repack`), preserving an unchanged output. ICU is maintained
  directly in this repository, not as a separate DEPS checkout.

The log's first `ninja: Entering directory` line must say `out/Shot`.

### Parallelism

- Start with the script's default `--jobs` value. Blink core jumbo units can
  exhaust memory at a parallelism that works for smaller sources; measure the
  current host before increasing it. Avoid overlapping generation or builds
  that compete for the same memory or output directory.
- Measure the phase you are about to run, not one you measured earlier:
  `Get-Process clang-cl | Measure-Object WorkingSet64 -Sum`, then choose `j`
  so that peak-per-compiler x `j` stays under half of free memory. ninja is
  incremental; stopping to change `--jobs` loses nothing already compiled.
- ThinLTO link memory is governed by `/opt:lldltojobs=N` in the linker
  flags, not by `--jobs`.

## Reading failures

Never read the raw log; every `FAILED:` is followed by ~6 KB of flags.

```powershell
pnpm build:errors out/Shot/build.log --targets   # errors grouped by target
pnpm build:errors out/Shot/build.log --top 30    # most frequent diagnostics first
pnpm build:errors out/Shot/build.log --full      # every message
pnpm build:errors out/Shot/build.log --files     # grouped by normalised diagnostic text, with file lists
```

Recognise these before debugging:

| Symptom | Meaning |
|---|---|
| `Permission denied: 'environment.x64'` | Toolchain race; the script retries. Only a problem if it persists after the retry |
| Same error in a file you did not touch, only on one platform | Jumbo grouping changed (see below) |
| `missing and no known rule to make it` | A deleted file is still an input in the graph; run `pnpm missing-inputs out/Shot` |
| `unknown type name 'FILE_INFO_BY_HANDLE_CLASS'` and friends | `win_ntddi_version` does not match the SDK; set both `CHROMIUM_WIN_SDK_VERSION` and `win_ntddi_version` |
| Link: `undefined symbol` for a Rust `hb_*` or `cxxbridge` symbol | A crate feature or DEPS checkout was removed; see `/cut-component` |
| Link: `FreeInUnknownRoot` at runtime on Linux | Two allocators in one process; the `.so` must be built with `use_allocator_shim = false` (`build/args/shot-linux.gn`) |

## The fast loop

Front-end errors (missing declaration, dangling include, member that no
longer exists) are decided before the optimiser runs. Do not pay for a full
compile to find them.

```powershell
pnpm check:syntax path/to/foo.cc other.cc            # -fsyntax-only from the compdb; ~8 s per core/ TU
pnpm check:syntax --from-log out/Shot/build.log      # only the TUs that failed last time
pnpm check:syntax --dir third_party/blink/renderer/core/frame
```

Cadence: one full build to collect the complete failure set, then
`pnpm check:syntax --from-log` until it is empty, then build again.

### Jumbo

`use_shot_jumbo_build` merges sources into one TU per unit. A unit holds
sources from one group, the first directory component of the path relative
to the target (`css`, `layout`, `root` for the target's own directory,
`extern` for paths outside it), split every `shot_jumbo_file_merge_limit`
(32; 16 on macOS) files, and is named `<target>_shot_jumbo_<group>_<n>.cc`.
Two files collide only when they land in the same unit, and a platform's
source list decides that: "it compiles on Windows" is not evidence about
Linux. The limit is 32 because parsing Blink's headers costs a unit about
14 s whatever it contains (8 files 13.9 s, 32 files 16.4 s, 64 files 21.1 s
on the development host; 1.8, 2.2 and 2.5 GB peak).

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
owns it*, with the reason. If the target is in a DEPS checkout or reached
through a template that forwards nothing (`mojom()` traits sources), use the
absolute label in `shot_jumbo_excluded_files` in `build/args/shot.gn`. Two silent failures: writing it on the wrong target (the obj path
`obj/.../core/animation/animation/x.obj` names `core/animation:animation`,
not `core:core`), and a path form that does not match the target's `sources`
(some targets use `rebase_path` and carry absolute paths). Verify by reading
`out/Shot/gen/<path>/<target>_shot_jumbo_<group>_<n>.cc` after `gn gen`,
checking the timestamp so you are not reading a unit from an earlier merge
limit (GN never deletes a stale unit). Both
path forms are tried in `build/config/BUILDCONFIG.gn`; if you change that
matcher, regress both Blink core and Skia.

## Other platforms

- **Linux GN configuration** can expose platform-specific graph problems
  locally. Use a separate out directory with:

  ```gn
  import("//build/args/shot-linux.gn")
  use_sysroot = false                                   # sysroot.gni asserts otherwise
  host_toolchain = "//build/toolchain/linux:clang_x64"  # or build/toolchain/win loads and asserts is_win
  ```

  `shot-linux.gn` imports `shot.gn` and sets `target_os`, Ozone headless and
  the allocator-shim setting; `scripts/tree/probe-platform-graph.ts` writes
  this same args file and additionally stubs every missing directory so one
  run lists every gap.

  Complaints involving host `cxxbridge.exe` / `*_build_script.exe` can reflect
  cross-host probe limitations. Report them as unresolved diagnostics rather
  than calling a failed probe green. A graph produced with stubs is diagnostic
  evidence only; the real platform build still has to pass. Run
  `pnpm missing-inputs` against the probe directory too.
- **macOS** cannot be cross-generated (`BUILDCONFIG.gn` asserts the host is
  mac or linux). Batch several fixes before dispatching `engine-macos.yml`,
  and use `mode=probe` first to check the graph without a build.
- **CI runners** use the jobs and sharding inputs in the current workflows.
  Read those values and the current runner capacity before tuning parallelism.
  The Windows job needs `CHROMIUM_WIN_SDK_VERSION` to follow the runner image.

## After it builds

A successful build of the affected targets is level 2 of 3. Record the source
state, successful build result and artifact identities for `shotium.exe`,
`shotium.dll` and/or `shotium.node`, according to the targets built. A timestamp
alone is not proof that an artifact contains the change. Follow `/verify-engine`
and report the checks that actually passed against those artifacts.
