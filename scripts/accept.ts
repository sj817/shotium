// The acceptance run: build, render the corpus, diff against the oracle, and
// report the binary's size. One script so the evidence is reproducible rather
// than a sequence of commands someone has to remember.
//
// The oracle in shot/testdata/out/oracle.png was captured from an unstripped
// Chrome 151.0.7922.138 with
//     --headless --disable-gpu --hide-scrollbars --force-device-scale-factor=1
// at 1248x1320, so shot is asked for exactly that geometry. A size mismatch
// makes every later number meaningless, so it is checked before diffing.
//
// Differences are expected and allowed. What is not allowed is an unexplained
// one -- see docs/cut-progress.md section 8.6 for the render-affecting changes
// that are already known and accounted for.
//
//   pnpm accept                # build first, then render and diff
//   pnpm accept --skip-build   # the binary in out/Shot as it is

import {existsSync, rmSync, statSync} from 'node:fs';

import {cac} from 'cac';
import {execa} from 'execa';

import {diffReport, loadRegions} from './diff-report.ts';
import {pixelDiff} from './pixel-diff.ts';
import {exeName, resolve, root} from './lib/repo.ts';

async function main(opts: {skipBuild: boolean; width: number; height: number}): Promise<number> {
  const corpus = resolve('shot/testdata/render_corpus.html');
  const oracle = resolve('shot/testdata/out/oracle.png');
  const actual = resolve('shot/testdata/out/shot.png');
  const diff = resolve('shot/testdata/out/diff.png');
  const exe = resolve('out/Shot', exeName);

  if (!opts.skipBuild) {
    console.log('== build ==');
    const build = await execa('pnpm', ['build:engine'], {cwd: root, stdio: 'inherit', reject: false});
    if (build.exitCode !== 0) {
      console.log('BUILD FAILED -- stopping; there is nothing to accept.');
      return 1;
    }
  }
  if (!existsSync(exe)) {
    console.log(`no ${exe} -- the link did not produce a binary`);
    return 1;
  }

  console.log('\n== size against the 336 MB baseline ==');
  const bytes = statSync(exe).size;
  const baseline = 336 * 1024 * 1024;
  console.log(`${exe}  ${bytes.toLocaleString('en-US')} bytes  ${(bytes / 1048576).toFixed(1)} MB  (${(100 * bytes / baseline).toFixed(1)}% of the 336 MB baseline)`);

  console.log('\n== render ==');
  rmSync(actual, {force: true});
  const args = ['--file', corpus, '--width', String(opts.width), '--height', String(opts.height), '--output', actual];
  console.log(`  ${exe} ${args.join(' ')}`);
  const render = await execa(exe, args, {cwd: root, reject: false, all: true});
  const tail = (render.all ?? '').split(/\r?\n/).filter(Boolean).slice(-20);
  if (tail.length) console.log(tail.join('\n'));
  console.log(`shot exit: ${render.exitCode}`);
  if (!existsSync(actual)) {
    console.log('NO PNG PRODUCED -- criterion 1 is met but nothing else is.');
    return 1;
  }
  console.log(`produced ${actual}  ${statSync(actual).size.toLocaleString('en-US')} bytes`);

  console.log('\n== pixel diff against the oracle ==');
  let diffExit = 0;
  try {
    pixelDiff(oracle, actual, diff, 8);
  } catch (error) {
    console.log(error instanceof Error ? error.message : String(error));
    diffExit = 1;
  }

  console.log('\n== difference by region ==');
  console.log("  A whole-image percentage cannot distinguish 'antialiasing is a");
  console.log("  shade different everywhere' from 'one element is missing', so");
  console.log('  the corpus is reported feature by feature. See section 15 of');
  console.log('  docs/cut-progress.md for what each surviving number is.');
  diffReport(oracle, actual, loadRegions(resolve('shot/testdata/regions.txt'), []), 8);
  return diffExit;
}

const cli = cac('accept');
cli.command('', 'build, render the corpus, diff it against the oracle, report the size')
    .option('--skip-build', 'use the binary in out/Shot as it is')
    .option('--width <px>', 'corpus viewport width', {default: 1248})
    .option('--height <px>', 'corpus viewport height', {default: 1320})
    .action(async (options: {skipBuild?: boolean; width: number; height: number}) => {
      process.exitCode = await main({skipBuild: options.skipBuild === true, width: Number(options.width), height: Number(options.height)});
    });
cli.help();
cli.parse();
