# HLFFI User Guide - Part 4: Advanced Topics

**Integration modes, hot reload, callbacks, and optimization**

[← Data Exchange](GUIDE_03_DATA_EXCHANGE.md)

---

## Overview

This guide covers advanced HLFFI features:
- **Integration Modes** - How your app and Haxe work together
- **Hot Reload** - Update code without restarting
- **C Callbacks** - Let Haxe call your C functions
- **Threading** - Worker threads and GC safety
- **Performance** - Optimizing hot paths
- **VM Restart** - Full reset capability

---

## Integration Modes

HLFFI supports two ways to run Haxe code:

| Mode | Use When |
|------|----------|
| **NON_THREADED** | Your app controls the main loop (games, engines) |
| **THREADED** | Haxe has its own main loop (servers, background tasks) |

### Mode 1: Non-Threaded (Default)

Your application controls the main loop and calls Haxe each frame.

```c
int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");

    // Default mode is NON_THREADED
    hlffi_call_entry(vm);

    // Your main loop
    while (running)
    {
        float dt = get_delta_time();

        // Process Haxe timers/events
        hlffi_update(vm, dt);

        // Call Haxe game logic
        hlffi_call_static(vm, "Game", "update", 0, NULL);

        render();
    }

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

**Key function: `hlffi_update()`**

Call this every frame to process:
- `haxe.Timer` callbacks
- `haxe.MainLoop` events
- UV loop events (async I/O)

```c
// In your frame/tick function
void on_frame(float delta_time)
{
    hlffi_update(vm, delta_time);
}
```

### Mode 2: Threaded

Haxe runs in a dedicated background thread.

```c
int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "server.hl");

    // Set threaded mode BEFORE call_entry
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);

    // Start Haxe in background thread
    hlffi_thread_start(vm);

    // Haxe is now running in background!
    // Use thread-safe calls to communicate:

    while (running)
    {
        // Sync call: blocks until complete
        hlffi_thread_call_sync(vm, my_callback, my_data);

        handle_input();
        sleep_ms(16);
    }

    // Stop the Haxe thread
    hlffi_thread_stop(vm);
    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}

void my_callback(hlffi_vm* vm, void* data)
{
    // This runs on the Haxe thread - safe to call HLFFI
    hlffi_call_static(vm, "Game", "processInput", 0, NULL);
}
```

**When to use THREADED mode:**
- Haxe code has a `while(true)` loop
- Running a server
- Background processing

---

## Hot Reload

Update your Haxe code without restarting the application.

### Setup

```c
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, 0, NULL);

// Enable hot reload BEFORE loading
hlffi_enable_hot_reload(vm, true);

hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);
```

### Manual Reload

```c
// When you want to reload (e.g., after recompiling)
if (key_pressed('R'))
{
    if (hlffi_reload_module(vm, "game.hl") == HLFFI_OK)
    {
        printf("Reloaded successfully!\n");
    }
    else
    {
        printf("Reload failed: %s\n", hlffi_get_error(vm));
    }
}
```

### Auto-Reload on File Change

```c
// In your main loop
while (running)
{
    // Automatically reloads if file changed
    if (hlffi_check_reload(vm))
    {
        printf("Auto-reloaded!\n");
    }

    hlffi_update(vm, dt);
    render();
}
```

### Reload Callback

Get notified when reload happens:

```c
void on_reload(hlffi_vm* vm, bool success, void* data)
{
    if (success)
    {
        printf("Reload successful!\n");
        // Maybe re-initialize some state
        hlffi_call_static(vm, "Game", "onReload", 0, NULL);
    }
    else
    {
        printf("Reload failed: %s\n", hlffi_get_error(vm));
    }
}

// Set the callback
hlffi_set_reload_callback(vm, on_reload, NULL);
```

### What Gets Reloaded?

| Updated | Preserved |
|---------|-----------|
| Function code | Static variables |
| Method bodies | Object instances |
| New logic | Current game state |

**Haxe example:**
```haxe
class Game
{
    public static var score:Int = 0;  // Preserved across reloads!

