// Assembles one of the six @pixel.js/shotium-<os>-<arch> packages.
//
// The engine cannot ship inside @pixel.js/shotium itself: it is a Chromium
// build, 42 MB, and there is a different one for every platform and
// architecture. So each build produces a package of its own, and the main
// package depends on all six as optionalDependencies with `os` and `cpu` set
// -- npm then installs the one that matches and skips the other five. See
// apps/typescript/src/lib/platform.ts, which is the code that finds whichever one
// landed.
//
//   pnpm package:platform --build out/Shot --os win --arch x64 --dest dist/npm \
//       --addon out/Shot/shotium.node
//   pnpm package:platform --from-archive dist/node/shotium-node-windows-amd64.7z \
//       --os win --arch x64 --dest dist/npm
//
// The second form is what ships. The engine job archives the addon and its
// packs without a version (pnpm package:node); publish.yml, the previews and
// check-ffi turn that archive into the package for the version at hand, so
// the version bump commit does not need the engine rebuilt.
//
// npm ships the Node addon and the two resource packs it reads. The CLI
// executable and the C ABI library are GitHub Release artifacts: the addon
// is a complete copy of the engine and node never spawns the executable, so
// a second copy in the package was 41 MB that nothing loaded. The command
// line for a machine that only has npm is apps/typescript/src/cli.ts.
// --addon is required; a package without one cannot render.
//
// This script does not run npm. It writes a directory; the caller runs
// `npm pack` or `npm publish` on it, because those need credentials and a
// registry and this needs neither. Relative paths are resolved against the
// repository root.

