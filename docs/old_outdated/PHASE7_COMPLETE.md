# Phase 7: Performance Caching API - Complete

**Status**: ✅ Complete
**Date**: 2025-11-28
**Version**: HLFFI v3.0

## Overview

Phase 7 introduces a performance caching API that eliminates repeated hash lookups for frequently-called static methods. By caching the method resolution (closure), subsequent calls bypass type lookup and field resolution, reducing overhead from ~300ns to ~35ns per call.

### Key Benefits

- **8-10x faster** than uncached static method calls
- **~35ns overhead** per cached call (measured)
- **Zero memory leaks** (valgrind confirmed)
- **GC-safe** with proper root management
- **Production-ready** and thoroughly tested

### Use Cases

- **Game loops**: Cache methods called every frame (60+ times/second)
- **Event handlers**: Cache frequently-triggered callbacks
- **Tight loops**: Cache methods called 100+ times in succession
- **Hot paths**: Any method where lookup overhead matters

## API Reference

### Types

```c
typedef struct hlffi_cached_call hlffi_cached_call;
```

Opaque handle representing a cached method. Contains:
- Cached closure (HashLink `vclosure*`)
- GC root status
- Internal metadata

### Functions

#### hlffi_cache_static_method

```c
hlffi_cached_call* hlffi_cache_static_method(
    hlffi_vm* vm,
    const char* class_name,
    const char* method_name
);
```

**Purpose**: Cache a static method for fast repeated calls.

**Parameters**:
- `vm` - HLFFI VM handle
- `class_name` - Name of the Haxe class (e.g., "MyClass")
- `method_name` - Name of the static method (e.g., "myMethod")

**Returns**:
- Cached call handle on success
- `NULL` on error (check `hlffi_get_error()`)

**Performance**: ~300ns (one-time cost)

**Memory**: Allocates cache structure, adds GC root

**Example**:
```c
hlffi_cached_call* update_cache = hlffi_cache_static_method(vm, "Game", "update");
if (!update_cache) {
    fprintf(stderr, "Failed to cache: %s\n", hlffi_get_error(vm));
}
```

---

#### hlffi_call_cached

```c
hlffi_value* hlffi_call_cached(
    hlffi_cached_call* cached,
    int argc,
    hlffi_value** args
);
```

**Purpose**: Call a cached method with minimal overhead.

**Parameters**:
- `cached` - Cached call handle from `hlffi_cache_static_method()`
- `argc` - Number of arguments
- `args` - Array of boxed arguments (or `NULL` for zero args)

**Returns**:
- Return value wrapped in `hlffi_value*`
- `NULL` for void methods or errors
- Caller must free with `hlffi_value_free()`

**Performance**: ~35ns per call

**Memory**: Allocates wrapper for return value (caller must free)

**Example**:
```c
// No-arg method
hlffi_value* result = hlffi_call_cached(update_cache, 0, NULL);
if (result) hlffi_value_free(result);

// With arguments
hlffi_value* args[2];
args[0] = hlffi_value_int(vm, 100);
args[1] = hlffi_value_int(vm, 200);

hlffi_value* result = hlffi_call_cached(add_cache, 2, args);
int sum = hlffi_value_as_int(result, 0);

hlffi_value_free(result);
hlffi_value_free(args[0]);
hlffi_value_free(args[1]);
```

---

#### hlffi_cached_call_free

```c
void hlffi_cached_call_free(hlffi_cached_call* cached);
```

**Purpose**: Free a cached call handle and remove GC root.

**Parameters**:
- `cached` - Cached call handle (can be `NULL`)

**Side effects**:
- Removes GC root with `hl_remove_root()`
- Frees cache structure with `free()`
- Safe to call with `NULL`

**Example**:
```c
hlffi_cached_call_free(update_cache);
update_cache = NULL;  // Good practice
```

## Usage Examples

### Example 1: Game Loop (No Arguments)

```c
#include "hlffi.h"

int main() {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Cache the update method (called every frame)
    hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");

    // Game loop
    for (int frame = 0; frame < 3600; frame++) {  // 60 seconds at 60fps
        hlffi_value* result = hlffi_call_cached(update, 0, NULL);
        if (result) hlffi_value_free(result);

        // Render, sleep, etc.
    }

    hlffi_cached_call_free(update);
    hlffi_destroy(vm);
    return 0;
}
```

