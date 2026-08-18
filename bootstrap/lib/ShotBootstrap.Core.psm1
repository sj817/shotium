# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#requires -Version 7.0

# Shared plumbing for bootstrap.ps1: run context, logging, checkpoints, guarded
# writes, external process execution and the docs/minimal-checkout.md section 7
# measurement primitives.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:LogLevels = @{ TRACE = 0; INFO = 1; STEP = 1; WARN = 2; ERROR = 3 }

function New-ShotRunContext {
    <#
    .SYNOPSIS
        Builds the immutable run context every other function takes.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$TargetRoot,
        [Parameter(Mandatory)][string]$SolutionName,
        [Parameter(Mandatory)][string]$DepotTools,
        [string]$ReferenceCheckout,
        [Parameter(Mandatory)][string]$PinnedChromiumCommit,
        [Parameter(Mandatory)][string]$PinnedDepotToolsCommit,
        [Parameter(Mandatory)][string]$BootstrapRoot,
        [switch]$DryRun,
        [switch]$Force
    )

    $stateRoot = Join-Path $TargetRoot 'bootstrap-state'
    $context = [ordered]@{
        RunId                  = (Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmmss')
        TargetRoot             = $TargetRoot
        SolutionName           = $SolutionName
        # DEPS hardcodes an 'src/' prefix on every entry, so the solution
        # directory must literally be named 'src' next to .gclient. The existing
        # archaeology checkout works around this with a D:\Github\src symlink;
        # a fresh target has no reason to inherit that indirection.
        SrcRoot                = Join-Path $TargetRoot $SolutionName
        GclientFile            = Join-Path $TargetRoot '.gclient'
        StateRoot              = $stateRoot
        LogRoot                = Join-Path $stateRoot 'logs'
        CheckpointRoot         = Join-Path $stateRoot 'checkpoints'
        MeasurementRoot        = Join-Path $stateRoot 'measurements'
        DepotTools             = $DepotTools
        ReferenceCheckout      = $ReferenceCheckout
        PinnedChromiumCommit   = $PinnedChromiumCommit
        PinnedDepotToolsCommit = $PinnedDepotToolsCommit
        BootstrapRoot          = $BootstrapRoot
        DryRun                 = [bool]$DryRun
        Force                  = [bool]$Force
        TranscriptFile         = $null
        Deviations             = [System.Collections.ArrayList]::new()
    }
    [pscustomobject]$context
}

function Initialize-ShotStateDirectory {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Context)

    foreach ($dir in @($Context.StateRoot, $Context.LogRoot, $Context.CheckpointRoot, $Context.MeasurementRoot)) {
        New-ShotDirectory -Context $Context -Path $dir | Out-Null
    }
    if (-not $Context.DryRun) {
        $Context.TranscriptFile = Join-Path $Context.LogRoot "bootstrap-$($Context.RunId).log"
    }
}

function Write-ShotLog {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Message,
        [ValidateSet('TRACE', 'INFO', 'STEP', 'WARN', 'ERROR')][string]$Level = 'INFO'
    )

    $stamp = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
    $prefix = if ($Context.DryRun) { 'DRYRUN ' } else { '' }
    $line = "[$stamp] $prefix$($Level.PadRight(5)) $Message"

    $color = switch ($Level) {
        'ERROR' { 'Red' }
        'WARN' { 'Yellow' }
        'STEP' { 'Cyan' }
        'TRACE' { 'DarkGray' }
        default { 'Gray' }
    }
    Write-Host $line -ForegroundColor $color

    if ($Context.TranscriptFile) {
        Add-Content -LiteralPath $Context.TranscriptFile -Value $line -Encoding utf8
    }
}

function Get-ShotStringSha256 {
    [CmdletBinding()]
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Text)

    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        ($sha.ComputeHash($bytes) | ForEach-Object { $_.ToString('x2') }) -join ''
    } finally {
        $sha.Dispose()
    }
}

