[CmdletBinding()]
param(
  [Parameter(Mandatory)][string]$BaselineExecutable,
  [ValidateSet('headless-shell', 'system-chrome')]
  [string]$BaselineEngine = 'headless-shell',
  [Parameter(Mandatory)][string]$ShotExecutable,
  [string]$BaselineRuntimeRoot,
  [string]$ShotRuntimeRoot,
  [string[]]$Cases,
  [ValidateRange(1, 1000)][int]$Iterations = 5,
  [ValidateRange(0, 100)][int]$WarmupIterations = 1,
  [ValidateRange(1, 600000)][int]$TimeoutMs = 30000,
  [ValidateRange(5, 1000)][int]$SamplingIntervalMs = 10,
  [string]$OutputDirectory = (Join-Path $PSScriptRoot 'out')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot '../tests/render/lib/ShotHarness.psm1') -Force

function Get-FreeLoopbackPort {
  $probe = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
  $probe.Start()
  try { return ([Net.IPEndPoint]$probe.LocalEndpoint).Port } finally { $probe.Stop() }
}

function Start-LoopbackServer {
  param([Parameter(Mandatory)][int]$Port)
  $info = [Diagnostics.ProcessStartInfo]::new()
  $info.FileName = Join-Path $PSHOME 'pwsh.exe'
  $info.UseShellExecute = $false
  $info.CreateNoWindow = $true
  $info.WorkingDirectory = $PSScriptRoot
  foreach ($argument in @(
      '-NoLogo', '-NoProfile', '-NonInteractive', '-File',
      (Join-Path $PSScriptRoot 'serve.ps1'), '-Root', (Split-Path -Parent $PSScriptRoot),
      '-Port', "$Port")) {
    [void]$info.ArgumentList.Add($argument)
  }
  $process = [Diagnostics.Process]::new()
  $process.StartInfo = $info
  if (-not $process.Start()) { throw 'Unable to start the loopback fixture server.' }

  $client = [Net.Http.HttpClient]::new()
  try {
    for ($attempt = 0; $attempt -lt 50; $attempt++) {
      if ($process.HasExited) { throw "Loopback server exited with code $($process.ExitCode)." }
      try {
        $response = $client.GetAsync("http://127.0.0.1:$Port/__health").GetAwaiter().GetResult()
        if ($response.IsSuccessStatusCode) { return $process }
      } catch {
        Start-Sleep -Milliseconds 100
      }
    }
    throw 'Loopback fixture server did not become ready within five seconds.'
  } finally {
    $client.Dispose()
  }
}

function Get-Percentile {
  param([Parameter(Mandatory)][double[]]$SortedValues, [Parameter(Mandatory)][double]$Percentile)
  if ($SortedValues.Count -eq 0) { return $null }
  $index = [Math]::Max(0, [Math]::Ceiling($Percentile * $SortedValues.Count) - 1)
  return $SortedValues[$index]
}

function Get-Distribution {
  param([Parameter(Mandatory)][object[]]$Values)
  $numbers = @($Values | ForEach-Object { [double]$_ } | Sort-Object)
  [ordered]@{
    min = $numbers[0]
    p50 = Get-Percentile -SortedValues $numbers -Percentile .50
    p95 = Get-Percentile -SortedValues $numbers -Percentile .95
    max = $numbers[-1]
    mean = ($numbers | Measure-Object -Average).Average
  }
}

$caseManifestPath = Join-Path $PSScriptRoot 'cases.json'
$caseDefinitions = @(Get-Content -LiteralPath $caseManifestPath -Raw | ConvertFrom-Json)
if ($Cases) {
  $unknown = @($Cases | Where-Object { $_ -notin $caseDefinitions.name })
  if ($unknown) { throw "Unknown benchmark case(s): $($unknown -join ', ')" }
  $caseDefinitions = @($caseDefinitions | Where-Object name -in $Cases)
}

$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($outputRoot) | Out-Null
$engineDefinitions = @(
  [pscustomobject]@{
    engine = $BaselineEngine
    label = if ($BaselineEngine -eq 'system-chrome') { 'external-system-chrome' } else { 'source-build-headless-shell-baseline' }
    executable = (Get-Item -LiteralPath $BaselineExecutable).FullName
    runtime_root = $BaselineRuntimeRoot
  },
  [pscustomobject]@{
    engine = 'shot'
    label = 'source-build-shot'
    executable = (Get-Item -LiteralPath $ShotExecutable).FullName
    runtime_root = $ShotRuntimeRoot
  }
)

