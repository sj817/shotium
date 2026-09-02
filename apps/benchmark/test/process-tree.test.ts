import assert from 'node:assert/strict';
import test from 'node:test';
import {competitorChromiumPolicy} from '../src/engines.ts';
import {
  collectAncestorPids,
  cpuLimitFromBaseline,
  parseLinuxProcStat,
  processIdentityForPid,
  partitionProtectedProcesses,
  ProcessMonitor,
  selectOwnedProcessEntries,
  waitForProcessTree,
} from '../src/process-tree.ts';

test('calibrates the host CPU gate from the idle baseline', () => {
  // Quiet Linux runner: the fixed floor still applies.
  assert.equal(cpuLimitFromBaseline(5), 25);
  assert.equal(cpuLimitFromBaseline(14.9), 25);
  // GitHub Windows/macOS runners idle at 28-41%; the gate follows them instead
  // of rejecting every cell.
  assert.equal(cpuLimitFromBaseline(38.4), 48);
  assert.equal(cpuLimitFromBaseline(41), 51);
  // A saturated host follows its own floor. Capping the gate below the floor is
  // how every macOS cell was rejected for load the runner produced at rest: the
  // baseline read 85-91% and the gate sat at 80%.
  assert.equal(cpuLimitFromBaseline(90), 100);
  assert.equal(cpuLimitFromBaseline(85.09), 95);
  assert.equal(cpuLimitFromBaseline(Number.NaN), 25);
  assert.equal(cpuLimitFromBaseline(30, {floor: 10, margin: 5}), 35);
});

test('waits for an exact owned process root to become observable', async () => {
  let calls = 0;
  const observed = {
    at: new Date().toISOString(),
    processes: [{
      pid: 41, parent_pid: 1, started: '', birth_token: 'test:41', name: 'owned',
      command: 'owned', rss_bytes: 1, cpu_percent: 0,
    }],
    rss_bytes: 1,
    cpu_percent: 0,
  };
  const result = await waitForProcessTree([41], {
    timeoutMs: 100,
    intervalMs: 0,
    snapshot: async (rootPids) => {
      assert.deepEqual(rootPids, [41]);
      calls += 1;
      return calls === 1 ? {...observed, processes: []} : observed;
    },
  });
  assert.equal(calls, 2);
  assert.equal(result.processes[0].pid, 41);
});

test('never substitutes an unrelated process when the owned root stays absent', async () => {
  await assert.rejects(() => waitForProcessTree([73], {
    timeoutMs: 0,
    snapshot: async () => ({
      at: new Date().toISOString(), processes: [], rss_bytes: 0, cpu_percent: 0,
    }),
  }), /owned process tree for PID\(s\) 73 was not observable/);
});

test('required-root monitoring waits for a short-lived client identity before release', async () => {
  let calls = 0;
  const root = {
    pid: 91, parent_pid: 1, started: '2026-08-30 08:00:00',
    birth_token: 'windows-creation-date:2026-08-30 08:00:00',
    name: 'node', command: 'client-once', rss_bytes: 4096, cpu_percent: 0,
  };
  const snapshot = async (rootPids) => {
    assert.deepEqual(rootPids, [91]);
    calls += 1;
    const processes = calls < 3 ? [] : [root];
    return {
      at: new Date().toISOString(),
      processes,
      rss_bytes: processes.length ? root.rss_bytes : 0,
      cpu_percent: 0,
    };
  };
  const monitor = new ProcessMonitor([91], 0, snapshot);
  await monitor.start({requireRoots: true, timeoutMs: 100});
  const telemetry = await monitor.stop();
  assert.ok(calls >= 3);
  assert.equal(telemetry.trusted, true);
  assert.deepEqual(telemetry.identities, [root]);
  assert.deepEqual(telemetry.errors, []);
});

test('Linux competitor policy disables the unavailable browser sandbox only there', () => {
  assert.deepEqual(competitorChromiumPolicy('linux'), {
    puppeteerArgs: ['--no-sandbox'],
    playwrightChromiumSandbox: false,
  });
  assert.deepEqual(competitorChromiumPolicy('win32'), {
    puppeteerArgs: [],
    playwrightChromiumSandbox: undefined,
  });
});

test('parses a stable Linux identity when the process name contains spaces and parentheses', () => {
  const fields = Array(22).fill('0');
  fields[0] = 'S';
  fields[1] = '123';
  fields[19] = '987654';
  const stat = `4321 (node (benchmark) worker) ${fields.join(' ')}`;
  assert.deepEqual(parseLinuxProcStat(stat), {
    pid: 4321,
    parent_pid: 123,
    started: null,
    birth_token: 'linux-proc-startticks:987654',
    name: 'node (benchmark) worker',
  });
});

test('does not follow a reused Windows parent PID into older system processes', () => {
  const list = [
    {pid: 1744, parentPid: 11116, started: '2026-08-29 13:58:32'},
    {pid: 9036, parentPid: 1744, started: '2026-08-29 13:59:08'},
    {pid: 740, parentPid: 9036, started: '2026-08-29 13:59:08'},
    {pid: 752, parentPid: 740, started: '2026-08-29 13:38:19'},
    {pid: 848, parentPid: 752, started: '2026-08-29 13:38:20'},
  ];
  assert.deepEqual(
      selectOwnedProcessEntries(list, [1744]).map((entry) => entry.pid),
      [1744, 9036, 740]);
});

test('forced cleanup always separates the current process ancestry', () => {
  const identities = [{pid: 10}, {pid: 20}, {pid: 30}, {pid: 40}];
  const processList = [
    {pid: 10, parentPid: 20},
    {pid: 20, parentPid: 40},
    {pid: 30, parentPid: 10},
    {pid: 40, parentPid: 0},
  ];
  const protectedPids = collectAncestorPids(processList, 10, 20);
  assert.deepEqual([...protectedPids], [10, 20, 40]);
  const result = partitionProtectedProcesses(identities, protectedPids);
  assert.deepEqual(result.killable, [{pid: 30}]);
  assert.deepEqual(result.protected, [{pid: 10}, {pid: 20}, {pid: 40}]);
});

test('a PID that exits mid-sample is gone, not an infrastructure failure', async (t) => {
  // A soak at concurrency 4 starts and reaps hundreds of browser subprocesses,
  // so losing the race against exit is the normal case. Linux answers ENOENT
  // once /proc is gone and ESRCH while the task is still being torn down; a
  // read that wins the race can also come back empty. Only the first of those
  // three was treated as gone, and the other two raised an infrastructure error
  // that marked the cell untrusted and failed the whole shard.
  if (process.platform !== 'linux') return t.skip('Linux /proc identity path');
  const {default: fs} = await import('node:fs');
  const original = fs.readFileSync;
  t.after(() => { (fs as any).readFileSync = original; });
  for (const outcome of ['ESRCH', 'ENOENT', 'empty']) {
    (fs as any).readFileSync = (file: any, ...rest: any[]) => {
      if (!String(file).startsWith('/proc/')) return (original as any)(file, ...rest);
      if (outcome === 'empty') return '';
      const error: any = new Error(outcome);
      error.code = outcome;
      throw error;
    };
    assert.equal(await processIdentityForPid(999999), null, `${outcome} should read as gone`);
  }
});
