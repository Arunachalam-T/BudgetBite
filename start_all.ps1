# start_all.ps1 — Launch both Backend and Frontend in separate windows
$Root = $PSScriptRoot

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Starting SmartMeal Optimizer (Backend + Frontend)" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan

Write-Host "Starting Backend server on port 8080..." -ForegroundColor Green
Start-Process powershell -ArgumentList "-NoExit", "-File", "$Root\run.ps1"

Write-Host "Starting Frontend Vite server on port 5173..." -ForegroundColor Green
Start-Process powershell -ArgumentList "-NoExit", "-Command", "Set-Location '$Root\frontend'; npm run dev"

Write-Host "`nBoth services launched in separate windows!" -ForegroundColor Green
Write-Host "Open your web browser at: http://localhost:5173`n" -ForegroundColor Yellow

