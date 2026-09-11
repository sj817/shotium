// The six engine targets, in one place.
//
// why: perf/ci.ts, ci/dispatch-engines.ts, package/platform.ts and
// publish.yml each kept their own six-row table -- public label, GN cpu,
// npm suffix, runner -- and four copies of the same table are four places
// for one of them to be wrong. Everything that needs a column reads it here.
//
// Two spellings coexist on purpose. The public one (windows-amd64) names
// Release archives, CI artifacts and job names, and is what a person types.
// The npm one (win32-x64) is process.platform-process.arch, which is what
// npm matches a platform package's `os`/`cpu` fields against.

export type EngineOS = 'windows' | 'linux' | 'macos';

export interface Platform {
  /** Public label: windows-amd64. Artifact names, archives, job names. */
  label: string;
  os: EngineOS;
  /** Public architecture spelling. */
  arch: 'amd64' | 'arm64';
  /** GN target_cpu and npm `cpu`. */
  cpu: 'x64' | 'arm64';
  /** The --os value the package scripts take. */
  packageOs: 'win' | 'linux' | 'mac';
  /** process.platform-process.arch: the suffix of @pixel.js/shotium-<npm>. */
  npm: string;
  /** Where the engine is compiled. arm64 Windows and Linux cross-compile on x64 hosts. */
  buildRunner: string;
  /** A hosted runner that can execute what was built. */
  nativeRunner: string;
  /** The reusable workflow that builds it. */
  workflow: string;
}

export const platforms: readonly Platform[] = [
  {label: 'windows-amd64', os: 'windows', arch: 'amd64', cpu: 'x64', packageOs: 'win', npm: 'win32-x64',
    buildRunner: 'windows-2025', nativeRunner: 'windows-2025', workflow: 'engine-windows.yml'},
  {label: 'windows-arm64', os: 'windows', arch: 'arm64', cpu: 'arm64', packageOs: 'win', npm: 'win32-arm64',
    buildRunner: 'windows-2025', nativeRunner: 'windows-11-arm', workflow: 'engine-windows.yml'},
  {label: 'linux-amd64', os: 'linux', arch: 'amd64', cpu: 'x64', packageOs: 'linux', npm: 'linux-x64',
    buildRunner: 'ubuntu-24.04', nativeRunner: 'ubuntu-24.04', workflow: 'engine-linux.yml'},
  {label: 'linux-arm64', os: 'linux', arch: 'arm64', cpu: 'arm64', packageOs: 'linux', npm: 'linux-arm64',
    buildRunner: 'ubuntu-24.04', nativeRunner: 'ubuntu-24.04-arm', workflow: 'engine-linux.yml'},
  {label: 'macos-amd64', os: 'macos', arch: 'amd64', cpu: 'x64', packageOs: 'mac', npm: 'darwin-x64',
    buildRunner: 'macos-15-intel', nativeRunner: 'macos-15-intel', workflow: 'engine-macos.yml'},
  {label: 'macos-arm64', os: 'macos', arch: 'arm64', cpu: 'arm64', packageOs: 'mac', npm: 'darwin-arm64',
    buildRunner: 'macos-15', nativeRunner: 'macos-15', workflow: 'engine-macos.yml'},
];

export const platformLabels: readonly string[] = platforms.map((p) => p.label);

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
