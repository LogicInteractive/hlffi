# HLFFI API Reference - Integration Modes

**[← Back to API Index](API_REFERENCE.md)** | **[VM Lifecycle](API_01_VM_LIFECYCLE.md)** | **[Event Loop →](API_03_EVENT_LOOP.md)**

Integration modes determine how HLFFI manages the VM lifecycle and event processing.

---

## Table of Contents

1. [Overview](#overview)
2. [Choosing a Mode](#choosing-a-mode)
3. [Functions](#functions)
4. [Examples](#examples)

---

## Overview

HLFFI supports two integration modes:

| Mode | Best For | Entry Point | Event Processing | Complexity |
|------|----------|-------------|------------------|------------|
| **NON_THREADED** | Game engines, Tools | Main thread | Call `hlffi_update()` every frame | Simple |
| **THREADED** | Android pattern, Servers | Separate thread | Automatic | Advanced |

**Key Decision:** If your Haxe code has `while(true)` loop, use **THREADED**. Otherwise, use **NON_THREADED**.

---

## Choosing a Mode

### NON_THREADED Mode (Default - Recommended)

**Use when:**
- Host application controls the main loop
- Haxe `main()` returns quickly
- Game engine integration (Unreal, Unity, custom engines)
- Tool development

**Pattern:**
```c
hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
hlffi_call_entry(vm);  // Returns immediately

// Game loop:
while (running)
{
    hlffi_update(vm, delta_time);  // Process Haxe events
    render();
}
```

**Advantages:**
- Simple, direct function calls
- No thread synchronization overhead
- Easy to debug
- Lowest latency

**Disadvantages:**
- Haxe code cannot have blocking while loop
- Must call `hlffi_update()` every frame

---

### THREADED Mode (Advanced)

**Use when:**
- Haxe code has blocking `while(true)` loop
- Android pattern (Haxe controls main loop)
- Server applications
- Background processing

**Pattern:**
```c
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
hlffi_thread_start(vm);  // Spawns thread, calls entry in thread

// Main thread continues, communicate via message queue:
hlffi_thread_call_sync(vm, callback, userdata);
```

**Advantages:**
- Haxe can have blocking loop
- VM runs independently
- Non-blocking host application

**Disadvantages:**
- More complex (message queue, synchronization)
- Higher latency (message passing overhead)
- Harder to debug

---

## Functions

### `hlffi_set_integration_mode()`

**Signature:**
```c
hlffi_error_code hlffi_set_integration_mode(hlffi_vm* vm, hlffi_integration_mode mode)
```

**Description:**
Sets the integration mode. **Must be called BEFORE `hlffi_call_entry()`**.

**Parameters:**
- `vm` - VM instance
- `mode` - `HLFFI_MODE_NON_THREADED` or `HLFFI_MODE_THREADED`

**Returns:**
- `HLFFI_OK` - Mode set successfully
- `HLFFI_ERROR_NULL_VM` - vm is NULL

**Thread Safety:** ❌ Not thread-safe

**When:** Must be called after `hlffi_init()`, before `hlffi_call_entry()`

**Example:**
```c
// Non-threaded (game engine pattern):
hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
hlffi_call_entry(vm);  // Returns immediately

// Threaded (blocking loop pattern):
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
hlffi_thread_start(vm);  // Spawns thread, calls entry in thread
```

**Default:** If not called, defaults to `HLFFI_MODE_NON_THREADED`

**See Also:** [`hlffi_get_integration_mode()`](#hlffi_get_integration_mode), [Threading](API_04_THREADING.md)

---

### `hlffi_get_integration_mode()`

**Signature:**
```c
hlffi_integration_mode hlffi_get_integration_mode(hlffi_vm* vm)
```

**Description:**
Returns the current integration mode.

**Parameters:**
- `vm` - VM instance

**Returns:**
- `HLFFI_MODE_NON_THREADED` or `HLFFI_MODE_THREADED`

**Thread Safety:** ✅ Read-only, safe after initialization

**Example:**
```c
if (hlffi_get_integration_mode(vm) == HLFFI_MODE_THREADED)
{
    printf("Using threaded mode\n");
}
else
{
    printf("Using non-threaded mode\n");
}
```

---

## Examples

### Example 1: Game Engine Integration (NON_THREADED)

```c
#include "hlffi.h"

// Unreal Engine Tick:
void AMyActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    hlffi_update(vm, DeltaTime);
}

// Unity Update:
void Update()
{
    hlffi_update(vm, Time.deltaTime);
}

// Custom Game Loop:
int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");

    // Set NON_THREADED mode:
    hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
    hlffi_call_entry(vm);  // Initializes, returns immediately

    // Game loop:
    while (!should_quit())
    {
        float dt = get_delta_time();

        // Process Haxe timers/events:
        hlffi_update(vm, dt);

        // Your game logic:
        update_game(dt);
        render();
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
        trace("Game initialized");
        // Setup, but don't block
        haxe.Timer.delay(() -> trace("Timer fired!"), 1000);
    }
}
```

---

### Example 2: Android Pattern (THREADED)

```c
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");

    // Set THREADED mode:
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    hlffi_thread_start(vm);  // Spawns thread, Haxe runs there

    // Main thread continues, can do other work:
    while (!should_quit())
    {
        // Communicate with Haxe via message queue:
        int score = 100;
        hlffi_thread_call_sync(vm, update_score_callback, &score);

        // Other native work:
        handle_events();
        sleep_ms(16);
    }

    hlffi_thread_stop(vm);
    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}

void update_score_callback(hlffi_vm* vm, void* userdata)
{
    int score = *(int*)userdata;
    hlffi_value* arg = hlffi_value_int(vm, score);
    hlffi_call_static(vm, "Game", "setScore", 1, &arg);
    hlffi_value_free(arg);
}
```

**Haxe Side:**
```haxe
class Main
{
    public static function main()
    {
        // Blocking game loop - runs in VM thread
        while (true)
        {
            update();
            render();
            Sys.sleep(0.016);  // 60 FPS
        }
    }
}
```

---

## Best Practices

### 1. Match Mode to Haxe Code

```c
// ✅ GOOD - Non-blocking Haxe → NON_THREADED
// Haxe: function main() { trace("Hello"); }
hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);

// ✅ GOOD - Blocking Haxe → THREADED
// Haxe: function main() { while(true) { /* loop */ } }
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);

// ❌ BAD - Blocking Haxe → NON_THREADED
// Haxe: function main() { while(true) { /* loop */ } }
hlffi_call_entry(vm);  // HANGS FOREVER!
```

### 2. Set Mode Before Entry Point

```c
// ✅ GOOD
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
hlffi_call_entry(vm);

// ❌ BAD
hlffi_call_entry(vm);
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);  // Too late!
```

### 3. Use Simplest Mode That Works

```c
// ✅ GOOD - Prefer NON_THREADED if possible
if (haxe_main_is_simple)
{
    hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
}
else
{
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
}

// Non-threaded is simpler, faster, easier to debug
```

---

## Common Pitfalls

### 1. Forgetting to Update in NON_THREADED Mode

**Problem:**
```c
hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
hlffi_call_entry(vm);
// Missing: hlffi_update() in game loop
// Timers never fire!
```

**Solution:** Call `hlffi_update()` every frame

---

### 2. Using Wrong Mode

**Problem:**
```c
// Haxe has while(true)
hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
hlffi_call_entry(vm);  // HANGS!
```

**Solution:** Use THREADED mode for blocking code

---

**[← Back to API Index](API_REFERENCE.md)** | **[VM Lifecycle](API_01_VM_LIFECYCLE.md)** | **[Event Loop →](API_03_EVENT_LOOP.md)**