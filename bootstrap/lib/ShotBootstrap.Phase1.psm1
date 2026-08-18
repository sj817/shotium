# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#requires -Version 7.0

# Phase 1: establish a buildable baseline with the official tools.
#
# docs/minimal-checkout.md section 6 Phase 1, with one rule from section 3 taken
# seriously: the source set stays deliberately wide. Sparse checkout is NOT
# configured here. Section 3 records what happened last time -- main-repo sparse
# and gclient DEPS are independent pruning layers, sparse rules do not reach
# into ANGLE/Dawn/DevTools, and GN needs //pdf, //printing, //device,
# //third_party/inspector_protocol and friends that are easy to guess wrong. A
# narrow checkout here would produce a GN graph that cannot be trusted, and the
# whole point of Phase 1 is to produce one that can.
#
# Phase 1 stops at gn gen. Ninja is Phase 1 step 6 in the doc but takes hours;
# it is driven separately so a configure failure is found in minutes.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-ShotInstalledWindowsSdk {
    <#
    .SYNOPSIS
        Windows 10/11 SDK versions present on this host.
    #>
    [CmdletBinding()]
    param()

    $roots = @()
    try {
        $key = Get-ItemProperty -Path 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots' -ErrorAction Stop
        if ($key.KitsRoot10) { $roots += $key.KitsRoot10 }
    } catch { }
    $roots += 'C:\Program Files (x86)\Windows Kits\10\'
    $versions = foreach ($root in ($roots | Select-Object -Unique)) {
        $include = Join-Path $root 'Include'
        if (-not (Test-Path -LiteralPath $include)) { continue }
        Get-ChildItem -LiteralPath $include -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } | ForEach-Object { $_.Name }
    }
    @($versions | Sort-Object -Unique)
}

