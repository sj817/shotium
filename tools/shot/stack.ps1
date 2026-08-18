# Run shot.exe under cdb and print the crashing stack.
#
# The build sets symbol_level = 0, which does not mean "no symbols": lld still
# writes a PDB with public symbols, so every frame comes back with a function
# name and an offset. That is enough to say which function faulted and who
# called it, which is all a startup crash usually needs -- and it costs nothing,
# where a symbol_level = 1 build costs a full recompile of the tree.
#
# Usage:
#   powershell -NoProfile -File tools\shot\stack.ps1
#   powershell -NoProfile -File tools\shot\stack.ps1 -Frames 60
param(
    [string]$Page = "shot\testdata\render_corpus.html",
    [int]$Width = 1248,
    [int]$Height = 1320,
    [string]$Output = "shot\testdata\out\shot.png",
    [int]$Frames = 40
)

Set-Location D:\Github\chromium

$cdb = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe"
if (-not (Test-Path $cdb)) {
    Write-Output "no cdb at $cdb -- install the Windows SDK debugging tools"
    exit 1
}

# -o follows child processes, -g/-G skip the initial and final breakpoints so
# the only stop is the fault itself. .symfix points at the public symbol server
# for the system DLLs; shot.exe's own PDB is found next to the binary.
$script = ".symfix;.reload;g;kb $Frames;q"

& $cdb -o -g -G -c $script .\out\Shot\shot.exe `
    --file $Page --width $Width --height $Height --output $Output 2>&1 |
    Select-String -Pattern 'shot!|ntdll!|KERNELBASE!|Access violation|second chance|FATAL|Check failed' |
    Select-Object -First ($Frames + 10)
