@echo off
echo =====================================================================
echo  BudgetBite - Affordable Meal Planning System Runner
echo =====================================================================
echo.

if not exist budgetbite.exe (
    echo [INFO] budgetbite.exe not found. Running build.bat first...
    call build.bat
    if %errorlevel% neq 0 exit /b %errorlevel%
)

echo Executing C++ optimization engine...
budgetbite.exe --csv "Data/indian_meal_planner_indb_2024_11 (1).csv" --budget 700 --kcal 2000 --protein 50 --diet any --out "web/meal_plan.json"

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] Optimization complete!
    echo Opening Web Interface in your default browser...
    start "" "web\index.html"
) else (
    echo [ERROR] Optimizer encountered an issue.
    pause
)