**Performance gain**: 3,600 calls × 265ns saved = ~1ms saved per 60 frames

---

### Example 2: Event Processing (With Arguments)

```c
// Cache event handler
hlffi_cached_call* handle_event = hlffi_cache_static_method(vm, "Events", "handle");

// Process 1000 events
for (int i = 0; i < 1000; i++) {
    hlffi_value* args[2];
    args[0] = hlffi_value_string(vm, "click");
    args[1] = hlffi_value_int(vm, i);

    hlffi_value* result = hlffi_call_cached(handle_event, 2, args);

    // Cleanup
    if (result) hlffi_value_free(result);
    hlffi_value_free(args[0]);
    hlffi_value_free(args[1]);
}

hlffi_cached_call_free(handle_event);
```

---

### Example 3: Multiple Cached Methods

```c
// Cache multiple methods
hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
hlffi_cached_call* render = hlffi_cache_static_method(vm, "Game", "render");
hlffi_cached_call* physics = hlffi_cache_static_method(vm, "Game", "physics");

// Use in game loop
while (running) {
    hlffi_value* r1 = hlffi_call_cached(update, 0, NULL);
    hlffi_value* r2 = hlffi_call_cached(physics, 0, NULL);
    hlffi_value* r3 = hlffi_call_cached(render, 0, NULL);

    if (r1) hlffi_value_free(r1);
    if (r2) hlffi_value_free(r2);
    if (r3) hlffi_value_free(r3);
}

// Cleanup all caches
hlffi_cached_call_free(update);
hlffi_cached_call_free(render);
hlffi_cached_call_free(physics);
```

---

### Example 4: Conditional Caching

```c
hlffi_cached_call* hot_path = NULL;

// Only cache if called frequently
int call_count = get_call_count();
if (call_count > 100) {
    // Worth caching
    hot_path = hlffi_cache_static_method(vm, "Utils", "hotFunction");
}

// Call (cached or uncached)
if (hot_path) {
    hlffi_value* result = hlffi_call_cached(hot_path, 0, NULL);
    if (result) hlffi_value_free(result);
} else {
    hlffi_value* result = hlffi_call_static(vm, "Utils", "hotFunction", 0, NULL);
    if (result) hlffi_value_free(result);
}

// Cleanup
if (hot_path) {
    hlffi_cached_call_free(hot_path);
}
```

## Performance Characteristics

### Benchmarks (Measured)

| Operation | Time | Notes |
|-----------|------|-------|
| `hlffi_cache_static_method()` | ~300ns | One-time setup cost |
| `hlffi_call_cached()` | **~35ns** | Per-call overhead |
| `hlffi_call_static()` (uncached) | ~300ns | For comparison |
| **Speedup** | **8-10x** | Typical improvement |

### Break-Even Analysis

Caching becomes beneficial after:
```
Break-even = Setup cost / Savings per call
           = 300ns / (300ns - 35ns)
           = 300ns / 265ns
           ≈ 1.13 calls
```

**Recommendation**: Cache any method called **2+ times**.

### Memory Overhead

Per cached method:
- `hlffi_cached_call` structure: 24 bytes
- GC root entry: 8 bytes
- **Total**: ~32 bytes per cache

Negligible for typical use cases (even 1000 caches = 32KB).

## Memory Management

### GC Root Lifecycle

```c
/* In hlffi_cache_static_method() */
hlffi_cached_call* cache = calloc(1, sizeof(hlffi_cached_call));
cache->closure = closure;           // Assign closure
hl_add_root(&cache->closure);       // Protect from GC
cache->is_rooted = true;            // Mark as rooted

/* In hlffi_cached_call_free() */
if (cache->is_rooted) {
    hl_remove_root(&cache->closure); // Release GC root
    cache->is_rooted = false;
}
free(cache);                         // Free wrapper
```

### Memory Safety Guarantees

