// Regenerates docs/assets/demo.gif from docs/demo.tape.
//
//   pnpm run docs:demo
//
// VHS drives a real terminal (ttyd) and encodes it with ffmpeg, so all three
// have to be on PATH:
//
//   go install github.com/charmbracelet/vhs@latest
//   ... plus ttyd and ffmpeg from your package manager.
//
// Windows note: `go install` currently produces a vhs whose PNG path crashes
// under Go 1.25 (wazero v1.8 and the newer garbage collector disagree). vhs
// itself is fine -- that bug only bites freeze, see tools/shot/docs_assets.mjs.

import { existsSync, statSync } from 'node:fs';
import { join, relative } from 'node:path';

import { prepareWorkspace, repoRoot, run, which } from './docs_workspace.mjs';

const TAPE = join('docs', 'demo.tape');
const OUTPUT = join(repoRoot, 'docs', 'assets', 'demo.gif');

for (const tool of ['vhs', 'ttyd', 'ffmpeg']) {
  if (!which(tool)) {
    console.error(`docs:demo needs ${tool} on PATH.`);
    console.error('  vhs:    go install github.com/charmbracelet/vhs@latest');
    console.error('  ttyd:   https://github.com/tsl0922/ttyd/releases');
    console.error('  ffmpeg: https://ffmpeg.org/download.html');
    process.exit(1);
  }
}
if (!which('bash')) {
  // The tape asks for bash so the recording looks the same on every platform.
  console.error('docs:demo needs bash on PATH (the tape sets `Set Shell "bash"`).');
  console.error('On Windows, Git for Windows provides one.');
  process.exit(1);
}

// The install is part of the recording, so leave the sandbox empty.
prepareWorkspace({ install: false });

console.log(`> vhs ${TAPE}`);
run('vhs', [TAPE], { cwd: repoRoot, noShell: true });

if (!existsSync(OUTPUT)) {
  throw new Error(`vhs reported success but ${OUTPUT} is missing`);
}
const kb = (statSync(OUTPUT).size / 1024).toFixed(0);
console.log(`wrote ${relative(repoRoot, OUTPUT)} (${kb} KB)`);
