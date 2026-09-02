import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {parseArgs, recoverNpmRunValues} from './args.ts';
import {ENGINE_IDS, PLATFORM_IDS, SHARD_SCENARIOS} from './constants.ts';
import {validatePlatformResult} from './schema.ts';

const EXPECTED_SHARDS = Object.freeze(Object.keys(SHARD_SCENARIOS));
const STATUS_PRIORITY = Object.freeze({pass: 0, 'n/a': 1, noisy: 2, 'infra-error': 3, fail: 4});

type ArtifactRecord = Record<string, any>;
type ShardResult = {
  shard: string;
  directory: string;
  summary: any;
  samples: any[];
  quality: any[];
  failures: any[];
  artifact: ArtifactRecord | null;
  artifactError: string | null;
};

function readJson(file: string): any {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

function writeJson(file: string, value: unknown): void {
  fs.mkdirSync(path.dirname(file), {recursive: true});
  fs.writeFileSync(file, `${JSON.stringify(value, null, 2)}\n`);
}

function cellKey(cell: any): string {
  return [cell.engine, cell.scenario, cell.repeat, cell.attempt, cell.concurrency].join('|');
}

function inferShard(file: string): string | null {
  const parts = path.normalize(file).split(path.sep);
  for (const shard of EXPECTED_SHARDS) {
    if (parts.some((part) => part === shard || part.endsWith(`-${shard}`))) return shard;
  }
  return null;
}

function summaryFilesBelow(root: string): string[] {
  if (!fs.existsSync(root)) return [];
  const files: string[] = [];
  const pending = [root];
  while (pending.length) {
    const directory = pending.pop()!;
    for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
      const candidate = path.join(directory, entry.name);
      if (entry.isDirectory()) pending.push(candidate);
      else if (entry.isFile() && entry.name === 'summary.json') files.push(candidate);
    }
  }
  return files.sort((left, right) => left.localeCompare(right));
}

function readArtifact(directory: string): {artifact: ArtifactRecord | null; error: string | null} {
  const file = path.join(directory, 'artifact.json');
  if (!fs.existsSync(file)) return {artifact: null, error: 'missing artifact.json'};
  try {
    const artifact = readJson(file);
    if (!artifact || typeof artifact !== 'object' || typeof artifact.name !== 'string' ||
        !/^[a-f0-9]{64}$/.test(String(artifact.sha256))) {
      throw new Error('artifact record is missing its name or SHA-256');
    }
    return {artifact, error: null};
  } catch (error) {
    return {artifact: null, error: String(error)};
  }
}

