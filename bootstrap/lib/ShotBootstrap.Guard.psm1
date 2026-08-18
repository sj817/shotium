# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#requires -Version 7.0

# Phase 0 safety boundary.
#
# docs/minimal-checkout.md section 6 states the rules in one paragraph; this
# module is that paragraph made executable. Two facts about this host drive most
# of it: the working .gclient lives at D:\Github\.gclient (one level *above* the
# checkout, because DEPS hardcodes an 'src/' prefix), and D:\Github\src is a
# symlink to D:\Github\chromium. A path guard that compares strings without
# resolving reparse points would happily accept 'D:\Github\src\out\isolated' as
# a fresh target and start writing into the archaeology checkout.
#
# Every guard returns a record instead of only throwing, so the Phase 0 JSON
# shows which guards ran and what they saw.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ShotRealPath {
    <#
    .SYNOPSIS
        Absolute path with every existing component's symlink/junction resolved.
    .DESCRIPTION
        Resolves component by component from the drive root down, so an
        intermediate link (D:\Github\src) is followed even when the leaf does
        not exist yet. Resolve-Path cannot be used: it fails on missing paths,
        and its -Relative/provider forms return provider paths.
    #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { throw 'Resolve-ShotRealPath: empty path.' }
    if (-not [System.IO.Path]::IsPathRooted($Path)) {
        $Path = Join-Path (Get-Location).ProviderPath $Path
    }
    $full = [System.IO.Path]::GetFullPath($Path)

    $root = [System.IO.Path]::GetPathRoot($full)
    $rest = $full.Substring($root.Length)
    $current = $root
    foreach ($part in ($rest -split '\\' | Where-Object { $_ -ne '' })) {
        $current = [System.IO.Path]::Combine($current, $part)
        if (-not (Test-Path -LiteralPath $current)) { continue }
        # Follow a chain of links; the depth cap turns a link loop into an
        # error rather than a hang.
        for ($depth = 0; $depth -lt 16; $depth++) {
            $target = $null
            try { $target = [System.IO.Directory]::ResolveLinkTarget($current, $false) } catch { $target = $null }
            if (-not $target) { break }
            $resolved = $target.FullName
            if (-not [System.IO.Path]::IsPathRooted($resolved)) {
                $resolved = [System.IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $current) $resolved))
            }
            if ($resolved -eq $current) { break }
            $current = $resolved
            if ($depth -eq 15) { throw "Too many symlink hops resolving: $Path" }
        }
    }
    [System.IO.Path]::GetFullPath($current).TrimEnd('\')
}

