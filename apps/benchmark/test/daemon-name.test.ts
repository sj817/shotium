import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import {
  BENCHMARK_DAEMON_NAME_MAX_BYTES,
  benchmarkDaemonName,
} from '../src/daemon-name.ts';

test('keeps every derived daemon name short, deterministic and auditable', () => {
  const scenarios = [
    'cold', 'cold-settled', 'warm', 'reuse-page', 'batch', 'resident', 'lifecycle',
    'faults', 'parallel', 'soak', 'recovery', 'interrupted', 'process-exit',
    'process-exit-recovered',
  ];
  for (const scenario of scenarios) {
    const name = benchmarkDaemonName({
      runId: '33258847880',
      platform: 'darwin-arm64',
      scenario,
      repeat: 1295,
      variant: 1295,
    });
    assert.equal(Buffer.byteLength(name), BENCHMARK_DAEMON_NAME_MAX_BYTES);
    assert.match(name, /^sb-[a-f0-9]{8}-da-[a-z0-9]{2}-zz-zz$/);
    assert.equal(name, benchmarkDaemonName({
      runId: '33258847880', platform: 'darwin-arm64', scenario, repeat: 1295, variant: 1295,
    }));
  }
});

test('fits the worst standard macOS temp prefix below sockaddr_un sun_path', () => {
  const worstMacTemp = '/var/folders/zz/abcdefghijklmnopqrstuvwxyz123456/T';
  const name = benchmarkDaemonName({
    runId: '18446744073709551615',
    platform: 'darwin-arm64',
    scenario: 'process-exit-recovered',
    repeat: 1295,
    variant: 1295,
  });
  const endpoint = path.posix.join(worstMacTemp, `shotium-4294967295-${name}.sock`);
  assert.ok(Buffer.byteLength(endpoint) < 104, `${endpoint} is too long`);
});
