
<img width="379" height="229" alt="HLFFI — HashLink FFI Library" src="https://github.com/user-attachments/assets/786155ad-73d4-41e3-acfb-a6af593cee07" />

---

**Embed Haxe/HashLink into any C/C++ application, game engine, or tool**

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C/C++](https://img.shields.io/badge/language-C%20%7C%20C%2B%2B-00599C)
![HashLink](https://img.shields.io/badge/HashLink-%F0%9F%94%A5-orange)

---

HLFFI is a C/C++ library that embeds [HashLink](https://hashlink.haxe.org/) into any application. It supports both **JIT mode** (runtime `.hl` bytecode loading) and **HLC mode** (HashLink/C - Haxe compiled to C). This makes it ideal for embedding Haxe in game engines, tools, and applications across desktop and mobile platforms.

```c
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Call Haxe functions from C
    hlffi_call_static(vm, "Game", "start", 0, NULL);

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

---

## Overview

HashLink provides a high-performance runtime for Haxe, but embedding it in C/C++ applications requires working with its internal APIs directly. HLFFI provides a production-ready abstraction layer that handles the complexity of VM lifecycle management, garbage collection, type conversions, and threading.

### Two Execution Modes

| Mode | Library | Use Case | Platforms |
|------|---------|----------|-----------|
| **JIT** | `hlffi_jit.lib` | Load `.hl` bytecode at runtime | Windows, Linux, macOS |
| **HLC** | `hlffi_hlc.lib` | Compile Haxe to C, link statically | All platforms including iOS, Android, consoles |

**JIT Mode**: Best for development - fast iteration with hot reload support. Load `.hl` bytecode files at runtime.

**HLC Mode**: Best for production/mobile - Haxe compiles to C code that gets compiled and linked with your application. Required for platforms that don't allow JIT (iOS, consoles).

### What it does

- Manages HashLink VM initialization, bytecode loading, and cleanup
- Provides automatic GC root management for values passed between C and Haxe
- Handles type conversions between C primitives and HashLink's type system
- Integrates with libuv and Haxe's event loop automatically
- Supports both engine-controlled and threaded integration modes
- Enables hot reload of Haxe modules without application restart (JIT mode)

**Problem solved:** Without HLFFI, embedding HashLink requires manual management of GC roots (`hl_add_root`/`hl_remove_root`), raw `vdynamic*` pointer manipulation, manual thread registration, and integration of multiple event loops. HLFFI encapsulates these operations into a consistent API with clear error handling and automatic resource management.

---

## Key Features

- **Dual Mode Support**: JIT (`.hl` bytecode) and HLC (Haxe-to-C) in the same API
- **Two Integration Modes**: Non-threaded (engine controls loop) and Threaded (dedicated VM thread)
- **Event Loop Integration**: Automatic UV and haxe.EventLoop processing
- **Hot Reload Support**: Reload Haxe modules without restarting your app (JIT mode, HL 1.12+)
- **Clean C API**: Explicit error codes, no hidden state
- **Automatic GC Root Management**: No manual dispose needed
- **Full Object Control**: Create, destroy, call methods, and access members on Haxe objects from C
- **Complete Type System**: Arrays, Maps, Bytes, Enums, Abstracts, and Callbacks
- **Performance Caching**: Cache method lookups for 60x speedup in hot paths
- **Cross-Platform**: Windows, Linux, macOS, with HLC enabling iOS/Android/consoles

---

## Quick Start

### Prerequisites

1. **Haxe**: Install from [haxe.org](https://haxe.org/download/)
2. **Visual Studio 2019 or 2022** (Windows) or GCC/Clang (Linux/macOS)
3. **CMake 3.15+**

### Build HLFFI

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

This builds:
- `hlffi_jit.lib` - JIT mode library (for loading .hl bytecode)
- `hlffi_hlc.lib` - HLC mode library (for Haxe compiled to C)

**Note:** You need `libhl.dll` from a HashLink installation to run JIT mode applications. Download from [hashlink.haxe.org](https://hashlink.haxe.org/) or build from vendor/hashlink with `-DHLFFI_BUILD_LIBHL=ON`.

### Integration Example: Non-Threaded Mode (Recommended)

**Use when**: Your engine/application controls the main loop (Unreal, Unity, Godot, custom game loops)

```c
// main.c
#include <hlffi.h>

int main()
{
    // Phase 1: Create and initialize VM
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);

    // Load bytecode
    hlffi_load_file(vm, "game.hl");

    // Set integration mode (NON_THREADED is default)
    hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);

    // Call entry point (returns immediately, doesn't block)
    hlffi_call_entry(vm);

    // Phase 2: Main game loop (your engine controls this)
    float delta_time = 0.016f; // 60 FPS
    bool running = true;
    while (running)
    {
        // Update HLFFI - processes UV and haxe.EventLoop events
        hlffi_update(vm, delta_time);

        // Call Haxe game logic
        hlffi_call_static(vm, "Game", "update", 0, NULL);

        // Your engine logic here...

        delta_time = calculate_frame_time();
    }

    // Phase 3: Shutdown
    hlffi_close(vm);
    hlffi_destroy(vm);

    return 0;
}
```

**What `hlffi_update()` does**:
1. Processes libuv events (if UV loop exists) - `uv_run(UV_RUN_NOWAIT)`
2. Processes haxe.EventLoop events (if MainLoop exists) - `EventLoop.main.loopOnce(false)`
3. Both are detected via weak symbols - no overhead if not used

### Integration Example: Threaded Mode (Advanced)

**Use when**: You want Haxe to run on a dedicated thread with message queue communication

```c
// main.c
#include <hlffi.h>

int main()
{
    // Create and initialize VM
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");

    // Set threaded mode
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);

    // Start dedicated VM thread (calls entry point internally)
    hlffi_thread_start(vm);

    // Main thread continues...
    bool running = true;
    while (running)
    {
        // Call Haxe functions from other threads
        hlffi_thread_call_sync(vm, my_callback, userdata);

        // Or asynchronously with callback
        hlffi_thread_call_async(vm, my_callback, on_complete, userdata);
    }

    // Stop VM thread
    hlffi_thread_stop(vm);
    hlffi_close(vm);
    hlffi_destroy(vm);

    return 0;
}
```

---

## Documentation

### User Guide (Start Here!)

- **[Part 1: Getting Started](docs/guide/GUIDE_01_GETTING_STARTED.md)** - Your first HLFFI program
- **[Part 2: Calling Haxe](docs/guide/GUIDE_02_CALLING_HAXE.md)** - Static methods, instance methods, fields
- **[Part 3: Data Exchange](docs/guide/GUIDE_03_DATA_EXCHANGE.md)** - Arrays, Maps, Bytes, Enums
- **[Part 4: Advanced Topics](docs/guide/GUIDE_04_ADVANCED.md)** - Callbacks, hot reload, threading

### Wiki

- **[Home](docs/wiki/HOME.md)** - Overview and use cases
- **[Prerequisites](docs/wiki/PREREQUISITES.md)** - What you need to install

### API Reference

- **[API Index](docs/API_INDEX.md)** - Complete API at a glance (~130 functions)
- **[API Reference](docs/API_REFERENCE.md)** - Full reference by category

#### Core VM

- **[VM Lifecycle](docs/API_01_VM_LIFECYCLE.md)** - Create, init, load, destroy
- **[Integration Modes](docs/API_02_INTEGRATION_MODES.md)** - Non-threaded vs threaded
- **[Event Loop](docs/API_03_EVENT_LOOP.md)** - UV and Haxe event processing
- **[Threading](docs/API_04_THREADING.md)** - Multi-threaded integration
- **[Hot Reload](docs/API_05_HOT_RELOAD.md)** - Update code without restart

#### Type System & Values

- **[Type System](docs/API_06_TYPE_SYSTEM.md)** - Type reflection and inspection
- **[Values](docs/API_07_VALUES.md)** - Creating and extracting values
- **[Static Members](docs/API_08_STATIC_MEMBERS.md)** - Static fields and methods
- **[Instance Members](docs/API_09_INSTANCE_MEMBERS.md)** - Object creation and access

#### Collections & Data Types

- **[Arrays](docs/API_10_ARRAYS.md)** - Array and NativeArray operations
- **[Maps](docs/API_11_MAPS.md)** - Map operations
- **[Bytes](docs/API_12_BYTES.md)** - Binary data handling
- **[Enums](docs/API_13_ENUMS.md)** - Enum value access
- **[Abstracts](docs/API_14_ABSTRACTS.md)** - Abstract type handling

#### Advanced Features

- **[Callbacks](docs/API_15_CALLBACKS.md)** - C callbacks from Haxe
- **[Exceptions](docs/API_16_EXCEPTIONS.md)** - Exception handling
- **[Performance](docs/API_17_PERFORMANCE.md)** - Method caching for 60x speedup
- **[Utilities](docs/API_18_UTILITIES.md)** - Worker threads, blocking guards
- **[Error Handling](docs/API_19_ERROR_HANDLING.md)** - Error codes and recovery

### Additional Resources

- **[HLC Support](docs/HLC_SUPPORT.md)** - Complete guide to HLC mode and ARM/mobile platforms

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `HLFFI_BUILD_EXAMPLES` | `ON` | Build example programs |
| `HLFFI_BUILD_TESTS` | `ON` | Build test suite |
| `HLFFI_ENABLE_HOT_RELOAD` | `ON` | Enable hot reload (requires HL 1.12+) |
| `HLFFI_HLC_MODE` | `OFF` | Set default `hlffi` alias to HLC mode |
| `HLFFI_BUILD_LIBHL` | `OFF` | Also build libhl from vendor/hashlink |

Example:
```bash
# Build HLFFI only (requires external libhl.dll)
cmake ..

