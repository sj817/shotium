import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import {aggregateResults, formatRunTimestamp} from '../src/aggregate.ts';
import {PLATFORM_IDS} from '../src/constants.ts';

function writeJson(file: string, value: unknown): void {
  fs.mkdirSync(path.dirname(file), {recursive: true});
  fs.writeFileSync(file, `${JSON.stringify(value)}\n`);
}

function platformResult(platform: string) {
  return {
    schema_version: 2,
    generated_utc: '2026-08-29T01:30:45.000Z',
    platform,
    status: 'pass',
    shotium_version: '0.3.2',
    profile: 'full',
    seed: 'fair-seed',
    source_revision: 'abc123',
    install: {
      main: {name: '@shotkit/shotium', version: '0.3.2', content_sha256: '1'.repeat(64), files: 1, bytes: 1},
      platform: {name: `@shotkit/shotium-${platform}`, version: '0.3.2', content_sha256: '2'.repeat(64), files: 1, bytes: 1},
      main_manifest_sha256: '3'.repeat(64),
      platform_manifest_sha256: 'd'.repeat(64),
      esm: true,
      commonjs: true,
      api_smoke: {width: 1280, height: 720, bytes: 10, sha256: 'e'.repeat(64)},
    },
    host: {
      runner: 'test', os: {}, cpu: {}, memory_bytes: 1, logical_processors: 1,
      node: process.version, npm: null,
    },
    packages: {
      '@shotkit/shotium': '0.3.2', tinybench: '1', execa: '1', systeminformation: '1',
      pngjs: '1', pixelmatch: '1', 'wait-on': '1', ajv: '1', tsx: '1', typescript: '1',
    },
    engines: [{
      engine: 'shotium', status: 'pass', reason: null, executable: '/native/shot.node',
      architectures: [platform.endsWith('arm64') ? 'arm64' : 'x64'], sha256: 'f'.repeat(64),
      binary_version: '0.3.2', binaries: [{}],
    }],
    scenarios: [{
      engine: 'shotium', scenario: 'warm', concurrency: 1, status: 'pass', runs: 1, shots: 20,
      ranking_eligible: true,
      wall_time_ms: {n: 1, min: 200, p50: 200, p95: 200, max: 200, mean: 200, mad: 0},
      orchestration_wall_time_ms: {n: 1, min: 250, p50: 250, p95: 250, max: 250, mean: 250, mad: 0},
      latency_ms: {n: 20, min: 8, p50: 10, p95: 12, max: 14, mean: 10, mad: 1},
      peak_rss_bytes: null,
      rss_slope_bytes_per_minute: null,
      throughput_per_second: 100,
      failure_rate: null,
    }],
    execution_order: [{
      engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, iterations: 20,
    }],
    raw_samples: 1,
    failures: 0,
  };
}

