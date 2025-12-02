# HLFFI v3.0 Build Notes

## Current Build Status (2025-01-24)

### ✅ Working Components

1. **HLFFI Static Library (hlffi.lib)**
   - Compiles successfully in Debug and Release configurations
   - Contains 6 HLFFI wrapper modules:
     - `hlffi_core.c` - Version info, utilities
     - `hlffi_lifecycle.c` - VM lifecycle (create, init, load, destroy)
     - `hlffi_integration.c` - Integration modes setup
     - `hlffi_events.c` - Event loop helpers
     - `hlffi_threading.c` - Threading mode support
     - `hlffi_reload.c` - Hot reload support
   - Location: `bin/x64/Debug/hlffi.lib`, `bin/x64/Release/hlffi.lib`

2. **HashLink Dependencies**
   - Using pre-built nightly binaries from `hashlink-3c9e352-win64`
   - `libhl.dll` and `libhl.lib` from nightly build
   - `ssl.hdll` and `uv.hdll` runtime plugins
   - Location: `vendor/hashlink/x64/Debug/` and `x64/Release/`

3. **HashLink Loader Code**
   - NOT included in hlffi.lib (intentionally)
   - Application must compile these files directly:
     - `code.c` - Bytecode loading
     - `module.c` - Module management
     - `jit.c` - JIT compilation
     - `debugger.c` - Debugger support
     - `profile.c` - Profiling support
   - Reason: These need to be in the same compilation unit as the application to resolve symbols properly

4. **Test Programs**
   - `test_hlffi_hello.exe` - Basic lifecycle test ✅
   - `test_nonblocking.exe` - NON_THREADED mode demonstration ✅
   - `test_hello.exe` - Direct HashLink API test (no HLFFI) ✅
   - All tests pass successfully!

### 🔧 Build System

Currently using **MSBuild/Visual Studio project files** (.vcxproj):
- `hlffi.vcxproj` - HLFFI library
- `test_hlffi_hello.vcxproj` - Basic test
- `test_nonblocking.vcxproj` - Non-blocking test
- `test_hello.vcxproj` - Direct HashLink test

**Note**: CMake build system exists but needs to be updated to match the current MSBuild configuration.

### 🐛 Known Issues & Workarounds

#### Issue #1: `hl_setup` Symbol Not Accessible

**Problem**: `hl_setup` is a global variable in HashLink's `sys.c` that should be exported from `libhl.dll`, but it's not accessible due to incorrect DLL export declarations in HashLink headers.

**Root Cause**:
- `hl_setup` is declared with `HL_PRIM` macro (not `HL_API`)
- `HL_PRIM` expands to `EXPORT` even when importing (should be `IMPORT`)
- This is a bug in HashLink's header configuration

**Workaround**:
- Removed all references to `hl_setup` from HLFFI code
- These values aren't critical for basic VM operation:
  - `hl_setup.sys_args` / `hl_setup.sys_nargs` - Command line arguments
  - `hl_setup.file_path` - Path to .hl file
- See: `src/hlffi_lifecycle.c` lines 129-132, 176

**Future**: If we need these values, we could:
1. Compile `sys.c` into the application (like we do with loader code)
2. Create wrapper functions in libhl to access these values
3. Submit a PR to HashLink to fix the export declarations

#### Issue #2: Application Must Include HashLink Loader Files

**Problem**: Applications must compile HashLink loader source files (`code.c`, `module.c`, etc.) directly instead of getting them from a library.

**Reason**: Symbol resolution works better when loader code is compiled into the application. Tried including in hlffi.lib but got linker errors.

**Impact**: Applications need to add these files to their build:
```xml
<ClCompile Include="vendor\hashlink\src\code.c" />
<ClCompile Include="vendor\hashlink\src\module.c" />
<ClCompile Include="vendor\hashlink\src\jit.c" />
<ClCompile Include="vendor\hashlink\src\debugger.c" />
<ClCompile Include="vendor\hashlink\src\profile.c" />
```

