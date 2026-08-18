# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

<#
.SYNOPSIS
    Auditable bootstrap for an isolated //shot:shot checkout. Phases 0 and 1.

.DESCRIPTION
    Implements docs/minimal-checkout.md section 6:

      Phase 0  lock the inputs and refuse anything that is not a clean, isolated
               target directory.
      Phase 1  partial clone at the pin, wide source set, checkout_configuration
               = "small" plus the first custom_deps batch, gclient sync, the
               official Windows hooks, then gn gen. It stops at configure: a
               ninja build takes hours and belongs to a separate step.

    The script writes only inside -TargetRoot. It reads the archaeology checkout
    (for the pinned DEPS, the fork commits and the GN args template) with
    read-only git plumbing and never writes to it.

.PARAMETER TargetRoot
    A new or empty directory that will hold .gclient and the src/ solution.

.EXAMPLE
    pwsh -NoProfile -File bootstrap\bootstrap.ps1 -TargetRoot E:\shot -WhatIf

    Runs every guard and builds the full input lock without fetching anything.

.EXAMPLE
    pwsh -NoProfile -File bootstrap\bootstrap.ps1 -TargetRoot E:\shot -Phase 0

    Locks the inputs only. Phase 1 can be run later; it resumes from the
    checkpoints under <TargetRoot>\bootstrap-state.
#>

#requires -Version 7.0

[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)]
    [string]$TargetRoot,

    # Which phases to run. Phase 1 requires a completed Phase 0 lock.
    [ValidateSet(0, 1)]
    [int[]]$Phase = @(0, 1),

    # --- locked inputs -------------------------------------------------------
    [string]$ChromiumCommit = 'c0bba1026178fe2a8b441fead7928b697a801c1e',
    [string]$ChromiumRemote = 'https://chromium.googlesource.com/chromium/src.git',
    [string]$DepotToolsCommit = '13febbee9ece9e03df923f69d540afc63c6db93e',
    [string]$DepotTools,
    [string]$ReferenceCheckout,
    # Tip of the fork work that is replayed on top of the pin. Resolved to a
    # commit hash at lock time so a moving branch cannot change a locked run.
    [string]$ForkRef = 'HEAD',
    [switch]$SkipForkOverlay,

    [string]$SolutionName = 'src',
    [string]$CustomDepsFile,
    [string]$CheckoutConfiguration = 'small',
    [ValidateSet('win')][string]$TargetOs = 'win',
    [ValidateSet('x64')][string]$TargetCpu = 'x64',
    [string]$GnArgsTemplate = '//build/args/shot.gn',
    [string]$GnOutDir = 'out/Shot',
    [string]$GnTarget = '//shot:shot',
    [int]$SyncJobs = 16,

    # --- guard tuning --------------------------------------------------------
    [double]$MinimumFreeGiB = 200,
    [string[]]$ProtectedRoot = @(),
    [switch]$AllowNonEmpty,
    [switch]$AllowAncestorGclient,
    [switch]$AllowDepotToolsDrift,
    [switch]$AllowMissingSdk,

    # --- step selection ------------------------------------------------------
    [switch]$SkipSync,
    [switch]$SkipHooks,
    [switch]$SkipGnGen,
    [switch]$IncludeDepsInLock,

    # Re-run a completed checkpoint. Never deletes anything.
    [switch]$Force,
    # Same effect as -WhatIf; kept as an explicit switch so it survives being
    # dot-sourced from a wrapper that swallows common parameters.
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:BootstrapRoot = $PSScriptRoot
$libRoot = Join-Path $script:BootstrapRoot 'lib'
foreach ($module in @('ShotBootstrap.Core', 'ShotBootstrap.Guard', 'ShotBootstrap.Lock', 'ShotBootstrap.Phase1')) {
    Import-Module (Join-Path $libRoot "$module.psm1") -Force -DisableNameChecking
}

# --- settings -----------------------------------------------------------------

if (-not $ReferenceCheckout) { $ReferenceCheckout = Split-Path -Parent $script:BootstrapRoot }
if (-not $DepotTools) {
    $DepotTools = if ($env:SHOT_DEPOT_TOOLS) { $env:SHOT_DEPOT_TOOLS }
                  else { Join-Path (Split-Path -Parent $ReferenceCheckout) 'depot_tools' }
}
if (-not $CustomDepsFile) { $CustomDepsFile = Join-Path $script:BootstrapRoot 'custom-deps-batch1.txt' }

$dryRun = [bool]$DryRun -or [bool]$WhatIfPreference

