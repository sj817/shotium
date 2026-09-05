---
name: cut-component
description: Remove a component, directory, third-party library, Rust crate or GN target from this Chromium slice and prove the removal is complete: find live callers (git grep plus gn path, never grep counts), decide restore-vs-cut by actual dependencies, delete the DEPS entry with the checkout, then gn gen, missing_inputs.py, check.py, jumbo verification, and the engine checks. Also for the reverse question, "should X be restored or cut". Triggers: cut, remove, delete, trim, "is this still used", shrink the binary, restore X.
---

# Cut a component

This is a one-time slice of Chromium, not a branch that will rebase. Anything
without a live caller today is deleted outright, without preserving upstream
shape and without asking first. The only question is "does anything use it
now", never "does upstream have it".

## 1. Find the callers

1. `git grep`, in both the working tree and `HEAD`:

   ```bash
   git --no-optional-locks grep -n -- '<symbol or path>' -- '*.gn' '*.gni' '*.cc' '*.h' '*.mm' '*.mojom' '*.idl' 'DEPS'
   git --no-optional-locks grep -n -- '<symbol or path>' HEAD -- '*.gn' '*.gni' '*.cc' '*.h'
   ```

   Whole-tree ripgrep on this checkout times out and prints the partial
   result as if it were complete; a third reference in `mojo/core/test/BUILD.gn`
   was missed that way and cost a CI round.

2. `gn path` for anything that crosses a component boundary:

   ```bash
   gn path out/Shot //shot:shot_core //third_party/zstd   # is there a path at all, and through what
   gn refs out/Shot //path/to/file.cc --all              # which targets own the file
   ```

   grep finding nothing means grep found nothing. `ExtendedErrorToString()`
   grepped as declaration-plus-definition only and was in use from
   `blink/renderer/platform/loader/fetch/resource_error.cc`.
   `disable_zstd_filter` looked like 0.48 MB saved from `//net` and saved
   nothing because Blink platform depends on `//third_party/zstd` directly.

3. Rust crates: the `features` list in the `cargo_crate()` target is the list
   of separate uses. `harfbuzz_rust` had `features = ["font", "shape"]`; only
   the `shape` half was grepped for, the whole crate was removed, and the
   link failed on `hb_fontations_font_set_funcs` from the `font` half, called
   from Blink's `web_font_typeface_factory.cc`. Read `lib.rs` for each
   `#[cfg(feature = ...)] mod`.

4. Non-code references count too: `.grd` files pull every `.xtb` they name
   (`ui/strings/translations` was deleted on that assumption and the build
   found it); GN `inputs` lists (`.rustfmt.toml` sat in
   `cpp_api_from_rust.gni` for a week after deletion); `nogncheck` comments;
   headers that `#undef` a macro for later files in a jumbo chunk.

The compiler and linker are the final judge. Everything before them is a
hypothesis.

## 2. Restore or cut

When a file references something already removed, the first question is not
"do we want this feature" but "what exactly does it depend on". Look at the
field types and function signatures, not the name, the directory, or the
number of grep hits:

| What it depends on | Do this |
|---|---|
| It merely *lives* in a removed directory | Restore it unchanged (`tools/shot/restore_from_upstream.py`) |
| A generated class whose input (`.idl`) still exists | Regenerate (`tools/shot/gen_idl_*.py`) |
| A runtime service of the removed component that has an equivalent | Replace the implementation; comment the difference |
| The removed component's own data structures | Cut; there is nothing to restore |
| An entry point only script could reach | Cut the entry point, keep the mechanism |

Cases that were called wrong by name and cost a build each:

- `ActiveScriptWrappable` has "Script" in its name and lived in
  `bindings/core/v8/`; it has zero V8 references, is plain cppgc, and 37 core
  classes inherit it. Restored.
- `core/sanitizer/` sits inside the HTML parser; `sanitizer.h` has no `v8::`
  at all, only `ExceptionState` (kept) and `V8Union*` (regenerable).
  Restored.
- `web_sandbox_support.h` on Linux/mac is a pure interface; every upstream
  caller tests the pointer first and has a fallback. The right fix was to
  restore the header (zero bytes, dead branches folded by WPO), not to
  rewrite four upstream `.cc` files, which also broke render determinism by
  querying host fontconfig.

Rules:

- "It's telemetry" does not mean the block can go: use counters, the
  first-contentful-paint callback (the screenshot waits on it) and histogram
  bucket variables live in the same blocks.
