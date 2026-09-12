// Acceptance policy is separate from the timing estimator. A release of the
// same runtime can prove byte identity and complete execution without claiming
// that identical code became faster. The improvement gate remains the default.
import {createHash} from 'node:crypto';
import {readdirSync, readFileSync, realpathSync} from 'node:fs';
import {createRequire} from 'node:module';
import path from 'node:path';

export type AcceptanceMode = 'improvement' | 'identical-runtime';
export const RUNTIME_IDENTITY_POLICY = 'runtime-identity-v1';
export function acceptanceMode(value: unknown): AcceptanceMode {
  if (value === undefined || value === 'improvement') return 'improvement';
  if (value === 'identical-runtime') return value;
  throw new Error('--acceptance must be improvement or identical-runtime');
}

interface RuntimeFile {path: string; bytes: number; sha256: string}
export interface RuntimeSnapshot {
  schema: 1;
  platform: string;
  packageVersion: string;
  packageDirectory: string;
  platformDirectory: string;
  files: RuntimeFile[];
  sha256: string;
}
export interface LoadedRuntime {
  packageVersion: string;
  addon: string;
  library: string;
  resourceDirectory: string;
  addonSha256: string;
  librarySha256: string;
  bundleSha256: string;
}
export interface AcceptanceCase {
  name: string;
  class?: string;
  status: string;
  accepted?: boolean;
  summary?: {baseline: {p50: number; p95: number; mean: number}; candidate: {p50: number; p95: number; mean: number}};
  metrics?: Record<string, {samples?: number}>;
}
export interface AcceptanceResult {
  acceptanceMode?: AcceptanceMode;
  acceptancePolicy?: string;
  platform: string;
  arch: string;
  complete?: boolean;
  shard?: string;
  requiredCases: string[];
  cases: AcceptanceCase[];
  sampling?: {minimumPairs?: number};
  metadata?: Record<string, LoadedRuntime>;
  runtimeIdentity?: Record<string, {before?: RuntimeSnapshot; after?: RuntimeSnapshot}>;
}

const hash = (bytes: Buffer | string) => createHash('sha256').update(bytes).digest('hex');
const digest = (snapshot: Omit<RuntimeSnapshot, 'sha256'>) => hash(JSON.stringify({
  schema: snapshot.schema, platform: snapshot.platform, packageVersion: snapshot.packageVersion, files: snapshot.files,
}));
const REQUIRED = ['package/package.json', 'package/dist/index.js', 'platform/package.json',
  'platform/shotium.node', 'platform/shotium_data.pak', 'platform/shotium_strings.pak'];

/** Hash the package manifests, every dist file and the entire native package.
 * Source/docs in the main package are not executed; empty runtime-created
 * directories are ignored. Inner symlinks are rejected, not followed outside
 * the package. Hashing happens outside all measurement intervals. */
export function snapshotRuntime(packagePath: string): RuntimeSnapshot {
  const packageDirectory = realpathSync(packagePath);
  const packageFile = path.join(packageDirectory, 'package.json');
  const manifest = JSON.parse(readFileSync(packageFile, 'utf8'));
  const platform = `${process.platform}-${process.arch}`;
  const name = `@pixel.js/shotium-${platform}`;
  const platformDirectory = realpathSync(path.dirname(createRequire(packageFile).resolve(`${name}/package.json`)));
  const native = JSON.parse(readFileSync(path.join(platformDirectory, 'package.json'), 'utf8'));
  if (manifest.name !== '@pixel.js/shotium' || !manifest.version || native.name !== name ||
      native.version !== manifest.version || manifest.optionalDependencies?.[name] !== manifest.version) {
    throw new Error('Runtime identity requires matching primary and native package manifests with an exact version pin');
  }
  const files: RuntimeFile[] = [];
  const add = (absolute: string, relative: string) => {
    const bytes = readFileSync(absolute);
    files.push({path: relative, bytes: bytes.length, sha256: hash(bytes)});
  };
  const walk = (directory: string, prefix: string) => {
    for (const entry of readdirSync(directory, {withFileTypes: true})) {
      const absolute = path.join(directory, entry.name), relative = `${prefix}/${entry.name}`;
      if (entry.isDirectory()) walk(absolute, relative);
      else if (entry.isFile()) add(absolute, relative);
      else throw new Error(`Runtime identity does not allow symlinks or special files: ${absolute}`);
    }
  };
  add(packageFile, 'package/package.json');
  walk(path.join(packageDirectory, 'dist'), 'package/dist');
  walk(platformDirectory, 'platform');
  files.sort((a, b) => a.path < b.path ? -1 : a.path > b.path ? 1 : 0);
  for (const required of REQUIRED) if (!files.some((f) => f.path === required)) throw new Error(`Missing runtime file: ${required}`);
  const snapshot = {schema: 1 as const, platform, packageVersion: manifest.version as string, packageDirectory, platformDirectory, files};
  return {...snapshot, sha256: digest(snapshot)};
}

