@echo off
REM Build script for Haxe test files
REM Requires Haxe compiler installed: https://haxe.org/download/

echo Building Main.hx to hello.hl...
haxe -hl hello.hl -main Main

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful!
    echo Output: hello.hl
    echo.
    echo To run with HashLink standalone:
    echo   hl hello.hl
    echo.
    echo To run with HLFFI examples:
    echo   ..\bin\x64\Debug\example_hello_world.exe hello.hl
) else (
    echo.
    echo Build failed!
    exit /b 1
)
