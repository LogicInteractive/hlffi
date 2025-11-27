# Event Loop Integration - Quick Start Guide

**TL;DR:** Call `hlffi_update(vm, dt)` every frame and your timers will work.

---

## 30-Second Setup

### 1. Add Helper to Your Haxe Code

```haxe
// In your Main.hx or any class:
public static function processEventLoop():Void {
    var loop = sys.thread.Thread.current().events;
    if (loop != null) {
        loop.progress();
    }
}
```

### 2. Call hlffi_update() Every Frame

```c
// In your C/C++ game loop:
while (running) {
    hlffi_update(vm, delta_time);
    update_game();
    render();
}
```

**That's it!** Your timers now work.

---

## What Just Works

✅ `haxe.Timer.delay(() -> trace("fired"), 100)` - One-shot timers
✅ `new haxe.Timer(100)` - Interval timers
✅ `haxe.MainLoop.add(() -> trace("tick"))` - Per-frame callbacks
✅ Multiple concurrent timers
✅ 1ms-100ms+ timer range

---

## Update Frequencies

### 60 FPS Updates (Standard)
```c
while (running) {
    hlffi_update(vm, 0.016f);  // ~16ms resolution
    render();
    vsync();
}
```
**Timer Precision:** ±16ms

### 1ms Updates (High Precision)
```c
while (running) {
    double now = get_time_ms();

    if (now - last_update >= 1.0) {
        hlffi_update(vm, 0.001f);  // 1ms resolution
        last_update = now;
    }

    if (now - last_frame >= 16.666) {
        render();
        last_frame = now;
    }

    sleep_ms(1);
}
```
**Timer Precision:** ±1ms

---

## Common Patterns

### Pattern 1: Use Timers in Haxe

```haxe
class Game {
    public static function main() {
        // One-shot timer
        haxe.Timer.delay(() -> {
            trace("Fired after 1 second");
        }, 1000);

        // Interval timer
        var timer = new haxe.Timer(100);
        timer.run = () -> {
            trace("Fires every 100ms");
        };

        // MainLoop callback
        haxe.MainLoop.add(() -> {
            trace("Every frame");
        });
    }
}
```

### Pattern 2: Call from C

```c
// Set up a timer from C
hlffi_call_static(vm, "Game", "setupTimers");

// Then just keep calling update
while (running) {
    hlffi_update(vm, dt);
    render();
}
```

---

## Troubleshooting

### Timers Don't Fire?

1. **Did you add `processEventLoop()` to your Haxe code?** ← Most common issue
2. **Are you calling `hlffi_update()` regularly?**
3. **Did you call `hlffi_call_entry(vm)` before using timers?**

### How Often Should I Call hlffi_update()?

| Update Frequency | Timer Precision | Use Case |
|-----------------|-----------------|----------|
| 60 FPS (~16ms) | ±16ms | Standard games |
| 120 FPS (~8ms) | ±8ms | Fast-paced games |
| 1000 Hz (1ms) | ±1ms | Audio, networking, precision timing |

**Rule of Thumb:** Update frequency = Timer precision

---

## Advanced: Custom Event Loop Processing

If you don't want to use the `Timers.processEventLoop()` helper:

```c
// Call event loops directly:
hlffi_process_events(vm, HLFFI_EVENTLOOP_ALL);    // Everything
hlffi_process_events(vm, HLFFI_EVENTLOOP_HAXE);   // Just Haxe
hlffi_process_events(vm, HLFFI_EVENTLOOP_UV);     // Just UV
```

---

## API Reference

### Main Function

```c
hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time);
```
- **vm:** Your VM instance
- **delta_time:** Time since last update (for your use, not used by HLFFI)
- **Returns:** `HLFFI_OK` on success

### Check for Pending Work

```c
bool hlffi_has_pending_work(hlffi_vm* vm);
```
Returns `true` if there might be pending events (conservative).

---

## Example: Complete Integration

```c
// example_eventloop.c
#include "hlffi.h"

int main() {
    // Create VM
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Game loop
    bool running = true;
    while (running) {
        // Process events (timers, callbacks)
        hlffi_update(vm, 0.016f);

        // Your game logic
        hlffi_call_static(vm, "Game", "update");

        // Render
        render();

        // 60 FPS
        sleep_ms(16);
    }

    // Cleanup
    hlffi_destroy(vm);
    return 0;
}
```

```haxe
// Game.hx
class Game {
    public static function main() {
        trace("Game initialized");

        // Set up a timer
        haxe.Timer.delay(() -> {
            trace("Timer fired!");
        }, 1000);
    }

    public static function update() {
        // Your game logic
    }

    // REQUIRED: Event loop helper
    public static function processEventLoop():Void {
        var loop = sys.thread.Thread.current().events;
        if (loop != null) {
            loop.progress();
        }
    }
}
```

---

## Performance Tips

1. **Update frequency:** Match your timer precision needs (60 FPS for most cases, 1000 Hz for precision)
2. **Sleep between updates:** Use `sleep_ms(1)` to prevent CPU spinning
3. **Batch timer setup:** Set up all timers at initialization when possible
4. **Cancel unused timers:** Use `timer.stop()` to clean up interval timers

---

## Summary

**Minimum Required:**
1. Add `processEventLoop()` helper to your Haxe code
2. Call `hlffi_update(vm, dt)` regularly from C
3. Use `haxe.Timer` as normal in your Haxe code

**That's all you need!** Everything else just works.

For detailed information, see `docs/PHASE1_EVENTLOOP_INTEGRATION.md`.
