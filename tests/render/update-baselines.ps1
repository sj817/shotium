[CmdletBinding()]
param(
  [Parameter(Mandatory)][string]$BaselineExecutable,
  [ValidateSet('headless-shell', 'system-chrome')]
  [string]$BaselineEngine = 'headless-shell',
  [string]$CasesManifest = (Join-Path $PSScriptRoot 'cases.json'),
  [string]$BaselineDirectory = (Join-Path $PSScriptRoot 'baselines'),
  [ValidateRange(1, 600000)][int]$TimeoutMs = 30000,
  [ValidateRange(5, 1000)][int]$SamplingIntervalMs = 10,
  [switch]$Accept
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $Accept) {
  throw 'Baseline replacement is intentional and destructive. Re-run with -Accept after reviewing the pinned baseline executable.'
}

Import-Module (Join-Path $PSScriptRoot 'lib/ShotHarness.psm1') -Force
$manifestPath = (Resolve-Path -LiteralPath $CasesManifest).Path
$manifestRoot = Split-Path -Parent $manifestPath
$cases = @(Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json)
$baselineRoot = [IO.Path]::GetFullPath($BaselineDirectory)
[IO.Directory]::CreateDirectory($baselineRoot) | Out-Null
$stagingRoot = Join-Path ([IO.Path]::GetTempPath()) ("shot-baselines-" + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($stagingRoot) | Out-Null
$engine = Get-EngineIdentity -Engine $BaselineEngine -Executable $BaselineExecutable
$caseResults = [Collections.Generic.List[object]]::new()

try {
foreach ($case in $cases) {
  $source = [IO.Path]::GetFullPath((Join-Path $manifestRoot $case.file))
  $output = Join-Path $stagingRoot ("$($case.name).png")
  Write-Host "baseline $($case.name): $BaselineEngine"
  $capture = Invoke-ScreenshotCapture -Engine $BaselineEngine `
    -Executable $BaselineExecutable -InputValue $source -OutputPath $output `
    -Width ([int]$case.width) -Height ([int]$case.height) -Scale ([double]$case.scale) `
    -TimeoutMs $TimeoutMs -SamplingIntervalMs $SamplingIntervalMs

  $expectedWidth = [int][Math]::Round([double]$case.width * [double]$case.scale)
  $expectedHeight = [int][Math]::Round([double]$case.height * [double]$case.scale)
  if ($capture.png.width -ne $expectedWidth -or $capture.png.height -ne $expectedHeight) {
    throw "Baseline $($case.name) has $($capture.png.width)x$($capture.png.height), expected ${expectedWidth}x${expectedHeight}."
  }

  $resources = @()
  if ($case.PSObject.Properties.Name -contains 'resources') {
    $resources = @($case.resources | ForEach-Object {
      $resourcePath = [IO.Path]::GetFullPath((Join-Path $manifestRoot $_))
      [ordered]@{
        path = $_
        sha256 = (Get-FileHash -LiteralPath $resourcePath -Algorithm SHA256).Hash.ToLowerInvariant()
      }
    })
  }

  $caseResults.Add([ordered]@{
    name = $case.name
    source = $case.file
    source_sha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
    resources = $resources
    png = "$($case.name).png"
    png_width = $capture.png.width
    png_height = $capture.png.height
    png_bytes = $capture.png.bytes
    png_sha256 = $capture.png.sha256
  })
}

$baselineManifest = [ordered]@{
  schema_version = 1
  baseline_engine = [ordered]@{
    identity = $engine.identity
    engine = $engine.engine
    executable_name = [IO.Path]::GetFileName($engine.executable)
    binary_bytes = $engine.binary_bytes
    binary_sha256 = $engine.binary_sha256
    file_version = $engine.file_version
  }
  cases_manifest_sha256 = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
  cases = $caseResults
}
$json = $baselineManifest | ConvertTo-Json -Depth 8
[IO.File]::WriteAllText(
  (Join-Path $stagingRoot 'manifest.json'), $json + [Environment]::NewLine,
  [Text.UTF8Encoding]::new($false))

foreach ($stagedFile in (Get-ChildItem -LiteralPath $stagingRoot -Filter '*.png' -File)) {
  Copy-Item -LiteralPath $stagedFile.FullName -Destination (Join-Path $baselineRoot $stagedFile.Name) -Force
}
Copy-Item -LiteralPath (Join-Path $stagingRoot 'manifest.json') `
  -Destination (Join-Path $baselineRoot 'manifest.json') -Force
} finally {
  if (Test-Path -LiteralPath $stagingRoot) {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $resolvedStaging = [IO.Path]::GetFullPath($stagingRoot)
    if (-not $resolvedStaging.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
      throw "Refusing to remove baseline staging directory outside temp: $resolvedStaging"
    }
    Remove-Item -LiteralPath $resolvedStaging -Recurse -Force
  }
}

Write-Host "Updated $($caseResults.Count) baselines in $baselineRoot"
