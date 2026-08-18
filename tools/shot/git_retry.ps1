# Run a git command, retrying while another process holds .git/index.lock.
#
# An external `git status --porcelain` poller (GitHub Desktop) takes the index
# lock every few seconds, so any staging command in this repo fails at random.
# `git fsmonitor--daemon` also shows up in the process list but does NOT hold
# the lock -- killing it is never the fix and costs a full rescan.
#
# A lock is only ever removed when both are true: no live git.exe other than
# fsmonitor--daemon, and the lock's mtime is not advancing. Size is not a
# criterion -- a 44 MB index.lock left by a killed `git add` is abandoned, and
# a 0-byte one held by a live process is not.
#
#   .\tools\shot\git_retry.ps1 add -- path/one path/two
#   .\tools\shot\git_retry.ps1 commit -m "msg"

param([Parameter(ValueFromRemainingArguments = $true)][string[]]$GitArgs)

$lock = Join-Path (git rev-parse --git-dir) "index.lock"

function Test-LockAbandoned {
    if (-not (Test-Path $lock)) { return $false }
    $live = Get-Process git -ErrorAction SilentlyContinue |
            Where-Object { $_.Path -and $_.CommandLine -notmatch 'fsmonitor--daemon' }
    if ($live) { return $false }
    $before = (Get-Item $lock).LastWriteTime
    Start-Sleep -Milliseconds 700
    if (-not (Test-Path $lock)) { return $false }
    return (Get-Item $lock).LastWriteTime -eq $before
}

for ($i = 1; $i -le 60; $i++) {
    $out = & git @GitArgs 2>&1
    if ($LASTEXITCODE -eq 0) { $out; exit 0 }
    if ($out -notmatch 'index\.lock') { $out; exit $LASTEXITCODE }
    if ((Test-LockAbandoned) -and (Test-LockAbandoned)) {
        Write-Host "index.lock looks abandoned (no live git, mtime frozen); removing"
        Remove-Item $lock -Force -ErrorAction SilentlyContinue
        # A removed lock can leave the index mid-write; make git re-read it.
        & git reset -q 2>&1 | Out-Null
    }
    Start-Sleep -Milliseconds 400
}
Write-Error "gave up after 60 attempts: $out"
exit 1
