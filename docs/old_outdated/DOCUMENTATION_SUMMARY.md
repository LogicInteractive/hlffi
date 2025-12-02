# HLFFI v3.0 - Complete Documentation Summary

**Last Updated**: 2025-11-23
**Status**: Phase 0 Complete ✅ | Phase 1 In Progress 🔨
**Target Platform**: Windows (MSVC), then cross-platform

---

## 📋 Table of Contents

1. [Project Overview](#project-overview)
2. [Current Status](#current-status)
3. [Core Architecture](#core-architecture)
4. [Critical Technical Findings](#critical-technical-findings)
5. [Implementation Roadmap](#implementation-roadmap)
6. [API Design (Phase 1 Focus)](#api-design-phase-1-focus)
7. [Key Learnings from Research](#key-learnings-from-research)
8. [Documentation Map](#documentation-map)

---

## 🎯 Project Overview

**HLFFI v3.0** is a complete redesign of the HashLink FFI library for embedding the HashLink VM (Haxe runtime) into C/C++ applications.

### What It Does
- Embeds Haxe/HashLink into native applications (game engines, tools, plugins)
- Provides clean C API for VM lifecycle, function calls, and event processing
- Two integration modes: non-threaded (engine controls loop) and threaded (dedicated VM thread)
- Hot reload support for development (change Haxe code without restart)
- Zero DLL dependencies (monolithic static library build)

### Primary Use Cases
- **Game Engines**: Unreal, Unity, Godot scripting
- **Native Tools**: C/C++ applications with Haxe logic
- **Mobile Apps**: Android/iOS with native host
- **Embedded Systems**: Custom hardware with Haxe runtime

### Target Platforms
- **Phase 1**: Windows (Visual Studio 2019/2022)
- **Phase 8**: Linux, macOS, Android, iOS, WASM

---

## 📊 Current Status

### What's Complete ✅
- **Phase 0: Foundation & Architecture**
  - Project structure
  - Build system (CMake + Visual Studio)
  - Monolithic library build (207 source files)
  - Core header design
  - Research documentation (75+ sources analyzed)

### What's In Progress 🔨
- **Phase 1: VM Lifecycle + Integration Modes**
  - Core lifecycle API (create, init, load, call, destroy)
  - Non-threaded integration mode (FIRST ITERATION FOCUS)
  - Event loop processing (libuv + haxe.EventLoop)
  - Hot reload support

### First Iteration Scope
**Focus**: Non-threaded mode ONLY, C API ONLY
- Simplify: No C++ wrappers yet (may not be needed)
- Test: Engine-style integration (Unreal/Unity pattern)
- Prove: Main-thread safety, event loop processing

---

## 🏗️ Core Architecture

### 1. Two Integration Modes

#### Mode 1: Non-Threaded (FIRST ITERATION - FOCUS HERE)
**When to use**: Game engines, tools where host controls main loop

**Pattern**:
```c
// Setup (once)
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "game.hl");
hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
hlffi_call_entry(vm);  // Returns immediately!

// Main loop (every frame)
while (running) {
    float dt = calculate_delta_time();

    // Process Haxe events (non-blocking)
    hlffi_update(vm, dt);

    // Call Haxe game logic
    hlffi_call_static(vm, "Game", "update", dt);

    // Engine rendering, physics, etc.
    render_frame();
    update_physics(dt);
}

// Cleanup (process exit only!)
hlffi_destroy(vm);
```

**Key Points**:
- ✅ Entry point returns immediately (Haxe `main()` does NOT block)
- ✅ Host controls frame timing and main loop
- ✅ Non-blocking event processing (UV + Haxe timers)
- ✅ Direct function calls from host thread
- ✅ Proven pattern (production Android app, WASM implementation)

#### Mode 2: Threaded (Phase 1, NOT first iteration)
**When to use**: Haxe code has blocking `while()` loop (Android-style apps)

**Pattern**:
```c
// Dedicated VM thread + message queue for calls
// Will implement AFTER non-threaded mode works
```

**First iteration**: Skip this entirely, focus on non-threaded mode.

---

### 2. Event Loop Integration

**Two Event Loops** (both OPTIONAL):

#### libuv (Async I/O, HTTP, Timers)
- Used for: `sys.Http`, async file I/O, network sockets
- API: `uv_run(loop, UV_RUN_NOWAIT)` - non-blocking
- Detection: Weak symbol `extern uv_loop_t* uv_default_loop() __attribute__((weak));`

#### haxe.EventLoop (Haxe Timers, Callbacks)
- Used for: `haxe.Timer`, `haxe.MainLoop.add()` callbacks
- API: `EventLoop.main.loopOnce(false)` - non-blocking
- Detection: Weak symbol `extern void* haxe_EventLoop_main() __attribute__((weak));`

**Critical Insight**: Both are **OPTIONAL** - only processed if Haxe code uses them!

**Example**:
```c
// Processes both event loops (safe even if they don't exist)
hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time) {
    // Check weak symbols - only process if compiled in
    if (uv_default_loop != NULL) {
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    }

    if (haxe_EventLoop_main != NULL) {
        // Call loopOnce(false) - non-blocking
        haxe_EventLoop_main()->loopOnce(false);
    }

    return HLFFI_OK;
}
```

**Two Scenarios**:
1. **Pure HL app** (no timers): `hlffi_update()` is no-op (returns immediately)
2. **With timers**: `hlffi_update()` processes events, returns immediately

---

### 3. Build System

**Monolithic Static Library Approach**:

```
libhl.lib (54 files)
  └── GC, stdlib, PCRE2

hlffi.lib (153 files)
  ├── depends on: libhl.lib
  ├── HLFFI wrapper (6 files)
  ├── HashLink VM core (6 files)
  ├── Plugins (uv, ssl - 2 files)
  ├── libuv (31 files)
  └── mbedtls (108 files)

Total: 207 source files, zero DLL dependencies
```

**Output**: Two static libraries, link both:
```bash
# Link order matters!
yourapp.exe: main.c
    cl main.c hlffi.lib libhl.lib ws2_32.lib advapi32.lib ...
```

**No DLLs**: Everything statically linked into final executable.

---

## 🧠 Critical Technical Findings

### 1. VM Threading Model ✅ 100% VERIFIED

**Question**: Does the VM block the calling thread?
**Answer**: **NO** - Entry point returns immediately!

**Evidence** (`src/main.c:300`):
```c
int main(int argc, char* argv[]) {
    // ... setup ...

    // Line 300: Call entry point - THIS RETURNS!
    ctx.ret = hl_dyn_call_safe(&cl, NULL, 0, &isExc);

    // Line 301: CONTINUES EXECUTING after entry point!
    hl_profile_end();

    // Cleanup and exit
    hl_module_free(ctx.m);
    return 0;
}
```

**Key Insights**:
- ✅ VM core does NOT block
- ✅ Entry point returns control to host
- ❌ **BUT**: Standard Heaps apps block in `runMainLoop()` with `while(isAlive())`
- ✅ **Solution**: Don't let Haxe code call `runMainLoop()` - host controls loop

**HL/JIT vs HL/C**: **IDENTICAL** threading behavior - both return immediately!

---

### 2. VM Restart NOT Supported ❌

**Question**: Can we stop and restart the VM?
**Answer**: **NO** - HashLink VM can only be initialized ONCE per process

**Why It Fails**:
1. `hl_gc_init()` has no idempotent guard (calling twice leaks mutexes)
2. `hl_gc_free()` incomplete cleanup (doesn't free mutexes or reset state)
3. Module system static state (`cur_modules`) never cleared
4. Issue #207: Unity crashes on relaunch confirmed this

**Implications for HLFFI**:
```c
// ❌ WRONG - This will CRASH!
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
hlffi_destroy(vm);

vm = hlffi_create();  // ❌ CRASH!
hlffi_init(vm, argc, argv);

// ✅ RIGHT - Destroy only at process exit
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
// ... entire app lifetime ...
hlffi_destroy(vm);  // Only called when process exits
```

**Alternatives**:
- ✅ **Hot Reload** (change code without restart - HL/JIT only)
- ✅ **Plugin System** (runtime module loading - Nov 2025 feature)
- ✅ **Process Restart** (fork/exec for complete isolation)

**API Design**: `hlffi_destroy()` should be documented as "ONLY safe at process exit"

---

### 3. haxe.MainLoop is OPTIONAL ✅

**Question**: Does `main()` automatically initialize MainLoop?
**Answer**: **NO** - Only if Haxe code uses `haxe.Timer` or `haxe.MainLoop.add()`

**Two Scenarios**:

#### Scenario A: Pure HL App (No Timers)
```haxe
class Game {
    static function main() {
        trace("Init");
        // No timers, no MainLoop!
    }

    public static function update(dt:Float) {
        // Called from C++
    }
}
```

**Behavior**:
- ✅ `main()` returns immediately
- ✅ No `haxe.EntryPoint.run()` call (DCE removes it)
- ✅ No EventLoop compiled in
- ✅ `hlffi_update()` is no-op (returns immediately)

#### Scenario B: With Timers
```haxe
class Game {
    static function main() {
        trace("Init");

        // Using timer!
        var timer = new haxe.Timer(1000);
        timer.run = function() {
            trace("Tick");
        };
    }
}
```

**Behavior**:
- ⚠️ `haxe.MainLoop` kept from DCE (timer uses it)
- ⚠️ `haxe.EntryPoint.run()` IS called (blocks on native targets!)
- ✅ **Solution**: Override `haxe.EntryPoint.run()` to do nothing
- ✅ Call `hlffi_update()` each frame to process timer events

**HLFFI Pattern** (from your WASM code):
```c
// Use weak symbols - safe even if EventLoop doesn't exist!
extern void* haxe_EventLoop_main() __attribute__((weak));

hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time) {
    // Only process if EventLoop exists
    if (haxe_EventLoop_main == NULL) {
        return HLFFI_OK;  // No-op, returns immediately
    }

    // Process events
    void* loop = haxe_EventLoop_main();
    if (loop) {
        loop->loopOnce(false);  // Non-blocking
    }

    return HLFFI_OK;
}
```

**Key Insight**: Follow your Android/WASM pattern - use weak symbols, make it optional!

---

## 🗺️ Implementation Roadmap

### Phase 0: Foundation & Architecture ✅ COMPLETE
- Project structure
- Build system (CMake + Visual Studio)
- Monolithic library build
- Research documentation

### Phase 1: VM Lifecycle + Integration Modes 🔨 IN PROGRESS

#### First Iteration Focus (Non-Threaded Mode, C API Only)

**Deliverables**:
1. Core lifecycle functions (C API)
2. Non-threaded integration mode
3. Event loop processing (UV + Haxe)
4. Basic example (Unreal/Unity-style pattern)
5. Test suite

**Deferred to Later**:
- ❌ Threaded mode (Phase 1, later iteration)
- ❌ C++ wrappers (re-evaluate if needed)
- ❌ Hot reload (Phase 1, later iteration)

**C API Only** (First Iteration):
```c
// Lifecycle
hlffi_vm*         hlffi_create();
hlffi_error_code  hlffi_init(hlffi_vm* vm, int argc, char** argv);
hlffi_error_code  hlffi_load_file(hlffi_vm* vm, const char* path);
hlffi_error_code  hlffi_call_entry(hlffi_vm* vm);
void              hlffi_destroy(hlffi_vm* vm);  // ONLY at process exit!

// Integration Mode
hlffi_error_code  hlffi_set_integration_mode(hlffi_vm* vm, hlffi_integration_mode mode);

// Event Processing
hlffi_error_code  hlffi_update(hlffi_vm* vm, float delta_time);

// Basic Calling (Phase 1, minimal)
hlffi_error_code  hlffi_call_static(hlffi_vm* vm, const char* cls, const char* method, ...);

// Error Handling
const char*       hlffi_get_error(hlffi_vm* vm);
```

**Types**:
```c
typedef struct hlffi_vm hlffi_vm;

typedef enum {
    HLFFI_OK = 0,
    HLFFI_ERROR_NULL_VM,
    HLFFI_ERROR_NOT_INITIALIZED,
    HLFFI_ERROR_FILE_NOT_FOUND,
    HLFFI_ERROR_INVALID_BYTECODE,
    HLFFI_ERROR_TYPE_NOT_FOUND,
    HLFFI_ERROR_METHOD_NOT_FOUND,
    // ... more codes
} hlffi_error_code;

typedef enum {
    HLFFI_MODE_NON_THREADED,   // First iteration
    HLFFI_MODE_THREADED,       // Later iteration
} hlffi_integration_mode;
```

### Phase 2-9: Future Phases (After Phase 1)
- Phase 2: Type System Basics
- Phase 3: Static Member Access
- Phase 4: Object Manipulation
- Phase 5: Advanced Value Types
- Phase 6: Callbacks & Exception Handling
- Phase 7: Optimization & Documentation
- Phase 8: Cross-Platform Support
- Phase 9: Plugin System

---

## 📚 API Design (Phase 1 Focus)

### Core Lifecycle

```c
// 1. Create VM handle
hlffi_vm* vm = hlffi_create();
if (!vm) {
    fprintf(stderr, "Failed to create VM\n");
    return 1;
}

// 2. Initialize HashLink runtime
const char* args[] = {"main"};
hlffi_error_code err = hlffi_init(vm, 1, (char**)args);
if (err != HLFFI_OK) {
    fprintf(stderr, "Init failed: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);
    return 1;
}

// 3. Load bytecode
err = hlffi_load_file(vm, "game.hl");
if (err != HLFFI_OK) {
    fprintf(stderr, "Load failed: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);
    return 1;
}

// 4. Set integration mode (non-threaded)
err = hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
if (err != HLFFI_OK) {
    fprintf(stderr, "Mode failed: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);
    return 1;
}

// 5. Call entry point (returns immediately!)
err = hlffi_call_entry(vm);
if (err != HLFFI_OK) {
    fprintf(stderr, "Entry failed: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);
    return 1;
}

// 6. Main loop
float delta_time = 0.016f;  // 60 FPS
while (running) {
    // Process events (non-blocking)
    hlffi_update(vm, delta_time);

    // Call game update
    hlffi_call_static(vm, "Game", "update", delta_time);

    // Engine rendering, physics, etc.
    render_frame();
    update_physics(delta_time);

    delta_time = calculate_frame_time();
}

// 7. Cleanup (ONLY at process exit!)
hlffi_destroy(vm);
```

### Event Loop Processing

**What `hlffi_update()` Does**:
1. Process libuv events (if UV loop exists) - `uv_run(UV_RUN_NOWAIT)`
2. Process haxe.EventLoop events (if MainLoop exists) - `EventLoop.main.loopOnce(false)`
3. Both detected via weak symbols - no overhead if not used

**Implementation** (simplified):
```c
hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time) {
    if (!vm || !vm->initialized)
        return HLFFI_ERROR_NOT_INITIALIZED;

    // Check weak symbol - only process if UV exists
    extern uv_loop_t* uv_default_loop() __attribute__((weak));
    if (uv_default_loop != NULL) {
        uv_loop_t* loop = uv_default_loop();
        if (loop) {
            uv_run(loop, UV_RUN_NOWAIT);  // Non-blocking
        }
    }

    // Check weak symbol - only process if EventLoop exists
    extern void* haxe_EventLoop_main() __attribute__((weak));
    if (haxe_EventLoop_main != NULL) {
        void* loop = haxe_EventLoop_main();
        if (loop) {
            // Call loopOnce(false) via HL API
            // ... implementation ...
        }
    }

    return HLFFI_OK;
}
```

### Error Handling

**Return Codes** (all C functions):
```c
typedef enum {
    HLFFI_OK = 0,
    HLFFI_ERROR_NULL_VM,
    HLFFI_ERROR_NOT_INITIALIZED,
    HLFFI_ERROR_FILE_NOT_FOUND,
    HLFFI_ERROR_INVALID_BYTECODE,
    HLFFI_ERROR_TYPE_NOT_FOUND,
    HLFFI_ERROR_METHOD_NOT_FOUND,
    HLFFI_ERROR_INVALID_ARGS,
    HLFFI_ERROR_EXCEPTION,
    // ... more codes
} hlffi_error_code;
```

**Error Details**:
```c
// Get detailed error message
const char* hlffi_get_error(hlffi_vm* vm);

// Thread-safe - each VM has its own error buffer
```

---

## 🎓 Key Learnings from Research

### 1. From Android Production Code
**Source**: ANDROID_INSIGHTS_FOR_HLFFI.md

✅ **External loop pattern works** (proven in production)
✅ **libuv `UV_RUN_NOWAIT` is non-blocking**
✅ **Haxe EventLoop.loopOnce(false) is non-blocking**
✅ **Both use weak symbols - make them optional**

**Pattern**:
```c
// Android pattern - same for HLFFI
while (GL.handleInput()) {  // Android event loop
    UVLoop.processEvents();  // UV_RUN_NOWAIT
    update();
    render();
}
```

### 2. From WASM Implementation
**Source**: MAINLOOP_CLARIFICATION.md

✅ **MainLoop is truly OPTIONAL** (only if using timers)
✅ **Weak symbols pattern**: `if (haxe_MainLoop_tick != NULL)`
✅ **JavaScript's `requestAnimationFrame` → Native engine's `Tick()`**
✅ **Same pattern for all platforms!**

**Pattern**:
```c
// WASM pattern - use for native
extern vdynamic* haxe_MainLoop_tick() __attribute__((weak));

void mainloop_tick_callback() {
    if (haxe_MainLoop_tick != NULL) {
        haxe_MainLoop_tick();
    }
}
```

### 3. From HashLink Source Analysis
**Source**: VM_MODES_AND_THREADING_DEEP_DIVE.md

✅ **Entry point does NOT block** (verified in `src/main.c:300`)
✅ **HL/JIT and HL/C have identical threading behavior**
❌ **Heaps apps block in `runMainLoop()` - must bypass for embedding**
❌ **VM restart impossible** (non-idempotent initialization)

### 4. From Community Issues
**Source**: FFI_RESEARCH.md

- **Issue #253**: Constructor calling needs helper
- **Issue #752**: Stack top must be persistent
- **Issue #207**: Restart crashes Unity (confirmed unsupported)
- **Issue #179**: Plugin system now available (Nov 2025)

### 5. 11 Critical Gotchas
**Source**: FFI_RESEARCH.md

1. Entry point is mandatory (sets up global state)
2. Global values NULL before entry
3. Stack top must remain valid (Issue #752)
4. Constructor type mismatch (Issue #253)
5. String encoding (UTF-16, not UTF-8!)
6. Closure lifetime (must add GC roots)
7. Thread blocking (`hl_blocking()` for external I/O)
8. **VM restart NOT supported**
9. Hot reload limitations (can't change class fields)
10. Field access by name (must hash strings)
11. **VM threading model** (core doesn't block, Heaps apps do)

---

## 📖 Documentation Map

### Primary Documents

| File | Purpose | Status | Key Insights |
|------|---------|--------|--------------|
| **README.md** | Project overview, quick start | ✅ Current | Shows v3.0 architecture, features |
| **MASTER_PLAN.md** | 9-phase roadmap, API design | ✅ Current | Complete implementation guide |
| **DOCUMENTATION_SUMMARY.md** | This file - complete overview | ✅ Current | All docs in one place |

### Technical Research

| File | Purpose | Key Insight |
|------|---------|-------------|
| **FFI_RESEARCH.md** | 75+ sources, 11 gotchas, API reference | Comprehensive knowledge base |
| **ANDROID_INSIGHTS_FOR_HLFFI.md** | Production Android patterns | Event loop integration pattern |
| **MAINLOOP_CLARIFICATION.md** | When MainLoop is needed | Optional, use weak symbols |
| **HAXE_MAINLOOP_INTEGRATION.md** | How to integrate EventLoop | Custom EntryPoint pattern |
| **THREADING_MODEL_REPORT.md** | Does VM block the thread? | NO, but Heaps apps do |
| **VM_MODES_AND_THREADING_DEEP_DIVE.md** | HL/JIT vs HL/C analysis | Identical threading behavior |
| **VM_RESTART_INVESTIGATION.md** | Can VM restart? | NO - use hot reload instead |

### Build & Legacy

| File | Purpose | Status |
|------|---------|--------|
| **BUILD.md** | Build system, monolithic libs | ✅ Current |
| **CLAUDE.md** | Claude Code guidance | ✅ Current |
| **hlffi_manual.md** | Old v2.2 single-header docs | ⚠️ Legacy (being replaced) |
| **test/README.md** | Haxe test file documentation | ✅ Current |

---

## 🎯 First Iteration Priorities

### Must Have (Non-Threaded Mode, C API)
1. ✅ VM lifecycle (create, init, load, call_entry, destroy)
2. ✅ Non-threaded integration mode
3. ✅ Event loop processing (UV + Haxe with weak symbols)
4. ✅ Basic static function calling
5. ✅ Error handling (return codes + messages)
6. ✅ Example application (Unreal/Unity pattern)
7. ✅ Test suite

### Nice to Have (Later Iterations)
- ❌ Threaded mode (Phase 1, later)
- ❌ Hot reload (Phase 1, later)
- ❌ C++ wrappers (re-evaluate need)
- ❌ Advanced calling (Phase 2-4)
- ❌ Type system (Phase 2)

### Re-Evaluate C++ Wrappers
**Question**: Do we really need C++ wrappers?

**Arguments FOR**:
- RAII for automatic cleanup
- Type safety with templates
- Nicer syntax for C++ users

**Arguments AGAINST**:
- More complexity to maintain
- C API works in both C and C++
- Most game engines use C APIs (Unreal, Unity plugins)
- Can add later if users request it

**Decision**: **Start with C API ONLY**, add C++ wrappers in later phase if needed.

---

## 🔑 TL;DR: The Most Important Points

### ✅ What We Know For Sure
1. **VM is main-thread safe** (100% verified in source code)
2. **Entry point returns immediately** (does NOT block)
3. **VM restart is impossible** (use hot reload instead)
4. **MainLoop is optional** (only if using timers - use weak symbols)
5. **Event loops are non-blocking** (UV_RUN_NOWAIT + loopOnce)
6. **Your Android/WASM pattern is correct** (external loop + weak symbols)

### 🎯 First Iteration Focus
1. **Non-threaded mode ONLY**
2. **C API ONLY** (no C++ wrappers yet)
3. **Prove the pattern works** (engine-style integration)
4. **Keep it simple** (defer hot reload, threading, advanced features)

### ⚠️ Critical Constraints
1. **`hlffi_destroy()` ONLY at process exit** (can't restart VM)
2. **Don't let Haxe call `runMainLoop()`** (host controls loop)
3. **Use weak symbols for event loops** (optional, no overhead if unused)

### 🚀 Next Steps
1. Implement Phase 1 core lifecycle (C API)
2. Implement `hlffi_update()` with weak symbols
3. Write example application (Unreal/Unity pattern)
4. Test with pure HL app (no timers) AND with timers
5. Document the pattern clearly

---

**End of Summary** - See individual .md files for deep dives on specific topics.
