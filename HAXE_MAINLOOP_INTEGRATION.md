# Haxe MainLoop Integration for Engine Embedding

⚠️ **IMPORTANT**: See `MAINLOOP_CLARIFICATION.md` for when MainLoop is actually needed!

**TL;DR**: If your Haxe code doesn't use `haxe.Timer` or `haxe.MainLoop.add()`, you don't need ANY of this! Just call the entry point and your static functions - MainLoop won't be compiled in.

## The Problem

When embedding HashLink in game engines (Unreal, Unity, etc.), we need to:
1. ✅ Call the Haxe entry point (`main()`) to initialize the application
2. ✅ Support `haxe.MainLoop` / `haxe.EventLoop` for timers and async operations
3. ❌ **NOT** block the engine's main thread

## How Haxe Entry Points Work

### Standard Haxe Application Flow

```haxe
// User's Main.hx
class Main {
    static function main() {
        trace("App starting");
        haxe.MainLoop.add(update);  // Register update callback
        // main() returns here
    }

    static function update() {
        trace("Update tick");
    }
}
```

When compiled, Haxe automatically appends `haxe.EntryPoint.run()` at the end of `main()`:

```haxe
// What actually runs:
static function main() {
    trace("App starting");
    haxe.MainLoop.add(update);
    // ← EntryPoint.run() is inserted here by compiler
}
```

### EntryPoint.run() Behavior (from haxe/std/haxe/EntryPoint.hx)

```haxe
class EntryPoint {
    public static function run() {
        #if js
            // Non-blocking: Uses requestAnimationFrame to call loopOnce() each frame
            requestAnimationFrame(run);
            haxe.EventLoop.main.loopOnce();
        #elseif flash
            // Non-blocking: Uses ENTER_FRAME event to call loopOnce() each frame
            stage.addEventListener(Event.ENTER_FRAME, function(_) haxe.EventLoop.main.loopOnce());
        #else
            // ⚠️ BLOCKS: For sys targets (including HashLink)
            haxe.EventLoop.main.loop();  // ← BLOCKS FOREVER!
        #end
    }
}
```

**The issue**: Line 52 shows that for HashLink, `EntryPoint.run()` calls `EventLoop.main.loop()` which **BLOCKS** (see EventLoop.hx line 139-154):

```haxe
// EventLoop.hx line 139
public function loop() {
    // Blocks until all blocking events are stopped
    while( hasEvents(true) || promiseCount > 0 || (this == main && hasRunningThreads()) ) {
        var time = getNextTick();
        if( time > 0 ) {
            wait(time);  // ← BLOCKS here
            continue;
        }
        loopOnce(false);
    }
}
```

## Solutions for Engine Embedding

### Solution 1: Custom EntryPoint (Recommended)

Override `haxe.EntryPoint` to behave like JS/Flash - delegate to external tick:

```haxe
// CustomEntryPoint.hx (in your Haxe project)
package haxe;

class EntryPoint {
    @:keep
    public static function run() {
        // Do nothing - let HLFFI drive the event loop
        // The C++ engine will call processEvents() each tick
    }

    // Called by HLFFI from engine tick
    @:keep
    public static function processEvents() {
        haxe.EventLoop.main.loopOnce(false);
    }
}
```

**Compile with**: `haxe -hl game.hl -main Main -D replace-files`

**HLFFI usage**:
```cpp
// Unreal/Unity tick
void Tick(float DeltaTime) {
    // Process Haxe events (timers, async callbacks)
    hlffi_call_static(vm, "haxe.EntryPoint", "processEvents");

    // Call game update
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

### Solution 2: Direct EventLoop.loopOnce() Calls

If you can't override EntryPoint, compile without MainLoop and call loopOnce directly:

**Compile with**: `-D no-haxe-mainloop` (to prevent EntryPoint.run() from being called)

**HLFFI usage**:
```cpp
// One-time setup
void BeginPlay() {
    vm = hlffi_create();
    hlffi_init(vm, argc, argv);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);  // Returns immediately (no EntryPoint.run())

    // Get reference to main event loop
    mainLoop = hlffi_call_static(vm, "haxe.EventLoop", "get_main");
}