$context = New-ShotRunContext -TargetRoot ([System.IO.Path]::GetFullPath($TargetRoot)) `
    -SolutionName $SolutionName -DepotTools $DepotTools -ReferenceCheckout $ReferenceCheckout `
    -PinnedChromiumCommit $ChromiumCommit -PinnedDepotToolsCommit $DepotToolsCommit `
    -BootstrapRoot $script:BootstrapRoot -DryRun:$dryRun -Force:$Force

# The archaeology checkout, the directory above it (which holds the working
# .gclient) and depot_tools are never valid targets. docs/minimal-checkout.md
# section 6 makes this a hard rule, and section 4 explains why it matters for
# results too: a run started next to existing caches cannot measure anything.
$protectedRoots = @($ReferenceCheckout, (Split-Path -Parent $ReferenceCheckout), $DepotTools, $script:BootstrapRoot) + $ProtectedRoot

function Invoke-ShotPhase0 {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Context,
        [Parameter(Mandatory)][hashtable]$Settings,
        [Parameter(Mandatory)][string[]]$ProtectedRoots
    )

    Write-ShotLog -Context $Context -Level STEP -Message 'Phase 0: input lock and safety boundary.'

    $environmentGuards = Assert-ShotHostEnvironment -DepotTools $Settings.DepotTools `
        -PinnedDepotToolsCommit $Settings.DepotToolsCommit -AllowDepotToolsDrift:$Settings.AllowDepotToolsDrift

    $target = Assert-ShotTargetDirectory -TargetRoot $Context.TargetRoot -ForbiddenRoots $ProtectedRoots `
        -MinimumFreeGiB $Settings.MinimumFreeGiB -AllowAncestorGclient:$Settings.AllowAncestorGclient `
        -AllowNonEmpty:$Settings.AllowNonEmpty

    $guards = @($environmentGuards) + @($target.Guards)
    foreach ($guard in $guards) {
        $level = switch ($guard.result) { 'fail' { 'ERROR' } 'warn' { 'WARN' } default { 'TRACE' } }
        Write-ShotLog -Context $Context -Level $level -Message ("guard {0} {1}: {2}" -f $guard.result.ToUpperInvariant(), $guard.id, $guard.detail)
    }

    # Only now, with the target accepted, is anything created.
    New-ShotDirectory -Context $Context -Path $Context.TargetRoot | Out-Null
    Initialize-ShotStateDirectory -Context $Context

    $lock = New-ShotInputLock -Context $Context -Settings $Settings

    $checkpoint = Start-ShotCheckpoint -Context $Context -Name 'phase-0' -InputDigest $lock.inputDigest `
        -Description 'record and verify the locked inputs'

    $lockPath = Join-Path $Context.StateRoot 'input-lock.json'
    if ($checkpoint.Action -eq 'Skip' -and (Test-Path -LiteralPath $lockPath)) {
        $existing = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
        if ($existing.inputDigest -ne $lock.inputDigest) {
            throw "input-lock.json digest $($existing.inputDigest) does not match the recomputed $($lock.inputDigest)."
        }
        Write-ShotLog -Context $Context -Level INFO -Message 'Phase 0 already locked; inputs unchanged.'
        return $existing
    }

    Set-ShotFile -Context $Context -Path $lockPath -Content (ConvertTo-Json $lock -Depth 20) | Out-Null
    Set-ShotFile -Context $Context -Path (Join-Path $Context.StateRoot 'guards.json') `
        -Content (ConvertTo-Json @{ target = $target.ResolvedTarget; protectedRoots = $ProtectedRoots; guards = $guards } -Depth 8) | Out-Null
    Set-ShotFile -Context $Context -Path (Join-Path $Context.StateRoot 'hooks.lock.json') `
        -Content (ConvertTo-Json $lock.hooks -Depth 10) | Out-Null

    Write-ShotLog -Context $Context -Level INFO -Message "chromium      $($lock.chromium.commit)"
    Write-ShotLog -Context $Context -Level INFO -Message "depot_tools   $($lock.depotTools.commit)"
    Write-ShotLog -Context $Context -Level INFO -Message "DEPS sha256   $($lock.deps.sha256)"
    Write-ShotLog -Context $Context -Level INFO -Message "args.gn sha256 $($lock.build.argsGnSha256) (template $($lock.build.argsTemplate) $($lock.build.argsTemplateSha256))"
    Write-ShotLog -Context $Context -Level INFO -Message "target        $($lock.build.targetOs)/$($lock.build.targetCpu), custom_vars checkout_configuration=$($lock.gclient.customVars.checkout_configuration)"
    Write-ShotLog -Context $Context -Level INFO -Message "custom_deps   $(@($lock.gclient.customDeps).Count) paths -> None"
    Write-ShotLog -Context $Context -Level INFO -Message "deps selected $($lock.depsSummary.remaining) = $($lock.depsSummary.git) git + $($lock.depsSummary.cipd) cipd + $($lock.depsSummary.gcs) gcs (matches doc section 7.3)"
    Write-ShotLog -Context $Context -Level INFO -Message "hooks         $($lock.hooks.selected) of $($lock.hooks.total) selected on win/x64"
    Write-ShotLog -Context $Context -Level INFO -Message "input digest  $($lock.inputDigest)"

    Complete-ShotCheckpoint -Context $Context -Checkpoint $checkpoint -ExitCode 0 `
        -Data @{ lock = 'input-lock.json'; guardCount = @($guards).Count } | Out-Null
    $lock
}

