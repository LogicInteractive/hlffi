# Android Production Insights for HLFFI

**Source**: User's AndroidBuildTools repository - real-world production HashLink/C Android implementation

---

## Key Pattern: Entry Point + External Event Loop

### 1. Entry Point Calling

**File**: `native/rawdrawandroid/rawdraw/CNFGEGLDriver.c` (line 541-562)

```c
void android_main(struct android_app* app) {
    int main(int argc, char **argv);  // Forward declare HL entry point
    char mainptr[5] = { 'm', 'a', 'i', 'n', 0 };
    char *argv[] = { mainptr, 0 };

    gapp = app;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    printf("Starting with Android SDK Version: %d\n", android_sdk_version);
    main(1, argv);  // ← CALL ENTRY POINT, RETURNS IMMEDIATELY!
    printf("Main Complete\n");  // ← THIS EXECUTES! Proves main() returned!
}
```

**Key insight**: Entry point is called ONCE and RETURNS immediately.

---

### 2. Event Loop Integration - libuv

**File**: `native/uv_integration.c`

```c
// Global UV loop - HashLink will use this
static uv_loop_t* g_uv_loop = NULL;

// Initialize UV loop (called from Haxe)
HL_PRIM uv_loop_t* HL_NAME(uv_init_loop)() {
    if (g_uv_loop == NULL) {
        g_uv_loop = uv_default_loop();
        if (g_uv_loop) {
            printf("[UV] Event loop initialized\n");
        }
    }
    return g_uv_loop;
}

// Process UV events (non-blocking) - call this every frame
HL_PRIM int HL_NAME(uv_process_events)() {
    if (g_uv_loop == NULL) {
        HL_NAME(uv_init_loop)();
    }

    // Run UV loop in non-blocking mode (UV_RUN_NOWAIT)
    // This processes pending events and returns immediately
    int result = uv_run(g_uv_loop, UV_RUN_NOWAIT);  // ← NON-BLOCKING!
    return result;
}

// Check if UV loop has pending work
HL_PRIM bool HL_NAME(uv_has_pending)() {
    if (g_uv_loop == NULL) return false;
    return uv_loop_alive(g_uv_loop) != 0;
}

DEFINE_PRIM(_ABSTRACT(uv_loop), uv_init_loop, _NO_ARG);
DEFINE_PRIM(_I32, uv_process_events, _NO_ARG);
DEFINE_PRIM(_BOOL, uv_has_pending, _NO_ARG);
```

**Key insight**: UV loop runs in `UV_RUN_NOWAIT` mode - processes pending events and returns immediately.

---

### 3. Haxe Wrapper - UVLoop.hx

**File**: `src/android/system/UVLoop.hx`

```haxe
package android.system;

// UV Event Loop integration for Haxe
@:hlNative("uv")
class UVLoop {
    @:hlNative("uv", "default_loop")
    static function getLoop():hl.Abstract<"uv_loop"> { return null; }

    @:hlNative("uv", "run")
    static function uvRun(loop:hl.Abstract<"uv_loop">, mode:Int):Int { return 0; }

    @:hlNative("uv", "loop_alive")
    static function uvLoopAlive(loop:hl.Abstract<"uv_loop">):Int { return 0; }

    static var loop:hl.Abstract<"uv_loop">;

    public static function init():Void {
        loop = getLoop();
    }

    // UV_RUN_NOWAIT = 1 (non-blocking)
    public static function processEvents():Int {
        if (loop == null) init();
        return uvRun(loop, 1);  // ← Mode 1 = UV_RUN_NOWAIT
    }

    public static function hasPending():Bool {
        if (loop == null) return false;
        return uvLoopAlive(loop) != 0;
    }
}
```

**Key insight**: Simple wrapper that calls native UV functions with non-blocking mode.

---

### 4. Main Application - MainO.hx

**File**: `src/MainO.hx` (lines 18-198)

```haxe
class MainO {
    public static function main() {
        trace("=== Haxe Android Main class starting! ===");

        // Initialize UV event loop FIRST
        UVLoop.init();  // ← Initialize once

        // Setup GL, NanoVG, Touch, etc.
        GL.setup("Haxe Android App", 0, 0);
        Touch.init();
        // ... more initialization ...

        // HTTP test - using haxe.Http with UV event loop
        callHttp();

        // Main render loop - exits when handleInput() returns false
        while (GL.handleInput()) {  // ← External loop (Android)
            // Update touch input
            Touch.update();

            // Process UV events (non-blocking) - handles async I/O, timers, HTTP, etc.
            UVLoop.processEvents();  // ← CALLED EVERY FRAME!

            // Get dimensions
            var w = 0, h = 0;
            GL.getDimensions(w, h);

            // Render with NanoVG
            if (nvgInitialized && nvg != null && w > 0 && h > 0) {
                nvg.beginFrame(w, h, 1.0);
                yogaVisualTest.render(nvg, w, h);
                nvg.endFrame();
            }

            // Swap buffers
            GL.swap();
        }

        trace("Main loop exited");
    }
}
```

