[CmdletBinding()]
param(
  [Parameter(Mandatory)][string]$ShotExecutable,
  [string]$CasesManifest = (Join-Path $PSScriptRoot 'cases.json'),
  [string]$BaselineDirectory = (Join-Path $PSScriptRoot 'baselines'),
  [string]$ArtifactsDirectory = (Join-Path $PSScriptRoot 'out'),
  [ValidateRange(1, 600000)][int]$TimeoutMs = 30000,
  [ValidateRange(5, 1000)][int]$SamplingIntervalMs = 10,
  [Nullable[double]]$MaxChangedFraction,
  [Nullable[int]]$MaxChannelDelta
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib/ShotHarness.psm1') -Force

$casesPath = (Resolve-Path -LiteralPath $CasesManifest).Path
$casesRoot = Split-Path -Parent $casesPath
$baselineRoot = (Resolve-Path -LiteralPath $BaselineDirectory).Path
$baselineManifestPath = Join-Path $baselineRoot 'manifest.json'
if (-not (Test-Path -LiteralPath $baselineManifestPath)) {
  throw "Missing baseline manifest. Run update-baselines.ps1 -Accept first: $baselineManifestPath"
}

$cases = @(Get-Content -LiteralPath $casesPath -Raw | ConvertFrom-Json)
$baselineManifest = Get-Content -LiteralPath $baselineManifestPath -Raw | ConvertFrom-Json
if ($baselineManifest.cases_manifest_sha256 -ne
    (Get-FileHash -LiteralPath $casesPath -Algorithm SHA256).Hash.ToLowerInvariant()) {
  throw 'cases.json changed after the baselines were generated. Regenerate and review the baselines.'
}

$artifactRoot = [IO.Path]::GetFullPath($ArtifactsDirectory)
[IO.Directory]::CreateDirectory($artifactRoot) | Out-Null
$engine = Get-EngineIdentity -Engine shot -Executable $ShotExecutable
$results = [Collections.Generic.List[object]]::new()
$failures = 0

foreach ($case in $cases) {
  $source = [IO.Path]::GetFullPath((Join-Path $casesRoot $case.file))
  $baselineEntry = @($baselineManifest.cases | Where-Object name -eq $case.name)
  if ($baselineEntry.Count -ne 1) {
    throw "Baseline manifest has $($baselineEntry.Count) entries for $($case.name)."
  }
  $baseline = Join-Path $baselineRoot $baselineEntry[0].png
  if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant() -ne
      $baselineEntry[0].source_sha256) {
    throw "Fixture source drifted after baseline generation: $source"
  }
  foreach ($resource in @($baselineEntry[0].resources)) {
    $resourcePath = [IO.Path]::GetFullPath((Join-Path $casesRoot $resource.path))
    if ((Get-FileHash -LiteralPath $resourcePath -Algorithm SHA256).Hash.ToLowerInvariant() -ne
        $resource.sha256) {
      throw "Fixture resource drifted after baseline generation: $resourcePath"
    }
  }
  if ((Get-FileHash -LiteralPath $baseline -Algorithm SHA256).Hash.ToLowerInvariant() -ne
      $baselineEntry[0].png_sha256) {
    throw "Baseline PNG does not match its manifest: $baseline"
  }

  $actual = Join-Path $artifactRoot ("$($case.name).actual.png")
  $diffImage = Join-Path $artifactRoot ("$($case.name).diff.png")
  Write-Host "render $($case.name): source-build-shot"
  $capture = Invoke-ScreenshotCapture -Engine shot -Executable $ShotExecutable `
    -InputValue $source -OutputPath $actual -Width ([int]$case.width) `
    -Height ([int]$case.height) -Scale ([double]$case.scale) `
    -TimeoutMs $TimeoutMs -SamplingIntervalMs $SamplingIntervalMs
  $diff = Compare-Png -ExpectedPath $baseline -ActualPath $actual -DiffPath $diffImage
  $fractionLimit = if ($null -ne $MaxChangedFraction) {
    [double]$MaxChangedFraction
  } else {
    [double]$case.max_changed_fraction
  }
  $channelLimit = if ($null -ne $MaxChannelDelta) {
    [int]$MaxChannelDelta
  } else {
    [int]$case.max_channel_delta
  }
  $passed = $diff.dimensions_match -and
    $diff.changed_fraction -le $fractionLimit -and
    $diff.max_channel_delta -le $channelLimit
  if (-not $passed) { $failures++ }

  $results.Add([ordered]@{
    name = $case.name
    passed = $passed
    limits = [ordered]@{
      max_changed_fraction = $fractionLimit
      max_channel_delta = $channelLimit
    }
    capture = $capture
    diff = $diff
  })
}

$report = [ordered]@{
  schema_version = 1
  engine = $engine
  baseline_engine = $baselineManifest.baseline_engine
  passed = $failures -eq 0
  failures = $failures
  results = $results
}
$reportPath = Join-Path $artifactRoot 'report.json'
[IO.File]::WriteAllText(
  $reportPath, ($report | ConvertTo-Json -Depth 12) + [Environment]::NewLine,
  [Text.UTF8Encoding]::new($false))

$results | ForEach-Object {
  [pscustomobject]@{
    case = $_.name
    pass = $_.passed
    dimensions = if ($_.diff.dimensions_match) { 'match' } else { 'mismatch' }
    changed_pixels = $_.diff.changed_pixels
    changed_fraction = $_.diff.changed_fraction
    max_delta = $_.diff.max_channel_delta
    sha_match = $_.diff.exact_sha256_match
  }
} | Format-Table -AutoSize

Write-Host "Report: $reportPath"
if ($failures -ne 0) {
  throw "$failures render regression case(s) exceeded their pixel thresholds."
}