function ConvertTo-ShotCanonicalJson {
    <#
    .SYNOPSIS
        Stable JSON for digesting: key order and formatting must not drift, or
        an unchanged input would produce a new digest and refuse to resume.
    #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][AllowNull()]$InputObject)

    if ($null -eq $InputObject) { return 'null' }
    if ($InputObject -is [string]) { return (ConvertTo-Json $InputObject -Compress) }
    if ($InputObject -is [bool] -or $InputObject -is [int] -or $InputObject -is [long] -or $InputObject -is [double]) {
        return (ConvertTo-Json $InputObject -Compress)
    }
    if ($InputObject -is [System.Collections.IDictionary]) {
        $parts = foreach ($key in ($InputObject.Keys | Sort-Object)) {
            '{0}:{1}' -f (ConvertTo-Json ([string]$key) -Compress), (ConvertTo-ShotCanonicalJson $InputObject[$key])
        }
        return '{' + ($parts -join ',') + '}'
    }
    if ($InputObject -is [System.Collections.IEnumerable]) {
        $parts = foreach ($item in $InputObject) { ConvertTo-ShotCanonicalJson $item }
        return '[' + ($parts -join ',') + ']'
    }
    if ($InputObject -is [pscustomobject]) {
        $map = [ordered]@{}
        foreach ($p in $InputObject.PSObject.Properties) { $map[$p.Name] = $p.Value }
        return ConvertTo-ShotCanonicalJson $map
    }
    ConvertTo-Json ([string]$InputObject) -Compress
}

function ConvertTo-ShotHashtable {
    <#
    .SYNOPSIS
        Normalises a map that may be a hashtable (freshly built lock) or a
        PSCustomObject (lock round-tripped through JSON on a resumed run).
    #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][AllowNull()]$InputObject)

    $out = @{}
    if ($null -eq $InputObject) { return $out }
    if ($InputObject -is [System.Collections.IDictionary]) {
        foreach ($key in $InputObject.Keys) { $out[[string]$key] = $InputObject[$key] }
        return $out
    }
    foreach ($property in $InputObject.PSObject.Properties) { $out[$property.Name] = $property.Value }
    $out
}

function Get-ShotInputDigest {
    [CmdletBinding()]
    param([Parameter(Mandatory)][AllowNull()]$InputObject)
    Get-ShotStringSha256 -Text (ConvertTo-ShotCanonicalJson $InputObject)
}

# --- guarded writes -----------------------------------------------------------
#
# Every filesystem mutation in this bootstrap goes through these three
# functions, and they all refuse to write outside the target root. That is the
# mechanical half of the Phase 0 rule "never touch the archaeology checkout":
# the path guards decide which root is legal once, and nothing downstream can
# quietly write somewhere else.

function Assert-ShotWritablePath {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Path
    )

    $full = [System.IO.Path]::GetFullPath($Path)
    $root = [System.IO.Path]::GetFullPath($Context.TargetRoot)
    $rootWithSep = $root.TrimEnd('\') + '\'
    if ($full -ne $root -and -not $full.StartsWith($rootWithSep, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to write outside the bootstrap target root.`n  target: $root`n  path:   $full"
    }
}

function New-ShotDirectory {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Path
    )

    Assert-ShotWritablePath -Context $Context -Path $Path
    if (Test-Path -LiteralPath $Path) {
        if (-not (Get-Item -LiteralPath $Path -Force).PSIsContainer) {
            throw "Expected a directory but found a file: $Path"
        }
        return $Path
    }
    if ($Context.DryRun) {
        Write-Host "[dryrun] mkdir $Path" -ForegroundColor DarkGray
        return $Path
    }
    New-Item -ItemType Directory -Path $Path -Force | Out-Null
    $Path
}

function Set-ShotFile {
    <#
    .SYNOPSIS
        Writes a bootstrap-owned file. Overwrites only files this bootstrap owns
        (state dir, .gclient, args.gn); it never overwrites checkout content.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Content,
        [switch]$NoClobber
    )

    Assert-ShotWritablePath -Context $Context -Path $Path
    if ($NoClobber -and (Test-Path -LiteralPath $Path)) {
        throw "Refusing to overwrite existing file: $Path"
    }
    if ($Context.DryRun) {
        Write-Host "[dryrun] write $Path ($($Content.Length) chars)" -ForegroundColor DarkGray
        return $Path
    }
    $parent = Split-Path -Parent $Path
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-ShotDirectory -Context $Context -Path $parent | Out-Null
    }
    # No BOM: gclient, GN and git all read these files as plain UTF-8.
    [System.IO.File]::WriteAllText($Path, $Content, [System.Text.UTF8Encoding]::new($false))
    $Path
}

# --- checkpoints --------------------------------------------------------------

function Get-ShotCheckpointPath {
    param([Parameter(Mandatory)]$Context, [Parameter(Mandatory)][string]$Name)
    Join-Path $Context.CheckpointRoot ("{0}.json" -f ($Name -replace '[^A-Za-z0-9._-]', '_'))
}

