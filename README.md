
<img width="379" height="229" alt="HLFFI — HashLink FFI Library" src="https://github.com/user-attachments/assets/786155ad-73d4-41e3-acfb-a6af593cee07" />


---

**Embed Haxe/HashLink into any C/C++ application, game engine, or tool**

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C/C++](https://img.shields.io/badge/language-C%20%7C%20C%2B%2B-00599C)
![HashLink](https://img.shields.io/badge/HashLink-%F0%9F%94%A5-orange)

---


**HLFFI** is a HashLink FFI library for embedding Haxe/HashLink into native applications like game engines (Unreal, Unity, Godot), mobile apps, and custom C/C++ tools.


## ✨ Key Features

- **Two Integration Modes**: Non-threaded (engine controls loop) and Threaded (dedicated VM thread)
- **Event Loop Integration**: Automatic UV and haxe.EventLoop processing
- **Hot Reload Support**: Reload Haxe modules without restarting your app (HL 1.12+)
- **VM Restart Support**: Experimental support for restarting the VM within a single process
- **Clean C API**: Explicit error codes, no hidden state, C++17 RAII wrappers
- **Automatic GC Root Management**: No manual dispose needed
- **Complete Type System**: Arrays, Maps, Bytes, Enums, Abstracts, and Callbacks
- **Cross-Platform**: Windows (Visual Studio) first, Linux/macOS/Android/WASM planned

## 🚀 Quick Start (Windows/Visual Studio)

### Prerequisites

1. **Haxe**: Install from [haxe.org](https://haxe.org/download/)
2. **HashLink**: Install from [hashlink.haxe.org](https://hashlink.haxe.org/) (1.12+ for hot reload)
3. **Visual Studio 2019 or 2022**
4. **CMake 3.15+**

### Build HLFFI

```bash
# Set HashLink location (if not in standard path)
set HASHLINK_DIR=C:\HashLink

# Configure and build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Optional: Install
cmake --install . --prefix C:\HLFFI
```

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

        // Your game logic here...

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

## 📚 Documentation

### User Guide (Start Here!)

- **[Part 1: Getting Started](docs/guide/GUIDE_01_GETTING_STARTED.md)** - Your first HLFFI program
- **[Part 2: Calling Haxe](docs/guide/GUIDE_02_CALLING_HAXE.md)** - Static methods, instance methods, fields
- **[Part 3: Data Exchange](docs/guide/GUIDE_03_DATA_EXCHANGE.md)** - Arrays, Maps, Bytes, Enums
- **[Part 4: Advanced Topics](docs/guide/GUIDE_04_ADVANCED.md)** - Callbacks, hot reload, threading

### Wiki

- **[Home](docs/wiki/HOME.md)** - Overview and use cases
- **[Prerequisites](docs/wiki/PREREQUISITES.md)** - What you need to install

### API Reference

- **[API Index](docs/API_INDEX.md)** - Complete API at a glance
- **[VM Lifecycle](docs/API_01_VM_LIFECYCLE.md)** - Create, init, load, destroy
- **[Integration Modes](docs/API_02_INTEGRATION_MODES.md)** - Non-threaded vs threaded
- **[Event Loop](docs/API_03_EVENT_LOOP.md)** - UV and Haxe event processing
- **[Threading](docs/API_04_THREADING.md)** - Multi-threaded integration
- **[Hot Reload](docs/API_05_HOT_RELOAD.md)** - Update code without restart
- **[Type System](docs/API_06_TYPE_SYSTEM.md)** - Type reflection and inspection
- **[Values](docs/API_07_VALUES.md)** - Creating and extracting values
- **[Static Members](docs/API_08_STATIC_MEMBERS.md)** - Static fields and methods
- **[Instance Members](docs/API_09_INSTANCE_MEMBERS.md)** - Object creation and access
- **[Arrays](docs/API_10_ARRAYS.md)** - Array and NativeArray operations
- **[Maps](docs/API_11_MAPS.md)** - Map operations
- **[Bytes](docs/API_12_BYTES.md)** - Binary data handling
- **[Enums](docs/API_13_ENUMS.md)** - Enum value access
- **[Abstracts](docs/API_14_ABSTRACTS.md)** - Abstract type handling
- **[Callbacks](docs/API_15_CALLBACKS.md)** - C callbacks from Haxe
- **[Exceptions](docs/API_16_EXCEPTIONS.md)** - Exception handling
- **[Performance](docs/API_17_PERFORMANCE.md)** - Optimization tips
- **[Utilities](docs/API_18_UTILITIES.md)** - Helper functions
- **[Error Handling](docs/API_19_ERROR_HANDLING.md)** - Error codes and recovery

### Additional Resources

- **[HLFFI Manual](docs/hlffi_manual.md)** - Complete developer manual
- **[Callback Guide](docs/CALLBACK_GUIDE.md)** - In-depth callback patterns
- **[VM Restart](docs/VM_RESTART.md)** - Experimental VM restart support
- **[Event Loop Quickstart](docs/EVENTLOOP_QUICKSTART.md)** - Quick event loop guide

## 🏗️ Project Structure

```
hlffi/
├── include/
│   └── hlffi.h              # Public API header
├── src/
│   ├── hlffi_core.c         # Version info, utilities
│   ├── hlffi_lifecycle.c    # VM lifecycle (create, init, destroy)
│   ├── hlffi_integration.c  # Integration modes (update, threading)
│   ├── hlffi_events.c       # Event loop integration (UV, MainLoop)
│   ├── hlffi_threading.c    # Threaded mode (message queue)
│   ├── hlffi_reload.c       # Hot reload support
│   ├── hlffi_types.c        # Type system
│   ├── hlffi_values.c       # Value creation and extraction
│   ├── hlffi_objects.c      # Object manipulation
│   ├── hlffi_callbacks.c    # Callbacks & exceptions
│   ├── hlffi_maps.c         # Map operations
│   ├── hlffi_bytes.c        # Bytes handling
│   ├── hlffi_enums.c        # Enum support
│   ├── hlffi_abstracts.c    # Abstract types
│   └── hlffi_cache.c        # Performance caching
├── examples/
│   ├── hello_world/         # Basic entry point example
│   ├── example_threaded.c   # Threaded integration
│   └── ...                  # More examples
├── test/
│   ├── *.hx                 # Haxe test files
│   ├── *.hl                 # Compiled bytecode
│   └── build.bat            # Compile test files
├── docs/
│   ├── guide/               # User guide (4 parts)
│   ├── wiki/                # Wiki pages
│   └── API_*.md             # API reference (19 sections)
└── CMakeLists.txt           # Build system
```

## 🔧 CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `HLFFI_BUILD_SHARED` | `ON` | Build shared library (.dll/.so) |
| `HLFFI_BUILD_STATIC` | `ON` | Build static library (.lib/.a) |
| `HLFFI_BUILD_EXAMPLES` | `ON` | Build example programs |
| `HLFFI_BUILD_TESTS` | `ON` | Build test suite |
| `HLFFI_ENABLE_HOT_RELOAD` | `ON` | Enable hot reload (requires HL 1.12+) |
| `HASHLINK_DIR` | Auto-detect | HashLink installation path |

Example:
```bash
cmake .. -DHLFFI_BUILD_SHARED=OFF -DHASHLINK_DIR=C:\HashLink
```

## 🔑 Key Design Decisions

### 1. VM Restart Support (Experimental)

HLFFI supports restarting the HashLink VM within a single process. This is experimental and works around HashLink's design which assumes single-initialization per process.

**Use for**: Hot reload scenarios, game level reloading, testing multiple configurations

See [VM_RESTART.md](docs/VM_RESTART.md) for details and limitations.

### 2. Main Thread is Safe (100% Verified)

**Myth**: "HashLink must run on a dedicated thread"

**Reality**: VM core does NOT block unless Haxe code has a while loop. Non-threaded mode is the recommended approach for most integrations.

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

## 🤝 Contributing

HLFFI is in active development. Contributions welcome!

## 📝 License

MIT License - See [LICENSE](LICENSE) for details.

## 🙏 Acknowledgements

- [HashLink](https://github.com/HaxeFoundation/hashlink) by Haxe Foundation / Nicolas Cannasse
- [hashlink-embed](https://github.com/lalawue/hashlink-embed) Ruby library (research reference)
- [AndroidBuildTools](https://github.com/LogicInteractive/AndroidBuildTools) production patterns

## 📧 Contact

Created by [LogicInteractive](https://github.com/LogicInteractive)

---

**HLFFI v3.0.0** | Windows/Visual Studio Focus 🪟 | HashLink 1.12+ Recommended

