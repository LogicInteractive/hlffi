# HLFFI Callback Usage Guide

**Complete guide to bidirectional C↔Haxe callbacks**

---

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Callback Patterns](#callback-patterns)
4. [API Reference](#api-reference)
5. [Best Practices](#best-practices)
6. [Common Patterns](#common-patterns)
7. [Troubleshooting](#troubleshooting)
8. [Examples](#examples)

---

## Overview

HLFFI callbacks enable **bidirectional communication** between C and Haxe:

- **C→Haxe**: Call Haxe functions from C (`hlffi_call_static`, `hlffi_call_method`)
- **Haxe→C**: Call C functions from Haxe (callbacks - THIS GUIDE)

### When to Use Callbacks

✅ **Good use cases:**
- Event-driven architectures (button clicks, game events)
- Native platform integration (OS callbacks, hardware events)
- Performance-critical operations (rendering, physics)
- Platform-specific functionality (file I/O, networking)

❌ **Not recommended for:**
- Tight inner loops (use direct Haxe instead)
- Simple data queries (use static fields or methods)
- Synchronous request-response (use static method calls)

---

## Quick Start

### Step 1: Define C Callback Function

```c
/* Callback signature: hlffi_value* func(hlffi_vm* vm, int argc, hlffi_value** args) */
static hlffi_value* on_button_click(hlffi_vm* vm, int argc, hlffi_value** args)
{
    /* Extract arguments */
    const char* button_id = hlffi_value_as_string(args[0]);

    /* Do something in C */
    printf("Button clicked: %s\n", button_id);

    /* Return value (or NULL for Void) */
    return hlffi_value_null(vm);
}
```

### Step 2: Register Callback

```c
/* Register with name and argument count */
if (!hlffi_register_callback(vm, "onButtonClick", on_button_click, 1))
{
    fprintf(stderr, "Failed to register callback: %s\n", hlffi_get_error(vm));
    return -1;
}
```

### Step 3: Pass to Haxe

```c
/* Get callback as hlffi_value */
hlffi_value* callback = hlffi_get_callback(vm, "onButtonClick");

/* Set as static field in Haxe */
hlffi_set_static_field(vm, "UI", "onButtonClick", callback);

/* Free the wrapper (not the callback itself - it's GC-managed) */
hlffi_value_free(callback);
```

### Step 4: Call from Haxe

```haxe
class UI
{
    public static var onButtonClick:Dynamic;  /* Must be Dynamic! */

    public static function handleClick(id:String):Void
    {
        if (onButtonClick != null)
        {
            onButtonClick(id);  /* Calls C function! */
        }
    }
}
```

---

## Callback Patterns

### Pattern 1: Simple Notification (Void→Void)

**Use case**: Notify C that something happened, no data needed.

```c
static hlffi_value* on_game_started(hlffi_vm* vm, int argc, hlffi_value** args)
{
    (void)argc; (void)args;  /* Unused */

    printf("Game started!\n");
    init_audio_system();
    load_resources();

    return hlffi_value_null(vm);
}

/* Register */
hlffi_register_callback(vm, "onGameStarted", on_game_started, 0);
```

```haxe
/* Haxe side */
class Game
{
    public static var onGameStarted:Dynamic;

    static function startGame():Void
    {
        if (onGameStarted != null) onGameStarted();
    }
}
```

### Pattern 2: Data Passing (Args→Void)

**Use case**: Send data from Haxe to C for processing.

```c
static hlffi_value* on_player_scored(hlffi_vm* vm, int argc, hlffi_value** args)
{
    int points = hlffi_value_as_int(args[0], 0);
    const char* reason = hlffi_value_as_string(args[1]);

    update_score(points);
    log_achievement(reason);
    play_sound("score.wav");

    return hlffi_value_null(vm);
}

/* Register */
hlffi_register_callback(vm, "onPlayerScored", on_player_scored, 2);
```

```haxe
/* Haxe side */
class Player
{
    public static var onPlayerScored:Dynamic;

    function collectCoin():Void
    {
        if (onPlayerScored != null)
        {
            onPlayerScored(100, "Coin collected");
        }
    }
}
```

### Pattern 3: Query with Return Value (Args→Result)

**Use case**: Haxe asks C for information.

```c
static hlffi_value* calculate_physics(hlffi_vm* vm, int argc, hlffi_value** args)
{
    float velocity = hlffi_value_as_float(args[0], 0);
    float mass = hlffi_value_as_float(args[1], 0);

    /* C performs complex calculation */
    float force = velocity * mass * 9.8f;

    return hlffi_value_float(vm, force);
}

/* Register */
hlffi_register_callback(vm, "calculatePhysics", calculate_physics, 2);
```

```haxe
/* Haxe side */
class Physics
{
    public static var calculatePhysics:Dynamic;

    function getForce(vel:Float, mass:Float):Float
    {
        if (calculatePhysics != null)
        {
            var result:Dynamic = calculatePhysics(vel, mass);
            return result;  /* Auto-converts to Float */
        }
        return 0.0;
    }
}
```

### Pattern 4: C Calls Back into Haxe (Bidirectional)

**Use case**: C callback triggers more Haxe code.

```c
static hlffi_value* on_file_loaded(hlffi_vm* vm, int argc, hlffi_value** args)
{
    const char* filename = hlffi_value_as_string(args[0]);

    /* Load file in C */
    FileData* data = load_file_sync(filename);

    if (data)
    {
        /* Success - notify Haxe */
        hlffi_value* success_arg = hlffi_value_bool(vm, true);
        hlffi_value* args[] = {success_arg};
        hlffi_call_static(vm, "FileLoader", "onLoadComplete", 1, args);
        hlffi_value_free(success_arg);
    }
    else
    {
        /* Failure - tell Haxe */
        hlffi_value* error_msg = hlffi_value_string(vm, "File not found");
        hlffi_value* args[] = {error_msg};
        hlffi_call_static(vm, "FileLoader", "onLoadError", 1, args);
        hlffi_value_free(error_msg);
    }

    return hlffi_value_null(vm);
}
```

---

## API Reference

### Registration

```c
bool hlffi_register_callback(
    hlffi_vm* vm,
    const char* name,        /* Callback name (for retrieval) */
    hlffi_native_func func,  /* Your C function */
    int nargs                /* Argument count (0-4) */
);
```

**Returns**: `true` on success, `false` on failure (check `hlffi_get_error(vm)`)

**Limitations**:
- Maximum 4 arguments (0-4 supported)
- Name must be unique per VM
- Function pointer must remain valid for VM lifetime

### Retrieval

```c
hlffi_value* hlffi_get_callback(hlffi_vm* vm, const char* name);
```

**Returns**: `hlffi_value*` wrapping the callback, or `NULL` if not found

**Important**: Free the wrapper with `hlffi_value_free()`, but the callback itself is GC-managed!

### Unregistration

```c
bool hlffi_unregister_callback(hlffi_vm* vm, const char* name);
```

**Returns**: `true` if found and removed, `false` if not found

**Note**: GC will clean up the callback automatically after removal.

### Callback Signature

```c
typedef hlffi_value* (*hlffi_native_func)(
    hlffi_vm* vm,          /* VM instance */
    int argc,              /* Actual argument count */
    hlffi_value** args     /* Argument array */
);
```

---

## Best Practices

### ✅ DO

1. **Check argument count**
   ```c
   if (argc != expected_count)
   {
       fprintf(stderr, "Wrong arg count\n");
       return hlffi_value_null(vm);
   }
   ```

2. **Validate arguments**
   ```c
   int value = hlffi_value_as_int(args[0], -1);  /* Default -1 */
   if (value == -1)
   {
       /* Handle invalid input */
   }
   ```

3. **Free temporary values**
   ```c
   hlffi_value* result = hlffi_value_string(vm, "test");
   hlffi_call_static(vm, "Foo", "bar", 1, &result);
   hlffi_value_free(result);  /* Important! */
   ```

4. **Use Dynamic type in Haxe**
   ```haxe
   public static var callback:Dynamic;  /* ✓ Works */
   /* NOT: public static var callback:(Int)->Void;  ✗ Crashes! */
   ```

5. **Return appropriate values**
   ```c
   return hlffi_value_null(vm);    /* For Void */
   return hlffi_value_int(vm, 42); /* For Int */
   return hlffi_value_string(vm, "hello"); /* For String */
   ```

### ❌ DON'T

1. **Don't use typed callbacks in Haxe**
   ```haxe
   /* ✗ WRONG - will crash! */
   public static var onScore:(Int)->Void;

   /* ✓ CORRECT - use Dynamic */
   public static var onScore:Dynamic;
   ```

2. **Don't forget to free hlffi_value wrappers**
   ```c
   /* ✗ MEMORY LEAK */
   hlffi_value* v = hlffi_value_int(vm, 10);
   /* ... use v ...*/
   /* Missing hlffi_value_free(v); */
   ```

3. **Don't manually free callbacks**
   ```c
   /* ✗ WRONG - callback is GC-managed! */
   hlffi_value* cb = hlffi_get_callback(vm, "foo");
   /* Don't manually free the underlying closure! */
   ```

4. **Don't exceed 4 arguments**
   ```c
   /* ✗ NOT SUPPORTED */
   hlffi_register_callback(vm, "tooMany", func, 5);  /* Fails! */

   /* ✓ Solution: Pack into struct/object */
   hlffi_register_callback(vm, "ok", func, 1);  /* Pass object with 5 fields */
   ```

5. **Don't call callbacks from multiple threads** (unless wrapped)
   ```c
   /* ✗ UNSAFE */
   void* worker_thread(void* arg)
   {
       on_my_callback(vm, 0, NULL);  /* NOT thread-safe! */
   }

   /* ✓ Use thread registration */
   void* worker_thread(void* arg)
   {
       hlffi_worker_register();
       on_my_callback(vm, 0, NULL);
       hlffi_worker_unregister();
   }
   ```

---

## Common Patterns

### Pattern: Event System

```c
/* Event dispatcher */
typedef void (*event_handler)(const char* event, void* data);

static hlffi_value* on_event(hlffi_vm* vm, int argc, hlffi_value** args)
{
    const char* event_type = hlffi_value_as_string(args[0]);

    /* Dispatch to native event system */
    if (strcmp(event_type, "pause") == 0)
    {
        pause_game();
    }
    else if (strcmp(event_type, "resume") == 0)
    {
        resume_game();
    }

    return hlffi_value_null(vm);
}

/* Haxe triggers events */
class Events
{
    public static var onEvent:Dynamic;

    public static function trigger(type:String):Void
    {
        if (onEvent != null) onEvent(type);
    }
}
```

### Pattern: Async Operations

```c
/* C performs async work, calls back when done */
static hlffi_value* start_download(hlffi_vm* vm, int argc, hlffi_value** args)
{
    const char* url = hlffi_value_as_string(args[0]);

    /* Start async download */
    download_async(url, [vm](bool success, const char* data)
    {
        /* On completion, call Haxe */
        hlffi_value* args[2] = {
            hlffi_value_bool(vm, success),
            hlffi_value_string(vm, data)
        };
        hlffi_call_static(vm, "Downloader", "onComplete", 2, args);
        hlffi_value_free(args[0]);
        hlffi_value_free(args[1]);
    });

    return hlffi_value_null(vm);
}
```

### Pattern: Resource Management

```c
/* C allocates resource, returns handle */
static hlffi_value* create_texture(hlffi_vm* vm, int argc, hlffi_value** args)
{
    int width = hlffi_value_as_int(args[0], 0);
    int height = hlffi_value_as_int(args[1], 0);

    /* Create native texture */
    int texture_id = gpu_create_texture(width, height);

    /* Store in resource manager */
    register_resource(texture_id);

    return hlffi_value_int(vm, texture_id);
}

static hlffi_value* destroy_texture(hlffi_vm* vm, int argc, hlffi_value** args)
{
    int texture_id = hlffi_value_as_int(args[0], 0);

    gpu_destroy_texture(texture_id);
    unregister_resource(texture_id);

    return hlffi_value_null(vm);
}
```

---

## Troubleshooting

### Problem: Callback Not Called

**Symptoms**: Haxe calls function, nothing happens in C

**Solutions**:
1. Check callback was registered: `hlffi_register_callback()` returned `true`
2. Check callback was set in Haxe: `hlffi_set_static_field()` succeeded
3. Verify Haxe field type is `Dynamic`, not typed function
4. Add debug printf in C callback to confirm it's being reached

### Problem: Crash on Callback Invocation

**Symptoms**: Segfault or crash when Haxe calls callback

**Solutions**:
1. **Most common**: Change Haxe type from `(Int)->Void` to `Dynamic`
2. Check argument count matches registration
3. Verify callback function pointer is still valid
4. Check for null VM pointer

### Problem: Memory Leak

**Symptoms**: Memory grows over time with callbacks

**Solutions**:
1. Free `hlffi_value*` wrappers after use: `hlffi_value_free(v)`
2. Don't manually free callback closures (they're GC-managed)
3. Unregister unused callbacks: `hlffi_unregister_callback()`

### Problem: Arguments Wrong Type

**Symptoms**: Get garbage data or crashes accessing arguments

**Solutions**:
1. Use correct `hlffi_value_as_*()` function for type
2. Provide fallback value: `hlffi_value_as_int(arg, 0)`
3. Check Haxe passes correct types (Int, Float, String, not custom classes)

---

## Examples

### Complete Example: See `examples/callback_event_system.c`

This example demonstrates:
- Event-driven game architecture
- Multiple callback types
- Bidirectional C↔Haxe communication
- State management across boundaries
- Return values from callbacks

**To run:**
```bash
cd examples
haxe -hl haxe/gamecallbacks.hl -main GameCallbacks
gcc -o callback_example callback_event_system.c \
    -I../include -I../vendor/hashlink/src \
    -L../bin -lhlffi -lhl -ldl -lm -lpthread -rdynamic
./callback_example haxe/gamecallbacks.hl
```

---

## See Also

- [PHASE6_COMPLETE.md](PHASE6_COMPLETE.md) - Implementation details
- [test_callbacks.c](../test_callbacks.c) - Unit tests
- [test/Callbacks.hx](../test/Callbacks.hx) - Test Haxe code

---

**Status**: Production ready
**Tested**: 14/14 tests passing
**Platforms**: Linux, Windows (MSVC)