function Test-ShotPathContains {
    <#
    .SYNOPSIS
        True when $Parent is $Child or an ancestor of it, after link resolution.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Parent,
        [Parameter(Mandatory)][string]$Child
    )

    $p = (Resolve-ShotRealPath $Parent).TrimEnd('\')
    $c = (Resolve-ShotRealPath $Child).TrimEnd('\')
    if ($p -eq $c) { return $true }
    $c.StartsWith($p + '\', [System.StringComparison]::OrdinalIgnoreCase)
}

function New-ShotGuardResult {
    param(
        [Parameter(Mandatory)][string]$Id,
        [Parameter(Mandatory)][string]$Description,
        [Parameter(Mandatory)][ValidateSet('pass', 'warn', 'fail')][string]$Result,
        [string]$Detail = ''
    )
    [pscustomobject]@{ id = $Id; description = $Description; result = $Result; detail = $Detail }
}

function Assert-ShotHostEnvironment {
    <#
    .SYNOPSIS
        Verifies the tools the lock names, and pins depot_tools' own behaviour.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$DepotTools,
        [Parameter(Mandatory)][string]$PinnedDepotToolsCommit,
        [switch]$AllowDepotToolsDrift
    )

    $results = [System.Collections.ArrayList]::new()

    if ($PSVersionTable.PSVersion.Major -lt 7) {
        throw "PowerShell 7+ (pwsh) is required; running $($PSVersionTable.PSVersion)."
    }
    [void]$results.Add((New-ShotGuardResult -Id 'ENV-PWSH' -Description 'PowerShell 7+' -Result 'pass' -Detail "$($PSVersionTable.PSVersion)"))

    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) { throw 'git was not found on PATH.' }
    [void]$results.Add((New-ShotGuardResult -Id 'ENV-GIT' -Description 'git on PATH' -Result 'pass' -Detail $git.Source))

    if (-not (Test-Path -LiteralPath (Join-Path $DepotTools 'gclient.py'))) {
        throw "depot_tools does not look like a depot_tools checkout: $DepotTools"
    }
    $head = (& git -C $DepotTools rev-parse HEAD 2>&1)
    if ($LASTEXITCODE -ne 0) { throw "depot_tools is not a git repository: $DepotTools" }
    $head = ([string]$head).Trim()
    if ($head -ne $PinnedDepotToolsCommit) {
        $message = "depot_tools is at $head but the lock pins $PinnedDepotToolsCommit."
        if (-not $AllowDepotToolsDrift) {
            throw "$message`nA different depot_tools evaluates DEPS differently and invalidates every number in the lock. Fix the checkout or pass -AllowDepotToolsDrift and accept the recorded deviation."
        }
        [void]$results.Add((New-ShotGuardResult -Id 'ENV-DEPOT-TOOLS-COMMIT' -Description 'depot_tools pinned commit' -Result 'warn' -Detail $message))
    } else {
        [void]$results.Add((New-ShotGuardResult -Id 'ENV-DEPOT-TOOLS-COMMIT' -Description 'depot_tools pinned commit' -Result 'pass' -Detail $head))
    }

    # DEPOT_TOOLS_UPDATE=0 keeps depot_tools from self-updating out from under
    # the pin. DEPOT_TOOLS_WIN_TOOLCHAIN=0 tells the Windows hooks to use the
    # locally installed Visual Studio instead of Google's internal toolchain
    # package, which is not fetchable outside Google.
    [Environment]::SetEnvironmentVariable('DEPOT_TOOLS_UPDATE', '0')
    [Environment]::SetEnvironmentVariable('DEPOT_TOOLS_WIN_TOOLCHAIN', '0')
    [void]$results.Add((New-ShotGuardResult -Id 'ENV-DEPOT-TOOLS-FLAGS' -Description 'DEPOT_TOOLS_UPDATE=0, DEPOT_TOOLS_WIN_TOOLCHAIN=0' -Result 'pass' -Detail 'set for this process and its children'))

    ,@($results)
}

function Get-ShotDepotToolsPython {
    <#
    .SYNOPSIS
        The python3 bundled with the pinned depot_tools.
    #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$DepotTools)

    $candidates = @(Get-ChildItem -LiteralPath $DepotTools -Directory -Filter 'bootstrap-*_bin' -ErrorAction SilentlyContinue |
        ForEach-Object { Join-Path $_.FullName 'python3\bin\python3.exe' } |
        Where-Object { Test-Path -LiteralPath $_ })
    if ($candidates.Count -ge 1) { return $candidates[0] }

    # depot_tools has not bootstrapped its python yet. Fall back to PATH and let
    # the caller record it: the DEPS evaluation only needs a stdlib-only python.
    $fallback = Get-Command python3, python -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($fallback) { return $fallback.Source }
    throw "No python found: neither depot_tools' bootstrap python under $DepotTools nor python on PATH."
}

