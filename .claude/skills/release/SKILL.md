---
name: release
description: Publish a shotium release to npm and GitHub Releases in the order publish.yml enforces: bump the seven version lines in shotium/package.json, push and wait for checks, dispatch the six engine builds on that exact commit, tag, watch publish.yml, verify all seven packages on the registry, then write bilingual release notes. Manual invocation only; run as /release <version>.
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

The only source of truth is `shotium/package.json`, and it holds the version
seven times: `version`, plus the six self-referencing pins under
`optionalDependencies`. Everything else (the `.7z` names, the platform
package tarball names, the release title) is derived from it at build time.

```bash
git --no-optional-locks grep -n '"<previous version>"' -- shotium/package.json   # exactly 7 lines
```

Edit all seven to `$version`. `checks.yml` fails if the pins and `version`
disagree.

Also note, for a follow-up commit *after* the release:
`.github/workflows/performance-regression.yml` carries the previous version as
the `baseline_version` default, and `tools/shot/daemon_protocol_check.cjs`
uses a literal version in one fixture.

Commit and push only that file:

```bash
git add shotium/package.json
git commit -m "release: v$version"
git push
gh run watch -R sj817/shotium $(gh run list -R sj817/shotium --workflow checks.yml --limit 1 --json databaseId --jq '.[0].databaseId')
```

## 2. Six engine builds on this commit

```bash
SHA=$(git rev-parse HEAD)
gh workflow run engine-windows.yml -R sj817/shotium --ref main -f arch=x64
gh workflow run engine-windows.yml -R sj817/shotium --ref main -f arch=arm64
gh workflow run engine-linux.yml   -R sj817/shotium --ref main -f mode=build -f arch=x64
gh workflow run engine-linux.yml   -R sj817/shotium --ref main -f mode=build -f arch=arm64
gh workflow run engine-macos.yml   -R sj817/shotium --ref main -f mode=build -f arch=x64
gh workflow run engine-macos.yml   -R sj817/shotium --ref main -f mode=build -f arch=arm64
gh run list -R sj817/shotium --commit "$SHA"
```

- `mode` defaults to `probe` on Linux and macOS. A probe run is
  `gn gen` + `ninja -n`, compiles nothing, and produces no artifact.
- `run_checks` defaults to true; leave it. The run is not green unless the
  check suites passed against the binary it built.
- With unchanged C++ the compile caches hit and all six finish in about
  25 minutes; cold builds take 1 to 4 hours (Windows arm64 and macOS arm64
  are the slow ones).
- Each run uploads the `.7z` and the npm platform package as artifacts; the
  release step is not part of these workflows any more.

Do not tag until all six show `completed success` for `$SHA`.

## 3. Tag

```bash
git tag -a v$version -m "v$version"
git push origin v$version
gh run watch -R sj817/shotium $(gh run list -R sj817/shotium --workflow publish.yml --limit 1 --json databaseId --jq '.[0].databaseId')
```

`publish.yml` publishes the six platform packages first, then
`@shotkit/shotium`, and only then creates a non-draft GitHub release with the
six `.7z` archives attached (the `.tgz` files belong to the registry). Never
create a draft release by hand: a draft creates no git tag until it is
undrafted, and its `targetCommitish` is frozen at creation, which is how
v0.1.0 ended up on npm with no tag in git.

`workflow_dispatch` with `dry_run=true` rehearses the whole thing against a
ref and publishes nothing; use it if the workflow itself changed.

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

Then install from a clean directory and run the README example once.

## 5. Release notes

The release is created with install instructions and provenance only. Add
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