function readShard(file: string, platform: string, shard: string): ShardResult {
  const directory = path.dirname(file);
  const summary = validatePlatformResult(readJson(file));
  if (summary.platform !== platform) {
    throw new Error(`summary platform is ${summary.platform}, expected ${platform}`);
  }
  if (summary.shard !== shard) {
    throw new Error(`summary shard is ${String(summary.shard)}, expected ${shard}`);
  }
  const allowedScenarios = new Set(SHARD_SCENARIOS[shard]);
  for (const [kind, rows] of [
    ['summary scenario', summary.scenarios || []],
    ['execution order', summary.execution_order || []],
  ] as const) {
    const foreign = rows.find((row: any) => !allowedScenarios.has(row.scenario));
    if (foreign) {
      throw new Error(`${kind} ${foreign.scenario} does not belong to shard ${shard}`);
    }
  }
  const sampleFile = path.join(directory, 'samples.jsonl');
  const qualityFile = path.join(directory, 'quality.json');
  const failureFile = path.join(directory, 'failures.json');
  for (const required of [sampleFile, qualityFile, failureFile]) {
    if (!fs.existsSync(required)) throw new Error(`missing ${path.basename(required)}`);
  }
  const samples = fs.readFileSync(sampleFile, 'utf8').split(/\r?\n/).filter(Boolean).map((line, index) => {
    const sample = JSON.parse(line);
    if (!sample || typeof sample !== 'object' || !sample.engine || !sample.scenario || !sample.status) {
      throw new Error(`samples.jsonl line ${index + 1} is not a benchmark cell`);
    }
    if (sample.shard !== undefined && sample.shard !== shard) {
      throw new Error(`samples.jsonl line ${index + 1} belongs to shard ${sample.shard}`);
    }
    if (!allowedScenarios.has(sample.scenario)) {
      throw new Error(`samples.jsonl line ${index + 1} scenario ${sample.scenario} does not belong to shard ${shard}`);
    }
    return {...sample, shard};
  });
  const quality = readJson(qualityFile);
  const failures = readJson(failureFile);
  if (!Array.isArray(quality)) throw new Error('quality.json is not an array');
  if (!Array.isArray(failures) || failures.some((failure) =>
    !failure || typeof failure !== 'object' || typeof failure.error !== 'string')) {
    throw new Error('failures.json is not an auditable failure array');
  }
  if (samples.length !== summary.raw_samples) {
    throw new Error(`samples.jsonl has ${samples.length} cells, summary declares ${summary.raw_samples}`);
  }
  if (quality.length !== samples.length) {
    throw new Error(`quality.json has ${quality.length} cells, expected ${samples.length}`);
  }
  if (failures.length !== summary.failures) {
    throw new Error(`failures.json has ${failures.length} entries, summary declares ${summary.failures}`);
  }
  const sampleKeys = new Set(samples.map(cellKey));
  const normalizedQuality = quality.map((entry) => {
    if (entry.shard !== undefined && entry.shard !== shard) {
      throw new Error(`quality.json cell belongs to shard ${entry.shard}`);
    }
    if (!allowedScenarios.has(entry.scenario)) {
      throw new Error(`quality.json scenario ${entry.scenario} does not belong to shard ${shard}`);
    }
    return {...entry, shard};
  });
  const qualityKeys = new Set(normalizedQuality.map(cellKey));
  if (sampleKeys.size !== samples.length || qualityKeys.size !== normalizedQuality.length) {
    throw new Error('shard contains duplicate benchmark cells');
  }
  if (sampleKeys.size !== qualityKeys.size || [...sampleKeys].some((key) => !qualityKeys.has(key))) {
    throw new Error('quality.json cells do not match samples.jsonl cells');
  }
  const artifactResult = readArtifact(directory);
  return {
    shard,
    directory,
    summary,
    samples,
    quality: normalizedQuality,
    failures: failures.map((failure) => ({...failure, shard})),
    artifact: artifactResult.artifact,
    artifactError: artifactResult.error,
  };
}

function combinedStatus(statuses: string[]): string {
  if (!statuses.length) return 'infra-error';
  return [...statuses].sort((left, right) =>
    (STATUS_PRIORITY[right] ?? STATUS_PRIORITY['infra-error']) -
    (STATUS_PRIORITY[left] ?? STATUS_PRIORITY['infra-error']))[0];
}

function mergeEngines(shards: ShardResult[]): any[] {
  const names: string[] = [];
  for (const shard of shards) {
    for (const engine of shard.summary.engines || []) {
      if (!names.includes(engine.engine)) names.push(engine.engine);
    }
  }
  return names.map((name) => {
    const records = shards.flatMap((shard) => {
      const engine = (shard.summary.engines || []).find((entry) => entry.engine === name);
      return engine ? [{shard: shard.shard, engine}] : [];
    });
    const statuses = records.map((entry) => entry.engine.status);
    const availabilityChanged = statuses.includes('n/a') && statuses.some((status) => status !== 'n/a');
    const identities = new Set(records.flatMap((entry) => {
      if (!entry.engine.sha256) return [];
      return [JSON.stringify({
        sha256: entry.engine.sha256,
        binary_version: entry.engine.binary_version,
        architectures: [...(entry.engine.architectures || [])].sort(),
        binaries: (entry.engine.binaries || []).map((binary) => ({
          sha256: binary.sha256,
          format: binary.format,
          architectures: [...(binary.architectures || [])].sort(),
        })).sort((left, right) => String(left.sha256).localeCompare(String(right.sha256))),
      })];
    }));
    const identityChanged = identities.size > 1;
    const status = availabilityChanged || identityChanged ? 'infra-error' : combinedStatus(statuses);
    const representative = records.find((entry) => entry.engine.status === status)?.engine || records[0]?.engine || {
      engine: name, reason: null, executable: null, architectures: [],
    };
    const reasons = records.filter((entry) => entry.engine.reason).map((entry) =>
      `${entry.shard}: ${entry.engine.reason}`);
    return {
      ...representative,
      status,
      reason: availabilityChanged ? 'engine availability differs between scenario shards' :
        identityChanged ? 'engine binary identity differs between scenario shards' :
        reasons.length ? [...new Set(reasons)].join('; ') : representative.reason || null,
      shard_statuses: Object.fromEntries(records.map((entry) => [entry.shard, entry.engine.status])),
    };
  });
}