import {chmodSync, copyFileSync, existsSync, mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, statSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';

import {verifyNativeDelivery, type NativeOS} from '../lib/release-artifacts.ts';
import {resolve} from '../lib/repo.ts';

// This script uses win/linux/mac; public Release archives use windows/linux/macos.
// Map the script values to the two
// spellings npm and node use. `os` in a package.json is matched against
// process.platform, which is `win32` and `darwin` and has been for long
// enough that nothing is going to change it.
const PLATFORMS: Record<string, {npmOs: string; extra: string[]}> = {
  // The import library is a build input, not a run-time one: nothing that
  // installs this package links against the DLL, so it stays out.
  win: {npmOs: 'win32', extra: []},
  mac: {npmOs: 'darwin', extra: []},
  linux: {npmOs: 'linux', extra: []},
};
const ARCHES = ['x64', 'arm64'];
const PAKS = ['shotium_data.pak', 'shotium_strings.pak'];

function copy(from: string, to: string, mode?: number): void {
  if (!existsSync(from)) throw new Error(`missing build output: ${from}`);
  copyFileSync(from, to);
  // npm preserves the mode through pack and publish. It is set here rather
  // than trusted from the build directory because a file that arrives over
  // an artifact download has lost it, and a package whose modes depend on
  // the route the bytes took is a diff nobody wants to explain.
  if (mode !== undefined) chmodSync(to, mode);
}

function main(args: {build: string; os: string; arch: string; dest: string; addon?: string}): void {
  const platform = PLATFORMS[args.os];
  if (!platform) throw new Error(`--os must be one of ${Object.keys(PLATFORMS).join(', ')}`);
  if (!ARCHES.includes(args.arch)) throw new Error(`--arch must be one of ${ARCHES.join(', ')}`);
  // The version is the main package's, always. Seven packages that must be
  // installed together are seven packages that have to agree on a number, and
  // the only way to keep them agreeing is for six of them not to have an
  // opinion.
  const mainPkg = JSON.parse(readFileSync(resolve('apps/typescript', 'package.json'), 'utf8')) as {version: string; license?: string; engines?: unknown; repository?: unknown};
  // npmOs, not args.os: the package is named for process.platform, because
  // that is what npm matches its `os` field against and what the caller's
  // machine calls itself. Public Release archives use their separate,
  // human-facing windows/linux/macos and amd64/arm64 names.
  const name = `@pixel.js/shotium-${platform.npmOs}-${args.arch}`;
  const buildDir = resolve(args.build);
  const dest = resolve(args.dest, `shotium-${args.os}-${args.arch}`);

  rmSync(dest, {recursive: true, force: true});
  mkdirSync(dest, {recursive: true});
  const shipped: string[] = [];
  for (const pak of PAKS) {
    copy(path.join(buildDir, pak), path.join(dest, pak), 0o644);
    shipped.push(pak);
  }
  for (const file of platform.extra) {
    copy(path.join(buildDir, file), path.join(dest, file), 0o644);
    shipped.push(file);
  }
  if (args.addon) {
    copy(resolve(args.addon), path.join(dest, 'shotium.node'), 0o755);
    shipped.push('shotium.node');
  } else {
    // Not a warning. A package with no addon is a package that installs and
    // then cannot render, and the machine that finds out is the user's.
    throw new Error('no --addon: the addon is the only thing in this package that node can call, so a package without one is not publishable');
  }

  const manifest: Record<string, unknown> = {
    name,
    version: mainPkg.version,
    description: `The shotium engine for ${args.os}-${args.arch}. Installed by @pixel.js/shotium; not useful on its own.`,
    // os and cpu are the whole point of this package. npm skips an optional
    // dependency whose os/cpu do not match the machine, which is how one
    // install of @pixel.js/shotium pulls one engine instead of six.
    os: [platform.npmOs],
    cpu: [args.arch],
    files: [...shipped].sort(),
    license: mainPkg.license,
    engines: mainPkg.engines,
  };
  if (mainPkg.repository) manifest.repository = mainPkg.repository;
  writeFileSync(path.join(dest, 'package.json'), JSON.stringify(manifest, null, 2) + '\n');
  writeFileSync(
      path.join(dest, 'README.md'),
      `# ${name}\n\n` +
          `The shotium engine built for ${args.os}-${args.arch}.\n\n` +
          'This package is one of six, and holds bytes rather than code: the\n' +
          'Node addon and the two resource packs it reads. The standalone CLI\n' +
          'and the C ABI library are on the GitHub releases page, not here.\n\n' +
          'Install [`@pixel.js/shotium`](https://www.npmjs.com/package/' +
          '@pixel.js/shotium) instead. It depends on all six and pnpm installs\n' +
          'whichever matches the machine.\n');

  const bytes = shipped.reduce((total, file) => total + statSync(path.join(dest, file)).size, 0);
  process.stdout.write(`${name}@${manifest.version}\n  ${dest}\n${shipped.map((f) => `  ${f}`).join('\n')}\n  ${(bytes / (1024 * 1024)).toFixed(1)} MB unpacked\n`);
}

// Extract a shotium-node-<platform>.7z and check it is exactly the node
// delivery before anything is copied out of it.
async function extractNodeArchive(archive: string, osName: NativeOS, sevenzip: string): Promise<{dir: string; cleanup: () => void}> {
  const temporary = mkdtempSync(path.join(os.tmpdir(), 'shotium-node-archive-'));
  const cleanup = () => rmSync(temporary, {recursive: true, force: true});
  try {
    await execa(sevenzip, ['x', resolve(archive), `-o${temporary}`, '-y'], {stdio: ['ignore', 'ignore', 'inherit']});
    const entries = readdirSync(temporary, {withFileTypes: true});
    if (entries.length !== 1 || !entries[0].isDirectory()) throw new Error(`${archive} must contain exactly one directory`);
    const dir = path.join(temporary, entries[0].name);
    await verifyNativeDelivery('node', osName, dir);
    return {dir, cleanup};
  } catch (error) {
    cleanup();
    throw error;
  }
}

const cli = cac('pnpm package:platform');
cli.command('', 'assemble one @pixel.js/shotium-<os>-<arch> package directory')
    .option('--build <dir>', 'the build directory holding the packs')
    .option('--addon <file>', 'the GN-built Node addon from this build')
    .option('--from-archive <file>', 'a shotium-node-<platform>.7z from an engine artifact, instead of --build/--addon')
    .option('--sevenzip <command>', '7-Zip executable, for --from-archive', {default: process.env.SHOTIUM_SEVENZIP || '7z'})
    .option('--os <name>', 'win, mac or linux')
    .option('--arch <name>', 'x64 or arm64')
    .option('--dest <dir>', 'where the package directory goes')
    .action(async (options: {build?: string; os?: string; arch?: string; dest?: string; addon?: string; fromArchive?: string; sevenzip: string}) => {
      try {
        for (const required of ['os', 'arch', 'dest'] as const) {
          if (!options[required]) throw new Error(`--${required} is required`);
        }
        if (options.fromArchive) {
          if (options.build || options.addon) throw new Error('--from-archive replaces --build and --addon');
          const {dir, cleanup} = await extractNodeArchive(options.fromArchive, options.os as NativeOS, options.sevenzip);
          try {
            main({build: dir, addon: path.join(dir, 'shotium.node'), os: options.os!, arch: options.arch!, dest: options.dest!});
          } finally {
            cleanup();
          }
          return;
        }
        if (!options.build) throw new Error('--build is required (or --from-archive)');
        main(options as {build: string; os: string; arch: string; dest: string; addon?: string});
      } catch (error) {
        console.error(error instanceof Error ? error.message : String(error));
        process.exitCode = 1;
      }
    });
cli.help();
cli.parse();
