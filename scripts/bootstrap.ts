// Auditable bootstrap for an isolated //shot:shot checkout. Phases 0 and 1.
//
//   Phase 0  lock the inputs and refuse anything that is not a clean,
//            isolated target directory.
//   Phase 1  partial clone at the pin, wide source set, checkout_configuration
//            = "small" plus the first custom_deps batch, gclient sync, the
//            official Windows hooks, then gn gen. It stops at configure: a
//            ninja build takes hours and belongs to a separate step.
//
// The script writes only inside --target-root. It reads the reference
// checkout (for the pinned DEPS, the fork commits and the GN args template)
// with read-only git plumbing and never writes to it.
//
//   pnpm bootstrap --target-root E:/shot --dry-run
//       Runs every guard and builds the full input lock without fetching.
//   pnpm bootstrap --target-root E:/shot --phase 0
//       Locks the inputs only. Phase 1 can be run later; it resumes from the
//       checkpoints under <target-root>/bootstrap-state.
//
// The lock reads DEPS at the pinned upstream commit from the reference
// checkout, and the fork overlay expects that commit to be an ancestor of
// --fork-ref. A checkout whose history was squashed carries no such ancestor;
// pass --skip-fork-overlay there, and the lock describes stock upstream at
// the pin.

import {existsSync, readFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';

import {completeCheckpoint, initializeStateDirectory, log, newDirectory, newRunContext, setFile, startCheckpoint, type RunContext} from './lib/bootstrap/core.ts';
import {assertHostEnvironment, assertTargetDirectory, depotToolsPython, type GuardResult} from './lib/bootstrap/guard.ts';
import {LockSchema, newInputLock, type Lock, type Settings} from './lib/bootstrap/lock.ts';
import {phase1} from './lib/bootstrap/phase1.ts';
import {root} from './lib/repo.ts';

function phase0(ctx: RunContext, s: Settings, protectedRoots: string[]): Lock {
  log(ctx, 'STEP', 'Phase 0: input lock and safety boundary.');
  const environmentGuards = assertHostEnvironment(s.depotTools, s.depotToolsCommit, s.allowDepotToolsDrift);
  const target = assertTargetDirectory(ctx.targetRoot, protectedRoots, s.minimumFreeGiB, s.allowAncestorGclient, s.allowNonEmpty);
  const guards: GuardResult[] = [...environmentGuards, ...target.guards];
  for (const g of guards) log(ctx, g.result === 'fail' ? 'ERROR' : g.result === 'warn' ? 'WARN' : 'TRACE', `guard ${g.result.toUpperCase()} ${g.id}: ${g.detail}`);

  // Only now, with the target accepted, is anything created.
  newDirectory(ctx, ctx.targetRoot);
  initializeStateDirectory(ctx);
  const lock = newInputLock(ctx, s);
  const checkpoint = startCheckpoint(ctx, 'phase-0', lock.inputDigest, 'record and verify the locked inputs');
  const lockPath = path.join(ctx.stateRoot, 'input-lock.json');
  if (checkpoint.action === 'Skip' && existsSync(lockPath)) {
    const existing = LockSchema.parse(JSON.parse(readFileSync(lockPath, 'utf8')));
    if (existing.inputDigest !== lock.inputDigest) throw new Error(`input-lock.json digest ${existing.inputDigest} does not match the recomputed ${lock.inputDigest}.`);
    log(ctx, 'INFO', 'Phase 0 already locked; inputs unchanged.');
    return existing;
  }
  setFile(ctx, lockPath, JSON.stringify(lock, null, 2));
  setFile(ctx, path.join(ctx.stateRoot, 'guards.json'), JSON.stringify({target: target.resolvedTarget, protectedRoots, guards}, null, 2));
  setFile(ctx, path.join(ctx.stateRoot, 'hooks.lock.json'), JSON.stringify(lock.hooks, null, 2));
  log(ctx, 'INFO', `chromium      ${lock.chromium.commit}`);
  log(ctx, 'INFO', `depot_tools   ${lock.depotTools.commit}`);
  log(ctx, 'INFO', `DEPS sha256   ${lock.deps.sha256}`);
  log(ctx, 'INFO', `args.gn sha256 ${lock.build.argsGnSha256} (template ${lock.build.argsTemplate} ${lock.build.argsTemplateSha256})`);
  log(ctx, 'INFO', `target        ${lock.build.targetOs}/${lock.build.targetCpu}, custom_vars checkout_configuration=${String(lock.gclient.customVars.checkout_configuration)}`);
  log(ctx, 'INFO', `custom_deps   ${lock.gclient.customDeps.length} paths -> None`);
  const d = lock.depsSummary as Record<string, number>;
  log(ctx, 'INFO', `deps selected ${d.remaining} = ${d.git} git + ${d.cipd} cipd + ${d.gcs} gcs (matches the pinned baseline)`);
  log(ctx, 'INFO', `hooks         ${lock.hooks.selected} of ${lock.hooks.total} selected on win/x64`);
  log(ctx, 'INFO', `input digest  ${lock.inputDigest}`);
  completeCheckpoint(ctx, checkpoint, 0, 'completed', {lock: 'input-lock.json', guardCount: guards.length});
  return lock;
}

async function main(o: Record<string, unknown>): Promise<number> {
  const bootstrapRoot = path.join(root, 'scripts', 'lib', 'bootstrap');
  const referenceCheckout = o.referenceCheckout ? path.resolve(String(o.referenceCheckout)) : root;
  const depotTools = o.depotTools ? path.resolve(String(o.depotTools)) : process.env.SHOT_DEPOT_TOOLS ?? path.join(path.dirname(referenceCheckout), 'depot_tools');
  const customDepsFile = o.customDepsFile ? path.resolve(root, String(o.customDepsFile)) : path.join(bootstrapRoot, 'custom-deps-batch1.txt');
  const phases = String(o.phase ?? '0,1').split(',').map((p) => Number(p.trim()));
  for (const p of phases) if (p !== 0 && p !== 1) throw new Error(`--phase takes 0, 1 or 0,1; got ${o.phase}`);
  const dryRun = o.dryRun === true;
  const targetRoot = path.resolve(String(o.targetRoot));

  const ctx = newRunContext({
    targetRoot, solutionName: String(o.solutionName), depotTools, referenceCheckout, pinnedChromiumCommit: String(o.chromiumCommit),
    pinnedDepotToolsCommit: String(o.depotToolsCommit), bootstrapRoot, dryRun, force: o.force === true,
  });
  // The reference checkout, the directory above it (which holds the working
  // .gclient) and depot_tools are never valid targets.
  const protectedRoots = [referenceCheckout, path.dirname(referenceCheckout), depotTools, bootstrapRoot, ...String(o.protectedRoot ?? '').split(';').filter(Boolean)];
  const s: Settings = {
    chromiumCommit: String(o.chromiumCommit), chromiumRemote: String(o.chromiumRemote), depotTools, depotToolsCommit: String(o.depotToolsCommit), referenceCheckout,
    forkRef: String(o.forkRef), skipForkOverlay: o.skipForkOverlay === true, solutionName: String(o.solutionName), customDepsFile,
    checkoutConfiguration: String(o.checkoutConfiguration), targetOs: String(o.targetOs), targetCpu: String(o.targetCpu), gnArgsTemplate: String(o.gnArgsTemplate),
    gnOutDir: String(o.gnOutDir), gnTarget: String(o.gnTarget), syncJobs: Number(o.syncJobs), minimumFreeGiB: Number(o.minimumFreeGib),
    allowNonEmpty: o.allowNonEmpty === true, allowAncestorGclient: o.allowAncestorGclient === true, allowDepotToolsDrift: o.allowDepotToolsDrift === true,
    allowMissingSdk: o.allowMissingSdk === true, skipSync: o.skipSync === true, skipHooks: o.skipHooks === true, skipGnGen: o.skipGnGen === true,
    includeDeps: o.includeDepsInLock === true, python: '',
  };
  if (s.targetOs !== 'win' || s.targetCpu !== 'x64') throw new Error('only --target-os win and --target-cpu x64 are supported');
  s.python = depotToolsPython(depotTools);

  log(ctx, 'STEP', `shot bootstrap run ${ctx.runId} -> ${ctx.targetRoot}`);
  if (ctx.dryRun) log(ctx, 'WARN', 'dry run: guards and the input lock are computed for real; nothing is fetched, written or built.');

  let lock: Lock | null = null;
  if (phases.includes(0)) lock = phase0(ctx, s, protectedRoots);
  if (phases.includes(1)) {
    if (!lock) {
      // Phase 1 on its own still needs the lock; read the one Phase 0 wrote
      // and refuse to guess if it is absent.
      const lockPath = path.join(ctx.stateRoot, 'input-lock.json');
      if (!existsSync(lockPath)) throw new Error(`Phase 1 requires a Phase 0 lock at ${lockPath}. Run --phase 0 first.`);
      lock = LockSchema.parse(JSON.parse(readFileSync(lockPath, 'utf8')));
      initializeStateDirectory(ctx);
      // Containment guards are cheap and are re-run every time: a target that
      // became a symlink into the reference checkout between phases must not
      // be written to.
      assertTargetDirectory(ctx.targetRoot, protectedRoots, s.minimumFreeGiB, s.allowAncestorGclient, s.allowNonEmpty);
    }
    await phase1(ctx, lock, s);
  }
  log(ctx, 'STEP', 'done.');
  if (ctx.deviations.length > 0) log(ctx, 'WARN', `${ctx.deviations.length} deviation(s) recorded; see bootstrap-state/measurements/phase-1-report.json.`);
  return 0;
}

// The switches are read off argv: cac 7 registers a boolean option under its
// camelCase name only, so a switch followed by a value would swallow it.
const SWITCHES = ['--dry-run', '--force', '--skip-fork-overlay', '--allow-non-empty', '--allow-ancestor-gclient', '--allow-depot-tools-drift', '--allow-missing-sdk', '--skip-sync', '--skip-hooks', '--skip-gn-gen', '--include-deps-in-lock'];
const argv = process.argv.slice(2);
const switches = Object.fromEntries(SWITCHES.map((sw) => [sw.slice(2).replace(/-([a-z])/g, (_, c: string) => c.toUpperCase()), argv.includes(sw)]));
const cli = cac('bootstrap');
cli.command('', 'lock the inputs and bring up an isolated //shot:shot checkout to gn gen')
    .option('--target-root <dir>', 'a new or empty directory that will hold .gclient and the src/ solution')
    .option('--phase <list>', 'which phases to run: 0, 1 or 0,1', {default: '0,1'})
    .option('--chromium-commit <sha>', 'the pinned upstream commit', {default: 'c0bba1026178fe2a8b441fead7928b697a801c1e'})
    .option('--chromium-remote <url>', 'where to clone from', {default: 'https://chromium.googlesource.com/chromium/src.git'})
    .option('--depot-tools-commit <sha>', 'the pinned depot_tools commit', {default: '13febbee9ece9e03df923f69d540afc63c6db93e'})
    .option('--depot-tools <dir>', 'the pinned depot_tools checkout (default: SHOT_DEPOT_TOOLS, or ../depot_tools)')
    .option('--reference-checkout <dir>', 'the checkout to read the pinned DEPS and the fork commits from (default: this repository)')
    .option('--fork-ref <ref>', 'tip of the fork work replayed on top of the pin', {default: 'HEAD'})
    .option('--skip-fork-overlay', 'lock and check out stock upstream at the pin')
    .option('--solution-name <name>', 'the gclient solution directory', {default: 'src'})
    .option('--custom-deps-file <file>', 'the custom_deps = None batch (default: scripts/lib/bootstrap/custom-deps-batch1.txt)')
    .option('--checkout-configuration <name>', '.gclient custom_vars checkout_configuration', {default: 'small'})
    .option('--target-os <os>', 'win', {default: 'win'})
    .option('--target-cpu <cpu>', 'x64', {default: 'x64'})
    .option('--gn-args-template <label>', 'the args.gn import', {default: '//build/args/shot.gn'})
    .option('--gn-out-dir <dir>', 'the build directory', {default: 'out/Shot'})
    .option('--gn-target <label>', 'the target the lock is about', {default: '//shot:shot'})
    .option('--sync-jobs <n>', 'gclient --jobs', {default: 16})
    .option('--minimum-free-gib <n>', 'refuse a volume with less free space', {default: 200})
    .option('--protected-root <dirs>', 'extra directories the target may not touch, semicolon-separated')
    .option('--allow-non-empty', 'accept a populated target directory')
    .option('--allow-ancestor-gclient', 'accept a .gclient above the target')
    .option('--allow-depot-tools-drift', 'accept a depot_tools that is not at the pinned commit')
    .option('--allow-missing-sdk', 'record a missing Windows SDK as a deviation instead of failing')
    .option('--skip-sync', 'do not fetch dependencies')
    .option('--skip-hooks', 'do not run the gclient hooks')
    .option('--skip-gn-gen', 'do not configure')
    .option('--include-deps-in-lock', 'write the per-dependency table into the lock')
    .option('--force', 're-run a completed checkpoint; never deletes anything')
    .option('--dry-run', 'guards and the input lock only; nothing is fetched, written or built')
    .action(async (options: Record<string, unknown>) => {
      if (!options.targetRoot) throw new Error('--target-root is required');
      process.exitCode = await main({...options, ...switches});
    });
cli.help();
try {
  cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => !SWITCHES.includes(a))], {run: false});
  if (!cli.options.help) await cli.runMatchedCommand();
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exitCode = 1;
}