$server = $null
$port = $null
$raw = [Collections.Generic.List[object]]::new()
try {
  if (@($caseDefinitions | Where-Object {
        $_.PSObject.Properties.Name -contains 'loopback_http' -and $_.loopback_http
      }).Count) {
    $port = Get-FreeLoopbackPort
    $server = Start-LoopbackServer -Port $port
  }

  foreach ($engine in $engineDefinitions) {
    foreach ($case in $caseDefinitions) {
      $inputValue = if ($case.PSObject.Properties.Name -contains 'loopback_http' -and $case.loopback_http) {
        "http://127.0.0.1:$port/bench/$($case.file)"
      } else {
        Join-Path $PSScriptRoot $case.file
      }
      $png = Join-Path $outputRoot ("$($engine.label).$($case.name).png")

      for ($warmup = 0; $warmup -lt $WarmupIterations; $warmup++) {
        Write-Host "warmup $($engine.label) $($case.name) $($warmup + 1)/$WarmupIterations"
        Invoke-ScreenshotCapture -Engine $engine.engine -Executable $engine.executable `
          -InputValue $inputValue -OutputPath $png -Width ([int]$case.width) `
          -Height ([int]$case.height) -Scale ([double]$case.scale) `
          -TimeoutMs $TimeoutMs -SamplingIntervalMs $SamplingIntervalMs | Out-Null
      }

      for ($iteration = 1; $iteration -le $Iterations; $iteration++) {
        Write-Host "measure $($engine.label) $($case.name) $iteration/$Iterations"
        $capture = Invoke-ScreenshotCapture -Engine $engine.engine -Executable $engine.executable `
          -InputValue $inputValue -OutputPath $png -Width ([int]$case.width) `
          -Height ([int]$case.height) -Scale ([double]$case.scale) `
          -TimeoutMs $TimeoutMs -SamplingIntervalMs $SamplingIntervalMs
        if ($capture.png.width -ne [int]$case.width -or $capture.png.height -ne [int]$case.height) {
          throw "$($engine.label) $($case.name) produced $($capture.png.width)x$($capture.png.height); v1 requires $($case.width)x$($case.height)."
        }
        $raw.Add([pscustomobject]@{
          engine = $engine.label
          case = $case.name
          iteration = $iteration
          wall_time_ms = $capture.measurement.wall_time_ms
          peak_processes = $capture.measurement.peak_processes
          peak_threads = $capture.measurement.peak_threads
          peak_rss_bytes = $capture.measurement.peak_rss_bytes
          processes_seen = $capture.measurement.processes_seen
          process_names = @($capture.measurement.process_names) -join ';'
          process_samples = $capture.measurement.sample_count
          process_sample_requested_interval_ms = $capture.measurement.requested_sample_interval_ms
          process_observed_mean_sample_period_ms = $capture.measurement.observed_mean_sample_period_ms
          png_width = $capture.png.width
          png_height = $capture.png.height
          png_bytes = $capture.png.bytes
          png_sha256 = $capture.png.sha256
        })
      }
    }
  }
} finally {
  if ($server -and -not $server.HasExited) {
    $server.Kill($true)
    $server.WaitForExit()
  }
}

$summaries = [Collections.Generic.List[object]]::new()
foreach ($group in ($raw | Group-Object engine, case)) {
  $rows = @($group.Group)
  $summaries.Add([ordered]@{
    engine = $rows[0].engine
    case = $rows[0].case
    samples = $rows.Count
    wall_time_ms = Get-Distribution -Values $rows.wall_time_ms
    peak_rss_bytes = Get-Distribution -Values $rows.peak_rss_bytes
    peak_processes = Get-Distribution -Values $rows.peak_processes
    peak_threads = Get-Distribution -Values $rows.peak_threads
  })
}

$engines = foreach ($engine in $engineDefinitions) {
  [ordered]@{
    identity = Get-EngineIdentity -Engine $engine.engine -Executable $engine.executable
    footprint = Get-RuntimeFootprint -Executable $engine.executable -RuntimeRoot $engine.runtime_root
  }
}
$gitHead = (& git -C (Split-Path -Parent $PSScriptRoot) rev-parse HEAD 2>$null)
$processorName = try {
  (Get-ItemProperty -LiteralPath 'HKLM:\HARDWARE\DESCRIPTION\System\CentralProcessor\0').ProcessorNameString.Trim()
} catch {
  $null
}
$report = [ordered]@{
  schema_version = 1
  generated_utc = [DateTime]::UtcNow.ToString('o')
  source_revision = if ($LASTEXITCODE -eq 0) { "$gitHead".Trim() } else { $null }
  measurement_model = [ordered]@{
    lifecycle = 'one-new-process-tree-per-capture'
    warmup_note = 'Warmups populate host file caches; every recorded sample still includes a new process startup.'
    rss = 'maximum sampled sum of working-set bytes across the discovered process tree'
    process_and_threads = 'maximum sampled process count and summed thread count across that tree'
    sampling_requested_interval_ms = $SamplingIntervalMs
    sampling_note = 'The requested sleep occurs between snapshots; native snapshot overhead is additional. Raw samples report the observed mean period.'
  }
  host = [ordered]@{
    machine = [Environment]::MachineName
    os = [Environment]::OSVersion.VersionString
    processor = $processorName
    logical_processors = [Environment]::ProcessorCount
    powershell = $PSVersionTable.PSVersion.ToString()
  }
  config = [ordered]@{
    iterations = $Iterations
    warmup_iterations = $WarmupIterations
    timeout_ms = $TimeoutMs
    cases_manifest_sha256 = (Get-FileHash -LiteralPath $caseManifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
  }
  engines = $engines
  summary = $summaries
  samples = $raw
}

$jsonPath = Join-Path $outputRoot 'benchmark.json'
$csvPath = Join-Path $outputRoot 'benchmark.csv'
[IO.File]::WriteAllText(
  $jsonPath, ($report | ConvertTo-Json -Depth 12) + [Environment]::NewLine,
  [Text.UTF8Encoding]::new($false))
$raw | Export-Csv -LiteralPath $csvPath -NoTypeInformation -Encoding utf8

$summaries | ForEach-Object {
  [pscustomobject]@{
    engine = $_.engine
    case = $_.case
    samples = $_.samples
    wall_p50_ms = [Math]::Round($_.wall_time_ms.p50, 2)
    wall_p95_ms = [Math]::Round($_.wall_time_ms.p95, 2)
    peak_rss_mib = [Math]::Round($_.peak_rss_bytes.max / 1MB, 2)
    peak_processes = $_.peak_processes.max
    peak_threads = $_.peak_threads.max
  }
} | Format-Table -AutoSize
Write-Host "JSON: $jsonPath"
Write-Host "CSV:  $csvPath"
