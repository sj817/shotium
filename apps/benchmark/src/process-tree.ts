import fs from 'node:fs';
import process from 'node:process';
import {execa} from 'execa';
import si from 'systeminformation';
import {SETTLE} from './constants.ts';
import {InfrastructureError} from './errors.ts';
import {median, percentile, relativeDrift, round} from './statistics.ts';

const sleep = (milliseconds: number) => new Promise<void>((resolve) => setTimeout(resolve, milliseconds));

async function readProcesses() {
  try {
    return await si.processes();
  } catch (error) {
    throw new InfrastructureError('systeminformation could not enumerate processes', error);
  }
}

function identity(entry) {
  if (!Number.isInteger(entry?.pid) || entry.pid <= 0 || !String(entry?.birth_token || '').trim()) {
    throw new InfrastructureError(`process identity is missing PID or stable birth token: ${JSON.stringify({
      pid: entry?.pid,
      started: entry?.started,
      birth_token: entry?.birth_token,
      name: entry?.name,
    })}`);
  }
  return `${entry.pid}:${entry.birth_token}`;
}

export function parseLinuxProcStat(stat: string) {
  const open = stat.indexOf('(');
  const close = stat.lastIndexOf(')');
  if (open <= 0 || close <= open) {
    throw new InfrastructureError('Linux /proc stat has an invalid process name field');
  }
  const pid = Number(stat.slice(0, open).trim());
  const fields = stat.slice(close + 2).trim().split(/\s+/);
  const parentPid = Number(fields[1]);
  const startTicks = fields[19];
  if (!Number.isInteger(pid) || pid <= 0 || !Number.isInteger(parentPid) || !startTicks) {
    throw new InfrastructureError('Linux /proc stat is missing PID, parent PID or starttime');
  }
  return {
    pid,
    parent_pid: parentPid,
    started: null,
    birth_token: `linux-proc-startticks:${startTicks}`,
    name: stat.slice(open + 1, close),
  };
}

export async function processIdentityForPid(pid: number) {
  if (!Number.isInteger(pid) || pid <= 0) {
    throw new InfrastructureError(`invalid process id ${pid}`);
  }
  if (process.platform !== 'linux') {
    const snapshot = await processSnapshot([pid]);
    return snapshot.processes.find((entry) => entry.pid === pid) || null;
  }
  let stat;
  try {
    stat = fs.readFileSync(`/proc/${pid}/stat`, 'utf8');
  } catch (error) {
    if (error?.code === 'ENOENT') return null;
    throw new InfrastructureError(`could not read /proc identity for ${pid}`, error);
  }
  const parsed = parseLinuxProcStat(stat);
  let command = '';
  let rssBytes = 0;
  try {
    command = fs.readFileSync(`/proc/${pid}/cmdline`, 'utf8').replaceAll('\0', ' ').trim();
  } catch (error) {
    if (error?.code !== 'ENOENT') {
      throw new InfrastructureError(`could not read /proc command for ${pid}`, error);
    }
  }
  try {
    const status = fs.readFileSync(`/proc/${pid}/status`, 'utf8');
    const rss = status.match(/^VmRSS:\s+(\d+)\s+kB$/m);
    if (rss) rssBytes = Number(rss[1]) * 1024;
  } catch (error) {
    if (error?.code !== 'ENOENT') {
      throw new InfrastructureError(`could not read /proc RSS for ${pid}`, error);
    }
  }
  return {...parsed, command, rss_bytes: rssBytes, cpu_percent: 0};
}

async function birthToken(entry, refresh = false) {
  if (process.platform === 'linux') {
    let stat;
    try {
      stat = fs.readFileSync(`/proc/${entry.pid}/stat`, 'utf8');
    } catch (error) {
      if (error?.code === 'ENOENT') return null;
      throw new InfrastructureError(`could not read /proc identity for ${entry.pid}`, error);
    }
    const fields = stat.slice(stat.lastIndexOf(')') + 2).split(' ');
    const startTicks = fields[19];
    if (!startTicks) throw new Error(`Linux process ${entry.pid} has no /proc starttime`);
    return `linux-proc-startticks:${startTicks}`;
  }
  if (process.platform === 'win32') {
    if (!String(entry.started || '').trim()) {
      throw new InfrastructureError(`Windows process ${entry.pid} has no CreationDate`);
    }
    return `windows-creation-date:${entry.started}`;
  }
  if (process.platform === 'darwin') throw new Error('macOS birth tokens must be resolved as a batch');
  throw new InfrastructureError(`stable process birth tokens are unsupported on ${process.platform}`);
}

