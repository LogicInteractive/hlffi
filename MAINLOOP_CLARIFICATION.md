# haxe.MainLoop - When It's Actually Needed

**CRITICAL CLARIFICATION**: In a pure HashLink app, `main()` does NOT automatically initialize `haxe.MainLoop`!

## The Truth

### Scenario 1: NO MainLoop Usage ✅

**Haxe code:**
```haxe
class Game {
    static function main() {
        trace("Hello");
        // No timers, no MainLoop.add()
    }

    public static function update(dt:Float) {
        // Called from C++
    }
}
```

**What happens:**
1. ✅ `main()` runs and returns immediately
2. ✅ No `haxe.EntryPoint.run()` call (DCE removes it)
3. ✅ No event loop
4. ✅ Entry point returns to C++
5. ✅ Engine can call `update()` each tick

**HLFFI usage:**
```cpp
void BeginPlay() {
    vm = hlffi_create();
    hlffi_call_entry(vm);  // ✅ Returns immediately!
}

void Tick(float DeltaTime) {
    // No hlffi_process_events() needed - no events!
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

### Scenario 2: WITH MainLoop Usage ⚠️

**Haxe code:**
```haxe
class Game {
    static function main() {
        trace("Hello");

        // Using timers!
        var timer = new haxe.Timer(1000);
        timer.run = function() {
            trace("Tick");
        };
    }

    public static function update(dt:Float) {
        // Called from C++
    }
}
```

**What happens:**
1. ✅ `main()` runs
2. ⚠️ **haxe.MainLoop is kept from DCE** (timer uses it)
3. ⚠️ **haxe.EntryPoint.run() IS called** (blocks on sys targets!)
4. ❌ Entry point BLOCKS in `EventLoop.loop()` on native targets
5. 💡 **Solution needed**: Override EntryPoint or process events manually

**HLFFI usage (WRONG - will block):**
```cpp
void BeginPlay() {
    vm = hlffi_create();
    hlffi_call_entry(vm);  // ❌ BLOCKS FOREVER! (on native targets)
}
```

**HLFFI usage (CORRECT):**
```cpp
void BeginPlay() {
    vm = hlffi_create();
    hlffi_call_entry(vm);  // ✅ Returns (if using custom EntryPoint)
}

void Tick(float DeltaTime) {
    // Process MainLoop events (timers, async callbacks)
    hlffi_process_events(vm);  // ← Calls EventLoop.main.loopOnce()

    // Call game update
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

## How WASM Solves This (Your Implementation)

From `hashlink-wasm/src/std/mainloop_wasm.c`:

```c
// Optional - only exists if Haxe code uses timers/MainLoop
extern vdynamic* haxe_MainLoop_tick() __attribute__((weak));

// Callback from Emscripten's event loop (60 FPS)
static void mainloop_tick_callback() {
    if (haxe_MainLoop_tick != NULL) {  // ← Check if it exists!
        haxe_MainLoop_tick();  // Process events
    }
}

// Emscripten drives the loop
emscripten_set_main_loop(mainloop_tick_callback, 60, 1);
```

**Your pattern**:
- JavaScript's `requestAnimationFrame` → `mainloop_tick_callback()` → `haxe_MainLoop_tick()`
- Checks if MainLoop exists before calling it
- Non-blocking - JavaScript drives the loop

## HLFFI Should Follow the Same Pattern

### Implementation Strategy

```c
// src/hlffi.c

// Check if haxe.EventLoop exists (weak symbol)
typedef struct {
    void (*loopOnce)(bool);
} hl_eventloop_t;

extern hl_eventloop_t* haxe_EventLoop_main() __attribute__((weak));

hlffi_error_code hlffi_process_events(hlffi_vm* vm) {
    if (!vm || !vm->initialized)
        return HLFFI_ERROR_NOT_INITIALIZED;

    // Check if haxe.EventLoop is compiled in
    if (haxe_EventLoop_main == NULL) {
        // No MainLoop in user code - nothing to do
        return HLFFI_OK;
    }

    // Get main event loop
    hl_eventloop_t* loop = haxe_EventLoop_main();
    if (!loop) return HLFFI_OK;

    // Process events (non-blocking)
    loop->loopOnce(false);  // threadCheck = false

    return HLFFI_OK;
}

bool hlffi_has_pending_events(hlffi_vm* vm) {
    if (!vm || !vm->initialized)
        return false;

    // Check if EventLoop exists
    if (haxe_EventLoop_main == NULL)
        return false;

    hl_eventloop_t* loop = haxe_EventLoop_main();
    if (!loop) return false;

    return loop->hasEvents(false);  // blocking = false
}
```

**Key insight from your WASM code**: Use **weak symbols** to make MainLoop optional!

## Comparison: WASM vs Native HLFFI

| Aspect | WASM (your impl) | Native HLFFI | Pattern |
|--------|------------------|--------------|---------|
| **Event loop driver** | Emscripten (browser) | Engine tick (Unreal/Unity) | External loop |
| **MainLoop check** | `if (haxe_MainLoop_tick != NULL)` | `if (haxe_EventLoop_main != NULL)` | Weak symbol |
| **Callback frequency** | 60 FPS (requestAnimationFrame) | Variable (engine tick) | Periodic |
| **Blocking behavior** | ✅ Non-blocking | ✅ Non-blocking | Returns immediately |

## Decision Tree for HLFFI Users

```
Does your Haxe code use haxe.Timer or haxe.MainLoop.add()?
│
├─ NO → Simple case
│   └─ Just call hlffi_call_entry() and hlffi_call_static()
│      No hlffi_process_events() needed
│
└─ YES → MainLoop case
    └─ Option A: Custom EntryPoint (recommended)
       ├─ Override haxe.EntryPoint.run() to do nothing
       ├─ Call hlffi_call_entry() (returns immediately)
       └─ Call hlffi_process_events() each engine tick

    └─ Option B: No custom EntryPoint
       ├─ Compile with -D no-haxe-mainloop
       └─ Call hlffi_process_events() each engine tick
```

## Example: Pure HL App (No MainLoop)

**Haxe:**
```haxe
class Game {
    static var score = 0;

    static function main() {
        trace("Game starting");
        // No timers, no MainLoop
    }

    public static function update(dt:Float) {
        score++;
    }
}
```

**C++ (Unreal):**
```cpp
void UHaxeComponent::BeginPlay() {
    vm = hlffi_create();
    hlffi_init(vm, 0, nullptr);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);  // ✅ Returns immediately (no MainLoop compiled in)
}

void UHaxeComponent::TickComponent(float DeltaTime, ...) {
    // No hlffi_process_events() - not needed!
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

**Binary size**: Smaller! (haxe.EventLoop not compiled in)

## Example: With MainLoop (Timers/Async)

**Haxe:**
```haxe
class Game {
    static var score = 0;

    static function main() {
        trace("Game starting");

        // Periodic scoring bonus
        var timer = new haxe.Timer(5000);
        timer.run = function() {
            score += 100;
            trace("Bonus! Score: " + score);
        };
    }

    public static function update(dt:Float) {
        score++;
    }
}
```

**Haxe (Custom EntryPoint):**
```haxe
// haxe/EntryPoint.hx - Add to your project
package haxe;

class EntryPoint {
    @:keep
    public static function run() {
        // Do nothing - let C++ engine drive the event loop
    }
}
```

**C++ (Unreal):**
```cpp
void UHaxeComponent::BeginPlay() {
    vm = hlffi_create();
    hlffi_init(vm, 0, nullptr);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);  // ✅ Returns (custom EntryPoint)
}