# --- run ----------------------------------------------------------------------

$settings = @{
    ChromiumCommit        = $ChromiumCommit
    ChromiumRemote        = $ChromiumRemote
    DepotTools            = $DepotTools
    DepotToolsCommit      = $DepotToolsCommit
    ReferenceCheckout     = $ReferenceCheckout
    ForkRef               = $ForkRef
    SkipForkOverlay       = [bool]$SkipForkOverlay
    SolutionName          = $SolutionName
    CustomDepsFile        = $CustomDepsFile
    CheckoutConfiguration = $CheckoutConfiguration
    TargetOs              = $TargetOs
    TargetCpu             = $TargetCpu
    GnArgsTemplate        = $GnArgsTemplate
    GnOutDir              = $GnOutDir
    GnTarget              = $GnTarget
    SyncJobs              = $SyncJobs
    MinimumFreeGiB        = $MinimumFreeGiB
    AllowNonEmpty         = [bool]$AllowNonEmpty
    AllowAncestorGclient  = [bool]$AllowAncestorGclient
    AllowDepotToolsDrift  = [bool]$AllowDepotToolsDrift
    AllowMissingSdk       = [bool]$AllowMissingSdk
    SkipSync              = [bool]$SkipSync
    SkipHooks             = [bool]$SkipHooks
    SkipGnGen             = [bool]$SkipGnGen
    IncludeDeps           = [bool]$IncludeDepsInLock
    DepsLockScript        = (Join-Path $libRoot 'deps_lock.py')
    Python                = $null
}
$settings.Python = Get-ShotDepotToolsPython -DepotTools $DepotTools

Write-ShotLog -Context $context -Level STEP -Message "shot bootstrap run $($context.RunId) -> $($context.TargetRoot)"
if ($context.DryRun) {
    Write-ShotLog -Context $context -Level WARN -Message 'dry run: guards and the input lock are computed for real; nothing is fetched, written or built.'
}

$lock = $null
if ($Phase -contains 0) {
    $lock = Invoke-ShotPhase0 -Context $context -Settings $settings -ProtectedRoots $protectedRoots
}

if ($Phase -contains 1) {
    if (-not $lock) {
        # Phase 1 on its own still needs the lock; read the one Phase 0 wrote and
        # refuse to guess if it is absent.
        $lockPath = Join-Path $context.StateRoot 'input-lock.json'
        if (-not (Test-Path -LiteralPath $lockPath)) {
            throw "Phase 1 requires a Phase 0 lock at $lockPath. Run -Phase 0 first."
        }
        $lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
        Initialize-ShotStateDirectory -Context $context
        # Containment guards are cheap and are re-run every time: a target that
        # became a symlink into the archaeology checkout between phases must not
        # be written to.
        Assert-ShotTargetDirectory -TargetRoot $context.TargetRoot -ForbiddenRoots $protectedRoots `
            -MinimumFreeGiB $MinimumFreeGiB -AllowAncestorGclient:$AllowAncestorGclient `
            -AllowNonEmpty:$AllowNonEmpty | Out-Null
    }
    Invoke-ShotPhase1 -Context $context -Lock $lock -Settings $settings | Out-Null
}

Write-ShotLog -Context $context -Level STEP -Message 'done.'
if ($context.Deviations.Count -gt 0) {
    Write-ShotLog -Context $context -Level WARN -Message "$($context.Deviations.Count) deviation(s) recorded; see bootstrap-state/measurements/phase-1-report.json."
}
