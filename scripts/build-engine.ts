// Build //shot:shot or //shot:shot_c into out/Shot, absorbing the two failure
// modes that are not build errors.
//
//   1. `gn gen` evaluates several Windows toolchain variants in parallel and
//      they all write the same environment.x86 / environment.x64, so one of
//      them intermittently loses and reports
//          PermissionError: [Errno 13] Permission denied: 'environment.x64'
//      This is a race, not a configuration error; the retry always wins.
//
//   2. ninja re-runs `gn gen` itself when a BUILD.gn changed since the last
//      generate, which puts that same race inside the build. Running gn gen
//      here first makes build.ninja current so ninja usually skips its own
//      regen, and the ninja invocation is retried once for the case where it
//      does not.
//
// Usage (from anywhere in the repository):
//
//   pnpm build:engine                         # shot: shotium.exe
//   pnpm build:engine --target shot_c         # shotium.dll, which the addon links
//   pnpm build:engine --jobs 16 --log out/Shot/build.log
//   pnpm build:engine --gen-only             # does the graph still parse?
//
// A relative --log is resolved against the repository root, not the directory
// you typed the command in. INIT_CWD cannot express the latter here: the root
// script is `pnpm -C scripts build-engine`, so a second pnpm runs and
// overwrites INIT_CWD with the outer package directory, which is the root
// again.
//
// The parallelism default is deliberately conservative. Measure the phase you
// are about to run before raising it; Blink core's jumbo TUs have run the host
// out of memory at -j 24. See CLAUDE.md, "Building".

import {createWriteStream, existsSync} from 'node:fs';
import {mkdir, readFile, rename, rm} from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {pipeline} from 'node:stream/promises';

import {cac} from 'cac';
import {execa} from 'execa';
import pRetry, {AbortError} from 'p-retry';
import pc from 'picocolors';

const root = path.resolve(import.meta.dirname, '..');
const outDir = 'out/Shot';

const platformDir: Record<string, string> = {win32: 'win', linux: 'linux64', darwin: 'mac'};
const gn = path.join(
    'buildtools', platformDir[process.platform] ?? 'linux64',
    process.platform === 'win32' ? 'gn.exe' : 'gn');
const python = process.platform === 'win32' ? 'python' : 'python3';

const cli = cac('build-engine')
    .option('--target <name>', 'GN target: shot (shotium.exe) or shot_c (shotium.dll)', {default: 'shot'})
    .option('--jobs <n>', 'ninja -j', {default: 12})
    .option('--log <path>', 'where ninja output goes (default: <tmp>/shot-build-<target>.log)')
    .option('--gen-only', 'stop after gn gen: does the graph still parse? (~25 s)');
cli.help();
const {options} = cli.parse();
if (options.help) process.exit(0);

const target: string = options.target;
// cac hands back `true` for a valueless `--jobs`, and `Number(true)` is 1, so
// a typo would quietly start a single-threaded full build that looks like a
// hang for hours. Anything that is not a positive integer is rejected in
// main() rather than coerced.
const jobs = typeof options.jobs === 'boolean' ? Number.NaN : Number(options.jobs);
// pnpm runs package scripts with cwd = the package directory, and this one runs
// one level down (`pnpm -C scripts build-engine`), so resolve against the
// repository root: it is the only anchor both levels agree on. See the header.
const log = options.log ?
    path.resolve(root, options.log) :
    path.join(os.tmpdir(), `shot-build-${target}.log`);

function say(message: string): void {
  console.log(message);
}

function lastLines(text: string | undefined, count: number): string {
  return (text ?? '').split(/\r?\n/).filter(Boolean).slice(-count).join('\n');
}

// Skia is a DEPS checkout, so gclient restores its upstream source rather than
// the patches tracked by this repository. Apply them once, and fail loudly if a
// Skia roll makes one stop matching instead of silently building without it --
// a missing parallel blur is merely slow, but a missing row limit means the
// streaming PNG decode falls back to a full-size bitmap on every image.
async function applySkiaPatches(): Promise<boolean> {
  const patches = [
    'third_party_skia_parallel_blur.patch',
    'third_party_skia_incremental_row_limit.patch',
  ];
  for (const name of patches) {
    const apply = (...args: string[]) => execa(
        'git', ['-C', 'third_party/skia', 'apply', ...args, `../../patches/${name}`],
        {cwd: root, reject: false});

    if ((await apply('--check', '--reverse')).exitCode === 0) {
      say(`skia: ${name} already applied`);
      continue;
    }
    if ((await apply('--check')).exitCode !== 0) {
      say(pc.red(`skia: patches/${name} no longer applies`));
      return false;
    }
    const applied = await execa(
        'git', ['-C', 'third_party/skia', 'apply', '--verbose', `../../patches/${name}`],
        {cwd: root, reject: false, stdio: 'inherit'});
    if (applied.exitCode !== 0) return false;
  }
  return true;
}

