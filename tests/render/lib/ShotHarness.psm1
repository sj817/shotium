Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not ('ShotHarness.NativeProcessSnapshot' -as [type])) {
  Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace ShotHarness {
  public sealed class NativeProcessSample {
    public int Id { get; set; }
    public int ParentId { get; set; }
    public int ThreadCount { get; set; }
    public long WorkingSetBytes { get; set; }
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
    private static extern bool GetProcessMemoryInfo(
        IntPtr process, out PROCESS_MEMORY_COUNTERS counters, uint size);

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
            WorkingSetBytes = 0,
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
      IntPtr process = OpenProcess(
          PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false,
          unchecked((uint)processId));
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

  public sealed class NativePixelDiffResult {
    public long ChangedPixels { get; set; }
    public long AbsoluteDelta { get; set; }
    public double SquaredDelta { get; set; }
    public int MaxDelta { get; set; }
    public byte[] DiffBytes { get; set; }
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
          if ((currentIds.Contains(entry.ParentId) ||
               knownIds.Contains(entry.ParentId)) &&
              currentIds.Add(entry.Id)) {
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

  public static class NativePixelDiff {
    public static NativePixelDiffResult Compare(
        byte[] expected, int expectedStride, byte[] actual, int actualStride,
        int width, int height, bool createDiff) {
      var result = new NativePixelDiffResult();
      if (createDiff)
        result.DiffBytes = new byte[expectedStride * height];

      for (int y = 0; y < height; ++y) {
        int expectedRow = y * expectedStride;
        int actualRow = y * actualStride;
        for (int x = 0; x < width; ++x) {
          int expectedOffset = expectedRow + x * 4;
          int actualOffset = actualRow + x * 4;
          bool pixelChanged = false;
          int pixelMax = 0;
          for (int channel = 0; channel < 4; ++channel) {
            int delta = Math.Abs(
                expected[expectedOffset + channel] -
                actual[actualOffset + channel]);
            if (delta != 0)
              pixelChanged = true;
            pixelMax = Math.Max(pixelMax, delta);
            result.MaxDelta = Math.Max(result.MaxDelta, delta);
            result.AbsoluteDelta += delta;
            result.SquaredDelta += (double)delta * delta;
          }
          if (pixelChanged)
            ++result.ChangedPixels;
          if (result.DiffBytes != null) {
            result.DiffBytes[expectedOffset] = 0;
            result.DiffBytes[expectedOffset + 1] = 0;
            result.DiffBytes[expectedOffset + 2] =
                pixelChanged ? (byte)Math.Max(64, pixelMax) : (byte)0;
            result.DiffBytes[expectedOffset + 3] = 255;
          }
        }
      }
      return result;
    }
  }
}
'@
}

function Resolve-CaptureInput {
  [CmdletBinding()]
  param([Parameter(Mandatory)][string]$InputValue)

  $uri = $null
  if ([Uri]::TryCreate($InputValue, [UriKind]::Absolute, [ref]$uri) -and
      $uri.Scheme -in @('http', 'https', 'file', 'data')) {
    return $uri.AbsoluteUri
  }

  $path = (Resolve-Path -LiteralPath $InputValue).Path
  return ([Uri]$path).AbsoluteUri
}

function Get-EngineIdentity {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory)][ValidateSet('system-chrome', 'headless-shell', 'shot')]
    [string]$Engine,
    [Parameter(Mandatory)][string]$Executable
  )

  $file = Get-Item -LiteralPath $Executable
  $identity = switch ($Engine) {
    'system-chrome' { 'external-system-chrome' }
    'headless-shell' { 'source-build-headless-shell-baseline' }
    'shot' { 'source-build-shot' }
  }

  [pscustomobject]@{
    identity = $identity
    engine = $Engine
    executable = $file.FullName
    binary_bytes = [long]$file.Length
    binary_sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    file_version = $file.VersionInfo.FileVersion
  }
}

