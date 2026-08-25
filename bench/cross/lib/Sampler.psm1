Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# A process-tree sampler that keeps a timeline and attributes bytes to image
# names.
#
# tests/render/lib/ShotHarness.psm1 already has a tracker, and this is not it
# for two reasons. It answers "how much of that was node and how much was the
# engine", which needs the working set per image name rather than one total --
# and every engine here is driven from node, so an unattributed total would
# charge shotium for a Node heap it shares with its competitors. And it keeps
# every sample with its timestamp, so that the runner's marks can cut the
# timeline into launch, warmup and steady state afterwards.
#
# Two numbers per process, because one is not enough to say what a tree costs.
#
# Working set is what task manager shows and what an operator watches, but it
# counts a shared page once per process that maps it, and every engine here is
# several processes mapping one large binary. Four shot workers map 43 MB of
# shotium.exe between them and the sum charges for it four times; twenty-one
# chrome processes map chrome.dll and the sum charges for it twenty-one times.
# That is not a small correction -- measured on this tree, the sum of working
# sets came to about 3.8x the memory four workers actually held.
#
# Private working set is the other half: the physical pages a process shares
# with nobody, which is what goes away when it exits. Summing that across a
# tree is exact, and what the two sums differ by is the shared part, charged
# once per mapping instead of once.
#
# Both are reported. Neither alone is "the" number: private working set
# understates a tree by leaving out the binary it has to have resident, and
# working set overstates it by however many processes are mapping it.

