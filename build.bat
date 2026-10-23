@echo off
echo =====================================================================
echo  BudgetBite - Affordable Meal Planning System
echo  C++17 Build Script (MinGW g++)
echo =====================================================================
echo.

where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] g++ compiler not found in PATH!
    echo Please ensure MinGW / GCC is installed and added to your PATH.
    pause
    exit /b 1
)

echo Compiling src/main.cpp with optimizations (-O2)...
g++ -std=c++1z -O2 -Wall -Wextra -o budgetbite.exe src/main.cpp

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] Compilation succeeded!
    echo Binary created: budgetbite.exe
    echo.
) else (
    echo.
    echo [ERROR] Compilation failed.
    pause
    exit /b %errorlevel%
)
