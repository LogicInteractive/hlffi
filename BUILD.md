# HLFFI Build Guide

## Build Structure

HLFFI v3.0 uses a **monolithic static library** approach with **zero DLL dependencies**.

### Build Order

1. **libhl.lib** - HashLink core runtime (GC + stdlib + PCRE2) - static library
2. **hlffi.lib** - HLFFI wrapper + VM core + embedded plugins:
   - HLFFI wrapper code (6 files)
   - HashLink VM core (allocator, code, module, JIT, debugger, profile - 6 files)
   - Plugin implementations (UV, SSL - 2 files)
   - Embedded dependencies (libuv 31 files, mbedtls 108 files)
   - **Links against libhl.lib** for GC, stdlib, PCRE2
3. **Examples** - example_version, example_hello_world (link against both hlffi.lib and libhl.lib)

### Output: Zero DLLs Required

All components are compiled into **two static libraries**:
- `libhl.lib` - HashLink core runtime (GC + stdlib + PCRE2)
- `hlffi.lib` - VM core + HLFFI wrapper + plugins + embedded dependencies

**No .hdll files**, **no DLLs**, **no runtime dependencies**.

### Library Dependencies

```
libhl.lib (54 files)
  └── GC, stdlib (23 files), PCRE2 (30 files)

hlffi.lib (153 files)
  ├── depends on: libhl.lib
  ├── HLFFI wrapper (6 files)
  ├── VM core (6 files)
  ├── Plugins (2 files)
  ├── libuv (31 files)
  └── mbedtls (108 files)

Total: 207 source files, zero DLL dependencies
```

## Projects in Solution

### HashLink Core Runtime (libhl.lib)

**Project**: `vendor\hashlink\libhl.vcxproj`

**Contains**:
- `src/gc.c` - Garbage collector
- `src/std/*.c` - Standard library (23 files)
  - array, buffer, bytes, cast, date, debug, error, file, fun, maps, math, obj, process, random, regexp, socket, string, sys, thread, track, types, ucs2
- `include/pcre/*.c` - PCRE2 regex library (30 files)

**Dependencies**: ws2_32.lib, winmm.lib, user32.lib

**Output**: `bin\x64\{Debug|Release}\libhl.lib` (static)

### HLFFI Wrapper Library (hlffi.lib)

**Project**: `hlffi.vcxproj`

**Contains** (153 source files total):

*HLFFI wrapper code* (6 files):
- `src/hlffi_core.c` - Version info, error strings
- `src/hlffi_lifecycle.c` - VM lifecycle (create, init, load, entry, destroy)
- `src/hlffi_integration.c` - Integration modes (non-threaded, threaded)
- `src/hlffi_events.c` - Event loop processing (UV + haxe.EventLoop)
- `src/hlffi_threading.c` - Threading support
- `src/hlffi_reload.c` - Hot reload

*HashLink VM core* (6 files):
- `vendor/hashlink/src/allocator.c` - Memory allocation
- `vendor/hashlink/src/code.c` - Bytecode reader (`hl_code_read`)
- `vendor/hashlink/src/module.c` - Module management (`hl_module_alloc`, `hl_module_init`)
- `vendor/hashlink/src/jit.c` - JIT compiler
- `vendor/hashlink/src/debugger.c` - Debugger support
- `vendor/hashlink/src/profile.c` - Profiler

*Plugin wrappers* (2 files):
- `vendor/hashlink/libs/uv/uv.c` - libuv integration (HTTP, async I/O, timers, sockets)
- `vendor/hashlink/libs/ssl/ssl.c` - SSL/TLS integration (HTTPS)

*libuv sources* (31 files):
- `vendor/hashlink/include/libuv/src/*.c` - Embedded libuv for async I/O
- `vendor/hashlink/include/libuv/src/win/*.c` - Windows-specific sources

*mbedtls sources* (108 files):
- `vendor/hashlink/include/mbedtls/library/*.c` - Embedded TLS/SSL library

**Dependencies**: ws2_32.lib, advapi32.lib, psapi.lib, user32.lib, iphlpapi.lib, userenv.lib, winmm.lib

**Output**: `bin\x64\{Debug|Release}\hlffi.lib` (static)

### Examples

**example_version**: Simple version check program
- Links against: hlffi.lib, libhl.lib

**example_hello_world**: Full VM lifecycle demonstration
- Links against: hlffi.lib, libhl.lib

## What's Included

✅ **HashLink Core**:
- Garbage collector (Immix)
- Complete standard library (23 modules)
- PCRE2 regex engine
- JIT compiler
- Debugger support
- Profiler

✅ **Networking & I/O**:
- libuv (HTTP, async I/O, timers, sockets, file watching)
- mbedtls (SSL/TLS, HTTPS)

✅ **Embedded Dependencies**:
- No external DLLs required
- All dependencies compiled into static library

## What's NOT Included

We only build the essential HashLink components. The following are **NOT** built:

