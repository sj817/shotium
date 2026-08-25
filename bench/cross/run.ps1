[CmdletBinding()]
param(
  [string]$ShotBinary = (Join-Path $PSScriptRoot '../../out/ShotWip/shot.exe'),
  [string[]]$Engines = @('shotium', 'puppeteer-shell', 'puppeteer-chrome',
                         'playwright-shell', 'playwright-chrome'),
  [string[]]$Scenarios = @('cold', 'cold-settled', 'warm', 'batch',
                           'batch-parallel', 'reuse'),
  [ValidateRange(1, 100)][int]$Repeats = 5,
  [ValidateRange(1, 200)][int]$Iterations = 10,
  [ValidateRange(0, 20)][int]$Warmup = 3,
  [ValidateRange(1, 32)][int]$Concurrency = 4,
  [ValidateRange(1, 1000)][int]$SamplingIntervalMs = 20,
  [ValidateRange(1000, 3600000)][int]$TimeoutMs = 120000,
  [ValidateRange(100, 60000)][int]$ResidentSampleMs = 3000,
  [ValidateRange(0, 120000)][int]$ResidentSettleMs = 15000,
  [switch]$IncludeReusePage,
  [string]$OutputDirectory = (Join-Path $PSScriptRoot 'out')
)

# Runs every engine through every scenario, one fresh process tree per sample.
#
# The runner measures what happens inside itself; this measures the process
# tree from outside and joins the two on wall-clock timestamps. Nothing here
# knows what an engine is -- that is bench/cross/lib/engines.js -- and nothing
# in the runner knows how memory is sampled, which is what keeps either from
# quietly flattering the other.
#
#   pwsh ./bench/cross/run.ps1 -Repeats 5
#   pwsh ./bench/cross/run.ps1 -Engines shotium,playwright-shell -Scenarios warm

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib/Sampler.psm1') -Force

$node = (Get-Command node).Source
$shot = (Resolve-Path -LiteralPath $ShotBinary).Path
$env:SHOTIUM_BINARY = $shot
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($outputRoot) | Out-Null
$sampleDir = Join-Path $outputRoot 'samples'
$sampleLog = Join-Path $outputRoot 'samples.jsonl'
$endpointFile = Join-Path $outputRoot 'host.json'

function Get-Percentile {
  param([Parameter(Mandatory)][double[]]$Sorted, [Parameter(Mandatory)][double]$P)
  if ($Sorted.Count -eq 0) { return $null }
  $index = [Math]::Max(0, [Math]::Ceiling($P * $Sorted.Count) - 1)
  $Sorted[$index]
}

function Get-Distribution {
  param([object[]]$Values)
  $numbers = @($Values | Where-Object { $null -ne $_ } | ForEach-Object { [double]$_ } | Sort-Object)
  if ($numbers.Count -eq 0) { return $null }
  [ordered]@{
    n = $numbers.Count
    min = [Math]::Round($numbers[0], 2)
    p50 = [Math]::Round((Get-Percentile -Sorted $numbers -P .50), 2)
    p95 = [Math]::Round((Get-Percentile -Sorted $numbers -P .95), 2)
    max = [Math]::Round($numbers[-1], 2)
    mean = [Math]::Round(($numbers | Measure-Object -Average).Average, 2)
  }
}

