<#
.SYNOPSIS
    Generate hlffi.vcxproj for static library build with embedded plugins.

.DESCRIPTION
    This includes:
    - HLFFI wrapper code (6 files)
    - HashLink VM core (6 files: allocator, code, module, jit, debugger, profile)
    - UV plugin + libuv sources (embedded)
    - SSL plugin + mbedtls sources (embedded)

    Explicitly EXCLUDES (these are in libhl.lib):
    - gc.c (garbage collector)
    - src/std/*.c (HashLink standard library)
    - include/pcre/*.c (PCRE2 regex library)

    Output: hlffi.lib - links against libhl.lib (no duplicate symbols)
#>

# Base vcxproj template
$vcxprojHeader = @'
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCProjectVersion>16.0</VCProjectVersion>
    <ProjectGuid>{F7A8B1E3-9C4D-4E5F-8A6B-1D2E3F4A5B6C}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>hlffi</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
    <OutDir>$(SolutionDir)bin\$(Platform)\$(Configuration)\</OutDir>
    <IntDir>$(SolutionDir)obj\$(ProjectName)\$(Platform)\$(Configuration)\</IntDir>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
    <OutDir>$(SolutionDir)bin\$(Platform)\$(Configuration)\</OutDir>
    <IntDir>$(SolutionDir)obj\$(ProjectName)\$(Platform)\$(Configuration)\</IntDir>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_LIB;LIBHL_EXPORTS;HAVE_CONFIG_H;PCRE2_CODE_UNIT_WIDTH=16;MBEDTLS_USER_CONFIG_FILE=&lt;mbedtls_user_config.h&gt;;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>false</ConformanceMode>
      <AdditionalIncludeDirectories>$(ProjectDir)include;$(ProjectDir)vendor\hashlink\src;$(ProjectDir)vendor\hashlink\include\pcre;$(ProjectDir)vendor\hashlink\include\libuv\include;$(ProjectDir)vendor\hashlink\include\libuv\src;$(ProjectDir)vendor\hashlink\include\mbedtls\include;$(ProjectDir)vendor\hashlink\libs\ssl;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <LanguageStandard>stdcpp17</LanguageStandard>
      <MultiProcessorCompilation>true</MultiProcessorCompilation>
      <TreatWarningAsError>false</TreatWarningAsError>
      <DisableSpecificWarnings>4244;4267;4456;4459;4127;4100;4201;4706;%(DisableSpecificWarnings)</DisableSpecificWarnings>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
    <Lib>
      <AdditionalDependencies>ws2_32.lib;advapi32.lib;psapi.lib;user32.lib;iphlpapi.lib;userenv.lib;winmm.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Lib>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>NDEBUG;_LIB;LIBHL_EXPORTS;HAVE_CONFIG_H;PCRE2_CODE_UNIT_WIDTH=16;MBEDTLS_USER_CONFIG_FILE=&lt;mbedtls_user_config.h&gt;;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>false</ConformanceMode>
      <AdditionalIncludeDirectories>$(ProjectDir)include;$(ProjectDir)vendor\hashlink\src;$(ProjectDir)vendor\hashlink\include\pcre;$(ProjectDir)vendor\hashlink\include\libuv\include;$(ProjectDir)vendor\hashlink\include\libuv\src;$(ProjectDir)vendor\hashlink\include\mbedtls\include;$(ProjectDir)vendor\hashlink\libs\ssl;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <LanguageStandard>stdcpp17</LanguageStandard>
      <MultiProcessorCompilation>true</MultiProcessorCompilation>
      <TreatWarningAsError>false</TreatWarningAsError>
      <DisableSpecificWarnings>4244;4267;4456;4459;4127;4100;4201;4706;%(DisableSpecificWarnings)</DisableSpecificWarnings>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
    <Lib>
      <AdditionalDependencies>ws2_32.lib;advapi32.lib;psapi.lib;user32.lib;iphlpapi.lib;userenv.lib;winmm.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Lib>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClInclude Include="include\hlffi.h" />
  </ItemGroup>
  <ItemGroup>
'@

$vcxprojFooter = @'
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>
'@

function Find-Sources {
    param(
        [string]$BasePath,
        [string]$Pattern
    )

    $fullPath = Join-Path $PSScriptRoot $BasePath
    $files = Get-ChildItem -Path $fullPath -Filter $Pattern -Recurse -File -ErrorAction SilentlyContinue

    $relativePaths = $files | ForEach-Object {
        $rel = $_.FullName.Substring($PSScriptRoot.Length + 1)
        $rel.Replace('/', '\')
    } | Sort-Object

    return $relativePaths
}

# Build source file list
$sources = @()

# HLFFI wrapper code
$sources += @{
    Name = 'HLFFI wrapper code'
    Files = @(
        'src\hlffi_core.c',
        'src\hlffi_events.c',
        'src\hlffi_integration.c',
        'src\hlffi_lifecycle.c',
        'src\hlffi_reload.c',
        'src\hlffi_threading.c'
    )
}

# HashLink VM core (EXCLUDING gc.c + allocator.c, stdlib, PCRE2 - those are in libhl.lib)
# NOTE: allocator.c is included by gc.c, so it must not be compiled separately
$sources += @{
    Name = 'HashLink VM core'
    Files = @(
        'vendor\hashlink\src\code.c',
        'vendor\hashlink\src\module.c',
        'vendor\hashlink\src\jit.c',
        'vendor\hashlink\src\debugger.c',
        'vendor\hashlink\src\profile.c'
    )
}

# NOTE: GC, stdlib, and PCRE2 are NOT included here
# They are built into libhl.lib (from vendor/hashlink/libhl.vcxproj)
# hlffi.lib links against libhl.lib to avoid duplicate symbols

# UV and SSL plugin wrappers
$sources += @{
    Name = 'Plugin wrappers'
    Files = @(
        'vendor\hashlink\libs\uv\uv.c',
        'vendor\hashlink\libs\ssl\ssl.c'
    )
}

# libuv sources
$uvSrc = Find-Sources -BasePath 'vendor\hashlink\include\libuv\src' -Pattern '*.c'
$uvWinSrc = Find-Sources -BasePath 'vendor\hashlink\include\libuv\src\win' -Pattern '*.c'
$uvSources = $uvSrc + $uvWinSrc | Sort-Object
$sources += @{
    Name = 'libuv sources (embedded)'
    Files = $uvSources
}

# mbedtls sources
$mbedtlsSources = Find-Sources -BasePath 'vendor\hashlink\include\mbedtls\library' -Pattern '*.c'
$sources += @{
    Name = 'mbedtls sources (embedded - TLS/SSL support)'
    Files = $mbedtlsSources
}

# Generate vcxproj
$vcxproj = $vcxprojHeader

foreach ($section in $sources) {
    $vcxproj += "    <!-- $($section.Name) -->`r`n"
    foreach ($file in $section.Files) {
        $vcxproj += "    <ClCompile Include=`"$file`" />`r`n"
    }
    $vcxproj += "`r`n"
}

$vcxproj += $vcxprojFooter

# Write output
$outputPath = Join-Path $PSScriptRoot 'hlffi_monolithic.vcxproj'
[System.IO.File]::WriteAllText($outputPath, $vcxproj, [System.Text.Encoding]::UTF8)

# Print summary
$totalFiles = ($sources | ForEach-Object { $_.Files.Count } | Measure-Object -Sum).Sum
Write-Host "Generated hlffi_monolithic.vcxproj"
Write-Host "Total source files: $totalFiles"
foreach ($section in $sources) {
    Write-Host "  $($section.Name): $($section.Files.Count) files"
}
Write-Host ""
Write-Host "To use:"
Write-Host "  Move-Item hlffi_monolithic.vcxproj hlffi.vcxproj -Force"