function validateSnapshot(snapshot: RuntimeSnapshot, platform: string): void {
  const names = snapshot.files.map((f) => f.path);
  if (snapshot.schema !== 1 || snapshot.platform !== platform || !snapshot.packageVersion ||
      !snapshot.packageDirectory || !snapshot.platformDirectory || snapshot.sha256 !== digest(snapshot) ||
      new Set(names).size !== names.length || names.join('\n') !== [...names].sort().join('\n') ||
      snapshot.files.some((f) => !/^(package|platform)\//.test(f.path) || /(^|\/)\.\.?($|\/)|\\/.test(f.path) ||
          !Number.isSafeInteger(f.bytes) || f.bytes < 0 || !/^[0-9a-f]{64}$/.test(f.sha256)) ||
      REQUIRED.some((file) => !names.includes(file))) {
    throw new Error('Invalid or incomplete runtime snapshot');
  }
}

/** Bind each worker's actual loaded addon and resource root to the snapshot. */
export function assertLoadedRuntime(snapshot: RuntimeSnapshot, metadata: LoadedRuntime): void {
  const canonical = (value: string) => {
    const normalized = String(value ?? '').replace(/\\/g, '/').replace(/\/$/, '');
    return snapshot.platform.startsWith('win32-') ? normalized.toLowerCase() : normalized;
  };
  const native = snapshot.files.find((f) => f.path === 'platform/shotium.node')!.sha256;
  const bundle = snapshot.files.find((f) => f.path === 'package/dist/index.js')!.sha256;
  const addon = `${canonical(snapshot.platformDirectory)}/shotium.node`;
  if (metadata.packageVersion !== snapshot.packageVersion || metadata.addonSha256 !== native ||
      metadata.librarySha256 !== native || metadata.bundleSha256 !== bundle ||
      canonical(metadata.addon) !== addon || canonical(metadata.library) !== addon ||
      canonical(metadata.resourceDirectory) !== canonical(snapshot.platformDirectory)) {
    throw new Error('Loaded runtime does not match the package snapshot');
  }
}

/** Returns reasons for refusal; never rewrites statistical verdicts/accepted. */
export function assessAcceptance(data: AcceptanceResult, mode: AcceptanceMode): string[] {
  const issues: string[] = [];
  if (acceptanceMode(data.acceptanceMode) !== mode) issues.push('Result acceptance mode does not match the requested gate');
  const required = new Set(data.requiredCases), observed = new Set(data.cases.map((c) => c.name));
  if (!data.complete || data.shard !== 'all' || !required.size || required.size !== data.requiredCases.length ||
      observed.size !== required.size || data.cases.length !== required.size || [...required].some((r) => !observed.has(r))) {
    issues.push('The complete case matrix is required');
  }
  if (mode === 'improvement') {
    if (data.cases.some((c) => !c.accepted || !(c.class === 'external' ? ['faster', 'equivalent'] : ['faster']).includes(c.status))) {
      issues.push('Every case must meet the improvement gate');
    }
    return issues;
  }
  if (data.acceptancePolicy !== RUNTIME_IDENTITY_POLICY) issues.push('Result was not collected under the runtime identity policy');
  const minimum = data.sampling?.minimumPairs;
  if (!Number.isInteger(minimum) || minimum! < 10 || data.cases.some((c) =>
    !['faster', 'equivalent', 'unproven', 'slower'].includes(c.status) || !Number.isInteger(c.metrics?.wall?.samples) || c.metrics!.wall.samples! < minimum! ||
    !c.summary || [c.summary.baseline, c.summary.candidate].some((s) => !s || [s.p50, s.p95, s.mean].some((n) => !Number.isFinite(n) || n < 0)))) {
    issues.push('Every case must finish sampling without runtime errors');
  }
  const snapshots: RuntimeSnapshot[] = [];
  for (const label of ['baseline', 'candidate']) {
    try {
      for (const when of ['before', 'after'] as const) {
        const snapshot = data.runtimeIdentity?.[label]?.[when];
        if (!snapshot) throw new Error(`Missing ${when} snapshot`);
        validateSnapshot(snapshot, `${data.platform}-${data.arch}`);
        snapshots.push(snapshot);
        if (!data.metadata?.[label]) throw new Error('Missing loaded runtime metadata');
        assertLoadedRuntime(snapshot, data.metadata[label]);
      }
    } catch (error) {
      issues.push(`${label}: ${(error as Error).message}`);
    }
  }
  if (snapshots.length !== 4 || new Set(snapshots.map((s) => s.sha256)).size !== 1) {
    issues.push('Baseline and candidate runtime files must be identical before and after measurement');
  }
  return issues;
}
