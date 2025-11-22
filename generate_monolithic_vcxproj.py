#!/usr/bin/env python3
"""
Generate a monolithic hlffi.vcxproj with all HashLink components built-in.

This merges:
- HLFFI wrapper code
- HashLink VM core
- UV plugin + libuv sources
- SSL plugin + mbedtls sources

Output: Complete static library with no separate .hdll files needed.
"""

import os
import glob

# Base vcxproj template
VCXPROJ_HEADER = '''<?xml version="1.0" encoding="utf-8"?>
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
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
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
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
    <OutDir>$(SolutionDir)bin\\$(Platform)\\$(Configuration)\\</OutDir>
    <IntDir>$(SolutionDir)obj\\$(ProjectName)\\$(Platform)\\$(Configuration)\\</IntDir>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
    <OutDir>$(SolutionDir)bin\\$(Platform)\\$(Configuration)\\</OutDir>
    <IntDir>$(SolutionDir)obj\\$(ProjectName)\\$(Platform)\\$(Configuration)\\</IntDir>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_LIB;LIBHL_EXPORTS;HAVE_CONFIG_H;PCRE2_CODE_UNIT_WIDTH=16;MBEDTLS_USER_CONFIG_FILE=&lt;mbedtls_user_config.h&gt;;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>false</ConformanceMode>
      <AdditionalIncludeDirectories>$(ProjectDir)include;$(ProjectDir)vendor\\hashlink\\src;$(ProjectDir)vendor\\hashlink\\include\\pcre;$(ProjectDir)vendor\\hashlink\\include\\libuv\\include;$(ProjectDir)vendor\\hashlink\\include\\libuv\\src;$(ProjectDir)vendor\\hashlink\\include\\mbedtls\\include;$(ProjectDir)vendor\\hashlink\\libs\\ssl;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
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
      <AdditionalIncludeDirectories>$(ProjectDir)include;$(ProjectDir)vendor\\hashlink\\src;$(ProjectDir)vendor\\hashlink\\include\\pcre;$(ProjectDir)vendor\\hashlink\\include\\libuv\\include;$(ProjectDir)vendor\\hashlink\\include\\libuv\\src;$(ProjectDir)vendor\\hashlink\\include\\mbedtls\\include;$(ProjectDir)vendor\\hashlink\\libs\\ssl;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
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
    <ClInclude Include="include\\hlffi.h" />
  </ItemGroup>
  <ItemGroup>
'''

VCXPROJ_FOOTER = '''  </ItemGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>
'''

def find_sources(base_path, pattern):
    """Find source files matching pattern"""
    full_pattern = os.path.join(base_path, pattern)
    files = glob.glob(full_pattern, recursive=True)
    # Convert to relative paths with Windows-style backslashes
    rel_files = [os.path.relpath(f, '.').replace('/', '\\') for f in files]
    return sorted(rel_files)

def main():
    sources = []

    # HLFFI wrapper code
    sources.append(('HLFFI wrapper code', [
        'src\\hlffi_core.c',
        'src\\hlffi_events.c',
        'src\\hlffi_integration.c',
        'src\\hlffi_lifecycle.c',
        'src\\hlffi_reload.c',
        'src\\hlffi_threading.c',
    ]))

    # HashLink VM core
    sources.append(('HashLink VM core', [
        'vendor\\hashlink\\src\\allocator.c',
        'vendor\\hashlink\\src\\code.c',
        'vendor\\hashlink\\src\\module.c',
        'vendor\\hashlink\\src\\jit.c',
        'vendor\\hashlink\\src\\debugger.c',
        'vendor\\hashlink\\src\\profile.c',
        'vendor\\hashlink\\src\\gc.c',  # Garbage collector
    ]))

    # HashLink standard library (from libhl)
    stdlib_sources = find_sources('vendor/hashlink/src/std', '*.c')
    sources.append(('HashLink standard library', stdlib_sources))

    # PCRE2 regex library (from libhl)
    pcre_sources = find_sources('vendor/hashlink/include/pcre', '*.c')
    sources.append(('PCRE2 regex library', pcre_sources))

    # UV and SSL plugin wrappers
    sources.append(('Plugin wrappers', [
        'vendor\\hashlink\\libs\\uv\\uv.c',
        'vendor\\hashlink\\libs\\ssl\\ssl.c',
    ]))

    # libuv sources
    uv_sources = (
        find_sources('vendor/hashlink/include/libuv/src', '*.c') +
        find_sources('vendor/hashlink/include/libuv/src/win', '*.c')
    )
    sources.append(('libuv sources (embedded)', uv_sources))

    # mbedtls sources
    mbedtls_sources = find_sources('vendor/hashlink/include/mbedtls/library', '*.c')
    sources.append(('mbedtls sources (embedded - TLS/SSL support)', mbedtls_sources))

    # Generate vcxproj
    vcxproj = VCXPROJ_HEADER

    for section_name, files in sources:
        vcxproj += f'    <!-- {section_name} -->\n'
        for f in files:
            vcxproj += f'    <ClCompile Include="{f}" />\n'
        vcxproj += '\n'

    vcxproj += VCXPROJ_FOOTER

    # Write output
    with open('hlffi_monolithic.vcxproj', 'w', newline='\r\n') as f:
        f.write(vcxproj)

    # Print summary
    total = sum(len(files) for _, files in sources)
    print(f"Generated hlffi_monolithic.vcxproj")
    print(f"Total source files: {total}")
    for name, files in sources:
        print(f"  {name}: {len(files)} files")
    print("\nTo use:")
    print("  mv hlffi_monolithic.vcxproj hlffi.vcxproj")

if __name__ == '__main__':
    main()