// args.gn sets icu_data_dir_override = "shot", which is a data set this
// repository generates rather than one third_party/icu ships. third_party/icu is
// a DEPS checkout, so gclient discards the directory; regenerate it here so a
// fresh sync does not fail gn gen with a missing input. The script is
// deterministic, so an unchanged output leaves ninja nothing to redo.
async function repackIcu(): Promise<boolean> {
  const src = path.join(root, 'third_party/icu/cast/icudtl.dat');
  const dst = path.join(root, 'third_party/icu/shot/icudtl.dat');
  const tmp = `${dst}.tmp`;

  if (!existsSync(src)) {
    say(pc.red(`icu: no ${src} -- is third_party/icu synced?`));
    return false;
  }
  await mkdir(path.dirname(dst), {recursive: true});
  const repack = await execa(
      python, ['tools/shot/icu_repack.py', src, tmp, '--preset', 'shot'],
      {cwd: root, reject: false, stdio: 'inherit'});
  if (repack.exitCode !== 0) {
    say(pc.red('icu: icu_repack.py failed'));
    return false;
  }
  // Only replace the file when the bytes changed, so ninja does not rebuild
  // the 4 MB data assembly on every invocation.
  if (existsSync(dst) && (await readFile(tmp)).equals(await readFile(dst))) {
    await rm(tmp);
  } else {
    await rename(tmp, dst);
  }
  return true;
}

// A real gn failure, as opposed to the toolchain race. p-retry unwraps an
// AbortError and rethrows its `originalError`, so the caller never sees the
// AbortError itself -- and handing the constructor a string makes that
// originalError a plain Error, which is indistinguishable from the race. Carry
// the distinction in a type of our own so the tail of gn's output survives.
class GnGenError extends Error {
  override name = 'GnGenError';
}

// Up to eight attempts, but only for the environment.x64 race; any other
// failure aborts on the first attempt with the tail of gn's output.
async function gnGen(): Promise<boolean> {
  try {
    await pRetry(async (attempt) => {
      const result = await execa(gn, ['gen', outDir], {cwd: root, reject: false, all: true});
      if (result.exitCode === 0) {
        say(`gn gen OK (attempt ${attempt}): ${lastLines(result.all, 2).replace(/\r?\n/g, ' ')}`);
        return;
      }
      if (!/PermissionError/.test(result.all ?? '')) {
        throw new AbortError(new GnGenError(
            `gn gen FAILED (attempt ${attempt})\n${lastLines(result.all, 25)}`));
      }
      throw new Error('environment.x64 race');
    }, {retries: 7, factor: 1, minTimeout: 0});
    return true;
  } catch (error) {
    say(pc.red(
        error instanceof GnGenError ?
            error.message :
            'gn gen: gave up after 8 attempts on the environment.x64 race'));
    return false;
  }
}

// ninja's combined output goes to the log file, not the terminal: a full build
// is tens of thousands of lines, and build_errors.py reads the log afterwards.
async function ninja(): Promise<number> {
  const attempts = 2;
  for (let attempt = 1; attempt <= attempts; attempt++) {
    const subprocess = execa(
        'ninja', ['-C', outDir, target, '-j', String(jobs), '-k', '0'],
        {cwd: root, reject: false, all: true, buffer: false});
    const [, result] = await Promise.all([
      pipeline(subprocess.all!, createWriteStream(log)),
      subprocess,
    ]);
    if (result.exitCode === 0) return 0;

    // Only the race is worth another attempt, and only while one is left:
    // regenerating after the last attempt costs a full gn gen whose result
    // nothing reads, and would throw away ninja's own exit code.
    const head = (await readFile(log, 'utf8')).split(/\r?\n/).slice(0, 30).join('\n');
    const raced = /PermissionError.*environment\.x/.test(head);
    if (!raced || attempt === attempts) return result.exitCode ?? 1;
    say('ninja hit the toolchain race while regenerating; retrying');
    if (!(await gnGen())) return 1;
  }
  return 1;
}

async function main(): Promise<number> {
  if (!['shot', 'shot_c'].includes(target)) {
    say(pc.red(`unknown target ${target}; expected shot or shot_c`));
    return 2;
  }
  if (!Number.isInteger(jobs) || jobs < 1) {
    say(pc.red(`--jobs takes a positive integer; got ${JSON.stringify(options.jobs)}`));
    return 2;
  }
  if (!(await applySkiaPatches())) return 1;
  if (!(await repackIcu())) return 1;
  if (!(await gnGen())) return 1;
  // A graph check after a cut wants the retry too -- running gn by hand is
  // where the environment.x64 race gets mistaken for a broken BUILD.gn.
  if (options.genOnly) return 0;

  say(`ninja -C ${outDir} ${target} -j ${jobs}  (log: ${log})`);
  const code = await ninja();

  say(`log: ${log}`);
  say(`ninja exit: ${code === 0 ? pc.green('0') : pc.red(String(code))}`);
  await execa(python, ['tools/shot/build_errors.py', log, '--limit', '40'],
              {cwd: root, reject: false, stdio: 'inherit'});
  return code;
}

process.exitCode = await main();
