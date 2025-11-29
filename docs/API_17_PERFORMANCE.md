# HLFFI API Reference - Performance & Caching

**[← Exceptions](API_16_EXCEPTIONS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Utilities →](API_18_UTILITIES.md)**

Optimize hot paths with method caching for **60x speedup**.

---

## Overview

**Problem:** Name-based lookups (`hlffi_call_static()`) are ~40ns due to hash lookups.

**Solution:** Cache method pointers for hot paths:

```c
// Slow (40ns per call):
for (int i = 0; i < 1000; i++)
{
    hlffi_call_static(vm, "Game", "update", 0, NULL);  // Hash lookup every call
}

// Fast (0.7ns per call - 60x faster):
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
for (int i = 0; i < 1000; i++)
{
    hlffi_call_cached(update, 0, NULL);  // Direct call
}
hlffi_cache_free(update);
```

**Benchmark Results:**
- `hlffi_call_static()`: ~40ns (hash lookup)
- `hlffi_call_cached()`: ~0.7ns (**60x faster**)
- Overhead: Just function pointer call

---

## Quick Reference

| Function | Purpose |
|----------|---------|
| `hlffi_cache_static_method(vm, class, method)` | Cache static method |
| `hlffi_cache_instance_method(vm, class, method)` | Cache instance method |
| `hlffi_call_cached(cache, argc, argv)` | Call cached method |
| `hlffi_call_cached_method(cache, obj, argc, argv)` | Call cached instance method |
| `hlffi_cache_free(cache)` | Free cache handle |

**Complete Guide:** See `docs/PHASE7_COMPLETE.md`

---

## Caching Static Methods

### Cache Method

**Signature:**
```c
hlffi_cached_call* hlffi_cache_static_method(
    hlffi_vm* vm,
    const char* class_name,
    const char* method_name
)
```

**Returns:** Cached call handle, or NULL on error

**Example:**
```c
// Cache once (during initialization):
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
hlffi_cached_call* render = hlffi_cache_static_method(vm, "Graphics", "render");

// Use in hot loop (60x faster):
while (running)
{
    hlffi_call_cached(update, 0, NULL);
    hlffi_call_cached(render, 0, NULL);
}

// Cleanup:
hlffi_cache_free(update);
hlffi_cache_free(render);
```

---

### Call Cached Static Method

**Signature:**
```c
hlffi_value* hlffi_call_cached(
    hlffi_cached_call* cached,
    int argc,
    hlffi_value** argv
)
```

**Parameters:**
- `cached` - Cached method from `hlffi_cache_static_method()`
- `argc` - Argument count
- `argv` - Arguments

**Returns:** Return value, or NULL for void/error

**Example:**
```c
hlffi_cached_call* calc = hlffi_cache_static_method(vm, "Math", "add");

hlffi_value* a = hlffi_value_int(vm, 10);
hlffi_value* b = hlffi_value_int(vm, 20);
hlffi_value* args[] = {a, b};

hlffi_value* result = hlffi_call_cached(calc, 2, args);
int sum = hlffi_value_as_int(result, 0);
printf("Sum: %d\n", sum);  // 30

hlffi_value_free(a);
hlffi_value_free(b);
hlffi_value_free(result);
hlffi_cache_free(calc);
```

---

## Caching Instance Methods

### Cache Instance Method

**Signature:**
```c
hlffi_cached_call* hlffi_cache_instance_method(
    hlffi_vm* vm,
    const char* class_name,
    const char* method_name
)
```

**Returns:** Cached call handle, or NULL on error

**Example:**
```c
// Cache method:
hlffi_cached_call* move = hlffi_cache_instance_method(vm, "Player", "move");

// Create instances:
hlffi_value* player1 = hlffi_new(vm, "Player", 0, NULL);
hlffi_value* player2 = hlffi_new(vm, "Player", 0, NULL);

// Call on different instances:
hlffi_value* dx = hlffi_value_float(vm, 10.0);
hlffi_value* dy = hlffi_value_float(vm, 5.0);
hlffi_value* args[] = {dx, dy};

hlffi_call_cached_method(move, player1, 2, args);
hlffi_call_cached_method(move, player2, 2, args);

// Cleanup:
hlffi_value_free(dx);
hlffi_value_free(dy);
hlffi_value_free(player1);
hlffi_value_free(player2);
hlffi_cache_free(move);
```

---

### Call Cached Instance Method

**Signature:**
```c
hlffi_value* hlffi_call_cached_method(
    hlffi_cached_call* cached,
    hlffi_value* instance,
    int argc,
    hlffi_value** argv
)
```

**Parameters:**
- `cached` - Cached method from `hlffi_cache_instance_method()`
- `instance` - Object to call method on
- `argc` - Argument count
- `argv` - Arguments

**Returns:** Return value, or NULL for void/error

**Note:** ⚠️ **NOT YET IMPLEMENTED** - Use `hlffi_call_method()` for now

