// Dispatch engine.yml by hand and, optionally, wait for it.
//
// why: engine.yml runs on its own -- on every push to main that touches an
// engine input, and inside every preview and publish -- so the everyday
// case needs nobody to press anything. What is left for a person is the
// forced rebuild: the runner image moved, a toolchain the fingerprint
// cannot see changed, or an artifact is suspected and should be replaced.
// That is one dispatch with force=true, which this wraps so the arguments
// are spelled once.
//
//   pnpm ci:dispatch-engines                            # build what is missing on main
//   pnpm ci:dispatch-engines --force --wait             # rebuild all six and wait
//   pnpm ci:dispatch-engines --only windows-amd64,linux-amd64 --ref my-branch
//
// The dispatch API does not return the run it created, so this finds the
// run the way the release procedure does: by workflow and ref, newest
// first, created after the dispatch. `gh` must be authenticated; nothing
// here needs the repository checked out.

import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import pc from 'picocolors';

import {platformLabels, selectPlatforms} from '../lib/platforms.ts';
import {sleep} from '../lib/repo.ts';

export const WORKFLOW = 'engine.yml';

export interface Dispatch {
  ref: string;
  repo: string;
  force: boolean;
  /** `all`, or a comma-separated subset of platform labels. */
  targets: string;
  shards: string;
  /** ninja -j; empty keeps each platform's default. */
  jobs: string;
}

export function dispatchArgs(d: Dispatch): string[] {
  selectPlatforms(d.targets);   // rejects a label that is not a platform before anything is sent
  return ['workflow', 'run', WORKFLOW, '-R', d.repo, '--ref', d.ref,
    '-f', `force=${d.force}`, '-f', `targets=${d.targets}`, '-f', `shards=${d.shards}`, '-f', `jobs=${d.jobs}`];
}

const gh = (args: string[]): Promise<{stdout: string}> => execa('gh', args, {stdio: ['ignore', 'pipe', 'inherit']});

async function findRun(ref: string, repo: string, after: number): Promise<string> {
  for (let attempt = 0; attempt < 30; attempt++) {
    const {stdout} = await gh(['run', 'list', '-R', repo, '--workflow', WORKFLOW,
      '--branch', ref, '--limit', '5', '--json', 'databaseId,createdAt,event']);
    const runs = JSON.parse(stdout) as {databaseId: number; createdAt: string; event: string}[];
    const mine = runs.filter((r) => r.event === 'workflow_dispatch' && Date.parse(r.createdAt) >= after);
    if (mine.length > 0) return String(mine[0].databaseId);
    await sleep(4000);
  }
  return '';
}

async function waitFor(id: string, repo: string): Promise<number> {
  for (;;) {
    const {stdout} = await gh(['api', `repos/${repo}/actions/runs/${id}`, '--jq', '"\\(.status)\\t\\(.conclusion)"']);
    const [status, conclusion] = stdout.trim().split('\t');
    if (status === 'completed') {
      console.log(conclusion === 'success' ? pc.green(`run ${id} succeeded`) : pc.red(`run ${id} ended as ${conclusion}`));
      return conclusion === 'success' ? 0 : 1;
    }
    await sleep(60_000);
  }
}

// tsx runs this file for the command and imports it for the tests. Without
// this guard the import parses a command line and dispatches: running the
// test suite once sent six engine builds, and checks.yml runs that suite.
if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('pnpm ci:dispatch-engines');
  cli.command('', 'dispatch engine.yml')
    .option('--ref <ref>', 'branch or tag to build', {default: 'main'})
    .option('--repo <owner/name>', 'repository', {default: 'sj817/shotium'})
    .option('--force', 'rebuild even where artifacts exist at this fingerprint')
    .option('--only <labels>', `comma-separated subset of ${platformLabels.join(', ')}`, {default: 'all'})
    .option('--shards <n>', 'slices per platform; auto is the per-platform default', {default: 'auto'})
    .option('--jobs <n>', 'ninja -j; empty keeps each platform its default', {default: ''})
    .option('--wait', 'poll until the run finishes, and exit non-zero if it failed')
    .option('--dry-run', 'print the gh command and stop')
    .action(async (options: {ref: string; repo: string; force?: boolean; only: string; shards: string; jobs: string; wait?: boolean; dryRun?: boolean}) => {
      let args: string[];
      try {
        args = dispatchArgs({ref: options.ref, repo: options.repo, force: options.force ?? false, targets: options.only, shards: options.shards, jobs: options.jobs});
      } catch (error) {
        console.error((error as Error).message);
        process.exitCode = 2;
        return;
      }
      if (options.dryRun) {
        console.log('gh ' + args.join(' '));
        return;
      }
      const at = Date.now() - 5000;   // the run's createdAt can precede our clock
      await gh(args);
      const id = await findRun(options.ref, options.repo, at);
      if (!id) {
        console.log(pc.yellow('dispatched, but the run did not appear in the listing'));
        return;
      }
      console.log(`${pc.cyan(WORKFLOW)} https://github.com/${options.repo}/actions/runs/${id}`);
      if (options.wait) process.exitCode = await waitFor(id, options.repo);
    });
  cli.help();
  cli.parse();
}
