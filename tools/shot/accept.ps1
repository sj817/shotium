# The acceptance run: build, render the corpus, diff against the oracle, and
# report the binary's size. One script so the evidence is reproducible rather
# than a sequence of commands someone has to remember.
#
# The oracle in shot/testdata/out/oracle.png was captured from an unstripped
# Chrome 151.0.7922.138 with
#     --headless --disable-gpu --hide-scrollbars --force-device-scale-factor=1
# at 1248x1320, so shot is asked for exactly that geometry. A size mismatch
# makes every later number meaningless, so it is checked before diffing.
#
# Differences are expected and allowed. What is not allowed is an unexplained
# one -- see docs/cut-progress.md section 8.6 for the render-affecting changes
# that are already known and accounted for.
param(
    [switch]$SkipBuild,
    [int]$Width = 1248,
    [int]$Height = 1320
)

Set-Location D:\Github\chromium
$ErrorActionPreference = "Continue"

$corpus = "shot\testdata\render_corpus.html"
$oracle = "shot\testdata\out\oracle.png"
$actual = "shot\testdata\out\shot.png"
$diff = "shot\testdata\out\diff.png"
$exe = "out\Shot\shotium.exe"

if (-not $SkipBuild) {
    Write-Output "== build =="
    & .\tools\shot\build.ps1
    if ($LASTEXITCODE -ne 0) {
        Write-Output "BUILD FAILED -- stopping; there is nothing to accept."
        exit 1
    }
}

if (-not (Test-Path $exe)) {
    Write-Output "no $exe -- the link did not produce a binary"
    exit 1
}

Write-Output ""
Write-Output "== size against the 336 MB baseline =="
$bytes = (Get-Item $exe).Length
"{0}  {1:N0} bytes  {2:N1} MB  ({3:P1} of the 336 MB baseline)" -f `
    $exe, $bytes, ($bytes / 1MB), ($bytes / (336 * 1MB))

Write-Output ""
Write-Output "== render =="
if (Test-Path $actual) { Remove-Item $actual -Force }
$cmd = "$exe --file $corpus --width $Width --height $Height --output $actual"
Write-Output "  $cmd"
& $exe --file $corpus --width $Width --height $Height --output $actual 2>&1 |
    Select-Object -Last 20
Write-Output "shot exit: $LASTEXITCODE"

if (-not (Test-Path $actual)) {
    Write-Output "NO PNG PRODUCED -- criterion 1 is met but nothing else is."
    exit 1
}
"produced {0}  {1:N0} bytes" -f $actual, (Get-Item $actual).Length

Write-Output ""
Write-Output "== pixel diff against the oracle =="
& python tools\shot\pixel_diff.py $oracle $actual --out $diff
$diffExit = $LASTEXITCODE

Write-Output ""
Write-Output "== difference by region =="
Write-Output "  A whole-image percentage cannot distinguish 'antialiasing is a"
Write-Output "  shade different everywhere' from 'one element is missing', so"
Write-Output "  the corpus is reported feature by feature. See section 15 of"
Write-Output "  docs/cut-progress.md for what each surviving number is."
& python tools\shot\diff_report.py $oracle $actual --regions shot\testdata\regions.txt
exit $diffExit