function Start-ShotCheckpoint {
    <#
    .SYNOPSIS
        Opens (or resumes) a phase/step checkpoint.
    .DESCRIPTION
        Returns an object whose Action is 'Run' or 'Skip'. A completed
        checkpoint with a matching input digest is skipped. A completed
        checkpoint with a *different* digest is a hard error: the inputs moved
        under a partially built tree, and continuing would silently mix two
        configurations. -Force is the only way past it, and it never deletes
        anything -- the operator has to choose a new target directory.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$InputDigest,
        [string]$Description = ''
    )

    $path = Get-ShotCheckpointPath -Context $Context -Name $Name
    $previous = $null
    if (Test-Path -LiteralPath $path) {
        $previous = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    }

    if ($previous) {
        if ($previous.inputDigest -ne $InputDigest -and -not $Context.Force) {
            throw @"
Checkpoint '$Name' was recorded with a different input digest.
  recorded: $($previous.inputDigest)
  current:  $($InputDigest)
The locked inputs changed after this step ran. Re-run in a new empty target
directory, or pass -Force if you have decided the mismatch is intentional.
Nothing was deleted.
"@
        }
        if ($previous.status -eq 'completed' -and -not $Context.Force) {
            Write-ShotLog -Context $Context -Level INFO -Message "checkpoint '$Name' already completed at $($previous.endUtc); skipping."
            return [pscustomobject]@{ Action = 'Skip'; Name = $Name; Path = $path; Record = $previous }
        }
        if ($previous.status -eq 'running') {
            Write-ShotLog -Context $Context -Level WARN -Message "checkpoint '$Name' was interrupted (started $($previous.startUtc)); resuming."
        }
    }

    $record = [ordered]@{
        name        = $Name
        description = $Description
        status      = 'running'
        startUtc    = (Get-Date).ToUniversalTime().ToString('o')
        endUtc      = $null
        exitCode    = $null
        inputDigest = $InputDigest
        runId       = $Context.RunId
        dryRun      = $Context.DryRun
        data        = @{}
    }
    if (-not $Context.DryRun) {
        Set-ShotFile -Context $Context -Path $path -Content (ConvertTo-Json $record -Depth 12) | Out-Null
    }
    [pscustomobject]@{ Action = 'Run'; Name = $Name; Path = $path; Record = $record }
}

function Complete-ShotCheckpoint {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$Checkpoint,
        [int]$ExitCode = 0,
        [ValidateSet('completed', 'failed', 'skipped')][string]$Status = 'completed',
        $Data
    )

    $record = $Checkpoint.Record
    if ($record -is [pscustomobject]) {
        $map = [ordered]@{}
        foreach ($p in $record.PSObject.Properties) { $map[$p.Name] = $p.Value }
        $record = $map
    }
    $record['status'] = $Status
    $record['endUtc'] = (Get-Date).ToUniversalTime().ToString('o')
    $record['exitCode'] = $ExitCode
    if ($PSBoundParameters.ContainsKey('Data')) { $record['data'] = $Data }

    if (-not $Context.DryRun) {
        Set-ShotFile -Context $Context -Path $Checkpoint.Path -Content (ConvertTo-Json $record -Depth 12) | Out-Null
    }
    [pscustomobject]$record
}

# --- external processes -------------------------------------------------------