function Invoke-MeasuredProcess {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string[]]$Arguments,
    [Parameter(Mandatory)][string]$WorkingDirectory,
    [ValidateRange(5, 1000)][int]$SamplingIntervalMs = 10,
    [ValidateRange(0, 900000)][int]$ProcessTimeoutMs = 0
  )

  $startInfo = [Diagnostics.ProcessStartInfo]::new()
  $startInfo.FileName = (Get-Item -LiteralPath $Executable).FullName
  $startInfo.WorkingDirectory = (Resolve-Path -LiteralPath $WorkingDirectory).Path
  $startInfo.UseShellExecute = $false
  $startInfo.CreateNoWindow = $true
  $startInfo.RedirectStandardOutput = $true
  $startInfo.RedirectStandardError = $true
  foreach ($argument in $Arguments) {
    [void]$startInfo.ArgumentList.Add($argument)
  }

  $process = [Diagnostics.Process]::new()
  $process.StartInfo = $startInfo
  $clock = [Diagnostics.Stopwatch]::StartNew()
  if (-not $process.Start()) {
    throw "Failed to start $Executable"
  }
  $stdoutTask = $process.StandardOutput.ReadToEndAsync()
  $stderrTask = $process.StandardError.ReadToEndAsync()
  $tracker = [ShotHarness.NativeProcessTreeTracker]::new($process.Id)
  $allNames = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
  $peakProcesses = 1
  $peakThreads = 0
  $peakRss = 0L
  $samples = 0
  $timedOut = $false

  while (-not $process.HasExited) {
    $sample = $tracker.Capture()
    $peakProcesses = [Math]::Max($peakProcesses, $sample.ProcessCount)
    $peakThreads = [Math]::Max($peakThreads, $sample.ThreadCount)
    $peakRss = [Math]::Max($peakRss, $sample.WorkingSetBytes)
    foreach ($name in $sample.Names) { [void]$allNames.Add($name) }
    $samples++
    if ($ProcessTimeoutMs -gt 0 -and $clock.ElapsedMilliseconds -ge $ProcessTimeoutMs) {
      $timedOut = $true
      $process.Kill($true)
      break
    }
    Start-Sleep -Milliseconds $SamplingIntervalMs
  }

  $process.WaitForExit()
  $clock.Stop()
  $stdout = $stdoutTask.GetAwaiter().GetResult()
  $stderr = $stderrTask.GetAwaiter().GetResult()

  [pscustomobject]@{
    exit_code = $process.ExitCode
    timed_out = $timedOut
    wall_time_ms = [Math]::Round($clock.Elapsed.TotalMilliseconds, 3)
    requested_sample_interval_ms = $SamplingIntervalMs
    sample_count = $samples
    observed_mean_sample_period_ms = if ($samples) {
      [Math]::Round($clock.Elapsed.TotalMilliseconds / $samples, 3)
    } else {
      $null
    }
    peak_processes = $peakProcesses
    peak_threads = $peakThreads
    peak_rss_bytes = $peakRss
    processes_seen = $tracker.KnownCount
    process_names = @($allNames | Sort-Object)
    stdout = $stdout.Trim()
    stderr = $stderr.Trim()
  }
}

function Get-PngInfo {
  [CmdletBinding()]
  param([Parameter(Mandatory)][string]$Path)

  $file = Get-Item -LiteralPath $Path
  $bytes = [IO.File]::ReadAllBytes($file.FullName)
  if ($bytes.Length -lt 24 -or
      $bytes[0] -ne 137 -or $bytes[1] -ne 80 -or $bytes[2] -ne 78 -or
      $bytes[3] -ne 71 -or $bytes[4] -ne 13 -or $bytes[5] -ne 10 -or
      $bytes[6] -ne 26 -or $bytes[7] -ne 10 -or
      [Text.Encoding]::ASCII.GetString($bytes, 12, 4) -ne 'IHDR') {
    throw "Not a valid PNG with an IHDR chunk: $($file.FullName)"
  }

  $width = [uint32]($bytes[16] * 16777216 + $bytes[17] * 65536 + $bytes[18] * 256 + $bytes[19])
  $height = [uint32]($bytes[20] * 16777216 + $bytes[21] * 65536 + $bytes[22] * 256 + $bytes[23])
  [pscustomobject]@{
    path = $file.FullName
    width = $width
    height = $height
    bytes = [long]$file.Length
    sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
  }
}

