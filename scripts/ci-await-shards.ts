// Wait, inside the final job, for the compile shards of the same run.
//
// why: the final job used to start only once every shard had finished, and
// its first thirteen minutes then repeated the setup the shards had already
// done -- source, DEPS, gn gen. That is 15% of a Windows run, paid a second
// time for nothing. It now starts with the shards, builds the last slice
// itself, and waits here for the others' artifacts, so a run costs one setup
// instead of two and a six-platform dispatch is 20 jobs rather than 26.
//
// GitHub has no "wait until an artifact exists" primitive, so this polls the
// run's own artifact list. The exit is deliberately forgiving: when every
// shard job has finished and an artifact is still missing -- a shard that
// died, or one whose upload failed -- it says so and returns 0, because the
// final job's own ninja can still build what did not arrive. The one thing
// it refuses to do is wait for jobs that do not exist: if no job name
// matches --job-prefix the naming has drifted, and returning 0 there would
// turn a typo into a silent full rebuild an hour long.
//
//   pnpm ci:await-shards --prefix shard-linux-amd64- --shards 4 \
//     --job-prefix "compile: shotium-linux-amd64"
//
// --shards is the number of slices the run was split into, this job's
// included, so it waits for indices 0 .. n-2. The repository, the run and
// the token come from the environment GitHub already provides.

import path from 'node:path';

import {cac} from 'cac';
import pRetry from 'p-retry';
import pc from 'picocolors';

import {sleep} from './lib/repo.ts';

export interface Artifact {
  name: string;
}

export interface Job {
  name: string;
  status: string;
  conclusion: string | null;
}

export function missingArtifacts(expected: string[], present: Iterable<string>): string[] {
  const have = new Set(present);
  return expected.filter((name) => !have.has(name));
}

// A shard job that has not reached "completed" may still upload. Anything
// else -- success, failure, cancelled -- is as done as it will ever be.
export function unfinishedJobs(jobs: Job[], prefix: string): string[] {
  return jobs.filter((job) => job.name.startsWith(prefix) && job.status !== 'completed').map((job) => job.name);
}

export function matchingJobs(jobs: Job[], prefix: string): string[] {
  return jobs.filter((job) => job.name.startsWith(prefix)).map((job) => job.name);
}

async function api<T>(url: string, token: string): Promise<T> {
  return pRetry(async () => {
    const response = await fetch(url, {
      headers: {
        accept: 'application/vnd.github+json',
        authorization: `Bearer ${token}`,
        'x-github-api-version': '2022-11-28',
      },
    });
    if (!response.ok) {
      const error = new Error(`${response.status} ${response.statusText} for ${url}`);
      // A 4xx is a wrong URL or a token without actions:read; retrying it
      // only makes the job take longer to say so. 429 is the exception.
      if (response.status < 500 && response.status !== 429) throw Object.assign(error, {shouldRetry: false});
      throw error;
    }
    return await response.json() as T;
  }, {
    retries: 4,
    shouldRetry: ({error}) => (error as {shouldRetry?: boolean}).shouldRetry !== false,
    onFailedAttempt: ({error, retriesLeft}) => console.error(pc.yellow(`  ${error.message}; ${retriesLeft} tries left`)),
  });
}

interface Options {
  repo: string;
  runId: string;
  prefix: string;
  shards: number;
  jobPrefix: string;
  timeout: number;
  interval: number;
}

