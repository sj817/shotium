// Every file the shot build graph depends on, checked against the disk.
//
// GN writes an edge's inputs into build.ninja whether or not the file is
// there. Ninja only looks when it comes to build that edge, and on a warm
// build directory that may be never. So a file deleted from the tree can sit
// in the graph unnoticed for as long as the cache keeps holding its output --
// and then the first cold build stops twelve seconds in with
//
//   ninja: error: '../../.rustfmt.toml', needed by
//   'gen/third_party/crubit/support/rs_std/rs_alloc.h',
//   missing and no known rule to make it
//
// which is what happened after 1ee6e5a, on two platforms, an hour of CI
// apiece. `gn gen` does not catch this: GN never opened .rustfmt.toml; it
// copied the path into an action's `inputs` and moved on. Neither does a probe
// -- `ninja -n` walks the same graph and gives up on the first missing input
// rather than listing them. What catches it is asking ninja for the input set
// and stat()ing every entry, which is all this does.
//
//   pnpm missing-inputs                 # out/Shot
//   pnpm missing-inputs out/ProbeLinuxX64
//
// Exit status is 1 if anything is missing, so it works as a build step. One
// build directory answers for one platform: the graph names the sources that
// platform selects. A relative directory is resolved against the repository
// root.

import {existsSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';

import {resolve, root} from './lib/repo.ts';

// The two targets the engine workflows build. Anything not reachable from
// them is not this project's problem -- the graph also carries Chromium's
// tests, whose sources were deliberately cut.
const TARGETS = ['shot', 'shot_c'];

function ninjaBinary(): string {
  for (const candidate of ['third_party/ninja/ninja', 'third_party/ninja/ninja.exe']) {
    if (existsSync(resolve(candidate))) return resolve(candidate);
  }
  return 'ninja';
}

async function main(outArg: string): Promise<number> {
  const out = resolve(outArg);
  if (!existsSync(path.join(out, 'build.ninja'))) {
    console.error(`${outArg} has no build.ninja -- run gn gen first`);
    return 2;
  }
  const listed = await execa(ninjaBinary(), ['-C', out, '-t', 'inputs', ...TARGETS], {cwd: root, reject: false, maxBuffer: 1 << 28});
  if (listed.exitCode !== 0) {
    console.error(listed.stderr);
    return 2;
  }
  let checked = 0;
  const missing: string[] = [];
  for (const line of listed.stdout.split(/\r?\n/)) {
    const p = line.trim().replace(/^"|"$/g, '');
    // Three kinds of input come back. `../../x` is a file in this tree and is
    // the only kind worth checking; an absolute path belongs to the SDK or the
    // toolchain, which is the host's business; anything else is generated and
    // lives under out/, so its absence is a build order question rather than
    // a missing file.
    if (!p.startsWith('../../')) continue;
    checked++;
    const source = p.slice(6);
    if (!existsSync(resolve(source))) missing.push(source);
  }
  console.log(`${outArg}: ${checked} source-tree inputs of ${TARGETS.join(' + ')}`);
  if (missing.length === 0) {
    console.log('all present');
    return 0;
  }
  console.log(`${missing.length} missing:`);
  for (const p of missing.sort()) console.log(`  ${p}`);
  console.log('\nEach one is an edge in the graph with nothing behind it. Restore\nit from before the commit that removed it -- `git show <sha>^:<path>`\n-- or cut the target that wants it. See docs/upstream-sync.md.');
  return 1;
}

const cli = cac('missing-inputs');
cli.command('[out]', 'stat every source-tree input of the engine targets')
    .action(async (out: string | undefined) => {
      process.exitCode = await main(out ?? 'out/Shot');
    });
cli.help();
cli.parse();
