import {createRequire} from 'node:module';
import path from 'node:path';

const require = createRequire(import.meta.url);

type LinuxLibc = 'glibc'|'musl';
type ReportLike = {header?: {glibcVersionRuntime?: unknown}};

// Linux package managers need libc as well as OS and architecture. Node's
// diagnostic report exposes the running glibc version on glibc and omits it on
// musl. This is synchronous, built in, and adds no install-time code or runtime
// dependency.
function detectLinuxLibc(report: unknown = process.report?.getReport?.()): LinuxLibc {
  const version = (report as ReportLike|undefined)?.header?.glibcVersionRuntime;
  return typeof version === 'string' && version.length > 0 ? 'glibc' : 'musl';
}

const PACKAGES: Readonly<Record<string, string>> = {
  'win32-x64': '@pixel.js/shotium-win32-x64',
  'win32-arm64': '@pixel.js/shotium-win32-arm64',
  'darwin-x64': '@pixel.js/shotium-darwin-x64',
  'darwin-arm64': '@pixel.js/shotium-darwin-arm64',
  'linux-x64-glibc': '@pixel.js/shotium-linux-x64',
  'linux-arm64-glibc': '@pixel.js/shotium-linux-arm64',
  'linux-x64-musl': '@pixel.js/shotium-linux-x64-musl',
  'linux-arm64-musl': '@pixel.js/shotium-linux-arm64-musl',
};

function packageName(
    platform: string = process.platform,
    arch: string = process.arch,
    libc?: LinuxLibc): string|null {
  const key = platform === 'linux'
    ? `${platform}-${arch}-${libc ?? detectLinuxLibc()}`
    : `${platform}-${arch}`;
  return PACKAGES[key] ?? null;
}

function packageDir(): string|null {
  const name = packageName();
  if (!name) return null;
  try {
    return path.dirname(require.resolve(`${name}/package.json`));
  } catch {
    return null;
  }
}

export {detectLinuxLibc, PACKAGES, packageDir, packageName};
