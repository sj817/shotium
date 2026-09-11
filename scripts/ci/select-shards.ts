// How many runners an engine build gets, and which saved build directory
// they all start from.
//
// why: a cold build of this tree needs four shards to finish inside the job
// limit; a warm one -- the build directory saved by the previous run, with
// nothing changed since -- needs one, and three more would each pay the
// full source setup to compile nothing. The build directories are
// artifacts named build-dir-<platform>, and each carries a marker naming
// the engine fingerprint it was saved at. If a marker for this run's
// fingerprint exists beside a blob, nothing needs compiling: one runner,
// which restores that blob and relinks. Otherwise the newest trusted blob
// is the warm start and the platform default decides the shard count.
//
// This only selects parallelism and the starting point: ninja and all the
// checks still run. Anything that goes wrong here -- no token, an API
// hiccup, no artifact yet -- falls back to a cold-shaped run, which is
// slower and never wrong.
import {appendFileSync} from 'node:fs';
import path from 'node:path';

import {platforms} from '../lib/platforms.ts';
import {environment, findBuildDir} from './engine-artifacts.ts';

export function shardCount(requested: string, platform: string, cpu: string): number {
  if (requested === 'auto') return platform === 'windows' ? 4 : platform === 'linux' ? (cpu === 'arm64' ? 3 : 4) : (cpu === 'arm64' ? 3 : 2);
  if (!/^[1-9]\d*$/.test(requested) || Number(requested) > 20) throw new Error('shards must be auto or an integer from 1 to 20');
  return Number(requested);
}

export interface Selection {
  count: number;
  buildDirRunId: number | null;
  reason: string;
}

export async function select(options: {
  requested: string; platform: string; cpu: string; fingerprint: string | undefined;
  lookup: (label: string, fingerprint: string | undefined) => Promise<{runId: number; exact: boolean} | null>;
}): Promise<Selection> {
  const target = platforms.find((p) => p.os === options.platform && p.cpu === options.cpu);
  if (!target) throw new Error(`invalid platform/cpu ${options.platform}/${options.cpu}`);
  const count = shardCount(options.requested, options.platform, options.cpu);
  let dir: {runId: number; exact: boolean} | null = null;
  try {
    dir = await options.lookup(target.label, options.fingerprint);
  } catch (error) {
    return {count, buildDirRunId: null, reason: `could not look up a build directory; ${count} shard(s) from nothing: ${String(error)}`};
  }
  if (!dir) return {count, buildDirRunId: null, reason: `no saved build directory for ${target.label}; ${count} shard(s) from nothing`};
  if (dir.exact && options.requested === 'auto') {
    return {count: 1, buildDirRunId: dir.runId, reason: `run ${dir.runId} saved a build directory at this fingerprint: one runner, nothing to compile`};
  }
  return {count, buildDirRunId: dir.runId, reason: `warm start from run ${dir.runId}; ${count} shard(s)`};
}

export function outputs(selection: Selection): string {
  const {count, buildDirRunId} = selection;
  return `count=${count}\nfinal=${count - 1}\nlist=${JSON.stringify(Array.from({length: count - 1}, (_, i) => i))}\nbuild_dir_run_id=${buildDirRunId ?? ''}\n`;
}

async function main(): Promise<void> {
  const {GITHUB_OUTPUT: output, SHOT_PLATFORM: platform = '', SHOT_CPU: cpu = '', SHOT_SHARDS: requested = 'auto', SHOT_FINGERPRINT: fingerprint} = process.env;
  const selection = await select({
    requested, platform, cpu, fingerprint: fingerprint || undefined,
    lookup: async (label, fp) => {
      const {api, currentRunId} = environment();
      return findBuildDir(api, label, fp, currentRunId);
    },
  });
  console.log(`${selection.reason}.`);
  if (!output) throw new Error('GITHUB_OUTPUT is required');
  appendFileSync(output, outputs(selection));
}

if (process.argv[1] && path.basename(process.argv[1]) === 'select-shards.ts') {
  main().catch((error: unknown) => { console.error(error); process.exitCode = 1; });
}