# The peak of a timeline, with the breakdown taken at that same instant rather
# than as a per-name maximum: the maxima of the parts do not have to happen
# together, and adding them up would report a total the machine never held.
function Get-TimelinePeak {
  param([object[]]$Timeline)
  $peak = $null
  foreach ($sample in $Timeline) {
    if ($null -eq $peak -or $sample.WorkingSetBytes -gt $peak.WorkingSetBytes) {
      $peak = $sample
    }
  }
  if ($null -eq $peak) { return $null }
  $byName = [ordered]@{}
  foreach ($pair in ($peak.ByName.GetEnumerator() | Sort-Object -Property Key)) {
    $byName[$pair.Key] = [long]$pair.Value
  }
  # The private half is taken from the same instant for the same reason the
  # breakdown is: it is a property of that sample, not a second maximum.
  $privateByName = [ordered]@{}
  foreach ($pair in ($peak.PrivateByName.GetEnumerator() | Sort-Object -Property Key)) {
    $privateByName[$pair.Key] = [long]$pair.Value
  }
  [ordered]@{
    at = $peak.At
    bytes = [long]$peak.WorkingSetBytes
    private_bytes = [long]$peak.PrivateWorkingSetBytes
    processes = $peak.ProcessCount
    threads = $peak.ThreadCount
    by_name = $byName
    private_by_name = $privateByName
  }
}

function Get-PhasePeaks {
  param([object[]]$Timeline, [object[]]$Marks)
  $phases = [ordered]@{}
  for ($i = 0; $i -lt $Marks.Count - 1; $i++) {
    $from = [long]$Marks[$i].at
    $to = [long]$Marks[$i + 1].at
    $window = @($Timeline | Where-Object { $_.At -ge $from -and $_.At -le $to })
    $peak = Get-TimelinePeak -Timeline $window
    $phases["$($Marks[$i].name)..$($Marks[$i + 1].name)"] = [ordered]@{
      ms = $to - $from
      samples = $window.Count
      peak_rss_bytes = if ($peak) { $peak.bytes } else { $null }
      peak_private_bytes = if ($peak) { $peak.private_bytes } else { $null }
    }
  }
  $phases
}

function Get-EngineBytes {
  param([object]$Peak, [switch]$Private)
  if ($null -eq $Peak) { return $null }
  $total = if ($Private) { [long]$Peak.private_bytes } else { [long]$Peak.bytes }
  $breakdown = if ($Private) { $Peak.private_by_name } else { $Peak.by_name }
  $node = 0L
  foreach ($pair in $breakdown.GetEnumerator()) {
    if ($pair.Key -ieq 'node.exe' -or $pair.Key -ieq 'node') { $node += [long]$pair.Value }
  }
  $total - $node
}

$rows = [Collections.Generic.List[object]]::new()
$residents = [Collections.Generic.List[object]]::new()
$failures = [Collections.Generic.List[object]]::new()

# One bad sample must not cost the other two hundred. A run takes twenty
# minutes, a browser occasionally wedges, and losing the whole dataset to the
# last cell in the grid is a worse outcome than a table with a hole in it -- so
# failures are recorded, reported at the end, and carried into the JSON.
function Invoke-MeasuredSafely {
  param([hashtable]$Arguments)
  try {
    Invoke-Measured @Arguments
  } catch {
    # ContainsKey, not $Arguments.ReusePage: Set-StrictMode makes reading a key
    # that is not there an error, and an error inside the handler for an error
    # takes down the run it was written to save.
    $failures.Add([pscustomobject]@{
      engine = $Arguments.Engine
      scenario = $Arguments.Scenario
      repeat = $Arguments.Repeat
      reuse_page = $Arguments.ContainsKey('ReusePage') -and $Arguments.ReusePage
      error = "$($_.Exception.Message)"
    })
    Write-Warning ("  {0} {1} {2}: {3}" -f $Arguments.Engine, $Arguments.Scenario,
                   $Arguments.Repeat, $_.Exception.Message)
  }
}

# Every sample, appended as it is taken.
#
# A full grid is twenty minutes of work, and losing all of it to whatever the
# last cell did is a bad trade for one line of code. The final JSON is still
# assembled at the end; this is the copy that survives an interrupted run.
function Write-SampleLine {
  param([Parameter(Mandatory)][object]$Row)
  $line = ($Row | ConvertTo-Json -Depth 12 -Compress)
  [IO.File]::AppendAllText($script:sampleLog, $line + [Environment]::NewLine,
                           [Text.UTF8Encoding]::new($false))
}

