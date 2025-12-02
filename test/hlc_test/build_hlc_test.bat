@echo off
REM HLC Test Build Script for Windows
REM Prerequisites:
REM   1. Haxe installed
REM   2. HashLink installed
REM   3. HLFFI built in HLC mode
REM   4. Visual Studio Build Tools

setlocal

REM Configuration
set HLFFI_DIR=%~dp0..\..
set HASHLINK_DIR=C:\HashLink
set HLC_OUTPUT=%~dp0..\hlc_output

echo === Building HLFFI HLC Test ===
echo.

REM Step 1: Compile Haxe to C (if not already done)
if not exist "%HLC_OUTPUT%\main.c" (
    echo [1] Compiling Haxe to C...
    cd "%~dp0.."
    haxe -hl hlc_output/main.c -main Main
    if errorlevel 1 (
        echo ERROR: Haxe compilation failed
        exit /b 1
    )
    echo     Haxe compiled to C
) else (
    echo [1] HLC output already exists, skipping Haxe compilation
)

REM Step 2: Build HLFFI in HLC mode (if not already done)
if not exist "%HLFFI_DIR%\build-hlc\lib\Debug\hlffi.lib" (
    echo [2] Building HLFFI in HLC mode...
    cd "%HLFFI_DIR%"
    cmake -B build-hlc -DHLFFI_HLC_MODE=ON -DHLFFI_BUILD_EXAMPLES=OFF -DHLFFI_BUILD_TESTS=OFF
    cmake --build build-hlc --config Debug
    if errorlevel 1 (
        echo ERROR: HLFFI HLC build failed
        exit /b 1
    )
    echo     HLFFI HLC library built
) else (
    echo [2] HLFFI HLC library already built, skipping
)

REM Step 3: Compile the HLC test
echo [3] Compiling HLC test executable...

REM Create output directory
if not exist "%~dp0build" mkdir "%~dp0build"

REM Collect all .c files from HLC output
set HLC_SOURCES=
for /r "%HLC_OUTPUT%" %%f in (*.c) do (
    set HLC_SOURCES=!HLC_SOURCES! "%%f"
)

REM Note: This script shows the concept. For actual compilation, use CMake or Visual Studio.
echo     Note: Use CMake to build the HLC test:
echo     cd "%HLFFI_DIR%"
echo     cmake -B build-hlc -DHLFFI_HLC_MODE=ON
echo     cmake --build build-hlc --target test_hlc
echo.
echo === Build configuration complete ===

endlocal