function Assert-ShotTargetDirectory {
    <#
    .SYNOPSIS
        The Phase 0 target-directory guards.
    .DESCRIPTION
        Fails on the first violation and never repairs anything: no deletes, no
        moves, no "cleaning" a partially populated directory. The operator picks
        a different path.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$TargetRoot,
        [string[]]$ForbiddenRoots = @(),
        [double]$MinimumFreeGiB = 200,
        [switch]$AllowAncestorGclient,
        [switch]$AllowNonEmpty
    )

    $results = [System.Collections.ArrayList]::new()
    $failures = [System.Collections.ArrayList]::new()

    function Add-Guard([string]$Id, [string]$Description, [string]$Result, [string]$Detail) {
        $record = New-ShotGuardResult -Id $Id -Description $Description -Result $Result -Detail $Detail
        [void]$results.Add($record)
        if ($Result -eq 'fail') { [void]$failures.Add("[$Id] $Description`n    $Detail") }
    }

    # TGT-ABSOLUTE ------------------------------------------------------------
    if (-not [System.IO.Path]::IsPathRooted($TargetRoot)) {
        throw "The target directory must be an absolute path; got '$TargetRoot'."
    }
    if ($TargetRoot -match '[*?]') {
        throw "The target directory must not contain wildcards; got '$TargetRoot'."
    }
    $real = Resolve-ShotRealPath $TargetRoot
    Add-Guard 'TGT-ABSOLUTE' 'Target is an absolute, wildcard-free path' 'pass' $real

    if ($real -ne [System.IO.Path]::GetFullPath($TargetRoot).TrimEnd('\')) {
        Add-Guard 'TGT-SYMLINK' 'Target path traverses a symlink or junction' 'warn' `
            "'$TargetRoot' really is '$real'; all guards below use the resolved path."
    } else {
        Add-Guard 'TGT-SYMLINK' 'Target path traverses a symlink or junction' 'pass' 'no reparse points on the path'
    }

    # TGT-NOT-DRIVE-ROOT ------------------------------------------------------
    $pathRoot = [System.IO.Path]::GetPathRoot($real).TrimEnd('\')
    if ($real -eq $pathRoot) {
        # Stop here rather than collecting more findings: the remaining guards
        # would query a bare 'D:' path, which several cmdlets reject outright.
        Add-Guard 'TGT-NOT-DRIVE-ROOT' 'Target is not a drive root' 'fail' `
            "A drive root cannot be a bootstrap target; a failed run there is unrecoverable."
        throw ("Phase 0 refused the target directory '$TargetRoot':`n  " + ($failures -join "`n  "))
    } else {
        Add-Guard 'TGT-NOT-DRIVE-ROOT' 'Target is not a drive root' 'pass' $real
    }

    # TGT-DRIVE-EXISTS --------------------------------------------------------
    if (-not (Test-Path -LiteralPath ($pathRoot + '\'))) {
        Add-Guard 'TGT-DRIVE-EXISTS' 'Target volume exists' 'fail' "No such volume: $pathRoot"
    } else {
        Add-Guard 'TGT-DRIVE-EXISTS' 'Target volume exists' 'pass' $pathRoot
    }

    # TGT-PATH-SHAPE ----------------------------------------------------------
    # Spaces in the source path break Chromium's Windows toolchain scripts and
    # several GN/ninja command lines; non-ASCII breaks more. Cheap to refuse now,
    # expensive to discover during a four-hour build.
    if ($real -match '\s') {
        Add-Guard 'TGT-PATH-SHAPE' 'Target path has no whitespace' 'fail' `
            "Chromium's Windows build does not survive spaces in the source path: '$real'"
    } elseif ($real -notmatch '^[\x20-\x7E]+$') {
        Add-Guard 'TGT-PATH-SHAPE' 'Target path is ASCII' 'fail' "Non-ASCII characters in '$real'"
    } else {
        Add-Guard 'TGT-PATH-SHAPE' 'Target path is ASCII and whitespace-free' 'pass' $real
    }
    if ($real.Length -gt 24) {
        Add-Guard 'TGT-PATH-LENGTH' 'Target path is short' 'warn' `
            "'$real' is $($real.Length) chars; Chromium generates deep paths and MAX_PATH failures appear late in a build."
    } else {
        Add-Guard 'TGT-PATH-LENGTH' 'Target path is short' 'pass' "$($real.Length) chars"
    }

    # TGT-FORBIDDEN -----------------------------------------------------------
    # Containment is checked in both directions: the target may not sit inside a
    # protected root, and it may not swallow one either.
    foreach ($forbidden in $ForbiddenRoots) {
        if ([string]::IsNullOrWhiteSpace($forbidden)) { continue }
        if (-not (Test-Path -LiteralPath $forbidden)) { continue }
        $forbiddenReal = Resolve-ShotRealPath $forbidden
        if (Test-ShotPathContains -Parent $forbiddenReal -Child $real) {
            Add-Guard 'TGT-NOT-IN-PROTECTED-ROOT' 'Target is outside every protected root' 'fail' `
                "'$real' is inside protected root '$forbiddenReal'. The archaeology checkout, its parent (which holds the working .gclient) and depot_tools are never valid targets."
        } elseif (Test-ShotPathContains -Parent $real -Child $forbiddenReal) {
            Add-Guard 'TGT-NOT-ABOVE-PROTECTED-ROOT' 'Target does not contain a protected root' 'fail' `
                "'$real' contains protected root '$forbiddenReal'."
        }
    }
    if (-not ($results | Where-Object { $_.id -like 'TGT-NOT-*PROTECTED-ROOT' })) {
        Add-Guard 'TGT-NOT-IN-PROTECTED-ROOT' 'Target is outside every protected root' 'pass' `
            ("checked: " + (($ForbiddenRoots | Where-Object { $_ }) -join '; '))
    }

    # TGT-EMPTY ---------------------------------------------------------------
    if (Test-Path -LiteralPath $real) {
        $item = Get-Item -LiteralPath $real -Force
        if (-not $item.PSIsContainer) {
            Add-Guard 'TGT-EMPTY' 'Target is a new or empty directory' 'fail' "'$real' is a file."
        } else {
            $children = @(Get-ChildItem -LiteralPath $real -Force -ErrorAction SilentlyContinue)
            # A completed Phase 0 checkpoint is this bootstrap's ownership mark.
            # Without it, a populated directory belongs to someone else and is
            # refused; with it, the checkpoints decide what still has to run.
            $resumeMarker = Join-Path $real 'bootstrap-state\checkpoints\phase-0.json'
            if ($children.Count -gt 0 -and (Test-Path -LiteralPath $resumeMarker)) {
                Add-Guard 'TGT-EMPTY' 'Target is a new, empty, or resumable directory' 'warn' `
                    "'$real' holds a previous run of this bootstrap ($($children.Count) entries); resuming from its checkpoints."
            } elseif ($children.Count -gt 0 -and -not $AllowNonEmpty) {
                Add-Guard 'TGT-EMPTY' 'Target is a new or empty directory' 'fail' `
                    ("'$real' already contains $($children.Count) entries (first: $($children[0].Name)) and no bootstrap checkpoint. This script never deletes or moves anything; choose an empty directory.")
            } elseif ($children.Count -gt 0) {
                Add-Guard 'TGT-EMPTY' 'Target is a new or empty directory' 'warn' `
                    "-AllowNonEmpty accepted $($children.Count) existing entries; measurements from this run are not comparable with a clean baseline (doc section 4: 'in a non-empty checkout, null deps prove nothing')."
            } else {
                Add-Guard 'TGT-EMPTY' 'Target is a new or empty directory' 'pass' 'existing and empty'
            }
        }
    } else {
        $parent = Split-Path -Parent $real
        if (-not (Test-Path -LiteralPath $parent)) {
            Add-Guard 'TGT-EMPTY' 'Target is a new or empty directory' 'fail' `
                "Neither '$real' nor its parent '$parent' exists. Create the parent deliberately; this script will not build an arbitrary path."
        } else {
            Add-Guard 'TGT-EMPTY' 'Target is a new or empty directory' 'pass' 'does not exist yet; will be created'
        }
    }

    # TGT-NOT-IN-GIT ----------------------------------------------------------
    # Landing inside somebody else's work tree means gclient and git would act
    # on that repository's index.
    $probe = $real
    while ($probe -and -not (Test-Path -LiteralPath $probe)) { $probe = Split-Path -Parent $probe }
    if ($probe) {
        $top = & git -C $probe rev-parse --show-toplevel 2>$null
        if ($LASTEXITCODE -eq 0 -and $top) {
            $top = (Resolve-ShotRealPath ([string]$top).Trim())
            Add-Guard 'TGT-NOT-IN-GIT-WORKTREE' 'Target is not inside an existing git work tree' 'fail' `
                "'$real' resolves inside the git work tree at '$top'."
        } else {
            Add-Guard 'TGT-NOT-IN-GIT-WORKTREE' 'Target is not inside an existing git work tree' 'pass' "probed from $probe"
        }
    }

    # TGT-NO-ANCESTOR-GCLIENT -------------------------------------------------
    # gclient walks upwards for a .gclient. On this host the working solution
    # file is D:\Github\.gclient, so any target under D:\Github would be
    # adopted by the archaeology solution instead of its own.
    $ancestor = Split-Path -Parent $real
    $found = $null
    while ($ancestor) {
        if (Test-Path -LiteralPath (Join-Path $ancestor '.gclient')) { $found = Join-Path $ancestor '.gclient'; break }
        $next = Split-Path -Parent $ancestor
        if ($next -eq $ancestor) { break }
        $ancestor = $next
    }
    if ($found -and -not $AllowAncestorGclient) {
        Add-Guard 'TGT-NO-ANCESTOR-GCLIENT' 'No .gclient above the target' 'fail' `
            "Found '$found' above the target. gclient searches upwards and would sync that solution instead of this one."
    } elseif ($found) {
        Add-Guard 'TGT-NO-ANCESTOR-GCLIENT' 'No .gclient above the target' 'warn' "-AllowAncestorGclient accepted '$found'."
    } else {
        Add-Guard 'TGT-NO-ANCESTOR-GCLIENT' 'No .gclient above the target' 'pass' 'none found'
    }

    # TGT-FREE-SPACE ----------------------------------------------------------
    $volume = $null
    try { $volume = Get-Volume -DriveLetter $real[0] -ErrorAction Stop } catch { $volume = $null }
    if ($volume) {
        $freeGiB = [math]::Round($volume.SizeRemaining / 1GB, 2)
        if ($freeGiB -lt $MinimumFreeGiB) {
            Add-Guard 'TGT-FREE-SPACE' "At least $MinimumFreeGiB GiB free" 'fail' `
                "$freeGiB GiB free on $($real[0]):. A full sync plus hooks needs far more; running out mid-sync leaves an unusable tree."
        } else {
            Add-Guard 'TGT-FREE-SPACE' "At least $MinimumFreeGiB GiB free" 'pass' "$freeGiB GiB free"
        }
    } else {
        Add-Guard 'TGT-FREE-SPACE' "At least $MinimumFreeGiB GiB free" 'warn' 'Get-Volume could not report the volume.'
    }

    if ($failures.Count -gt 0) {
        throw ("Phase 0 refused the target directory '$TargetRoot':`n  " + ($failures -join "`n  "))
    }

    [pscustomobject]@{
        TargetRoot     = $TargetRoot
        ResolvedTarget = $real
        Guards         = @($results)
    }
}

Export-ModuleMember -Function @(
    'Resolve-ShotRealPath', 'Test-ShotPathContains', 'New-ShotGuardResult',
    'Assert-ShotHostEnvironment', 'Get-ShotDepotToolsPython', 'Assert-ShotTargetDirectory'
)