async function normalizeBirthTokens(entries) {
  if (process.platform !== 'darwin') {
    const normalized = await Promise.all(entries.map(async (entry) => {
      const token = await birthToken(entry);
      return token ? {...entry, birth_token: token} : null;
    }));
    return normalized.filter(Boolean);
  }
  if (!entries.length) return [];
  let result;
  try {
    result = await execa('ps', [
      '-o', 'pid=,lstart=', '-p', entries.map((entry) => entry.pid).join(','),
    ], {timeout: 2000, reject: false});
  } catch (error) {
    throw new InfrastructureError('could not query macOS process birth times', error);
  }
  const tokens = new Map<number, string>();
  for (const line of result.stdout.split(/\r?\n/)) {
    const match = line.match(/^\s*(\d+)\s+(.+?)\s*$/);
    if (match) tokens.set(Number(match[1]), `darwin-lstart:${match[2].replace(/\s+/g, ' ')}`);
  }
  const normalized = [];
  for (const entry of entries) {
    const token = tokens.get(entry.pid);
    if (token) {
      normalized.push({...entry, birth_token: token});
      continue;
    }
    try {
      process.kill(entry.pid, 0);
    } catch (error) {
      if (error?.code === 'ESRCH') continue;
    }
    throw new InfrastructureError(`macOS process ${entry.pid} has no stable lstart value`);
  }
  return normalized;
}

function startedAt(entry) {
  const value = Date.parse(String(entry?.started || ''));
  return Number.isFinite(value) ? value : null;
}

export function selectOwnedProcessEntries(list, rootPids) {
  const byPid = new Map(list.map((entry) => [Number(entry.pid), entry]));
  const byParent = new Map();
  for (const entry of list) {
    const bucket = byParent.get(Number(entry.parentPid)) || [];
    bucket.push(entry);
    byParent.set(Number(entry.parentPid), bucket);
  }
  const selected = new Map<number, any>();
  const queue = [];
  for (const pid of rootPids.map(Number)) {
    const root = byPid.get(pid);
    if (root && !selected.has(pid)) {
      selected.set(pid, root);
      queue.push(root);
    }
  }
  while (queue.length) {
    const parent: any = queue.shift();
    const parentStarted = startedAt(parent);
    for (const child of byParent.get(Number(parent.pid)) || []) {
      if (selected.has(child.pid)) continue;
      const childStarted = startedAt(child);
      // Windows retains historical parent PIDs after the original parent has
      // exited. Once that PID is reused, following parentPid alone can absorb
      // arbitrary system processes into an owned tree. A real child cannot
      // predate its parent; missing timestamps are therefore not kill proof.
      if (parentStarted === null || childStarted === null || childStarted < parentStarted) continue;
      selected.set(child.pid, child);
      queue.push(child);
    }
  }
  return [...selected.values()];
}

export async function processSnapshot(rootPids) {
  const data = await readProcesses();
  const list = data.list || [];
  if (!list.length) {
    throw new InfrastructureError('systeminformation returned no process list; PID ownership cannot be verified');
  }
  const entries = await normalizeBirthTokens(selectOwnedProcessEntries(list, rootPids));
  return {
    at: new Date().toISOString(),
    processes: entries.map((entry) => ({
      pid: entry.pid,
      parent_pid: entry.parentPid,
      started: entry.started,
      birth_token: entry.birth_token,
      name: entry.name,
      command: entry.command,
      // systeminformation exposes process RSS in KiB on every backend.
      rss_bytes: (Number(entry.memRss) || 0) * 1024,
      cpu_percent: Number(entry.cpu) || 0,
    })),
    rss_bytes: entries.reduce((sum, entry) => sum + ((Number(entry.memRss) || 0) * 1024), 0),
    cpu_percent: entries.reduce((sum, entry) => sum + (Number(entry.cpu) || 0), 0),
  };
}

