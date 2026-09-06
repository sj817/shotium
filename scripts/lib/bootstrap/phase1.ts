// Phase 1: establish a buildable baseline with the official tools.
//
// The source set stays deliberately wide. Sparse checkout is NOT configured
// here: main-repo sparse and gclient DEPS are independent pruning layers,
// sparse rules do not reach into ANGLE/Dawn/DevTools, and GN needs
// directories that are easy to guess wrong. A narrow checkout here would
// produce a GN graph that cannot be trusted, and the whole point of Phase 1
// is to produce one that can.
//
// Phase 1 stops at gn gen. Ninja takes hours and is driven separately, so a
// configure failure is found in minutes.

import {existsSync, readdirSync, readFileSync} from 'node:fs';
import path from 'node:path';

import {execaSync} from 'execa';

import {addDeviation, completeCheckpoint, firstLine, gitRead, log, measureTree, networkDelta, networkSnapshot, newDirectory, runProcess, saveMeasurement, setFile, sha256File, sha256Text, startCheckpoint, volumeSnapshot, type ProcessResult, type RunContext} from './core.ts';
import {assertCheckoutMatchesLock, gclientContent, type Lock, type Settings} from './lock.ts';

// Windows 10/11 SDK versions present on this host.
export function installedWindowsSdks(): string[] {
  const roots: string[] = [];
  const reg = execaSync('reg', ['query', String.raw`HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots`, '/v', 'KitsRoot10'], {reject: false});
  const m = /KitsRoot10\s+REG_SZ\s+(.+)$/m.exec(reg.stdout);
  if (m) roots.push(m[1].trim());
  roots.push('C:\\Program Files (x86)\\Windows Kits\\10\\');
  const versions = new Set<string>();
  for (const root of new Set(roots)) {
    const include = path.join(root, 'Include');
    if (!existsSync(include)) continue;
    for (const name of readdirSync(include)) if (/^\d+\.\d+\.\d+\.\d+$/.test(name)) versions.add(name);
  }
  return [...versions].sort();
}

