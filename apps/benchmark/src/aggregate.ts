import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {parseArgs, booleanArg, recoverNpmRunValues} from './args.ts';
import {APP_ROOT, PLATFORM_IDS, RESULT_STATUSES, SHARD_SCENARIOS} from './constants.ts';
import {isPublishableResult, renderLatest, renderReport, renderSummaryCsv} from './report.ts';
import {validatePlatformResult} from './schema.ts';

const PERMANENT_FILES = ['summary.json', 'samples.jsonl', 'quality.json', 'failures.json'] as const;

function readJson(file: string): any {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

function writeJson(file: string, value: unknown): void {
  fs.mkdirSync(path.dirname(file), {recursive: true});
  fs.writeFileSync(file, `${JSON.stringify(value, null, 2)}\n`);
}

function sha256(file: string): string {
  return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
}

function validatePermanentFiles(source: string) {
  const files: Record<string, string> = {};
  for (const name of PERMANENT_FILES) {
    const file = path.join(source, name);
    if (!fs.existsSync(file)) throw new Error(`missing ${name}`);
    files[name] = sha256(file);
  }
  const summary = validatePlatformResult(readJson(path.join(source, 'summary.json')));
  const sampleText = fs.readFileSync(path.join(source, 'samples.jsonl'), 'utf8');
  const sampleKeys = new Set<string>();
  let sampleCount = 0;
  for (const [index, line] of sampleText.split(/\r?\n/).entries()) {
    if (!line.trim()) continue;
    const sample = JSON.parse(line);
    if (!sample || typeof sample !== 'object' ||
        !sample.engine || !sample.scenario || !sample.status) {
      throw new Error(`samples.jsonl line ${index + 1} is not a benchmark cell`);
    }
    const key = [sample.engine, sample.scenario, sample.repeat, sample.attempt, sample.concurrency].join('|');
    if (sampleKeys.has(key)) throw new Error(`samples.jsonl has duplicate cell ${key}`);
    sampleKeys.add(key);
    sampleCount += 1;
  }
  const quality = readJson(path.join(source, 'quality.json'));
  const failures = readJson(path.join(source, 'failures.json'));
  if (!Array.isArray(quality)) throw new Error('quality.json is not an array');
  if (!Array.isArray(failures) || failures.some((failure) =>
    !failure || typeof failure !== 'object' || typeof failure.error !== 'string')) {
    throw new Error('failures.json is not an auditable failure array');
  }
  if (sampleCount !== summary.raw_samples) {
    throw new Error(`samples.jsonl has ${sampleCount} cells, summary declares ${summary.raw_samples}`);
  }
  if (quality.length !== sampleCount) {
    throw new Error(`quality.json has ${quality.length} cells, expected ${sampleCount}`);
  }
  const qualityKeys = new Set(quality.map((entry) =>
    [entry.engine, entry.scenario, entry.repeat, entry.attempt, entry.concurrency].join('|')));
  if (quality.length && qualityKeys.size !== quality.length) {
    throw new Error('quality.json contains duplicate cell records');
  }
  if (sampleKeys.size !== qualityKeys.size ||
      [...sampleKeys].some((key) => !qualityKeys.has(key))) {
    throw new Error('quality.json cells do not match samples.jsonl cells');
  }
  if (failures.length !== summary.failures) {
    throw new Error(`failures.json has ${failures.length} entries, summary declares ${summary.failures}`);
  }
  return files;
}

export function formatRunTimestamp(date: Date): string {
  if (!Number.isFinite(date.getTime())) throw new Error('invalid aggregate timestamp');
  return date.toISOString().replace(/[-:]/g, '').replace(/\.\d{3}Z$/, 'Z');
}

function platformQualityStatus(summary: any): 'pass' | 'noisy' | 'fail' {
  const engines = Array.isArray(summary?.engines) ? summary.engines : [];
  const shotium = engines.find((engine) => engine.engine === 'shotium');
  if (!shotium || ['fail', 'infra-error', 'n/a'].includes(shotium.status) ||
      summary?.status === 'infra-error' || summary?.error ||
      (summary?.invalid_shards && Object.keys(summary.invalid_shards).length)) {
    return 'fail';
  }
  // A competitor timeout, crash or bad image is a measured product outcome,
  // not proof that the harness is untrustworthy. Keep those rows as failures
  // and exclude them from paired rankings without suppressing every passing
  // comparison on the platform.
  if (engines.some((engine) => engine.status === 'infra-error')) return 'fail';
  if (shotium.status === 'noisy' || engines.some((engine) => engine.status === 'noisy')) return 'noisy';
  return 'pass';
}

function findSummaries(root: string): Map<string, string> {
  const found = new Map<string, string>();
  if (!fs.existsSync(root)) return found;
  const pending = [root];
  while (pending.length) {
    const directory = pending.pop()!;
    for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
      const candidate = path.join(directory, entry.name);
      if (entry.isDirectory()) pending.push(candidate);
      else if (entry.name === 'summary.json') {
        try {
          const summary = validatePlatformResult(readJson(candidate));
          if (PLATFORM_IDS.includes(summary.platform)) found.set(summary.platform, candidate);
        } catch {
          // Malformed platform output is represented as missing in the manifest.
        }
      }
    }
  }
  return found;
}

