// Phase 0 safety boundary.
//
// Two facts about the development host drive most of it: the working
// .gclient lives one level *above* the checkout (because DEPS hardcodes an
// 'src/' prefix), and D:\Github\src is a symlink to the checkout. A path
// guard that compares strings without resolving reparse points would happily
// accept 'D:\Github\src\out\isolated' as a fresh target and start writing
// into the reference checkout.
//
// Every guard returns a record instead of only throwing, so the Phase 0 JSON
// shows which guards ran and what they saw.

import {existsSync, readdirSync, readlinkSync, statSync, lstatSync} from 'node:fs';
import path from 'node:path';

import {execaSync} from 'execa';
import which from 'which';

import {volumeSnapshot} from './core.ts';

export interface GuardResult {
  id: string;
  description: string;
  result: 'pass' | 'warn' | 'fail';
  detail: string;
}

const guard = (id: string, description: string, result: GuardResult['result'], detail = ''): GuardResult => ({id, description, result, detail});

// Absolute path with every existing component's symlink/junction resolved,
// component by component from the drive root down, so an intermediate link
// is followed even when the leaf does not exist yet.
export function resolveRealPath(p: string): string {
  if (!p.trim()) throw new Error('resolveRealPath: empty path.');
  const full = path.resolve(p);
  const root = path.parse(full).root;
  const rest = full.slice(root.length);
  let current = root;
  for (const part of rest.split(/[\\/]/).filter(Boolean)) {
    current = path.join(current, part);
    if (!existsSync(current)) continue;
    // Follow a chain of links; the depth cap turns a link loop into an error
    // rather than a hang.
    for (let depth = 0; depth < 16; depth++) {
      let target: string | null = null;
      try {
        if (lstatSync(current).isSymbolicLink()) target = readlinkSync(current);
      } catch {
        target = null;
      }
      if (!target) break;
      const resolved = path.isAbsolute(target) ? target : path.resolve(path.dirname(current), target);
      if (resolved === current) break;
      current = resolved;
      if (depth === 15) throw new Error(`Too many symlink hops resolving: ${p}`);
    }
  }
  return path.resolve(current).replace(/[\\/]+$/, '');
}

// True when parent is child or an ancestor of it, after link resolution.
export function pathContains(parent: string, child: string): boolean {
  const p = resolveRealPath(parent).toLowerCase(), c = resolveRealPath(child).toLowerCase();
  return p === c || c.startsWith(p + path.sep);
}

// Verifies the tools the lock names, and pins depot_tools' own behaviour.
export function assertHostEnvironment(depotTools: string, pinnedDepotToolsCommit: string, allowDrift: boolean): GuardResult[] {
  const results: GuardResult[] = [];
  const pwsh = which.sync('pwsh', {nothrow: true});
  if (!pwsh) throw new Error('PowerShell 7+ (pwsh) is required for the volume and network measurements and was not found on PATH.');
  results.push(guard('ENV-PWSH', 'PowerShell 7+', 'pass', execaSync(pwsh, ['-NoProfile', '-Command', '$PSVersionTable.PSVersion.ToString()'], {reject: false}).stdout.trim()));
  const git = which.sync('git', {nothrow: true});
  if (!git) throw new Error('git was not found on PATH.');
  results.push(guard('ENV-GIT', 'git on PATH', 'pass', git));
  if (!existsSync(path.join(depotTools, 'gclient.py'))) throw new Error(`depot_tools does not look like a depot_tools checkout: ${depotTools}`);
  const head = execaSync('git', ['-C', depotTools, 'rev-parse', 'HEAD'], {reject: false});
  if (head.exitCode !== 0) throw new Error(`depot_tools is not a git repository: ${depotTools}`);
  const commit = head.stdout.trim();
  if (commit !== pinnedDepotToolsCommit) {
    const message = `depot_tools is at ${commit} but the lock pins ${pinnedDepotToolsCommit}.`;
    if (!allowDrift) throw new Error(`${message}\nA different depot_tools evaluates DEPS differently and invalidates every number in the lock. Fix the checkout or pass --allow-depot-tools-drift and accept the recorded deviation.`);
    results.push(guard('ENV-DEPOT-TOOLS-COMMIT', 'depot_tools pinned commit', 'warn', message));
  } else {
    results.push(guard('ENV-DEPOT-TOOLS-COMMIT', 'depot_tools pinned commit', 'pass', commit));
  }
  // DEPOT_TOOLS_UPDATE=0 keeps depot_tools from self-updating out from under
  // the pin. DEPOT_TOOLS_WIN_TOOLCHAIN=0 tells the Windows hooks to use the
  // locally installed Visual Studio instead of Google's internal toolchain
  // package, which is not fetchable outside Google.
  process.env.DEPOT_TOOLS_UPDATE = '0';
  process.env.DEPOT_TOOLS_WIN_TOOLCHAIN = '0';
  results.push(guard('ENV-DEPOT-TOOLS-FLAGS', 'DEPOT_TOOLS_UPDATE=0, DEPOT_TOOLS_WIN_TOOLCHAIN=0', 'pass', 'set for this process and its children'));
  return results;
}

