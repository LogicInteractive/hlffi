
<img width="379" height="229" alt="HLFFI — HashLink FFI Library" src="https://github.com/user-attachments/assets/786155ad-73d4-41e3-acfb-a6af593cee07" />


---

**Embed Haxe/HashLink into any C/C++ application, game engine, or tool**

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C/C++](https://img.shields.io/badge/language-C%20%7C%20C%2B%2B-00599C)
![HashLink](https://img.shields.io/badge/HashLink-%F0%9F%94%A5-orange)

---


**HLFFI is a HashLink FFI library, purpose-built for embedding Haxe/HashLink into native applications like game engines (Unreal, Unity, Godot), mobile apps, and custom C/C++ tools.


## ⚠️ Status : Experimental



**Phase 0: Foundation & Architecture** ✅ **COMPLETE**
**Phase 1: Core Lifecycle & Integration Modes** 🔨 **IN PROGRESS**

✅ **Working**: VM lifecycle (create, init, load, destroy), NON_THREADED mode, basic tests
🔨 **TODO**: Event loop integration, THREADED mode, hot reload

See [MASTER_PLAN.md](docs/MASTER_PLAN.md) for the complete 9-phase roadmap.
See [BUILD_NOTES.md](BUILD_NOTES.md) for current build status and known issues.

## ✨ Key Features

- **Two Integration Modes**: Non-threaded (engine controls loop) and Threaded (dedicated VM thread)
- **Event Loop Integration**: Automatic UV and haxe.EventLoop processing with weak symbols
- **Hot Reload Support**: Reload Haxe modules without restarting your app
- **Clean C API**: Explicit error codes, no hidden state, C++17 RAII wrappers
- **Automatic GC Root Management**: No manual dispose needed
- **Cross-Platform**: Windows (Visual Studio) first, Linux/macOS/Android/WASM later

## 🚀 Quick Start (Windows/Visual Studio)

### Prerequisites

1. **HashLink**: Install from [hashlink.haxe.org](https://hashlink.haxe.org/)
2. **Visual Studio 2019 or 2022**
3. **CMake 3.15+**

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

```cpp
// main.cpp
#include <hlffi.h>

int main() {
    // Phase 1: Create and initialize VM
    hlffi_vm* vm = hlffi_create();

    const char* args[] = {"main"};
    hlffi_init(vm, 1, (char**)args);

    // Load bytecode
    hlffi_load_file(vm, "game.hl");

    // Set integration mode (NON_THREADED is default)
    hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);

    // Call entry point (returns immediately, doesn't block)
    hlffi_call_entry(vm);

    // Phase 2: Main game loop (your engine controls this)
    float delta_time = 0.016f; // 60 FPS
    while (running) {
        // Update HLFFI - processes UV and haxe.EventLoop events
        hlffi_update(vm, delta_time);

        // Your game logic here...

        delta_time = calculate_frame_time();
    }

    // Phase 3: Shutdown
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

```cpp
// main.cpp
#include <hlffi.h>

int main() {
    // Create and initialize VM
    hlffi_vm* vm = hlffi_create();

    const char* args[] = {"main"};
    hlffi_init(vm, 1, (char**)args);
    hlffi_load_file(vm, "game.hl");

    // Set threaded mode
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);

    // Call entry point (may block if Haxe has while loop)
    hlffi_call_entry(vm);

    // Start dedicated VM thread
    hlffi_thread_start(vm);

    // Main thread continues...
    while (running) {
        // Call Haxe functions from other threads
        hlffi_thread_call_sync(vm, my_callback, userdata);

        // Or asynchronously with callback
        hlffi_thread_call_async(vm, my_callback, on_complete, userdata);
    }

    // Stop VM thread
    hlffi_thread_stop(vm);
    hlffi_destroy(vm);

    return 0;
}
```

## 📚 Documentation

### Core Documentation
- **[MASTER_PLAN.md](docs/MASTER_PLAN.md)** - Complete implementation roadmap (9 phases)
- **[API Reference](docs/API_REFERENCE.md)** - Full API documentation (Phase 7)

### Research & Design Decisions
- **[FFI_RESEARCH.md](docs/FFI_RESEARCH.md)** - Analysis of 75+ sources, 11 documented gotchas
- **[ANDROID_INSIGHTS_FOR_HLFFI.md](docs/ANDROID_INSIGHTS_FOR_HLFFI.md)** - Production Android implementation patterns
- **[HAXE_MAINLOOP_INTEGRATION.md](docs/HAXE_MAINLOOP_INTEGRATION.md)** - MainLoop integration guide
- **[MAINLOOP_CLARIFICATION.md](docs/MAINLOOP_CLARIFICATION.md)** - When MainLoop is needed (optional)
- **[THREADING_MODEL_REPORT.md](docs/THREADING_MODEL_REPORT.md)** - Threading model analysis
- **[VM_RESTART_INVESTIGATION.md](docs/VM_RESTART_INVESTIGATION.md)** - Why VM restart is NOT supported
- **[VM_MODES_AND_THREADING_DEEP_DIVE.md](docs/VM_MODES_AND_THREADING_DEEP_DIVE.md)** - HL/JIT vs HL/C threading (100% verified)

## 🏗️ Project Structure

```
hlffi/
├── include/
│   └── hlffi.h              # Public API header (500+ lines)
├── src/
│   ├── hlffi_core.c         # Version info, utilities
│   ├── hlffi_lifecycle.c    # VM lifecycle (create, init, destroy)
│   ├── hlffi_integration.c  # Integration modes (update, threading)
│   ├── hlffi_events.c       # Event loop integration (UV, MainLoop)
│   ├── hlffi_threading.c    # Threaded mode (message queue)
│   ├── hlffi_reload.c       # Hot reload support
│   ├── hlffi_types.c        # Type system (Phase 2)
│   ├── hlffi_static.c       # Static member access (Phase 3)
│   ├── hlffi_objects.c      # Object manipulation (Phase 4)
│   ├── hlffi_values.c       # Advanced value types (Phase 5)
│   └── hlffi_callbacks.c    # Callbacks & exceptions (Phase 6)
├── examples/
│   ├── version.c            # Simple version check
│   ├── hello_world.c        # Entry point example
│   ├── non_threaded.c       # Non-threaded integration
│   └── hot_reload.c         # Hot reload demo
├── tests/
│   └── CMakeLists.txt       # Test suite (Phase 1+)
├── docs/
│   └── *.md                 # Research & design docs
└── CMakeLists.txt           # Build system
```

## 🔧 CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `HLFFI_BUILD_SHARED` | `ON` | Build shared library (.dll/.so) |
| `HLFFI_BUILD_STATIC` | `ON` | Build static library (.lib/.a) |
| `HLFFI_BUILD_EXAMPLES` | `ON` | Build example programs |
| `HLFFI_BUILD_TESTS` | `ON` | Build test suite |
| `HASHLINK_DIR` | Auto-detect | HashLink installation path |

Example:
```bash
cmake .. -DHLFFI_BUILD_SHARED=OFF -DHASHLINK_DIR=C:\HashLink
```

## 🎯 Implementation Roadmap

- ✅ **Phase 0**: Foundation & Architecture (COMPLETE)
  - Project structure, build system, core header
- 🔨 **Phase 1**: Core Lifecycle & Integration Modes (IN PROGRESS)
  - VM lifecycle, non-threaded/threaded modes, event loops, hot reload
- ⏳ **Phase 2**: Type System Basics
- ⏳ **Phase 3**: Static Member Access
- ⏳ **Phase 4**: Object Manipulation
- ⏳ **Phase 5**: Advanced Value Types
- ⏳ **Phase 6**: Callbacks & Exception Handling
- ⏳ **Phase 7**: Optimization & Documentation
- ⏳ **Phase 8**: Cross-Platform Support
- ⏳ **Phase 9**: Plugin System (Future)

## 🔑 Key Design Decisions (From Research Phase)

### 1. VM Restart NOT Supported
**Why**: HashLink's GC initialization is non-idempotent (`hl_gc_init()` can't be called twice)
**Solution**: Use hot reload instead (`hlffi_reload_module()`)
**Source**: [VM_RESTART_INVESTIGATION.md](docs/VM_RESTART_INVESTIGATION.md)

### 2. Main Thread is Safe (100% Verified)
**Myth**: "HashLink must run on a dedicated thread"
**Reality**: VM core does NOT block unless Haxe code has a while loop
**Source**: [VM_MODES_AND_THREADING_DEEP_DIVE.md](docs/VM_MODES_AND_THREADING_DEEP_DIVE.md)

### 3. haxe.MainLoop is OPTIONAL
**When needed**: Only if Haxe code uses `haxe.Timer` or `haxe.MainLoop.add()`
**Detection**: Weak symbols at runtime
**Source**: [MAINLOOP_CLARIFICATION.md](docs/MAINLOOP_CLARIFICATION.md)

### 4. Two Integration Patterns
**Pattern A (Android)**: Haxe controls loop with `while()` - entry point blocks
**Pattern B (Unreal/Unity)**: Engine controls loop - entry point returns
**Source**: [ANDROID_INSIGHTS_FOR_HLFFI.md](docs/ANDROID_INSIGHTS_FOR_HLFFI.md)

### 5. Event Loop Processing
**UV Loop**: `uv_run(loop, UV_RUN_NOWAIT)` - non-blocking
**Haxe EventLoop**: `EventLoop.main.loopOnce(false)` - non-blocking
**Both**: Called every frame in `hlffi_update()`
**Source**: [ANDROID_INSIGHTS_FOR_HLFFI.md](docs/ANDROID_INSIGHTS_FOR_HLFFI.md)

### 6. No Manual GC Root Management
**Old way**: Manual `hl_add_root()` / `hl_remove_root()` (error-prone)
**New way**: Automatic root management in HLFFI (Issue #624 pattern)
**Source**: [FFI_RESEARCH.md](docs/FFI_RESEARCH.md) - Gotcha #9

## 🤝 Contributing

HLFFI v3.0 is currently in active development. See [MASTER_PLAN.md](docs/MASTER_PLAN.md) for the roadmap.

## 📝 License

MIT License - See [LICENSE](LICENSE) for details.

## 🙏 Acknowledgements

- [HashLink](https://github.com/HaxeFoundation/hashlink) by Haxe Foundation / Nicolas Cannasse
- [hashlink-embed](https://github.com/lalawue/hashlink-embed) Ruby library (research reference)
- [AndroidBuildTools](https://github.com/LogicInteractive/AndroidBuildTools) production patterns

## 📧 Contact

Created by [LogicInteractive](https://github.com/LogicInteractive)

---

**Status**: Phase 0 Complete ✅ | Phase 1 In Progress 🔨 | Windows/Visual Studio Focus 🪟
