// The eight native engine targets, in one place.
//
// Public labels name Release archives, CI artifacts and build directories.
// npm suffixes follow process.platform/process.arch; Linux adds a suffix only
// for musl so the established glibc package names remain stable.

export type EngineOS = 'windows' | 'linux' | 'macos';
export type LinuxLibc = 'glibc' | 'musl';

export interface Platform {
  /** Public label: windows-amd64, linux-amd64-musl. */
  label: string;
  os: EngineOS;
  /** Public architecture spelling. */
  arch: 'amd64' | 'arm64';
  /** GN target_cpu and npm `cpu`. */
  cpu: 'x64' | 'arm64';
  /** The --os value the package scripts take. */
  packageOs: 'win' | 'linux' | 'mac';
  /** The suffix of @pixel.js/shotium-<npm>. */
  npm: string;
  /** Present only on Linux, where the package manager must select a libc. */
  libc?: LinuxLibc;
  /** Where the engine is compiled. Linux arm64 cross-compiles on an x64 host. */
  buildRunner: string;
  /** A hosted runner that can execute the architecture. */
  nativeRunner: string;
  /** The reusable workflow that builds it. */
  workflow: string;
}

export const platforms: readonly Platform[] = [
  {label: 'windows-amd64', os: 'windows', arch: 'amd64', cpu: 'x64', packageOs: 'win', npm: 'win32-x64',
    buildRunner: 'windows-2025', nativeRunner: 'windows-2025', workflow: 'engine-windows.yml'},
  {label: 'windows-arm64', os: 'windows', arch: 'arm64', cpu: 'arm64', packageOs: 'win', npm: 'win32-arm64',
    buildRunner: 'windows-2025', nativeRunner: 'windows-11-arm', workflow: 'engine-windows.yml'},
  {label: 'linux-amd64', os: 'linux', arch: 'amd64', cpu: 'x64', packageOs: 'linux', npm: 'linux-x64', libc: 'glibc',
    buildRunner: 'ubuntu-24.04', nativeRunner: 'ubuntu-24.04', workflow: 'engine-linux.yml'},
  {label: 'linux-arm64', os: 'linux', arch: 'arm64', cpu: 'arm64', packageOs: 'linux', npm: 'linux-arm64', libc: 'glibc',
    buildRunner: 'ubuntu-24.04', nativeRunner: 'ubuntu-24.04-arm', workflow: 'engine-linux.yml'},
  {label: 'linux-amd64-musl', os: 'linux', arch: 'amd64', cpu: 'x64', packageOs: 'linux', npm: 'linux-x64-musl', libc: 'musl',
    buildRunner: 'ubuntu-24.04', nativeRunner: 'ubuntu-24.04', workflow: 'engine-linux.yml'},
  {label: 'linux-arm64-musl', os: 'linux', arch: 'arm64', cpu: 'arm64', packageOs: 'linux', npm: 'linux-arm64-musl', libc: 'musl',
    buildRunner: 'ubuntu-24.04', nativeRunner: 'ubuntu-24.04-arm', workflow: 'engine-linux.yml'},
  {label: 'macos-amd64', os: 'macos', arch: 'amd64', cpu: 'x64', packageOs: 'mac', npm: 'darwin-x64',
    buildRunner: 'macos-15-intel', nativeRunner: 'macos-15-intel', workflow: 'engine-macos.yml'},
  {label: 'macos-arm64', os: 'macos', arch: 'arm64', cpu: 'arm64', packageOs: 'mac', npm: 'darwin-arm64',
    buildRunner: 'macos-15', nativeRunner: 'macos-15', workflow: 'engine-macos.yml'},
];

export const platformLabels: readonly string[] = platforms.map((p) => p.label);

/** v0.7.x has no musl packages, so the release performance gate stays on the six comparable targets. */
export const performancePlatforms: readonly Platform[] = platforms.filter((p) => p.libc !== 'musl');

export function platformByLabel(label: string): Platform {
  const platform = platforms.find((p) => p.label === label);
  if (!platform) throw new Error(`unknown platform ${label}; known: ${platformLabels.join(', ')}`);
  return platform;
}

/** `all`, or a comma-separated subset of labels; unknown names are an error, not a skip. */
export function selectPlatforms(targets: string | undefined): Platform[] {
  if (!targets || targets.trim() === 'all') return [...platforms];
  return targets.split(',').map((s) => s.trim()).filter(Boolean).map(platformByLabel);
}