    public static function getValue():Int
    {
        return 100;  // Change to 200, reload, see new value
    }
}
```

### Limitations

- Cannot add/remove fields on existing classes
- Cannot change type structures
- Requires HashLink 1.12+
- JIT mode only (not HL/C)

---

## C Callbacks

Let Haxe code call your C functions.

### Basic Callback

**C side:**
```c
// Your C function
hlffi_value* my_print(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    char* msg = hlffi_value_as_string(argv[0]);
    printf("[C] %s\n", msg);
    free(msg);
    return hlffi_value_null(vm);
}

// Register BEFORE loading bytecode
hlffi_register_callback(vm, "myPrint", my_print, 1);

hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);
```

**Haxe side:**
```haxe
// Declare the native function
@:hlNative("", "myPrint")
static function myPrint(msg:Dynamic):Void;

class Main
{
    public static function main()
    {
        myPrint("Hello from Haxe!");  // Calls C function
    }
}
```

### Callback with Return Value

**C:**
```c
hlffi_value* add_numbers(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    int a = hlffi_value_as_int(argv[0], 0);
    int b = hlffi_value_as_int(argv[1], 0);
    return hlffi_value_int(vm, a + b);
}

hlffi_register_callback(vm, "addNumbers", add_numbers, 2);
```

**Haxe:**
```haxe
@:hlNative("", "addNumbers")
static function addNumbers(a:Dynamic, b:Dynamic):Dynamic;

// Usage:
var result = addNumbers(10, 20);  // Returns 30
```

### Important Rules

1. **Register before loading:**
```c
hlffi_register_callback(vm, "func", callback, nargs);
hlffi_load_file(vm, "game.hl");  // After registration
```

2. **Use Dynamic types in Haxe:**
```haxe
// Good
@:hlNative("", "func")
static function func(arg:Dynamic):Dynamic;

// Bad - may not work correctly
@:hlNative("", "func")
static function func(arg:Int):Int;
```

3. **Always return something:**
```c
// For void functions, return null
return hlffi_value_null(vm);
```

4. **Free strings from arguments:**
```c
char* str = hlffi_value_as_string(argv[0]);
// ... use str ...
free(str);  // Don't forget!
```

---

## Worker Threads

If you need to call HLFFI from a background thread, you must register it with the GC.

### Register/Unregister

```c
void* my_worker(void* arg)
{
    hlffi_vm* vm = (hlffi_vm*)arg;

    // MUST register before using HLFFI
    hlffi_worker_register();

    // Now safe to call HLFFI functions
    hlffi_call_static(vm, "Task", "process", 0, NULL);

    // MUST unregister before thread exits
    hlffi_worker_unregister();

    return NULL;
}

// Spawn thread
pthread_t thread;
pthread_create(&thread, NULL, my_worker, vm);
```

### Blocking I/O in Callbacks

When doing slow I/O in a callback, notify the GC:

```c
hlffi_value* load_file(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    char* path = hlffi_value_as_string(argv[0]);

    // Tell GC we're doing external I/O
    hlffi_blocking_begin();

    FILE* f = fopen(path, "r");
    // ... read file (might be slow) ...
    fclose(f);

    // Back to normal
    hlffi_blocking_end();

    free(path);
    return hlffi_value_string(vm, content);
}
```

### C++ RAII Guards

If using C++, use the automatic guards:

```cpp
void* my_worker(void* arg)
{
    hlffi::WorkerGuard guard;  // Auto register/unregister

    // Safe to use HLFFI here
    hlffi_call_static(vm, "Task", "run", 0, NULL);

    return NULL;
}  // Auto unregister on scope exit

hlffi_value* my_callback(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    hlffi::BlockingGuard guard;  // Auto begin/end

    slow_io_operation();

    return hlffi_value_null(vm);
}  // Auto end on scope exit
```

---

## Performance Optimization

### Cache Hot Path Methods

For methods called every frame, cache them:

```c
// Cache once at startup
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
hlffi_cached_call* render = hlffi_cache_static_method(vm, "Graphics", "render");

// Use in loop (60x faster!)
while (running)
{
    hlffi_call_cached(update, 0, NULL);
    hlffi_call_cached(render, 0, NULL);
}

// Free when done
hlffi_cache_free(update);
hlffi_cache_free(render);
```

**Performance comparison:**
| Method | Time per call |
|--------|---------------|
| `hlffi_call_static()` | ~40ns |
| `hlffi_call_cached()` | ~0.7ns |

### Use Convenience Functions

The convenience API avoids temporary allocations:

```c
// Slower (3 function calls + allocation)
hlffi_value* val = hlffi_get_field(obj, "health");
int hp = hlffi_value_as_int(val, 0);
hlffi_value_free(val);