function rebuildIndex(resultsRoot: string): any[] {
  const entries: any[] = [];
  if (!fs.existsSync(resultsRoot)) return entries;
  for (const versionEntry of fs.readdirSync(resultsRoot, {withFileTypes: true})) {
    if (!versionEntry.isDirectory() || versionEntry.name === 'legacy') continue;
    const versionRoot = path.join(resultsRoot, versionEntry.name);
    for (const runEntry of fs.readdirSync(versionRoot, {withFileTypes: true})) {
      if (!runEntry.isDirectory()) continue;
      const manifestFile = path.join(versionRoot, runEntry.name, 'manifest.json');
      if (!fs.existsSync(manifestFile)) continue;
      const value = readJson(manifestFile);
      entries.push({
        path: `${versionEntry.name}/${runEntry.name}`,
        shotium_version: value.shotium_version,
        generated_utc: value.generated_utc,
        status: value.status,
        quality_status: value.quality_status,
        evidence_status: value.evidence_status || 'unknown',
        publishable: isPublishableResult(value),
        run_id: value.run_id,
        source_sha: value.source_sha,
      });
    }
  }
  return entries.sort((left, right) => String(right.generated_utc).localeCompare(String(left.generated_utc)));
}

export function aggregateResults(options: Record<string, any>) {
  const input = path.resolve(options.input);
  const resultsRoot = path.resolve(options.resultsRoot || path.join(APP_ROOT, '..', '..', 'benchmark-results'));
  const summaries = findSummaries(input);
  const summaryValues = [...summaries.values()].map((file) => readJson(file));
  const exactVersion = /^\d+\.\d+\.\d+(?:[-+].+)?$/;
  const nonExactProducts = summaryValues.filter((summary) =>
    !exactVersion.test(String(summary.shotium_version)) && summary.status !== 'infra-error');
  if (nonExactProducts.length) {
    throw new Error(`non-infrastructure platform output did not record an exact Shotium version: ${
      nonExactProducts.map((summary) => `${summary.platform}=${summary.shotium_version}`).join(', ')}`);
  }
  const publishedVersions = [...new Set(summaryValues
      .map((summary) => String(summary.shotium_version))
      .filter((version) => exactVersion.test(version)))];
  if (publishedVersions.length > 1) {
    throw new Error(`platforms tested different Shotium versions: ${publishedVersions.join(', ')}`);
  }
  const requestedVersion = String(options.shotiumVersion).replace(/^v/, '');
  const safeRequestedVersion = requestedVersion.startsWith('unresolved-') ? requestedVersion :
    `unresolved-${requestedVersion.replace(/[^A-Za-z0-9._-]/g, '-') || 'unknown'}`;
  const version = publishedVersions[0] ||
    (exactVersion.test(requestedVersion) ? requestedVersion : safeRequestedVersion);
  if (exactVersion.test(requestedVersion) && publishedVersions[0] &&
      publishedVersions[0] !== requestedVersion) {
    throw new Error(`requested Shotium ${requestedVersion}, platform output is ${publishedVersions[0]}`);
  }
  const generated = options.timestamp ? new Date(String(options.timestamp)) : new Date();
  const timestamp = formatRunTimestamp(generated);
  const runId = String(options.runId || 'local');
  const runAttempt = String(options.runAttempt || '1');
  const runName = `${timestamp}-gh${runId}-a${runAttempt}`;
  const destination = path.join(resultsRoot, `v${version}`, runName);
  if (fs.existsSync(destination) && !booleanArg(options.allowExisting, false)) {
    throw new Error(`result directory already exists: ${destination}`);
  }
  fs.mkdirSync(destination, {recursive: true});

  const platforms: any[] = [];
  const platformRecords: any[] = [];
  for (const platform of PLATFORM_IDS) {
    const summaryFile = summaries.get(platform);
    if (!summaryFile) {
      platformRecords.push({platform, missing: true, status: 'infra-error'});
      continue;
    }
    const source = path.dirname(summaryFile);
    let permanentHashes;
    try {
      permanentHashes = validatePermanentFiles(source);
    } catch (error) {
      platformRecords.push({
        platform,
        missing: true,
        status: 'infra-error',
        reason: `invalid permanent result: ${error}`,
      });
      continue;
    }
    const target = path.join(destination, platform);
    fs.mkdirSync(target, {recursive: true});
    for (const name of PERMANENT_FILES) {
      const file = path.join(source, name);
      if (!fs.existsSync(file)) throw new Error(`${platform} is missing ${name}`);
      fs.copyFileSync(file, path.join(target, name));
    }
    const summary = validatePlatformResult(readJson(summaryFile));
    platforms.push(summary);
    const artifactFile = path.join(source, 'artifact.json');
    let artifact = null;
    let artifacts = null;
    let artifactError = null;
    let evidenceCompleteForPlatform = false;
    if (fs.existsSync(artifactFile)) {
      try {
        const record = readJson(artifactFile);
        if (Array.isArray(record.artifacts)) {
          if (!/^[a-f0-9]{64}$/.test(String(record.sha256))) {
            throw new Error('sharded artifact index is missing its SHA-256');
          }
          const expectedShards = Object.keys(SHARD_SCENARIOS);
          if (record.artifacts.some((entry) => !entry || typeof entry !== 'object' ||
              !expectedShards.includes(entry.shard) || !entry.name ||
              !/^[a-f0-9]{64}$/.test(String(entry.sha256)))) {
            throw new Error('sharded artifact index contains an invalid artifact');
          }
          const shards = new Set(record.artifacts.map((entry) => entry.shard));
          if (shards.size !== record.artifacts.length) {
            throw new Error('sharded artifact index contains duplicate shards');
          }
          artifacts = record.artifacts.map((entry) => ({
            shard: entry.shard,
            name: entry.name,
            sha256: entry.sha256,
            run_url: entry.run_url,
            uploaded: entry.uploaded === true,
            actions_artifact_id: entry.actions_artifact_id || null,
            actions_artifact_url: entry.actions_artifact_url || null,
            actions_artifact_digest: entry.actions_artifact_digest || null,
          }));
          artifact = {kind: record.kind, sha256: record.sha256, uploaded: record.uploaded === true};
          evidenceCompleteForPlatform = record.uploaded === true &&
            artifacts.length === expectedShards.length && expectedShards.every((shard) => shards.has(shard)) &&
            artifacts.every((entry) => entry.uploaded === true);
        } else {
          artifact = record;
          if (!artifact.name || !/^[a-f0-9]{64}$/.test(String(artifact.sha256))) {
            throw new Error('artifact record is missing its name or SHA-256');
          }
          evidenceCompleteForPlatform = artifact.uploaded === true;
        }
      } catch (error) {
        artifact = null;
        artifacts = null;
        artifactError = String(error);
      }
    }
    platformRecords.push({
      platform,
      missing: false,
      status: summary.status,
      quality_status: platformQualityStatus(summary),
      shards_complete: summary.shards_complete !== false,
      summary_sha256: sha256(summaryFile),
      permanent_sha256: permanentHashes,
      artifact_error: artifactError,
      evidence_complete: evidenceCompleteForPlatform,
      artifacts,
      artifact: artifact?.name ? {
        name: artifact.name,
        sha256: artifact.sha256,
        run_url: artifact.run_url,
        uploaded: artifact.uploaded === true,
        actions_artifact_id: artifact.actions_artifact_id || null,
        actions_artifact_url: artifact.actions_artifact_url || null,
        actions_artifact_digest: artifact.actions_artifact_digest || null,
      } : artifact,
    });
  }

  const complete = platformRecords.every((entry) =>
    !entry.missing && entry.shards_complete !== false && RESULT_STATUSES.includes(entry.status));
  const evidenceComplete = platformRecords.every((entry) => !entry.missing && entry.evidence_complete === true);
  const qualityStatus = !complete || !evidenceComplete ||
    platformRecords.some((entry) => entry.quality_status === 'fail') ? 'fail' :
    platformRecords.some((entry) => entry.quality_status === 'noisy') ? 'noisy' : 'pass';
  const manifest = {
    schema_version: 1,
    generated_utc: generated.toISOString(),
    status: complete ? 'complete' : 'incomplete',
    quality_status: qualityStatus,
    evidence_status: evidenceComplete ? 'complete' : 'incomplete',
    publishable: complete && qualityStatus !== 'fail' && evidenceComplete,
    shotium_version: version,
    profile: String(options.profile || platforms[0]?.profile || 'unknown'),
    seed: String(options.seed || platforms[0]?.seed || ''),
    run_id: runId,
    run_attempt: Number(runAttempt),
    run_url: options.runUrl ? String(options.runUrl) : null,
    source_sha: options.sourceSha ? String(options.sourceSha) : platforms[0]?.source_revision || null,
    platforms: platformRecords,
  };
  writeJson(path.join(destination, 'manifest.json'), manifest);
  fs.writeFileSync(path.join(destination, 'report.md'), renderReport(platforms, manifest, 'en'));
  fs.writeFileSync(path.join(destination, 'report.zh-CN.md'), renderReport(platforms, manifest, 'zh-CN'));
  fs.writeFileSync(path.join(destination, 'summary.csv'), renderSummaryCsv(platforms, manifest));

  const index = rebuildIndex(resultsRoot);
  writeJson(path.join(resultsRoot, 'index.json'), {schema_version: 1, generated_utc: new Date().toISOString(), results: index});
  const latest = index.find(isPublishableResult);
  fs.writeFileSync(path.join(resultsRoot, 'LATEST.md'), renderLatest(latest));

  if (options.githubOutput) {
    fs.appendFileSync(String(options.githubOutput), `result_directory=${destination.replaceAll('\\', '/')}\n` +
      `result_name=${runName}\nversion=${version}\nmanifest_status=${manifest.status}\n` +
      `quality_status=${manifest.quality_status}\nevidence_status=${manifest.evidence_status}\n` +
      `publishable=${manifest.publishable}\n`);
  }
  return {destination, manifest};
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const options = parseArgs(process.argv.slice(2));
  recoverNpmRunValues(options, [
    'input', 'resultsRoot', 'shotiumVersion', 'profile', 'seed', 'timestamp', 'runId',
    'runAttempt', 'runUrl', 'sourceSha', 'githubOutput', 'allowExisting',
  ]);
  if (!options.input || !options.shotiumVersion) {
    throw new Error('usage: aggregate --input <dir> --shotium-version <version> [--results-root <dir>]');
  }
  const result = aggregateResults({...options, commitResults: booleanArg(options.commitResults, false)});
  process.stdout.write(`${JSON.stringify({directory: result.destination, status: result.manifest.status})}\n`);
}