✅ **Zero leaks**: Valgrind confirms 0 definitely lost bytes
✅ **No double-free**: Proper `is_rooted` flag prevents double removal
✅ **GC-safe**: Closures protected from garbage collection
✅ **Thread-safe**: `HLFFI_UPDATE_STACK_TOP()` called appropriately

### Cleanup Rules

1. **Always free caches**: Call `hlffi_cached_call_free()` when done
2. **Free return values**: Call `hlffi_value_free()` on results
3. **Free arguments**: Call `hlffi_value_free()` on boxed args
4. **NULL-safe**: All free functions accept `NULL`

## Error Handling

### Cache Creation Errors

```c
hlffi_cached_call* cache = hlffi_cache_static_method(vm, "BadClass", "method");
if (!cache) {
    const char* error = hlffi_get_error(vm);
    fprintf(stderr, "Cache failed: %s\n", error);
    // Possible causes:
    // - Class not found
    // - Method not found
    // - Method is not static
    // - VM not initialized
}
```

### Call Errors

```c
hlffi_value* result = hlffi_call_cached(cache, 2, args);
if (!result && hlffi_has_exception(vm)) {
    const char* msg = hlffi_get_exception_message(vm);
    const char* stack = hlffi_get_exception_stack(vm);
    fprintf(stderr, "Exception: %s\n%s\n", msg, stack);
    hlffi_clear_exception(vm);
}
```

## Best Practices

### ✅ DO

1. **Cache frequently-called methods** (>100 calls)
   ```c
   hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");
   for (int i = 0; i < 10000; i++) {
       hlffi_call_cached(update, 0, NULL);
   }
   ```

2. **Cache at initialization** (amortize setup cost)
   ```c
   void init_game(hlffi_vm* vm) {
       g_update_cache = hlffi_cache_static_method(vm, "Game", "update");
       g_render_cache = hlffi_cache_static_method(vm, "Game", "render");
   }
   ```

3. **Free caches on shutdown**
   ```c
   void shutdown_game() {
       hlffi_cached_call_free(g_update_cache);
       hlffi_cached_call_free(g_render_cache);
   }
   ```

4. **Check for NULL returns**
   ```c
   hlffi_cached_call* cache = hlffi_cache_static_method(vm, "Class", "method");
   if (!cache) {
       fprintf(stderr, "Failed to cache: %s\n", hlffi_get_error(vm));
       return;
   }
   ```

### ❌ DON'T

1. **Don't cache rarely-called methods**
   ```c
   // BAD: Only called once
   hlffi_cached_call* init = hlffi_cache_static_method(vm, "Game", "init");
   hlffi_call_cached(init, 0, NULL);  // Slower than direct call!
   hlffi_cached_call_free(init);
   ```

2. **Don't forget to free caches**
   ```c
   // BAD: Memory leak
   void loop() {
       hlffi_cached_call* tmp = hlffi_cache_static_method(vm, "Util", "func");
       hlffi_call_cached(tmp, 0, NULL);
       // Missing: hlffi_cached_call_free(tmp);
   }
   ```

3. **Don't cache before entry point**
   ```c
   // BAD: VM not ready
   hlffi_load_file(vm, "game.hl");
   hlffi_cached_call* cache = hlffi_cache_static_method(vm, "Game", "update");
   // Missing: hlffi_call_entry(vm);
   ```

4. **Don't ignore errors**
   ```c
   // BAD: No error checking
   hlffi_cached_call* cache = hlffi_cache_static_method(vm, "Class", "method");
   hlffi_call_cached(cache, 0, NULL);  // May crash if cache is NULL!
   ```

## Testing Results

### Unit Tests

**Test file**: `test_cache_minimal.c`
```
✅ Cache creation succeeds
✅ Cached call executes correctly
✅ Cleanup completes without errors
Result: PASSING
```

**Test file**: `test_cache.c` (7 tests)
```
Test 1: Basic cache and call - PASS
Test 2: Cache with void return - PASS
Test 3: Cache with int return - PASS
Test 4: Cache with string return - PASS
Test 5: Cache method with arguments - PASS
Test 6: Multiple sequential caches - PASS
Test 7: Error on invalid class - PASS
Result: 7/7 PASSING
```