void UHaxeComponent::TickComponent(float DeltaTime, ...) {
    // Process MainLoop events (timers fire here)
    hlffi_process_events(vm);  // ✅ Non-blocking loopOnce()

    // Call game update
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

**Binary size**: Larger (haxe.EventLoop included), but timers work!

## Your WASM Pattern Applied to HLFFI

**Your WASM code** (JavaScript drives loop):
```javascript
// JavaScript (browser)
function gameLoop() {
    Module._hl_mainloop_tick();  // Process Haxe events
    requestAnimationFrame(gameLoop);
}
requestAnimationFrame(gameLoop);
```

**HLFFI equivalent** (Engine drives loop):
```cpp
// Unreal Engine
void UHaxeComponent::TickComponent(float DeltaTime, ...) {
    hlffi_process_events(vm);  // Process Haxe events
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

**Same pattern**: External loop → Check if MainLoop exists → Call tick/loopOnce() → Non-blocking

## Updated MASTER_PLAN.md Changes

### Phase 1 API - Clarified Documentation

```c
// Event loop integration (OPTIONAL - only needed if Haxe code uses haxe.Timer/haxe.MainLoop)
// These functions safely check if EventLoop exists before calling it (weak symbols)

hlffi_error_code hlffi_process_events(hlffi_vm* vm);
// Calls haxe.EventLoop.main.loopOnce() if EventLoop exists
// Returns HLFFI_OK if no EventLoop (nothing to do)
// Non-blocking - returns immediately after processing pending events

bool hlffi_has_pending_events(hlffi_vm* vm);
// Returns true if EventLoop exists AND has pending events
// Returns false if no EventLoop (normal for pure HL apps)
```

**Usage note**:
```cpp
// If your Haxe code doesn't use haxe.Timer or haxe.MainLoop.add():
// ✅ You can call hlffi_process_events() anyway - it's a no-op (returns immediately)
// ✅ Or skip calling it - your choice!

// If your Haxe code DOES use timers:
// ✅ You MUST call hlffi_process_events() each tick for timers to work
// ✅ Or provide custom haxe.EntryPoint.run() override
```

## Summary: Your WASM Insight is Correct

**You're 100% right**:
1. ✅ Pure HL app `main()` does NOT init MainLoop
2. ✅ MainLoop only exists if code uses `haxe.Timer` or `haxe.MainLoop.add()`
3. ✅ WASM uses external JS event loop (requestAnimationFrame)
4. ✅ HLFFI should use external engine tick (Unreal/Unity tick)
5. ✅ Check if MainLoop exists before calling it (weak symbols)

**My earlier docs were misleading** - I assumed MainLoop was always present. Your WASM implementation shows the correct pattern: **MainLoop is OPTIONAL**.

**Next step**: Update HAXE_MAINLOOP_INTEGRATION.md to clarify:
- MainLoop is optional (only if timers used)
- Use weak symbols to detect if it exists
- Follow your WASM pattern for native HLFFI
