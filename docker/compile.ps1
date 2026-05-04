# =============================================================
# compile.ps1 — Build main.tex inside a Docker container
# Project: Learning Real Analysis
#
# Usage:
#   .\docker\compile.ps1            # full build
#   .\docker\compile.ps1 -Clean     # latexmk -C then full build
#   .\docker\compile.ps1 -Build     # build the Docker image first
#   .\docker\compile.ps1 -Build -Clean  # rebuild image + clean build
#   .\docker\compile.ps1 -Open      # open PDF after successful build
#
# The script must be run from *anywhere* inside the repo — it
# resolves the repo root automatically.
# =============================================================

param (
    [switch]$Build,   # (Re)build the Docker image before compiling
    [switch]$Clean,   # Run latexmk -C (full clean) before compiling
    [switch]$Open     # Open the PDF in the default viewer on success
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ── Resolve paths ─────────────────────────────────────────────
# Walk up from the script's location to find the repo root
# (the directory that contains main.tex).
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot  = Split-Path -Parent $scriptDir   # docker/ is one level down

if (-not (Test-Path (Join-Path $repoRoot 'main.tex'))) {
    Write-Error "Could not locate main.tex from repo root: $repoRoot"
    exit 1
}

$imageName  = 'learning-real-analysis-latex'
$dockerfile = Join-Path $scriptDir 'Dockerfile'
$outputPdf  = Join-Path $repoRoot 'build\main.pdf'

# ── Build the Docker image ────────────────────────────────────
if ($Build) {
    Write-Host ""
    Write-Host "==> Building Docker image: $imageName" -ForegroundColor Cyan
    docker build -t $imageName -f $dockerfile $scriptDir
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Docker image build failed."
        exit 1
    }
    Write-Host "==> Image built successfully." -ForegroundColor Green
}

# Verify image exists
$imageExists = docker images -q $imageName
if (-not $imageExists) {
    Write-Host ""
    Write-Host "Image '$imageName' not found. Run with -Build first:" -ForegroundColor Yellow
    Write-Host "  .\docker\compile.ps1 -Build" -ForegroundColor Yellow
    exit 1
}

# ── Clean previous build artifacts ────────────────────────────
if ($Clean) {
    Write-Host ""
    Write-Host "==> Cleaning previous build artifacts..." -ForegroundColor Cyan
    docker run --rm `
        -v "${repoRoot}:/workspace" `
        -w /workspace `
        $imageName `
        latexmk -C main.tex
    Write-Host "==> Clean complete." -ForegroundColor Green
}

# ── Compile ───────────────────────────────────────────────────
Write-Host ""
Write-Host "==> Compiling main.tex with LuaLaTeX..." -ForegroundColor Cyan
Write-Host "    Repo root : $repoRoot"
Write-Host "    Output PDF: $outputPdf"
Write-Host ""

docker run --rm `
    -v "${repoRoot}:/workspace" `
    -w /workspace `
    $imageName

$exitCode = $LASTEXITCODE

# ── Result ────────────────────────────────────────────────────
Write-Host ""
if ($exitCode -eq 0) {
    Write-Host "==> Build succeeded!" -ForegroundColor Green
    if (Test-Path $outputPdf) {
        Write-Host "    PDF written to: $outputPdf" -ForegroundColor Green
    } else {
        Write-Host "    WARNING: latexmk exited 0 but PDF not found at expected path." -ForegroundColor Yellow
        Write-Host "    Check build/ directory." -ForegroundColor Yellow
    }
    if ($Open -and (Test-Path $outputPdf)) {
        Write-Host "==> Opening PDF..." -ForegroundColor Cyan
        Start-Process $outputPdf
    }
} else {
    Write-Host "==> Build FAILED (exit code $exitCode)." -ForegroundColor Red
    Write-Host "    Check the output above for LaTeX errors." -ForegroundColor Red
    Write-Host "    Tip: build/main.log contains the full compiler log." -ForegroundColor Yellow
    exit $exitCode
}
