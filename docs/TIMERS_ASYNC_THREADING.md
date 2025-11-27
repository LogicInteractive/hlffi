# Timers, Async, and Threading Considerations for Embedded HashLink

**Date**: 2025-11-27
**Status**: Reference Documentation

---

## Overview

When embedding HashLink in a host application (game engine, native app), you control when Haxe code executes. This has implications for:
- **Timers** (`haxe.Timer`)
- **Async operations** (`haxe.MainLoop`, `sys.thread`)
- **Threading** (Haxe threads vs host threads)

---

## The Fundamental Principle

**HashLink is NOT automatically ticking.** In embedded mode:
- Haxe code only runs when YOU call it from C
- Timers only fire when you process events
- Callbacks only execute when you poll for them

---

## Timer Resolution

### The Problem

If you update at 60 FPS (~16.6ms per frame):

```
Frame 1: t=0ms      → loopOnce()
Frame 2: t=16.6ms   → loopOnce()
Frame 3: t=33.3ms   → loopOnce()
```

A 4ms timer wants to fire at: 4ms, 8ms, 12ms, 16ms, 20ms...

**Result**: Timer fires once per frame, always late. A 4ms timer effectively becomes a 16.6ms timer.

### Solution: Dual Update Pattern

```c
// Main loop with high-frequency timer processing
while (running) {
    double now = get_time_ms();

    // High-frequency: process timers/events (1-4ms granularity)
    if (now - last_event_tick >= 1.0) {
        hlffi_call_static(vm, "haxe.EntryPoint", "processEvents");
        // Or directly: EventLoop.main.loopOnce()
        last_event_tick = now;
    }

    // Frame-rate: game logic and rendering (60 FPS)
    if (now - last_frame_tick >= 16.666) {
        hlffi_call_static(vm, "Game", "update", delta_time);
        render();
        last_frame_tick = now;
    }

    sleep_ms(1);  // Prevent CPU spinning
}
```

### Timer Processing APIs

```c
// Process all pending timers and callbacks (non-blocking)
hlffi_process_events(vm);

// Or call directly into Haxe:
hlffi_call_static(vm, "haxe.EventLoop", "loopOnce", false);
```

**Key point**: `loopOnce()` is **non-blocking**. It processes all ready events and returns immediately.

---

## Async Operations

### MainLoop Callbacks

```haxe
// Haxe code
haxe.MainLoop.add(() -> {
    trace("This fires every loopOnce() call");
});

haxe.MainLoop.addOnce(() -> {
    trace("This fires once on next loopOnce()");
});
```

**When do these execute?** Only when you call `loopOnce()` from C.

### Delayed Execution

```haxe
// Haxe code
haxe.Timer.delay(() -> {
    trace("This fires after 100ms");
}, 100);
```

**Actual behavior**: Fires on the first `loopOnce()` call AFTER 100ms has elapsed.

If you only call `loopOnce()` at 60 FPS:
- Best case: fires at 100ms (if frame aligns)
- Worst case: fires at 116ms (100ms + one frame delay)

---

## Threading

### Haxe Threads DO Run Independently

When Haxe code creates a thread:

```haxe
sys.thread.Thread.create(() -> {
    while (true) {
        // This runs independently of your loopOnce() calls
        doBackgroundWork();
        Sys.sleep(0.001);
    }
});
```

This thread:
- **IS** a real OS thread
- **DOES** run continuously
- **DOES NOT** depend on your frame rate
- **IS** registered with the GC automatically

### Thread-to-Main Communication

```haxe
// In worker thread
haxe.MainLoop.runInMainThread(() -> {
    trace("Execute this on main thread");
});
```

**Problem**: This callback is QUEUED. It only executes when main thread calls `loopOnce()`.

```
Worker Thread                    Main Thread
┌─────────────────┐             ┌─────────────────┐
│ runInMainThread │ ─────────▶  │ [callback queue]│
│ (queues callback│             │                 │
└─────────────────┘             │ ...waiting...   │
                                │                 │
                                │ loopOnce()      │ ◀── callback executes HERE
                                └─────────────────┘
```

**Recommendation**: Call `loopOnce()` frequently (1-4ms) if you expect thread→main callbacks.

### Thread Safety Rules

| Operation | Thread Safe? | Notes |
|-----------|-------------|-------|
| Read/write Haxe objects | NO | Use mutex or runInMainThread |
| Call Haxe static methods | NO | One thread at a time |
| GC allocations | YES | Each thread is GC-registered |
| Mutex/Lock/Semaphore | YES | Use `sys.thread.Mutex` etc. |
| `runInMainThread` | YES | Queues safely, executes on main |

### C Threads Touching Haxe

If you create a C thread that needs to call Haxe code:

```c
void my_c_thread(void* param) {
    // MUST register before touching any Haxe objects
    hl_register_thread(&param);

    // Now safe to call Haxe
    hlffi_call_static(vm, "Game", "onNetworkData", data);

    // Unregister when done
    hl_unregister_thread();
}
```

**Warning**: Don't call Haxe from C threads simultaneously with main thread - HashLink is not reentrant.

---

## Event Loop Modes

### Blocking Mode (Default for Standalone)

```haxe
// This BLOCKS forever in a standalone Haxe app
haxe.EventLoop.main.loop();
```

