$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$stagingRoot = Join-Path $repositoryRoot 'staging'
$emulatorPath = Join-Path $stagingRoot 'melonDS.exe'
$romPath = Join-Path $stagingRoot 'dldi/boot.nds'

if (-not (Test-Path -LiteralPath $emulatorPath -PathType Leaf)) {
    throw "melonDS executable not found: $emulatorPath"
}
if (-not (Test-Path -LiteralPath $romPath -PathType Leaf)) {
    throw "Staged boot.nds not found: $romPath. Run the staging task first."
}

Start-Process -FilePath $emulatorPath -ArgumentList ('"{0}"' -f $romPath) -WorkingDirectory $stagingRoot
Write-Host "Opened $romPath in melonDS"
