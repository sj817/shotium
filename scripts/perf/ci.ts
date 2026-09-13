// Find the six engine artifacts for the tree under test and stage one of
// them as an npm-shaped candidate installation. Never substitute a release
// binary for a missing build.
//
//   pnpm perf:ci plan                       # in perf-gate.yml's resolve job
//   pnpm perf:ci stage <download> <dest>    # in each platform job
//
// `plan` reads FINGERPRINT (the engine fingerprint of the tree, see
// scripts/ci/fingerprint.ts) and BASELINE_VERSION from the environment,
// resolves each platform's engine-<platform>-<fingerprint> artifact with
// the same trust rules publish.yml applies (scripts/ci/engine-artifacts.ts),
// and writes performance-plan.json plus the job matrix to GITHUB_OUTPUT.
// `stage` unpacks the one downloaded platform tarball into a directory
// shaped like an installed @pixel.js/shotium.

import {createHash} from 'node:crypto';
import {appendFileSync, copyFileSync, cpSync, existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';

import {environment, findEngineSet} from '../ci/engine-artifacts.ts';
import {performancePlatforms} from '../lib/platforms.ts';
import {resolve} from '../lib/repo.ts';

async function plan(): Promise<void> {
  const fingerprint = process.env.FINGERPRINT ?? '';
  if (!/^[0-9a-f]{16}$/.test(fingerprint)) throw new Error('FINGERPRINT must be the 16-hex engine fingerprint');
  if (!/^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$/.test(process.env.BASELINE_VERSION ?? '')) throw new Error('Baseline must be an exact npm version');
  const {api, currentRunId} = environment();
  const matrix = [];
  for (const target of performancePlatforms) {
    const set = await findEngineSet(api, fingerprint, target.label, currentRunId);
    if (!set) throw new Error(`${target.label}: no engine with evidence at ${fingerprint}; run engine.yml first`);
    matrix.push({
      platform: target.npm, label: target.label, runner: target.nativeRunner, packageOs: target.packageOs, cpu: target.cpu,
      libc: target.libc ?? '',
      runId: set.runId, artifactName: set.engine.name, artifactId: set.engine.id, sourceSha: set.engine.workflow_run!.head_sha,
      fingerprint,
    });
  }
  writeFileSync(resolve('performance-plan.json'), JSON.stringify(matrix, null, 2));
  appendFileSync(process.env.GITHUB_OUTPUT!, `matrix=${JSON.stringify(matrix)}\n`);
}

async function stage(downloadArg: string, destinationArg: string): Promise<void> {
  const download = resolve(downloadArg), destination = resolve(destinationArg);
  const tarballs = readdirSync(download).filter((f) => f.endsWith('.tgz'));
  if (tarballs.length !== 1) throw new Error('Expected exactly one platform tarball');
  if (existsSync(destination)) throw new Error('Candidate destination must be new');
  mkdirSync(destination, {recursive: true});
  copyFileSync(resolve('apps/typescript/package.json'), path.join(destination, 'package.json'));
  cpSync(resolve('apps/typescript/dist'), path.join(destination, 'dist'), {recursive: true});
  const platform = `${process.platform}-${process.arch}`;
  const platformDirectory = path.join(destination, 'node_modules/@pixel.js', `shotium-${platform}`);
  mkdirSync(platformDirectory, {recursive: true});
  const tarball = path.join(download, tarballs[0]);
  // Keep native paths out of tar's arguments: GNU tar treats drive letters
  // as remote hosts, and MSYS tar does not accept backslash paths for -C.
  await execa('tar', ['-xzf', '-', '--strip-components=1'], {cwd: platformDirectory, inputFile: tarball, windowsHide: true});
  const manifest = JSON.parse(readFileSync(path.join(platformDirectory, 'package.json'), 'utf8')) as {name: string};
  if (manifest.name !== `@pixel.js/shotium-${platform}`) throw new Error('Wrong platform artifact');
  writeFileSync(path.join(destination, 'provenance.json'), JSON.stringify({
    sourceSha: process.env.GITHUB_SHA, fingerprint: process.env.FINGERPRINT ?? null, platform,
    tarballSha256: createHash('sha256').update(readFileSync(tarball)).digest('hex'),
  }, null, 2));
}

const cli = cac('pnpm perf:ci');
cli.command('plan', 'find the six engine artifacts at FINGERPRINT and write the job matrix')
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