// Checks that the SDK version the tree asks for is actually installed. Both
// the upstream value and a lowered one are deviations from *some* baseline,
// so neither is assumed: the requested version is read from the tree, the
// installed versions from the host, and the two are compared. A mismatch
// fails now instead of halfway through the win_toolchain hook.
export function assertWindowsSdkDeviation(ctx: RunContext, srcRoot: string, allowMissingSdk: boolean) {
  const files = ['build/vs_toolchain.py', 'build/toolchain/win/setup_toolchain.py'];
  const requested: Array<{file: string; version: string}> = [];
  for (const relative of files) {
    const file = path.join(srcRoot, relative);
    if (!existsSync(file)) continue;
    for (const m of readFileSync(file, 'utf8').matchAll(/SDK_VERSION\s*=\s*['"]([0-9.]+)['"]/g)) requested.push({file: relative.replace(/\//g, '\\'), version: m[1]});
  }
  const installed = installedWindowsSdks();
  const wanted = [...new Set(requested.map((r) => r.version))].sort();
  const missing = wanted.filter((v) => !installed.includes(v));
  const detail = {requested, installed, missing};
  if (missing.length) {
    const message = `The tree pins Windows SDK ${missing.join(', ')} but this host has ${installed.join(', ')}.`;
    if (!allowMissingSdk) {
      throw new Error(`${message}\nThe win_toolchain hook and every subsequent compile would fail. Either install\nthe SDK, or carry the fork commit that lowers SDK_VERSION in\nbuild/vs_toolchain.py and build/toolchain/win/setup_toolchain.py (see the fork\noverlay), or re-run with --allow-missing-sdk and accept the recorded deviation.\n`);
    }
    addDeviation(ctx, 'win-sdk-version', message, 'required-not-applied', detail);
  } else {
    addDeviation(ctx, 'win-sdk-version', `Tree requests Windows SDK ${wanted.join(', ')}; this host provides ${installed.join(', ')}. Recorded because the SDK version is part of the toolchain the measurements describe.`, 'applied', detail);
  }
  return detail;
}

// Partial clone of Chromium at the pinned commit. No sparse checkout.
export async function clone(ctx: RunContext, lock: Lock) {
  const src = ctx.srcRoot;
  const logFile = path.join(ctx.logRoot, 'clone.log');
  if (existsSync(path.join(src, '.git'))) {
    // Resume path. Verify rather than re-clone; never delete.
    const remote = firstLine(gitRead(src, ['config', '--get', 'remote.origin.url'], true).output);
    if (remote !== lock.chromium.remote) throw new Error(`Existing checkout at ${src} has remote '${remote}', expected '${lock.chromium.remote}'. Nothing was deleted; choose a different target.`);
    log(ctx, 'INFO', `existing clone found at ${src}; verifying instead of re-cloning.`);
  } else {
    if (existsSync(src) && readdirSync(src).length > 0) throw new Error(`${src} exists and is not empty but has no .git. Refusing to clone into it.`);
    await runProcess(ctx, {file: 'git', cwd: ctx.targetRoot, logFile, purpose: 'blob:none keeps history metadata but defers file contents until checkout', args: ['clone', '--filter=blob:none', '--no-checkout', '--progress', lock.chromium.remote, src]});
  }
  if (!ctx.dryRun && gitRead(src, ['cat-file', '-e', `${lock.chromium.commit}^{commit}`], true).exitCode !== 0) {
    await runProcess(ctx, {file: 'git', cwd: src, logFile, purpose: 'the pin may not be on a fetched branch tip', args: ['fetch', '--filter=blob:none', '--progress', 'origin', lock.chromium.commit]});
  }
  await runProcess(ctx, {file: 'git', cwd: src, logFile, purpose: 'detached at the pin; no branch means no accidental fast-forward', args: ['checkout', '--detach', lock.chromium.commit]});
  if (!ctx.dryRun && firstLine(gitRead(src, ['config', '--get', 'core.sparseCheckout'], true).output) === 'true') {
    throw new Error(`core.sparseCheckout is enabled in ${src}. Phase 1 requires the wide source set.`);
  }
  return {SrcRoot: src, Commit: lock.chromium.commit, SparseCheckout: false, SparseRationale: 'sparse before a trusted GN graph produced a checkout that could not configure.'};
}

// Replays the fork's commits on top of the pinned upstream commit, as
// individual patches exported from the reference checkout, one deviation
// record per commit. Patches rather than a git fetch because the reference
// checkout is a blob-filtered clone whose pinned commit is the graft
// boundary: it can produce diffs, but it cannot reliably serve a fetch.
export async function forkOverlay(ctx: RunContext, lock: Lock) {
  if (!lock.forkOverlay.enabled) {
    addDeviation(ctx, 'fork-overlay', 'Overlay disabled: the checkout is stock upstream at the pin. //shot:shot does not exist there, so gn gen has nothing to configure.', 'skipped');
    return null;
  }
  const patchDir = path.join(ctx.stateRoot, 'deviations');
  newDirectory(ctx, patchDir);
  if (!ctx.dryRun) {
    const existing = readdirSync(patchDir).filter((n) => n.endsWith('.patch'));
    if (existing.length === 0) {
      const r = execaSync('git', ['-C', lock.chromium.reference, 'format-patch', '--binary', '--no-signature', '-o', patchDir, `${lock.chromium.commit}..${lock.forkOverlay.commit}`], {reject: false, all: true});
      if (r.exitCode !== 0) throw new Error(`Could not export the fork overlay from ${lock.chromium.reference}:\n${r.all}`);
    } else {
      log(ctx, 'INFO', `reusing ${existing.length} exported patches in ${patchDir}.`);
    }
  }
  const patches = existsSync(patchDir) ? readdirSync(patchDir).filter((n) => n.endsWith('.patch')).sort().map((n) => path.join(patchDir, n)) : [];
  if (!ctx.dryRun && patches.length !== lock.forkOverlay.commits.length) throw new Error(`Expected ${lock.forkOverlay.commits.length} overlay patches, found ${patches.length} in ${patchDir}.`);

  // Already applied? git am on top of an applied series would fail; ancestry
  // of the pin plus a non-empty diff is enough to detect the resumed case.
  let alreadyApplied = false;
  if (!ctx.dryRun) {
    const head = firstLine(gitRead(ctx.srcRoot, ['rev-parse', 'HEAD']).output);
    const distance = Number(firstLine(gitRead(ctx.srcRoot, ['rev-list', '--count', `${lock.chromium.commit}..${head}`], true).output));
    if (distance && distance >= patches.length && distance > 0) {
      alreadyApplied = true;
      log(ctx, 'INFO', `overlay already applied (${distance} commits above the pin); skipping git am.`);
    }
  }
  if (!alreadyApplied) {
    await runProcess(ctx, {
      file: 'git', cwd: ctx.srcRoot, logFile: path.join(ctx.logRoot, 'overlay.log'),
      purpose: 'replay the fork commits; identity is fixed so a machine without git identity still applies them',
      args: ['-c', 'user.name=shot-bootstrap', '-c', 'user.email=shot-bootstrap@localhost', 'am', '--keep-non-patch', '--3way', ...patches],
    });
  }
  for (const commit of lock.forkOverlay.commits) addDeviation(ctx, `fork-commit:${commit.commit.slice(0, 12)}`, commit.subject, 'applied', {commit: commit.commit, files: commit.files});
  return {PatchDirectory: patchDir, PatchCount: patches.length, Patches: patches.map((p) => ({name: path.basename(p), sha256: sha256File(p)}))};
}

export function writeGclientFile(ctx: RunContext, lock: Lock) {
  const content = gclientContent(lock.gclient.solutionName, lock.gclient.url, lock.gclient.customVars, lock.gclient.customDeps, lock.build.targetOs);
  const sha = sha256Text(content);
  if (sha !== lock.gclient.contentSha256) throw new Error(`Rendered .gclient does not match the locked hash (${sha} vs ${lock.gclient.contentSha256}).`);
  setFile(ctx, ctx.gclientFile, content);
  log(ctx, 'INFO', `.gclient written with checkout_configuration=${String(lock.gclient.customVars.checkout_configuration)} and ${lock.gclient.customDeps.length} custom_deps null entries.`);
  return {Path: ctx.gclientFile, Sha256: sha, CustomDepsCount: lock.gclient.customDeps.length};
}

export const toolEnvironment = (ctx: RunContext): Record<string, string> => ({
  DEPOT_TOOLS_UPDATE: '0', DEPOT_TOOLS_WIN_TOOLCHAIN: '0', DEPOT_TOOLS_METRICS: '0', PATH: `${ctx.depotTools};${process.env.PATH ?? ''}`,
});

// Runs one gclient sub-command with the measurements around it.
export async function gclientStep(ctx: RunContext, python: string, gclientArgs: string[], logName: string, measurementName: string, allowNonZeroExit = false) {
  const netBefore = networkSnapshot();
  const volumeBefore = volumeSnapshot(ctx.targetRoot);
  const result = await runProcess(ctx, {
    file: python, args: [path.join(ctx.depotTools, 'gclient.py'), ...gclientArgs], cwd: ctx.targetRoot, logFile: path.join(ctx.logRoot, logName),
    env: toolEnvironment(ctx), allowNonZeroExit, purpose: 'invoked through the pinned depot_tools gclient.py, not a wrapper on PATH',
  });
  const netAfter = networkSnapshot();
  const volumeAfter = volumeSnapshot(ctx.targetRoot);
  const measurement = {
    step: measurementName, command: result.command, exitCode: result.exitCode, elapsedSeconds: result.elapsedSeconds,
    network: ctx.dryRun ? null : networkDelta(netBefore, netAfter), volumeBefore, volumeAfter,
    tree: ctx.dryRun ? null : measureTree(ctx.srcRoot), gitStore: ctx.dryRun ? null : measureTree(path.join(ctx.srcRoot, '.git')),
    countObjects: ctx.dryRun ? [] : gitRead(ctx.srcRoot, ['count-objects', '-vH'], true).output,
    capturedUtc: new Date().toISOString(),
  };
  saveMeasurement(ctx, measurementName, measurement);
  return {Result: result, Measurement: measurement};
}

// Writes args.gn and runs gn gen. Reports; does not build.
export async function gnGen(ctx: RunContext, lock: Lock) {
  const outDir = path.join(ctx.srcRoot, lock.build.outDir);
  newDirectory(ctx, outDir);
  const argsPath = path.join(outDir, 'args.gn');
  setFile(ctx, argsPath, lock.build.argsGnContent);
  if (!ctx.dryRun) {
    const sha = sha256File(argsPath);
    const expected = sha256Text(lock.build.argsGnContent);
    if (sha !== expected) throw new Error(`args.gn on disk hashes ${sha} but the locked content hashes ${expected}.`);
  }
  const gn = path.join(ctx.srcRoot, 'buildtools', 'win', 'gn.exe');
  if (!ctx.dryRun && !existsSync(gn)) throw new Error(`gn.exe is missing at ${gn}. buildtools/win comes from DEPS; sync did not complete.`);
  const result: ProcessResult = await runProcess(ctx, {
    file: gn, args: ['gen', lock.build.outDir.replace(/\//g, '\\')], cwd: ctx.srcRoot, logFile: path.join(ctx.logRoot, 'gn-gen.log'),
    env: toolEnvironment(ctx), allowNonZeroExit: true, purpose: 'configure only; the ninja build is deliberately not started here',
  });
  const measurement = {
    step: 'gn-gen', command: result.command, exitCode: result.exitCode, elapsedSeconds: result.elapsedSeconds, outDir, argsGnSha256: lock.build.argsGnSha256,
    gnCheckRun: false, gnCheckNote: 'gn gen does not run header checking. gn check must be invoked explicitly and is not part of Phase 1.',
    ninjaRun: false, ninjaNote: 'Phase 1 stops at configure; //shot:shot is built separately because a full build takes hours.',
    capturedUtc: new Date().toISOString(),
  };
  saveMeasurement(ctx, 'gn-gen', measurement);
  if (result.exitCode === 0) log(ctx, 'INFO', `gn gen succeeded for ${lock.build.outDir}.`);
  else if (result.executed) log(ctx, 'ERROR', `gn gen FAILED (exit ${result.exitCode}); see ${result.logFile}.`);
  return {Result: result, Measurement: measurement};
}

export async function phase1(ctx: RunContext, lock: Lock, s: Settings) {
  log(ctx, 'STEP', 'Phase 1: buildable baseline with the official tools.');
  const checkpoint = startCheckpoint(ctx, 'phase-1', lock.inputDigest, 'partial clone, fork overlay, .gclient, gclient sync, official hooks, gn gen');
  if (checkpoint.action === 'Skip') return checkpoint.record;
  const report: Record<string, unknown> & {steps: Record<string, unknown>} = {phase: 1, inputDigest: lock.inputDigest, startedUtc: new Date().toISOString(), steps: {}};
  try {
    let step = startCheckpoint(ctx, 'phase-1.1-clone', lock.inputDigest, 'partial clone at the pinned commit');
    if (step.action === 'Run') {
      const c = await clone(ctx, lock);
      report.steps.clone = c;
      completeCheckpoint(ctx, step, 0, 'completed', c);
    }
    step = startCheckpoint(ctx, 'phase-1.2-overlay', lock.inputDigest, 'replay fork commits and record deviations');
    if (step.action === 'Run') {
      const o = await forkOverlay(ctx, lock);
      report.steps.overlay = o;
      completeCheckpoint(ctx, step, 0, 'completed', o);
    }
    if (!ctx.dryRun) {
      assertCheckoutMatchesLock(ctx, lock, ctx.srcRoot, lock.forkOverlay.enabled);
      report.steps.sdk = assertWindowsSdkDeviation(ctx, ctx.srcRoot, s.allowMissingSdk);
    }
    step = startCheckpoint(ctx, 'phase-1.3-gclient-config', lock.inputDigest, 'write .gclient with checkout_configuration=small and the first custom_deps batch');
    if (step.action === 'Run') {
      const g = writeGclientFile(ctx, lock);
      report.steps.gclientConfig = g;
      completeCheckpoint(ctx, step, 0, 'completed', g);
    }
    if (s.skipSync) {
      log(ctx, 'WARN', '--skip-sync: no dependencies were fetched.');
    } else {
      step = startCheckpoint(ctx, 'phase-1.4-sync', lock.inputDigest, 'gclient sync --no-history --nohooks');
      if (step.action === 'Run') {
        // --nohooks separates dependency fetching from hook execution so a
        // hook failure does not look like a sync failure. --nohooks does not
        // stop GCS/CIPD downloads: they are deps, not hooks.
        const sync = await gclientStep(ctx, s.python, ['sync', '--no-history', '--nohooks', '--jobs', String(s.syncJobs)], 'sync.log', 'sync');
        report.steps.sync = sync.Measurement;
        completeCheckpoint(ctx, step, sync.Result.exitCode ?? 0, 'completed', sync.Measurement);
      }
    }
    if (s.skipHooks) {
      log(ctx, 'WARN', '--skip-hooks: the official Windows hooks did not run; gn gen will almost certainly fail.');
    } else {
      step = startCheckpoint(ctx, 'phase-1.5-hooks', lock.inputDigest, `run the ${lock.hooks.selected} official Windows hooks`);
      if (step.action === 'Run') {
        log(ctx, 'INFO', `hooks expected in order: ${lock.hooks.entries.map((h) => h.name).join(', ')}`);
        // Round one runs the stock hook set unmodified.
        const hooks = await gclientStep(ctx, s.python, ['runhooks', '--jobs', String(s.syncJobs)], 'hooks.log', 'hooks');
        report.steps.hooks = hooks.Measurement;
        completeCheckpoint(ctx, step, hooks.Result.exitCode ?? 0, 'completed', hooks.Measurement);
      }
    }
    if (s.skipGnGen) {
      log(ctx, 'WARN', '--skip-gn-gen: configure was not attempted.');
    } else {
      step = startCheckpoint(ctx, 'phase-1.6-gn-gen', lock.inputDigest, 'gn gen only; no ninja');
      if (step.action === 'Run') {
        const gn = await gnGen(ctx, lock);
        report.steps.gnGen = gn.Measurement;
        const code = gn.Result.exitCode ?? 0;
        completeCheckpoint(ctx, step, code, code === 0 ? 'completed' : 'failed', gn.Measurement);
      }
    }
    report.deviations = [...ctx.deviations];
    report.finishedUtc = new Date().toISOString();
    report.finalTree = ctx.dryRun ? null : measureTree(ctx.srcRoot);
    saveMeasurement(ctx, 'phase-1-report', report);
    completeCheckpoint(ctx, checkpoint, 0, 'completed', {report: 'measurements/phase-1-report.json'});
  } catch (error) {
    completeCheckpoint(ctx, checkpoint, 1, 'failed', {error: error instanceof Error ? error.message : String(error)});
    throw error;
  }
  return report;
}
