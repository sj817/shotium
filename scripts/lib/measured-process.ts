// Run a process and sample its process tree while it runs: peak processes,
// threads and working set, for the render regression report.
//
// Node cannot read another process's memory counters or enumerate its
// children, so the sampling is done by the platform. On Windows a PowerShell
// 7 script starts the process and polls a Toolhelp32 snapshot -- the same
// C# helper the PowerShell harness carried, compiled with Add-Type, because
// Get-CimInstance takes longer than a whole shotium render and would return
// no samples at all -- and hands back one JSON object. Elsewhere `ps` is
// polled the same way. The interval asked for and the interval actually
// achieved are both reported.

import {execa} from 'execa';
import which from 'which';

import {sleep} from './repo.ts';

export interface Measurement {
  exit_code: number;
  timed_out: boolean;
  wall_time_ms: number;
  requested_sample_interval_ms: number;
  sample_count: number;
  observed_mean_sample_period_ms: number | null;
  peak_processes: number;
  peak_threads: number;
  peak_rss_bytes: number;
  processes_seen: number;
  process_names: string[];
  stdout: string;
  stderr: string;
}

export interface MeasureOptions {
  cwd: string;
  samplingIntervalMs?: number;
  timeoutMs?: number;
}

// PowerShell 7 (.NET Core) has ProcessStartInfo.ArgumentList, which takes
// each argument verbatim; Windows PowerShell 5.1 (.NET Framework) does not.
// The harness has always run under pwsh, so that is what is looked for.
export function powershell(): string {
  return which.sync('pwsh', {nothrow: true}) ?? 'powershell';
}

const ps = (value: string) => `'${value.replace(/'/g, "''")}'`;

// The Toolhelp32 snapshot and the process-tree tracker, as the PowerShell
// harness defined them.
const SNAPSHOT_CS = String.raw`
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace ShotHarness {
  public sealed class NativeProcessSample {
    public int Id { get; set; }
    public int ParentId { get; set; }
    public int ThreadCount { get; set; }
    public string Name { get; set; }
  }

  public static class NativeProcessSnapshot {
    private const uint TH32CS_SNAPPROCESS = 0x00000002;
    private const uint PROCESS_QUERY_INFORMATION = 0x0400;
    private const uint PROCESS_VM_READ = 0x0010;
    private static readonly IntPtr InvalidHandleValue = new IntPtr(-1);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct PROCESSENTRY32 {
      public uint dwSize;
      public uint cntUsage;
      public uint th32ProcessID;
      public IntPtr th32DefaultHeapID;
      public uint th32ModuleID;
      public uint cntThreads;
      public uint th32ParentProcessID;
      public int pcPriClassBase;
      public uint dwFlags;
      [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
      public string szExeFile;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct PROCESS_MEMORY_COUNTERS {
      public uint cb;
      public uint PageFaultCount;
      public UIntPtr PeakWorkingSetSize;
      public UIntPtr WorkingSetSize;
      public UIntPtr QuotaPeakPagedPoolUsage;
      public UIntPtr QuotaPagedPoolUsage;
      public UIntPtr QuotaPeakNonPagedPoolUsage;
      public UIntPtr QuotaNonPagedPoolUsage;
      public UIntPtr PagefileUsage;
      public UIntPtr PeakPagefileUsage;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint processId);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool Process32FirstW(IntPtr snapshot, ref PROCESSENTRY32 entry);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool Process32NextW(IntPtr snapshot, ref PROCESSENTRY32 entry);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr OpenProcess(uint access, bool inheritHandle, uint processId);
    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(IntPtr handle);
    [DllImport("psapi.dll", SetLastError = true)]
    private static extern bool GetProcessMemoryInfo(IntPtr process, out PROCESS_MEMORY_COUNTERS counters, uint size);

    public static NativeProcessSample[] Capture() {
      var result = new List<NativeProcessSample>();
      IntPtr snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
      if (snapshot == InvalidHandleValue)
        throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
      try {
        var entry = new PROCESSENTRY32();
        entry.dwSize = (uint)Marshal.SizeOf(typeof(PROCESSENTRY32));
        if (!Process32FirstW(snapshot, ref entry))
          return result.ToArray();
        do {
          result.Add(new NativeProcessSample {
            Id = unchecked((int)entry.th32ProcessID),
            ParentId = unchecked((int)entry.th32ParentProcessID),
            ThreadCount = unchecked((int)entry.cntThreads),
            Name = entry.szExeFile ?? String.Empty
          });
          entry.dwSize = (uint)Marshal.SizeOf(typeof(PROCESSENTRY32));
        } while (Process32NextW(snapshot, ref entry));
      } finally {
        CloseHandle(snapshot);
      }
      return result.ToArray();
    }

    public static long GetWorkingSetBytes(int processId) {
      IntPtr process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, unchecked((uint)processId));
      if (process == IntPtr.Zero)
        return 0;
      try {
        var counters = new PROCESS_MEMORY_COUNTERS();
        counters.cb = (uint)Marshal.SizeOf(typeof(PROCESS_MEMORY_COUNTERS));
        return GetProcessMemoryInfo(process, out counters, counters.cb)
            ? unchecked((long)counters.WorkingSetSize.ToUInt64())
            : 0;
      } finally {
        CloseHandle(process);
      }
    }
  }

  public sealed class NativeProcessTreeSample {
    public int ProcessCount { get; set; }
    public int ThreadCount { get; set; }
    public long WorkingSetBytes { get; set; }
    public string[] Names { get; set; }
  }

  public sealed class NativeProcessTreeTracker {
    private readonly int rootId;
    private readonly HashSet<int> knownIds = new HashSet<int>();

    public NativeProcessTreeTracker(int rootId) {
      this.rootId = rootId;
      knownIds.Add(rootId);
    }

    public int KnownCount { get { return knownIds.Count; } }

    public NativeProcessTreeSample Capture() {
      NativeProcessSample[] snapshot = NativeProcessSnapshot.Capture();
      var currentIds = new HashSet<int>();
      currentIds.Add(rootId);
      bool changed;
      do {
        changed = false;
        foreach (NativeProcessSample entry in snapshot) {
          if ((currentIds.Contains(entry.ParentId) || knownIds.Contains(entry.ParentId)) && currentIds.Add(entry.Id)) {
            changed = true;
          }
        }
      } while (changed);
      int processCount = 0;
      int threadCount = 0;
      long workingSetBytes = 0;
      var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
      foreach (NativeProcessSample entry in snapshot) {
        if (!currentIds.Contains(entry.Id) && !knownIds.Contains(entry.Id))
          continue;
        knownIds.Add(entry.Id);
        ++processCount;
        threadCount += entry.ThreadCount;
        workingSetBytes += NativeProcessSnapshot.GetWorkingSetBytes(entry.Id);
        names.Add(entry.Name);
      }
      var sortedNames = new List<string>(names);
      sortedNames.Sort(StringComparer.OrdinalIgnoreCase);
      return new NativeProcessTreeSample {
        ProcessCount = processCount,
        ThreadCount = threadCount,
        WorkingSetBytes = workingSetBytes,
        Names = sortedNames.ToArray()
      };
    }
  }
}
`;