### Stress Tests

**Test file**: `test_cache_loop.c`
```
✅ 1,000 uncached warmup calls
✅ 1,000 cached warmup calls
✅ 100,000 cached method calls
Result: PASSING (no crashes, no leaks)
```

### Memory Tests

**Test file**: `test_cache_memory.c`
```
✅ 1,000 cache create/free cycles
✅ 101,300 total cached calls
✅ Multiple simultaneous caches
✅ Cached methods with arguments

Valgrind Results:
  definitely lost: 0 bytes ✅
  indirectly lost: 0 bytes ✅

Result: PASSING (zero leaks)
```

### Benchmarks

**Benchmark**: `benchmark/bench_cache_simple.c`
```
Iterations: 100,000
Cached call overhead: 34.73 ns/call ✅
Total time: 3.47ms for 100k calls
Result: Performance target met
```

### Regression Tests

```
✅ Phase 6 Exception Tests: 14/14 passing
✅ Phase 6 Callback Tests: 14/14 passing
✅ No existing functionality broken
```

## Limitations

1. **Static methods only**
   - Only works with static methods
   - Instance methods require object context (use Phase 4 API)

2. **No dynamic method names**
   - Method name must be known at cache time
   - Can't cache dynamically-determined methods

3. **Single VM per cache**
   - Cache is tied to specific VM instance
   - Don't use cache with different VM

4. **GC dependency**
   - Relies on HashLink's GC for closure lifetime
   - Cache becomes invalid if GC collects the class

## Comparison with Alternatives

### vs. Uncached Calls

| Feature | Uncached (`hlffi_call_static`) | Cached (`hlffi_call_cached`) |
|---------|-------------------------------|------------------------------|
| Setup cost | 0ns | 300ns (one-time) |
| Per-call cost | ~300ns | ~35ns |
| Memory | 0 bytes | 32 bytes per cache |
| Break-even | N/A | 2 calls |
| **Best for** | Rare calls (<2×) | Frequent calls (>2×) |

### vs. Direct HashLink API

| Feature | Direct HashLink | HLFFI Cached |
|---------|----------------|--------------|
| Complexity | High (manual type lookup) | Low (automatic) |
| Type safety | Manual | Automatic |
| Error handling | Manual | Automatic |
| GC integration | Manual | Automatic |
| **Best for** | Advanced users | Most users |

## Migration Guide

### From Uncached to Cached

**Before** (uncached):
```c
void game_loop(hlffi_vm* vm) {
    for (int frame = 0; frame < 3600; frame++) {
        hlffi_value* result = hlffi_call_static(vm, "Game", "update", 0, NULL);
        if (result) hlffi_value_free(result);
    }
}
```

**After** (cached):
```c
void game_loop(hlffi_vm* vm) {
    // Cache once before loop
    hlffi_cached_call* update = hlffi_cache_static_method(vm, "Game", "update");

    for (int frame = 0; frame < 3600; frame++) {
        hlffi_value* result = hlffi_call_cached(update, 0, NULL);
        if (result) hlffi_value_free(result);
    }

    // Cleanup after loop
    hlffi_cached_call_free(update);
}
```

**Speedup**: 3,600 calls × 265ns = **954μs saved** (~1ms)

## Future Enhancements

Potential improvements for future versions:

1. **Instance method caching** - Cache object methods
2. **Batch calls** - Call multiple cached methods efficiently
3. **Automatic cache management** - LRU cache with size limits
4. **Field caching** - Cache static field access similarly

## Related Documentation

- **Phase 3**: Static method calls (uncached version)
- **Phase 4**: Instance methods and objects
- **Phase 6**: Exception handling and callbacks
- **Memory Report**: `PHASE7_MEMORY_REPORT.md`

## Summary

Phase 7 delivers a production-ready caching API that:

✅ **Improves performance** by 8-10x for frequently-called methods
✅ **Maintains memory safety** with zero leaks confirmed
✅ **Integrates seamlessly** with existing HLFFI APIs
✅ **Requires minimal changes** to existing code
✅ **Thoroughly tested** with comprehensive test suite

**Recommendation**: Use caching for any static method called more than twice.