function Invoke-Measured {
  param(
    [Parameter(Mandatory)][string]$Engine,
    [Parameter(Mandatory)][string]$Scenario,
    [Parameter(Mandatory)][int]$Repeat,
    [string[]]$Extra = @(),
    [bool]$ReusePage = $false
  )
  $arguments = @(
    (Join-Path $PSScriptRoot 'runner.js'),
    '--engine', $Engine, '--scenario', $Scenario,
    '--iterations', "$Iterations", '--warmup', "$Warmup",
    '--concurrency', "$Concurrency", '--sample-dir', $sampleDir) + $Extra
  if ($ReusePage) { $arguments += '--reuse-page' }

  $run = Invoke-SampledProcess -Executable $node -Arguments $arguments `
    -WorkingDirectory $PSScriptRoot -IntervalMs $SamplingIntervalMs -TimeoutMs $TimeoutMs
  if ($run.timed_out) { throw "$Engine/$Scenario exceeded $TimeoutMs ms" }
  if ($run.exit_code -ne 0) {
    throw "$Engine/$Scenario exited $($run.exit_code): $($run.stderr)"
  }
  $report = $run.stdout | ConvertFrom-Json
  $peak = Get-TimelinePeak -Timeline $run.timeline
  $shots = @($report.shots)

  $row = [ordered]@{
    engine = $Engine
    scenario = $Scenario
    reuse_page = $ReusePage
    repeat = $Repeat
    # Wall time of the whole process tree: node starting, the library loading,
    # the engine launching, every screenshot, and the shutdown. It is the only
    # number here that a user can feel directly.
    wall_time_ms = $run.wall_time_ms
    require_ms = $report.timings.require_ms
    launch_ms = if ($report.timings.PSObject.Properties.Name -contains 'launch_ms') { $report.timings.launch_ms } else { $null }
    connect_ms = if ($report.timings.PSObject.Properties.Name -contains 'connect_ms') { $report.timings.connect_ms } else { $null }
    first_shot_ms = if ($report.timings.PSObject.Properties.Name -contains 'first_shot_ms') { $report.timings.first_shot_ms } else { $null }
    total_ms = if ($report.timings.PSObject.Properties.Name -contains 'total_ms') { $report.timings.total_ms } else { $null }
    close_ms = if ($report.timings.PSObject.Properties.Name -contains 'close_ms') { $report.timings.close_ms } else { $null }
    shot_count = $shots.Count
    shot_ms = @($shots | ForEach-Object { $_.ms })
    shot_bytes = @($shots | ForEach-Object { $_.bytes })
    png_sha256 = @($shots | ForEach-Object { $_.sha256 })
    png_size = if ($shots.Count) { "$($shots[0].width)x$($shots[0].height)" } else { $null }
    peak_rss_bytes = if ($peak) { $peak.bytes } else { $null }
    peak_rss_engine_bytes = Get-EngineBytes -Peak $peak
    peak_private_bytes = if ($peak) { $peak.private_bytes } else { $null }
    peak_private_engine_bytes = Get-EngineBytes -Peak $peak -Private
    peak_processes = if ($peak) { $peak.processes } else { $null }
    peak_threads = if ($peak) { $peak.threads } else { $null }
    peak_by_name = if ($peak) { $peak.by_name } else { $null }
    peak_private_by_name = if ($peak) { $peak.private_by_name } else { $null }
    phases = Get-PhasePeaks -Timeline $run.timeline -Marks @($report.marks)
    samples = $run.timeline.Count
    observed_mean_period_ms = $run.observed_mean_period_ms
  }
  $rows.Add([pscustomobject]$row)
  Write-SampleLine -Row $row
  Write-Host ("  {0,-18} {1,-14} {2}/{3}  wall {4,7:N0}ms  peak {5,6:N1} MiB ({6,6:N1} private)" -f `
      $Engine, $Scenario, $Repeat, $Repeats, $run.wall_time_ms,
      $(if ($peak) { $peak.bytes / 1MB } else { 0 }),
      $(if ($peak) { $peak.private_bytes / 1MB } else { 0 }))
}

