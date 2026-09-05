// Shared plumbing for the two README asset generators, docs-demo.ts and
// docs-assets.ts.
//
// Both of them need the same thing: a throwaway directory that looks like a
// user's project -- docs/demo/*.html and docs/demo/card.mjs beside a
// package.json -- so that what the README shows is a real install of the
// published package rather than a checkout rendering itself.

import {copyFileSync, existsSync, mkdirSync, rmSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {execaSync} from 'execa';
import which from 'which';

import {root} from './repo.ts';

export const repoRoot = root;
export const demoDir = path.join(root, 'docs', 'demo');
export const assetsDir = path.join(root, 'docs', 'assets');
export const runDir = path.join(root, 'docs', '.demo-run');
export const isWindows = process.platform === 'win32';

// The sources copied into the sandbox. card.mjs is also the file frozen into
// docs/assets/example-node.webp, so the code in the README and the code in
// the demo cannot drift apart.
const SOURCES = ['card.html', 'card.mjs'];

export interface RunOptions {
  cwd?: string;
  capture?: boolean;
  allowFailure?: boolean;
}

export function run(command: string, args: string[], options: RunOptions = {}) {
  // stdin is closed rather than inherited: freeze reads from it when it is
  // left open and not a terminal, and then waits forever.
  const result = execaSync(command, args, {
    cwd: options.cwd,
    stdin: 'ignore',
    stdout: options.capture ? 'pipe' : 'inherit',
    stderr: options.capture ? 'pipe' : 'inherit',
    reject: false,
  });
  if (result.failed && result.exitCode === undefined) {
    throw new Error(`${command} could not be started: ${result.shortMessage}`);
  }
  if (result.exitCode !== 0 && !options.allowFailure) {
    const detail = options.capture ? `\n${result.stdout}${result.stderr}` : '';
    throw new Error(`${command} exited with ${result.exitCode}${detail}`);
  }
  return {stdout: result.stdout ?? '', stderr: result.stderr ?? '', status: result.exitCode};
}

export function findOnPath(command: string): string | null {
  return which.sync(command, {nothrow: true});
}

// Rebuilds docs/.demo-run from docs/demo. `install` adds the published
// package now: docs-assets needs the module immediately; docs-demo wants the
// install to happen on camera instead, so it passes false.
export function prepareWorkspace({install}: {install: boolean}): string {
  rmSync(runDir, {recursive: true, force: true});
  mkdirSync(runDir, {recursive: true});
  mkdirSync(assetsDir, {recursive: true});
  for (const name of SOURCES) copyFileSync(path.join(demoDir, name), path.join(runDir, name));
  writeFileSync(
      path.join(runDir, 'package.json'),
      `${JSON.stringify({name: 'shotium-demo', private: true, type: 'module', packageManager: 'pnpm@9.15.9'}, null, 2)}\n`);
  if (install) {
    console.log('> pnpm add @shotkit/shotium');
    run('pnpm', ['add', '--save-exact', '--no-lockfile', '@shotkit/shotium'], {cwd: runDir});
  }
  return runDir;
}

export function requireWorkspaceModule(): string {
  const entry = path.join(runDir, 'node_modules', '@shotkit', 'shotium', 'dist', 'index.js');
  if (!existsSync(entry)) throw new Error(`@shotkit/shotium is not installed in ${runDir}`);
  return entry;
}