**Never use this in embedded mode** - it will freeze your host application.

### Non-Blocking Mode (For Embedding)

```haxe
// This processes events and returns immediately
haxe.EventLoop.main.loopOnce();
```

**Always use this** in embedded mode.

### Custom EntryPoint (Recommended)

Override `haxe.EntryPoint` to prevent blocking:

```haxe
// CustomEntryPoint.hx - put in your Haxe project
package haxe;

class EntryPoint {
    @:keep
    public static function run() {
        // Do nothing - let C code drive the event loop
    }

    @:keep
    public static function processEvents() {
        haxe.EventLoop.main.loopOnce(false);
    }
}
```

Then from C:
```c
// After loading bytecode
hlffi_call_entry(vm);  // Returns immediately (doesn't block)

// In your main loop
hlffi_call_static(vm, "haxe.EntryPoint", "processEvents");
```

---

## Recommended Patterns

### Pattern 1: Simple Frame-Based (No High-Frequency Timers)

Use this if your Haxe code doesn't need sub-frame timer precision.

```c
void game_loop() {
    while (running) {
        // Process events and update together
        hlffi_process_events(vm);
        hlffi_call_static(vm, "Game", "update", delta_time);
        render();

        wait_for_vsync();  // ~16.6ms
    }
}
```

**Timer resolution**: ~16ms (frame rate)

### Pattern 2: High-Frequency Timer Support

Use this if Haxe code has 1-10ms timers (audio, networking, input).

```c
void game_loop() {
    double last_event = 0;
    double last_frame = 0;

    while (running) {
        double now = get_time_ms();

        // High-frequency event processing (1ms)
        if (now - last_event >= 1.0) {
            hlffi_process_events(vm);
            last_event = now;
        }

        // Frame-rate game update (60 FPS)
        if (now - last_frame >= 16.666) {
            hlffi_call_static(vm, "Game", "update", now - last_frame);
            render();
            last_frame = now;
        }

        sleep_us(500);  // 0.5ms sleep, prevents CPU spinning
    }
}
```

**Timer resolution**: ~1-2ms

### Pattern 3: Dedicated Timer Thread

Use this for microsecond-precision or when main thread is busy.

```c
// Timer thread
void timer_thread(void* vm) {
    hl_register_thread(&vm);

    while (running) {
        // Acquire lock before calling into Haxe
        mutex_lock(&haxe_mutex);
        hlffi_process_events(vm);
        mutex_unlock(&haxe_mutex);

        sleep_us(500);  // 0.5ms granularity
    }

    hl_unregister_thread();
}

// Main thread
void game_loop() {
    while (running) {
        mutex_lock(&haxe_mutex);
        hlffi_call_static(vm, "Game", "update", delta_time);
        mutex_unlock(&haxe_mutex);

        render();  // Can happen without lock
        wait_for_vsync();
    }
}
```

**Timer resolution**: ~0.5-1ms
**Complexity**: Higher (thread synchronization required)

---

## What NOT to Do

### Don't Block in Haxe Callbacks

```haxe
// BAD - blocks the entire frame
haxe.Timer.delay(() -> {
    Sys.sleep(1.0);  // Freezes everything for 1 second!
}, 100);
```

### Don't Expect Precise Timing Without Frequent Updates

```haxe
// This WON'T fire every 4ms if you only call loopOnce() at 60fps
var timer = new haxe.Timer(4);
timer.run = () -> trace("tick");  // Actually fires every ~16ms
```

### Don't Call Haxe from Multiple C Threads Simultaneously

```c
// BAD - race condition, potential crash
void thread1() { hlffi_call_static(vm, "A", "foo"); }
void thread2() { hlffi_call_static(vm, "B", "bar"); }
// Both running at the same time = CRASH
```

### Don't Forget to Process Events

```c
// BAD - timers and callbacks never fire
while (running) {
    hlffi_call_static(vm, "Game", "update", dt);
    render();
    // Missing: hlffi_process_events(vm)!
}
```

---

## Summary Table

| Feature | Frame-Based (60fps) | High-Frequency (1ms) | Dedicated Thread |
|---------|---------------------|----------------------|------------------|
| Timer resolution | ~16ms | ~1-2ms | ~0.5-1ms |
| CPU usage | Low | Medium | Medium-High |
| Complexity | Simple | Simple | Complex |
| Thread→Main callbacks | 16ms delay | 1-2ms delay | 0.5-1ms delay |
| Use case | Games, UI | Audio, Network | Real-time |

---

## API Reference

```c
// Process all pending events (timers, callbacks, thread messages)
// Non-blocking - returns immediately after processing
hlffi_error_code hlffi_process_events(hlffi_vm* vm);

// Check if there are pending events
bool hlffi_has_pending_events(hlffi_vm* vm);

// Register a C thread for Haxe/GC interaction
void hl_register_thread(void* stack_top);
void hl_unregister_thread(void);
```

---

## See Also

- [HAXE_MAINLOOP_INTEGRATION.md](HAXE_MAINLOOP_INTEGRATION.md) - MainLoop/EventLoop details
- [THREADING_MODEL_REPORT.md](../THREADING_MODEL_REPORT.md) - Threading architecture
- [PHASE4_INSTANCE_MEMBERS.md](PHASE4_INSTANCE_MEMBERS.md) - Instance/object handling
