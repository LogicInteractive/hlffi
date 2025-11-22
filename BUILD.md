# HLFFI Build Guide

## Build Structure

HLFFI uses a multi-project Visual Studio solution with proper dependency ordering.

### Build Order

1. **libhl.dll** - HashLink core runtime (GC + stdlib + PCRE2)
2. **uv.hdll** - libuv plugin (HTTP, async I/O, timers, sockets)
3. **ssl.hdll** - SSL/TLS plugin (HTTPS via mbedtls)
4. **hlffi.lib** - HLFFI wrapper + HashLink VM core
5. **Examples** - example_version, example_hello_world

## Projects in Solution

### HashLink Core Runtime (libhl.dll)

**Project**: `vendor\hashlink\libhl.vcxproj`

**Contains**:
- `src/gc.c` - Garbage collector
- `src/std/*.c` - Standard library (27 files)
  - array, buffer, bytes, cast, date, debug, error, file, fun, maps, math, obj, process, random, regexp, socket, string, sys, thread, track, types, ucs2
- `include/pcre/*.c` - PCRE2 regex library (27 files)

**Dependencies**: ws2_32.lib, winmm.lib, user32.lib

**Output**: `bin\x64\{Debug|Release}\libhl.dll`

### libuv Plugin (uv.hdll)

**Project**: `vendor\hashlink\libs\uv\uv.vcxproj`

**Contains**:
- `libs/uv/uv.c` - libuv integration for HashLink
- Links against external libuv library

**Purpose**: Async I/O, HTTP, timers, sockets, file watching

**Depends on**: libhl.dll

**Output**: `bin\x64\{Debug|Release}\uv.hdll`

### SSL Plugin (ssl.hdll)

**Project**: `vendor\hashlink\libs\ssl\ssl.vcxproj`

**Contains**:
- `libs/ssl/ssl.c` - SSL/TLS integration via mbedtls
- Embedded mbedtls library

**Purpose**: HTTPS, SSL/TLS connections

**Depends on**: libhl.dll

**Output**: `bin\x64\{Debug|Release}\ssl.hdll`

### HLFFI Wrapper Library (hlffi.lib)

**Project**: `hlffi.vcxproj`

**Contains**:

*HLFFI wrapper code*:
- `src/hlffi_core.c` - Version info, error strings
- `src/hlffi_lifecycle.c` - VM lifecycle (create, init, load, entry, destroy)
- `src/hlffi_integration.c` - Integration modes (non-threaded, threaded)
- `src/hlffi_events.c` - Event loop processing (UV + haxe.EventLoop)
- `src/hlffi_threading.c` - Threading support
- `src/hlffi_reload.c` - Hot reload

*HashLink VM core* (compiled directly into hlffi.lib):
- `vendor/hashlink/src/allocator.c` - Memory allocation
- `vendor/hashlink/src/code.c` - Bytecode reader (`hl_code_read`)
- `vendor/hashlink/src/module.c` - Module management (`hl_module_alloc`, `hl_module_init`)
- `vendor/hashlink/src/jit.c` - JIT compiler
- `vendor/hashlink/src/debugger.c` - Debugger support
- `vendor/hashlink/src/profile.c` - Profiler

**Depends on**: libhl.dll, uv.hdll, ssl.hdll

**Output**: `bin\x64\{Debug|Release}\hlffi.lib`

### Examples

**example_version**: Simple version check program

**example_hello_world**: Full VM lifecycle demonstration

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

## Output Directory Structure

```
bin/
└── x64/
    ├── Debug/
    │   ├── libhl.dll          # HashLink runtime
    │   ├── libhl.pdb
    │   ├── uv.hdll            # UV plugin
    │   ├── ssl.hdll           # SSL plugin
    │   ├── hlffi.lib          # HLFFI wrapper + VM core
    │   ├── example_version.exe
    │   └── example_hello_world.exe
    └── Release/
        └── (same structure)
```

## Linking Against HLFFI

### In Your Application

```cpp
// 1. Include header
#include <hlffi.h>

// 2. Link against:
//    - hlffi.lib
//    - libhl.lib (import library for libhl.dll)
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

### Runtime Requirements

Your application must have these DLLs available at runtime:
- `libhl.dll` - HashLink core runtime
- `uv.hdll` - If using HTTP, async I/O, timers
- `ssl.hdll` - If using HTTPS

Copy from `bin\x64\{Debug|Release}\` to your application's directory.

## Build Requirements

- Visual Studio 2019 or 2022
- Windows SDK 10.0
- Platform toolset v142 or v143
- C11/C++17 support

## Notes

- **libhl.dll** is built from HashLink's own project (unmodified)
- **hlffi.lib** compiles HashLink VM core files directly (no need to modify HashLink submodule)
- Plugins (uv, ssl) are built from HashLink's plugin projects (unmodified)
- All HashLink components are x64 only
- Debug builds include full symbols (.pdb files)
