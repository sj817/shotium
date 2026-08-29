import assert from 'node:assert/strict';
import test from 'node:test';
import {validatePlatformResult} from '../src/schema.ts';

function result() {
  return {
    schema_version: 2,
    generated_utc: '2026-08-29T01:30:45.000Z',
    platform: 'linux-x64',
    status: 'pass',
    shotium_version: '0.3.2',
    profile: 'smoke',
    seed: 'schema-test',
    source_revision: null,
    install: {
      main: {name: '@shotkit/shotium', version: '0.3.2', content_sha256: '1'.repeat(64), files: 1, bytes: 1},
      platform: {name: '@shotkit/shotium-linux-x64', version: '0.3.2', content_sha256: '2'.repeat(64), files: 1, bytes: 1},
      main_manifest_sha256: '0'.repeat(64),
      platform_manifest_sha256: 'a'.repeat(64),
      esm: true,
      commonjs: true,
      api_smoke: {width: 1280, height: 720, bytes: 10, sha256: 'b'.repeat(64)},
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
      engine: 'shotium', status: 'pass', reason: null, executable: '/shot.node', architectures: ['x64'],
      sha256: 'c'.repeat(64), binary_version: '0.3.2', binaries: [{}],
    }],
    scenarios: [{
      engine: 'shotium', scenario: 'warm', concurrency: 1, status: 'pass',
      ranking_eligible: true, runs: 1, shots: 1,
      wall_time_ms: null, orchestration_wall_time_ms: null, latency_ms: null,
      peak_rss_bytes: null, rss_slope_bytes_per_minute: null,
      throughput_per_second: null, failure_rate: null,
    }],
    execution_order: [{
      engine: 'shotium', scenario: 'warm', repeat: 1, attempt: 1, concurrency: 1, iterations: 1,
    }],
    raw_samples: 1,
    failures: 0,
  };
}

test('accepts a complete platform result envelope', () => {
  assert.equal(validatePlatformResult(result()).status, 'pass');
});

test('rejects engine records that cannot be audited', () => {
  const invalid = result();
  delete (invalid.engines[0] as any).architectures;
  assert.throws(() => validatePlatformResult(invalid), /schema failed/);
});

test('rejects scenario rows without ranking eligibility and distributions', () => {
  const invalid = result();
  (invalid.scenarios as any[]).push({
    engine: 'shotium', scenario: 'warm', concurrency: 1, status: 'pass', runs: 1, shots: 1,
  });
  assert.throws(() => validatePlatformResult(invalid), /schema failed/);
});

test('rejects an empty result that claims to pass', () => {
  const invalid = result();
  invalid.scenarios = [];
  invalid.execution_order = [];
  invalid.raw_samples = 0;
  assert.throws(() => validatePlatformResult(invalid), /schema failed/);
});
