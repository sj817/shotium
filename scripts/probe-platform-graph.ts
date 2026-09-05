// List every GN label a platform wants that this tree cannot provide.
//
// This tree is a pruned Chromium, and bringing another platform up means
// finding what the pruning discarded -- and `gn gen` reports exactly one
// missing label before it stops, so discovering them one at a time costs a
// round trip each. On CI that is fifteen minutes per label.
//
// Two things make it cheaper. The first is that a Linux `gn gen` runs fine
// on a Windows host, because gn evaluates build files and never invokes a
// compiler:
//
//     pnpm probe:platform --os linux
//
// writes out/ProbeLinux/args.gn (target_os, use_sysroot = false, a Linux
// host_toolchain) and runs gn against it. The errors come back word for word
// the same as the runner's, in about half a minute.
//
// The second is this script's own trick: when gn names a directory with no
// BUILD.gn, it writes a stub there and runs gn again, so one pass reports
// the whole set rather than its first element. The stubs are scaffolding and
// never a fix -- every file written is recorded and removed before the
// script exits, and it refuses to overwrite a path that already exists.
//
// Once gn resolves, `ninja -t inputs` names every file the target reads, and
// each one that is not on disk is classified: toolchain (this host carries
// the Windows one), DEPS (fetched by gclient on the runner, checked against
// both DEPS and .gitmodules, because the two can disagree), or REPO -- a
// file this repository is supposed to carry and does not. Only the third is
// a finding, and it is one gn cannot make.
//
// macOS cannot be configured from here (BUILDCONFIG.gn asserts), and
// anything that only shows up when a compiler runs is out of reach.

import {existsSync, mkdirSync, readdirSync, readFileSync, rmSync, rmdirSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execaSync} from 'execa';

import {resolve, root} from './lib/repo.ts';

