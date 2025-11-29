# HLFFI API Reference - Event Loop Integration

**[← Integration Modes](API_02_INTEGRATION_MODES.md)** | **[Back to Index](API_REFERENCE.md)** | **[Threading →](API_04_THREADING.md)**

Event loop integration enables Haxe asynchronous features like `haxe.Timer`, `MainLoop.add()`, and `sys.thread.EventLoop` in **NON_THREADED mode**.

---

## Overview

**Supported Event Systems:**
- **UV Loop** (`libuv`) - Async I/O, HTTP, file watching, timers
- **Haxe EventLoop** - `haxe.Timer.delay()`, `new haxe.Timer()`, `MainLoop.add()`

**Integration Pattern:**
```c
// Engine tick function (called every frame):
void on_tick(float delta_time)
{
    hlffi_update(vm, delta_time);  // Process both UV + Haxe events
}
```

**Complete Guide:** See `docs/TIMERS_ASYNC_THREADING.md`

---

## Functions

### `hlffi_update()`

**Signature:**
```c
hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time)
```

**Description:**
**Call this every frame** from your engine tick in NON_THREADED mode. Processes both UV and Haxe event loops non-blockingly.

**Parameters:**
- `vm` - VM instance
- `delta_time` - Frame delta time in seconds (optional, can be 0)

**Returns:**
- `HLFFI_OK` - Events processed
- `HLFFI_ERROR_NULL_VM` - vm is NULL

**Thread Safety:** ❌ Call from main thread only

**Performance:** Non-blocking, returns immediately (~1-5ms typical)

**What It Does:**
1. Calls `uv_run(UV_RUN_NOWAIT)` if UV loop exists
2. Calls `sys.thread.Thread.current().events.progress()` for haxe.Timer
3. Calls `haxe.MainLoop.tick()` for MainLoop callbacks

**Example:**
```c
// Unreal Engine:
void AMyActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    hlffi_update(vm, DeltaTime);
}

// Unity:
void Update()
{
    hlffi_update(vm, Time.deltaTime);
}

// Custom Engine:
while (running)
{
    float dt = get_delta_time();
    hlffi_update(vm, dt);
    render();
}
```

**Notes:**
- Uses weak symbols - only processes event loops that exist
- Safe to call even if Haxe doesn't use timers/events
- For THREADED mode, VM thread handles this automatically

---

### `hlffi_has_pending_work()`

**Signature:**
```c
bool hlffi_has_pending_work(hlffi_vm* vm)
```

**Description:**
Checks if there are pending events in either UV or Haxe event loops.

**Returns:** `true` if pending events exist, `false` otherwise

**Example:**
```c
if (hlffi_has_pending_work(vm))
{
    hlffi_update(vm, 0);  // Process events
}
```

---

### `hlffi_process_events()`

**Signature:**
```c
hlffi_error_code hlffi_process_events(hlffi_vm* vm, hlffi_eventloop_type type)
```

**Description:**
Low-level event processing for specific event loop. **Most users should use `hlffi_update()` instead.**

**Parameters:**
- `vm` - VM instance
- `type` - `HLFFI_EVENTLOOP_UV`, `HLFFI_EVENTLOOP_HAXE`, or `HLFFI_EVENTLOOP_ALL`

**Example:**
```c
// Process only Haxe events (skip UV):
hlffi_process_events(vm, HLFFI_EVENTLOOP_HAXE);

// Process both (equivalent to hlffi_update):
hlffi_process_events(vm, HLFFI_EVENTLOOP_ALL);
```

---

### `hlffi_has_pending_events()`

**Signature:**
```c
bool hlffi_has_pending_events(hlffi_vm* vm, hlffi_eventloop_type type)
```

**Description:**
Checks if a specific event loop has pending events.

**Parameters:**
- `vm` - VM instance
- `type` - `HLFFI_EVENTLOOP_UV` or `HLFFI_EVENTLOOP_HAXE`

**Example:**
```c
if (hlffi_has_pending_events(vm, HLFFI_EVENTLOOP_HAXE))
{
    printf("Haxe timers/callbacks pending\n");
}
```

---

## Complete Example

```c
#include "hlffi.h"

int main()
{
    // Setup VM:
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
    hlffi_call_entry(vm);  // Initializes Haxe timers
    
    // Game loop (60 FPS):
    while (!should_quit())
    {
        float dt = 1.0f / 60.0f;
        
        // Process Haxe timers and MainLoop callbacks:
        hlffi_update(vm, dt);
        
        // Your game logic:
        update_game(dt);
        render();
        
        sleep_ms(16);
    }

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class Main
{
    public static function main()
    {
        trace("Game starting");
        
        // One-shot timer (1 second):
        haxe.Timer.delay(() -> {
            trace("1 second elapsed!");
        }, 1000);
        
        // Recurring timer (every 2 seconds):
        new haxe.Timer(2000).run = () -> {
            trace("Tick every 2 seconds");
        };
        
        // MainLoop callback:
        haxe.MainLoop.add(() -> {
            trace("Called every frame");
        });
    }
}
```

---

## Best Practices

### 1. Call Every Frame

```c
// ✅ GOOD
while (running)
{
    hlffi_update(vm, dt);  // Every frame
    render();
}

// ❌ BAD
while (running)
{
    if (frame % 60 == 0)
    {
        hlffi_update(vm, dt);  // Only every 60 frames - timers will be delayed!
    }
    render();
}
```

### 2. Use Non-Blocking

```c
// ✅ GOOD - hlffi_update() is non-blocking
hlffi_update(vm, dt);  // Returns immediately

// ❌ BAD - Don't block on events
// This would hang if using blocking UV operations
```

### 3. Check for Pending Work (Optimization)

```c
// Optimize when no Haxe events:
if (hlffi_has_pending_work(vm))
{
    hlffi_update(vm, 0);
}
else
{
    // Can skip processing this frame
}
```

---

**[← Integration Modes](API_02_INTEGRATION_MODES.md)** | **[Back to Index](API_REFERENCE.md)** | **[Threading →](API_04_THREADING.md)**