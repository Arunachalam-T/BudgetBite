@echo off
setlocal
REM build.bat — Build SmartMeal Optimizer backend
set ROOT=%~dp0
set BACKEND=%ROOT%backend
set BUILD=%BACKEND%\build\release

set MINGW_BIN=C:\Users\Arunachalam\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin

if exist "%MINGW_BIN%\g++.exe" (
    set "PATH=%MINGW_BIN%;%PATH%"
    set "CC=%MINGW_BIN%\gcc.exe"
    set "CXX=%MINGW_BIN%\g++.exe"
)

set CMAKE_TLS_VERIFY=0

echo ============================================================
echo   SmartMeal Optimizer - Backend Build
echo ============================================================

if not exist "%BUILD%" mkdir "%BUILD%"

if exist "%BUILD%\Makefile" goto do_build

echo [SmartMeal] Configuring backend with CMake (GCC 16 64-bit)...
cmake -S "%BACKEND%" -B "%BUILD%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="%MINGW_BIN%\gcc.exe" -DCMAKE_CXX_COMPILER="%MINGW_BIN%\g++.exe" -DCMAKE_MAKE_PROGRAM="%MINGW_BIN%\mingw32-make.exe" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_TLS_VERIFY=0

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [FAIL] CMake configuration failed.
    exit /b 1
)

:do_build
echo.
echo [SmartMeal] Compiling C++ backend (parallel build)...
cmake --build "%BUILD%" --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [FAIL] Compilation failed.
    exit /b 1
)

echo.
echo ============================================================
echo   BUILD SUCCESSFUL!
echo   Server executable: %BUILD%\bin\smartmeal_server.exe
echo   Test suite:        %BUILD%\bin\smartmeal_tests.exe
echo ============================================================
echo.
echo Start the server with: .\run.bat
