@echo off
REM Patch HashLink for HLFFI integration
REM Run this after cloning or updating the hashlink submodule

setlocal enabledelayedexpansion

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
REM Remove trailing backslash
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
REM Go up one level to project root
for %%I in ("%SCRIPT_DIR%\..") do set "PROJECT_DIR=%%~fI"
set "HL_DIR=%PROJECT_DIR%\vendor\hashlink"

echo === HLFFI HashLink Patcher ===
echo.
echo Project dir: %PROJECT_DIR%
echo HashLink dir: %HL_DIR%
echo.

REM Check if hashlink directory exists
if not exist "%HL_DIR%" (
    echo ERROR: HashLink directory not found: %HL_DIR%
    echo Please clone the hashlink submodule first.
    exit /b 1
)

REM === Patch 1: Disable vcpkg in libhl.vcxproj ===
echo [1/2] Patching libhl.vcxproj to disable vcpkg...

set "VCXPROJ=%HL_DIR%\libhl.vcxproj"
if not exist "%VCXPROJ%" (
    echo   WARNING: libhl.vcxproj not found, skipping vcpkg patch
    goto :patch2
)

REM Check if already patched
findstr /C:"VcpkgEnabled" "%VCXPROJ%" >nul 2>&1
if %errorlevel% equ 0 (
    echo   Already patched ^(VcpkgEnabled found^)
) else (
    REM Add VcpkgEnabled=false to PropertyGroup Globals
    powershell -Command "$content = Get-Content '%VCXPROJ%' -Raw; $content = $content -replace '(<PropertyGroup Label=\"Globals\">)', \"`$1`r`n    <VcpkgEnabled>false</VcpkgEnabled>\"; Set-Content '%VCXPROJ%' -Value $content -NoNewline"
    if %errorlevel% equ 0 (
        echo   Patched: Added VcpkgEnabled=false
    ) else (
        echo   ERROR: Failed to patch libhl.vcxproj
    )
)

:patch2
REM === Patch 2: Export obj_resolve_field in obj.c ===
echo [2/2] Patching obj.c to export obj_resolve_field...

set "OBJ_C=%HL_DIR%\src\std\obj.c"
if not exist "%OBJ_C%" (
    echo   WARNING: obj.c not found, skipping obj_resolve_field patch
    goto :done
)

REM Check if already patched (look for HL_API before obj_resolve_field)
findstr /C:"HL_API hl_field_lookup" "%OBJ_C%" >nul 2>&1
if %errorlevel% equ 0 (
    echo   Already patched ^(HL_API obj_resolve_field found^)
) else (
    REM Replace "static hl_field_lookup* obj_resolve_field" with "HL_API hl_field_lookup* obj_resolve_field"
    powershell -Command "$content = Get-Content '%OBJ_C%' -Raw; $content = $content -replace 'static hl_field_lookup\* obj_resolve_field', 'HL_API hl_field_lookup* obj_resolve_field'; Set-Content '%OBJ_C%' -Value $content -NoNewline"
    if %errorlevel% equ 0 (
        echo   Patched: Changed obj_resolve_field to HL_API
    ) else (
        echo   ERROR: Failed to patch obj.c
    )
)

:done
echo.
echo === Patching complete ===
echo.
echo NOTE: You need to rebuild libhl.dll after patching:
echo   msbuild vendor\hashlink\libhl.vcxproj /p:Configuration=Debug /p:Platform=x64
echo.
echo Then copy the patched libhl.dll to bin\x64\Debug\

endlocal