function Invoke-ShotProcess {
    <#
    .SYNOPSIS
        Runs an external command, tees combined output to a log and times it.
    .DESCRIPTION
        Non-zero exit is fatal unless -AllowNonZeroExit; "fail loudly" is the
        default everywhere in this bootstrap. In dry-run the command is recorded
        and not executed.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$FilePath,
        [string[]]$Arguments = @(),
        [Parameter(Mandatory)][string]$WorkingDirectory,
        [string]$LogFile,
        [hashtable]$Environment,
        [switch]$AllowNonZeroExit,
        [string]$Purpose = ''
    )

    $printable = '{0} {1}' -f $FilePath, ($Arguments -join ' ')
    Write-ShotLog -Context $Context -Level STEP -Message ("run: " + $printable)
    if ($Purpose) { Write-ShotLog -Context $Context -Level TRACE -Message ("why: " + $Purpose) }

    if ($Context.DryRun) {
        return [pscustomobject]@{
            Command = $printable; ExitCode = $null; ElapsedSeconds = 0.0
            LogFile = $LogFile; Executed = $false; WorkingDirectory = $WorkingDirectory
        }
    }

    if (-not (Test-Path -LiteralPath $WorkingDirectory)) {
        throw "Working directory does not exist: $WorkingDirectory"
    }
    if ($LogFile) {
        Assert-ShotWritablePath -Context $Context -Path $LogFile
        $logParent = Split-Path -Parent $LogFile
        if ($logParent -and -not (Test-Path -LiteralPath $logParent)) {
            New-ShotDirectory -Context $Context -Path $logParent | Out-Null
        }
    }

    $saved = @{}
    if ($Environment) {
        foreach ($key in $Environment.Keys) {
            $saved[$key] = [Environment]::GetEnvironmentVariable($key)
            [Environment]::SetEnvironmentVariable($key, $Environment[$key])
        }
    }

    $watch = [System.Diagnostics.Stopwatch]::StartNew()
    $exitCode = $null
    Push-Location -LiteralPath $WorkingDirectory
    try {
        # 2>&1 merges the child's stderr into the success stream so the log is a
        # faithful transcript; gclient and the hooks write progress to stderr.
        # Out-Host, not a bare pipeline: the child's output must reach the
        # console and the log without becoming part of this function's return
        # value, which is the result record callers read the exit code from.
        if ($LogFile) {
            & $FilePath @Arguments 2>&1 | Tee-Object -FilePath $LogFile -Append | Out-Host
        } else {
            & $FilePath @Arguments 2>&1 | Out-Host
        }
        $exitCode = $LASTEXITCODE
    } finally {
        $watch.Stop()
        Pop-Location
        foreach ($key in $saved.Keys) { [Environment]::SetEnvironmentVariable($key, $saved[$key]) }
    }

    $result = [pscustomobject]@{
        Command          = $printable
        ExitCode         = $exitCode
        ElapsedSeconds   = [math]::Round($watch.Elapsed.TotalSeconds, 3)
        LogFile          = $LogFile
        Executed         = $true
        WorkingDirectory = $WorkingDirectory
    }
    Write-ShotLog -Context $Context -Level INFO -Message ("exit {0} after {1}s" -f $exitCode, $result.ElapsedSeconds)

    if ($exitCode -ne 0 -and -not $AllowNonZeroExit) {
        throw "Command failed with exit code ${exitCode}: $printable`nLog: $LogFile"
    }
    $result
}

function Get-ShotFirstLine {
    <#
    .SYNOPSIS
        First line of a native command's output as a trimmed string, '' if none.
    .DESCRIPTION
        A native command that printed nothing returns AutomationNull, and
        casting that to [string] yields $null rather than ''. Calling .Trim() on
        it throws a confusing "cannot call a method on a null-valued expression"
        far from the git invocation that actually failed.
    #>
    [CmdletBinding()]
    param([Parameter()][AllowNull()]$Value)

    if ($null -eq $Value) { return '' }
    $first = @($Value) | Select-Object -First 1
    if ($null -eq $first) { return '' }
    ([string]$first).Trim()
}