function Start-EngineHost {
  param([Parameter(Mandatory)][string]$Engine)
  if (Test-Path -LiteralPath $endpointFile) { Remove-Item -LiteralPath $endpointFile -Force }
  $info = [Diagnostics.ProcessStartInfo]::new()
  $info.FileName = $node
  $info.WorkingDirectory = $PSScriptRoot
  $info.UseShellExecute = $false
  $info.CreateNoWindow = $true
  $info.RedirectStandardOutput = $true
  $info.RedirectStandardError = $true
  foreach ($argument in @((Join-Path $PSScriptRoot 'host.js'), '--engine', $Engine,
                          '--endpoint-file', $endpointFile, '--workers', "$Concurrency")) {
    [void]$info.ArgumentList.Add($argument)
  }
  $process = [Diagnostics.Process]::new()
  $process.StartInfo = $info
  if (-not $process.Start()) { throw "could not start the $Engine host" }
  $stderr = $process.StandardError.ReadToEndAsync()
  for ($attempt = 0; $attempt -lt 600; $attempt++) {
    if (Test-Path -LiteralPath $endpointFile) {
      return [pscustomobject]@{
        process = $process
        info = (Get-Content -LiteralPath $endpointFile -Raw | ConvertFrom-Json)
      }
    }

    if ($process.HasExited -and -not (Test-Path -LiteralPath $endpointFile)) {
      throw "the $Engine host exited before publishing an endpoint: $($stderr.GetAwaiter().GetResult())"
    }
    Start-Sleep -Milliseconds 100
  }
  throw "the $Engine host did not publish an endpoint within a minute"
}

function Stop-EngineHost {
  # Not $Host: that name is PowerShell's own, and shadowing it inside a
  # function that also writes to the console is a debugging session nobody
  # needs.
  param([Parameter(Mandatory)][string]$Engine, [Parameter(Mandatory)][object]$Running)
  if ($Engine -eq 'shotium-daemon') {
    & $node (Join-Path $PSScriptRoot 'host.js') '--engine' $Engine '--stop' | Out-Null
  }
  if (-not $Running.process.HasExited) {
    $Running.process.Kill($true)
    $Running.process.WaitForExit()
  }
}

Write-Host "shot binary: $shot"
Write-Host "engines:     $($Engines -join ', ')"
Write-Host "scenarios:   $($Scenarios -join ', ')"
Write-Host ''

