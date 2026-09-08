[CmdletBinding()]
param(
    [string]$Python = "python",
    [string]$OutputDirectory = "Release"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$outputPath = Join-Path $projectRoot $OutputDirectory
$workPath = Join-Path $projectRoot "build\\teacher_sim"
$specPath = Join-Path $projectRoot "teacher_sim.spec"

& $Python -m PyInstaller --noconfirm --clean --distpath $outputPath --workpath $workPath $specPath
if ($LASTEXITCODE -ne 0) { throw "PyInstaller failed with exit code $LASTEXITCODE." }

$teacherExecutable = Join-Path $outputPath "teacher_sim.exe"
if (-not (Test-Path -LiteralPath $teacherExecutable -PathType Leaf)) {
    throw "Expected package was not produced: $teacherExecutable"
}

Write-Host "Created $teacherExecutable"
