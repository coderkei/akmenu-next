$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$archivePath = Join-Path $repositoryRoot 'package/akmenu-next-flashcart.zip'
$destinationRoot = Join-Path $repositoryRoot 'staging/dldi'

if (-not (Test-Path -LiteralPath $archivePath -PathType Leaf)) {
    throw "Flashcart archive not found: $archivePath"
}

$archive = [System.IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $selectedEntries = @(
        $archive.Entries | Where-Object {
            $name = $_.FullName.Replace('\', '/')
            $name -eq 'boot.nds' -or $name.StartsWith('_nds/') -or $name.StartsWith('_pico/')
        }
    )

    if (-not ($selectedEntries | Where-Object { $_.FullName.Replace('\', '/') -eq 'boot.nds' })) {
        throw "The flashcart archive does not contain a root boot.nds: $archivePath"
    }
    if (-not ($selectedEntries | Where-Object { $_.FullName.Replace('\', '/').StartsWith('_nds/') })) {
        throw "The flashcart archive does not contain the _nds folder: $archivePath"
    }
    if (-not ($selectedEntries | Where-Object { $_.FullName.Replace('\', '/').StartsWith('_pico/') })) {
        throw "The flashcart archive does not contain the _pico folder: $archivePath"
    }

    New-Item -ItemType Directory -Path $destinationRoot -Force | Out-Null
    foreach ($item in @('_nds', '_pico', 'boot.nds')) {
        $destination = Join-Path $destinationRoot $item
        if (Test-Path -LiteralPath $destination) {
            Remove-Item -LiteralPath $destination -Recurse -Force
        }
    }

    $destinationPrefix = [System.IO.Path]::GetFullPath($destinationRoot) + [System.IO.Path]::DirectorySeparatorChar
    foreach ($entry in $selectedEntries) {
        $relativePath = $entry.FullName.Replace('/', '\')
        if ($relativePath.StartsWith('\') -or $relativePath -match '(^|\\)\.\.(\\|$)') {
            throw "Unsafe path in flashcart archive: $($entry.FullName)"
        }

        $outputPath = [System.IO.Path]::GetFullPath((Join-Path $destinationRoot $relativePath))
        if (-not $outputPath.StartsWith($destinationPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Unsafe path in flashcart archive: $($entry.FullName)"
        }

        if ($entry.FullName.EndsWith('/') -or $entry.FullName.EndsWith('\')) {
            New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
            continue
        }

        $parentPath = Split-Path -Parent $outputPath
        New-Item -ItemType Directory -Path $parentPath -Force | Out-Null
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $outputPath, $true)
    }
}
finally {
    $archive.Dispose()
}

Write-Host "Staged _nds, _pico, and boot.nds in $destinationRoot"