function Assert-ShotWindowsSdkDeviation {
    <#
    .SYNOPSIS
        Checks that the SDK version the tree asks for is actually installed.
    .DESCRIPTION
        The pinned revision wants VS 2026 with SDK 10.0.28000.2270, which no
        machine in this project had when the archaeology round started; the fork
        carried a commit lowering SDK_VERSION in build/vs_toolchain.py and
        build/toolchain/win/setup_toolchain.py to a version that was installed.
        Both the upstream value and the lowered one are deviations from *some*
        baseline, so neither is assumed here: the requested version is read from
        the tree, the installed versions are read from the host, and the two are
        compared. A mismatch fails now instead of halfway through the
        win_toolchain hook, and a match is still recorded so the report says
        which SDK the numbers came from.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$SrcRoot,
        [switch]$AllowMissingSdk
    )

    $files = @('build\vs_toolchain.py', 'build\toolchain\win\setup_toolchain.py')
    $requested = [System.Collections.ArrayList]::new()
    foreach ($relative in $files) {
        $path = Join-Path $SrcRoot $relative
        if (-not (Test-Path -LiteralPath $path)) { continue }
        foreach ($match in (Select-String -LiteralPath $path -Pattern "SDK_VERSION\s*=\s*['`"]([0-9.]+)['`"]" -AllMatches)) {
            foreach ($m in $match.Matches) { [void]$requested.Add([pscustomobject]@{ file = $relative; version = $m.Groups[1].Value }) }
        }
    }

    $installed = Get-ShotInstalledWindowsSdk
    $wanted = @($requested | ForEach-Object { $_.version } | Sort-Object -Unique)
    $missing = @($wanted | Where-Object { $installed -notcontains $_ })

    $detail = [ordered]@{
        requested = @($requested)
        installed = $installed
        missing   = $missing
    }

    if ($missing.Count -gt 0) {
        $message = "The tree pins Windows SDK $($missing -join ', ') but this host has $($installed -join ', ')."
        if (-not $AllowMissingSdk) {
            throw @"
$message
The win_toolchain hook and every subsequent compile would fail. Either install
the SDK, or carry the fork commit that lowers SDK_VERSION in
build/vs_toolchain.py and build/toolchain/win/setup_toolchain.py (see the fork
overlay), or re-run with -AllowMissingSdk and accept the recorded deviation.
"@
        }
        Add-ShotDeviation -Context $Context -Id 'win-sdk-version' -Status 'required-not-applied' `
            -Reason $message -Detail $detail | Out-Null
    } else {
        Add-ShotDeviation -Context $Context -Id 'win-sdk-version' -Status 'applied' `
            -Reason "Tree requests Windows SDK $($wanted -join ', '); this host provides $($installed -join ', '). Recorded because the SDK version is part of the toolchain the measurements describe." `
            -Detail $detail | Out-Null
    }
    [pscustomobject]$detail
}

function Invoke-ShotClone {
    <#
    .SYNOPSIS
        Partial clone of Chromium at the pinned commit. No sparse checkout.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$Lock,
        [Parameter(Mandatory)][hashtable]$Settings
    )

    $src = $Context.SrcRoot
    $gitDir = Join-Path $src '.git'

    if (Test-Path -LiteralPath $gitDir) {
        # Resume path. Verify rather than re-clone; never delete.
        $remote = Get-ShotFirstLine (& git -C $src config --get remote.origin.url 2>$null)
        if ($remote -ne $Lock.chromium.remote) {
            throw "Existing checkout at $src has remote '$remote', expected '$($Lock.chromium.remote)'. Nothing was deleted; choose a different target."
        }
        Write-ShotLog -Context $Context -Level INFO -Message "existing clone found at $src; verifying instead of re-cloning."
    } else {
        if ((Test-Path -LiteralPath $src) -and @(Get-ChildItem -LiteralPath $src -Force).Count -gt 0) {
            throw "$src exists and is not empty but has no .git. Refusing to clone into it."
        }
        Invoke-ShotProcess -Context $Context -FilePath 'git' -WorkingDirectory $Context.TargetRoot `
            -LogFile (Join-Path $Context.LogRoot 'clone.log') `
            -Purpose 'blob:none keeps history metadata but defers file contents until checkout' `
            -Arguments @('clone', '--filter=blob:none', '--no-checkout', '--progress',
                         $Lock.chromium.remote, $src) | Out-Null
    }

    if (-not $Context.DryRun) {
        & git -C $src cat-file -e "$($Lock.chromium.commit)^{commit}" 2>$null
        if ($LASTEXITCODE -ne 0) {
            Invoke-ShotProcess -Context $Context -FilePath 'git' -WorkingDirectory $src `
                -LogFile (Join-Path $Context.LogRoot 'clone.log') `
                -Purpose 'the pin may not be on a fetched branch tip' `
                -Arguments @('fetch', '--filter=blob:none', '--progress', 'origin', $Lock.chromium.commit) | Out-Null
        }
    }

    Invoke-ShotProcess -Context $Context -FilePath 'git' -WorkingDirectory $src `
        -LogFile (Join-Path $Context.LogRoot 'clone.log') `
        -Purpose 'detached at the pin; no branch means no accidental fast-forward' `
        -Arguments @('checkout', '--detach', $Lock.chromium.commit) | Out-Null

    if (-not $Context.DryRun) {
        $sparse = Get-ShotFirstLine (& git -C $src config --get core.sparseCheckout 2>$null)
        if ($sparse -eq 'true') {
            throw "core.sparseCheckout is enabled in $src. Phase 1 requires the wide source set (doc section 3)."
        }
    }

    [pscustomobject]@{
        SrcRoot        = $src
        Commit         = $Lock.chromium.commit
        SparseCheckout = $false
        SparseRationale = 'docs/minimal-checkout.md section 3: sparse before a trusted GN graph produced a checkout that could not configure.'
    }
}

function Invoke-ShotForkOverlay {
    <#
    .SYNOPSIS
        Replays the fork's commits on top of the pinned upstream commit.
    .DESCRIPTION
        Applied as individual patches exported from the reference checkout, one
        deviation record per commit. Patches rather than a git fetch because the
        reference checkout is a shallow, blob-filtered clone whose pinned commit
        is the graft boundary: it can produce diffs, but it cannot reliably serve
        a fetch.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$Lock,
        [Parameter(Mandatory)][hashtable]$Settings
    )

    if (-not $Lock.forkOverlay.enabled) {
        Add-ShotDeviation -Context $Context -Id 'fork-overlay' -Status 'skipped' `
            -Reason 'Overlay disabled: the checkout is stock upstream at the pin. //shot:shot does not exist there, so gn gen has nothing to configure.' | Out-Null
        return $null
    }

    $patchDir = Join-Path $Context.StateRoot 'deviations'
    New-ShotDirectory -Context $Context -Path $patchDir | Out-Null

    if (-not $Context.DryRun) {
        $existing = @(Get-ChildItem -LiteralPath $patchDir -Filter '*.patch' -File -ErrorAction SilentlyContinue)
        if ($existing.Count -eq 0) {
            $out = & git -C $Lock.chromium.reference format-patch --binary --no-signature `
                -o $patchDir "$($Lock.chromium.commit)..$($Lock.forkOverlay.commit)" 2>&1
            if ($LASTEXITCODE -ne 0) {
                throw "Could not export the fork overlay from $($Lock.chromium.reference):`n$($out -join "`n")"
            }
        } else {
            Write-ShotLog -Context $Context -Level INFO -Message "reusing $($existing.Count) exported patches in $patchDir."
        }
    }

    $patches = @(Get-ChildItem -LiteralPath $patchDir -Filter '*.patch' -File -ErrorAction SilentlyContinue | Sort-Object Name)
    if (-not $Context.DryRun -and $patches.Count -ne $Lock.forkOverlay.commits.Count) {
        throw "Expected $($Lock.forkOverlay.commits.Count) overlay patches, found $($patches.Count) in $patchDir."
    }

    # Already applied? git am on top of an applied series would fail; ancestry of
    # the pin plus a non-empty diff is enough to detect the resumed case.
    $alreadyApplied = $false
    if (-not $Context.DryRun) {
        $head = Get-ShotFirstLine (& git -C $Context.SrcRoot rev-parse HEAD)
        $distance = Get-ShotFirstLine (& git -C $Context.SrcRoot rev-list --count "$($Lock.chromium.commit)..$head" 2>$null)
        if ($distance -and [int]$distance -ge $patches.Count -and [int]$distance -gt 0) {
            $alreadyApplied = $true
            Write-ShotLog -Context $Context -Level INFO -Message "overlay already applied ($distance commits above the pin); skipping git am."
        }
    }

    if (-not $alreadyApplied) {
        Invoke-ShotProcess -Context $Context -FilePath 'git' -WorkingDirectory $Context.SrcRoot `
            -LogFile (Join-Path $Context.LogRoot 'overlay.log') `
            -Purpose 'replay the fork commits; identity is fixed so a machine without git identity still applies them' `
            -Arguments (@('-c', 'user.name=shot-bootstrap', '-c', 'user.email=shot-bootstrap@localhost',
                          'am', '--keep-non-patch', '--3way') + @($patches | ForEach-Object { $_.FullName })) | Out-Null
    }

    foreach ($commit in $Lock.forkOverlay.commits) {
        Add-ShotDeviation -Context $Context -Id ("fork-commit:" + $commit.commit.Substring(0, 12)) -Status 'applied' `
            -Reason $commit.subject -Detail @{ commit = $commit.commit; files = $commit.files } | Out-Null
    }

    [pscustomobject]@{
        PatchDirectory = $patchDir
        PatchCount     = $patches.Count
        Patches        = @($patches | ForEach-Object {
            [ordered]@{ name = $_.Name; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
        })
    }
}

function Write-ShotGclientFile {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$Lock
    )

    $customVars = ConvertTo-ShotHashtable $Lock.gclient.customVars
    $content = New-ShotGclientContent -SolutionName $Lock.gclient.solutionName -Url $Lock.gclient.url `
        -CustomVars $customVars -CustomDeps @($Lock.gclient.customDeps) -TargetOs $Lock.build.targetOs

    $sha = Get-ShotStringSha256 -Text $content
    if ($sha -ne $Lock.gclient.contentSha256) {
        throw "Rendered .gclient does not match the locked hash ($sha vs $($Lock.gclient.contentSha256))."
    }
    Set-ShotFile -Context $Context -Path $Context.GclientFile -Content $content | Out-Null
    Write-ShotLog -Context $Context -Level INFO -Message ".gclient written with checkout_configuration=$($customVars.checkout_configuration) and $(@($Lock.gclient.customDeps).Count) custom_deps null entries."
    [pscustomobject]@{ Path = $Context.GclientFile; Sha256 = $sha; CustomDepsCount = @($Lock.gclient.customDeps).Count }
}

function Get-ShotToolEnvironment {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Context)

    @{
        DEPOT_TOOLS_UPDATE        = '0'
        DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
        DEPOT_TOOLS_METRICS       = '0'
        PATH                      = ($Context.DepotTools + ';' + $env:PATH)
    }
}

function Invoke-ShotGclientStep {
    <#
    .SYNOPSIS
        Runs one gclient sub-command with the section 7.2 measurements around it.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][string]$Python,
        [Parameter(Mandatory)][string[]]$GclientArguments,
        [Parameter(Mandatory)][string]$LogName,
        [Parameter(Mandatory)][string]$MeasurementName,
        [switch]$AllowNonZeroExit
    )

    $netBefore = Get-ShotNetworkSnapshot
    $volumeBefore = Get-ShotVolumeSnapshot -Path $Context.TargetRoot

    $result = Invoke-ShotProcess -Context $Context -FilePath $Python `
        -Arguments (@((Join-Path $Context.DepotTools 'gclient.py')) + $GclientArguments) `
        -WorkingDirectory $Context.TargetRoot `
        -LogFile (Join-Path $Context.LogRoot $LogName) `
        -Environment (Get-ShotToolEnvironment -Context $Context) `
        -AllowNonZeroExit:$AllowNonZeroExit `
        -Purpose 'invoked through the pinned depot_tools gclient.py, not a wrapper on PATH'

    $netAfter = Get-ShotNetworkSnapshot
    $volumeAfter = Get-ShotVolumeSnapshot -Path $Context.TargetRoot

    $measurement = [ordered]@{
        step           = $MeasurementName
        command        = $result.Command
        exitCode       = $result.ExitCode
        elapsedSeconds = $result.ElapsedSeconds
        network        = if ($Context.DryRun) { $null } else { Get-ShotNetworkDelta -Before $netBefore -After $netAfter }
        volumeBefore   = $volumeBefore
        volumeAfter    = $volumeAfter
        tree           = if ($Context.DryRun) { $null } else { Measure-ShotTree -Path $Context.SrcRoot }
        gitStore       = if ($Context.DryRun) { $null } else { Measure-ShotTree -Path (Join-Path $Context.SrcRoot '.git') }
        countObjects   = if ($Context.DryRun) { @() } else {
            @((Invoke-ShotGitRead -RepositoryPath $Context.SrcRoot -Arguments @('count-objects', '-vH') -AllowFailure).Output)
        }
        capturedUtc    = (Get-Date).ToUniversalTime().ToString('o')
    }
    Save-ShotMeasurement -Context $Context -Name $MeasurementName -Data $measurement | Out-Null
    [pscustomobject]@{ Result = $result; Measurement = [pscustomobject]$measurement }
}