**Status**: This is acceptable for now, but we should revisit this pattern.

### ⏸️ Postponed: Monolithic HashLink Static Library

#### Original Plan
Build a single monolithic static library containing:
- All HashLink VM code (runtime, GC, types, etc.)
- HashLink loader code (code.c, module.c, jit.c, etc.)
- All standard library modules (.hdll code statically linked)
- HLFFI wrapper code

**Benefits**:
- Single .lib file to link against
- No DLL dependencies (except system DLLs)
- Easier distribution
- No symbol export/import issues

**Why Postponed**:
1. **Time Constraints**: Building HashLink from source as a static library is complex
2. **Pre-built Binaries Work**: The nightly DLL-based build works fine for development
3. **Symbol Issues**: The `hl_setup` issue and loader code issues suggest we need to better understand HashLink's build system first
4. **Not Blocking Progress**: We can continue Phase 1 development with the current DLL-based approach

#### When to Revisit
- Before Phase 7 (Optimization & Documentation)
- When preparing for distribution/packaging
- If DLL dependencies become problematic
- After completing Phase 2-3 to ensure the API is stable

#### What Needs to be Done
1. Build libhl as static library:
   - Modify HashLink's CMakeLists.txt or vcxproj
   - Ensure all symbols are properly exported with correct linkage
   - Static link mbedTLS (ssl) and libuv
2. Create HLFFI monolithic build:
   - Link HLFFI code with static libhl
   - Include all .hdll code statically
3. Test thoroughly:
   - Ensure no symbol conflicts
   - Verify all functionality works
   - Test on different platforms

**Reference Issue**: Track this as "TODO: Monolithic Static Library Build"

### 📝 Build Instructions (Current)

#### Building HLFFI

```bash
# Build HLFFI library (Debug)
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" hlffi.vcxproj -p:Configuration=Debug -p:Platform=x64 -v:minimal

# Build HLFFI library (Release)
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" hlffi.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal
```

#### Building Tests

```bash
# Compile Haxe test files first
cd test
haxe -hl hello.hl -main Main
haxe -hl callback.hl -main Callback
cd ..

# Build test programs
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" test_hlffi_hello.vcxproj -p:Configuration=Debug -p:Platform=x64 -v:minimal
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" test_nonblocking.vcxproj -p:Configuration=Debug -p:Platform=x64 -v:minimal
```

#### Running Tests

```bash
# Basic lifecycle test
bin/x64/Debug/test_hlffi_hello.exe test/hello.hl

# Non-blocking mode test (demonstrates NON_THREADED mode)
bin/x64/Debug/test_nonblocking.exe test/hello.hl
```

### 🎯 Next Steps

1. **Complete Phase 1 Implementation**:
   - Finish `hlffi_integration.c` stubs
   - Finish `hlffi_events.c` stubs
   - Finish `hlffi_threading.c` stubs
   - Finish `hlffi_reload.c` stubs

2. **Update CMake Build System**:
   - Sync with current MSBuild configuration
   - Ensure it handles HashLink loader files correctly
   - Add proper dependency tracking

3. **Phase 2: Type System**:
   - Implement `hlffi_types.c`
   - Add type reflection and lookup

4. **Revisit Build System** (Phase 7):
   - Monolithic static library approach
   - Fix `hl_setup` access issue
   - Cleaner integration story

### 📦 Dependencies

- **Visual Studio 2022** (v143 platform toolset)
- **Haxe 4.x** (for compiling test .hx files)
- **HashLink 1.16.0** (nightly build: hashlink-3c9e352-win64)
- **Windows 10 SDK**

### 🔗 References

- HashLink source: https://github.com/HaxeFoundation/hashlink
- HLFFI v3.0 Master Plan: [docs/MASTER_PLAN.md](docs/MASTER_PLAN.md)
- Build issues tracking: This file
