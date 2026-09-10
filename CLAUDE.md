# shotium Development Guide

shotium renders static HTML/CSS to PNG, JPEG and WebP using a slice of Chromium:
Blink for DOM/layout/paint, Skia for rasterisation, and `//net` for resources.
It ships a CLI, a C ABI and the `@shotkit/shotium` Node package.

This is the shared entry point for coding agents; `AGENTS.md` points here.
Read the task-specific references below when needed. Commands, source paths
and workflow behaviour must agree with the current checkout.

## Working in this tree

- Keep changes within the requested task and follow the surrounding code.
  Preserve unrelated edits, including files already staged by someone else.
- Use `git --no-optional-locks` for read-only Git commands. Before editing,
  inspect the relevant diff; before committing, inspect both staged and
  unstaged changes with `git --no-optional-locks status --short`.
- Commit only when asked. Stage explicit paths with `git add <path>`; never
  use `git add -A`, `git add -u` or `git add .`. An existing staged file
  is not automatically part of your commit.
- A lock file alone does not prove a Git process is abandoned. Establish
  ownership and the absence of an active writer before removing a stale lock.
  Identify build processes by PID, command line and output directory; do not
  kill every compiler or ninja process by name.
- Push to `origin` (`sj817/shotium`); pass `-R sj817/shotium` to `gh`.
  Inspect the current branch and tracking configuration before a push.
  Never merge `upstream/main`, push the old `codex/shot-engine-round1`
  history, or rewrite published `main`. Upstream changes are replayed via
  the [sync procedure](apps/docs/upstream-sync.md).
- Commit messages use English Conventional Commits. Usual scopes are
  `shot`, `shotium`, `blink`, `bench`, `ci`, `docs`. Explain the
  mechanism and relevant verification in the body.

## Product and architecture constraints

- Keep V8, `//content`, the compositor, the GPU process and DevTools out.
  Page JavaScript is not executed; inputs must already contain the HTML to
  render. Retained files under a directory named `v8` can still implement
  required Blink types or GC support; names are not dependency evidence.
- Blink initialisation is process-wide and cannot be restarted. Keep Blink
  work serialised through the engine service. Parallel rendering uses
  separate processes; destroying a handle does not reset Blink.
- Preserve font rasterisation settings in `shot/shot_runtime.cc`: grayscale
  antialiasing, fixed gamma, `kUnknown_SkPixelGeometry`, isolated DirectWrite
  on Windows. Repeated renders in the same environment must be deterministic.
  Host fonts can differ; this is not a promise of identical pixels across OSes.
- The C ABI in `shot/shot_api.h` has no Chromium headers or C++ ownership
  crossing the boundary. Free returned buffers through the matching API.
- The published TypeScript package has zero runtime dependencies. Importing
  it must not start the engine, and invalid options must reject before native
  initialisation. Keep option names and defaults consistent across the TS,
  native and JSON interfaces.
- Chromium, ICU, Skia and Perfetto changes are maintained in their source
  files. For dependencies still fetched by DEPS, remove the DEPS and
  `.gitmodules` entries with an authorised removal of that checkout.
- A search with no matches does not prove code is unused. Use scoped `rg`
  for navigation; use `git --no-optional-locks grep`, GN dependency paths
  and the supported platforms' build inputs for removal decisions.
  Follow the cut skill before deleting a component.

## Repository map

| Area | Purpose |
|---|---|
| `shot/` | Runtime, renderer, resource loading, worker protocol, C ABI and GN targets |
| `apps/typescript/` | Published Node API; `src/` for TS, `native/` for the GN-built addon |
| `scripts/` | Repository tooling, grouped by command; aliases in root `package.json` |
| `build/args/shot*.gn` | Shared engine arguments and platform overlays |
| `.github/workflows/` | Package checks, engine builds, delivery, benchmarks and publishing |
| `shot/testdata/`, `apps/test/render/` | Offline fixtures, reftests and pixel regression |
| `apps/c-abi/`, `apps/{go,python,rust,csharp,java}/` | C ABI documentation and language demos |
| `apps/demo-card/` | Source card shared by demos and README assets |
| `apps/benchmark/`, `apps/benchmark-site/` | Measurement harness and published reports |
| `apps/docs/` | Design history, engineering references and benchmark evidence |