const GN = existsSync(resolve('buildtools/win/gn.exe')) ? resolve('buildtools/win/gn.exe') : resolve('buildtools/linux64/gn');
const NINJA = resolve('third_party/ninja', process.platform === 'win32' ? 'ninja.exe' : 'ninja');
const UNABLE = /Unable to load "([^"]+)"/;
const NEED_TARGET = /no target named "([^"]+)" in "([^"]+)"/;
// The tail of a run that got all the way through the graph: host_os is win,
// so rust_cxx.gni suffixes the host tools it hands to actions, and gn cannot
// match them against the linux-toolchain outputs that actually produce them.
const HOST_ARTIFACT = /(cxxbridge|_build_script)\.exe/;

const ARGS_FOR: Record<string, string[]> = {
  linux: [
    'target_os = "linux"',
    // No sysroot in this checkout; build/config/sysroot.gni asserts on it.
    'use_sysroot = false',
    // Without this, anything reaching a host tool loads
    // build/toolchain/win/toolchain.gni, which opens with assert(is_win).
    'host_toolchain = "//build/toolchain/linux:clang_x64"',
  ],
};
const BASE_ARGS: Record<string, string> = {linux: 'import("//build/args/shot-linux.gn")'};

function runGn(outDir: string): [number, string] {
  const r = execaSync(GN, ['gen', outDir], {cwd: root, reject: false, all: true});
  return [r.exitCode ?? 1, r.all ?? ''];
}

// The gclient entries, with the condition each is checked out under. Both
// files have to be read: gclient fetches what DEPS lists; .gitmodules carries
// the same paths for git's benefit, and a pruning pass that dropped a DEPS
// entry and left the submodule behind produces a path that looks checked out
// for this platform and is never fetched.
function depsPaths(): [Map<string, string>, Set<string>] {
  const out = execaSync('git', ['config', '-f', '.gitmodules', '--get-regexp', String.raw`^submodule\..*\.(path|gclient-condition)$`], {cwd: root, reject: false}).stdout;
  const paths = new Map<string, string>(), conditions = new Map<string, string>();
  for (const line of out.split('\n')) {
    if (!line.trim()) continue;
    const space = line.indexOf(' ');
    const key = line.slice(0, space), value = line.slice(space + 1).trim();
    const name = key.slice('submodule.'.length, key.lastIndexOf('.'));
    if (key.endsWith('.path')) paths.set(name, value);
    else conditions.set(name, value);
  }
  const inDeps = new Set([...readFileSync(resolve('DEPS'), 'utf8').matchAll(/^\s*'src\/([^']+)'\s*:/gm)].map((m) => m[1]));
  const result = new Map<string, string>();
  for (const [name, p] of paths) {
    let condition = conditions.get(name) || '(unconditional)';
    if (!inDeps.has(p)) condition += '  -- NOT IN DEPS, so gclient will not fetch it';
    result.set(p, condition);
  }
  return [result, inDeps];
}

// Every file the target reads, and which of them are not here.
function checkInputs(outDir: string, target = 'shot'): string[] {
  if (!existsSync(NINJA)) {
    console.log(`\nno ninja at ${NINJA}; skipping the input check`);
    return [];
  }
  const r = execaSync(NINJA, ['-C', outDir, '-t', 'inputs', target], {cwd: root, reject: false, maxBuffer: 1 << 28});
  if (r.exitCode !== 0) {
    console.log(`\nninja could not list inputs:\n${r.stderr.slice(0, 1000)}`);
    return [];
  }
  const [deps, inDeps] = depsPaths();
  const toolchain: string[] = [], repo: string[] = [], unfetchable: string[] = [];
  const fromDeps = new Map<string, string>();
  for (const raw of r.stdout.split('\n')) {
    const p = raw.trim().replace(/\\/g, '/');
    if (!p || !p.startsWith('../../')) continue;  // generated in the build directory; ninja makes it
    const rel = p.slice(6);
    if (existsSync(path.join(root, rel))) continue;
    if (rel.includes('llvm-build') || rel.includes('rust-toolchain') || rel.endsWith('.exe')) {
      toolchain.push(rel);
      continue;
    }
    let matched = false;
    for (const [d, condition] of deps) {
      if (rel === d || rel.startsWith(d + '/')) {
        if (!fromDeps.has(d)) fromDeps.set(d, condition);
        if (!inDeps.has(d) && !unfetchable.includes(d)) unfetchable.push(d);
        matched = true;
        break;
      }
    }
    if (!matched) repo.push(rel);
  }
  console.log('\n=== inputs ===');
  console.log(`${toolchain.length} absent from the toolchain package this host carries`);
  console.log(`${fromDeps.size} gclient entr(ies) this build reads from:`);
  for (const d of [...fromDeps.keys()].sort()) console.log(`    ${d.padEnd(42)} ${fromDeps.get(d)}`);
  console.log(`${repo.length} file(s) this repository should carry and does not:`);
  for (const rel of repo) console.log(`    ${rel}`);
  if (repo.length) console.log('\nRestore them:\n  pnpm restore:upstream <paths>\nand prefer restoring a vendored crate or library whole over restoring the\nfiles ninja happened to name first.');
  if (unfetchable.length) {
    console.log(`\n${unfetchable.length} gclient entr(ies) the build needs that DEPS does not list.`);
    console.log('The submodule is still in .gitmodules, so the path looks accounted for,\nbut gclient reads DEPS and will not fetch it. Restore the entry:');
    for (const d of unfetchable) console.log(`    git cat-file blob <baseline>:DEPS | grep -A3 "${d}"`);
  }
  return [...repo, ...unfetchable];
}

function main(os_: string, outArg: string | undefined, maxSteps: number): number {
  if (os_ === 'mac') {
    console.log('macOS cannot be configured from here: build/config/BUILDCONFIG.gn\nasserts host_os is "mac" or "linux" ("Mac cross-compiles are\nunsupported"). Use the engine-macos workflow in probe mode.');
    return 2;
  }
  if (!ARGS_FOR[os_]) throw new Error(`--os must be one of ${[...Object.keys(ARGS_FOR), 'mac'].join(', ')}`);
  const outDir = outArg ?? `out/Probe${os_[0].toUpperCase()}${os_.slice(1)}`;
  mkdirSync(resolve(outDir), {recursive: true});
  writeFileSync(path.join(resolve(outDir), 'args.gn'), `${BASE_ARGS[os_]}\n${ARGS_FOR[os_].join('\n')}\n`);

  const created: string[] = [];            // stub files written, to be removed at the end
  const stubTargets = new Map<string, Set<string>>();  // build file -> target names it has to define
  const missingFiles: string[] = [];
  const missingTargets: Array<[string, string]> = [];
  const writeStub = (file: string) => {
    let body = '# TEMPORARY STUB from scripts/probe-platform-graph.ts.\n';
    for (const name of [...(stubTargets.get(file) ?? [])].sort()) body += `group("${name}") {\n}\n`;
    writeFileSync(file, body);
  };

  let verdict = 'gave up';
  for (let step = 0; step < maxSteps; step++) {
    const [code, text] = runGn(outDir);
    if (code === 0) {
      verdict = 'gn gen succeeded';
      break;
    }
    let m = UNABLE.exec(text);
    if (m) {
      const file = m[1].replace(/\\/g, '/');
      const rel = path.relative(root, file).replace(/\\/g, '/');
      if (existsSync(file)) {
        console.log(`gn cannot load a file that exists; stopping:\n${text.slice(0, 1500)}`);
        verdict = 'unreadable build file';
        break;
      }
      mkdirSync(path.dirname(file), {recursive: true});
      stubTargets.set(file, (stubTargets.get(file) ?? new Set()).add(path.basename(path.dirname(file))));
      writeStub(file);
      created.push(file);
      missingFiles.push(rel);
      console.log(`[${String(step).padStart(3)}] no BUILD.gn    ${rel}`);
      continue;
    }
    m = NEED_TARGET.exec(text);
    if (m) {
      const target = m[1], inFile = m[2].replace(/\\/g, '/');
      const file = path.join(root, inFile.replace(/^\/+/, '')).replace(/\\/g, '/');
      if (!stubTargets.has(file)) {
        // A real BUILD.gn is missing a target. That is a genuine finding, not
        // something to stub over.
        console.log(`a build file in this tree does not define a target that is asked for:\n${text.slice(0, 1500)}`);
        verdict = 'missing target in a real build file';
        break;
      }
      stubTargets.get(file)!.add(target);
      writeStub(file);
      missingTargets.push([inFile, target]);
      console.log(`[${String(step).padStart(3)}] no target     ${inFile}:${target}`);
      continue;
    }
    if (HOST_ARTIFACT.test(text)) {
      verdict = 'graph resolved; only host-suffix artifacts left (rust_cxx.gni appends .exe when host_os == "win")';
      break;
    }
    console.log(`gn failed in a way this script does not recognise:\n${text.slice(0, 2500)}`);
    verdict = 'unrecognised gn failure';
    break;
  }

  console.log();
  console.log(`=== ${verdict} ===`);
  console.log(`directories with no BUILD.gn (${missingFiles.length})`);
  for (const rel of missingFiles) console.log(`  ${rel}`);
  if (missingTargets.length) {
    console.log(`target names a stub had to define (${missingTargets.length})`);
    for (const [f, t] of missingTargets) console.log(`  ${f}:${t}`);
  }
  console.log(`\nremoving ${created.length} stub file(s)`);
  for (const file of created) {
    try {
      rmSync(file);
      let d = path.dirname(file);
      while (d.startsWith(root.replace(/\\/g, '/')) && d !== root.replace(/\\/g, '/') && readdirSync(d).length === 0) {
        rmdirSync(d);
        d = path.dirname(d);
      }
    } catch (e) {
      console.log(`  COULD NOT REMOVE ${file}: ${e instanceof Error ? e.message : String(e)}`);
    }
  }
  if (missingFiles.length || missingTargets.length) {
    console.log('\nFor each one: restore it with\n  pnpm restore:upstream <paths>\nor cut whatever names it. A directory only the test targets of a\nloaded BUILD.gn reach is usually the second.');
  }
  const resolved = verdict.startsWith('gn gen succeeded') || verdict.startsWith('graph resolved');
  if (!resolved) return 1;  // build.ninja is stale or absent
  if (created.length) {
    console.log('\nstubs were in play, so the input list below is not trustworthy; fix the\ndirectories above and run again');
    return 1;
  }
  return checkInputs(outDir).length ? 1 : 0;
}

const cli = cac('probe-platform-graph');
cli.command('', 'configure another platform here and list what its graph cannot find')
    .option('--os <name>', 'platform to configure for', {default: 'linux'})
    .option('--out <dir>', 'build directory to use (default out/Probe<Os>)')
    .option('--max-steps <n>', 'gn rounds before giving up', {default: 200})
    .action((options: {os: string; out?: string; maxSteps: number}) => {
      try {
        process.exitCode = main(options.os, options.out, Number(options.maxSteps));
      } catch (error) {
        console.error(error instanceof Error ? error.message : String(error));
        process.exitCode = 2;
      }
    });
cli.help();
cli.parse();
