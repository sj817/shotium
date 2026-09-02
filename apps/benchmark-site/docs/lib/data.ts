import {
  PLATFORM_IDS,
  type BenchmarkIndex,
  type BenchmarkManifest,
  type FailureRecord,
  type IndexEntry,
  type LoadedRun,
  type PlatformData,
  type PlatformId,
  type PlatformSummary,
} from './types';

/*
 * Every fetch is relative to the site base (VitePress `site.base`, e.g.
 * `/shotium/`) resolved against document.baseURI, so `/` and `/en/` read the
 * same archive: `<base>benchmark-results/index.json`, `<run>/manifest.json`,
 * `<run>/<platform>/summary.json` and `<run>/<platform>/failures.json`.
 */
export function resultsUrl(base: string, relativePath: string): string {
  const root = base.endsWith('/') ? base : `${base}/`;
  const normalized = relativePath.replace(/^\/+/, '');
  return new URL(`${root}benchmark-results/${normalized}`, document.baseURI).toString();
}

export function isPublishableEntry(entry: IndexEntry): boolean {
  return entry.publishable ?? (entry.status === 'complete' && ['pass', 'noisy'].includes(entry.quality_status) &&
    entry.evidence_status === 'complete');
}

async function fetchJson<T>(base: string, relativePath: string): Promise<T> {
  const response = await fetch(resultsUrl(base, relativePath), {cache: 'no-cache'});
  if (!response.ok) throw new Error(`${response.status} ${response.statusText}`);
  return response.json() as Promise<T>;
}

export async function loadIndex(base: string): Promise<BenchmarkIndex> {
  const index = await fetchJson<BenchmarkIndex>(base, 'index.json');
  if (!Array.isArray(index.results)) throw new Error('benchmark-results/index.json has no results array');
  return index;
}

async function loadPlatform(base: string, runPath: string, id: PlatformId): Promise<PlatformData> {
  try {
    const [summary, failures] = await Promise.all([
      fetchJson<PlatformSummary>(base, `${runPath}/${id}/summary.json`),
      fetchJson<FailureRecord[]>(base, `${runPath}/${id}/failures.json`).catch(() => []),
    ]);
    if (summary.platform !== id) throw new Error(`summary platform is ${summary.platform}`);
    return {id, summary, failures: Array.isArray(failures) ? failures : [], loadError: null};
  } catch (error) {
    return {id, summary: null, failures: [], loadError: error instanceof Error ? error.message : String(error)};
  }
}

export async function loadRun(base: string, entry: IndexEntry): Promise<LoadedRun> {
  const manifest = await fetchJson<BenchmarkManifest>(base, `${entry.path}/manifest.json`);
  const loaded = await Promise.all(PLATFORM_IDS.map((id) => loadPlatform(base, entry.path, id)));
  const platforms = Object.fromEntries(loaded.map((platform) => [platform.id, platform])) as
    Record<PlatformId, PlatformData>;
  return {entry, manifest, platforms};
}
