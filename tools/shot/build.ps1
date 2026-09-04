# Build //shot:shot, absorbing the two failure modes that are not build errors.
#
#   1. `gn gen` evaluates several Windows toolchain variants in parallel and they
#      all write the same environment.x86 / environment.x64, so one of them
#      intermittently loses and reports
#          PermissionError: [Errno 13] Permission denied: 'environment.x64'
#      This is a race, not a configuration error; the retry always wins.
#
#   2. ninja re-runs `gn gen` itself when a BUILD.gn changed since the last
#      generate, which puts that same race inside the build. Running gn gen here
#      first makes build.ninja current so ninja usually skips its own regen, and
#      the ninja invocation is retried once for the case where it does not.
#
# -j 12 is a hard constraint on this host, not a preference: higher runs it out
# of memory. See memory `chromium-build-host-quirks`.
param(
    [string]$Target = "shot",
    [string]$Log = "",
    [int]$Jobs = 12
)

$ErrorActionPreference = "Continue"
Set-Location D:\Github\chromium

# args.gn sets icu_data_dir_override = "shot", which is a data set this
# repository generates rather than one third_party/icu ships. third_party/icu is
# a DEPS checkout, so gclient discards the directory; regenerate it here so a
# fresh sync does not fail gn gen with a missing input. The script is
# deterministic, so an unchanged output leaves ninja nothing to redo.
function Invoke-IcuRepack {
    $src = "third_party\icu\cast\icudtl.dat"
    $dst = "third_party\icu\shot\icudtl.dat"
    if (-not (Test-Path $src)) {
        Write-Output "icu: no $src -- is third_party/icu synced?"
        return $false
    }
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    $tmp = "$dst.tmp"
    & python tools\shot\icu_repack.py $src $tmp --preset shot
    if ($LASTEXITCODE -ne 0) {
        Write-Output "icu: icu_repack.py failed"
        return $false
    }
    # Only replace the file when the bytes changed, so ninja does not rebuild
    # the 4 MB data assembly on every invocation.
    if ((Test-Path $dst) -and
        ((Get-FileHash $tmp).Hash -eq (Get-FileHash $dst).Hash)) {
        Remove-Item $tmp
    } else {
        Move-Item -Force $tmp $dst
    }
    return $true
}

# Skia is a DEPS checkout, so gclient restores its upstream source rather than
# the patches tracked by this repository. Apply them once, and fail loudly if a
# Skia roll makes one stop matching instead of silently building without it --
# a missing parallel blur is merely slow, but a missing row limit means the
# streaming PNG decode falls back to a full-size bitmap on every image.
function Invoke-SkiaPatch {
    $patches = @("third_party_skia_parallel_blur.patch",
                 "third_party_skia_incremental_row_limit.patch")
    foreach ($name in $patches) {
        $patch = "..\..\patches\$name"
        & git -C third_party\skia apply --check --reverse $patch 2>$null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "skia: $name already applied"
            continue
        }

        & git -C third_party\skia apply --check $patch
        if ($LASTEXITCODE -ne 0) {
            Write-Host "skia: patches/$name no longer applies"
            return $false
        }
        & git -C third_party\skia apply --verbose $patch
        if ($LASTEXITCODE -ne 0) { return $false }
    }
    return $true
}

function Invoke-GnGen {
    for ($i = 1; $i -le 8; $i++) {
        $out = (& .\buildtools\win\gn.exe gen out\Shot 2>&1 | Out-String)
        if ($LASTEXITCODE -eq 0) {
            Write-Output ("gn gen OK (attempt {0}): {1}" -f $i,
                          (($out -split "`n" | Select-Object -Last 2) -join ' ').Trim())
            return $true
        }
        if ($out -notmatch 'PermissionError') {
            Write-Output "gn gen FAILED (attempt $i)"
            Write-Output (($out -split "`n" | Select-Object -Last 25) -join "`n")
            return $false
        }
    }
    Write-Output "gn gen: gave up after 8 attempts on the environment.x64 race"
    return $false
}

if (-not (Invoke-SkiaPatch)) { exit 1 }
if (-not (Invoke-IcuRepack)) { exit 1 }
if (-not (Invoke-GnGen)) { exit 1 }

if (-not $Log) {
    $Log = Join-Path $env:TEMP ("shot-build-{0}.log" -f $Target)
}

for ($attempt = 1; $attempt -le 2; $attempt++) {
    & ninja -C out\Shot $Target -j $Jobs -k 0 2>&1 | Out-File -Encoding utf8 $Log
    $code = $LASTEXITCODE
    if ($code -eq 0) { break }
    $head = (Get-Content $Log -TotalCount 30) -join "`n"
    if ($head -notmatch 'PermissionError.*environment\.x') { break }
    Write-Output "ninja hit the toolchain race while regenerating; retrying"
    if (-not (Invoke-GnGen)) { exit 1 }
}

Write-Output "log: $Log"
Write-Output "ninja exit: $code"
& python tools\shot\build_errors.py $Log --limit 40
exit $code
