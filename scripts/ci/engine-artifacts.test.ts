// The lookup decides what a release ships, so every rule that keeps the
// wrong bytes out is pinned: a fork's upload is not an engine, an engine
// without evidence from its own run is not an engine, a build directory is
// "exact" only when its marker shares the run, and an upload from a
// workflow that does not build engines is not one either.
import assert from 'node:assert/strict';
import test from 'node:test';

import {platforms} from '../lib/platforms.ts';
import {
  type Api, type ArtifactRecord, findBuildDir, findEngineSet, listByName, names, status, statusOutputs, trusted,
} from './engine-artifacts.ts';

const REPO = 100;
const FORK = 200;
const FP = 'd234ab93f5f87385';

let nextId = 1;
function record(name: string, run: number, extra: Partial<ArtifactRecord & {head: number; at: string}> = {}): ArtifactRecord {
  return {
    id: nextId++, name, size_in_bytes: extra.size_in_bytes ?? 400_000_000, expired: extra.expired ?? false,
    created_at: extra.at ?? `2026-09-${String(run % 28 + 1).padStart(2, '0')}T00:00:00Z`,
    workflow_run: {id: run, repository_id: REPO, head_repository_id: extra.head ?? REPO, head_branch: 'main', head_sha: 'abc'},
  };
}

// A stub of the two routes the module reads: the name-filtered artifact
// list and the run whose workflow file it double-checks.
function api(records: ArtifactRecord[], runs: Record<number, string> = {}): Api {
  return async (route) => {
    const list = /^actions\/artifacts\?name=([^&]+)&per_page=100&page=(\d+)$/.exec(route);
    if (list) {
      const name = decodeURIComponent(list[1]);
      const page = Number(list[2]);
      const matching = records.filter((r) => r.name === name);
      return {total_count: matching.length, artifacts: page === 1 ? matching : []};
    }
    const run = /^actions\/runs\/(\d+)$/.exec(route);
    if (run) return {path: runs[Number(run[1])] ?? '.github/workflows/engine-linux.yml'};
    throw new Error(`unexpected route ${route}`);
  };
}

test('trust: this repository or this run; never a fork, never expired', () => {
  assert.equal(trusted(record('x', 1)), true);
  assert.equal(trusted(record('x', 1, {head: FORK})), false);
  assert.equal(trusted(record('x', 1, {head: FORK}), 1), true, 'a fork PR may use what its own run built');
  assert.equal(trusted(record('x', 1, {head: FORK}), 2), false);
  assert.equal(trusted(record('x', 1, {expired: true})), false);
  assert.equal(trusted({...record('x', 1), workflow_run: null}), false);
});

test('listByName drops expired records and orders newest first', async () => {
  const records = [record('a', 1, {at: '2026-09-01T00:00:00Z'}), record('a', 2, {at: '2026-09-03T00:00:00Z'}), record('a', 3, {expired: true}), record('b', 4)];
  assert.deepEqual((await listByName(api(records), 'a')).map((r) => r.workflow_run!.id), [2, 1]);
});

test('an engine counts only with evidence from the same trusted run', async () => {
  const n = names('linux-amd64', FP);
  assert.deepEqual(n, {engine: `engine-linux-amd64-${FP}`, evidence: `ffi-evidence-linux-amd64-${FP}`, buildDir: 'build-dir-linux-amd64', marker: `build-dir-linux-amd64-${FP}`});

  // run 3: engine without evidence (run_checks=false); run 2: both, newer than run 1.
  const records = [
    record(n.engine, 1, {at: '2026-09-01T00:00:00Z'}), record(n.evidence, 1, {at: '2026-09-01T00:00:00Z'}),
    record(n.engine, 2, {at: '2026-09-02T00:00:00Z'}), record(n.evidence, 2, {at: '2026-09-02T00:00:00Z'}),
    record(n.engine, 3, {at: '2026-09-03T00:00:00Z'}),
  ];
  assert.equal((await findEngineSet(api(records), FP, 'linux-amd64'))?.runId, 2);

  // The newest complete pair is a fork's: skipped, unless it is our own run.
  const forked = [...records, record(n.engine, 4, {head: FORK, at: '2026-09-04T00:00:00Z'}), record(n.evidence, 4, {head: FORK, at: '2026-09-04T00:00:00Z'})];
  assert.equal((await findEngineSet(api(forked), FP, 'linux-amd64'))?.runId, 2);
  assert.equal((await findEngineSet(api(forked), FP, 'linux-amd64', 4))?.runId, 4);

  // Evidence must come from the engine's run, not merely exist.
  const crossed = [record(n.engine, 5), record(n.evidence, 6)];
  assert.equal(await findEngineSet(api(crossed), FP, 'linux-amd64'), null);

  // A run of a workflow that does not build engines is not a source.
  const stray = [record(n.engine, 7), record(n.evidence, 7)];
  assert.equal(await findEngineSet(api(stray, {7: '.github/workflows/checks.yml'}), FP, 'linux-amd64'), null);
  assert.equal((await findEngineSet(api(stray, {7: '.github/workflows/preview.yml'}), FP, 'linux-amd64'))?.runId, 7);
});

