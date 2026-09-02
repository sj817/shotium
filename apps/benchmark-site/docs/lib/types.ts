export const PLATFORM_IDS = [
  'linux-x64',
  'linux-arm64',
  'win32-x64',
  'win32-arm64',
  'darwin-x64',
  'darwin-arm64',
] as const;

export type PlatformId = typeof PLATFORM_IDS[number];
export type ResultStatus = 'pass' | 'fail' | 'noisy' | 'n/a' | 'infra-error';

export interface IndexEntry {
  path: string;
  shotium_version: string;
  generated_utc: string;
  status: 'complete' | 'incomplete';
  quality_status: 'pass' | 'fail' | 'noisy' | string;
  evidence_status: 'complete' | 'incomplete' | string;
  publishable?: boolean;
  run_id: string;
  source_sha: string | null;
}

export interface BenchmarkIndex {
  schema_version: number;
  generated_utc: string;
  results: IndexEntry[];
}

export interface ManifestPlatform {
  platform: PlatformId;
  missing: boolean;
  status: ResultStatus;
  quality_status?: ResultStatus;
  shards_complete?: boolean;
  evidence_complete?: boolean;
  artifact_error?: string | null;
}

export interface BenchmarkManifest {
  schema_version: number;
  generated_utc: string;
  status: 'complete' | 'incomplete';
  quality_status: string;
  evidence_status: string;
  publishable?: boolean;
  shotium_version: string;
  profile: string;
  seed: string;
  run_id: string;
  run_attempt: number;
  run_url: string | null;
  source_sha: string | null;
  platforms: ManifestPlatform[];
}

export interface Distribution {
  n: number;
  min: number;
  p50: number;
  p95: number;
  max: number;
  mean: number;
  mad: number;
}

export interface EngineSummary {
  engine: string;
  status: ResultStatus;
  reason: string | null;
  executable: string | null;
  architectures: string[];
  binary_version?: string | null;
  shard_statuses?: Record<string, ResultStatus>;
}

export interface ScenarioSummary {
  shard?: string;
  engine: string;
  scenario: string;
  concurrency: number;
  status: Exclude<ResultStatus, 'n/a'>;
  ranking_eligible: boolean;
  runs: number;
  shots: number;
  wall_time_ms: Distribution | null;
  orchestration_wall_time_ms: Distribution | null;
  latency_ms: Distribution | null;
  peak_rss_bytes: Distribution | null;
  rss_slope_bytes_per_minute: Distribution | null;
  throughput_per_second: number | null;
  failure_rate: number | null;
}

export interface PlatformSummary {
  schema_version: number;
  generated_utc: string;
  platform: PlatformId;
  status: ResultStatus;
  shotium_version: string;
  profile: string;
  seed: string;
  source_revision: string | null;
  shards_complete?: boolean;
  missing_shards?: string[];
  error?: string;
  engines: EngineSummary[];
  scenarios: ScenarioSummary[];
  raw_samples: number;
  failures: number;
}

export interface FailureRecord {
  at?: string;
  shard?: string;
  status?: string;
  engine?: string;
  scenario?: string;
  repeat?: number;
  attempt?: number;
  concurrency?: number;
  error: string;
}

export interface PlatformData {
  id: PlatformId;
  summary: PlatformSummary | null;
  failures: FailureRecord[];
  loadError: string | null;
}

export interface LoadedRun {
  entry: IndexEntry;
  manifest: BenchmarkManifest;
  platforms: Record<PlatformId, PlatformData>;
}
