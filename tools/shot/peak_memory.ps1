# Runs shotium.exe with the given arguments and prints peak working set and
# peak private bytes, sampled every 2 ms. Usage:
#   pwsh peak.ps1 <label> <exe> <args...>
param([string]$Label, [string]$Exe, [Parameter(ValueFromRemainingArguments = $true)][string[]]$Args)
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $Exe
foreach ($a in $Args) { $psi.ArgumentList.Add($a) }
$psi.UseShellExecute = $false
$psi.RedirectStandardError = $true
$psi.RedirectStandardOutput = $true
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$p = [System.Diagnostics.Process]::Start($psi)
$peakWs = 0; $peakPriv = 0
$errTask = $p.StandardError.ReadToEndAsync()
$outTask = $p.StandardOutput.ReadToEndAsync()
while (-not $p.HasExited) {
  try {
    $p.Refresh()
    if ($p.PeakWorkingSet64 -gt $peakWs) { $peakWs = $p.PeakWorkingSet64 }
    if ($p.PrivateMemorySize64 -gt $peakPriv) { $peakPriv = $p.PrivateMemorySize64 }
  } catch {}
  Start-Sleep -Milliseconds 2
}
$sw.Stop()
$err = $errTask.Result
$out = $outTask.Result
$lines = ($err -split "`n") | Where-Object { $_ -match "shot: (mem|banded|raster)|ERROR|error" } | ForEach-Object { $_.Trim() }
"{0,-28} peak_ws={1,6:N0} MB  peak_private={2,6:N0} MB  wall={3,6:N0} ms  exit={4}" -f $Label, ($peakWs / 1MB), ($peakPriv / 1MB), $sw.ElapsedMilliseconds, $p.ExitCode
if ($env:PEAK_VERBOSE) { $lines | ForEach-Object { "    $_" } }