test('the build directory: exact when its marker shares the run, otherwise the newest warm start', async () => {
  const n = names('windows-arm64', FP);
  const records = [
    record(n.buildDir, 1, {at: '2026-09-01T00:00:00Z'}), record(n.marker, 1, {at: '2026-09-01T00:00:00Z'}),
    record(n.buildDir, 2, {at: '2026-09-02T00:00:00Z'}),
    record(n.marker, 9, {at: '2026-09-09T00:00:00Z'}),   // a marker whose blob expired: means nothing
  ];
  const exact = await findBuildDir(api(records), 'windows-arm64', FP);
  assert.deepEqual([exact?.runId, exact?.exact], [1, true]);
  const other = await findBuildDir(api(records), 'windows-arm64', 'ffffffffffffffff');
  assert.deepEqual([other?.runId, other?.exact], [2, false]);
  assert.equal(await findBuildDir(api([]), 'windows-arm64', FP), null);
  assert.equal((await findBuildDir(api(records), 'windows-arm64', undefined))?.runId, 2, 'without a fingerprint, the newest');
  // A cancelled job saves a tar of nothing; it must neither be chosen nor
  // shadow the real one.
  const junk = [...records, record(n.buildDir, 3, {at: '2026-09-03T00:00:00Z', size_in_bytes: 173})];
  assert.equal((await findBuildDir(api(junk), 'windows-arm64', undefined))?.runId, 2);
});

test('status lists what to build per OS, and --force builds everything', async () => {
  const have = ['linux-amd64', 'macos-amd64', 'macos-arm64'];
  const records = have.flatMap((label, i) => [record(names(label, FP).engine, i + 1), record(names(label, FP).evidence, i + 1)]);
  const result = await status(api(records), FP, [...platforms], false);
  assert.deepEqual(result.build, {
    windows: [
      {label: 'windows-amd64', arch: 'amd64', cpu: 'x64'},
      {label: 'windows-arm64', arch: 'arm64', cpu: 'arm64'},
    ],
    linux: [
      {label: 'linux-arm64', arch: 'arm64', cpu: 'arm64', libc: 'glibc'},
      {label: 'linux-amd64-musl', arch: 'amd64', cpu: 'x64', libc: 'musl'},
      {label: 'linux-arm64-musl', arch: 'arm64', cpu: 'arm64', libc: 'musl'},
    ],
    macos: [],
  });
  assert.deepEqual(result.missing, [
    'windows-amd64', 'windows-arm64', 'linux-arm64', 'linux-amd64-musl', 'linux-arm64-musl',
  ]);
  assert.equal(result.complete, false);
  assert.equal(statusOutputs(result),
    `fingerprint=${FP}\n` +
    'windows=[{"label":"windows-amd64","arch":"amd64","cpu":"x64"},{"label":"windows-arm64","arch":"arm64","cpu":"arm64"}]\n' +
    'windows_count=2\n' +
    'linux=[{"label":"linux-arm64","arch":"arm64","cpu":"arm64","libc":"glibc"},{"label":"linux-amd64-musl","arch":"amd64","cpu":"x64","libc":"musl"},{"label":"linux-arm64-musl","arch":"arm64","cpu":"arm64","libc":"musl"}]\n' +
    'linux_count=3\nmacos=[]\nmacos_count=0\n' +
    'missing=windows-amd64,windows-arm64,linux-arm64,linux-amd64-musl,linux-arm64-musl\ncomplete=false\n');

  const forced = await status(api(records), FP, [...platforms], true);
  assert.equal(forced.missing.length, 8);
  const done = await status(api(records), FP, platforms.filter((p) => have.includes(p.label)), false);
  assert.deepEqual([done.complete, done.build], [true, {windows: [], linux: [], macos: []}]);
});
