// Assembles one of the six @shotkit/shotium-<os>-<arch> packages.
//
// The engine cannot ship inside @shotkit/shotium itself: it is a Chromium
// build, 42 MB, and there is a different one for every platform and
// architecture. So each build produces a package of its own, and the main
// package depends on all six as optionalDependencies with `os` and `cpu` set
// -- npm then installs the one that matches and skips the other five. See
// shotium/src/lib/platform.ts, which is the code that finds whichever one
// landed.
//
//   pnpm package:platform --build out/Shot --os win --arch x64 --dest dist/npm \
//       --addon shotium/native/build/Release/shotium.node
//
// What ships is the shared library, the addon linked against it, and the two
// resource packs. Not the executable: it is a second, independent copy of the
// same engine (shot/BUILD.gn links shot_core into both the executable and the
// shared library), and shipping both put 19.75 MB of duplicate engine in a
// 40 MB download for a process-spawning path node has no reason to take. The
// executable is what the standalone .7z archives on the releases page carry.
//
// --addon is therefore required. A package without one installs and then
// cannot render: there is nothing else in it that node can call.
//
// This script does not run npm. It writes a directory; the caller runs
// `npm pack` or `npm publish` on it, because those need credentials and a
// registry and this needs neither. Relative paths are resolved against the
// repository root.

import {chmodSync, copyFileSync, existsSync, mkdirSync, readFileSync, rmSync, statSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';

import {resolve} from './lib/repo.ts';

// os as this script and the release archives spell it, mapped to the two
// spellings npm and node use. `os` in a package.json is matched against
// process.platform, which is `win32` and `darwin` and has been for long
// enough that nothing is going to change it.
const PLATFORMS: Record<string, {npmOs: string; library: string; extra: string[]}> = {
  // The import library is a build input, not a run-time one: nothing that
  // installs this package links against the DLL, so it stays out.
  win: {npmOs: 'win32', library: 'shotium.dll', extra: []},
  mac: {npmOs: 'darwin', library: 'libshotium.dylib', extra: []},
  linux: {npmOs: 'linux', library: 'libshotium.so', extra: []},
};
const ARCHES = ['x64', 'arm64'];
const PAKS = ['shotium_data.pak', 'shotium_strings.pak'];

function copy(from: string, to: string, mode?: number): void {
  if (!existsSync(from)) throw new Error(`missing build output: ${from}`);
  copyFileSync(from, to);
  // npm preserves the executable bit through pack and publish, so this is
  // what makes the engine spawnable on the machine that installs it. It is
  // set here rather than trusted from the build directory because a file
  // that arrives over an artifact download has lost it.
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
  const mainPkg = JSON.parse(readFileSync(resolve('shotium', 'package.json'), 'utf8')) as {version: string; license?: string; engines?: unknown; repository?: unknown};
  // npmOs, not args.os: the package is named for process.platform, because
  // that is what npm matches its `os` field against and what the caller's
  // machine calls itself. The archives keep win/mac -- people read those.
  const name = `@shotkit/shotium-${platform.npmOs}-${args.arch}`;
  const buildDir = resolve(args.build);
  const dest = resolve(args.dest, `shotium-${args.os}-${args.arch}`);

  rmSync(dest, {recursive: true, force: true});
  mkdirSync(dest, {recursive: true});
  const shipped: string[] = [];
  copy(path.join(buildDir, platform.library), path.join(dest, platform.library), 0o755);
  shipped.push(platform.library);
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
    description: `The shotium engine for ${args.os}-${args.arch}. Installed by @shotkit/shotium; not useful on its own.`,
    // os and cpu are the whole point of this package. npm skips an optional
    // dependency whose os/cpu do not match the machine, which is how one
    // install of @shotkit/shotium pulls one engine instead of six.
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
          'shared library behind the C ABI, the node addon linked against it,\n' +
          'and the two resource packs it reads.\n\n' +
          'Install [`@shotkit/shotium`](https://www.npmjs.com/package/' +
          '@shotkit/shotium) instead. It depends on all six and pnpm installs\n' +
          'whichever matches the machine.\n');

  const bytes = shipped.reduce((total, file) => total + statSync(path.join(dest, file)).size, 0);
  process.stdout.write(`${name}@${manifest.version}\n  ${dest}\n${shipped.map((f) => `  ${f}`).join('\n')}\n  ${(bytes / (1024 * 1024)).toFixed(1)} MB unpacked\n`);
}

const cli = cac('make-platform-package');
cli.command('', 'assemble one @shotkit/shotium-<os>-<arch> package directory')
    .option('--build <dir>', 'the build directory holding the library and the packs')
    .option('--os <name>', 'win, mac or linux')
    .option('--arch <name>', 'x64 or arm64')
    .option('--dest <dir>', 'where the package directory goes')
    .option('--addon <file>', 'the node addon linked against this build')
    .action((options: {build?: string; os?: string; arch?: string; dest?: string; addon?: string}) => {
      try {
        for (const required of ['build', 'os', 'arch', 'dest'] as const) {
          if (!options[required]) throw new Error(`--${required} is required`);
        }
        main(options as {build: string; os: string; arch: string; dest: string; addon?: string});
      } catch (error) {
        console.error(error instanceof Error ? error.message : String(error));
        process.exitCode = 1;
      }
    });
cli.help();
cli.parse();