For the request-to-image path and non-obvious runtime invariants, read
[engine contracts](apps/docs/agent-reference.md#engine-contracts).

## Read for the task

Skills are maintained in `.claude/skills/`. Read the matching `SKILL.md`
before following its procedure; do not load every skill for every task.

| Task | Entry |
|---|---|
| Build or troubleshoot the Windows engine, change C++ or GN | [build-engine](.claude/skills/build-engine/SKILL.md) |
| Verify rendering, native API or daemon behaviour | [verify-engine](.claude/skills/verify-engine/SKILL.md) |
| Remove, trim or restore a component | [cut-component](.claude/skills/cut-component/SKILL.md) |
| Compare performance or memory | [perf-compare](.claude/skills/perf-compare/SKILL.md) |
| Publish an explicitly requested release | [release](.claude/skills/release/SKILL.md), invoked as `/release <version>` |
| Change public options, ABI or protocol | [API changes](apps/docs/agent-reference.md#api-changes) |
| Add or change repository tooling | [tooling conventions](apps/docs/agent-reference.md#tooling-conventions) |
| Change build infrastructure or CI | [build and CI](apps/docs/agent-reference.md#build-and-ci) and the affected workflow |
| Sync upstream Chromium | [sync procedure](apps/docs/upstream-sync.md) and [sync tool](apps/docs/upstream-sync-tool.md) |

Agents that discover skills through `.agents/skills/` use links to these
canonical files. `pnpm skills:link` creates or repairs those links; run it
when setting up a checkout or changing skill names. Edit the canonical files.

## Building

Run these commands from the repository root. The root package declares the
tooling Node/pnpm versions; `scripts/`, `apps/typescript/` and
`apps/benchmark/` are independently installed projects, not a root workspace.

```sh
pnpm -C scripts install --frozen-lockfile
pnpm build:engine --log out/Shot/build.log
pnpm build:engine --target shot_c --log out/Shot/c-build.log
pnpm build:engine --target shot_node --log out/Shot/node-build.log
```

Use `pnpm build:engine` for local engine generation and builds into
`out/Shot`; the script handles ICU data, Node SDK preparation and toolchain
retries. Do not replace it with hand-written `gn gen` or `autoninja`
commands. The Windows targets produce `shotium.exe`, `shotium.dll` and
`shotium.node`, with resource packs alongside them.

Use the script's default parallelism until the current machine's memory
supports a change. Relative `--log` paths resolve from the repository root.
Use `pnpm build:errors <log>` to inspect failures and
`pnpm check:syntax <file.cc>` for the fast syntax loop.
`--gen-only` and separate platform probes provide graph diagnostics only.
Linux and macOS engine builds use the corresponding CI workflows.

## Verification

Choose checks for the changed behaviour and report exactly what ran.
Documentation-only changes need reference/command validation and a diff
check; they do not require an engine rebuild.

| Change | Verification entry |
|---|---|
| Repository scripts | `pnpm scripts:check`, `pnpm scripts:test`, plus the affected command's behaviour |
| TypeScript package | `pnpm -C apps/typescript build`, `pnpm -C apps/typescript check:types`; native/API changes also need the relevant engine checks |
| Benchmark harness | `pnpm -C apps/benchmark check`; use its README for actual measurements |
| Engine, native binding, ABI or rendering | Build affected targets, then follow `verify-engine` |
| File or dependency removal | Cut skill, including `pnpm missing-inputs out/Shot` and platform checks |
| Shared demo card | `pnpm demo:sync`, then `pnpm demo:sync --check` |

Report engine evidence on separate levels:

1. **Graph passes**: generation/dry-run checks only; no compilation proved.
2. **Compiles**: the affected targets built successfully from the intended source.
3. **Binary and checks pass**: named checks passed against those built artifacts.

A binary's existence or timestamp alone is insufficient provenance.
`checks.yml` covers checks that need no engine, including daemon protocol
negotiation; it does not establish rendering or native-addon correctness.
Its path filters are in the workflow. A missing run is not a pass, and a
successful probe is not a successful build. Report skipped checks and reasons.

For a release, the version commit, all six platform builds and the tag must
identify the same commit. A check on another SHA or platform does not cover it.
Only run the publishing procedure when a release is requested.

## Code and documentation

- Follow Chromium style in C++; preserve buffer-safety and lock annotations.
  Comments explain non-obvious reasons. Keep generated IDL output tied to
  `pnpm gen:idl` and validate the generator when its output changes.
- New repository tooling belongs in `scripts/<group>/<kebab-case>.ts`,
  uses established libraries, and has a root `pnpm` alias. Keep the package's
  zero-dependency rule separate from tooling, which uses libraries.
- Update public API documentation in both English and Chinese with behaviour
  changes. The C ABI guides ship in release archives. Change the shared card
  in `apps/demo-card/card.html` and sync its copies.
- Use [the engineering reference](apps/docs/agent-reference.md) for test
  conventions and build caveats, and [the docs index](apps/docs/README.md)
  for design history. Historical plans and `shot/README.md` can predate the
  current architecture; verify executable facts against source and manifests.
- Keep measured results in the benchmark archive with their provenance.
  Put machine-specific paths, SDK versions and local preferences in
  `CLAUDE.local.md` (gitignored). Keep this entry point concise and remove
  obsolete or conflicting instructions when updating it.