foreach ($scenario in $Scenarios) {
  foreach ($engine in $Engines) {
    if ($scenario -eq 'reuse') {
      $hostEngine = if ($engine -eq 'shotium') { 'shotium-daemon' } else { $engine }
      $engineHost = Start-EngineHost -Engine $hostEngine
      try {
        # What the resident engine costs while nothing is being asked of it,
        # measured after it has had a chance to stop doing things. Every engine
        # is left alone for the same stretch first: each one has housekeeping
        # it runs on its own once the requests stop -- a garbage collection, a
        # cache purge, an allocator returning pages -- and sampling from the
        # instant the warmup shot returned would report the peak of that work
        # rather than the cost of being there. Which is the question.
        if ($ResidentSettleMs -gt 0) { Start-Sleep -Milliseconds $ResidentSettleMs }
        $idle = Measure-ResidentTree -RootIds @([int[]]$engineHost.info.pids) `
          -DurationMs $ResidentSampleMs -IntervalMs 50
        $idlePeak = Get-TimelinePeak -Timeline $idle
        $residents.Add([pscustomobject][ordered]@{
          engine = $engine
          host_engine = $hostEngine
          resident_rss_bytes = $idlePeak.bytes
          resident_engine_bytes = Get-EngineBytes -Peak $idlePeak
          resident_private_bytes = $idlePeak.private_bytes
          resident_private_engine_bytes = Get-EngineBytes -Peak $idlePeak -Private
          resident_processes = $idlePeak.processes
          resident_threads = $idlePeak.threads
          resident_by_name = $idlePeak.by_name
          resident_private_by_name = $idlePeak.private_by_name
        })
        Write-Host ("  {0,-18} resident       {1,6:N1} MiB ({2,6:N1} private) in {3} processes" -f `
            $engine, ($idlePeak.bytes / 1MB), ($idlePeak.private_bytes / 1MB),
            $idlePeak.processes)
        for ($repeat = 1; $repeat -le $Repeats; $repeat++) {
          Invoke-MeasuredSafely -Arguments @{
            Engine = $hostEngine; Scenario = 'reuse'; Repeat = $repeat
            Extra = @('--endpoint-file', $endpointFile)
          }
        }
      } finally {
        Stop-EngineHost -Engine $hostEngine -Running $engineHost
      }
      continue
    }

    for ($repeat = 1; $repeat -le $Repeats; $repeat++) {
      Invoke-MeasuredSafely -Arguments @{
        Engine = $engine; Scenario = $scenario; Repeat = $repeat
      }
    }
    # Sequential scenarios only. Reusing several pages at once is a
    # configuration full headless Chrome does not like -- captures of the pages
    # that are not in front took tens of seconds each on this host -- so the
    # timing would measure Chrome's frame scheduling for non-foreground pages
    # rather than screenshot throughput. The fresh-page rows cover concurrency.
    if ($IncludeReusePage -and $engine -ne 'shotium' -and
        $scenario -in @('warm', 'batch')) {
      for ($repeat = 1; $repeat -le $Repeats; $repeat++) {
        Invoke-MeasuredSafely -Arguments @{
          Engine = $engine; Scenario = $scenario; Repeat = $repeat; ReusePage = $true
        }
      }
    }
  }
}

$summaries = [Collections.Generic.List[object]]::new()
foreach ($group in ($rows | Group-Object engine, scenario, reuse_page)) {
  $items = @($group.Group)
  $perShot = @($items | ForEach-Object { $_.shot_ms } | ForEach-Object { $_ })
  $summaries.Add([ordered]@{
    engine = $items[0].engine
    scenario = $items[0].scenario
    reuse_page = $items[0].reuse_page
    repeats = $items.Count
    wall_time_ms = Get-Distribution -Values @($items | ForEach-Object { $_.wall_time_ms })
    launch_ms = Get-Distribution -Values @($items | ForEach-Object { $_.launch_ms })
    connect_ms = Get-Distribution -Values @($items | ForEach-Object { $_.connect_ms })
    first_shot_ms = Get-Distribution -Values @($items | ForEach-Object { $_.first_shot_ms })
    total_ms = Get-Distribution -Values @($items | ForEach-Object { $_.total_ms })
    shot_ms = Get-Distribution -Values $perShot
    peak_rss_bytes = Get-Distribution -Values @($items | ForEach-Object { $_.peak_rss_bytes })
    peak_rss_engine_bytes = Get-Distribution -Values @($items | ForEach-Object { $_.peak_rss_engine_bytes })
    peak_private_bytes = Get-Distribution -Values @($items | ForEach-Object { $_.peak_private_bytes })
    peak_private_engine_bytes = Get-Distribution -Values @($items | ForEach-Object { $_.peak_private_engine_bytes })
    peak_processes = Get-Distribution -Values @($items | ForEach-Object { $_.peak_processes })
    peak_threads = Get-Distribution -Values @($items | ForEach-Object { $_.peak_threads })
  })
}

function Get-Footprint {
  param([Parameter(Mandatory)][string]$Path)
  $file = Get-Item -LiteralPath $Path
  [ordered]@{
    path = $file.FullName
    bytes = [long]$file.Length
    sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
  }
}

