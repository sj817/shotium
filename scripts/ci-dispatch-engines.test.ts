// The six dispatches differ from each other in exactly two ways -- the
// workflow file and whether the platform needs `mode=build` to compile
// anything -- and getting the second one wrong produces a green run that
// built nothing, which is the failure this project has a rule about.
import assert from 'node:assert/strict';
import test from 'node:test';

import {TARGETS, dispatchArgs, selectTargets} from './ci-dispatch-engines.ts';

test('targets: six of them, one per platform and architecture', () => {
  assert.equal(TARGETS.length, 6);
  assert.deepEqual(TARGETS.map((t) => t.id).sort(), [
    'linux-amd64', 'linux-arm64', 'macos-amd64', 'macos-arm64', 'windows-amd64', 'windows-arm64',
  ]);
  // Linux and macOS default to probe, which compiles nothing and produces no
  // artifact; Windows has no such input.
  assert.deepEqual(TARGETS.filter((t) => t.build).map((t) => t.id).sort(),
    ['linux-amd64', 'linux-arm64', 'macos-amd64', 'macos-arm64']);
});

test('--only picks a subset and rejects a name that is not a target', () => {
  assert.deepEqual(selectTargets('windows-amd64,linux-arm64').map((t) => t.id), ['windows-amd64', 'linux-arm64']);
  assert.equal(selectTargets(undefined).length, 6);
  assert.throws(() => selectTargets('windows-x64'), /unknown target/);
});

test('the command line carries mode=build only where the workflow has it', () => {
  const win = TARGETS.find((t) => t.id === 'windows-arm64')!;
  const mac = TARGETS.find((t) => t.id === 'macos-amd64')!;
  assert.deepEqual(dispatchArgs(win, 'main', 'sj817/shotium', true, 'auto'), [
    'workflow', 'run', 'engine-windows.yml', '-R', 'sj817/shotium', '--ref', 'main',
    '-f', 'arch=arm64', '-f', 'run_checks=true', '-f', 'shards=auto',
  ]);
  assert.deepEqual(dispatchArgs(mac, 'my-branch', 'sj817/shotium', false, '1'), [
    'workflow', 'run', 'engine-macos.yml', '-R', 'sj817/shotium', '--ref', 'my-branch',
    '-f', 'arch=amd64', '-f', 'mode=build', '-f', 'run_checks=false', '-f', 'shards=1',
  ]);
});