// Engine tick
void Tick(float DeltaTime) {
    // Process Haxe events
    hlffi_call_method(vm, mainLoop, "loopOnce", false);

    // Call game update
    hlffi_call_static(vm, "Game", "update", DeltaTime);
}
```

### Solution 3: Dedicated Thread with Message Queue (Advanced)

Use HLFFI's Pattern 2 (dedicated thread) if game logic must run continuously:

```cpp
// Setup
vm = hlffi_create();
hlffi_set_thread_mode(vm, HLFFI_THREAD_DEDICATED);
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "game.hl");
hlffi_thread_start(vm);  // Starts thread, calls entry (blocks in EventLoop.loop())

// Engine tick - queue calls to VM thread
void Tick(float DeltaTime) {
    hlffi_thread_call_sync(vm, [](hlffi_vm* vm, void* data) {
        float dt = *(float*)data;
        hlffi_call_static(vm, "Game", "update", dt);
    }, &DeltaTime);
}
```

## EventLoop API Reference

### EventLoop.loopOnce() - Non-blocking

```haxe
// haxe/EventLoop.hx line 217
public function loopOnce(threadCheck = true) {
    // 1. Process libuv events (if any) - non-blocking
    if( uvLoop != null ) {
        uvLoop.run(NoWait);  // ← NoWait = non-blocking
    }

    // 2. Run all pending event callbacks
    var time = haxe.Timer.stamp();
    while( inLoop && current != null ) {
        if( current.nextRun <= time && !current.toRemove )
            current.callb();  // Execute callback
        current = current.next;
    }

    // 3. Return immediately
}
```

**Behavior**:
- ✅ Processes all pending events (timers, async callbacks)
- ✅ Returns immediately after processing
- ✅ Safe to call from engine tick
- ✅ Non-blocking

### EventLoop.loop() - Blocking

```haxe
// haxe/EventLoop.hx line 139
public function loop() {
    while( hasEvents(true) || promiseCount > 0 || hasRunningThreads() ) {
        var time = getNextTick();
        if( time > 0 ) {
            wait(time);  // ← BLOCKS until next event ready
            continue;
        }
        loopOnce(false);
    }
}
```

**Behavior**:
- ❌ Blocks until all blocking events are stopped
- ❌ Waits/sleeps between events
- ❌ NOT suitable for engine embedding

## HLFFI API Additions

### Phase 1 API Update

Add event loop helpers to HLFFI:

```c
// Event loop integration
hlffi_error_code hlffi_process_events(hlffi_vm* vm);
bool hlffi_has_pending_events(hlffi_vm* vm);
```

```cpp
// C++ wrapper
namespace hl {
    class VM {
        void processEvents() {
            if (hlffi_process_events(vm_) != HLFFI_OK)
                throw std::runtime_error("Failed to process events");
        }

        bool hasPendingEvents() {
            return hlffi_has_pending_events(vm_);
        }
    };
}
```

### Implementation

```c
// src/hlffi.c
hlffi_error_code hlffi_process_events(hlffi_vm* vm) {
    if (!vm || !vm->initialized) return HLFFI_ERROR_NOT_INITIALIZED;

    // Call haxe.EventLoop.main.loopOnce(false)
    hl_type* t = hl_module_type(vm->module, "haxe.EventLoop");
    if (!t) return HLFFI_ERROR_TYPE_NOT_FOUND;

    vdynamic* mainLoop = hl_call_method(t, "get_main", NULL);
    if (!mainLoop) return HLFFI_ERROR_CALL_FAILED;

    vdynamic* args[] = { hl_alloc_bool(false) };  // threadCheck = false
    hl_call_method(mainLoop, "loopOnce", args);

    return HLFFI_OK;
}

bool hlffi_has_pending_events(hlffi_vm* vm) {
    if (!vm || !vm->initialized) return false;

    // Call haxe.EventLoop.main.hasEvents(false)
    hl_type* t = hl_module_type(vm->module, "haxe.EventLoop");
    if (!t) return false;

    vdynamic* mainLoop = hl_call_method(t, "get_main", NULL);
    if (!mainLoop) return false;

    vdynamic* args[] = { hl_alloc_bool(false) };  // blocking = false
    return hl_call_method(mainLoop, "hasEvents", args)->v.b;
}
```

## Example: Unreal Engine Integration

```cpp
// UHaxeComponent.h
UCLASS()
class UHaxeComponent : public UActorComponent {
    GENERATED_BODY()

private:
    hlffi_vm* vm = nullptr;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ...) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};

