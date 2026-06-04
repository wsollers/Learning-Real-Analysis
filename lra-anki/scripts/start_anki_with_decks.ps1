param(
    [string]$AnkiExe = "$env:LOCALAPPDATA\Programs\Anki\anki.exe",
    [switch]$OpenDeckFolder
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$DeckRoot = Resolve-Path (Join-Path $ScriptDir "..")

if (-not (Test-Path -LiteralPath $AnkiExe)) {
    $fallback = "C:\Program Files\Anki\anki.exe"
    if (Test-Path -LiteralPath $fallback) {
        $AnkiExe = $fallback
    } else {
        throw "Anki was not found. Pass -AnkiExe with the full path to anki.exe."
    }
}

$AnkiDir = Split-Path -Parent $AnkiExe
$pathParts = $env:PATH -split ';' | Where-Object { $_ }
if ($pathParts -notcontains $AnkiDir) {
    $env:PATH = "$AnkiDir;$env:PATH"
}

$CsvDecks = Get-ChildItem -LiteralPath $DeckRoot -Recurse -Filter "*.csv" |
    Sort-Object FullName

if (-not $CsvDecks) {
    throw "No CSV decks found under $DeckRoot."
}

Write-Host "Anki added to PATH for this PowerShell session:"
Write-Host "  $AnkiDir"
Write-Host ""
Write-Host "Starting Anki:"
Write-Host "  $AnkiExe"
Start-Process -FilePath $AnkiExe

Write-Host ""
Write-Host "CSV decks ready to import:"
foreach ($deck in $CsvDecks) {
    Write-Host "  $($deck.FullName)"
}

Write-Host ""
Write-Host "In Anki, use File -> Import, then choose the CSV deck files above."
Write-Host "Map fields as Front, Back, Tags."

if ($OpenDeckFolder) {
    Start-Process explorer.exe -ArgumentList "`"$DeckRoot`""
}
