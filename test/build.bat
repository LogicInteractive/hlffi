@echo off
REM Build script for Haxe test files
REM Requires Haxe compiler installed: https://haxe.org/download/

echo Building Main.hx to hello.hl...
haxe -hl hello.hl -main Main
if %ERRORLEVEL% NEQ 0 (
    echo Build failed for Main.hx!
    exit /b 1
)

echo Building Player.hx to player.hl...
haxe -hl player.hl -main Player
if %ERRORLEVEL% NEQ 0 (
    echo Build failed for Player.hx!
    exit /b 1
)

echo.
echo Build successful!
echo Output files:
echo   - hello.hl (Main.hx)
echo   - player.hl (Player.hx)
echo.
echo To run with HashLink standalone:
echo   hl hello.hl
echo   hl player.hl
echo.
echo To run with HLFFI examples:
echo   ..\bin\x64\Debug\example_hello_world.exe hello.hl
echo   ..\build\bin\Debug\test_instance_basic.exe player.hl
