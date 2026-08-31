// Regenerates the still images the README embeds.
//
//   npm run docs:assets
//
// Three files land in docs/assets:
//
//   card.webp          docs/demo/card.html, rendered by shotium itself
//   example-node.webp  docs/demo/card.mjs, frozen as syntax-highlighted code
//   example-cli.webp   a real CLI session, frozen the same way
//
// Needs freeze (https://github.com/charmbracelet/freeze) and ffmpeg on PATH.
// freeze only implements .svg and .png despite what its --help claims, so the
// .webp conversion is done here with ffmpeg.
//
// Windows caveat: `go install github.com/charmbracelet/freeze@latest` builds
// v0.2.2 against wazero v1.8.0, whose WebAssembly host crashes the Go 1.25
// garbage collector as soon as freeze rasterises anything ("unexpected signal
// during runtime execution", 0xc0000005). Build it with a newer wazero:
//
//   git clone --depth 1 --branch v0.2.2 https://github.com/charmbracelet/freeze
//   cd freeze && go get github.com/tetratelabs/wazero@v1.9.0 && go build .
//
// The CLI session needs a shotium executable and a bash. Point SHOTIUM_CLI at
// one, or let this script find out/Shot*/shotium. Without either, the session
// recorded in docs/demo/cli-session.txt is reused, so the image can always be
// rebuilt even on a machine that has never compiled the engine.

import { existsSync, readFileSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { join, relative } from 'node:path';
import { pathToFileURL } from 'node:url';

import {
  assetsDir,
  demoDir,
  isWindows,
  prepareWorkspace,
  repoRoot,
  requireWorkspaceModule,
  run,
  runDir,
  which,
} from './docs_workspace.mjs';

const exe = isWindows ? '.exe' : '';
const CLI_TRANSCRIPT = join(demoDir, 'cli-session.txt');

// Every command in the CLI still. SHOTIUM is replaced by the path to the
// executable under test; the image always shows the plain name.
const CLI_STEPS = [
  'SHOTIUM card.html --width 720 --height 380 --scale 2 -o card.png',
  'SHOTIUM https://example.com --full-page --type webp --quality 85 -o page.webp',
  'cat card.html | SHOTIUM --stdin --width 720 --height 380 -o piped.png',
  'du -h card.png page.webp piped.png',
];

function findCli() {
  if (process.env.SHOTIUM_CLI) {
    return process.env.SHOTIUM_CLI;
  }
  for (const dir of ['Shot', 'ShotPgo', 'ShotWip']) {
    const candidate = join(repoRoot, 'out', dir, `shotium${exe}`);
    if (existsSync(candidate)) {
      return candidate;
    }
  }
  return which('shotium');
}

// freeze -> png -> webp. Straight to .webp would silently write SVG bytes.
function freeze(source, output, extra) {
  const png = `${output}.png`;
  run('freeze', [
    source,
    '--theme', 'catppuccin-mocha',
    '--window',
    '--border.radius', '8',
    '--padding', '30,40',
    '--margin', '0',
    '--shadow.blur', '24',
    '--shadow.x', '0',
    '--shadow.y', '12',
    '--font.size', '15',
    '--line-height', '1.5',
    ...extra,
    '--output', png,
  ], { noShell: true });
  if (!existsSync(png) || readFileSync(png).subarray(1, 4).toString('latin1') !== 'PNG') {
    throw new Error(`freeze wrote ${png} but it is not a PNG -- see the wazero note at the top of this file`);
  }
  run('ffmpeg', ['-y', '-loglevel', 'error', '-i', png, '-c:v', 'libwebp', '-lossless', '1',
    '-compression_level', '6', output], { noShell: true });
  rmSync(png, { force: true });
  report(output);
}

function report(file) {
  const kb = (statSync(file).size / 1024).toFixed(0);
  console.log(`wrote ${relative(repoRoot, file)} (${kb} KB)`);
}

async function renderWithShotium(shots) {
  const shotium = await import(pathToFileURL(requireWorkspaceModule()).href);
  shotium.default.start();
  try {
    for (const { file, output, viewport } of shots) {
      await shotium.screenshot({
        // An explicit file:// URL, so an absolute path is never weighed up
        // against the URL schemes first.
        file: pathToFileURL(join(runDir, file)).href,
        viewport,
        scale: 2,
        type: 'webp',
        quality: 92,
        path: output,
      });
      report(output);
    }
  } finally {
    await shotium.default.stop();
  }
}

function recordCliSession() {
  const cli = findCli();
  if (!cli || !which('bash')) {
    console.log(`reusing ${relative(repoRoot, CLI_TRANSCRIPT)} (no shotium executable and bash on this machine)`);
    return;
  }
  console.log(`> recording the CLI session with ${cli}`);
  const lines = [];
  for (const step of CLI_STEPS) {
    lines.push(`$ ${step.replaceAll('SHOTIUM', 'shotium')}`);
    const result = run('bash', ['-c', step.replaceAll('SHOTIUM', JSON.stringify(cli))], {
      cwd: runDir,
      capture: true,
      noShell: true,
    });
    const output = `${result.stdout}${result.stderr}`.trim();
    if (output) {
      lines.push(output);
    }
  }
  writeFileSync(CLI_TRANSCRIPT, `${lines.join('\n')}\n`);
}

prepareWorkspace({ install: true });

await renderWithShotium([
  { file: 'card.html', output: join(assetsDir, 'card.webp'), viewport: { width: 720, height: 380 } },
]);

freeze(join(demoDir, 'card.mjs'), join(assetsDir, 'example-node.webp'), ['--language', 'javascript']);

recordCliSession();
freeze(CLI_TRANSCRIPT, join(assetsDir, 'example-cli.webp'), ['--language', 'console']);