function Invoke-ScreenshotCapture {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory)][ValidateSet('system-chrome', 'headless-shell', 'shot')]
    [string]$Engine,
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$InputValue,
    [Parameter(Mandatory)][string]$OutputPath,
    [ValidateRange(1, 32768)][int]$Width = 1280,
    [ValidateRange(1, 32768)][int]$Height = 720,
    [ValidateRange(0.1, 8.0)][double]$Scale = 1.0,
    [ValidateRange(1, 600000)][int]$TimeoutMs = 30000,
    [ValidateRange(5, 1000)][int]$SamplingIntervalMs = 10
  )

  if ($Engine -eq 'shot' -and [Math]::Abs($Scale - 1.0) -gt [double]::Epsilon) {
    throw 'Shot v1 only supports --scale 1. Non-1 DPR requires explicit screen configuration and is not benchmarked yet.'
  }
  $inputUri = Resolve-CaptureInput -InputValue $InputValue
  $outputFile = [IO.Path]::GetFullPath($OutputPath)
  [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($outputFile)) | Out-Null
  if (Test-Path -LiteralPath $outputFile) {
    Remove-Item -LiteralPath $outputFile -Force
  }

  $arguments = [Collections.Generic.List[string]]::new()
  $profileDirectory = $null
  if ($Engine -eq 'shot') {
    foreach ($argument in @(
        $inputUri, '--width', "$Width", '--height', "$Height", '--scale',
        $Scale.ToString([Globalization.CultureInfo]::InvariantCulture),
        '--timeout-ms', "$TimeoutMs", '--output', $outputFile)) {
      $arguments.Add($argument)
    }
  } else {
    if ($Engine -eq 'system-chrome') {
      $arguments.Add('--headless=new')
      $profileDirectory = Join-Path ([IO.Path]::GetTempPath()) ("shot-chrome-profile-" + [Guid]::NewGuid().ToString('N'))
      [IO.Directory]::CreateDirectory($profileDirectory) | Out-Null
      $arguments.Add("--user-data-dir=$profileDirectory")
    }
    foreach ($argument in @(
        '--disable-background-networking', '--disable-component-update',
        '--disable-default-apps', '--disable-extensions', '--disable-sync',
        '--no-first-run', '--no-default-browser-check',
        '--allow-file-access-from-files', '--hide-scrollbars',
        '--run-all-compositor-stages-before-draw',
        "--force-device-scale-factor=$($Scale.ToString([Globalization.CultureInfo]::InvariantCulture))",
        "--window-size=$Width,$Height", "--screenshot=$outputFile", $inputUri)) {
      $arguments.Add($argument)
    }
  }

  try {
    $hardTimeoutMs = [Math]::Min(900000, $TimeoutMs + 10000)
    $measurement = Invoke-MeasuredProcess -Executable $Executable -Arguments $arguments.ToArray() `
      -WorkingDirectory ([IO.Path]::GetDirectoryName((Get-Item -LiteralPath $Executable).FullName)) `
      -SamplingIntervalMs $SamplingIntervalMs -ProcessTimeoutMs $hardTimeoutMs
  } finally {
    if ($profileDirectory -and (Test-Path -LiteralPath $profileDirectory)) {
      $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
      $resolvedProfile = [IO.Path]::GetFullPath($profileDirectory)
      if (-not $resolvedProfile.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove profile outside the temp directory: $resolvedProfile"
      }
      Remove-Item -LiteralPath $resolvedProfile -Recurse -Force
    }
  }

  if ($measurement.timed_out) {
    throw "$Engine exceeded the $hardTimeoutMs ms hard process timeout."
  }
  if ($measurement.exit_code -ne 0) {
    throw "$Engine exited with code $($measurement.exit_code): $($measurement.stderr)"
  }
  if (-not (Test-Path -LiteralPath $outputFile)) {
    throw "$Engine exited successfully but did not produce $outputFile"
  }

  [pscustomobject]@{
    engine = $Engine
    input = $inputUri
    output = $outputFile
    viewport_width = $Width
    viewport_height = $Height
    scale = $Scale
    png = Get-PngInfo -Path $outputFile
    measurement = $measurement
  }
}

function Copy-BitmapBytes {
  param([Parameter(Mandatory)][Drawing.Bitmap]$Bitmap)

  $rectangle = [Drawing.Rectangle]::new(0, 0, $Bitmap.Width, $Bitmap.Height)
  $data = $Bitmap.LockBits(
      $rectangle, [Drawing.Imaging.ImageLockMode]::ReadOnly,
      [Drawing.Imaging.PixelFormat]::Format32bppArgb)
  try {
    $stride = [Math]::Abs($data.Stride)
    $length = $stride * $data.Height
    $bytes = [byte[]]::new($length)
    for ($row = 0; $row -lt $data.Height; $row++) {
      $source = [IntPtr]::Add($data.Scan0, $row * $data.Stride)
      [Runtime.InteropServices.Marshal]::Copy(
          $source, [byte[]]$bytes, $row * $stride, $stride)
    }
    [pscustomobject]@{ bytes = $bytes; stride = $stride }
  } finally {
    $Bitmap.UnlockBits($data)
  }
}