async function measureWindows(exe: string, args: string[], o: Required<MeasureOptions>): Promise<Measurement> {
  const script = [
    '$ErrorActionPreference = "Stop"',
    `if (-not ('ShotHarness.NativeProcessSnapshot' -as [type])) { Add-Type -TypeDefinition @'\n${SNAPSHOT_CS}\n'@ }`,
    '$psi = New-Object System.Diagnostics.ProcessStartInfo',
    `$psi.FileName = ${ps(exe)}`,
    `$psi.WorkingDirectory = ${ps(o.cwd)}`,
    ...args.map((a) => `[void]$psi.ArgumentList.Add(${ps(a)})`),
    '$psi.UseShellExecute = $false; $psi.CreateNoWindow = $true',
    '$psi.RedirectStandardOutput = $true; $psi.RedirectStandardError = $true',
    '$p = [System.Diagnostics.Process]::new(); $p.StartInfo = $psi',
    '$clock = [System.Diagnostics.Stopwatch]::StartNew()',
    `if (-not $p.Start()) { throw "Failed to start ${exe.replace(/"/g, '`"')}" }`,
    '$outTask = $p.StandardOutput.ReadToEndAsync(); $errTask = $p.StandardError.ReadToEndAsync()',
    '$tracker = [ShotHarness.NativeProcessTreeTracker]::new($p.Id)',
    '$names = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)',
    '$peakProcesses = 1; $peakThreads = 0; $peakRss = [long]0; $samples = 0; $timedOut = $false',
    'while (-not $p.HasExited) {',
    '  $sample = $tracker.Capture()',
    '  $peakProcesses = [Math]::Max($peakProcesses, $sample.ProcessCount)',
    '  $peakThreads = [Math]::Max($peakThreads, $sample.ThreadCount)',
    '  $peakRss = [Math]::Max($peakRss, $sample.WorkingSetBytes)',
    '  foreach ($n in $sample.Names) { [void]$names.Add($n) }',
    '  $samples++',
    `  if (${o.timeoutMs} -gt 0 -and $clock.ElapsedMilliseconds -ge ${o.timeoutMs}) { $timedOut = $true; $p.Kill($true); break }`,
    `  Start-Sleep -Milliseconds ${o.samplingIntervalMs}`,
    '}',
    '$p.WaitForExit(); $clock.Stop()',
    '$sorted = [System.Collections.Generic.List[string]]::new($names); $sorted.Sort([System.StringComparer]::OrdinalIgnoreCase)',
    '@{ exit_code = $p.ExitCode; timed_out = $timedOut; wall_time_ms = [Math]::Round($clock.Elapsed.TotalMilliseconds, 3); sample_count = $samples; peak_processes = $peakProcesses; peak_threads = $peakThreads; peak_rss_bytes = $peakRss; processes_seen = $tracker.KnownCount; process_names = @($sorted); stdout = $outTask.Result; stderr = $errTask.Result } | ConvertTo-Json -Compress -Depth 4',
  ].join('\n');
  const result = await execa(powershell(), ['-NoProfile', '-Command', script], {reject: false, maxBuffer: 1 << 26});
  if (result.exitCode !== 0 || !result.stdout.trim()) throw new Error(`Failed to start ${exe}: ${result.stderr}`);
  const raw = JSON.parse(result.stdout.trim()) as Omit<Measurement, 'requested_sample_interval_ms' | 'observed_mean_sample_period_ms'>;
  return finish(raw, o.samplingIntervalMs);
}

async function measurePosix(exe: string, args: string[], o: Required<MeasureOptions>): Promise<Measurement> {
  const started = process.hrtime.bigint();
  const child = execa(exe, args, {cwd: o.cwd, reject: false, all: false});
  const known = new Set<number>([child.pid!]);
  const names = new Set<string>();
  let peakProcesses = 1, peakThreads = 0, peakRss = 0, samples = 0, timedOut = false;
  let exited = false;
  child.then(() => { exited = true; }, () => { exited = true; });
  while (!exited) {
    const table = await execa('ps', ['-eo', 'pid=,ppid=,nlwp=,rss=,comm='], {reject: false});
    const rows = table.stdout.split('\n').map((l) => l.trim().split(/\s+/)).filter((r) => r.length >= 5)
                     .map((r) => ({pid: Number(r[0]), ppid: Number(r[1]), threads: Number(r[2]), rss: Number(r[3]) * 1024, name: r.slice(4).join(' ')}));
    const current = new Set<number>([child.pid!]);
    let changed: boolean;
    do {
      changed = false;
      for (const r of rows) {
        if ((current.has(r.ppid) || known.has(r.ppid)) && !current.has(r.pid)) {
          current.add(r.pid);
          changed = true;
        }
      }
    } while (changed);
    let count = 0, threads = 0, rss = 0;
    for (const r of rows) {
      if (!current.has(r.pid) && !known.has(r.pid)) continue;
      known.add(r.pid);
      count++;
      threads += r.threads;
      rss += r.rss;
      names.add(r.name);
    }
    peakProcesses = Math.max(peakProcesses, count);
    peakThreads = Math.max(peakThreads, threads);
    peakRss = Math.max(peakRss, rss);
    samples++;
    const elapsed = Number(process.hrtime.bigint() - started) / 1e6;
    if (o.timeoutMs > 0 && elapsed >= o.timeoutMs) {
      timedOut = true;
      child.kill('SIGKILL');
      break;
    }
    await sleep(o.samplingIntervalMs);
  }
  const result = await child;
  const wall = Number(process.hrtime.bigint() - started) / 1e6;
  return finish({
    exit_code: result.exitCode ?? -1, timed_out: timedOut, wall_time_ms: Math.round(wall * 1000) / 1000, sample_count: samples,
    peak_processes: peakProcesses, peak_threads: peakThreads, peak_rss_bytes: peakRss, processes_seen: known.size,
    process_names: [...names].sort((a, b) => a.localeCompare(b)), stdout: result.stdout ?? '', stderr: result.stderr ?? '',
  }, o.samplingIntervalMs);
}

function finish(raw: Omit<Measurement, 'requested_sample_interval_ms' | 'observed_mean_sample_period_ms'>, interval: number): Measurement {
  return {
    ...raw,
    stdout: (raw.stdout ?? '').trim(),
    stderr: (raw.stderr ?? '').trim(),
    process_names: raw.process_names ?? [],
    requested_sample_interval_ms: interval,
    observed_mean_sample_period_ms: raw.sample_count ? Math.round((raw.wall_time_ms / raw.sample_count) * 1000) / 1000 : null,
  };
}

export function measure(exe: string, args: string[], options: MeasureOptions): Promise<Measurement> {
  const o = {samplingIntervalMs: 10, timeoutMs: 0, ...options};
  return process.platform === 'win32' ? measureWindows(exe, args, o) : measurePosix(exe, args, o);
}
