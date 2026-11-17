# Source slice

`src/` holds the files of `https://chromium.googlesource.com/external/github.com/llvm/llvm-project/libcxxabi` at
`fc1897a2c12aa27e703c3ed48b62eba8abf4ce19` that at least one of the eight engine builds reads
(30 files). It was a gclient DEPS checkout until shotium 0.12.4 and is
now maintained directly in this repository; there is no DEPS entry, gitlink
or patch to replay.

The set is the union of the build graphs every engine workflow exports
(`pnpm graph:export`: ninja inputs, `-t graph` nodes, reachable deps-log
entries, depfiles, jumbo sources, gn's build.ninja.d) for Windows, macOS and
Linux glibc/musl on x64 and arm64, closed over the rules in
`scripts/tree/trim-tree.ts` (Python packages around a kept script, licence
files and .gitignore in every ancestor directory).

A build that starts reading a file outside the set fails with a missing
include or input; take that file from the upstream repository at the
revision above. To move to a newer upstream, fetch it and compare the two
revisions with `pnpm upstream:sync plan --upstream-prefix third_party/libc++abi/src --retained-only`
(apps/docs/upstream-sync-tool.md), which only touches the files kept here.