- Never neutralise a check with `&& false`. It leaves dead code that
  `-Wunreachable-code-return` rejects and misleads the next reader. Delete
  the block and write why.
- Restoring an `#include` line is not restoring a component:
  `restore_includes.py` only re-adds includes whose header exists on disk
  today.
- The full decision record is `docs/cut-progress.md` section 11 (and 8.8
  for the V8-removal round).

## 3. Delete

- Remove the directory, its targets in `BUILD.gn`, and its entries in any
  `.gni` source list together, by hand. The batch editors of the first
  cutting rounds (`gn_drop_*.py`, `cpp_drop_*.py`, `strip_component.py`,
  `find_*.py`) were deleted once those rounds were over; `git log -- tools/shot`
  has them if a whole subtree ever needs the same treatment again.
- For files no build reads at all, `pnpm trim-tree plan` (with the six
  platforms' graph exports) is the authority; see `scripts/trim-tree.ts`.
- **DEPS-fetched directories (most of `third_party/`) need their DEPS entry
  removed in the same change**, or CI's `gclient sync` restores the
  directory. `.gitmodules` is a separate file that git reads and gclient
  ignores; keep the two consistent, and delete both entries.
  `tools/shot/probe_platform_graph.py` reports "in `.gitmodules`, not in
  DEPS" as a finding.
- Regenerating IDL enums or unions: use the generator's `--check` mode
  (`gen_idl_enums.py`, `gen_idl_unions.py`), which collects identifiers from
  call sites with `git grep -ohE` and diffs them against the generated set in
  both directions. Inferred naming rules produced `kRGBAFloat16` where call
  sites said `kRgbaFloat16`; the compiler then reported hundreds of unrelated
  files.
- Cuts that change a behaviour rather than delete one (a feature default, an
  added parameter) go into the disagreements table in
  `docs/upstream-sync.md`, so sync replays them by meaning.

## 4. Prove it is gone

In order; no step substitutes for the next:

1. `pnpm build:engine --gen-only` (~25 s). Proves only that the graph parses.
   Not a bare `gn gen`: several Windows toolchain variants race to write
   `environment.x64`, and the `PermissionError` that produces reads exactly
   like a broken `BUILD.gn`. The script retries it.
2. `python tools/shot/missing_inputs.py out/Shot` (~10 s). Walks
   `ninja -t inputs shot shot_c` and stats every file. GN never opens an
   `inputs` entry, `ninja -n` stops at the first missing one, and a warm
   build directory never looks. Skipping this step costs an hour of CI per
   platform on the next cold build.
3. `python tools/shot/check.py --dir <affected dir>` or `--from-log` for the
   syntax-only pass.
4. Full build (`/build-engine`). If a jumbo exclusion was added, read
   `out/Shot/gen/<path>/<target>_shot_jumbo_N.cc` and confirm the file is
   absent from a chunk regenerated in this `gn gen` (check the timestamp;
   older merge limits leave stale chunks in `gen/`).
5. Linux graph in a second out directory (`import("//build/args/shot-linux.gn")`,
   `use_sysroot = false`, `host_toolchain = "//build/toolchain/linux:clang_x64"`),
   ~30 s, then `missing_inputs.py` against it; `tools/shot/probe_platform_graph.py`
   does this and stubs missing directories so one pass lists every gap. One
   out directory answers for one platform. macOS only through
   `engine-macos.yml` in `probe` mode.
6. `/verify-engine`, including the acceptance run
   (`pwsh tools/shot/accept.ps1 -SkipBuild`). No region of the corpus may move
   against the Chrome oracle; a rendering difference from a cut is a bug unless
   it is documented in `docs/cut-progress.md` section 8.6.

Report on the three-level ladder. A green Linux probe is level 1 of 3 and
has been followed by real compile failures (MPRIS includes,
`WebSandboxSupport`) more than once.

## 5. Size

Deleting code is for compiling, not for shrinking the binary. `obj` size is
not image contribution (ThinLTO already dropped the unreachable: 3.4 MB of
Skia Graphite bitcode was 162 KB in the image), and `optimize_for_size` on
Windows was a no-op until `build/config/compiler/BUILD.gn` was fixed to apply
`-Os` inside the `is_win` branch. Measure with `tools/shot/size_report.py`
(`--by-object <name>`) and the method in `docs/cut-progress.md` section 17
before planning a size cut; what remains in the image is mostly statically
reachable, and the large remaining items are data (ICU), not code.
