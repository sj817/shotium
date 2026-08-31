// Shared plumbing for the two README asset generators, tools/shot/docs_demo.mjs
// and tools/shot/docs_assets.mjs.
//
// Both of them need the same thing: a throwaway directory that looks like a
// user's project -- docs/demo/*.html and docs/demo/card.mjs beside a
// package.json -- so that what the README shows is a real install of the
// published package rather than a checkout rendering itself.

import { spawnSync } from 'node:child_process';
import { copyFileSync, existsSync, mkdirSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

export const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
export const demoDir = join(repoRoot, 'docs', 'demo');
export const assetsDir = join(repoRoot, 'docs', 'assets');
export const runDir = join(repoRoot, 'docs', '.demo-run');

export const isWindows = process.platform === 'win32';

// The sources copied into the sandbox. card.mjs is also the file frozen into
// docs/assets/example-node.webp, so the code in the README and the code in the
// demo cannot drift apart.
const SOURCES = ['card.html', 'card.mjs', 'hero.html'];

export function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    // stdin is closed rather than inherited: freeze reads from it when it is
    // left open and not a terminal, and then waits forever.
    stdio: options.capture ? 'pipe' : ['ignore', 'inherit', 'inherit'],
    encoding: 'utf8',
    shell: isWindows && !options.noShell,
    ...options,
  });
  if (result.error) {
    throw new Error(`${command} could not be started: ${result.error.message}`);
  }
  if (result.status !== 0 && !options.allowFailure) {
    const detail = options.capture ? `\n${result.stdout}${result.stderr}` : '';
    throw new Error(`${command} exited with ${result.status}${detail}`);
  }
  return result;
}

export function which(command) {
  const probe = spawnSync(isWindows ? 'where' : 'which', [command], {
    encoding: 'utf8',
    shell: isWindows,
  });
  if (probe.status !== 0 || !probe.stdout) {
    return null;
  }
  return probe.stdout.split(/\r?\n/)[0].trim() || null;
}

/**
 * Rebuilds docs/.demo-run from docs/demo.
 *
 * @param {object} options
 * @param {boolean} options.install  npm install the published package now.
 *   docs_assets.mjs needs the module immediately; docs_demo.mjs wants the
 *   install to happen on camera instead, so it passes false.
 */
export function prepareWorkspace({ install }) {
  rmSync(runDir, { recursive: true, force: true });
  mkdirSync(runDir, { recursive: true });
  mkdirSync(assetsDir, { recursive: true });

  for (const name of SOURCES) {
    copyFileSync(join(demoDir, name), join(runDir, name));
  }
  writeFileSync(
    join(runDir, 'package.json'),
    `${JSON.stringify({ name: 'shotium-demo', private: true, type: 'module' }, null, 2)}\n`,
  );

  if (install) {
    console.log('> npm install @shotkit/shotium');
    run('npm', ['install', '--no-audit', '--no-fund', '@shotkit/shotium'], { cwd: runDir });
  }
  return runDir;
}

export function requireWorkspaceModule() {
  const entry = join(runDir, 'node_modules', '@shotkit', 'shotium', 'dist', 'index.js');
  if (!existsSync(entry)) {
    throw new Error(`@shotkit/shotium is not installed in ${runDir}`);
  }
  return entry;
}