function Invoke-ShotGnGen {
    <#
    .SYNOPSIS
        Writes args.gn and runs gn gen. Reports; does not build.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$Lock
    )

    $outDir = Join-Path $Context.SrcRoot ($Lock.build.outDir -replace '/', '\')
    New-ShotDirectory -Context $Context -Path $outDir | Out-Null

    $argsPath = Join-Path $outDir 'args.gn'
    Set-ShotFile -Context $Context -Path $argsPath -Content $Lock.build.argsGnContent | Out-Null

    if (-not $Context.DryRun) {
        $sha = (Get-FileHash -LiteralPath $argsPath -Algorithm SHA256).Hash.ToLowerInvariant()
        $expected = Get-ShotStringSha256 -Text $Lock.build.argsGnContent
        if ($sha -ne $expected) {
            # Written and read back differ only if something re-encoded the file.
            throw "args.gn on disk hashes $sha but the locked content hashes $expected."
        }
    }

    $gn = Join-Path $Context.SrcRoot 'buildtools\win\gn.exe'
    if (-not $Context.DryRun -and -not (Test-Path -LiteralPath $gn)) {
        throw "gn.exe is missing at $gn. buildtools/win comes from DEPS; sync did not complete."
    }

    $watch = [System.Diagnostics.Stopwatch]::StartNew()
    $result = Invoke-ShotProcess -Context $Context -FilePath $gn `
        -Arguments @('gen', ($Lock.build.outDir -replace '/', '\')) `
        -WorkingDirectory $Context.SrcRoot `
        -LogFile (Join-Path $Context.LogRoot 'gn-gen.log') `
        -Environment (Get-ShotToolEnvironment -Context $Context) `
        -AllowNonZeroExit `
        -Purpose 'configure only; the ninja build is deliberately not started here'
    $watch.Stop()

    $measurement = [ordered]@{
        step           = 'gn-gen'
        command        = $result.Command
        exitCode       = $result.ExitCode
        elapsedSeconds = $result.ElapsedSeconds
        outDir         = $outDir
        argsGnSha256   = $Lock.build.argsGnSha256
        gnCheckRun     = $false
        gnCheckNote    = 'gn gen does not run header checking. gn check must be invoked explicitly and is not part of Phase 1.'
        ninjaRun       = $false
        ninjaNote      = 'Phase 1 stops at configure; //shot:shot is built separately because a full build takes hours.'
        capturedUtc    = (Get-Date).ToUniversalTime().ToString('o')
    }
    Save-ShotMeasurement -Context $Context -Name 'gn-gen' -Data $measurement | Out-Null

    if ($result.ExitCode -eq 0) {
        Write-ShotLog -Context $Context -Level INFO -Message "gn gen succeeded for $($Lock.build.outDir)."
    } elseif ($result.Executed) {
        Write-ShotLog -Context $Context -Level ERROR -Message "gn gen FAILED (exit $($result.ExitCode)); see $($result.LogFile)."
    }
    [pscustomobject]@{ Result = $result; Measurement = [pscustomobject]$measurement }
}

