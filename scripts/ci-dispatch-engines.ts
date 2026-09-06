// Dispatch the six engine builds and, optionally, wait for them.
//
// why: two procedures need the same six lines. A release builds all six on
// the version-bump commit (.claude/skills/release), and every merge that
// touches the engine should leave a warm build directory on the default
// branch -- because an Actions cache is visible only to the branch that
// wrote it and to `main`, so a run on a feature branch restores nothing
// unless `main` has one. Eleven caches in a row were written on feature
// branches, and every new branch therefore started cold: 86 minutes on
// Windows against 20 warm.
//
//   pnpm ci:dispatch-engines                      # all six on main
//   pnpm ci:dispatch-engines --ref my-branch --no-checks
//   pnpm ci:dispatch-engines --only windows-amd64,linux-amd64 --wait
//
// The dispatch API does not return the run it created, so this finds each
// run the way the release procedure does: by workflow and ref, newest first,
// created after the dispatch. `gh` must be authenticated; nothing here needs
// the repository checked out.

import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';
import pc from 'picocolors';

import {sleep} from './lib/repo.ts';

export interface Target {
  id: string;
  workflow: string;
  arch: string;
  /** Linux and macOS default to `probe`, which compiles nothing. */
  build: boolean;
}

export const TARGETS: Target[] = [
  {id: 'windows-amd64', workflow: 'engine-windows.yml', arch: 'amd64', build: false},
  {id: 'windows-arm64', workflow: 'engine-windows.yml', arch: 'arm64', build: false},
  {id: 'linux-amd64', workflow: 'engine-linux.yml', arch: 'amd64', build: true},
  {id: 'linux-arm64', workflow: 'engine-linux.yml', arch: 'arm64', build: true},
  {id: 'macos-amd64', workflow: 'engine-macos.yml', arch: 'amd64', build: true},
  {id: 'macos-arm64', workflow: 'engine-macos.yml', arch: 'arm64', build: true},
];

export function selectTargets(only: string | undefined): Target[] {
  if (!only) return TARGETS;
  const wanted = only.split(',').map((s) => s.trim()).filter(Boolean);
  const unknown = wanted.filter((w) => !TARGETS.some((t) => t.id === w));
  if (unknown.length > 0) {
    throw new Error(`unknown target(s) ${unknown.join(', ')}; known: ${TARGETS.map((t) => t.id).join(', ')}`);
  }
  return TARGETS.filter((t) => wanted.includes(t.id));
}

export function dispatchArgs(target: Target, ref: string, repo: string, checks: boolean, shards: string): string[] {
  const args = ['workflow', 'run', target.workflow, '-R', repo, '--ref', ref, '-f', `arch=${target.arch}`];
  if (target.build) args.push('-f', 'mode=build');
  args.push('-f', `run_checks=${checks}`, '-f', `shards=${shards}`);
  return args;
}

const gh = (args: string[]): Promise<{stdout: string}> => execa('gh', args, {stdio: ['ignore', 'pipe', 'inherit']});

// The dispatch returns nothing identifying, so poll the list until a run of
// this workflow and ref appears that did not exist a moment ago.
async function findRun(target: Target, ref: string, repo: string, after: number): Promise<string> {
  for (let attempt = 0; attempt < 30; attempt++) {
    const {stdout} = await gh(['run', 'list', '-R', repo, '--workflow', target.workflow,
      '--branch', ref, '--limit', '5', '--json', 'databaseId,createdAt,event']);
    const runs = JSON.parse(stdout) as {databaseId: number; createdAt: string; event: string}[];
    const mine = runs.filter((r) => r.event === 'workflow_dispatch' && Date.parse(r.createdAt) >= after);
    if (mine.length > 0) return String(mine[0].databaseId);
    await sleep(4000);
  }
  return '';
}

async function waitFor(ids: string[], repo: string): Promise<number> {
  const done = new Map<string, string>();
  while (done.size < ids.length) {
    for (const id of ids) {
      if (done.has(id)) continue;
      const {stdout} = await gh(['api', `repos/${repo}/actions/runs/${id}`, '--jq', '"\\(.status)\\t\\(.conclusion)"']);
      const [status, conclusion] = stdout.trim().split('\t');
      if (status === 'completed') {
        done.set(id, conclusion);
        const mark = conclusion === 'success' ? pc.green('success') : pc.red(conclusion);
        console.log(`  ${id} ${mark} (${done.size}/${ids.length})`);
      }
    }
    if (done.size < ids.length) await sleep(60_000);
  }
  const failed = [...done].filter(([, c]) => c !== 'success');
  console.log(failed.length === 0
    ? pc.green(`all ${ids.length} runs succeeded`)
    : pc.red(`${failed.length} of ${ids.length} runs did not: ${failed.map(([id]) => id).join(', ')}`));
  return failed.length === 0 ? 0 : 1;
}

// tsx runs this file for the command and imports it for the tests. Without
// this guard the import parses a command line and dispatches: running the
// test suite once sent six engine builds, and checks.yml runs that suite.
if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('ci-dispatch-engines');
  cli.command('', 'dispatch the engine builds')
    .option('--ref <ref>', 'branch or tag to build', {default: 'main'})
    .option('--repo <owner/name>', 'repository', {default: 'sj817/shotium'})
    .option('--only <ids>', 'comma-separated subset, e.g. windows-amd64,linux-arm64')
    .option('--shards <n>', 'slices per platform; auto is the per-platform default', {default: 'auto'})
    .option('--no-checks', 'skip the check suites (a cache-warming run does not need them)')
    .option('--wait', 'poll until every run finishes, and exit non-zero if any failed')
    .option('--dry-run', 'print the gh commands and stop')
    .action(async (options: {ref: string; repo: string; only?: string; shards: string; checks: boolean; wait?: boolean; dryRun?: boolean}) => {
      let targets: Target[];
      try {
        targets = selectTargets(options.only);
      } catch (error) {
        console.error((error as Error).message);
        process.exitCode = 2;
        return;
      }
      const ids: string[] = [];
      for (const target of targets) {
        const args = dispatchArgs(target, options.ref, options.repo, options.checks, options.shards);
        if (options.dryRun) {
          console.log('gh ' + args.join(' '));
          continue;
        }
        const at = Date.now() - 5000;   // the run's createdAt can precede our clock
        await gh(args);
        const id = await findRun(target, options.ref, options.repo, at);
        if (id) {
          ids.push(id);
          console.log(`${pc.cyan(target.id.padEnd(14))} https://github.com/${options.repo}/actions/runs/${id}`);
        } else {
          console.log(`${pc.cyan(target.id.padEnd(14))} ${pc.yellow('dispatched, but the run did not appear in the listing')}`);
        }
      }
      if (options.dryRun || !options.wait || ids.length === 0) return;
      console.log(`waiting for ${ids.length} runs`);
      process.exitCode = await waitFor(ids, options.repo);
    });
  cli.help();
  cli.parse();
}
