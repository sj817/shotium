// Shared plumbing for the bootstrap: run context, logging, checkpoints,
// guarded writes, external process execution and the measurement primitives.
//
// Every filesystem mutation in the bootstrap goes through the guarded writes
// here, and they all refuse to write outside the target root. That is the
// mechanical half of the Phase 0 rule "never touch the reference checkout":
// the path guards decide which root is legal once, and nothing downstream
// can quietly write somewhere else.

import {createHash} from 'node:crypto';
import {appendFileSync, existsSync, mkdirSync, readdirSync, readFileSync, statSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {execa, execaSync} from 'execa';
import pc from 'picocolors';

export type Level = 'TRACE' | 'INFO' | 'STEP' | 'WARN' | 'ERROR';

export interface Deviation {
  id: string;
  reason: string;
  status: 'applied' | 'planned' | 'skipped' | 'required-not-applied';
  detail: unknown;
  recordedUtc: string;
}

export interface RunContext {
  runId: string;
  targetRoot: string;
  solutionName: string;
  // DEPS hardcodes an 'src/' prefix on every entry, so the solution directory
  // must literally be named 'src' next to .gclient.
  srcRoot: string;
  gclientFile: string;
  stateRoot: string;
  logRoot: string;
  checkpointRoot: string;
  measurementRoot: string;
  depotTools: string;
  referenceCheckout: string;
  pinnedChromiumCommit: string;
  pinnedDepotToolsCommit: string;
  bootstrapRoot: string;
  dryRun: boolean;
  force: boolean;
  transcriptFile: string | null;
  deviations: Deviation[];
}

const utcNow = () => new Date().toISOString();
const utcStamp = () => new Date().toISOString().replace(/[-:]/g, '').replace(/T/, '-').slice(0, 15);

export function newRunContext(o: {targetRoot: string; solutionName: string; depotTools: string; referenceCheckout: string; pinnedChromiumCommit: string; pinnedDepotToolsCommit: string; bootstrapRoot: string; dryRun: boolean; force: boolean}): RunContext {
  const stateRoot = path.join(o.targetRoot, 'bootstrap-state');
  return {
    runId: utcStamp(),
    targetRoot: o.targetRoot,
    solutionName: o.solutionName,
    srcRoot: path.join(o.targetRoot, o.solutionName),
    gclientFile: path.join(o.targetRoot, '.gclient'),
    stateRoot,
    logRoot: path.join(stateRoot, 'logs'),
    checkpointRoot: path.join(stateRoot, 'checkpoints'),
    measurementRoot: path.join(stateRoot, 'measurements'),
    depotTools: o.depotTools,
    referenceCheckout: o.referenceCheckout,
    pinnedChromiumCommit: o.pinnedChromiumCommit,
    pinnedDepotToolsCommit: o.pinnedDepotToolsCommit,
    bootstrapRoot: o.bootstrapRoot,
    dryRun: o.dryRun,
    force: o.force,
    transcriptFile: null,
    deviations: [],
  };
}

export function initializeStateDirectory(ctx: RunContext): void {
  for (const dir of [ctx.stateRoot, ctx.logRoot, ctx.checkpointRoot, ctx.measurementRoot]) newDirectory(ctx, dir);
  if (!ctx.dryRun) ctx.transcriptFile = path.join(ctx.logRoot, `bootstrap-${ctx.runId}.log`);
}

export function log(ctx: RunContext, level: Level, message: string): void {
  const stamp = new Date().toISOString();
  const line = `[${stamp}] ${ctx.dryRun ? 'DRYRUN ' : ''}${level.padEnd(5)} ${message}`;
  const paint = {ERROR: pc.red, WARN: pc.yellow, STEP: pc.cyan, TRACE: pc.gray, INFO: (s: string) => s}[level];
  console.log(paint(line));
  if (ctx.transcriptFile) appendFileSync(ctx.transcriptFile, line + '\n');
}

export const sha256Text = (text: string) => createHash('sha256').update(text, 'utf8').digest('hex');
export const sha256File = (file: string) => createHash('sha256').update(readFileSync(file)).digest('hex');

// Stable JSON for digesting: key order and formatting must not drift, or an
// unchanged input would produce a new digest and refuse to resume.
export function canonicalJson(value: unknown): string {
  if (value === null || value === undefined) return 'null';
  if (typeof value === 'string' || typeof value === 'boolean' || typeof value === 'number') return JSON.stringify(value);
  if (Array.isArray(value)) return '[' + value.map(canonicalJson).join(',') + ']';
  if (typeof value === 'object') {
    const o = value as Record<string, unknown>;
    return '{' + Object.keys(o).sort().map((k) => `${JSON.stringify(k)}:${canonicalJson(o[k])}`).join(',') + '}';
  }
  return JSON.stringify(String(value));
}

export const inputDigest = (value: unknown) => sha256Text(canonicalJson(value));

// --- guarded writes ---------------------------------------------------------

export function assertWritablePath(ctx: RunContext, p: string): void {
  const full = path.resolve(p);
  const root = path.resolve(ctx.targetRoot);
  const rootWithSep = root.replace(/[\\/]+$/, '') + path.sep;
  if (full.toLowerCase() !== root.toLowerCase() && !full.toLowerCase().startsWith(rootWithSep.toLowerCase())) {
    throw new Error(`Refusing to write outside the bootstrap target root.\n  target: ${root}\n  path:   ${full}`);
  }
}

export function newDirectory(ctx: RunContext, p: string): string {
  assertWritablePath(ctx, p);
  if (existsSync(p)) {
    if (!statSync(p).isDirectory()) throw new Error(`Expected a directory but found a file: ${p}`);
    return p;
  }
  if (ctx.dryRun) {
    console.log(pc.gray(`[dryrun] mkdir ${p}`));
    return p;
  }
  mkdirSync(p, {recursive: true});
  return p;
}

// Writes a bootstrap-owned file. Overwrites only files this bootstrap owns
// (state dir, .gclient, args.gn); it never overwrites checkout content.
export function setFile(ctx: RunContext, p: string, content: string, noClobber = false): string {
  assertWritablePath(ctx, p);
  if (noClobber && existsSync(p)) throw new Error(`Refusing to overwrite existing file: ${p}`);
  if (ctx.dryRun) {
    console.log(pc.gray(`[dryrun] write ${p} (${content.length} chars)`));
    return p;
  }
  const parent = path.dirname(p);
  if (parent && !existsSync(parent)) newDirectory(ctx, parent);
  // No BOM: gclient, GN and git all read these files as plain UTF-8.
  writeFileSync(p, content, 'utf8');
  return p;
}

// --- checkpoints ------------------------------------------------------------

export interface CheckpointRecord {
  name: string;
  description: string;
  status: 'running' | 'completed' | 'failed' | 'skipped';
  startUtc: string;
  endUtc: string | null;
  exitCode: number | null;
  inputDigest: string;
  runId: string;
  dryRun: boolean;
  data: unknown;
}

export interface Checkpoint {
  action: 'Run' | 'Skip';
  name: string;
  path: string;
  record: CheckpointRecord;
}

export const checkpointPath = (ctx: RunContext, name: string) => path.join(ctx.checkpointRoot, `${name.replace(/[^A-Za-z0-9._-]/g, '_')}.json`);

// Opens (or resumes) a phase/step checkpoint. A completed checkpoint with a
// matching input digest is skipped. A completed checkpoint with a *different*
// digest is a hard error: the inputs moved under a partially built tree.
// --force is the only way past it, and it never deletes anything.
export function startCheckpoint(ctx: RunContext, name: string, digest: string, description = ''): Checkpoint {
  const file = checkpointPath(ctx, name);
  let previous: CheckpointRecord | null = null;
  if (existsSync(file)) previous = JSON.parse(readFileSync(file, 'utf8')) as CheckpointRecord;
  if (previous) {
    if (previous.inputDigest !== digest && !ctx.force) {
      throw new Error(`Checkpoint '${name}' was recorded with a different input digest.\n  recorded: ${previous.inputDigest}\n  current:  ${digest}\nThe locked inputs changed after this step ran. Re-run in a new empty target\ndirectory, or pass --force if you have decided the mismatch is intentional.\nNothing was deleted.\n`);
    }
    if (previous.status === 'completed' && !ctx.force) {
      log(ctx, 'INFO', `checkpoint '${name}' already completed at ${previous.endUtc}; skipping.`);
      return {action: 'Skip', name, path: file, record: previous};
    }
    if (previous.status === 'running') log(ctx, 'WARN', `checkpoint '${name}' was interrupted (started ${previous.startUtc}); resuming.`);
  }
  const record: CheckpointRecord = {name, description, status: 'running', startUtc: utcNow(), endUtc: null, exitCode: null, inputDigest: digest, runId: ctx.runId, dryRun: ctx.dryRun, data: {}};
  if (!ctx.dryRun) setFile(ctx, file, JSON.stringify(record, null, 2));
  return {action: 'Run', name, path: file, record};
}

export function completeCheckpoint(ctx: RunContext, checkpoint: Checkpoint, exitCode = 0, status: CheckpointRecord['status'] = 'completed', data?: unknown): CheckpointRecord {
  const record = {...checkpoint.record, status, endUtc: utcNow(), exitCode};
  if (data !== undefined) record.data = data;
  if (!ctx.dryRun) setFile(ctx, checkpoint.path, JSON.stringify(record, null, 2));
  return record;
}

// --- external processes -----------------------------------------------------

export interface ProcessResult {
  command: string;
  exitCode: number | null;
  elapsedSeconds: number;
  logFile: string | undefined;
  executed: boolean;
  workingDirectory: string;
}

// Runs an external command, tees combined output to a log and times it.
// Non-zero exit is fatal unless allowed; "fail loudly" is the default
// everywhere in this bootstrap. In dry-run the command is recorded and not
// executed.
export async function runProcess(ctx: RunContext, o: {file: string; args?: string[]; cwd: string; logFile?: string; env?: Record<string, string>; allowNonZeroExit?: boolean; purpose?: string}): Promise<ProcessResult> {
  const args = o.args ?? [];
  const printable = `${o.file} ${args.join(' ')}`;
  log(ctx, 'STEP', `run: ${printable}`);
  if (o.purpose) log(ctx, 'TRACE', `why: ${o.purpose}`);
  if (ctx.dryRun) return {command: printable, exitCode: null, elapsedSeconds: 0, logFile: o.logFile, executed: false, workingDirectory: o.cwd};
  if (!existsSync(o.cwd)) throw new Error(`Working directory does not exist: ${o.cwd}`);
  if (o.logFile) {
    assertWritablePath(ctx, o.logFile);
    const parent = path.dirname(o.logFile);
    if (parent && !existsSync(parent)) newDirectory(ctx, parent);
  }
  const started = process.hrtime.bigint();
  // The child's stderr is merged into the log so it is a faithful transcript:
  // gclient and the hooks write progress to stderr. It also reaches the
  // console.
  const child = execa(o.file, args, {cwd: o.cwd, env: {...process.env, ...(o.env ?? {})}, reject: false, all: true, buffer: false});
  child.all!.on('data', (chunk: Buffer) => {
    process.stdout.write(chunk);
    if (o.logFile) appendFileSync(o.logFile, chunk);
  });
  const result = await child;
  const elapsed = Math.round(Number(process.hrtime.bigint() - started) / 1e6) / 1000;
  const exitCode = result.exitCode ?? null;
  log(ctx, 'INFO', `exit ${exitCode} after ${elapsed}s`);
  if (exitCode !== 0 && !o.allowNonZeroExit) throw new Error(`Command failed with exit code ${exitCode}: ${printable}\nLog: ${o.logFile}`);
  return {command: printable, exitCode, elapsedSeconds: elapsed, logFile: o.logFile, executed: true, workingDirectory: o.cwd};
}

// Read-only git query against any repository, including the reference
// checkout. Only plumbing that cannot mutate a working tree belongs here.
const READ_ONLY_GIT = new Set(['rev-parse', 'cat-file', 'log', 'show', 'status', 'diff', 'diff-tree', 'merge-base', 'rev-list', 'config', 'format-patch', 'describe', 'count-objects', 'ls-tree', 'symbolic-ref']);

export function gitRead(repo: string, args: string[], allowFailure = false): {exitCode: number; output: string[]} {
  if (args.length === 0 || !READ_ONLY_GIT.has(args[0])) throw new Error(`gitRead only permits read-only git subcommands; got '${args.join(' ')}'.`);
  const r = execaSync('git', ['-C', repo, ...args], {reject: false, all: true, maxBuffer: 1 << 28});
  const code = r.exitCode ?? 1;
  if (code !== 0 && !allowFailure) throw new Error(`git -C ${repo} ${args.join(' ')} failed (${code}):\n${r.all}`);
  return {exitCode: code, output: (r.all ?? '').split(/\r?\n/).filter((l, i, a) => !(i === a.length - 1 && l === ''))};
}

export const firstLine = (lines: string[]) => (lines[0] ?? '').trim();

// --- measurements -----------------------------------------------------------

export interface TreeMeasure {
  Path: string;
  Exists: boolean;
  Files: number;
  LogicalBytes: number;
  LogicalGiB: number;
}

export function measureTree(p: string): TreeMeasure {
  if (!existsSync(p)) return {Path: p, Exists: false, Files: 0, LogicalBytes: 0, LogicalGiB: 0};
  let files = 0, bytes = 0;
  const stack = [p];
  while (stack.length) {
    const dir = stack.pop()!;
    let entries: string[];
    try {
      entries = readdirSync(dir);
    } catch {
      continue;
    }
    for (const name of entries) {
      const full = path.join(dir, name);
      try {
        const st = statSync(full);
        if (st.isDirectory()) stack.push(full);
        else {
          files++;
          bytes += st.size;
        }
      } catch {
        // unreadable entry; skipped as Get-ChildItem -ErrorAction SilentlyContinue did
      }
    }
  }
  return {Path: p, Exists: true, Files: files, LogicalBytes: bytes, LogicalGiB: Math.round((bytes / 2 ** 30) * 1000) / 1000};
}

export interface VolumeSnapshot {
  DriveLetter: string;
  Size: number | null;
  SizeRemaining: number | null;
  CapturedUtc?: string;
  Error?: string;
}

export function volumeSnapshot(p: string): VolumeSnapshot {
  const letter = path.resolve(p)[0];
  try {
    const r = execaSync('pwsh', ['-NoProfile', '-Command', `$v = Get-Volume -DriveLetter ${letter} -ErrorAction Stop; Write-Output ("{0} {1}" -f $v.Size, $v.SizeRemaining)`], {reject: false});
    const [size, remaining] = r.stdout.trim().split(/\s+/).map(Number);
    if (r.exitCode !== 0 || Number.isNaN(size)) throw new Error(r.stderr.trim() || 'Get-Volume failed');
    return {DriveLetter: letter, Size: size, SizeRemaining: remaining, CapturedUtc: utcNow()};
  } catch (e) {
    return {DriveLetter: letter, Size: null, SizeRemaining: null, Error: e instanceof Error ? e.message : String(e)};
  }
}

export interface AdapterStats {
  Name: string;
  ReceivedBytes: number;
  SentBytes: number;
}

// Adapter-level byte counters. These include everything else running on the
// machine; any report built from them has to say so.
export function networkSnapshot(): AdapterStats[] {
  try {
    const r = execaSync('pwsh', ['-NoProfile', '-Command', 'Get-NetAdapterStatistics -ErrorAction Stop | Select-Object Name,ReceivedBytes,SentBytes | ConvertTo-Json -Compress'], {reject: false});
    if (r.exitCode !== 0 || !r.stdout.trim()) return [];
    const parsed = JSON.parse(r.stdout) as AdapterStats | AdapterStats[];
    return (Array.isArray(parsed) ? parsed : [parsed]).map((a) => ({Name: a.Name, ReceivedBytes: Number(a.ReceivedBytes), SentBytes: Number(a.SentBytes)}));
  } catch {
    return [];
  }
}

export function networkDelta(before: AdapterStats[], after: AdapterStats[]) {
  const prior = new Map(before.map((a) => [a.Name, a]));
  const rows = after.filter((b) => prior.has(b.Name)).map((b) => ({
    Name: b.Name,
    ReceivedBytesDelta: b.ReceivedBytes - prior.get(b.Name)!.ReceivedBytes,
    SentBytesDelta: b.SentBytes - prior.get(b.Name)!.SentBytes,
  }));
  return {
    PerAdapter: rows,
    TotalReceivedBytesDelta: rows.reduce((s, r) => s + r.ReceivedBytesDelta, 0),
    TotalSentBytesDelta: rows.reduce((s, r) => s + r.SentBytesDelta, 0),
    Caveat: 'Adapter-level counters include unrelated traffic on this host.',
  };
}

export function saveMeasurement(ctx: RunContext, name: string, data: unknown): string {
  const file = path.join(ctx.measurementRoot, `${name.replace(/[^A-Za-z0-9._-]/g, '_')}.json`);
  setFile(ctx, file, JSON.stringify(data, null, 2));
  return file;
}

// Records a documented departure from stock upstream Chromium. Anything this
// bootstrap changes relative to the pinned upstream tree is a deviation and
// must be visible in the run record.
export function addDeviation(ctx: RunContext, id: string, reason: string, status: Deviation['status'], detail?: unknown): Deviation {
  const entry: Deviation = {id, reason, status, detail: detail ?? null, recordedUtc: utcNow()};
  ctx.deviations.push(entry);
  log(ctx, status === 'required-not-applied' ? 'WARN' : 'INFO', `deviation [${status}] ${id}: ${reason}`);
  return entry;
}