function Invoke-ShotPhase1 {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)]$Lock,
        [Parameter(Mandatory)][hashtable]$Settings
    )

    Write-ShotLog -Context $Context -Level STEP -Message 'Phase 1: buildable baseline with the official tools.'

    $checkpoint = Start-ShotCheckpoint -Context $Context -Name 'phase-1' -InputDigest $Lock.inputDigest `
        -Description 'partial clone, fork overlay, .gclient, gclient sync, official hooks, gn gen'
    if ($checkpoint.Action -eq 'Skip') { return $checkpoint.Record }

    $report = [ordered]@{
        phase       = 1
        inputDigest = $Lock.inputDigest
        startedUtc  = (Get-Date).ToUniversalTime().ToString('o')
        steps       = [ordered]@{}
    }

    try {
        # --- 1.1 partial clone -----------------------------------------------
        $step = Start-ShotCheckpoint -Context $Context -Name 'phase-1.1-clone' -InputDigest $Lock.inputDigest -Description 'partial clone at the pinned commit'
        if ($step.Action -eq 'Run') {
            $clone = Invoke-ShotClone -Context $Context -Lock $Lock -Settings $Settings
            $report.steps['clone'] = $clone
            Complete-ShotCheckpoint -Context $Context -Checkpoint $step -ExitCode 0 -Data $clone | Out-Null
        }

        # --- 1.2 fork overlay -------------------------------------------------
        $step = Start-ShotCheckpoint -Context $Context -Name 'phase-1.2-overlay' -InputDigest $Lock.inputDigest -Description 'replay fork commits and record deviations'
        if ($step.Action -eq 'Run') {
            $overlay = Invoke-ShotForkOverlay -Context $Context -Lock $Lock -Settings $Settings
            $report.steps['overlay'] = $overlay
            Complete-ShotCheckpoint -Context $Context -Checkpoint $step -ExitCode 0 -Data $overlay | Out-Null
        }

        if (-not $Context.DryRun) {
            Assert-ShotCheckoutMatchesLock -Context $Context -Lock $Lock -SrcRoot $Context.SrcRoot `
                -ExpectForkOverlay:([bool]$Lock.forkOverlay.enabled) | Out-Null
            $report.steps['sdk'] = Assert-ShotWindowsSdkDeviation -Context $Context -SrcRoot $Context.SrcRoot `
                -AllowMissingSdk:$Settings.AllowMissingSdk
        }

        # --- 1.3 .gclient ------------------------------------------------------
        $step = Start-ShotCheckpoint -Context $Context -Name 'phase-1.3-gclient-config' -InputDigest $Lock.inputDigest -Description 'write .gclient with checkout_configuration=small and the first custom_deps batch'
        if ($step.Action -eq 'Run') {
            $gclient = Write-ShotGclientFile -Context $Context -Lock $Lock
            $report.steps['gclientConfig'] = $gclient
            Complete-ShotCheckpoint -Context $Context -Checkpoint $step -ExitCode 0 -Data $gclient | Out-Null
        }

        # --- 1.4 gclient sync --------------------------------------------------
        if ($Settings.SkipSync) {
            Write-ShotLog -Context $Context -Level WARN -Message '-SkipSync: no dependencies were fetched.'
        } else {
            $step = Start-ShotCheckpoint -Context $Context -Name 'phase-1.4-sync' -InputDigest $Lock.inputDigest -Description 'gclient sync --no-history --nohooks'
            if ($step.Action -eq 'Run') {
                # --nohooks separates dependency fetching from hook execution so
                # a hook failure does not look like a sync failure. Section 2 of
                # the doc is emphatic that --nohooks does not stop GCS/CIPD
                # downloads: they are deps, not hooks.
                $sync = Invoke-ShotGclientStep -Context $Context -Python $Settings.Python -MeasurementName 'sync' -LogName 'sync.log' `
                    -GclientArguments @('sync', '--no-history', '--nohooks', '--jobs', [string]$Settings.SyncJobs)
                $report.steps['sync'] = $sync.Measurement
                Complete-ShotCheckpoint -Context $Context -Checkpoint $step -ExitCode ([int]($sync.Result.ExitCode ?? 0)) -Data $sync.Measurement | Out-Null
            }
        }

        # --- 1.5 official hooks -------------------------------------------------
        if ($Settings.SkipHooks) {
            Write-ShotLog -Context $Context -Level WARN -Message '-SkipHooks: the official Windows hooks did not run; gn gen will almost certainly fail.'
        } else {
            $step = Start-ShotCheckpoint -Context $Context -Name 'phase-1.5-hooks' -InputDigest $Lock.inputDigest -Description "run the $($Lock.hooks.selected) official Windows hooks"
            if ($step.Action -eq 'Run') {
                Write-ShotLog -Context $Context -Level INFO -Message ("hooks expected in order: " + (@($Lock.hooks.entries | ForEach-Object { $_.name }) -join ', '))
                # Round one runs the stock hook set unmodified. Replacing it with
                # an allowlist is Phase 3, and it is only meaningful once the
                # stock set is known to succeed against the pruned custom_deps.
                $hooks = Invoke-ShotGclientStep -Context $Context -Python $Settings.Python -MeasurementName 'hooks' -LogName 'hooks.log' `
                    -GclientArguments @('runhooks', '--jobs', [string]$Settings.SyncJobs)
                $report.steps['hooks'] = $hooks.Measurement
                Complete-ShotCheckpoint -Context $Context -Checkpoint $step -ExitCode ([int]($hooks.Result.ExitCode ?? 0)) -Data $hooks.Measurement | Out-Null
            }
        }

        # --- 1.6 gn gen ---------------------------------------------------------
        if ($Settings.SkipGnGen) {
            Write-ShotLog -Context $Context -Level WARN -Message '-SkipGnGen: configure was not attempted.'
        } else {
            $step = Start-ShotCheckpoint -Context $Context -Name 'phase-1.6-gn-gen' -InputDigest $Lock.inputDigest -Description 'gn gen only; no ninja'
            if ($step.Action -eq 'Run') {
                $gn = Invoke-ShotGnGen -Context $Context -Lock $Lock
                $report.steps['gnGen'] = $gn.Measurement
                $status = if (($gn.Result.ExitCode ?? 0) -eq 0) { 'completed' } else { 'failed' }
                Complete-ShotCheckpoint -Context $Context -Checkpoint $step -ExitCode ([int]($gn.Result.ExitCode ?? 0)) -Status $status -Data $gn.Measurement | Out-Null
            }
        }

        $report['deviations'] = @($Context.Deviations)
        $report['finishedUtc'] = (Get-Date).ToUniversalTime().ToString('o')
        $report['finalTree'] = if ($Context.DryRun) { $null } else { Measure-ShotTree -Path $Context.SrcRoot }
        Save-ShotMeasurement -Context $Context -Name 'phase-1-report' -Data $report | Out-Null
        Complete-ShotCheckpoint -Context $Context -Checkpoint $checkpoint -ExitCode 0 -Data @{ report = 'measurements/phase-1-report.json' } | Out-Null
    } catch {
        Complete-ShotCheckpoint -Context $Context -Checkpoint $checkpoint -ExitCode 1 -Status 'failed' -Data @{ error = $_.Exception.Message } | Out-Null
        throw
    }

    [pscustomobject]$report
}

Export-ModuleMember -Function @(
    'Get-ShotInstalledWindowsSdk', 'Assert-ShotWindowsSdkDeviation', 'Invoke-ShotClone',
    'Invoke-ShotForkOverlay', 'Write-ShotGclientFile', 'Get-ShotToolEnvironment',
    'Invoke-ShotGclientStep', 'Invoke-ShotGnGen', 'Invoke-ShotPhase1'
)
