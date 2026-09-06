// The two decisions this script makes are "have they all uploaded" and "can
// any of them still upload". Both are one-liners, and both are the difference
// between a final job that links and one that waits four hours or rebuilds
// the tree in silence, so they are tested rather than read.
import assert from 'node:assert/strict';
import test from 'node:test';

import {type Job, matchingJobs, missingArtifacts, unfinishedJobs} from './ci-await-shards.ts';

const jobs: Job[] = [
  {name: 'resolve: shotium-linux-amd64', status: 'completed', conclusion: 'success'},
  {name: 'compile: shotium-linux-amd64-v0.4.0 0/4', status: 'completed', conclusion: 'success'},
  {name: 'compile: shotium-linux-amd64-v0.4.0 1/4', status: 'in_progress', conclusion: null},
  {name: 'compile: shotium-linux-amd64-v0.4.0 2/4', status: 'completed', conclusion: 'failure'},
  {name: 'build: shotium-linux-amd64-v0.4.0', status: 'in_progress', conclusion: null},
];

test('artifacts: the ones not yet seen, in the order they were expected', () => {
  const expected = ['shard-linux-amd64-0', 'shard-linux-amd64-1', 'shard-linux-amd64-2'];
  assert.deepEqual(missingArtifacts(expected, ['shard-linux-amd64-1']), ['shard-linux-amd64-0', 'shard-linux-amd64-2']);
  assert.deepEqual(missingArtifacts(expected, expected), []);
  assert.deepEqual(missingArtifacts([], ['whatever']), []);
});

test('jobs: only shard jobs count, and only "completed" ends the wait', () => {
  const prefix = 'compile: shotium-linux-amd64';
  // The final job is in_progress by definition -- it is the one asking --
  // and must not keep itself waiting.
  assert.deepEqual(unfinishedJobs(jobs, prefix), ['compile: shotium-linux-amd64-v0.4.0 1/4']);
  // A failed shard is finished: it will not upload, and waiting for it is
  // waiting for the timeout.
  assert.equal(unfinishedJobs(jobs.filter((job) => !job.name.endsWith('1/4')), prefix).length, 0);
  assert.equal(matchingJobs(jobs, prefix).length, 3);
  // A prefix that matches nothing is a naming drift, and the caller fails on
  // it rather than assuming the shards are done.
  assert.deepEqual(matchingJobs(jobs, 'compile: shotium-linux-arm64'), []);
});