export async function waitForProcessTree(rootPids, {
  timeoutMs = 2000,
  intervalMs = 50,
  snapshot = processSnapshot,
}: {
  timeoutMs?: number;
  intervalMs?: number;
  snapshot?: typeof processSnapshot;
} = {}) {
  if (!rootPids.length || rootPids.some((pid) => !Number.isInteger(pid) || pid <= 0)) {
    throw new InfrastructureError('owned process-tree roots must be positive integer PIDs');
  }
  const deadline = Date.now() + timeoutMs;
  do {
    const tree = await snapshot(rootPids);
    const observed = new Set(tree.processes.map((entry) => entry.pid));
    if (rootPids.every((pid) => observed.has(pid))) return tree;
    if (Date.now() >= deadline) break;
    await sleep(intervalMs);
  } while (true);
  throw new InfrastructureError(
      `owned process tree for PID(s) ${rootPids.join(', ')} was not observable within ${timeoutMs}ms`);
}

export async function verifyProcessMonitoring() {
  const snapshot = await processSnapshot([process.pid]);
  const current = snapshot.processes.find((entry) => entry.pid === process.pid);
  if (!current) {
    throw new InfrastructureError(`PID ownership monitor cannot observe the benchmark process ${process.pid}`);
  }
  return current;
}

export class ProcessMonitor {
  rootPids: number[];
  intervalMs: number;
  timeline: any[];
  identities: Map<string, any>;
  running: boolean;
  loop: Promise<void> | undefined;
  snapshot: typeof processSnapshot;

  constructor(rootPids, intervalMs = 100, snapshot = processSnapshot) {
    this.rootPids = rootPids;
    this.intervalMs = intervalMs;
    this.timeline = [];
    this.identities = new Map();
    this.running = false;
    this.snapshot = snapshot;
  }

  #record(sample) {
    this.timeline.push(sample);
    for (const entry of sample.processes) {
      this.identities.set(identity(entry), entry);
    }
  }

  async start({requireRoots = false, timeoutMs = 2000} = {}) {
    this.running = true;
    if (requireRoots) {
      try {
        const sample = await waitForProcessTree(this.rootPids, {
          timeoutMs,
          intervalMs: this.intervalMs,
          snapshot: this.snapshot,
        });
        this.#record(sample);
      } catch (error) {
        this.running = false;
        throw error;
      }
      this.loop = this.#sampleLoop();
      return;
    }
    // Prime exact root identities before the first systeminformation sample.
    // Short-lived lifecycle clients can otherwise exit while the relatively
    // expensive process-table query is still in flight.
    const primed = [];
    for (const pid of this.rootPids) {
      try {
        const entry = await processIdentityForPid(pid);
        if (entry) {
          this.identities.set(identity(entry), entry);
          primed.push(entry);
        }
      } catch (error) {
        this.timeline.push({at: new Date().toISOString(), error: String(error)});
      }
    }
    if (primed.length) {
      this.timeline.push({
        at: new Date().toISOString(),
        processes: primed,
        rss_bytes: primed.reduce((sum, entry) => sum + (Number(entry.rss_bytes) || 0), 0),
        cpu_percent: primed.reduce((sum, entry) => sum + (Number(entry.cpu_percent) || 0), 0),
      });
    }
    this.loop = this.#sampleLoop();
  }

  async #sampleLoop() {
    while (this.running) {
      const started = performance.now();
      try {
        const sample = await this.snapshot(this.rootPids);
        this.#record(sample);
      } catch (error) {
        this.timeline.push({at: new Date().toISOString(), error: String(error)});
      }
      const elapsed = performance.now() - started;
      await sleep(Math.max(0, this.intervalMs - elapsed));
    }
  }

  async stop() {
    this.running = false;
    await this.loop;
    const valid = this.timeline.filter((sample) => Number.isFinite(sample.rss_bytes));
    const errors = this.timeline.filter((sample) => sample.error).map((sample) => sample.error);
    const observedRoots = new Set([...this.identities.values()].map((entry) => entry.pid));
    const peak = valid.reduce((best, sample) => !best || sample.rss_bytes > best.rss_bytes ? sample : best, null);
    return {
      timeline: this.timeline,
      identities: [...this.identities.values()],
      errors,
      trusted: errors.length === 0 && valid.length > 0 &&
        this.rootPids.every((pid) => observedRoots.has(pid)),
      peak_rss_bytes: peak?.rss_bytes ?? null,
      peak_cpu_percent: valid.length ? Math.max(...valid.map((sample) => sample.cpu_percent)) : null,
      observed_mean_period_ms: valid.length > 1 ?
        (Date.parse(valid.at(-1).at) - Date.parse(valid[0].at)) / (valid.length - 1) : null,
    };
  }
}