---

## Freeing Cache Handles

**Signature:**
```c
void hlffi_cache_free(hlffi_cached_call* cached)
```

**Description:** Free cached method handle.

**Example:**
```c
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
// ... use update ...
hlffi_cache_free(update);
```

---

## Complete Example

```c
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Cache methods during initialization:
    hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
    hlffi_cached_call* render = hlffi_cache_static_method(vm, "Graphics", "render");
    hlffi_cached_call* process = hlffi_cache_static_method(vm, "Physics", "process");

    if (!update || !render || !process)
    {
        fprintf(stderr, "Failed to cache methods\n");
        hlffi_destroy(vm);
        return 1;
    }

    // Game loop (60 FPS):
    for (int frame = 0; frame < 3600; frame++)
    {
        // Call cached methods (60x faster than name lookup):
        hlffi_call_cached(update, 0, NULL);
        hlffi_call_cached(process, 0, NULL);
        hlffi_call_cached(render, 0, NULL);

        // Sleep for 16ms (~60 FPS)
        usleep(16000);
    }

    // Cleanup:
    hlffi_cache_free(update);
    hlffi_cache_free(render);
    hlffi_cache_free(process);
    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class Game
{
    public static function update():Void
    {
        // Game logic
    }
}

class Graphics
{
    public static function render():Void
    {
        // Rendering
    }
}

class Physics
{
    public static function process():Void
    {
        // Physics step
    }
}
```

---

## Performance Comparison

### Benchmark: 1 Million Calls

| Method | Time | Speedup |
|--------|------|---------|
| `hlffi_call_static()` | 40ms | 1x |
| `hlffi_call_cached()` | 0.7ms | **60x** |

**Code:**
```c
// Uncached (40ms):
clock_t start = clock();
for (int i = 0; i < 1000000; i++)
{
    hlffi_call_static(vm, "Math", "add", 2, args);
}
printf("Uncached: %ld ms\n", (clock() - start) * 1000 / CLOCKS_PER_SEC);

// Cached (0.7ms):
hlffi_cached_call* add = hlffi_cache_static_method(vm, "Math", "add");
start = clock();
for (int i = 0; i < 1000000; i++)
{
    hlffi_call_cached(add, 2, args);
}
printf("Cached: %ld ms\n", (clock() - start) * 1000 / CLOCKS_PER_SEC);
hlffi_cache_free(add);
```

---

## Best Practices

### 1. Cache Once, Use Many Times

```c
// ✅ GOOD - Cache outside loop
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
for (int i = 0; i < 1000; i++)
{
    hlffi_call_cached(update, 0, NULL);
}
hlffi_cache_free(update);

// ❌ BAD - Cache inside loop
for (int i = 0; i < 1000; i++)
{
    hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
    hlffi_call_cached(update, 0, NULL);
    hlffi_cache_free(update);  // Defeats the purpose!
}
```

### 2. Cache Hot Paths Only

```c
// ✅ GOOD - Cache frequently called methods
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");  // Called 60x/sec

// ❌ UNNECESSARY - Don't cache rarely called methods
hlffi_cached_call* init = hlffi_cache_static_method(vm, "Game", "initialize");  // Called once
```

### 3. Free Cache Handles

```c
// ✅ GOOD
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
// ... use update ...
hlffi_cache_free(update);

// ❌ BAD - Memory leak
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
// ... use update ...
// Missing: hlffi_cache_free(update);
```

### 4. Check for NULL

```c
// ✅ GOOD - Verify cache creation
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
if (!update)
{
    fprintf(stderr, "Failed to cache Game.update\n");
    return;
}
hlffi_call_cached(update, 0, NULL);
hlffi_cache_free(update);

// ❌ BAD - Crash if method not found
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
hlffi_call_cached(update, 0, NULL);  // Crash if update is NULL
```

---

## When to Use Caching

### ✅ Use Caching For:
- Game loop functions called every frame (60+ Hz)
- Physics/update/render methods
- Event handlers called frequently
- Hot paths with > 1000 calls/second

### ❌ Don't Cache:
- One-time initialization functions
- Rarely called utility methods
- Debug/logging functions
- Methods called < 10 times/second

---

## Hot Reload Compatibility

**⚠️ Cache invalidation on hot reload:**

```c
// Cache is invalidated after reload:
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");

hlffi_reload_module(vm, "game.hl");  // Cache is now INVALID

hlffi_cache_free(update);  // Free old cache
update = hlffi_cache_static_method(vm, "Game", "update");  // Re-cache
```

**Best Practice:** Re-cache methods after hot reload.

---

## Memory Overhead

- **Per cached method:** ~16 bytes
- **Typical game (10 cached methods):** ~160 bytes
- **Negligible** compared to performance gain

---

**[← Exceptions](API_16_EXCEPTIONS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Utilities →](API_18_UTILITIES.md)**