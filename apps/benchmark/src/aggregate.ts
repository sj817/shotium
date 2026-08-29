import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {parseArgs, booleanArg, recoverNpmRunValues} from './args.ts';
import {APP_ROOT, PLATFORM_IDS, RESULT_STATUSES} from './constants.ts';
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

function csv(value: unknown): string {
  const text = value === null || value === undefined ? '' : String(value);
  return /[",\r\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
}

export function formatRunTimestamp(date: Date): string {
  if (!Number.isFinite(date.getTime())) throw new Error('invalid aggregate timestamp');
  return date.toISOString().replace(/[-:]/g, '').replace(/\.\d{3}Z$/, 'Z');
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

function scenarioRows(platform: any): string[] {
  const shotium = new Map<string, any>();
  for (const scenario of platform.scenarios || []) {
    if (scenario.engine === 'shotium' && scenario.status === 'pass' && scenario.ranking_eligible) {
      shotium.set(`${scenario.scenario}|${scenario.concurrency}`, scenario);
    }
  }
  const rows: string[] = [];
  for (const scenario of platform.scenarios || []) {
    const baseline = shotium.get(`${scenario.scenario}|${scenario.concurrency}`);
    const p50 = scenario.latency_ms?.p50 ?? scenario.wall_time_ms?.p50 ?? null;
    const baselineP50 = baseline?.latency_ms?.p50 ?? baseline?.wall_time_ms?.p50 ?? null;
    const ratio = !scenario.ranking_eligible ? null : scenario.engine === 'shotium' ? 1 :
      Number.isFinite(p50) && Number.isFinite(baselineP50) && baselineP50 > 0 ? p50 / baselineP50 : null;
    rows.push([
      platform.platform, platform.status, scenario.engine, scenario.scenario,
      scenario.concurrency, scenario.status, scenario.ranking_eligible, scenario.runs, scenario.shots,
      p50, scenario.latency_ms?.p95 ?? null, scenario.latency_ms?.max ?? scenario.wall_time_ms?.max ?? null,
      scenario.latency_ms?.mad ?? null, scenario.throughput_per_second ?? null,
      scenario.failure_rate ?? null, scenario.rss_slope_bytes_per_minute?.p50 ?? null,
      ratio === null ? null : Number(ratio.toFixed(4)),
    ].map(csv).join(','));
  }
  return rows;
}

type ReportLocale = 'en' | 'zh-CN';

const REPORT_COPY = {
  en: {
    title: (version: string) => `# Shotium ${version} benchmark`,
    result: (manifest: any) =>
      `Result: **${manifest.status}**; quality: **${manifest.quality_status}**; ` +
      `evidence: **${manifest.evidence_status}**. ` +
      `Profile \`${manifest.profile}\`, seed \`${manifest.seed}\`.`,
    ratios: 'Ratios are calculated only against Shotium on the same native runner, scenario and concurrency. ' +
      'No absolute timing is ranked across operating systems or architectures.',
    engineHeader: '| engine | availability | reason / binary architecture |',
    scenarioHeader: '| scenario | c | engine | status | ranked | p50 ms | worst ms | throughput/s | vs Shotium |',
    ranked: (eligible: boolean) => eligible ? 'yes' : 'no',
    missingHeading: '## Missing platform outputs',
  },
  'zh-CN': {
    title: (version: string) => `# Shotium ${version} 基准报告`,
    result: (manifest: any) =>
      `结果：**${manifest.status}**；质量：**${manifest.quality_status}**；` +
      `证据：**${manifest.evidence_status}**。` +
      `配置 \`${manifest.profile}\`，种子 \`${manifest.seed}\`。`,
    ratios: '性能比仅以同一原生运行器、同一场景和同一并发度下的 Shotium 为基准计算。' +
      '不同操作系统或处理器架构之间的绝对耗时不参与排名。',
    engineHeader: '| 引擎 | 可用性 | 原因 / 二进制架构 |',
    scenarioHeader: '| 场景 | 并发 | 引擎 | 状态 | 参与排名 | p50 毫秒 | 最差毫秒 | 吞吐量/秒 | 相对 Shotium |',
    ranked: (eligible: boolean) => eligible ? '是' : '否',
    missingHeading: '## 缺失的平台输出',
  },
} satisfies Record<ReportLocale, {
  title: (version: string) => string;
  result: (manifest: any) => string;
  ratios: string;
  engineHeader: string;
  scenarioHeader: string;
  ranked: (eligible: boolean) => string;
  missingHeading: string;
}>;

function report(platforms: any[], manifest: any, locale: ReportLocale): string {
  const copy = REPORT_COPY[locale];
  const lines = [
    copy.title(manifest.shotium_version), '',
    copy.result(manifest), '',
    copy.ratios, '',
  ];
  for (const platform of platforms) {
    lines.push(`## ${platform.platform}`, '');
    lines.push(copy.engineHeader, '|:--|:--|:--|');
    for (const engine of platform.engines || []) {
      const detail = engine.reason || `${(engine.architectures || []).join('+') || 'unknown'}; ${engine.executable || ''}`;
      lines.push(`| ${engine.engine} | ${engine.status} | ${String(detail).replaceAll('|', '\\|')} |`);
    }
    lines.push('', copy.scenarioHeader,
        '|:--|--:|:--|:--|:--|--:|--:|--:|--:|');
    const baseline = new Map<string, any>();
    for (const row of platform.scenarios || []) {
      if (row.engine === 'shotium' && row.status === 'pass' && row.ranking_eligible) {
        baseline.set(`${row.scenario}|${row.concurrency}`, row);
      }
    }
    for (const row of platform.scenarios || []) {
      const p50 = row.latency_ms?.p50 ?? row.wall_time_ms?.p50;
      const worst = row.latency_ms?.max ?? row.wall_time_ms?.max;
      const base = baseline.get(`${row.scenario}|${row.concurrency}`);
      const baseP50 = base?.latency_ms?.p50 ?? base?.wall_time_ms?.p50;
      const ratio = !row.ranking_eligible ? 'N/A' : row.engine === 'shotium' ? '1.00x' :
        Number.isFinite(p50) && Number.isFinite(baseP50) && baseP50 > 0 ? `${(p50 / baseP50).toFixed(2)}x` : 'N/A';
      lines.push(`| ${row.scenario} | ${row.concurrency} | ${row.engine} | ${row.status} | ${copy.ranked(row.ranking_eligible)} | ` +
        `${p50 ?? 'N/A'} | ${worst ?? 'N/A'} | ${row.throughput_per_second?.toFixed?.(3) ?? 'N/A'} | ${ratio} |`);
    }
    lines.push('');
  }
  const missing = manifest.platforms.filter((entry: any) => entry.missing).map((entry: any) => entry.platform);
  if (missing.length) lines.push(copy.missingHeading, '', missing.map((item: string) => `- ${item}`).join('\n'), '');
  return `${lines.join('\n')}\n`;
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
    let artifactError = null;
    if (fs.existsSync(artifactFile)) {
      try {
        artifact = readJson(artifactFile);
        if (!artifact.name || !/^[a-f0-9]{64}$/.test(String(artifact.sha256))) {
          throw new Error('artifact record is missing its name or SHA-256');
        }
      } catch (error) {
        artifact = null;
        artifactError = String(error);
      }
    }
    platformRecords.push({
      platform,
      missing: false,
      status: summary.status,
      summary_sha256: sha256(summaryFile),
      permanent_sha256: permanentHashes,
      artifact_error: artifactError,
      artifact: artifact ? {
        name: artifact.name,
        sha256: artifact.sha256,
        run_url: artifact.run_url,
        uploaded: artifact.uploaded === true,
        actions_artifact_id: artifact.actions_artifact_id || null,
        actions_artifact_url: artifact.actions_artifact_url || null,
        actions_artifact_digest: artifact.actions_artifact_digest || null,
      } : null,
    });
  }

  const complete = platformRecords.every((entry) => !entry.missing && RESULT_STATUSES.includes(entry.status));
  const evidenceComplete = platformRecords.every((entry) =>
    !entry.missing && entry.artifact?.uploaded === true);
  const qualityStatus = !complete || !evidenceComplete ||
    platformRecords.some((entry) => ['fail', 'infra-error'].includes(entry.status)) ? 'fail' :
    platformRecords.some((entry) => entry.status === 'noisy') ? 'noisy' : 'pass';
  const manifest = {
    schema_version: 1,
    generated_utc: generated.toISOString(),
    status: complete ? 'complete' : 'incomplete',
    quality_status: qualityStatus,
    evidence_status: evidenceComplete ? 'complete' : 'incomplete',
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
  fs.writeFileSync(path.join(destination, 'report.md'), report(platforms, manifest, 'en'));
  fs.writeFileSync(path.join(destination, 'report.zh-CN.md'), report(platforms, manifest, 'zh-CN'));
  const header = 'platform,platform_status,engine,scenario,concurrency,status,ranking_eligible,runs,shots,p50_ms,p95_ms,worst_ms,mad_ms,throughput_per_second,failure_rate,rss_slope_bytes_per_minute,ratio_to_shotium';
  fs.writeFileSync(path.join(destination, 'summary.csv'), `${[header, ...platforms.flatMap(scenarioRows)].join('\n')}\n`);

  const index = rebuildIndex(resultsRoot);
  writeJson(path.join(resultsRoot, 'index.json'), {schema_version: 1, generated_utc: new Date().toISOString(), results: index});
  const latest = index[0];
  fs.writeFileSync(path.join(resultsRoot, 'LATEST.md'), latest ?
    `# Latest benchmark / 最新基准\n\n${latest.shotium_version} · ${latest.generated_utc}: ` +
      `[English](${latest.path}/report.md) · ` +
      `[简体中文](${latest.path}/report.zh-CN.md) ` +
      `— status/状态 ${latest.status}, quality/质量 ${latest.quality_status}, ` +
      `evidence/证据 ${latest.evidence_status}.\n` :
    '# Latest benchmark / 最新基准\n\nNo benchmark results yet. / 暂无基准结果。\n');

  if (options.githubOutput) {
    fs.appendFileSync(String(options.githubOutput), `result_directory=${destination.replaceAll('\\', '/')}\n` +
      `result_name=${runName}\nversion=${version}\nmanifest_status=${manifest.status}\n`);
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
