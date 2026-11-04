@echo off
set ROOT=%~dp0
cd /d "%ROOT%"

echo ============================================================
echo   Starting SmartMeal Optimizer (Backend + Frontend)
echo ============================================================
echo Starting Backend Server on http://localhost:8080...
start "SmartMeal Backend" cmd /k "%ROOT%run.bat"

echo Starting Frontend Dev Server on http://localhost:5173...
start "SmartMeal Frontend" cmd /k "cd /d %ROOT%frontend && npm run dev"

echo.
echo Both backend and frontend are starting up in separate terminal windows.
echo Open your browser and navigate to: http://localhost:5173
echo.