// Faster (1 function call)
int hp = hlffi_get_field_int(obj, "health", 0);
```

### Use Struct Arrays for Data

For large amounts of numerical data, use struct arrays:

```c
// Define matching struct
typedef struct { double x, y, z; } Vec3;

// Create struct array (zero-copy access)
hlffi_type* vec_type = hlffi_find_type(vm, "Vec3");
hlffi_value* positions = hlffi_array_new_struct(vm, vec_type, 1000);

// Direct pointer access (very fast!)
Vec3* data = (Vec3*)hlffi_array_get_struct(positions, 0);
for (int i = 0; i < 1000; i++)
{
    data[i].x = i * 1.0;
    data[i].y = i * 2.0;
    data[i].z = i * 3.0;
}
```

---

## VM Restart (Experimental)

HLFFI supports restarting the VM within a single process:

```c
for (int session = 0; session < 3; session++)
{
    // Create fresh VM
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Run game session
    while (session_running)
    {
        hlffi_update(vm, dt);
        // ...
    }

    // Full cleanup
    hlffi_close(vm);
    hlffi_destroy(vm);

    // Brief pause before restart
    sleep(1);
}
```

**Use cases:**
- Full state reset between levels
- Session-based applications
- Testing scenarios

**Note:** This is experimental. For code changes during development, prefer hot reload.

---

## Exception Handling

Catch Haxe exceptions in C:

```c
// Safe call that catches exceptions
hlffi_call_result res = hlffi_try_call_static(
    vm, "File", "load", 1, args
);

if (res.exception)
{
    // Exception was thrown
    char* msg = hlffi_get_exception_message(res.exception);
    char* stack = hlffi_get_exception_stack(res.exception);

    printf("Exception: %s\n", msg);
    printf("Stack:\n%s\n", stack);

    free(msg);
    free(stack);
    hlffi_value_free(res.exception);
}
else
{
    // Success
    if (res.value)
    {
        // Use the return value
        hlffi_value_free(res.value);
    }
}
```

---

## Complete Example: Game Engine Integration

Here's a complete example of integrating HLFFI into a game engine:

**game.hx:**
```haxe
class Game
{
    static var player:Player;
    static var score:Int = 0;

    public static function init():Void
    {
        player = new Player(100, 100);
        trace("Game initialized");
    }

    public static function update(dt:Float):Void
    {
        player.update(dt);
    }

    public static function render():Void
    {
        player.render();
    }

    public static function onInput(key:String, pressed:Bool):Void
    {
        if (pressed)
        {
            switch (key)
            {
                case "left": player.moveLeft();
                case "right": player.moveRight();
                case "jump": player.jump();
            }
        }
    }

    public static function getScore():Int
    {
        return score;
    }

    public static function addScore(points:Int):Void
    {
        score += points;
        trace('Score: $score');
    }
}

class Player
{
    public var x:Float;
    public var y:Float;
    public var vx:Float = 0;
    public var vy:Float = 0;

    public function new(x:Float, y:Float)
    {
        this.x = x;
        this.y = y;
    }

    public function update(dt:Float):Void
    {
        x += vx * dt;
        y += vy * dt;
        vy += 9.8 * dt;  // Gravity
    }

    public function render():Void
    {
        // Rendering handled by C side
    }

    public function moveLeft():Void { vx = -100; }
    public function moveRight():Void { vx = 100; }
    public function jump():Void { vy = -200; }
}

class Main
{
    public static function main()
    {
        Game.init();
    }
}
```

**main.c:**
```c
#include <stdio.h>
#include "hlffi.h"

// Cached methods for performance
static hlffi_cached_call* game_update = NULL;
static hlffi_cached_call* game_render = NULL;