if (-not ('ShotBench.TreeSampler' -as [type])) {
  Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace ShotBench {
  public sealed class TreeSample {
    public long At { get; set; }
    public int ProcessCount { get; set; }
    public int ThreadCount { get; set; }
    public long WorkingSetBytes { get; set; }
    public long PrivateWorkingSetBytes { get; set; }
    public Dictionary<string, long> ByName { get; set; }
    public Dictionary<string, long> PrivateByName { get; set; }
  }

  public sealed class TreeSampler {
    private const uint TH32CS_SNAPPROCESS = 0x00000002;
    private const uint PROCESS_QUERY_INFORMATION = 0x0400;
    // Enough for GetProcessTimes, and granted where the fuller right is not.
    private const uint PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;
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

    // PROCESS_MEMORY_COUNTERS_EX2, which is PROCESS_MEMORY_COUNTERS plus the
    // two fields this needs and cannot compute: PrivateUsage is the commit
    // charge, and PrivateWorkingSetSize is the resident part of it. Windows 10
    // 1809 and later fill it in; the call asks for the extended size and falls
    // back to the base one, so an older host still gets a working set.
    [StructLayout(LayoutKind.Sequential)]
    private struct PROCESS_MEMORY_COUNTERS_EX2 {
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
      public UIntPtr PrivateUsage;
      public UIntPtr PrivateWorkingSetSize;
      public ulong SharedCommitUsage;
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
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetProcessTimes(
        IntPtr process, out long creation, out long exit, out long kernel, out long user);
    [DllImport("psapi.dll", SetLastError = true)]
    private static extern bool GetProcessMemoryInfo(
        IntPtr process, out PROCESS_MEMORY_COUNTERS_EX2 counters, uint size);

    // Every process that has ever been in the tree, and when it started.
    //
    // The birthday is not bookkeeping, it is the whole correctness argument.
    // Windows reuses process ids, and a sampler that remembers a dead id --
    // which this one must, so that a tree does not shrink out of view when a
    // browser respawns a child -- will otherwise adopt whatever unrelated
    // process inherits that id next, and every descendant it has. On a busy
    // machine that is not a rare event: it produced a cold-start cell of
    // 3.6 GB across 24 processes for an engine that runs six.
    //
    // So a process is only itself if its id and its start time both match, and
    // a child is only a child if its parent started before it did.
    private readonly Dictionary<int, long> born = new Dictionary<int, long>();

    public TreeSampler(int[] rootIds) {
      foreach (int id in rootIds) {
        long birth = CreationTime(id);
        if (birth != 0)
          born[id] = birth;
      }
    }

    public int KnownCount { get { return born.Count; } }

    // The process's creation time, or 0 if it cannot be read -- which is what
    // an exited process looks like, and is treated as "not this one".
    private static long CreationTime(int processId) {
      IntPtr process = OpenProcess(
          PROCESS_QUERY_LIMITED_INFORMATION, false, unchecked((uint)processId));
      if (process == IntPtr.Zero)
        return 0;
      try {
        long creation, exit, kernel, user;
        return GetProcessTimes(process, out creation, out exit, out kernel, out user)
            ? creation : 0;
      } finally {
        CloseHandle(process);
      }
    }

    // Working set and private working set for one process, in that order.
    private static long[] MemoryBytes(int processId) {
      IntPtr process = OpenProcess(
          PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, unchecked((uint)processId));
      if (process == IntPtr.Zero)
        return new long[] { 0, 0 };
      try {
        PROCESS_MEMORY_COUNTERS_EX2 counters;
        uint extended = (uint)Marshal.SizeOf(typeof(PROCESS_MEMORY_COUNTERS_EX2));
        if (GetProcessMemoryInfo(process, out counters, extended)) {
          return new long[] {
            unchecked((long)counters.WorkingSetSize.ToUInt64()),
            unchecked((long)counters.PrivateWorkingSetSize.ToUInt64())
          };
        }
        // No PrivateWorkingSetSize on this host. Reporting a zero for it is
        // the right answer: it is missing rather than nothing, and a report
        // that says zero is easier to catch than one that quietly substitutes
        // a number meaning something else.
        uint basic = (uint)(8 + 8 * IntPtr.Size);
        return GetProcessMemoryInfo(process, out counters, basic)
            ? new long[] { unchecked((long)counters.WorkingSetSize.ToUInt64()), 0 }
            : new long[] { 0, 0 };
      } finally {
        CloseHandle(process);
      }
    }

    // One snapshot of every process descended from a root, plus every process
    // that ever was -- a browser that has already exited still counted while it
    // ran, and forgetting it would let a tree shrink out of view. See `born`
    // for why remembering an id is not the same as remembering a process.
    public TreeSample Capture() {
      var entries = new List<PROCESSENTRY32>();
      IntPtr snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
      if (snapshot == InvalidHandleValue)
        throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
      try {
        var entry = new PROCESSENTRY32();
        entry.dwSize = (uint)Marshal.SizeOf(typeof(PROCESSENTRY32));
        if (Process32FirstW(snapshot, ref entry)) {
          do {
            entries.Add(entry);
            entry.dwSize = (uint)Marshal.SizeOf(typeof(PROCESSENTRY32));
          } while (Process32NextW(snapshot, ref entry));
        }
      } finally {
        CloseHandle(snapshot);
      }

      // Start times for everything in this snapshot, read once.
      var birth = new Dictionary<int, long>();
      foreach (PROCESSENTRY32 e in entries) {
        int id = unchecked((int)e.th32ProcessID);
        if (!birth.ContainsKey(id))
          birth[id] = CreationTime(id);
      }

      // The tree is the remembered members that are still the same process,
      // grown by children whose parent is in it and started before them.
      var current = new HashSet<int>();
      foreach (KeyValuePair<int, long> remembered in born) {
        long now;
        if (birth.TryGetValue(remembered.Key, out now) && now == remembered.Value)
          current.Add(remembered.Key);
      }
      bool changed;
      do {
        changed = false;
        foreach (PROCESSENTRY32 e in entries) {
          int id = unchecked((int)e.th32ProcessID);
          int parent = unchecked((int)e.th32ParentProcessID);
          if (!current.Contains(parent) || current.Contains(id))
            continue;
          long childBirth, parentBirth;
          if (!birth.TryGetValue(id, out childBirth) ||
              !birth.TryGetValue(parent, out parentBirth))
            continue;
          // A parent that started after its "child" is an id that was handed
          // on, not a parent.
          if (childBirth != 0 && parentBirth != 0 && childBirth < parentBirth)
            continue;
          current.Add(id);
          changed = true;
        }
      } while (changed);

      var sample = new TreeSample {
        At = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
        ByName = new Dictionary<string, long>(StringComparer.OrdinalIgnoreCase),
        PrivateByName = new Dictionary<string, long>(StringComparer.OrdinalIgnoreCase)
      };
      foreach (PROCESSENTRY32 e in entries) {
        int id = unchecked((int)e.th32ProcessID);
        if (!current.Contains(id))
          continue;
        if (!born.ContainsKey(id))
          born[id] = birth[id];
        long[] bytes = MemoryBytes(id);
        sample.ProcessCount++;
        sample.ThreadCount += unchecked((int)e.cntThreads);
        sample.WorkingSetBytes += bytes[0];
        sample.PrivateWorkingSetBytes += bytes[1];
        string name = e.szExeFile ?? "";
        long running;
        sample.ByName[name] = sample.ByName.TryGetValue(name, out running)
            ? running + bytes[0] : bytes[0];
        sample.PrivateByName[name] = sample.PrivateByName.TryGetValue(name, out running)
            ? running + bytes[1] : bytes[1];
      }
      return sample;
    }
  }
}
'@
}

# Samples one process tree until the process exits, and returns the timeline
# with it. The requested interval is a sleep between snapshots; taking one is
# not free, so the observed period is reported rather than assumed.
function Invoke-SampledProcess {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string[]]$Arguments,
    [Parameter(Mandatory)][string]$WorkingDirectory,
    [ValidateRange(1, 1000)][int]$IntervalMs = 20,
    [ValidateRange(1000, 3600000)][int]$TimeoutMs = 120000
  )

  $info = [Diagnostics.ProcessStartInfo]::new()
  $info.FileName = $Executable
  $info.WorkingDirectory = (Resolve-Path -LiteralPath $WorkingDirectory).Path
  $info.UseShellExecute = $false
  $info.CreateNoWindow = $true
  $info.RedirectStandardOutput = $true
  $info.RedirectStandardError = $true
  foreach ($argument in $Arguments) { [void]$info.ArgumentList.Add($argument) }

  $process = [Diagnostics.Process]::new()
  $process.StartInfo = $info
  $startedUtc = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  $clock = [Diagnostics.Stopwatch]::StartNew()
  if (-not $process.Start()) { throw "could not start $Executable" }
  $stdout = $process.StandardOutput.ReadToEndAsync()
  $stderr = $process.StandardError.ReadToEndAsync()

  $sampler = [ShotBench.TreeSampler]::new(@($process.Id))
  $timeline = [Collections.Generic.List[object]]::new()
  $timedOut = $false
  while (-not $process.HasExited) {
    $timeline.Add($sampler.Capture())
    if ($clock.ElapsedMilliseconds -ge $TimeoutMs) {
      $timedOut = $true
      $process.Kill($true)
      break
    }
    Start-Sleep -Milliseconds $IntervalMs
  }
  # One last sample after exit is deliberately not taken: the tree is gone, and
  # a zero would drag the tail of the timeline down.
  $process.WaitForExit()
  $clock.Stop()

  [pscustomobject]@{
    exit_code = $process.ExitCode
    timed_out = $timedOut
    started_utc_ms = $startedUtc
    wall_time_ms = [Math]::Round($clock.Elapsed.TotalMilliseconds, 3)
    requested_interval_ms = $IntervalMs
    observed_mean_period_ms = if ($timeline.Count) {
      [Math]::Round($clock.Elapsed.TotalMilliseconds / $timeline.Count, 3)
    } else { $null }
    timeline = $timeline
    stdout = $stdout.GetAwaiter().GetResult()
    stderr = $stderr.GetAwaiter().GetResult()
  }
}

# The same sampler pointed at something already running, for a fixed stretch of
# time. This is how a resident engine's idle footprint is measured: nothing is
# being asked of it, and the question is what it costs to have it there.
function Measure-ResidentTree {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory)][int[]]$RootIds,
    [ValidateRange(100, 60000)][int]$DurationMs = 2000,
    [ValidateRange(1, 1000)][int]$IntervalMs = 50
  )
  $sampler = [ShotBench.TreeSampler]::new($RootIds)
  $timeline = [Collections.Generic.List[object]]::new()
  $clock = [Diagnostics.Stopwatch]::StartNew()
  while ($clock.ElapsedMilliseconds -lt $DurationMs) {
    $timeline.Add($sampler.Capture())
    Start-Sleep -Milliseconds $IntervalMs
  }
  $timeline
}

Export-ModuleMember -Function Invoke-SampledProcess, Measure-ResidentTree
