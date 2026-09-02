'use strict';

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
//   node tools/shot/make_platform_package.cjs \
//       --build out/Shot --os win --arch x64 --dest dist/npm \
//       --addon shotium/native/build/Release/shotium.node
//
// What ships is the shared library, the addon linked against it, and the two
// resource packs. Not the executable.
//
// That is worth saying because the executable is also built, sits beside the
// library, and used to ship here. It is a second, independent copy of the same
// engine: shot/BUILD.gn links `shot_core` -- a source_set, so its objects go
// into every target that depends on it -- into both `executable("shot")` and
// `shared_library("shot_c")`. The two came to 43,582,464 and 43,560,448 bytes,
// 22 KB apart, and the executable held no reference to the library at all.
// Shipping both put 19.75 MB of duplicate engine in a 40 MB download, for a
// process-spawning path that node has no reason to take: the addon is in-
// process, so there is no spawn, no pipe and no copy of the image across a
// process boundary.
//
// The executable still exists and is still worth having -- it is what the
// standalone .7z archives on the releases page carry, for using the engine
// without node at all. It is just not part of a pnpm install.
//
// --addon is therefore required in practice. A package without one installs
// and then cannot render: there is nothing else in it that node can call.
//
// This script does not run npm. It writes a directory; the caller runs
// `npm pack` or `npm publish` on it, because those need credentials and a
// registry and this needs neither.

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..', '..');

// os as this script and the release archives spell it, mapped to the two
// spellings npm and node use. `os` in a package.json is matched against
// process.platform, which is `win32` and `darwin` and has been for long
// enough that nothing is going to change it.
const PLATFORMS = {
  win: {
    npmOs: 'win32',
    library: 'shotium.dll',
    // The import library is a build input, not a run-time one: nothing that
    // installs this package links against the DLL, so it stays out.
    extra: [],
  },
  mac: {
    npmOs: 'darwin',
    library: 'libshotium.dylib',
    extra: [],
  },
  linux: {
    npmOs: 'linux',
    library: 'libshotium.so',
    extra: [],
  },
};

const ARCHES = ['x64', 'arm64'];

const PAKS = ['shotium_data.pak', 'shotium_strings.pak'];

function parseArgs(argv) {
  const args = {};
  for (let i = 0; i < argv.length; i += 2) {
    const flag = argv[i];
    if (!flag.startsWith('--') || argv[i + 1] === undefined) {
      throw new Error(`bad argument: ${flag}`);
    }
    args[flag.slice(2)] = argv[i + 1];
  }
  for (const required of ['build', 'os', 'arch', 'dest']) {
    if (!args[required]) {
      throw new Error(`--${required} is required`);
    }
  }
  if (!PLATFORMS[args.os]) {
    throw new Error(`--os must be one of ${Object.keys(PLATFORMS).join(', ')}`);
  }
  if (!ARCHES.includes(args.arch)) {
    throw new Error(`--arch must be one of ${ARCHES.join(', ')}`);
  }
  return args;
}

// The version is the main package's, always. Seven packages that must be
// installed together are seven packages that have to agree on a number, and
// the only way to keep them agreeing is for six of them not to have an opinion.
function mainPackage() {
  return JSON.parse(
      fs.readFileSync(path.join(ROOT, 'shotium', 'package.json'), 'utf8'));
}

function copy(from, to, mode) {
  if (!fs.existsSync(from)) {
    throw new Error(`missing build output: ${from}`);
  }
  fs.copyFileSync(from, to);
  if (mode !== undefined) {
    // npm preserves the executable bit through pack and publish, so this is
    // what makes the engine spawnable on the machine that installs it. It is
    // set here rather than trusted from the build directory because a file
    // that arrives over an artifact download has lost it.
    fs.chmodSync(to, mode);
  }
}

function main() {
  const args = parseArgs(process.argv.slice(2));
  const platform = PLATFORMS[args.os];
  const mainPkg = mainPackage();
  // npmOs, not args.os: the package is named for process.platform, because
  // that is what npm matches its `os` field against and what the caller's
  // machine calls itself. The archives keep win/mac -- people read those.
  // See shotium/src/lib/platform.ts, which is the other half of this.
  const name = `@shotkit/shotium-${platform.npmOs}-${args.arch}`;
  const buildDir = path.resolve(ROOT, args.build);
  const dest = path.resolve(ROOT, args.dest, `shotium-${args.os}-${args.arch}`);

  fs.rmSync(dest, {recursive: true, force: true});
  fs.mkdirSync(dest, {recursive: true});

  const shipped = [];

  copy(path.join(buildDir, platform.library), path.join(dest, platform.library),
       0o755);
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
    copy(path.resolve(ROOT, args.addon), path.join(dest, 'shotium.node'), 0o755);
    shipped.push('shotium.node');
  } else {
    // Not a warning. A package with no addon is a package that installs and
    // then cannot render, and the machine that finds out is the user's.
    throw new Error(
        'no --addon: the addon is the only thing in this package that node ' +
        'can call, so a package without one is not publishable');
  }

  const manifest = {
    name,
    version: mainPkg.version,
    description:
        `The shotium engine for ${args.os}-${args.arch}. Installed by ` +
        '@shotkit/shotium; not useful on its own.',
    // os and cpu are the whole point of this package. npm skips an optional
    // dependency whose os/cpu do not match the machine, which is how one
    // install of @shotkit/shotium pulls one engine instead of six.
    os: [platform.npmOs],
    cpu: [args.arch],
    files: shipped.slice().sort(),
    license: mainPkg.license,
    engines: mainPkg.engines,
  };
  if (mainPkg.repository) {
    manifest.repository = mainPkg.repository;
  }

  fs.writeFileSync(path.join(dest, 'package.json'),
                   JSON.stringify(manifest, null, 2) + '\n');

  fs.writeFileSync(
      path.join(dest, 'README.md'),
      `# ${name}\n\n` +
          `The shotium engine built for ${args.os}-${args.arch}.\n\n` +
          'This package is one of six, and holds bytes rather than code: the\n' +
          'shared library behind the C ABI, the node addon linked against it,\n' +
          'and the two resource packs it reads.\n\n' +
          'Install [`@shotkit/shotium`](https://www.npmjs.com/package/' +
          '@shotkit/shotium) instead. It depends on all six and pnpm installs\n' +
          'whichever matches the machine.\n');

  const bytes = shipped.reduce(
      (total, file) => total + fs.statSync(path.join(dest, file)).size, 0);
  process.stdout.write(
      `${name}@${manifest.version}\n` +
      `  ${dest}\n` +
      shipped.map((f) => `  ${f}`).join('\n') + '\n' +
      `  ${(bytes / (1024 * 1024)).toFixed(1)} MB unpacked\n`);
}

main();
