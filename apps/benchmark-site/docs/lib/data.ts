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

function resultsUrl(relativePath: string): string {
  const normalized = relativePath.replace(/^\/+/, '');
  return new URL(`benchmark-results/${normalized}`, document.baseURI).toString();
}

async function fetchJson<T>(relativePath: string): Promise<T> {
  const response = await fetch(resultsUrl(relativePath), {cache: 'no-cache'});
  if (!response.ok) throw new Error(`${response.status} ${response.statusText}`);
  return response.json() as Promise<T>;
}

export async function loadIndex(): Promise<BenchmarkIndex> {
  const index = await fetchJson<BenchmarkIndex>('index.json');
  if (!Array.isArray(index.results)) throw new Error('benchmark-results/index.json has no results array');
  return index;
}

async function loadPlatform(runPath: string, id: PlatformId): Promise<PlatformData> {
  try {
    const [summary, failures] = await Promise.all([
      fetchJson<PlatformSummary>(`${runPath}/${id}/summary.json`),
      fetchJson<FailureRecord[]>(`${runPath}/${id}/failures.json`).catch(() => []),
    ]);
    if (summary.platform !== id) throw new Error(`summary platform is ${summary.platform}`);
    return {id, summary, failures: Array.isArray(failures) ? failures : [], loadError: null};
  } catch (error) {
    return {id, summary: null, failures: [], loadError: error instanceof Error ? error.message : String(error)};
  }
}

export async function loadRun(entry: IndexEntry): Promise<LoadedRun> {
  const manifest = await fetchJson<BenchmarkManifest>(`${entry.path}/manifest.json`);
  const loaded = await Promise.all(PLATFORM_IDS.map((id) => loadPlatform(entry.path, id)));
  const platforms = Object.fromEntries(loaded.map((platform) => [platform.id, platform])) as
    Record<PlatformId, PlatformData>;
  return {entry, manifest, platforms};
}