**Key insights**:
- `main()` runs and enters a **WHILE LOOP** (doesn't return)
- BUT: The loop is **driven by Android** (`GL.handleInput()` polls Android events)
- `UVLoop.processEvents()` called **EVERY FRAME** (non-blocking)
- Timers, async I/O, HTTP - all handled by UV loop

---

## The Complete Flow

```
Android Main Thread (NativeActivity)
    ↓
android_main(app)
    ↓
main(1, argv)  // HL entry point
    ↓
main() {
    UVLoop.init();  // Initialize UV loop

    while (GL.handleInput()) {  // Android event loop
        UVLoop.processEvents();  // Process UV events (UV_RUN_NOWAIT)
        // Render frame
        GL.swap();
    }
}
```

**Thread model**:
- ✅ Everything runs on **Android main thread**
- ✅ No dedicated HL thread
- ✅ No blocking
- ✅ Android drives the loop

---

## Comparison: UV vs haxe.EventLoop

| Aspect | UV Loop (Android) | haxe.EventLoop (WASM/Native) |
|--------|-------------------|------------------------------|
| **Library** | libuv (C library) | Haxe standard library |
| **Used for** | Timers, async I/O, HTTP | Timers, callbacks, promises |
| **API** | `uv_run(loop, UV_RUN_NOWAIT)` | `EventLoop.main.loopOnce()` |
| **Mode** | Non-blocking (NOWAIT) | Non-blocking (loopOnce) |
| **Pattern** | External loop calls `processEvents()` | External loop calls `loopOnce()` |
| **When needed** | If using async I/O, HTTP, UV timers | If using `haxe.Timer`, `haxe.MainLoop` |

**Key insight**: Same pattern works for both!

---

## HLFFI API Design - Based on Android Pattern

### Pattern 1: Main Thread Integration

```c
// HLFFI API for event loop integration
typedef enum {
    HLFFI_EVENTLOOP_UV,       // libuv (async I/O, HTTP)
    HLFFI_EVENTLOOP_HAXE      // haxe.EventLoop (timers, callbacks)
} hlffi_eventloop_type;

// Process events (non-blocking) - call this every frame
hlffi_error_code hlffi_process_events(hlffi_vm* vm, hlffi_eventloop_type type);

// Check if event loop has pending work
bool hlffi_has_pending_events(hlffi_vm* vm, hlffi_eventloop_type type);
```

**Implementation**:
```c
hlffi_error_code hlffi_process_events(hlffi_vm* vm, hlffi_eventloop_type type) {
    if (!vm || !vm->initialized)
        return HLFFI_ERROR_NOT_INITIALIZED;

    switch (type) {
        case HLFFI_EVENTLOOP_UV:
            // Get UV loop (weak symbol, optional)
            extern uv_loop_t* uv_default_loop() __attribute__((weak));
            if (uv_default_loop != NULL) {
                uv_loop_t* loop = uv_default_loop();
                uv_run(loop, UV_RUN_NOWAIT);  // Non-blocking
            }
            break;

        case HLFFI_EVENTLOOP_HAXE:
            // Get haxe.EventLoop (weak symbol, optional)
            extern void* haxe_EventLoop_main() __attribute__((weak));
            if (haxe_EventLoop_main != NULL) {
                void* loop = haxe_EventLoop_main();
                // Call loopOnce(false) method
                // ... implementation
            }
            break;
    }

    return HLFFI_OK;
}
```

### Usage Example: Unreal Engine

```cpp
// Unreal Engine integration (following Android pattern)
void UHaxeComponent::BeginPlay() {
    Super::BeginPlay();

    vm = hlffi_create();
    hlffi_init(vm, 0, nullptr);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);  // ✅ Runs main(), enters game loop

    // ⚠️ If Haxe main() has a while loop, this will block!
    // For embedding, Haxe code should NOT have a main loop
    // Instead, export update functions to call from TickComponent
}

void UHaxeComponent::TickComponent(float DeltaTime, ...) {
    Super::TickComponent(DeltaTime, ...);

    // Process UV events (if using async I/O, HTTP)
    hlffi_process_events(vm, HLFFI_EVENTLOOP_UV);

    // Process Haxe events (if using haxe.Timer, haxe.MainLoop)
    hlffi_process_events(vm, HLFFI_EVENTLOOP_HAXE);

    // Call game update
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

**⚠️ CRITICAL DIFFERENCE**:
- **Android**: Haxe `main()` HAS a while loop (`while (GL.handleInput())`) - blocks until app quits
- **Unreal/Unity**: Haxe `main()` should NOT have a while loop - just initialization, then return
- Engine tick calls update functions repeatedly

---

## Two Embedding Patterns

### Pattern A: Haxe Controls Main Loop (Android)

**Haxe code**:
```haxe
class Main {
    static function main() {
        init();

        // Haxe controls the loop!
        while (shouldRun()) {
            processEvents();
            update();
            render();
        }
    }
}
```

**C++ code**:
```cpp
int main() {
    vm = hlffi_create();
    hlffi_call_entry(vm);  // ← BLOCKS here until app quits
    hlffi_destroy(vm);
    return 0;
}
```

**Use cases**: Standalone apps, Android, mobile

---

### Pattern B: Engine Controls Main Loop (Unreal/Unity)

**Haxe code**:
```haxe
class Game {
    static function main() {
        init();  // Just initialization
        // NO WHILE LOOP!
    }

    public static function update(dt:Float) {
        // Called from engine tick
    }
}
```

**C++ code**:
```cpp
void BeginPlay() {
    vm = hlffi_create();
    hlffi_call_entry(vm);  // ✅ Returns immediately (no loop)
}

void Tick(float DeltaTime) {
    hlffi_process_events(vm, HLFFI_EVENTLOOP_UV);
    hlffi_process_events(vm, HLFFI_EVENTLOOP_HAXE);
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

**Use cases**: Unreal, Unity, game engines

---

## Key Takeaways for HLFFI

### 1. Entry Point Behavior
- ✅ **CAN call entry point on main thread** - proven in production Android app
- ✅ **Entry point MAY or MAY NOT return** - depends on Haxe code
- ⚠️ **If Haxe has while loop** - entry point blocks (Pattern A)
- ✅ **If Haxe has no loop** - entry point returns (Pattern B)

### 2. Event Loop Integration
- ✅ **Use weak symbols** to detect if UV/EventLoop exists
- ✅ **Call processEvents() every frame** - non-blocking
- ✅ **Support both UV and haxe.EventLoop** - different use cases

### 3. Threading Model
- ✅ **Main thread is recommended** - proven pattern
- ✅ **No dedicated HL thread needed** - for most use cases
- ✅ **Non-blocking** - all event processing returns immediately

### 4. Documentation
- ⚠️ **Clarify the two patterns** - Haxe loop vs Engine loop
- ⚠️ **Provide examples for both** - Android pattern vs Unreal pattern
- ⚠️ **Explain when to use each** - standalone vs embedded

---

## Updated MASTER_PLAN.md - Event Loop API

### Phase 1 API Addition

```c
// Event loop integration (OPTIONAL - only needed if using async I/O or timers)
typedef enum {
    HLFFI_EVENTLOOP_UV,       // libuv (async I/O, HTTP, UV timers)
    HLFFI_EVENTLOOP_HAXE      // haxe.EventLoop (haxe.Timer, haxe.MainLoop)
} hlffi_eventloop_type;

// Process events (non-blocking) - call this every engine tick/frame
hlffi_error_code hlffi_process_events(hlffi_vm* vm, hlffi_eventloop_type type);

// Check if event loop has pending work
bool hlffi_has_pending_events(hlffi_vm* vm, hlffi_eventloop_type type);

// Convenience: Process all event loops
hlffi_error_code hlffi_process_all_events(hlffi_vm* vm);
```

### Implementation Notes

```c
// Use weak symbols to make event loops optional
extern uv_loop_t* uv_default_loop() __attribute__((weak));
extern void* haxe_EventLoop_main() __attribute__((weak));

hlffi_error_code hlffi_process_all_events(hlffi_vm* vm) {
    // Process UV if it exists
    if (uv_default_loop != NULL) {
        hlffi_process_events(vm, HLFFI_EVENTLOOP_UV);
    }

    // Process Haxe EventLoop if it exists
    if (haxe_EventLoop_main != NULL) {
        hlffi_process_events(vm, HLFFI_EVENTLOOP_HAXE);
    }

    return HLFFI_OK;
}
```

---

## Comparison: Android vs WASM vs HLFFI

| Aspect | Android (User's Impl) | WASM (User's Impl) | HLFFI (Unreal/Unity) |
|--------|----------------------|-------------------|----------------------|
| **Entry Point** | Blocks in while loop | Returns immediately | Returns immediately |
| **Main Loop** | Haxe controls (`while`) | JS controls (RAF) | Engine controls (Tick) |
| **Event Loop** | UV (libuv) | Emscripten + custom | UV + haxe.EventLoop |
| **Processing** | `UVLoop.processEvents()` | `mainloop_tick_callback()` | `hlffi_process_events()` |
| **Mode** | UV_RUN_NOWAIT | loopOnce + weak symbols | UV_RUN_NOWAIT + loopOnce |
| **Thread** | Main thread | Main thread (JS) | Engine thread |
| **Pattern** | Pattern A (Haxe loop) | Pattern B (JS loop) | Pattern B (Engine loop) |

**All three use the same core pattern**: External loop + non-blocking event processing!

---

## Credits

**Source repository**: https://github.com/LogicInteractive/AndroidBuildTools
**Key files analyzed**:
- `native/rawdrawandroid/rawdraw/CNFGEGLDriver.c` - Entry point calling
- `native/uv_integration.c` - UV loop integration
- `src/android/system/UVLoop.hx` - Haxe wrapper
- `src/MainO.hx` - Main application pattern

**Key insight**: Production code proves entry point CAN run on main thread, and event loops work with external (non-HL) loop drivers.
