'use strict';
// Validate exact-SHA engine artifacts before placing them in an npm-shaped
// candidate installation. Never substitute a release binary for a missing build.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const {execFileSync} = require('node:child_process');
// npm platform id, runner, engine workflow, artifact label. The label is the
// platform id the engine workflows put on their artifacts and job names
// (windows/linux/macos, amd64/arm64); npm keeps process.platform's spelling.
const targets = [
  ['linux-x64', 'ubuntu-24.04', 'linux', 'linux-amd64'], ['linux-arm64', 'ubuntu-24.04-arm', 'linux', 'linux-arm64'],
  ['win32-x64', 'windows-2025', 'windows', 'windows-amd64'], ['win32-arm64', 'windows-11-arm', 'windows', 'windows-arm64'],
  ['darwin-x64', 'macos-15-intel', 'macos', 'macos-amd64'], ['darwin-arm64', 'macos-15', 'macos', 'macos-arm64'],
];

async function plan() {
  const runs = JSON.parse(process.env.BUILD_RUNS);
  const expected = targets.map(([platform]) => platform).sort();
  if (JSON.stringify(Object.keys(runs).sort()) !== JSON.stringify(expected)) throw new Error('All six build run IDs are required');
  if (!/^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$/.test(process.env.BASELINE_VERSION)) throw new Error('Baseline must be an exact npm version');
  const api = async suffix => {
    const response = await fetch(`https://api.github.com/repos/${process.env.GITHUB_REPOSITORY}/${suffix}`, {
      headers: {authorization: `Bearer ${process.env.GH_TOKEN}`, accept: 'application/vnd.github+json'},
      signal: AbortSignal.timeout(30000)});
    if (!response.ok) throw new Error(`GitHub ${response.status}: ${suffix}`);
    return response.json();
  };
  const matrix = [];
  for (const [platform, runner, workflowOs, label] of targets) {
    const runId = String(runs[platform]);
    if (!/^\d+$/.test(runId)) throw new Error(`Invalid run ID for ${platform}`);
    const run = await api(`actions/runs/${runId}`);
    if (run.path !== `.github/workflows/engine-${workflowOs}.yml`) {
      throw new Error(`${platform}: artifact must come from the matching engine build workflow`);
    }
    if (run.head_sha !== process.env.GITHUB_SHA || run.status !== 'completed' || run.conclusion !== 'success') {
      throw new Error(`${platform}: build must be successful at this workflow SHA ${process.env.GITHUB_SHA}; got ${run.head_sha}/${run.conclusion}`);
    }
    const artifactName = `npm-shotium-${label}`;
    const artifacts = await api(`actions/runs/${runId}/artifacts?per_page=100`);
    const matching = artifacts.artifacts.filter(a => a.name === artifactName && !a.expired);
    if (matching.length !== 1) throw new Error(`${platform}: exact build artifact missing or ambiguous`);
    matrix.push({platform, label, runner, runId, artifactName, artifactId: matching[0].id,
      artifactDigest: matching[0].digest, sourceSha: run.head_sha});
  }
  fs.writeFileSync('performance-plan.json', JSON.stringify(matrix, null, 2));
  fs.appendFileSync(process.env.GITHUB_OUTPUT, `matrix=${JSON.stringify(matrix)}\n`);
}

function stage() {
  const [download, destination] = process.argv.slice(3).map(p => path.resolve(p));
  const tarballs = fs.readdirSync(download).filter(f => f.endsWith('.tgz'));
  if (tarballs.length !== 1) throw new Error('Expected exactly one platform tarball');
  if (fs.existsSync(destination)) throw new Error('Candidate destination must be new');
  fs.mkdirSync(destination, {recursive: true});
  fs.copyFileSync('shotium/package.json', path.join(destination, 'package.json'));
  fs.cpSync('shotium/dist', path.join(destination, 'dist'), {recursive: true});
  const platform = `${process.platform}-${process.arch}`;
  const platformDirectory = path.join(destination, 'node_modules/@shotkit', `shotium-${platform}`);
  fs.mkdirSync(platformDirectory, {recursive: true});
  const tarball = path.join(download, tarballs[0]);
  execFileSync('tar', ['-xzf', tarball, '--strip-components=1', '-C', platformDirectory], {windowsHide: true});
  const manifest = JSON.parse(fs.readFileSync(path.join(platformDirectory, 'package.json')));
  if (manifest.name !== `@shotkit/shotium-${platform}`) throw new Error('Wrong platform artifact');
  fs.writeFileSync(path.join(destination, 'provenance.json'), JSON.stringify({
    sourceSha: process.env.GITHUB_SHA, platform,
    tarballSha256: crypto.createHash('sha256').update(fs.readFileSync(tarball)).digest('hex'),
  }, null, 2));
}

if (process.argv[2] === 'plan') plan().catch(error => { console.error(error); process.exitCode = 1; });
else if (process.argv[2] === 'stage') stage();
else throw new Error('Use plan or stage');
