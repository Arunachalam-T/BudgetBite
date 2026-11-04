# build.ps1 — Build SmartMeal Optimizer backend in PowerShell
$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$Backend = Join-Path $Root "backend"
$Build = Join-Path $Backend "build\release"

$MingwBin = "C:\Users\Arunachalam\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"

if (Test-Path "$MingwBin\g++.exe") {
    $env:PATH = "$MingwBin;$env:PATH"
    $env:CC = "$MingwBin\gcc.exe"
    $env:CXX = "$MingwBin\g++.exe"
}
$env:CMAKE_TLS_VERIFY = "0"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  SmartMeal Optimizer - Backend Build (GCC 16 64-bit)" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan

if (-not (Test-Path $Build)) {
    New-Item -ItemType Directory -Path $Build -Force | Out-Null
}

$Makefile = Join-Path $Build "Makefile"
if (-not (Test-Path $Makefile)) {
    Write-Host "[SmartMeal] Configuring backend with CMake..." -ForegroundColor Cyan
    & cmake -S "$Backend" -B "$Build" `
            -G "MinGW Makefiles" `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_C_COMPILER="$MingwBin\gcc.exe" `
            -DCMAKE_CXX_COMPILER="$MingwBin\g++.exe" `
            -DCMAKE_MAKE_PROGRAM="$MingwBin\mingw32-make.exe" `
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 `
            -DCMAKE_TLS_VERIFY=0

    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n[FAIL] CMake configuration failed." -ForegroundColor Red
        exit 1
    }
}

Write-Host "`n[SmartMeal] Compiling C++ backend (parallel build)..." -ForegroundColor Cyan
& cmake --build "$Build" --parallel

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[FAIL] Compilation failed." -ForegroundColor Red
    exit 1
}

Write-Host "`n============================================================" -ForegroundColor Green
Write-Host "  BUILD SUCCESSFUL!" -ForegroundColor Green
Write-Host "  Server executable: $Build\bin\smartmeal_server.exe" -ForegroundColor Green
Write-Host "  Test suite:        $Build\bin\smartmeal_tests.exe" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green
Write-Host "`nStart the server with: .\run.ps1" -ForegroundColor Yellow
