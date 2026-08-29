import fs from 'node:fs';
import process from 'node:process';
import {execa} from 'execa';
import si from 'systeminformation';
import {SETTLE} from './constants.ts';
import {InfrastructureError} from './errors.ts';
import {relativeDrift} from './statistics.ts';

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

export async function processSnapshot(rootPids) {
  const data = await readProcesses();
  const list = data.list || [];
  if (!list.length) {
    throw new InfrastructureError('systeminformation returned no process list; PID ownership cannot be verified');
  }
  const byParent = new Map();
  for (const entry of list) {
    const bucket = byParent.get(entry.parentPid) || [];
    bucket.push(entry);
    byParent.set(entry.parentPid, bucket);
  }
  const roots = new Set(rootPids.map(Number));
  const selected = new Map<number, any>();
  const queue = [...roots];
  while (queue.length) {
    const pid = queue.shift();
    const entry = list.find((candidate) => candidate.pid === pid);
    if (entry && !selected.has(entry.pid)) selected.set(entry.pid, entry);
    for (const child of byParent.get(pid) || []) {
      if (selected.has(child.pid)) continue;
      selected.set(child.pid, child);
      queue.push(child.pid);
    }
  }
  const entries = await normalizeBirthTokens([...selected.values()]);
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

  constructor(rootPids, intervalMs = 100) {
    this.rootPids = rootPids;
    this.intervalMs = intervalMs;
    this.timeline = [];
    this.identities = new Map();
    this.running = false;
  }

  async start() {
    this.running = true;
    this.loop = this.#sampleLoop();
  }

  async #sampleLoop() {
    while (this.running) {
      const started = performance.now();
      try {
        const sample = await processSnapshot(this.rootPids);
        this.timeline.push(sample);
        for (const entry of sample.processes) {
          this.identities.set(identity(entry), entry);
        }
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
  const owned = identities
      .filter((entry) => entry.pid !== process.pid)
      .filter((entry) => live.has(identity(entry)))
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
  return remaining;
}

export async function waitForSystemStable({timeoutMs = SETTLE.cooldownTimeoutMs, cpuLimit = SETTLE.cpuLimit}: {
  timeoutMs?: number;
  cpuLimit?: number;
} = {}) {
  const deadline = Date.now() + timeoutMs;
  const samples = [];
  try {
    await si.currentLoad();
  } catch (error) {
    throw new InfrastructureError('systeminformation could not sample initial CPU load', error);
  }
  while (Date.now() < deadline) {
    let load;
    let memory;
    try {
      [load, memory] = await Promise.all([si.currentLoad(), si.mem()]);
    } catch (error) {
      throw new InfrastructureError('systeminformation could not sample host stability', error);
    }
    samples.push({
      at: new Date().toISOString(),
      cpu_percent: load.currentLoad,
      available_bytes: memory.available,
    });
    const recent = samples.slice(-(SETTLE.stableSamples + 1));
    const spanMs = recent.length > 1 ?
      Date.parse(recent.at(-1).at) - Date.parse(recent[0].at) : 0;
    if (recent.length === SETTLE.stableSamples + 1 && spanMs >= 3000 &&
        recent.every((sample) => sample.cpu_percent <= cpuLimit) &&
        relativeDrift(recent.map((sample) => sample.available_bytes)) <= SETTLE.memoryDriftLimit) {
      return {stable: true, samples};
    }
    await sleep(1000);
  }
  return {stable: false, samples};
}
