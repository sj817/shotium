'use strict';

// Assembles one of the six @shotkit/shotium-<os>-<arch> packages.
//
// The engine cannot ship inside @shotkit/shotium itself: it is a Chromium
// build, 41 MB, and there is a different one for every platform and
// architecture. So each build produces a package of its own, and the main
// package depends on all six as optionalDependencies with `os` and `cpu` set
// -- npm then installs the one that matches and skips the other five. See
// shotium/lib/platform.js, which is the code that finds whichever one landed.
//
//   node tools/shot/make_platform_package.cjs \
//       --build out/Shot --os win --arch x64 --dest dist/npm
//
// --addon is optional and names a built shotium.node. Where it is given the
// package carries the in-process engine as well; where it is not -- an arm64
// build cross-compiled on an x64 host, which has no way to produce or check an
// addon -- the package is still complete for the worker pool and the daemon,
// which is what require("@shotkit/shotium") uses. native.js reports the addon
// as missing rather than the install as broken.
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
    binary: 'shotium.exe',
    library: 'shotium.dll',
    // The import library is a build input, not a run-time one: nothing that
    // installs this package links against the DLL, so it stays out.
    extra: [],
  },
  mac: {
    npmOs: 'darwin',
    binary: 'shotium',
    library: 'libshotium.dylib',
    extra: [],
  },
  linux: {
    npmOs: 'linux',
    binary: 'shotium',
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
  // See shotium/lib/platform.js, which is the other half of this.
  const name = `@shotkit/shotium-${platform.npmOs}-${args.arch}`;
  const buildDir = path.resolve(ROOT, args.build);
  const dest = path.resolve(ROOT, args.dest, `shotium-${args.os}-${args.arch}`);

  fs.rmSync(dest, {recursive: true, force: true});
  fs.mkdirSync(dest, {recursive: true});

  const shipped = [];

  copy(path.join(buildDir, platform.binary), path.join(dest, platform.binary),
       0o755);
  shipped.push(platform.binary);

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
  }

  const manifest = {
    name,
    version: mainPkg.version,
    description:
        `The shotium engine for ${args.os}-${args.arch}. Installed by ` +
        '@shotkit/shotium; not useful on its own.',
    // os and cpu are the whole point of this package. npm skips an optional
    // dependency whose os/cpu do not match the machine, which is how one
    // install of @shotkit/shotium pulls 41 MB instead of six times that.
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
          'engine executable, the shared library behind the C ABI, the two\n' +
          'resource packs they both read' +
          (args.addon ? ', and the node addon' : '') + '.\n\n' +
          'Install [`@shotkit/shotium`](https://www.npmjs.com/package/' +
          '@shotkit/shotium) instead. It depends on all six and npm installs\n' +
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