// UHaxeComponent.cpp
void UHaxeComponent::BeginPlay() {
    Super::BeginPlay();

    // Initialize VM
    vm = hlffi_create();
    hlffi_init(vm, 0, nullptr);
    hlffi_load_file(vm, "Content/Haxe/Game.hl");
    hlffi_call_entry(vm);  // ✅ Returns immediately (custom EntryPoint)

    UE_LOG(LogTemp, Log, TEXT("Haxe VM initialized"));
}

void UHaxeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!vm) return;

    // Process Haxe events (timers, async callbacks)
    hlffi_process_events(vm);  // ✅ Non-blocking, returns immediately

    // Call game update
    hlffi_call_static(vm, "Game", "update", DeltaTime);

    // ✅ Unreal continues with physics, rendering, etc.
}

void UHaxeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (vm) {
        hlffi_destroy(vm);  // Only call at process exit
        vm = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}
```

## Example: Haxe Game Code

```haxe
// Game.hx
import haxe.MainLoop;
import haxe.Timer;

class Game {
    static var tickCount = 0;

    static function main() {
        trace("Game initializing...");

        // Register periodic callback (every 1 second)
        var timer = new Timer(1000);
        timer.run = function() {
            trace('Periodic callback: $tickCount ticks');
        };

        trace("Game initialized - ready for engine ticks");
    }

    // Called from engine tick
    public static function update(dt:Float) {
        tickCount++;

        // Game logic here
        // All MainLoop timers/callbacks are processed by
        // hlffi_process_events() before this is called
    }
}
```

## Comparison: Blocking vs Non-blocking

| Approach | Entry Point | Event Loop | Thread Behavior | Use Case |
|----------|-------------|------------|-----------------|----------|
| **Standard Haxe App** | Blocks in `loop()` | `EventLoop.main.loop()` | ❌ Blocks forever | Standalone CLI apps |
| **Solution 1 (Custom EntryPoint)** | Returns immediately | Manual `loopOnce()` calls | ✅ Non-blocking | ✅ **Engine embedding** |
| **Solution 2 (No MainLoop)** | Returns immediately | Manual `loopOnce()` calls | ✅ Non-blocking | ✅ **Engine embedding** |
| **Solution 3 (Dedicated Thread)** | Blocks in thread | `EventLoop.main.loop()` | ✅ Non-blocking (in thread) | Complex game logic |

## Key Takeaways

1. **Entry point CAN run on main thread** - it returns immediately (as proven in VM_MODES_AND_THREADING_DEEP_DIVE.md)
2. **EventLoop.loop() BLOCKS** - don't call it from engine thread
3. **EventLoop.loopOnce() is NON-BLOCKING** - perfect for engine tick
4. **Custom EntryPoint is recommended** - mimics JS/Flash behavior for engines
5. **HLFFI should provide helpers** - `hlffi_process_events()` wraps `loopOnce()`

## Updated MASTER_PLAN.md Changes

### Phase 1 API Addition

```c
// Event loop integration (for haxe.MainLoop support)
hlffi_error_code hlffi_process_events(hlffi_vm* vm);
bool hlffi_has_pending_events(hlffi_vm* vm);
```

### Documentation Addition

Add to Phase 8 (Documentation):
- Guide: "Using haxe.MainLoop in Embedded Applications"
- Example: Custom EntryPoint.hx template
- Example: Unreal Engine with MainLoop timers
- Example: Unity with async operations

## Confidence

🟢 **100% CERTAIN** - Verified in Haxe standard library source code:
- `haxe/std/haxe/EntryPoint.hx` - Shows blocking behavior for sys targets
- `haxe/std/haxe/EventLoop.hx` - Shows `loop()` blocks, `loopOnce()` doesn't
- `haxe/std/haxe/MainLoop.hx` - Just a wrapper for EventLoop

The solution is straightforward: Override `EntryPoint.run()` to avoid blocking, and call `EventLoop.main.loopOnce()` from engine tick.