test('aggregates exactly six native platform results into the standard directory', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-aggregate-test-'));
  try {
    const input = path.join(root, 'input');
    const results = path.join(root, 'benchmark-results');
    for (const platform of PLATFORM_IDS) {
      const directory = path.join(input, `benchmark-permanent-${platform}`);
      writeJson(path.join(directory, 'summary.json'), platformResult(platform));
      fs.writeFileSync(path.join(directory, 'samples.jsonl'),
          `${JSON.stringify({
            engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
          })}\n`);
      writeJson(path.join(directory, 'quality.json'), [{
        engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
      }]);
      writeJson(path.join(directory, 'failures.json'), []);
      writeJson(path.join(directory, 'artifact.json'), {
        name: `benchmark-evidence-${platform}`, sha256: 'a'.repeat(64), uploaded: true,
      });
    }
    const aggregate = aggregateResults({
      input, resultsRoot: results, shotiumVersion: '0.3.2', runId: '123456789', runAttempt: '1',
      timestamp: '2026-08-29T01:30:45.000Z', profile: 'full', seed: 'fair-seed',
    });
    assert.equal(path.basename(aggregate.destination), '20260829T013045Z-gh123456789-a1');
    assert.equal(aggregate.manifest.status, 'complete');
    assert.equal(aggregate.manifest.evidence_status, 'complete');
    assert.equal(aggregate.manifest.publishable, true);
    assert.equal(aggregate.manifest.platforms.length, 6);
    assert.match(fs.readFileSync(path.join(aggregate.destination, 'report.md'), 'utf8'), /No cross-platform ranking/);
    const chineseReport = path.join(aggregate.destination, 'report.zh-CN.md');
    assert.equal(fs.existsSync(chineseReport), true);
    assert.match(fs.readFileSync(chineseReport, 'utf8'), /不进行跨平台混排/);
    assert.match(fs.readFileSync(path.join(aggregate.destination, 'summary.csv'), 'utf8'), /ratio_to_shotium/);
    const index = JSON.parse(fs.readFileSync(path.join(results, 'index.json'), 'utf8'));
    assert.equal(index.results.length, 1);
    assert.equal(index.results[0].publishable, true);
    const latest = fs.readFileSync(path.join(results, 'LATEST.md'), 'utf8');
    assert.match(latest, /\[English\]\(v0\.3\.2\/20260829T013045Z-gh123456789-a1\/report\.md\)/);
    assert.match(latest, /\[简体中文\]\(v0\.3\.2\/20260829T013045Z-gh123456789-a1\/report\.zh-CN\.md\)/);
    assert.match(latest, /https:\/\/sj817\.github\.io\/shotium\//);
    assert.match(latest, /status complete \/ 状态 完整/);
    assert.match(latest, /quality pass \/ 质量 通过/);
    assert.match(latest, /evidence complete \/ 证据 完整/);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('publishes trusted results when a competitor records a product failure', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-aggregate-baseline-failure-test-'));
  try {
    const input = path.join(root, 'input');
    for (const platform of PLATFORM_IDS) {
      const directory = path.join(input, platform);
      const summary: any = platformResult(platform);
      if (platform === 'linux-x64') {
        summary.status = 'fail';
        summary.engines.push({
          engine: 'puppeteer-chrome', status: 'fail', reason: null,
          executable: '/chrome', architectures: ['x64'],
        });
      }
      writeJson(path.join(directory, 'summary.json'), summary);
      fs.writeFileSync(path.join(directory, 'samples.jsonl'), `${JSON.stringify({
        engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
      })}\n`);
      writeJson(path.join(directory, 'quality.json'), [{
        engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
      }]);
      writeJson(path.join(directory, 'failures.json'), []);
      writeJson(path.join(directory, 'artifact.json'), {
        name: `benchmark-evidence-${platform}`, sha256: 'a'.repeat(64), uploaded: true,
      });
    }
    const aggregate = aggregateResults({
      input, resultsRoot: path.join(root, 'results'), shotiumVersion: '0.3.2',
      runId: 'baseline-failure', runAttempt: '1', timestamp: '2026-08-29T01:31:00Z',
    });
    assert.equal(aggregate.manifest.platforms[0].status, 'fail');
    assert.equal(aggregate.manifest.platforms[0].quality_status, 'pass');
    assert.equal(aggregate.manifest.quality_status, 'pass');
    assert.equal(aggregate.manifest.publishable, true);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('publishes a complete noisy run while retaining the noisy quality label', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-aggregate-noisy-test-'));
  try {
    const input = path.join(root, 'input');
    for (const platform of PLATFORM_IDS) {
      const directory = path.join(input, platform);
      const summary: any = platformResult(platform);
      if (platform === 'linux-x64') {
        summary.status = 'noisy';
        summary.engines[0].status = 'noisy';
      }
      writeJson(path.join(directory, 'summary.json'), summary);
      fs.writeFileSync(path.join(directory, 'samples.jsonl'), `${JSON.stringify({
        engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
      })}\n`);
      writeJson(path.join(directory, 'quality.json'), [{
        engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
      }]);
      writeJson(path.join(directory, 'failures.json'), []);
      writeJson(path.join(directory, 'artifact.json'), {
        name: `benchmark-evidence-${platform}`, sha256: 'a'.repeat(64), uploaded: true,
      });
    }
    const aggregate = aggregateResults({
      input, resultsRoot: path.join(root, 'results'), shotiumVersion: '0.3.2',
      runId: 'noisy', runAttempt: '1', timestamp: '2026-08-29T01:32:00Z',
    });
    assert.equal(aggregate.manifest.quality_status, 'noisy');
    assert.equal(aggregate.manifest.publishable, true);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('marks a partial matrix incomplete instead of inventing a passing platform', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-incomplete-test-'));
  try {
    const input = path.join(root, 'input', 'linux-x64');
    writeJson(path.join(input, 'summary.json'), platformResult('linux-x64'));
    fs.writeFileSync(path.join(input, 'samples.jsonl'),
        `${JSON.stringify({
          engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
        })}\n`);
    writeJson(path.join(input, 'quality.json'), [{
      engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
    }]);
    writeJson(path.join(input, 'failures.json'), []);
    const aggregate = aggregateResults({
      input: path.join(root, 'input'), resultsRoot: path.join(root, 'results'), shotiumVersion: 'latest',
      runId: '1', runAttempt: '2', timestamp: '2026-08-29T02:00:00Z',
    });
    assert.equal(aggregate.manifest.status, 'incomplete');
    assert.equal(aggregate.manifest.publishable, false);
    assert.equal(aggregate.manifest.platforms.filter((entry: any) => entry.missing).length, 5);
    assert.match(fs.readFileSync(path.join(root, 'results', 'LATEST.md'), 'utf8'),
        /No publishable benchmark result yet/);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('marks the manifest incomplete when a platform is missing a scenario shard', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-incomplete-shard-test-'));
  try {
    const input = path.join(root, 'input');
    for (const platform of PLATFORM_IDS) {
      const directory = path.join(input, platform);
      const summary: any = platformResult(platform);
      if (platform === 'linux-x64') {
        summary.status = 'infra-error';
        summary.shards_complete = false;
        summary.missing_shards = ['resilience'];
      }
      writeJson(path.join(directory, 'summary.json'), summary);
      fs.writeFileSync(path.join(directory, 'samples.jsonl'), `${JSON.stringify({
        engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
      })}\n`);
      writeJson(path.join(directory, 'quality.json'), [{
        engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
      }]);
      writeJson(path.join(directory, 'failures.json'), []);
    }
    const aggregate = aggregateResults({
      input,
      resultsRoot: path.join(root, 'results'),
      shotiumVersion: '0.3.2',
      timestamp: '2026-08-29T02:15:00Z',
      runId: '11',
      runAttempt: '1',
    });
    assert.equal(aggregate.manifest.status, 'incomplete');
    assert.equal(aggregate.manifest.platforms[0].shards_complete, false);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('timestamp format is stable and UTC', () => {
  assert.equal(formatRunTimestamp(new Date('2026-08-29T01:30:45.999Z')), '20260829T013045Z');
});

test('archives a zero-output dist-tag run under an auditable unresolved version', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-zero-output-test-'));
  try {
    const aggregate = aggregateResults({
      input: path.join(root, 'empty'),
      resultsRoot: path.join(root, 'results'),
      shotiumVersion: 'latest',
      timestamp: '2026-08-29T02:30:00Z',
      runId: '10',
      runAttempt: '1',
    });
    assert.match(aggregate.destination, /[\\/]vunresolved-latest[\\/]/);
    assert.equal(aggregate.manifest.status, 'incomplete');
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('uses the exact successful platform version when another platform failed before resolving a dist-tag', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-dist-tag-test-'));
  try {
    const input = path.join(root, 'input');
    const exact = platformResult('linux-x64');
    const unresolved = platformResult('linux-arm64');
    unresolved.status = 'infra-error';
    unresolved.shotium_version = 'latest';
    for (const summary of [exact, unresolved]) {
      const directory = path.join(input, summary.platform);
      writeJson(path.join(directory, 'summary.json'), summary);
      fs.writeFileSync(path.join(directory, 'samples.jsonl'),
          `${JSON.stringify({
            engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
          })}\n`);
      writeJson(path.join(directory, 'quality.json'), [{
        engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, status: 'pass',
      }]);
      writeJson(path.join(directory, 'failures.json'), []);
    }
    const aggregate = aggregateResults({
      input,
      resultsRoot: path.join(root, 'results'),
      shotiumVersion: 'unresolved-latest',
      timestamp: '2026-08-29T03:00:00Z',
      runId: '2',
      runAttempt: '1',
    });
    assert.match(aggregate.destination, /[\\/]v0\.3\.2[\\/]/);
    assert.equal(aggregate.manifest.status, 'incomplete');
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});

test('treats malformed raw evidence as a missing platform instead of a complete result', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-corrupt-result-test-'));
  try {
    const directory = path.join(root, 'input', 'linux-x64');
    writeJson(path.join(directory, 'summary.json'), platformResult('linux-x64'));
    fs.writeFileSync(path.join(directory, 'samples.jsonl'), '{not-json}\n');
    writeJson(path.join(directory, 'quality.json'), []);
    writeJson(path.join(directory, 'failures.json'), []);
    const aggregate = aggregateResults({
      input: path.join(root, 'input'),
      resultsRoot: path.join(root, 'results'),
      shotiumVersion: '0.3.2',
      timestamp: '2026-08-29T04:00:00Z',
      runId: '3',
      runAttempt: '1',
    });
    assert.equal(aggregate.manifest.status, 'incomplete');
    assert.match(aggregate.manifest.platforms[0].reason, /invalid permanent result/);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});