async function waitForShards(options: Options, token: string): Promise<number> {
  const expected = Array.from({length: options.shards - 1}, (_, i) => `${options.prefix}${i}`);
  if (expected.length === 0) {
    console.log('one slice in this run, and this job is it: nothing to wait for');
    return 0;
  }
  const base = `https://api.github.com/repos/${options.repo}/actions/runs/${options.runId}`;
  const deadline = Date.now() + options.timeout * 60_000;
  const seen = new Set<string>();
  let sawJobs = false;
  // Two extra rounds after the last shard job finishes: an artifact becomes
  // visible a moment before the job that uploaded it is marked completed,
  // and the reverse order has been seen too.
  let grace = 2;

  console.log(`waiting for ${expected.length} shard artifacts of run ${options.runId}: ${expected.join(', ')}`);
  for (;;) {
    const {artifacts} = await api<{artifacts: Artifact[]}>(`${base}/artifacts?per_page=100`, token);
    for (const name of artifacts.map((artifact) => artifact.name)) {
      if (expected.includes(name) && !seen.has(name)) {
        seen.add(name);
        console.log(pc.green(`  ${name} (${seen.size}/${expected.length})`));
      }
    }
    const missing = missingArtifacts(expected, seen);
    if (missing.length === 0) {
      console.log(`all ${expected.length} shards uploaded`);
      return 0;
    }

    const {jobs} = await api<{jobs: Job[]}>(`${base}/jobs?per_page=100`, token);
    const mine = matchingJobs(jobs, options.jobPrefix);
    if (mine.length === 0) {
      if (sawJobs) return 0;   // they were there and the listing lost them; do not hang
      console.error(pc.red(`no job of this run is named "${options.jobPrefix}..."; the run has:`));
      for (const job of jobs) console.error(`  ${job.name}`);
      return 1;
    }
    sawJobs = true;
    if (unfinishedJobs(jobs, options.jobPrefix).length === 0 && grace-- <= 0) {
      console.error(pc.yellow(`every shard job has finished and ${missing.length} artifact(s) never appeared: ${missing.join(', ')}`));
      console.error(pc.yellow('this job will build what is missing itself'));
      return 0;
    }
    if (Date.now() > deadline) {
      console.error(pc.red(`gave up after ${options.timeout} minutes; still missing ${missing.join(', ')}`));
      return 1;
    }
    await sleep(options.interval * 1000);
  }
}

// tsx runs this file for the command and imports it for the tests; only
// the first should parse a command line.
if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('ci-await-shards');
  cli.command('', 'wait for this run\'s other compile shards to upload their artifacts')
    .option('--prefix <p>', 'artifact name prefix; indices 0 .. shards-2 are appended')
    .option('--shards <n>', 'slices this run was split into, this job included')
    .option('--job-prefix <p>', 'name prefix of the shard jobs to watch')
    .option('--repo <owner/name>', 'repository', {default: process.env.GITHUB_REPOSITORY ?? ''})
    .option('--run-id <id>', 'workflow run', {default: process.env.GITHUB_RUN_ID ?? ''})
    .option('--timeout <minutes>', 'give up after this long', {default: '240'})
    .option('--interval <seconds>', 'time between polls', {default: '20'})
    .action(async (raw: Record<string, string>) => {
      const token = process.env.GITHUB_TOKEN ?? process.env.GH_TOKEN ?? '';
      const options: Options = {
        repo: raw.repo,
        runId: raw.runId,
        prefix: raw.prefix ?? '',
        shards: Number(raw.shards),
        jobPrefix: raw.jobPrefix ?? '',
        timeout: Number(raw.timeout),
        interval: Number(raw.interval),
      };
      const bad = !token ? 'GITHUB_TOKEN (or GH_TOKEN) is not set'
        : !options.repo ? '--repo, or GITHUB_REPOSITORY, is not set'
        : !options.runId ? '--run-id, or GITHUB_RUN_ID, is not set'
        : !options.prefix ? '--prefix is required'
        : !options.jobPrefix ? '--job-prefix is required'
        : !Number.isInteger(options.shards) || options.shards < 1 ? `--shards must be a positive integer, got ${raw.shards}`
        : !Number.isFinite(options.timeout) || !Number.isFinite(options.interval) ? 'bad --timeout or --interval'
        : '';
      if (bad) {
        console.error(bad);
        process.exitCode = 2;
        return;
      }
      process.exitCode = await waitForShards(options, token);
    });
  cli.help();
  cli.parse();
}
