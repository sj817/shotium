# Expose .claude/skills to agents that look in .agents/skills (Codex, Gemini
# CLI, Copilot, Antigravity). The canonical copies live in .claude/skills so
# Claude Code needs no setup; this makes .agents/skills/<name> point at each of
# them. Junctions on Windows (no admin, no Developer Mode), symlinks elsewhere.
# .agents/skills is gitignored; run this once per checkout, and again after
# adding a skill.
#
#   pwsh tools/shot/link_agent_skills.ps1
#   pwsh tools/shot/link_agent_skills.ps1 -Remove
param([switch]$Remove)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$source = Join-Path $root '.claude/skills'
$target = Join-Path $root '.agents/skills'
$onWindows = [System.Environment]::OSVersion.Platform -eq 'Win32NT'

if ($Remove) {
    if (Test-Path $target) {
        # DirectoryInfo.Delete() on a junction or symlink removes the link,
        # never the directory it points at.
        foreach ($link in Get-ChildItem $target -Directory) {
            if ($link.LinkType) { $link.Delete(); Write-Output "removed $($link.Name)" }
        }
    }
    return
}

New-Item -ItemType Directory -Force $target | Out-Null
foreach ($skill in Get-ChildItem $source -Directory) {
    $link = Join-Path $target $skill.Name
    if (Test-Path $link) {
        if ((Get-Item $link).LinkType) { Write-Output "ok      $($skill.Name)"; continue }
        Write-Output "skip    $($skill.Name): a real directory is in the way"
        continue
    }
    $type = if ($onWindows) { 'Junction' } else { 'SymbolicLink' }
    New-Item -ItemType $type -Path $link -Target $skill.FullName | Out-Null
    Write-Output "linked  $($skill.Name)"
}
