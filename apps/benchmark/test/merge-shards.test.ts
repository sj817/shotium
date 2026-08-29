import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import {aggregateResults} from '../src/aggregate.ts';
import {ENGINE_IDS, SHARD_SCENARIOS} from '../src/constants.ts';
import {mergeShardResults} from '../src/merge-shards.ts';
import {validatePlatformResult} from '../src/schema.ts';

const SHARDS = Object.keys(SHARD_SCENARIOS);

function writeJson(file: string, value: unknown): void {
  fs.mkdirSync(path.dirname(file), {recursive: true});
  fs.writeFileSync(file, `${JSON.stringify(value, null, 2)}\n`);
}

function shardSummary(platform: string, shard: string, status = 'pass') {
  const scenario = SHARD_SCENARIOS[shard][0];
  return {
    schema_version: 2,
    generated_utc: '2026-08-30T01:00:00.000Z',
    platform,
    shard,
    status,
    shotium_version: '0.3.2',
    profile: 'smoke',
    seed: 'matrix-seed',
    source_revision: 'abc123',
    install: {
      main: {name: '@shotkit/shotium', version: '0.3.2', content_sha256: '1'.repeat(64), files: 1, bytes: 1},
      platform: {name: `@shotkit/shotium-${platform}`, version: '0.3.2', content_sha256: '2'.repeat(64), files: 1, bytes: 1},
      main_manifest_sha256: '3'.repeat(64),
      platform_manifest_sha256: '4'.repeat(64),
      esm: true,
      commonjs: true,
      api_smoke: {width: 1280, height: 720, bytes: 10, sha256: '5'.repeat(64)},
    },
    host: {
      runner: `runner-${shard}`,
      os: {platform},
      cpu: {model: shard},
      memory_bytes: 1024,
      logical_processors: 2,
      node: process.version,
      npm: null,
    },
    packages: {
      '@shotkit/shotium': '0.3.2', tinybench: '1', execa: '1', systeminformation: '1',
      pngjs: '1', pixelmatch: '1', 'wait-on': '1', ajv: '1', tsx: '1', typescript: '1',
    },
    measurement_contract: {viewport: {width: 1280, height: 720}, wait_until: 'load'},
    engines: ENGINE_IDS.map((engine) => engine === 'shotium' ? {
      engine, status, reason: null, executable: '/native/shot.node', architectures: ['x64'],
      sha256: '6'.repeat(64), binary_version: '0.3.2', binaries: [{}],
    } : {
      engine, status: 'n/a', reason: 'not installed in merger fixture', executable: null, architectures: [],
    }),
    scenarios: [{
      engine: 'shotium', scenario, concurrency: 1, status, ranking_eligible: status === 'pass',
      runs: 1, shots: 1,
      wall_time_ms: null, orchestration_wall_time_ms: null, latency_ms: null,
      peak_rss_bytes: null, rss_slope_bytes_per_minute: null,
      throughput_per_second: null, failure_rate: null,
    }],
    execution_order: [{engine: 'shotium', scenario, repeat: 1, attempt: 1, concurrency: 1, iterations: 1}],
    raw_samples: 1,
    failures: 0,
  };
}

function writeShard(root: string, platform: string, shard: string, status = 'pass', uploaded = true): void {
  const directory = path.join(root, `benchmark-permanent-${platform}-${shard}`);
  const summary = shardSummary(platform, shard, status);
  const scenario = summary.scenarios[0].scenario;
  writeJson(path.join(directory, 'summary.json'), summary);
  fs.writeFileSync(path.join(directory, 'samples.jsonl'), `${JSON.stringify({
    shard, engine: 'shotium', scenario, repeat: 1, attempt: 1, concurrency: 1, status,
  })}\n`);
  writeJson(path.join(directory, 'quality.json'), [{
    shard, engine: 'shotium', scenario, repeat: 1, attempt: 1, concurrency: 1, status,
  }]);
  writeJson(path.join(directory, 'failures.json'), []);
  writeJson(path.join(directory, 'artifact.json'), {
    name: `benchmark-evidence-${platform}-${shard}`,
    sha256: shard.charCodeAt(0).toString(16).padStart(2, '0').repeat(32),
    uploaded,
    run_url: 'https://example.test/actions/1',
  });
}