async function liveIdentityMap(expectedIdentities: any[] = []) {
  if (process.platform === 'linux') {
    const entries: Array<[string, any]> = [];
    for (const expected of expectedIdentities) {
      const live = await processIdentityForPid(expected.pid);
      if (live && identity(live) === identity(expected)) entries.push([identity(live), live]);
    }
    return new Map<string, any>(entries);
  }
  const data = await readProcesses();
  const list = data.list || [];
  if (!list.length) {
    throw new InfrastructureError('systeminformation returned no process list; live PID ownership cannot be verified');
  }
  const expected = new Map(expectedIdentities.map((entry) => [entry.pid, entry]));
  const liveEntries = await normalizeBirthTokens(list.filter((entry) => expected.has(entry.pid)));
  const entries: Array<[string, any]> = liveEntries.map((entry) => [identity(entry), entry]);
  return new Map<string, any>(entries);
}

export function collectAncestorPids(list, currentPid: number, currentParentPid: number) {
  const byPid = new Map(list.map((entry) => [Number(entry.pid), entry]));
  const protectedPids = new Set<number>([currentPid]);
  if (Number.isInteger(currentParentPid) && currentParentPid > 0) {
    protectedPids.add(currentParentPid);
  }
  let cursor = currentPid;
  const visited = new Set<number>();
  for (let depth = 0; depth < 128; depth += 1) {
    if (visited.has(cursor)) break;
    visited.add(cursor);
    const entry: any = byPid.get(cursor);
    if (!entry) break;
    const parentPid = Number(entry.parentPid);
    if (!Number.isInteger(parentPid) || parentPid <= 0 || visited.has(parentPid)) break;
    protectedPids.add(parentPid);
    cursor = parentPid;
  }
  return protectedPids;
}

async function protectedAncestorPids() {
  const data = await readProcesses();
  const list = data.list || [];
  if (!list.length) {
    throw new InfrastructureError('process ancestry is unavailable; refusing forced cleanup');
  }
  return collectAncestorPids(list, process.pid, process.ppid);
}

export function partitionProtectedProcesses(identities, protectedPids: Set<number>) {
  return {
    killable: identities.filter((entry) => !protectedPids.has(entry.pid)),
    protected: identities.filter((entry) => protectedPids.has(entry.pid)),
  };
}

export async function waitForIdentitiesToExit(identities, timeoutMs: number = SETTLE.gracefulExitMs) {
  const expected = new Set<string>(identities.map(identity));
  const deadline = Date.now() + timeoutMs;
  let remaining = [];
  do {
    const live = await liveIdentityMap(identities);
    remaining = [...expected].filter((key) => live.has(key)).map((key) => live.get(key));
    if (!remaining.length) return [];
    await sleep(250);
  } while (Date.now() < deadline);
  return remaining;
}

export async function terminateOwnedProcesses(identities) {
  const live = await liveIdentityMap(identities);
  const liveOwned = identities.filter((entry) => live.has(identity(entry)));
  const protectedPids = await protectedAncestorPids();
  const partitioned = partitionProtectedProcesses(liveOwned, protectedPids);
  const owned = partitioned.killable
      .sort((left, right) => right.pid - left.pid);
  for (const entry of owned) {
    try {
      process.kill(entry.pid, 'SIGTERM');
    } catch {}
  }
  let remaining = await waitForIdentitiesToExit(owned, SETTLE.forcedExitMs);
  for (const entry of remaining) {
    try {
      process.kill(entry.pid, 'SIGKILL');
    } catch {}
  }
  remaining = await waitForIdentitiesToExit(remaining, SETTLE.forcedExitMs);
  return [...remaining, ...partitioned.protected];
}