function Compare-Png {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory)][string]$ExpectedPath,
    [Parameter(Mandatory)][string]$ActualPath,
    [string]$DiffPath
  )

  Add-Type -AssemblyName System.Drawing
  $expectedInfo = Get-PngInfo -Path $ExpectedPath
  $actualInfo = Get-PngInfo -Path $ActualPath
  if ($expectedInfo.width -ne $actualInfo.width -or $expectedInfo.height -ne $actualInfo.height) {
    return [pscustomobject]@{
      dimensions_match = $false
      expected = $expectedInfo
      actual = $actualInfo
      exact_sha256_match = $false
      changed_pixels = $null
      changed_fraction = $null
      max_channel_delta = $null
      mean_absolute_channel_delta = $null
      root_mean_square_channel_delta = $null
      diff_path = $null
    }
  }

  if ($expectedInfo.sha256 -eq $actualInfo.sha256) {
    return [pscustomobject]@{
      dimensions_match = $true
      expected = $expectedInfo
      actual = $actualInfo
      exact_sha256_match = $true
      changed_pixels = 0L
      changed_fraction = 0.0
      max_channel_delta = 0
      mean_absolute_channel_delta = 0.0
      root_mean_square_channel_delta = 0.0
      diff_path = $null
    }
  }

  $expectedBitmap = [Drawing.Bitmap]::new($expectedInfo.path)
  $actualBitmap = [Drawing.Bitmap]::new($actualInfo.path)
  try {
    $expectedPixels = Copy-BitmapBytes -Bitmap $expectedBitmap
    $actualPixels = Copy-BitmapBytes -Bitmap $actualBitmap
    $nativeDiff = [ShotHarness.NativePixelDiff]::Compare(
        [byte[]]$expectedPixels.bytes, [int]$expectedPixels.stride,
        [byte[]]$actualPixels.bytes, [int]$actualPixels.stride,
        [int]$expectedInfo.width, [int]$expectedInfo.height, [bool]$DiffPath)
    $diffBytes = $nativeDiff.DiffBytes

    $resolvedDiff = $null
    if ($DiffPath) {
      $resolvedDiff = [IO.Path]::GetFullPath($DiffPath)
      [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($resolvedDiff)) | Out-Null
      $diffBitmap = [Drawing.Bitmap]::new(
          [int]$expectedInfo.width, [int]$expectedInfo.height,
          [Drawing.Imaging.PixelFormat]::Format32bppArgb)
      try {
        $rect = [Drawing.Rectangle]::new(0, 0, $diffBitmap.Width, $diffBitmap.Height)
        $data = $diffBitmap.LockBits(
            $rect, [Drawing.Imaging.ImageLockMode]::WriteOnly,
            [Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
          $targetStride = [Math]::Abs($data.Stride)
          for ($row = 0; $row -lt $data.Height; $row++) {
            $target = [IntPtr]::Add($data.Scan0, $row * $data.Stride)
            [Runtime.InteropServices.Marshal]::Copy(
                [byte[]]$diffBytes, $row * $targetStride, $target, $targetStride)
          }
        } finally {
          $diffBitmap.UnlockBits($data)
        }
        $diffBitmap.Save($resolvedDiff, [Drawing.Imaging.ImageFormat]::Png)
      } finally {
        $diffBitmap.Dispose()
      }
    }

    $pixelCount = [long]$expectedInfo.width * [long]$expectedInfo.height
    $channelCount = [double]$pixelCount * 4.0
    [pscustomobject]@{
      dimensions_match = $true
      expected = $expectedInfo
      actual = $actualInfo
      exact_sha256_match = $expectedInfo.sha256 -eq $actualInfo.sha256
      changed_pixels = $nativeDiff.ChangedPixels
      changed_fraction = if ($pixelCount) { $nativeDiff.ChangedPixels / [double]$pixelCount } else { 0.0 }
      max_channel_delta = $nativeDiff.MaxDelta
      mean_absolute_channel_delta = if ($channelCount) { $nativeDiff.AbsoluteDelta / $channelCount } else { 0.0 }
      root_mean_square_channel_delta = if ($channelCount) { [Math]::Sqrt($nativeDiff.SquaredDelta / $channelCount) } else { 0.0 }
      diff_path = $resolvedDiff
    }
  } finally {
    $expectedBitmap.Dispose()
    $actualBitmap.Dispose()
  }
}

function Get-RuntimeFootprint {
  [CmdletBinding()]
  param(
    [Parameter(Mandatory)][string]$Executable,
    [string]$RuntimeRoot
  )

  $binary = Get-Item -LiteralPath $Executable
  if ($RuntimeRoot) {
    $root = (Resolve-Path -LiteralPath $RuntimeRoot).Path
    $files = @(Get-ChildItem -LiteralPath $root -File -Recurse)
    $scope = 'explicit-runtime-root'
  } else {
    $root = $binary.DirectoryName
    $files = @($binary)
    $scope = 'executable-only-runtime-root-not-supplied'
  }

  $runtimeBytes = 0L
  foreach ($runtimeFile in $files) {
    $runtimeBytes += $runtimeFile.Length
  }

  [pscustomobject]@{
    scope = $scope
    root = $root
    binary_bytes = [long]$binary.Length
    runtime_bytes = $runtimeBytes
    runtime_file_count = $files.Count
  }
}

Export-ModuleMember -Function Resolve-CaptureInput, Get-EngineIdentity, Invoke-MeasuredProcess, `
  Invoke-ScreenshotCapture, Get-PngInfo, Compare-Png, Get-RuntimeFootprint
