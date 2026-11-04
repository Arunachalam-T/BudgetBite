# run.ps1 — Start SmartMeal Optimizer server in PowerShell
$Root = $PSScriptRoot
$Server = Join-Path $Root "backend\build\release\bin\smartmeal_server.exe"
$Port = 8080

$MingwBin = "C:\Users\Arunachalam\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
if (Test-Path $MingwBin) {
    $env:PATH = "$MingwBin;$env:PATH"
}

if (-not (Test-Path "$Server")) {
    Write-Host "[ERROR] Server not built yet. Run .\build.ps1 first." -ForegroundColor Red
    exit 1
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  SmartMeal Optimizer - Backend API Server" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "[SmartMeal] Starting REST API server on http://localhost:$Port" -ForegroundColor Green
Write-Host "[SmartMeal] Press Ctrl+C to stop.`n"
& "$Server" $Port