test('merges four shards without pretending they shared one host or one evidence artifact', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-merge-shards-'));
  try {
    const input = path.join(root, 'input');
    const output = path.join(root, 'output');
    for (const shard of SHARDS) writeShard(input, 'linux-x64', shard, shard === 'resident' ? 'noisy' : 'pass');
    const result = mergeShardResults({input, output, platform: 'linux-x64'});
    assert.equal(result.summary.status, 'noisy');
    assert.equal(result.summary.shards_complete, true);
    assert.equal(result.summary.host, null);
    assert.equal(result.summary.host_mode, 'sharded');
    assert.deepEqual(result.summary.execution_shards.map((entry) => entry.host.runner),
        SHARDS.map((shard) => `runner-${shard}`));
    assert.equal(result.summary.scenarios.length, 4);
    assert.equal(result.summary.raw_samples, 4);
    assert.equal(validatePlatformResult(result.summary), result.summary);
    assert.equal(result.artifact.uploaded, true);
    assert.equal(result.artifact.artifacts.length, 4);
    assert.deepEqual(result.artifact.artifacts.map((entry) => entry.name).sort(),
        SHARDS.map((shard) => `benchmark-evidence-linux-x64-${shard}`).sort());
    const samples = fs.readFileSync(path.join(output, 'samples.jsonl'), 'utf8').trim().split('\n').map((line) =>
      JSON.parse(line));
    assert.deepEqual(samples.map((entry) => entry.shard), SHARDS);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('writes an auditable incomplete platform result when a shard is missing', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-merge-missing-'));
  try {
    const input = path.join(root, 'input');
    const output = path.join(root, 'output');
    for (const shard of SHARDS.slice(0, 3)) writeShard(input, 'linux-x64', shard);
    const result = mergeShardResults({input, output, platform: 'linux-x64'});
    assert.equal(result.summary.status, 'infra-error');
    assert.equal(result.summary.shards_complete, false);
    assert.deepEqual(result.summary.missing_shards, ['resilience']);
    assert.match(result.summary.error, /missing resilience shard/);
    assert.equal(result.summary.failures, 1);
    assert.equal(result.artifact.uploaded, false);
    assert.deepEqual(result.artifact.missing_artifact_shards, ['resilience']);
    assert.equal(validatePlatformResult(result.summary), result.summary);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('keeps benchmark completeness separate from evidence upload completeness', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-merge-evidence-'));
  try {
    const input = path.join(root, 'input');
    const output = path.join(root, 'output');
    for (const shard of SHARDS) writeShard(input, 'linux-x64', shard, 'pass', shard !== 'resilience');
    const result = mergeShardResults({input, output, platform: 'linux-x64'});
    assert.equal(result.summary.status, 'pass');
    assert.equal(result.summary.shards_complete, true);
    assert.equal(result.artifact.uploaded, false);

    const aggregate = aggregateResults({
      input: output,
      resultsRoot: path.join(root, 'results'),
      shotiumVersion: '0.3.2',
      timestamp: '2026-08-30T01:30:00Z',
      runId: '1',
      runAttempt: '1',
    });
    const linux = aggregate.manifest.platforms.find((entry) => entry.platform === 'linux-x64');
    assert.equal(linux.evidence_complete, false);
    assert.equal(linux.artifacts.length, 4);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('rejects cross-shard scenario leakage as incomplete infrastructure output', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-merge-leakage-'));
  try {
    const input = path.join(root, 'input');
    const output = path.join(root, 'output');
    for (const shard of SHARDS) writeShard(input, 'linux-x64', shard);
    const startup = path.join(input, 'benchmark-permanent-linux-x64-startup');
    const summary = JSON.parse(fs.readFileSync(path.join(startup, 'summary.json'), 'utf8'));
    summary.scenarios[0].scenario = 'soak';
    writeJson(path.join(startup, 'summary.json'), summary);
    const result = mergeShardResults({input, output, platform: 'linux-x64'});
    assert.equal(result.summary.status, 'infra-error');
    assert.equal(result.summary.shards_complete, false);
    assert.deepEqual(result.summary.missing_shards, ['startup']);
    assert.match(result.summary.error, /does not belong to shard startup/);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('marks binary identity drift between runners as infrastructure failure', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-merge-binary-drift-'));
  try {
    const input = path.join(root, 'input');
    const output = path.join(root, 'output');
    for (const shard of SHARDS) writeShard(input, 'linux-x64', shard);
    const resident = path.join(input, 'benchmark-permanent-linux-x64-resident', 'summary.json');
    const summary = JSON.parse(fs.readFileSync(resident, 'utf8'));
    summary.engines[0].sha256 = '7'.repeat(64);
    writeJson(resident, summary);
    const result = mergeShardResults({input, output, platform: 'linux-x64'});
    assert.equal(result.summary.shards_complete, true);
    assert.equal(result.summary.status, 'infra-error');
    assert.equal(result.summary.engines[0].status, 'infra-error');
    assert.match(result.summary.engines[0].reason, /binary identity differs/);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('never promotes a fatal shard summary to a passing platform', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-merge-fatal-shard-'));
  try {
    const input = path.join(root, 'input');
    const output = path.join(root, 'output');
    for (const shard of SHARDS) writeShard(input, 'linux-x64', shard);
    const directory = path.join(input, 'benchmark-permanent-linux-x64-resilience');
    const summary = JSON.parse(fs.readFileSync(path.join(directory, 'summary.json'), 'utf8'));
    Object.assign(summary, {
      status: 'infra-error', engines: [], scenarios: [], execution_order: [], raw_samples: 0, failures: 1,
    });
    writeJson(path.join(directory, 'summary.json'), summary);
    fs.writeFileSync(path.join(directory, 'samples.jsonl'), '');
    writeJson(path.join(directory, 'quality.json'), []);
    writeJson(path.join(directory, 'failures.json'), [{error: 'runner process exited during setup'}]);
    const result = mergeShardResults({input, output, platform: 'linux-x64'});
    assert.equal(result.summary.shards_complete, true);
    assert.equal(result.summary.status, 'infra-error');
    assert.notEqual(result.summary.status, 'pass');
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('does not require API smoke PNG bytes to match across separate runners', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-merge-api-smoke-'));
  try {
    const input = path.join(root, 'input');
    const output = path.join(root, 'output');
    for (const shard of SHARDS) writeShard(input, 'linux-x64', shard);
    const resident = path.join(input, 'benchmark-permanent-linux-x64-resident', 'summary.json');
    const summary = JSON.parse(fs.readFileSync(resident, 'utf8'));
    summary.install.api_smoke.sha256 = '9'.repeat(64);
    summary.install.api_smoke.bytes += 1;
    writeJson(resident, summary);
    const result = mergeShardResults({input, output, platform: 'linux-x64'});
    assert.equal(result.summary.shards_complete, true);
    assert.equal(result.summary.status, 'pass');
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});