# Build HLFFI + libhl from vendor/hashlink
cmake .. -DHLFFI_BUILD_LIBHL=ON
```

---

## Key Design Decisions

### 1. JIT vs HLC Mode Selection

Choose based on your target platform and development stage:

| Consideration | JIT Mode | HLC Mode |
|---------------|----------|----------|
| Development iteration | Fast (just recompile .hl) | Slower (recompile C) |
| Hot reload | Supported | Not supported |
| iOS/Android/Consoles | Not available | Required |
| Binary size | Smaller (bytecode separate) | Larger (code embedded) |
| Startup time | Slightly slower (JIT compile) | Faster |

### 2. Main Thread is Safe

VM core does NOT block unless Haxe code has a blocking while loop. Non-threaded mode is the recommended approach for most integrations.

### 3. haxe.MainLoop is OPTIONAL

**When needed**: Only if Haxe code uses `haxe.Timer` or `haxe.MainLoop.add()`

**Detection**: Weak symbols at runtime - no overhead if not used

### 4. Two Integration Patterns

**Pattern A (Non-Threaded)**: Engine controls loop - entry point returns immediately
- Recommended for: Unreal, Unity, Godot, custom game loops
- Call `hlffi_update()` every frame

**Pattern B (Threaded)**: Haxe controls loop with `while()` - runs on dedicated thread
- Use when: Haxe code has blocking loop (Android pattern)
- Use `hlffi_thread_call_sync()` for thread-safe calls

### 5. Event Loop Processing

**UV Loop**: `uv_run(loop, UV_RUN_NOWAIT)` - non-blocking

**Haxe EventLoop**: `EventLoop.main.loopOnce(false)` - non-blocking

**Both**: Called every frame in `hlffi_update()` when event loops exist

### 6. Automatic GC Root Management

**Old way**: Manual `hl_add_root()` / `hl_remove_root()` (error-prone)

**HLFFI way**: Automatic root management - objects created with `hlffi_new()` are GC-rooted until freed with `hlffi_value_free()`

### 7. Performance Caching

For hot paths (game loops, per-frame calls), cache method lookups:

```c
// Slow: Hash lookup every call (~40ns overhead)
for (int i = 0; i < 1000; i++)
    hlffi_call_static(vm, "Game", "update", 0, NULL);

// Fast: Cache once, call many times (~0.7ns overhead - 60x faster)
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
for (int i = 0; i < 1000; i++)
    hlffi_call_cached(update, 0, NULL);
hlffi_cache_free(update);
```

See [API_17_PERFORMANCE.md](docs/API_17_PERFORMANCE.md) for details.

---

## Contributing

HLFFI is in active development. Contributions welcome!

---

## License

MIT License - Same as HashLink. See [LICENSE](LICENSE) for details.

---

## Acknowledgements

- [HashLink](https://github.com/HaxeFoundation/hashlink) by Haxe Foundation / Nicolas Cannasse
- [hashlink-embed](https://github.com/lalawue/hashlink-embed) Ruby library (research reference)

---

## Contact

Created by [LogicInteractive](https://github.com/LogicInteractive)

---

**HLFFI v3.0.0** | JIT + HLC Support | Windows, Linux, macOS + mobile via HLC
