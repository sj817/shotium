---
name: release
description: >-
  Publish an explicitly requested shotium release to npm and GitHub Releases.
  Align the version commit, six platform builds and tag, verify publication,
  and write bilingual release notes. Invoke manually with /release and a version.
disable-model-invocation: true
argument-hint: "<version>"
arguments: [version]
---

# Release $version

`publish.yml` finds the six engine artifacts by the commit the tag points at
(`gh run list --commit "$GITHUB_SHA"`). That single fact fixes the order of
everything below: the engine builds must run on the version-bump commit,
and the tag must point at that same commit. A commit after the tag, or a
build on an earlier commit, fails the collect step with
`NOT FOUND at this commit`, and a whole batch was redone in August 2026 for
a README change made in between.

## 1. Bump the version

The only source of truth is `apps/typescript/package.json`, and it holds the version
seven times: `version`, plus the six self-referencing pins under
`optionalDependencies`. npm platform tarball names, the release title and
example manifests derive their version from it. Public `.7z` filenames and
top-level directories are stable and do not contain the version.

```bash
git --no-optional-locks grep -n '"<previous version>"' -- apps/typescript/package.json   # exactly 7 lines
```

Edit all seven to `$version`. `checks.yml` fails if the pins and `version`
disagree.

Also note, for a follow-up commit *after* the release:
`.github/workflows/perf-gate.yml` carries the previous version as
the `baseline_version` default, and `pnpm verify:daemon-protocol`
uses a literal version in one fixture.

Commit and push only that file:

```bash
git add apps/typescript/package.json
git commit -m "release: v$version"
git push
SHA=$(git rev-parse HEAD)
# GitHub takes a few seconds to create the run, and `--limit 1` without
# `--commit` returns the *previous* run in that window -- which `gh run watch`
# reports as an instant success, sending you on to the engine builds on the
# strength of an older release. Select by commit, and wait for it to exist.
run_id() { gh run list -R sj817/shotium --workflow "$1" --commit "$2" --json databaseId --jq '.[0].databaseId'; }
until RUN=$(run_id checks.yml "$SHA") && [ -n "$RUN" ]; do sleep 5; done
gh run watch -R sj817/shotium "$RUN"
```

## 2. Six engine builds on this commit

```bash
SHA=$(git rev-parse HEAD)
# The six dispatches, with mode=build where the workflow has that input.
# --dry-run prints them without sending; --wait polls until all six finish.
pnpm ci:dispatch-engines --ref main
gh run list -R sj817/shotium --commit "$SHA"
```

- `shards` defaults to `auto` (Windows 4+4, Linux 4+3, macOS 2+3). That is
  the number of slices, and the final job takes the last one, so the six
  dispatches above are 20 jobs in total -- exactly the free plan's
  concurrency. Dispatch all six at once. `-f shards=1` is the old single job.
- `mode` defaults to `probe` on Linux and macOS. A probe run is
  `gn gen` + `ninja -n`, compiles nothing, and produces no artifact.
- `run_checks` defaults to true; leave it. The run is not green unless the
  check suites passed against the binary it built.
- With unchanged C++ the compile caches hit and all six finish in about
  25 minutes; cold builds take 1 to 4 hours (Windows arm64 and macOS arm64
  are the slow ones).
- Each run uploads two separate archives, `shotium-cli-<platform>.7z` and
  `shotium-c-abi-<platform>.7z`, in the existing `shotium-<platform>` artifact;
  the npm platform tarball remains a separate artifact. The release step is
  not part of these workflows.

Do not tag until all six show `completed success` for `$SHA`, including the
five-language FFI and npm delivery jobs. A run with `run_checks=false` is not
release evidence. Inspect the collected `ffi-evidence-shotium-<platform>`
reports and ensure each names the CLI and C ABI artifacts it exercised.

Before tagging, dispatch `publish.yml` with `dry_run=true` against this same
commit and wait for success. It collects 12 native archives, packages the five
examples and generates/verifies one `SHA256SUMS` before any npm dry run:

