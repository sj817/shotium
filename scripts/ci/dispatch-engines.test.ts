// One dispatch carries everything engine.yml needs to know; a wrong flag
// name is a run that builds nothing and looks green, which is the failure
// this project has a rule about.
import assert from 'node:assert/strict';
import test from 'node:test';

import {dispatchArgs, WORKFLOW} from './dispatch-engines.ts';

test('the command line names engine.yml and every input it declares', () => {
  assert.equal(WORKFLOW, 'engine.yml');
  assert.deepEqual(dispatchArgs({ref: 'main', repo: 'sj817/shotium', force: false, targets: 'all', shards: 'auto', jobs: ''}), [
    'workflow', 'run', 'engine.yml', '-R', 'sj817/shotium', '--ref', 'main',
    '-f', 'force=false', '-f', 'targets=all', '-f', 'shards=auto', '-f', 'jobs=',
  ]);
  assert.deepEqual(dispatchArgs({ref: 'my-branch', repo: 'sj817/shotium', force: true, targets: 'windows-amd64,linux-arm64', shards: '1', jobs: '3'}).slice(-8), [
    '-f', 'force=true', '-f', 'targets=windows-amd64,linux-arm64', '-f', 'shards=1', '-f', 'jobs=3',
  ]);
});

test('a target that is not a platform is refused before anything is sent', () => {
  assert.throws(() => dispatchArgs({ref: 'main', repo: 'sj817/shotium', force: false, targets: 'windows-x64', shards: 'auto', jobs: ''}), /unknown platform windows-x64/);
});
