// Where the repository is, for every script in this directory.
//
// pnpm runs a package script with cwd = scripts/, and the root aliases go
// through a second pnpm (`pnpm -C scripts <name>`) that overwrites INIT_CWD
// with the root again, so the directory the user typed the command in is not
// recoverable. The repository root is the one anchor both levels agree on;
// every user-supplied relative path is resolved against it, and --help says so.

import path from 'node:path';

export const root = path.resolve(import.meta.dirname, '..', '..');

export function resolve(...segments: string[]): string {
  return path.resolve(root, ...segments);
}

// The engine's CLI and shared library, by platform, as build-engine.ts names them.
export const exeName = process.platform === 'win32' ? 'shotium.exe' : 'shotium';
export const libraryName = process.platform === 'win32' ? 'shotium.dll' :
    process.platform === 'darwin'                       ? 'libshotium.dylib' :
                                                          'libshotium.so';

export const sleep = (ms: number): Promise<void> => new Promise((r) => setTimeout(r, ms));
