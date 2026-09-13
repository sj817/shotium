// Select build parallelism and a platform-isolated warm build directory.
import {appendFileSync} from 'node:fs';
import path from 'node:path';

import {platformByLabel} from '../lib/platforms.ts';
import {environment, findBuildDir} from './engine-artifacts.ts';

export function shardCount(requested: string, platform: string, cpu: string): number {
  if (requested === 'auto') return platform === 'windows' ? 4 : platform === 'linux' ? (cpu === 'arm64' ? 3 : 4) : (cpu === 'arm64' ? 3 : 2);
  if (!/^[1-9]\d*$/.test(requested) || Number(requested) > 20) throw new Error('shards must be auto or an integer from 1 to 20');
  return Number(requested);
}

export interface Selection {
  count: number;
  buildDirRunId: number|null;
  reason: string;
}

export async function select(options: {
  requested: string; target: string; fingerprint: string|undefined;
  lookup: (label: string, fingerprint: string|undefined) => Promise<{runId: number; exact: boolean}|null>;
}): Promise<Selection> {
  const target = platformByLabel(options.target);
  const count = shardCount(options.requested, target.os, target.cpu);
  let dir: {runId: number; exact: boolean}|null = null;
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
  const {GITHUB_OUTPUT: output, SHOT_TARGET: target = '', SHOT_SHARDS: requested = 'auto', SHOT_FINGERPRINT: fingerprint} = process.env;
  const selection = await select({
    requested, target, fingerprint: fingerprint || undefined,
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
