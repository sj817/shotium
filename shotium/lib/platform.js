'use strict';

const path = require('path');

// Which package carries the engine for this machine.
//
// The engine is not in this package and cannot be: it is a Chromium build,
// 41 MB per platform and architecture, six of them, and `npm install` is never
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
// The keys are node's `${process.platform}-${process.arch}`. The names are the
// release archives' spelling -- win/mac/linux -- because a user who downloads
// `shotium-mac-arm64-v0.1.0.7z` by hand and a user who installs
// `@shotkit/shotium-mac-arm64` should not have to work out that they got the
// same thing.
const PACKAGES = {
  'win32-x64': '@shotkit/shotium-win-x64',
  'win32-arm64': '@shotkit/shotium-win-arm64',
  'darwin-x64': '@shotkit/shotium-mac-x64',
  'darwin-arm64': '@shotkit/shotium-mac-arm64',
  'linux-x64': '@shotkit/shotium-linux-x64',
  'linux-arm64': '@shotkit/shotium-linux-arm64',
};

function packageName(platform = process.platform, arch = process.arch) {
  return PACKAGES[`${platform}-${arch}`] || null;
}

// Where the matching platform package unpacked, or null if it is not installed.
//
// require.resolve rather than a path built from __dirname: the package can be
// hoisted to a workspace root, nested under this one, or left in a pnpm store
// with a symlink pointing at it, and the resolver is the only thing that knows
// which of those happened.
function packageDir() {
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

// What the engine executable is called, which is not what the platform calls
// it: Windows wants the extension and nothing else does.
function binaryName() {
  return process.platform === 'win32' ? 'shotium.exe' : 'shotium';
}

module.exports = {PACKAGES, binaryName, packageDir, packageName};