function Invoke-ShotGitRead {
    <#
    .SYNOPSIS
        Read-only git query against any repository, including the archaeology
        checkout. Only plumbing that cannot mutate a working tree belongs here.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$RepositoryPath,
        [Parameter(Mandatory)][string[]]$Arguments,
        [switch]$AllowFailure
    )

    $allowed = @('rev-parse', 'cat-file', 'log', 'show', 'status', 'diff', 'diff-tree', 'merge-base',
                 'rev-list', 'config', 'format-patch', 'describe', 'count-objects', 'ls-tree', 'symbolic-ref')
    if ($Arguments.Count -eq 0 -or $allowed -notcontains $Arguments[0]) {
        throw "Invoke-ShotGitRead only permits read-only git subcommands; got '$($Arguments -join ' ')'."
    }

    $output = & git -C $RepositoryPath @Arguments 2>&1
    $code = $LASTEXITCODE
    if ($code -ne 0 -and -not $AllowFailure) {
        throw "git -C $RepositoryPath $($Arguments -join ' ') failed ($code):`n$($output -join "`n")"
    }
    [pscustomobject]@{ ExitCode = $code; Output = @($output | ForEach-Object { [string]$_ }) }
}

# --- measurements (docs/minimal-checkout.md section 7) ------------------------

function Measure-ShotTree {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return [pscustomobject]@{ Path = $Path; Exists = $false; Files = 0; LogicalBytes = [int64]0; LogicalGiB = 0.0 }
    }
    $measure = Get-ChildItem -LiteralPath $Path -Force -File -Recurse -ErrorAction SilentlyContinue |
        Measure-Object -Property Length -Sum
    [pscustomobject]@{
        Path         = $Path
        Exists       = $true
        Files        = $measure.Count
        LogicalBytes = [int64]$measure.Sum
        LogicalGiB   = [math]::Round(($measure.Sum / 1GB), 3)
    }
}

function Get-ShotVolumeSnapshot {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path)

    $letter = ([System.IO.Path]::GetPathRoot([System.IO.Path]::GetFullPath($Path)))[0]
    try {
        $volume = Get-Volume -DriveLetter $letter -ErrorAction Stop
        [pscustomobject]@{
            DriveLetter   = [string]$letter
            Size          = [int64]$volume.Size
            SizeRemaining = [int64]$volume.SizeRemaining
            CapturedUtc   = (Get-Date).ToUniversalTime().ToString('o')
        }
    } catch {
        [pscustomobject]@{ DriveLetter = [string]$letter; Size = $null; SizeRemaining = $null; Error = $_.Exception.Message }
    }
}

function Get-ShotNetworkSnapshot {
    <#
    .SYNOPSIS
        Adapter-level byte counters. These include everything else running on
        the machine; any report built from them has to say so.
    #>
    [CmdletBinding()]
    param()
    try {
        @(Get-NetAdapterStatistics -ErrorAction Stop |
            Select-Object Name, ReceivedBytes, SentBytes |
            ForEach-Object {
                [pscustomobject]@{ Name = $_.Name; ReceivedBytes = [int64]$_.ReceivedBytes; SentBytes = [int64]$_.SentBytes }
            })
    } catch {
        @()
    }
}

function Get-ShotNetworkDelta {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Before, [Parameter(Mandatory)]$After)

    $beforeMap = @{}
    foreach ($a in $Before) { $beforeMap[$a.Name] = $a }
    $rows = foreach ($b in $After) {
        $prior = $beforeMap[$b.Name]
        if (-not $prior) { continue }
        [pscustomobject]@{
            Name              = $b.Name
            ReceivedBytesDelta = [int64]($b.ReceivedBytes - $prior.ReceivedBytes)
            SentBytesDelta     = [int64]($b.SentBytes - $prior.SentBytes)
        }
    }
    $rows = @($rows)
    [pscustomobject]@{
        PerAdapter              = $rows
        TotalReceivedBytesDelta = [int64](($rows | Measure-Object -Property ReceivedBytesDelta -Sum).Sum)
        TotalSentBytesDelta     = [int64](($rows | Measure-Object -Property SentBytesDelta -Sum).Sum)
        Caveat                  = 'Adapter-level counters include unrelated traffic on this host.'
    }
}

function Save-ShotMeasurement {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)]$Data
    )

    $path = Join-Path $Context.MeasurementRoot ("{0}.json" -f ($Name -replace '[^A-Za-z0-9._-]', '_'))
    Set-ShotFile -Context $Context -Path $path -Content (ConvertTo-Json $Data -Depth 12) | Out-Null
    $path
}

function Add-ShotDeviation {
    <#
    .SYNOPSIS
        Records a documented departure from stock upstream Chromium.
    .DESCRIPTION
        Anything this bootstrap changes relative to the pinned upstream tree is
        a deviation and must be visible in the run record. Patching quietly is
        how a "reproducible" checkout stops being reproducible.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Id,
        [Parameter(Mandatory)][string]$Reason,
        [Parameter(Mandatory)][ValidateSet('applied', 'planned', 'skipped', 'required-not-applied')][string]$Status,
        $Detail
    )

    $entry = [ordered]@{
        id         = $Id
        reason     = $Reason
        status     = $Status
        detail     = $Detail
        recordedUtc = (Get-Date).ToUniversalTime().ToString('o')
    }
    [void]$Context.Deviations.Add([pscustomobject]$entry)
    $level = if ($Status -eq 'required-not-applied') { 'WARN' } else { 'INFO' }
    Write-ShotLog -Context $Context -Level $level -Message "deviation [$Status] ${Id}: $Reason"
    [pscustomobject]$entry
}

Export-ModuleMember -Function @(
    'New-ShotRunContext', 'Initialize-ShotStateDirectory', 'Write-ShotLog',
    'Get-ShotStringSha256', 'ConvertTo-ShotCanonicalJson', 'ConvertTo-ShotHashtable', 'Get-ShotInputDigest',
    'Assert-ShotWritablePath', 'New-ShotDirectory', 'Set-ShotFile',
    'Get-ShotCheckpointPath', 'Start-ShotCheckpoint', 'Complete-ShotCheckpoint',
    'Invoke-ShotProcess', 'Invoke-ShotGitRead', 'Get-ShotFirstLine',
    'Measure-ShotTree', 'Get-ShotVolumeSnapshot', 'Get-ShotNetworkSnapshot',
    'Get-ShotNetworkDelta', 'Save-ShotMeasurement', 'Add-ShotDeviation'
)
