import {createRequire} from 'node:module';
import path from 'node:path';

// require.resolve is the resolver, and ESM has no synchronous equivalent
// that answers for a package that may not be installed at all.
const require = createRequire(import.meta.url);

// Which package carries the engine for this machine.
//
// The engine is not in this package and cannot be: it is a Chromium build,
// 41 MB per platform and architecture, six of them, and `pnpm install` is never
// going to produce one. So the bytes live in six packages of their own and
// this one depends on all six as optionalDependencies with `os` and `cpu` set,
// which is npm's way of saying "install the one that matches this machine and
// skip the other five". A machine nobody builds for installs none of them and
// still gets a working package -- it just has to be pointed at an engine.
//
// The alternative, a postinstall script that downloads a tarball, was not
// chosen. It defeats a lockfile, which is supposed to pin what you get; it
// fails behind a registry mirror, which is the one place a large dependency
// most needs to work; and it runs code at install time in exchange for saving
// nothing that npm was not already doing.
//
// Key and name are both `${process.platform}-${process.arch}`, so the table is
// the identity map with a prefix on it. That is deliberate: the value npm
// matches `os` and `cpu` against is process.platform, and a package named for
// anything else makes the reader hold two spellings of one machine in their
// head. It is also what every other package of this shape does -- esbuild,
// swc, lightningcss all publish darwin-arm64 and win32-x64.
//
// The release archives and CI artifacts spell it windows/linux/macos and
// amd64/arm64 instead -- shotium-macos-arm64-v0.4.0.7z -- and that is not
// going to change either. They are downloaded by people, and that is what
// people call the machines. So the two spellings do differ, in the one place
// where each is right: the registry gets node's, the download page gets the
// reader's; publish.yml holds the six-line map between them.
const PACKAGES: Readonly<Record<string, string>> = {
  'win32-x64': '@shotkit/shotium-win32-x64',
  'win32-arm64': '@shotkit/shotium-win32-arm64',
  'darwin-x64': '@shotkit/shotium-darwin-x64',
  'darwin-arm64': '@shotkit/shotium-darwin-arm64',
  'linux-x64': '@shotkit/shotium-linux-x64',
  'linux-arm64': '@shotkit/shotium-linux-arm64',
};

function packageName(
    platform: string = process.platform,
    arch: string = process.arch): string|null {
  return PACKAGES[`${platform}-${arch}`] ?? null;
}

// Where the matching platform package unpacked, or null if it is not installed.
//
// require.resolve rather than a path built from the module's own location: the
// package can be hoisted to a workspace root, nested under this one, or left
// in a pnpm store with a symlink pointing at it, and the resolver is the only
// thing that knows which of those happened.
function packageDir(): string|null {
  const name = packageName();
  if (!name) {
    return null;
  }
  try {
    return path.dirname(require.resolve(`${name}/package.json`));
  } catch {
    return null;
  }
}

export {PACKAGES, packageDir, packageName};
