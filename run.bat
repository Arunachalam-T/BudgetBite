@echo off
setlocal
REM run.bat — Start SmartMeal Optimizer server
set ROOT=%~dp0
set SERVER=%ROOT%backend\build\release\bin\smartmeal_server.exe
set PORT=8080

set MINGW_BIN=C:\Users\Arunachalam\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin

if exist "%MINGW_BIN%" (
    set "PATH=%MINGW_BIN%;%PATH%"
)

if not exist "%SERVER%" (
    echo [ERROR] Server not built yet. Run build.bat first.
    exit /b 1
)

echo ============================================================
echo   SmartMeal Optimizer - Backend API Server
echo ============================================================
echo [SmartMeal] Starting REST API server on http://localhost:%PORT%
echo [SmartMeal] Press Ctrl+C to stop.
echo.
"%SERVER%" %PORT%