async function primeCpuLoad() {
  // systeminformation reports load as the delta since its previous call; the
  // first reading after process start covers the whole process lifetime.
  try {
    await si.currentLoad();
  } catch (error) {
    throw new InfrastructureError('systeminformation could not sample initial CPU load', error);
  }
}

export async function sampleHostLoad() {
  let load;
  let memory;
  try {
    [load, memory] = await Promise.all([si.currentLoad(), si.mem()]);
  } catch (error) {
    throw new InfrastructureError('systeminformation could not sample host stability', error);
  }
  return {
    at: new Date().toISOString(),
    cpu_percent: load.currentLoad,
    available_bytes: memory.available,
  };
}

export function cpuLimitFromBaseline(idleCpuP95: number, {
  floor = SETTLE.cpuLimit as number,
  margin = SETTLE.cpuLimitMargin as number,
  ceiling = SETTLE.cpuLimitMax as number,
}: {floor?: number; margin?: number; ceiling?: number} = {}) {
  if (!Number.isFinite(idleCpuP95)) return floor;
  return Math.min(ceiling, Math.max(floor, Math.round(idleCpuP95 + margin)));
}

/**
 * systeminformation reads the Windows process table through a PowerShell
 * Get-CimInstance query. Without a persistent session every sample spawns a
 * fresh powershell.exe (≈0.5 s of CPU on a 2-core runner); with two monitors
 * sampling back to back that alone held GitHub's Windows runners at 35-60% CPU
 * and made every stability check fail. Keep one PowerShell session per process.
 */
export function startProcessSampler() {
  if (process.platform === 'win32') si.powerShellStart();
}

export function stopProcessSampler() {
  if (process.platform === 'win32') si.powerShellRelease();
}

export async function calibrateHostLoad({durationMs = SETTLE.calibrationMs, monitors = 2}: {
  durationMs?: number;
  monitors?: number;
} = {}) {
  // Measure the runner the way a cell sees it: the CLI monitors the worker and
  // the worker monitors itself, so two process samplers run during every
  // stability check. Their cost belongs in the baseline, not in the noise.
  const samplers = Array.from({length: monitors}, () => new ProcessMonitor([process.pid], 100));
  for (const sampler of samplers) await sampler.start();
  const samples = [];
  try {
    await primeCpuLoad();
    const deadline = Date.now() + durationMs;
    do {
      await sleep(1000);
      samples.push(await sampleHostLoad());
    } while (Date.now() < deadline);
  } finally {
    for (const sampler of samplers) await sampler.stop();
  }
  const sorted = samples.map((sample) => sample.cpu_percent).sort((left, right) => left - right);
  const p50 = median(sorted);
  const p95 = percentile(sorted, 0.95);
  return {
    samples,
    sampler_monitors: monitors,
    idle_cpu_p50: round(p50, 2),
    idle_cpu_p95: round(p95, 2),
    cpu_limit: cpuLimitFromBaseline(p95),
  };
}

export async function waitForSystemStable({timeoutMs = SETTLE.cooldownTimeoutMs, cpuLimit = SETTLE.cpuLimit}: {
  timeoutMs?: number;
  cpuLimit?: number;
} = {}) {
  const deadline = Date.now() + timeoutMs;
  const samples = [];
  await primeCpuLoad();
  while (Date.now() < deadline) {
    samples.push(await sampleHostLoad());
    const recent = samples.slice(-SETTLE.stableSamples);
    const spanMs = recent.length > 1 ?
      Date.parse(recent.at(-1).at) - Date.parse(recent[0].at) : 0;
    if (recent.length === SETTLE.stableSamples && spanMs >= SETTLE.stableSpanMs &&
        recent.every((sample) => sample.cpu_percent <= cpuLimit) &&
        relativeDrift(recent.map((sample) => sample.available_bytes)) <= SETTLE.memoryDriftLimit) {
      return {stable: true, cpu_limit: cpuLimit, samples};
    }
    await sleep(1000);
  }
  return {stable: false, cpu_limit: cpuLimit, samples};
}
