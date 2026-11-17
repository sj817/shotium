import {createHash} from 'node:crypto';
import {closeSync, existsSync, mkdirSync, openSync, readFileSync, readSync, realpathSync, statSync, utimesSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {graphInputs, parseDeps} from './ninja-state.ts';

interface Entry {hash: string; time: number}

function hashFile(file: string, buffer: Buffer): string {
  const digest = createHash('sha256');
  const fd = openSync(file, 'r');
  try {
    for (;;) {
      const length = readSync(fd, buffer, 0, buffer.length, null);
      if (!length) break;
      digest.update(buffer.subarray(0, length));
    }
  } finally { closeSync(fd); }
  return digest.digest('hex');
}

// A changed input must be newer than cached outputs, even when switching
// to an older commit. All shards restoring this cache derive the same time.
function changedTimeFor(buildDir: string, headTime: number): number {
  const log = path.join(buildDir, '.ninja_log');
  return Math.max(headTime, existsSync(log) ? statSync(log).mtimeMs / 1000 + 1 : headTime);
}

function readState(stateFile: string): Record<string, Entry> | null {
  if (!existsSync(stateFile)) return null;
  const state = JSON.parse(readFileSync(stateFile, 'utf8')) as {version: number; files: Record<string, Entry>};
  if (state.version !== 1) throw new Error(`Unsupported input timestamp state in ${stateFile}`);
  return state.files;
}

function writeState(stateFile: string, files: Record<string, Entry>): void {
  mkdirSync(path.dirname(stateFile), {recursive: true});
  writeFileSync(stateFile, JSON.stringify({version: 1, files}));
}

// This state travels with the objects it describes, inside the build cache.
export class InputMtimes {
  private previous: Record<string, Entry> = {};
  private current: Record<string, Entry> = {};
  private buffer = Buffer.allocUnsafe(1024 * 1024);
  private changedTime: number;
  private hasState: boolean;
  private stateFile: string;
  private fallbackTime: number;

  constructor(stateFile: string, fallbackTime: number, headTime: number) {
    this.stateFile = stateFile;
    this.fallbackTime = fallbackTime;
    const previous = readState(stateFile);
    this.hasState = previous !== null;
    this.previous = previous ?? {};
    this.changedTime = changedTimeFor(path.dirname(stateFile), headTime);
  }

  time(file: string, key: string): number {
    const hash = hashFile(file, this.buffer);
    const previous = this.previous[key];
    const time = previous?.hash === hash ? previous.time
      : previous ? Math.max(this.changedTime, previous.time + 1)
      : this.hasState ? this.changedTime : this.fallbackTime;
    this.current[key] = {hash, time};
    return time;
  }

  save(): void {
    writeState(this.stateFile, this.current);
  }
}

// Git commit dates are not cache identities: the cache may have been built
// on another branch or with source bytes older than the checked-out revision.
// Unknown legacy sources must invalidate once, not inherit an old Git date.
export function createSourceMtimes(buildDir: string, headTime: number): InputMtimes {
  return new InputMtimes(path.join(buildDir, 'ci-source-inputs.json'), changedTimeFor(buildDir, headTime), headTime);
}

// Files outside the workspace that the restored objects were compiled
// against: the host toolchain's headers -- MSVC and the Windows Kits on
// Windows, the Xcode SDK behind sdk/xcode_links on macOS. A runner image
// refresh reinstalls them with the same bytes and new modification times,
// and ninja then rebuilds most of the engine: 40 minutes of ninja on
// windows-amd64 and 70 on macos-arm64 for a change that compiled nothing
// (run 44011084803), while linux-amd64, whose sysroot is in the tree,
// finished in 14 seconds. They get the same content-faithful time as the
// sources, with one difference: a file this state has never seen keeps its
// own time, which is what ninja compared before the state existed, so the
// first run with it and a newly included header behave as they always did.

/** The build-graph paths (relative to the build directory, or absolute) that resolve outside the workspace, as [graph path, file] pairs. */
export function hostInputs(buildDir: string, workspace: string, depPaths: Iterable<string>): Array<[string, string]> {
  const root = realpathSync.native(workspace);
  const inside = (real: string) => {
    const rel = path.relative(root, real);
    return rel === '' || (!rel.startsWith('..') && !path.isAbsolute(rel));
  };
  // The links are directory links (sdk/xcode_links/MacOSX26.2.sdk), so
  // resolving each directory once classifies every file under it.
  const dirs = new Map<string, string | null>();
  const result: Array<[string, string]> = [];
  for (const dep of new Set(depPaths)) {
    const normalized = dep.replace(/\\/g, '/');
    // The source tree, which the walk over the checkout already stamps.
    if (normalized.startsWith('../../') && !normalized.startsWith('../../../')) continue;
    const file = path.resolve(buildDir, dep);
    const dir = path.dirname(file);
    let real = dirs.get(dir);
    if (real === undefined) {
      try { real = realpathSync.native(dir); } catch { real = null; }
      dirs.set(dir, real);
    }
    if (real === null || inside(real)) continue;
    const resolved = path.join(real, path.basename(file));
    if (existsSync(resolved)) result.push([dep, resolved]);
  }
  return result;
}

export class HostInputMtimes {
  private previous: Record<string, Entry>;
  private current: Record<string, Entry> = {};
  private buffer = Buffer.allocUnsafe(1024 * 1024);
  private changedTime: number;
  private stateFile: string;

  constructor(buildDir: string, headTime: number) {
    this.stateFile = path.join(buildDir, 'ci-host-inputs.json');
    this.previous = readState(this.stateFile) ?? {};
    this.changedTime = changedTimeFor(buildDir, headTime);
  }

  /** The time to give the file, or null to leave it the time it has. */
  time(file: string, key: string): number | null {
    const hash = hashFile(file, this.buffer);
    const previous = this.previous[key];
    const time = previous === undefined ? null
      : previous.hash === hash ? previous.time
      : Math.max(this.changedTime, previous.time + 1);
    this.current[key] = {hash, time: time ?? statSync(file).mtimeMs / 1000};
    return time;
  }

  save(): void {
    writeState(this.stateFile, this.current);
  }
}

/** Stamp the host toolchain files a restored build directory's graph names; null when there is no graph to read. */
export function stampHostInputs(buildDir: string, workspace: string, headTime: number): {stamped: number; unseen: number; failed: number} | null {
  if (!existsSync(path.join(buildDir, 'build.ninja'))) return null;
  // The restored graph is the one the objects were built with. Host
  // headers are build-statement inputs (the system module map, the SDK
  // settings file every Apple compile names); clang-cl's deps log leaves
  // system headers out, other compilers' may name some.
  const graph = graphInputs(buildDir);
  const depsLog = path.join(buildDir, '.ninja_deps');
  if (existsSync(depsLog)) for (const dep of parseDeps(readFileSync(depsLog)).paths) graph.add(dep);
  const host = new HostInputMtimes(buildDir, headTime);
  const seen = {stamped: 0, unseen: 0, failed: 0};
  for (const [key, file] of hostInputs(buildDir, workspace, graph)) {
    const when = host.time(file, key);
    if (when === null) { seen.unseen++; continue; }
    try {
      utimesSync(file, when, when);
      seen.stamped++;
    } catch {
      // A read-only toolchain file keeps its time: ninja rebuilds what
      // reads it, as it did before this state existed.
      seen.failed++;
    }
  }
  host.save();
  return seen;
}
