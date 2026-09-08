// Warm release/documentation runs need one runner, not N copies of source sync.
// This only selects parallelism: ninja and all existing checks still run.
import {appendFileSync} from 'node:fs';
import path from 'node:path';

export function changesNeedCompilation(files: string[]): boolean {
  return files.some((file) => !(
    file === 'shotium/package.json' ||
    /^(README(?:\.zh)?\.md|CLAUDE\.md|AGENTS\.md|LICENSE)$/.test(file) ||
    file.startsWith('docs/') ||
    (file.startsWith('shotium/') && file.endsWith('.md'))
  ));
}

export function shardCount(requested: string, platform: string, cpu: string): number {
  if (requested === 'auto') return platform === 'windows' ? 4 : platform === 'linux' ? (cpu === 'arm64' ? 3 : 4) : (cpu === 'arm64' ? 3 : 2);
  if (!/^[1-9]\d*$/.test(requested) || Number(requested) > 20) throw new Error('shards must be auto or an integer from 1 to 20');
  return Number(requested);
}

async function main(): Promise<void> {
  const {GITHUB_REPOSITORY: repo, GITHUB_REF: ref, GITHUB_SHA: sha, GH_TOKEN: token,
    GITHUB_OUTPUT: output, SHOT_PLATFORM: platform = '', SHOT_CPU: cpu = '', SHOT_SHARDS: requested = 'auto'} = process.env;
  let count = shardCount(requested, platform, cpu);
  if (!['windows', 'linux', 'macos'].includes(platform) || !['x64', 'arm64'].includes(cpu)) throw new Error('invalid platform/cpu');
  if (requested === 'auto') {
    try {
      const api = async (route: string) => {
        const response = await fetch(`https://api.github.com/repos/${repo}${route ? `/${route}` : ''}`, {
          headers: {Authorization: `Bearer ${token}`, Accept: 'application/vnd.github+json'},
          signal: AbortSignal.timeout(20_000),
        });
        if (!response.ok) throw new Error(`GitHub API ${response.status}`);
        return response.json();
      };
      const prefix = `out-shot-${platform}-${cpu}-`;
      // Match the branch visibility and newest-first order used by cache restore.
      const repository = await api('');
      let caches: {key: string}[] = [];
      for (const branch of [...new Set([ref, `refs/heads/${repository.default_branch}`])]) {
        const result = await api(`actions/caches?key=${prefix}&ref=${encodeURIComponent(branch!)}&sort=created_at&direction=desc&per_page=1`);
        if (result.actions_caches.length) { caches = result.actions_caches; break; }
      }
      const runId = caches[0]?.key.slice(prefix.length);
      if (runId && /^\d+$/.test(runId)) {
        const run = await api(`actions/runs/${runId}`);
        if (run.conclusion === 'success') {
          const comparison = await api(`compare/${run.head_sha}...${sha}`);
          const files = comparison.files as {filename: string; previous_filename?: string}[] | undefined;
          // GitHub truncates this list at 300. Unknown history stays parallel.
          if (['ahead', 'identical'].includes(comparison.status) && files && files.length < 300 &&
              !changesNeedCompilation(files.flatMap((f) => [f.filename, ...(f.previous_filename ? [f.previous_filename] : [])]))) {
            count = 1;
            console.log(`Warm cache ${caches[0].key}; no engine input changes: one runner.`);
          }
        }
      }
    } catch (error) {
      console.log(`Could not prove warm cache; retaining ${count} shards: ${String(error)}`);
    }
  }
  console.log(`Selected ${count} shard(s) for ${platform}-${cpu}.`);
  if (!output) throw new Error('GITHUB_OUTPUT is required');
  appendFileSync(output, `count=${count}\nfinal=${count - 1}\nlist=${JSON.stringify(Array.from({length: count - 1}, (_, i) => i))}\n`);
}

if (process.argv[1] && path.basename(process.argv[1]) === 'ci-select-shards.ts') {
  main().catch((error: unknown) => { console.error(error); process.exitCode = 1; });
}