// Callback: Get player position
hlffi_value* get_player_pos(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    // Return current player position to C
    hlffi_value* player = hlffi_get_static_field(vm, "Game", "player");
    if (!player) return hlffi_value_null(vm);

    float x = hlffi_get_field_float(player, "x", 0);
    float y = hlffi_get_field_float(player, "y", 0);

    printf("Player at (%.1f, %.1f)\n", x, y);

    hlffi_value_free(player);
    return hlffi_value_null(vm);
}

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);

    // Register callbacks
    hlffi_register_callback(vm, "getPlayerPos", get_player_pos, 0);

    // Enable hot reload for development
    hlffi_enable_hot_reload(vm, true);

    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Cache hot path methods
    game_update = hlffi_cache_static_method(vm, "Game", "update");
    game_render = hlffi_cache_static_method(vm, "Game", "render");

    // Game loop
    float dt = 1.0f / 60.0f;
    for (int frame = 0; frame < 300; frame++)  // 5 seconds
    {
        // Check for hot reload
        hlffi_check_reload(vm);

        // Process Haxe events
        hlffi_update(vm, dt);

        // Update game (using cached call)
        hlffi_value* dt_val = hlffi_value_float(vm, dt);
        hlffi_value* args[] = {dt_val};
        hlffi_call_cached(game_update, 1, args);
        hlffi_value_free(dt_val);

        // Render (using cached call)
        hlffi_call_cached(game_render, 0, NULL);

        // Simulate input
        if (frame == 60)
        {
            hlffi_value* key = hlffi_value_string(vm, "jump");
            hlffi_value* pressed = hlffi_value_bool(vm, true);
            hlffi_value* input_args[] = {key, pressed};
            hlffi_call_static(vm, "Game", "onInput", 2, input_args);
            hlffi_value_free(key);
            hlffi_value_free(pressed);
        }

        // Add score
        if (frame % 60 == 0)
        {
            hlffi_value* points = hlffi_value_int(vm, 10);
            hlffi_value* score_args[] = {points};
            hlffi_call_static(vm, "Game", "addScore", 1, score_args);
            hlffi_value_free(points);
        }
    }

    // Get final score
    hlffi_value* score = hlffi_call_static(vm, "Game", "getScore", 0, NULL);
    printf("Final score: %d\n", hlffi_value_as_int(score, 0));
    hlffi_value_free(score);

    // Cleanup
    hlffi_cache_free(game_update);
    hlffi_cache_free(game_render);
    hlffi_close(vm);
    hlffi_destroy(vm);

    return 0;
}
```

---

## Quick Reference

### Integration Modes
```c
hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);  // Default
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
hlffi_update(vm, dt);  // Non-threaded: call every frame
hlffi_thread_start(vm);  // Threaded: start background thread
hlffi_thread_stop(vm);   // Threaded: stop thread
```

### Hot Reload
```c
hlffi_enable_hot_reload(vm, true);  // Before load
hlffi_reload_module(vm, "game.hl");  // Manual reload
hlffi_check_reload(vm);  // Auto-reload if changed
hlffi_set_reload_callback(vm, callback, data);
```

### Callbacks
```c
hlffi_register_callback(vm, "name", func, nargs);  // Before load
// Callback signature:
hlffi_value* func(hlffi_vm* vm, int argc, hlffi_value** argv);
```

### Threading
```c
hlffi_worker_register();    // Before using HLFFI in thread
hlffi_worker_unregister();  // Before thread exits
hlffi_blocking_begin();     // Before slow I/O
hlffi_blocking_end();       // After slow I/O
```

### Caching
```c
hlffi_cached_call* c = hlffi_cache_static_method(vm, "Class", "method");
hlffi_call_cached(c, argc, argv);
hlffi_cache_free(c);
```

### Exceptions
```c
hlffi_call_result res = hlffi_try_call_static(vm, "Class", "method", argc, argv);
if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    // ...
}
```

---

## Further Reading

- [API Reference](../API_REFERENCE.md) - Complete API documentation
- [VM Lifecycle](../API_01_VM_LIFECYCLE.md) - Detailed lifecycle information
- [Hot Reload](../API_05_HOT_RELOAD.md) - Full hot reload documentation
- [Threading](../API_04_THREADING.md) - Complete threading guide
- [Performance](../API_17_PERFORMANCE.md) - Optimization techniques

---

## Summary

You've learned:
1. **Getting Started** - Basic setup and lifecycle
2. **Calling Haxe** - Functions, fields, and objects
3. **Data Exchange** - Arrays, maps, bytes, and enums
4. **Advanced Topics** - Modes, hot reload, callbacks, and performance

HLFFI makes it easy to integrate Haxe into your C/C++ applications. Start simple, and add advanced features as needed.

Happy coding!
