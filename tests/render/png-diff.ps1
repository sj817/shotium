[CmdletBinding()]
param(
  [Parameter(Mandatory)][string]$Expected,
  [Parameter(Mandatory)][string]$Actual,
  [string]$Diff,
  [ValidateRange(0, 1)][double]$MaxChangedFraction = 0,
  [ValidateRange(0, 255)][int]$MaxChannelDelta = 0,
  [string]$Json
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib/ShotHarness.psm1') -Force
$result = Compare-Png -ExpectedPath $Expected -ActualPath $Actual -DiffPath $Diff
if ($Json) {
  $jsonPath = [IO.Path]::GetFullPath($Json)
  [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($jsonPath)) | Out-Null
  [IO.File]::WriteAllText(
    $jsonPath, ($result | ConvertTo-Json -Depth 6) + [Environment]::NewLine,
    [Text.UTF8Encoding]::new($false))
}
$result | Format-List

$passed = $result.dimensions_match -and
  $result.changed_fraction -le $MaxChangedFraction -and
  $result.max_channel_delta -le $MaxChannelDelta
if (-not $passed) {
  throw 'PNG comparison exceeded the requested threshold.'
}