$packages = [ordered]@{}
foreach ($name in @('puppeteer', 'playwright')) {
  $manifest = Join-Path $PSScriptRoot "node_modules/$name/package.json"
  if (Test-Path -LiteralPath $manifest) {
    $packages[$name] = (Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json).version
  }
}

$processorName = try {
  (Get-ItemProperty -LiteralPath 'HKLM:\HARDWARE\DESCRIPTION\System\CentralProcessor\0').ProcessorNameString.Trim()
} catch { $null }

$report = [ordered]@{
  schema_version = 1
  generated_utc = [DateTime]::UtcNow.ToString('o')
  source_revision = (& git -C (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) rev-parse HEAD 2>$null)
  measurement_model = [ordered]@{
    lifecycle = 'one fresh node process tree per sample'
    wall_time = 'process start to process exit, sampled from outside'
    rss = 'maximum sampled sum of working sets over the tree, broken down by image name at that instant'
    engine_rss = 'that peak minus the node processes in it'
    corpus = 'bench/cases.json, local cases only, 1280x720, scale 1, PNG, viewport'
    wait_until = 'load'
    pages = 'a fresh page per screenshot unless reuse_page is true'
  }
  host = [ordered]@{
    machine = [Environment]::MachineName
    os = [Environment]::OSVersion.VersionString
    processor = $processorName
    logical_processors = [Environment]::ProcessorCount
    powershell = $PSVersionTable.PSVersion.ToString()
    node = (& $node --version)
  }
  config = [ordered]@{
    repeats = $Repeats
    iterations = $Iterations
    warmup = $Warmup
    concurrency = $Concurrency
    sampling_interval_ms = $SamplingIntervalMs
    resident_sample_ms = $ResidentSampleMs
    resident_settle_ms = $ResidentSettleMs
  }
  engines = [ordered]@{
    shot = Get-Footprint -Path $shot
    packages = $packages
  }
  resident = $residents
  failures = $failures
  summary = $summaries
  samples = $rows
}

$jsonPath = Join-Path $outputRoot 'benchmark.json'
[IO.File]::WriteAllText($jsonPath, ($report | ConvertTo-Json -Depth 14) + [Environment]::NewLine,
                        [Text.UTF8Encoding]::new($false))
$rows | Select-Object engine, scenario, reuse_page, repeat, wall_time_ms, launch_ms, connect_ms,
    first_shot_ms, total_ms, peak_rss_bytes, peak_rss_engine_bytes, peak_private_bytes,
    peak_private_engine_bytes, peak_processes, peak_threads |
  Export-Csv -LiteralPath (Join-Path $outputRoot 'benchmark.csv') -NoTypeInformation -Encoding utf8

$summaries | ForEach-Object {
  [pscustomobject]@{
    engine = $_.engine
    scenario = $_.scenario
    reuse = $_.reuse_page
    wall_p50 = if ($_.wall_time_ms) { $_.wall_time_ms.p50 } else { $null }
    shot_p50 = if ($_.shot_ms) { $_.shot_ms.p50 } else { $null }
    shot_p95 = if ($_.shot_ms) { $_.shot_ms.p95 } else { $null }
    peak_mib = if ($_.peak_rss_bytes) { [Math]::Round($_.peak_rss_bytes.max / 1MB, 1) } else { $null }
    private_mib = if ($_.peak_private_bytes) { [Math]::Round($_.peak_private_bytes.max / 1MB, 1) } else { $null }
    procs = if ($_.peak_processes) { $_.peak_processes.max } else { $null }
  }
} | Format-Table -AutoSize

if ($failures.Count) {
  Write-Warning "$($failures.Count) sample(s) failed; the JSON lists them under 'failures'."
}
& $node (Join-Path $PSScriptRoot 'report.js') $jsonPath
Write-Host "JSON:   $jsonPath"
Write-Host "report: $(Join-Path $outputRoot 'REPORT.md')"