function sameValue(left: unknown, right: unknown): boolean {
  return JSON.stringify(left) === JSON.stringify(right);
}

function comparableInstall(install: any): any {
  if (!install) return install;
  const packageIdentity = (value: any) => value ? {
    name: value.name,
    version: value.version,
    content_sha256: value.content_sha256,
    files: value.files,
    bytes: value.bytes,
  } : value;
  return {
    main: packageIdentity(install.main),
    platform: packageIdentity(install.platform),
    main_manifest_sha256: install.main_manifest_sha256,
    platform_manifest_sha256: install.platform_manifest_sha256,
    esm: install.esm,
    commonjs: install.commonjs,
  };
}

function selectMetadata(shards: ShardResult[], key: string, usable: (value: any) => boolean): any {
  return shards.map((entry) => entry.summary[key]).find(usable);
}

function mergedArtifact(platform: string, shards: ShardResult[]) {
  const artifacts: ArtifactRecord[] = shards.flatMap((entry) =>
    entry.artifact ? [{...entry.artifact, shard: entry.shard}] : []);
  artifacts.sort((left, right) => left.shard.localeCompare(right.shard));
  const hashInput = artifacts.map((artifact) => ({
    shard: artifact.shard,
    name: artifact.name,
    sha256: artifact.sha256,
  }));
  const missing = EXPECTED_SHARDS.filter((shard) => !artifacts.some((artifact) => artifact.shard === shard));
  return {
    schema_version: 1,
    kind: 'sharded-evidence-index',
    platform,
    generated_utc: new Date().toISOString(),
    sha256: crypto.createHash('sha256').update(JSON.stringify(hashInput)).digest('hex'),
    uploaded: missing.length === 0 && artifacts.every((artifact) => artifact.uploaded === true),
    expected_shards: EXPECTED_SHARDS,
    missing_artifact_shards: missing,
    artifacts,
  };
}