// The python3 bundled with the pinned depot_tools, or the one on PATH: the
// DEPS evaluation only needs a stdlib-only python.
export function depotToolsPython(depotTools: string): string {
  const candidates = readdirSync(depotTools).filter((n) => /^bootstrap-.*_bin$/.test(n)).map((n) => path.join(depotTools, n, 'python3', 'bin', 'python3.exe')).filter((p) => existsSync(p));
  if (candidates.length) return candidates[0];
  const fallback = which.sync('python3', {nothrow: true}) ?? which.sync('python', {nothrow: true});
  if (fallback) return fallback;
  throw new Error(`No python found: neither depot_tools' bootstrap python under ${depotTools} nor python on PATH.`);
}

export interface TargetCheck {
  targetRoot: string;
  resolvedTarget: string;
  guards: GuardResult[];
}

// The Phase 0 target-directory guards. Fails on the first violation and
// never repairs anything: no deletes, no moves, no "cleaning" a partially
// populated directory. The operator picks a different path.
export function assertTargetDirectory(targetRoot: string, forbiddenRoots: string[], minimumFreeGiB: number, allowAncestorGclient: boolean, allowNonEmpty: boolean): TargetCheck {
  const results: GuardResult[] = [];
  const failures: string[] = [];
  const add = (id: string, description: string, result: GuardResult['result'], detail: string) => {
    results.push(guard(id, description, result, detail));
    if (result === 'fail') failures.push(`[${id}] ${description}\n    ${detail}`);
  };
  const refuse = () => new Error(`Phase 0 refused the target directory '${targetRoot}':\n  ${failures.join('\n  ')}`);

  if (!path.isAbsolute(targetRoot)) throw new Error(`The target directory must be an absolute path; got '${targetRoot}'.`);
  if (/[*?]/.test(targetRoot)) throw new Error(`The target directory must not contain wildcards; got '${targetRoot}'.`);
  const real = resolveRealPath(targetRoot);
  add('TGT-ABSOLUTE', 'Target is an absolute, wildcard-free path', 'pass', real);
  if (real.toLowerCase() !== path.resolve(targetRoot).replace(/[\\/]+$/, '').toLowerCase()) {
    add('TGT-SYMLINK', 'Target path traverses a symlink or junction', 'warn', `'${targetRoot}' really is '${real}'; all guards below use the resolved path.`);
  } else {
    add('TGT-SYMLINK', 'Target path traverses a symlink or junction', 'pass', 'no reparse points on the path');
  }

  const pathRoot = path.parse(real).root.replace(/[\\/]+$/, '');
  if (real.toLowerCase() === pathRoot.toLowerCase()) {
    // Stop here rather than collecting more findings: the remaining guards
    // would query a bare 'D:' path.
    add('TGT-NOT-DRIVE-ROOT', 'Target is not a drive root', 'fail', 'A drive root cannot be a bootstrap target; a failed run there is unrecoverable.');
    throw refuse();
  }
  add('TGT-NOT-DRIVE-ROOT', 'Target is not a drive root', 'pass', real);
  if (!existsSync(pathRoot + path.sep)) add('TGT-DRIVE-EXISTS', 'Target volume exists', 'fail', `No such volume: ${pathRoot}`);
  else add('TGT-DRIVE-EXISTS', 'Target volume exists', 'pass', pathRoot);

  // Spaces in the source path break Chromium's Windows toolchain scripts and
  // several GN/ninja command lines; non-ASCII breaks more.
  if (/\s/.test(real)) add('TGT-PATH-SHAPE', 'Target path has no whitespace', 'fail', `Chromium's Windows build does not survive spaces in the source path: '${real}'`);
  else if (!/^[\x20-\x7E]+$/.test(real)) add('TGT-PATH-SHAPE', 'Target path is ASCII', 'fail', `Non-ASCII characters in '${real}'`);
  else add('TGT-PATH-SHAPE', 'Target path is ASCII and whitespace-free', 'pass', real);
  if (real.length > 24) add('TGT-PATH-LENGTH', 'Target path is short', 'warn', `'${real}' is ${real.length} chars; Chromium generates deep paths and MAX_PATH failures appear late in a build.`);
  else add('TGT-PATH-LENGTH', 'Target path is short', 'pass', `${real.length} chars`);

  // Containment is checked in both directions: the target may not sit inside
  // a protected root, and it may not swallow one either.
  for (const forbidden of forbiddenRoots) {
    if (!forbidden.trim() || !existsSync(forbidden)) continue;
    const forbiddenReal = resolveRealPath(forbidden);
    if (pathContains(forbiddenReal, real)) {
      add('TGT-NOT-IN-PROTECTED-ROOT', 'Target is outside every protected root', 'fail', `'${real}' is inside protected root '${forbiddenReal}'. The reference checkout, its parent (which holds the working .gclient) and depot_tools are never valid targets.`);
    } else if (pathContains(real, forbiddenReal)) {
      add('TGT-NOT-ABOVE-PROTECTED-ROOT', 'Target does not contain a protected root', 'fail', `'${real}' contains protected root '${forbiddenReal}'.`);
    }
  }
  if (!results.some((r) => /^TGT-NOT-.*PROTECTED-ROOT$/.test(r.id))) {
    add('TGT-NOT-IN-PROTECTED-ROOT', 'Target is outside every protected root', 'pass', `checked: ${forbiddenRoots.filter(Boolean).join('; ')}`);
  }

  if (existsSync(real)) {
    if (!statSync(real).isDirectory()) {
      add('TGT-EMPTY', 'Target is a new or empty directory', 'fail', `'${real}' is a file.`);
    } else {
      const children = readdirSync(real);
      // A completed Phase 0 checkpoint is this bootstrap's ownership mark.
      const resumeMarker = path.join(real, 'bootstrap-state', 'checkpoints', 'phase-0.json');
      if (children.length > 0 && existsSync(resumeMarker)) {
        add('TGT-EMPTY', 'Target is a new, empty, or resumable directory', 'warn', `'${real}' holds a previous run of this bootstrap (${children.length} entries); resuming from its checkpoints.`);
      } else if (children.length > 0 && !allowNonEmpty) {
        add('TGT-EMPTY', 'Target is a new or empty directory', 'fail', `'${real}' already contains ${children.length} entries (first: ${children[0]}) and no bootstrap checkpoint. This script never deletes or moves anything; choose an empty directory.`);
      } else if (children.length > 0) {
        add('TGT-EMPTY', 'Target is a new or empty directory', 'warn', `--allow-non-empty accepted ${children.length} existing entries; measurements from this run are not comparable with a clean baseline (in a non-empty checkout, null deps prove nothing).`);
      } else {
        add('TGT-EMPTY', 'Target is a new or empty directory', 'pass', 'existing and empty');
      }
    }
  } else {
    const parent = path.dirname(real);
    if (!existsSync(parent)) add('TGT-EMPTY', 'Target is a new or empty directory', 'fail', `Neither '${real}' nor its parent '${parent}' exists. Create the parent deliberately; this script will not build an arbitrary path.`);
    else add('TGT-EMPTY', 'Target is a new or empty directory', 'pass', 'does not exist yet; will be created');
  }

  // Landing inside somebody else's work tree means gclient and git would act
  // on that repository's index.
  let probe: string | null = real;
  while (probe && !existsSync(probe)) {
    const up = path.dirname(probe);
    probe = up === probe ? null : up;
  }
  if (probe) {
    const top = execaSync('git', ['-C', probe, 'rev-parse', '--show-toplevel'], {reject: false});
    if (top.exitCode === 0 && top.stdout.trim()) {
      add('TGT-NOT-IN-GIT-WORKTREE', 'Target is not inside an existing git work tree', 'fail', `'${real}' resolves inside the git work tree at '${resolveRealPath(top.stdout.trim())}'.`);
    } else {
      add('TGT-NOT-IN-GIT-WORKTREE', 'Target is not inside an existing git work tree', 'pass', `probed from ${probe}`);
    }
  }

  // gclient walks upwards for a .gclient, so any target under a directory
  // holding one would be adopted by that solution instead of its own.
  let ancestor: string | null = path.dirname(real);
  let found: string | null = null;
  while (ancestor) {
    if (existsSync(path.join(ancestor, '.gclient'))) {
      found = path.join(ancestor, '.gclient');
      break;
    }
    const next = path.dirname(ancestor);
    if (next === ancestor) break;
    ancestor = next;
  }
  if (found && !allowAncestorGclient) add('TGT-NO-ANCESTOR-GCLIENT', 'No .gclient above the target', 'fail', `Found '${found}' above the target. gclient searches upwards and would sync that solution instead of this one.`);
  else if (found) add('TGT-NO-ANCESTOR-GCLIENT', 'No .gclient above the target', 'warn', `--allow-ancestor-gclient accepted '${found}'.`);
  else add('TGT-NO-ANCESTOR-GCLIENT', 'No .gclient above the target', 'pass', 'none found');

  const volume = volumeSnapshot(real);
  if (volume.SizeRemaining !== null) {
    const freeGiB = Math.round((volume.SizeRemaining / 2 ** 30) * 100) / 100;
    if (freeGiB < minimumFreeGiB) add('TGT-FREE-SPACE', `At least ${minimumFreeGiB} GiB free`, 'fail', `${freeGiB} GiB free on ${real[0]}:. A full sync plus hooks needs far more; running out mid-sync leaves an unusable tree.`);
    else add('TGT-FREE-SPACE', `At least ${minimumFreeGiB} GiB free`, 'pass', `${freeGiB} GiB free`);
  } else {
    add('TGT-FREE-SPACE', `At least ${minimumFreeGiB} GiB free`, 'warn', 'Get-Volume could not report the volume.');
  }

  if (failures.length) throw refuse();
  return {targetRoot, resolvedTarget: real, guards: results};
}