- ❌ sdl.hdll - SDL graphics
- ❌ directx.hdll - DirectX graphics
- ❌ openal.hdll - OpenAL audio
- ❌ fmt.hdll - Format library
- ❌ ui.hdll - UI framework
- ❌ video.hdll - Video playback
- ❌ heaps.hdll - Heaps game engine
- ❌ mesa.hdll - Mesa 3D
- ❌ mysql.hdll - MySQL database
- ❌ sqlite.hdll - SQLite database

## Building

### Visual Studio

1. Open `hlffi.sln` in Visual Studio 2019 or 2022
2. Select configuration: **Debug|x64** or **Release|x64**
3. Build → Build Solution (Ctrl+Shift+B)

Build order is automatic via project dependencies.

### Command Line (MSBuild)

```cmd
# Debug build
msbuild hlffi.sln /p:Configuration=Debug /p:Platform=x64

# Release build
msbuild hlffi.sln /p:Configuration=Release /p:Platform=x64
```

### Regenerating the vcxproj

If you need to modify the included sources, use the Python script:

```bash
python3 generate_monolithic_vcxproj.py
mv hlffi_monolithic.vcxproj hlffi.vcxproj
```

This regenerates `hlffi.vcxproj` with 153 source files automatically scanned from the vendor/hashlink directory.

**Note**: The script explicitly EXCLUDES gc.c, stdlib, and PCRE2 sources to avoid duplicate symbols with libhl.lib.

## Output Directory Structure

```
bin/
└── x64/
    ├── Debug/
    │   ├── libhl.lib          # HashLink runtime (static)
    │   ├── hlffi.lib          # Complete HLFFI (static)
    │   ├── example_version.exe
    │   └── example_hello_world.exe
    └── Release/
        └── (same structure)
```

**No DLLs, no .hdll files** - everything is statically linked.

## Linking Against HLFFI

### In Your Application

```cpp
// 1. Include header
#include <hlffi.h>

// 2. Link against (in this order):
//    - hlffi.lib
//    - libhl.lib
```

### Visual Studio Project Settings

**Additional Include Directories**:
- `$(HLFFI_DIR)\include`
- `$(HLFFI_DIR)\vendor\hashlink\src`

**Additional Library Directories**:
- `$(HLFFI_DIR)\bin\x64\$(Configuration)`

**Additional Dependencies**:
- `hlffi.lib`
- `libhl.lib`
- `ws2_32.lib`
- `advapi32.lib`
- `psapi.lib`
- `user32.lib`
- `iphlpapi.lib`
- `userenv.lib`
- `winmm.lib`

### Runtime Requirements

**NONE!** Both hlffi.lib and libhl.lib are static libraries. Your application is fully standalone.

No need to copy any DLLs or .hdll files - everything is embedded in your executable.

## Build Requirements

- Visual Studio 2019 or 2022
- Windows SDK 10.0
- Platform toolset v142 or v143
- C11/C++17 support

## Compiler Settings

### Preprocessor Definitions

- `_LIB` - Building static library
- `LIBHL_EXPORTS` - HashLink exports
- `HAVE_CONFIG_H` - libuv configuration
- `PCRE2_CODE_UNIT_WIDTH=16` - PCRE2 UTF-16 mode
- `MBEDTLS_USER_CONFIG_FILE=<mbedtls_user_config.h>` - mbedtls custom config

### Include Directories

- `include` - HLFFI public headers
- `vendor\hashlink\src` - HashLink VM headers
- `vendor\hashlink\include\pcre` - PCRE2 headers
- `vendor\hashlink\include\libuv\include` - libuv public headers
- `vendor\hashlink\include\libuv\src` - libuv internal headers
- `vendor\hashlink\include\mbedtls\include` - mbedtls headers
- `vendor\hashlink\libs\ssl` - SSL plugin headers

### Disabled Warnings

- 4244 - Conversion possible loss of data
- 4267 - Size_t conversion
- 4456 - Declaration hides previous local
- 4459 - Declaration hides global
- 4127 - Conditional expression is constant
- 4100 - Unreferenced parameter
- 4201 - Nameless struct/union
- 4706 - Assignment within conditional

## Notes

- **Static linking**: Everything is compiled into hlffi.lib - no dynamic loading
- **Zero DLLs**: No libhl.dll, no uv.hdll, no ssl.hdll - all embedded
- **libhl.lib**: Built from HashLink's own project (converted to static library)
- **hlffi.lib**: Monolithic library with all VM, plugins, and dependencies
- All HashLink components are x64 only
- Debug builds include full symbols (.pdb files)
- Monolithic build increases compile time but simplifies deployment

## Monolithic vs Modular

### Advantages of Monolithic Build

✅ **Simple deployment**: Just link two static libs, done
✅ **No runtime dependencies**: Everything embedded
✅ **No plugin loading**: Faster startup, no file I/O
✅ **Better optimization**: Compiler can optimize across boundaries
✅ **Easier distribution**: No DLL hell, no missing plugins

### Trade-offs

⚠️ **Larger binary**: ~2-3 MB larger executable
⚠️ **Longer compile time**: 207 source files to compile
⚠️ **No plugin modularity**: Can't swap plugins at runtime

For embedded scenarios (Unreal, Unity, game engines), the monolithic approach is preferred.