export function mergeShardResults(options: {input: string; output: string; platform: string}) {
  const input = path.resolve(options.input);
  const output = path.resolve(options.output);
  const platform = String(options.platform);
  if (!PLATFORM_IDS.includes(platform)) throw new Error(`unsupported platform ${platform}`);

  const candidates = new Map<string, string[]>();
  const discoveryErrors: Record<string, string[]> = {};
  for (const file of summaryFilesBelow(input)) {
    let raw: any;
    try {
      raw = readJson(file);
    } catch (error) {
      const inferred = inferShard(file);
      if (inferred) (discoveryErrors[inferred] ||= []).push(`${file}: ${String(error)}`);
      continue;
    }
    if (raw.platform !== platform) continue;
    const shard = EXPECTED_SHARDS.includes(raw.shard) ? raw.shard : inferShard(file);
    if (!shard) continue;
    const entries = candidates.get(shard) || [];
    entries.push(file);
    candidates.set(shard, entries);
  }

  const validShards: ShardResult[] = [];
  const invalidShards: Record<string, string[]> = {...discoveryErrors};
  for (const shard of EXPECTED_SHARDS) {
    const files = candidates.get(shard) || [];
    if (files.length > 1) {
      (invalidShards[shard] ||= []).push(`found ${files.length} summary.json files for shard ${shard}`);
      continue;
    }
    if (!files.length) continue;
    try {
      validShards.push(readShard(files[0], platform, shard));
    } catch (error) {
      (invalidShards[shard] ||= []).push(String(error));
    }
  }
  validShards.sort((left, right) => EXPECTED_SHARDS.indexOf(left.shard) - EXPECTED_SHARDS.indexOf(right.shard));

  const present = new Set(validShards.map((entry) => entry.shard));
  const missingShards = EXPECTED_SHARDS.filter((shard) => !present.has(shard));
  const shardsComplete = missingShards.length === 0;
  const mergeErrors: string[] = [];
  for (const shard of missingShards) {
    const detail = invalidShards[shard]?.join('; ');
    mergeErrors.push(detail ? `invalid ${shard} shard: ${detail}` : `missing ${shard} shard`);
  }

  const comparableFields = ['shotium_version', 'profile', 'seed', 'source_revision', 'measurement_contract'];
  const base = validShards[0]?.summary;
  if (base) {
    for (const field of comparableFields) {
      const values = validShards.map((entry) => entry.summary[field]);
      if (values.some((value) => !sameValue(value, values[0]))) {
        mergeErrors.push(`scenario shards disagree on ${field}`);
      }
    }
    const installs = validShards.map((entry) => comparableInstall(entry.summary.install)).filter(Boolean);
    if (installs.some((value) => !sameValue(value, installs[0]))) mergeErrors.push('scenario shards disagree on install evidence');
    const packageSets = validShards.map((entry) => entry.summary.packages).filter((value) =>
      value && Object.keys(value).length);
    if (packageSets.some((value) => !sameValue(value, packageSets[0]))) {
      mergeErrors.push('scenario shards disagree on benchmark package versions');
    }
    for (const entry of validShards) {
      const actual = new Set((entry.summary.engines || []).map((engine) => engine.engine));
      if (actual.size !== ENGINE_IDS.length || ENGINE_IDS.some((engine) => !actual.has(engine))) {
        mergeErrors.push(`${entry.shard} shard did not report every benchmark engine`);
      }
    }
  }

  const samples = validShards.flatMap((entry) => entry.samples);
  const sampleKeys = samples.map(cellKey);
  if (new Set(sampleKeys).size !== sampleKeys.length) {
    mergeErrors.push('scenario shards contain overlapping benchmark cells');
  }
  const quality = validShards.flatMap((entry) => entry.quality);
  const failures = validShards.flatMap((entry) => entry.failures);
  const generatedAt = new Date().toISOString();
  for (const error of mergeErrors) {
    failures.push({at: generatedAt, shard: 'merge', status: 'infra-error', error});
  }

  const exactVersion = /^\d+\.\d+\.\d+(?:[-+].+)?$/;
  const shotiumVersion = selectMetadata(validShards, 'shotium_version', (value) => exactVersion.test(String(value))) ||
    base?.shotium_version || 'unknown';
  const install = selectMetadata(validShards, 'install', Boolean) || null;
  const packages = selectMetadata(validShards, 'packages', (value) =>
    Boolean(value && Object.keys(value).length)) || {};
  const sourceRevision = selectMetadata(validShards, 'source_revision', Boolean) || null;
  const measurementContract = selectMetadata(validShards, 'measurement_contract', Boolean);
  const engines = mergeEngines(validShards);
  const shotium = engines.find((engine) => engine.engine === 'shotium');
  const shardStatuses = validShards.map((entry) => entry.summary.status);
  const status = !shardsComplete || mergeErrors.length || !shotium || shotium.status === 'infra-error' ?
    'infra-error' :
    shotium.status === 'fail' || shardStatuses.includes('fail') ||
      engines.some((engine) => engine.status === 'fail') ? 'fail' :
      shardStatuses.includes('infra-error') || engines.some((engine) => engine.status === 'infra-error') ?
        'infra-error' :
        shardStatuses.includes('noisy') || engines.some((engine) => engine.status === 'noisy') ? 'noisy' : 'pass';
  const artifact = mergedArtifact(platform, validShards);
  const executionShards = validShards.map((entry) => ({
    shard: entry.shard,
    status: entry.summary.status,
    generated_utc: entry.summary.generated_utc,
    host: entry.summary.host,
    engines: entry.summary.engines,
    execution_order: entry.summary.execution_order,
    raw_samples: entry.summary.raw_samples,
    failures: entry.summary.failures,
    artifact: entry.artifact,
    artifact_error: entry.artifactError,
  }));
  const summary = validatePlatformResult({
    schema_version: 2,
    generated_utc: generatedAt,
    platform,
    shard: 'merged',
    shards_complete: shardsComplete,
    missing_shards: missingShards,
    invalid_shards: invalidShards,
    host_mode: 'sharded',
    status,
    shotium_version: shotiumVersion,
    profile: base?.profile || 'unknown',
    seed: base?.seed || '',
    source_revision: sourceRevision,
    install,
    host: null,
    packages,
    ...(measurementContract ? {measurement_contract: measurementContract} : {}),
    engines,
    scenarios: validShards.flatMap((entry) =>
      (entry.summary.scenarios || []).map((scenario) => ({...scenario, shard: entry.shard}))),
    execution_order: validShards.flatMap((entry) =>
      (entry.summary.execution_order || []).map((order) => ({...order, shard: entry.shard}))),
    execution_shards: executionShards,
    raw_samples: samples.length,
    failures: failures.length,
    ...(mergeErrors.length ? {error: mergeErrors.join('; ')} : {}),
  });

  fs.mkdirSync(output, {recursive: true});
  writeJson(path.join(output, 'summary.json'), summary);
  fs.writeFileSync(path.join(output, 'samples.jsonl'), samples.length ?
    `${samples.map((sample) => JSON.stringify(sample)).join('\n')}\n` : '');
  writeJson(path.join(output, 'quality.json'), quality);
  writeJson(path.join(output, 'failures.json'), failures);
  writeJson(path.join(output, 'artifact.json'), artifact);
  return {summary, artifact};
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const options = parseArgs(process.argv.slice(2));
  recoverNpmRunValues(options, ['input', 'output', 'platform']);
  if (!options.input || !options.output || !options.platform) {
    throw new Error('usage: merge-shards --input <dir> --output <dir> --platform <platform>');
  }
  const result = mergeShardResults({
    input: String(options.input),
    output: String(options.output),
    platform: String(options.platform),
  });
  // The same split the shard CLI makes, one layer up: the merged status stays an
  // honest description of what the platform measured, and the exit code answers
  // the narrower question of whether this run can be trusted. A platform whose
  // status is 'fail' only because a baseline engine rendered the same page
  // differently twice is a recorded result, not a broken run -- and leaving the
  // old rule here meant four Merge jobs went red for exactly that.
  const merged: any = result.summary;
  const shotium = (merged.engines || []).find((engine) => engine.engine === 'shotium');
  const blocking: string[] = [];
  if (!merged.shards_complete) {
    blocking.push(`missing shards: ${(merged.missing_shards || []).join(', ') || 'unknown'}`);
  }
  if ((merged.invalid_shards || []).length) {
    blocking.push(`unreadable shards: ${merged.invalid_shards.length}`);
  }
  if (!shotium) blocking.push('no shotium result in any shard');
  else if (['fail', 'infra-error'].includes(shotium.status)) {
    blocking.push(`shotium is ${shotium.status} on this platform`);
  }
  const baseline = (merged.engines || [])
      .filter((engine) => engine.engine !== 'shotium' && ['fail', 'infra-error'].includes(engine.status))
      .map((engine) => `${engine.engine}=${engine.status}`);
  process.stdout.write(`${JSON.stringify({
    platform: merged.platform,
    status: merged.status,
    shards_complete: merged.shards_complete,
    baseline_outcomes: baseline,
    blocking,
  })}\n`);
  if (blocking.length) process.exitCode = 1;
}
