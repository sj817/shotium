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

import { existsSync, rmSync, statSync } from 'node:fs';
import { join, relative } from 'node:path';

import { prepareWorkspace, repoRoot, run, which } from './docs_workspace.mjs';

const TAPE = join('docs', 'demo.tape');
const TERMINAL_OUTPUT = join(repoRoot, 'docs', '.demo-terminal.gif');
const CARD = join(repoRoot, 'docs', 'assets', 'card.webp');
const OUTPUT = join(repoRoot, 'docs', 'assets', 'demo.gif');
const EFFECT_SECONDS = 5;

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

// Download once off camera, then rebuild an empty project. The recorded npm
// install remains a real install while avoiding a long, network-bound pause in
// the GIF.
const warmWorkspace = prepareWorkspace({ install: false });
console.log('> warm npm cache for @shotkit/shotium');
run('npm', ['install', '--no-audit', '--no-fund', '@shotkit/shotium'], { cwd: warmWorkspace });
prepareWorkspace({ install: false });
rmSync(TERMINAL_OUTPUT, { force: true });

console.log(`> vhs ${TAPE}`);
run('vhs', [TAPE], { cwd: repoRoot, noShell: true });

if (!existsSync(TERMINAL_OUTPUT)) {
  throw new Error(`vhs reported success but ${TERMINAL_OUTPUT} is missing`);
}

// End on the image that the recorded command produced. Re-encoding the short
// terminal capture and the still together lets the final card remain visible
// before the GIF loops, while keeping one compact, README-friendly asset.
const filter = [
  '[1:v]scale=1000:-1,pad=1080:830:(ow-iw)/2:(oh-ih)/2:color=0x11131c,fps=20,setsar=1[effect]',
  '[0:v]fps=20,setsar=1[terminal]',
  '[terminal][effect]concat=n=2:v=1:a=0,split[pgen][puse]',
  '[pgen]palettegen=max_colors=256:stats_mode=full[palette]',
  '[puse][palette]paletteuse=dither=sierra2_4a',
].join(';');

console.log(`> append ${EFFECT_SECONDS}s rendered-card preview`);
run('ffmpeg', [
  '-y',
  '-v', 'warning',
  '-i', TERMINAL_OUTPUT,
  '-loop', '1',
  '-t', String(EFFECT_SECONDS),
  '-i', CARD,
  '-filter_complex', filter,
  '-loop', '0',
  OUTPUT,
], { cwd: repoRoot, noShell: true });
rmSync(TERMINAL_OUTPUT, { force: true });

if (!existsSync(OUTPUT)) {
  throw new Error(`ffmpeg reported success but ${OUTPUT} is missing`);
}
const kb = (statSync(OUTPUT).size / 1024).toFixed(0);
console.log(`wrote ${relative(repoRoot, OUTPUT)} (${kb} KB)`);
