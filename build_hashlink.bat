@echo off
REM Build HashLink libraries for HLFFI

set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
set HL_DIR=vendor\hashlink

echo Building libhl.dll...
cd %HL_DIR%
%MSBUILD% libhl.vcxproj -p:Configuration=Debug -p:Platform=x64 -p:PlatformToolset=v143 -v:minimal
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: libhl Debug build failed!
    exit /b 1
)

%MSBUILD% libhl.vcxproj -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143 -v:minimal
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: libhl Release build failed!
    exit /b 1
)

echo.
echo Building uv.hdll...
cd libs\uv
%MSBUILD% uv.vcxproj -p:Configuration=Debug -p:Platform=x64 -p:PlatformToolset=v143 -v:minimal
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: uv Debug build failed!
    exit /b 1
)

%MSBUILD% uv.vcxproj -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143 -v:minimal
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: uv Release build failed!
    exit /b 1
)

echo.
echo Building ssl.hdll...
cd ..\ssl
%MSBUILD% ssl.vcxproj -p:Configuration=Debug -p:Platform=x64 -p:PlatformToolset=v143 -v:minimal
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: ssl Debug build failed!
    exit /b 1
)

%MSBUILD% ssl.vcxproj -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143 -v:minimal
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: ssl Release build failed!
    exit /b 1
)

cd ..\..\..
echo.
echo All builds completed successfully!
echo.
echo Output files:
echo   libhl.dll: %HL_DIR%\x64\Debug\libhl.dll
echo   libhl.dll: %HL_DIR%\x64\Release\libhl.dll
echo   uv.hdll:   %HL_DIR%\x64\Debug\uv.hdll
echo   uv.hdll:   %HL_DIR%\x64\Release\uv.hdll
echo   ssl.hdll:  %HL_DIR%\x64\Debug\ssl.hdll
echo   ssl.hdll:  %HL_DIR%\x64\Release\ssl.hdll
