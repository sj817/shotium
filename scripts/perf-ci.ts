// Validate exact-SHA engine artifacts before placing them in an npm-shaped
// candidate installation. Never substitute a release binary for a missing
// build.
//
//   pnpm perf:ci plan                       # in performance-regression.yml's resolve job
//   pnpm perf:ci stage <download> <dest>    # in each platform job
//
// `plan` reads BUILD_RUNS (a JSON object of platform -> engine run id) and
// BASELINE_VERSION from the environment, checks each run against the GitHub
// API, and writes performance-plan.json plus the job matrix to GITHUB_OUTPUT.
// `stage` unpacks the one downloaded platform tarball into a directory shaped
// like an installed @shotkit/shotium.

import {createHash} from 'node:crypto';
import {appendFileSync, copyFileSync, cpSync, existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';

import {resolve} from './lib/repo.ts';

// npm platform id, runner, engine workflow, artifact label. The label is the
// platform id the engine workflows put on their artifacts and job names
// (windows/linux/macos, amd64/arm64); npm keeps process.platform's spelling.
const targets: Array<[string, string, 'linux' | 'windows' | 'macos', string]> = [
  ['linux-x64', 'ubuntu-24.04', 'linux', 'linux-amd64'], ['linux-arm64', 'ubuntu-24.04-arm', 'linux', 'linux-arm64'],
  ['win32-x64', 'windows-2025', 'windows', 'windows-amd64'], ['win32-arm64', 'windows-11-arm', 'windows', 'windows-arm64'],
  ['darwin-x64', 'macos-15-intel', 'macos', 'macos-amd64'], ['darwin-arm64', 'macos-15', 'macos', 'macos-arm64'],
];

async function plan(): Promise<void> {
  const runs = JSON.parse(process.env.BUILD_RUNS ?? '{}') as Record<string, string | number>;
  const expected = targets.map(([platform]) => platform).sort();
  if (JSON.stringify(Object.keys(runs).sort()) !== JSON.stringify(expected)) throw new Error('All six build run IDs are required');
  if (!/^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$/.test(process.env.BASELINE_VERSION ?? '')) throw new Error('Baseline must be an exact npm version');
  const api = async (suffix: string) => {
    const response = await fetch(`https://api.github.com/repos/${process.env.GITHUB_REPOSITORY}/${suffix}`, {
      headers: {authorization: `Bearer ${process.env.GH_TOKEN}`, accept: 'application/vnd.github+json'},
      signal: AbortSignal.timeout(30000),
    });
    if (!response.ok) throw new Error(`GitHub ${response.status}: ${suffix}`);
    return response.json() as Promise<Record<string, unknown>>;
  };
  const matrix = [];
  for (const [platform, runner, workflowOs, label] of targets) {
    const runId = String(runs[platform]);
    if (!/^\d+$/.test(runId)) throw new Error(`Invalid run ID for ${platform}`);
    const run = await api(`actions/runs/${runId}`) as {path: string; head_sha: string; status: string; conclusion: string};
    if (run.path !== `.github/workflows/engine-${workflowOs}.yml`) {
      throw new Error(`${platform}: artifact must come from the matching engine build workflow`);
    }
    if (run.head_sha !== process.env.GITHUB_SHA || run.status !== 'completed' || run.conclusion !== 'success') {
      throw new Error(`${platform}: build must be successful at this workflow SHA ${process.env.GITHUB_SHA}; got ${run.head_sha}/${run.conclusion}`);
    }
    const artifactName = `npm-shotium-${label}`;
    const artifacts = await api(`actions/runs/${runId}/artifacts?per_page=100`) as {artifacts: Array<{name: string; expired: boolean; id: number; digest?: string}>};
    const matching = artifacts.artifacts.filter((a) => a.name === artifactName && !a.expired);
    if (matching.length !== 1) throw new Error(`${platform}: exact build artifact missing or ambiguous`);
    matrix.push({platform, label, runner, runId, artifactName, artifactId: matching[0].id, artifactDigest: matching[0].digest, sourceSha: run.head_sha});
  }
  writeFileSync('performance-plan.json', JSON.stringify(matrix, null, 2));
  appendFileSync(process.env.GITHUB_OUTPUT!, `matrix=${JSON.stringify(matrix)}\n`);
}

async function stage(downloadArg: string, destinationArg: string): Promise<void> {
  const download = resolve(downloadArg), destination = resolve(destinationArg);
  const tarballs = readdirSync(download).filter((f) => f.endsWith('.tgz'));
  if (tarballs.length !== 1) throw new Error('Expected exactly one platform tarball');
  if (existsSync(destination)) throw new Error('Candidate destination must be new');
  mkdirSync(destination, {recursive: true});
  copyFileSync(resolve('shotium/package.json'), path.join(destination, 'package.json'));
  cpSync(resolve('shotium/dist'), path.join(destination, 'dist'), {recursive: true});
  const platform = `${process.platform}-${process.arch}`;
  const platformDirectory = path.join(destination, 'node_modules/@shotkit', `shotium-${platform}`);
  mkdirSync(platformDirectory, {recursive: true});
  const tarball = path.join(download, tarballs[0]);
  await execa('tar', ['-xzf', tarball, '--strip-components=1', '-C', platformDirectory], {windowsHide: true});
  const manifest = JSON.parse(readFileSync(path.join(platformDirectory, 'package.json'), 'utf8')) as {name: string};
  if (manifest.name !== `@shotkit/shotium-${platform}`) throw new Error('Wrong platform artifact');
  writeFileSync(path.join(destination, 'provenance.json'), JSON.stringify({
    sourceSha: process.env.GITHUB_SHA, platform,
    tarballSha256: createHash('sha256').update(readFileSync(tarball)).digest('hex'),
  }, null, 2));
}

const cli = cac('perf-ci');
cli.command('plan', 'check the six engine runs and write the job matrix')
    .action(() => plan().catch((error) => {
      console.error(error);
      process.exitCode = 1;
    }));
cli.command('stage <download> <destination>', 'unpack the platform tarball into an npm-shaped candidate')
    .action((download: string, destination: string) => stage(download, destination).catch((error) => {
      console.error(error);
      process.exitCode = 1;
    }));
cli.help();
cli.parse(process.argv, {run: false});
if (!cli.matchedCommand && !cli.options.help) {
  console.error('Use plan or stage');
  process.exitCode = 2;
} else {
  await cli.runMatchedCommand();
}