```bash
gh workflow run publish.yml -R sj817/shotium --ref main -f dry_run=true
# Select the newly dispatched run at $SHA and inspect its conclusion/artifacts.
```

Check the `release-attachments` artifact: exactly 17 `.7z` files and
`SHA256SUMS`, with the six CLI, six C ABI and five example names from
[the artifact contract](../../../apps/docs/agent-reference.md#release-artifacts).
Rehearsal needs the real six-platform artifacts; synthetic checksum fixtures
or a local Windows package alone do not satisfy this gate.

## 3. Tag

```bash
git tag -a v$version -m "v$version"
git push origin v$version
TAG_SHA=$(git rev-parse "v$version^{commit}")
# Same reason as step 1: never `--limit 1` right after a push.
run_id() { gh run list -R sj817/shotium --workflow "$1" --commit "$2" --json databaseId --jq '.[0].databaseId'; }
until RUN=$(run_id publish.yml "$TAG_SHA") && [ -n "$RUN" ]; do sleep 5; done
gh run watch -R sj817/shotium "$RUN"
```

`publish.yml` publishes the six platform packages first, then
`@shotkit/shotium`, and only then creates a non-draft GitHub release with the
17 `.7z` archives (6 CLI, 6 C ABI, 5 source examples) and one `SHA256SUMS`:
18 uploaded assets, plus two automatic GitHub source archives, for 20 items.
Names are `shotium-cli-<platform>.7z`, `shotium-c-abi-<platform>.7z` and
`shotium-example-<language>.7z`. The checksum file uses filename-sorted SHA256,
two spaces and archive basenames; it excludes itself, npm tarballs and the
automatic source archives. Missing, duplicate or mismatched files stop release.
The `.tgz` files belong to the registry. Never
create a draft release by hand: a draft creates no git tag until it is
undrafted, and its `targetCommitish` is frozen at creation, which is how
v0.1.0 ended up on npm with no tag in git.

`workflow_dispatch` with `dry_run=true` rehearses the whole thing against a
ref and publishes nothing; complete this rehearsal before tagging.

## 4. Verify the registry

```bash
for p in shotium shotium-win32-x64 shotium-win32-arm64 shotium-darwin-x64 shotium-darwin-arm64 shotium-linux-x64 shotium-linux-arm64; do
  printf '%-24s ' "@shotkit/$p"
  curl -s -o /dev/null -w '%{http_code}\n' "https://registry.npmjs.org/@shotkit/$p/$version"
done
```

A 404 is not proof of failure: `@shotkit/shotium-win32-arm64` has become
visible 15 minutes after the other six on two releases. The evidence is the
publish job log: a `Publishing to https://registry.npmjs.org/` line **without**
`(dry-run)`. The `+ @shotkit/...@$version` line is printed by dry runs too and
proves nothing.

Then install from a clean directory and run the README example once. Download
the 18 Release attachments into an empty directory and run
`pnpm package:checksums --dir <download-directory> --check`; inspect the archive
roots and isolated CLI/C ABI contents. The public guides use
`releases/latest/download/<fixed-name>` and show selected-file verification,
so end users do not need all 17 archives.

## 5. Release notes

The release is created with install instructions, archive categories/counts,
checksum guidance and provenance. Add
the changelog by hand:

- Two complete halves: English on top, Chinese below. Each half has its own
  Highlights, Install, Standalone binaries and Provenance; repeating the
  install block is fine. Do not interleave languages line by line.
- Material comes from commit *bodies* between the previous tag and this one
  (`git log v<prev>..v$version`). Engine `perf(...)` commits carry the
  mechanism and before/after numbers; use them. Commits with an empty body
  must be read as diffs; two bench notes were written wrong from titles
  alone.
- Chinese paragraphs must not be soft-wrapped: GitHub renders the line
  breaks as spaces.

## Redoing a release

Cheap when C++ did not change: the six builds hit their caches and finish in
25 minutes. If anything must change after the tag, delete the tag and the
release, fix, and start again from step 1 with the same version if nothing
was published, or the next patch version if any package reached the
registry (npm does not allow republishing a version).
