# GC Stack Scanning in Embedded HashLink

This document explains a critical issue when embedding HashLink and how HLFFI handles it.

## The Problem

When HashLink's garbage collector (GC) runs, it needs to find all live objects to avoid collecting them prematurely. One place it looks is the **C call stack** - local variables that might hold pointers to Haxe objects.

The GC scans memory between two pointers:
```
stack_top (registered when thread starts)
    |
    v  (scans downward through memory)
    |
    v
current_stack_pointer (where CPU is now)
```

### What Went Wrong

In `hlffi_init()`, we originally registered `stack_top` like this:

```c
// The VM struct is allocated on the HEAP with malloc()
hlffi_vm* vm = calloc(1, sizeof(hlffi_vm));

// WRONG: This points to HEAP memory, not the actual call stack!
hl_register_thread(&vm->stack_context);
```

The `vm` struct lives on the heap, so `&vm->stack_context` points to heap memory. When the GC tried to "scan the stack", it was actually scanning random heap memory, missing object references that existed on the real call stack.

### Symptoms

In Debug builds with `GC_DEBUG` enabled, this caused:
```
allocator.c(369) : FATAL ERROR : assert
```

The assertion checks that memory being allocated contains the `0xDD` pattern (freed memory marker). When stack scanning is broken, the GC may incorrectly collect live objects, corrupting the heap.

In Release builds, this could cause random crashes or memory corruption.

## The Solution

### Approach 1: Internal Fix (Current Implementation)

HLFFI now updates `stack_top` automatically inside every function that allocates GC memory:

```c
// Internal macro in hlffi_values.c
#define HLFFI_UPDATE_STACK_TOP() \
    do { \
        hl_thread_info* _t = hl_get_thread(); \
        if (_t) { \
            int _stack_marker;              /* Local variable ON THE STACK */ \
            _t->stack_top = &_stack_marker; /* Tell GC: scan from HERE */ \
        } \
    } while(0)
```

This is called at the start of:
- `hlffi_value_int()`
- `hlffi_value_float()`
- `hlffi_value_bool()`
- `hlffi_value_string()`
- `hlffi_get_static_field()`
- `hlffi_set_static_field()`
- `hlffi_call_static()`

**Users don't need to do anything special** - the fix is transparent.

### Approach 2: Manual Fix (Fallback)

If the internal fix ever proves insufficient (e.g., in complex threading scenarios), users can manually update `stack_top` using the public API:

```c
void my_game_update(hlffi_vm* vm)
{
    // Tell GC where the current stack frame is
    HLFFI_ENTER_SCOPE();

    // Now safe to call HLFFI functions in loops
    for (int i = 0; i < 1000; i++)
    {
        hlffi_value* val = hlffi_value_float(vm, delta);
        hlffi_call_static(vm, "Game", "update", 1, &val);
        hlffi_value_free(val);
    }
}
```

Or using the lower-level function:

```c
void my_game_update(hlffi_vm* vm)
{
    int stack_marker;
    hlffi_update_stack_top(&stack_marker);

    // ... HLFFI calls ...
}
```

## Technical Details

### Why This Happens

HashLink's `hl_register_thread()` expects a pointer to a stack variable. From [HashLink issue #752](https://github.com/HaxeFoundation/hashlink/issues/752):

> The pointer passed to `hl_register_thread` must point to actual stack memory, not heap memory. The GC uses this pointer to determine the range of stack memory to scan for object roots.

### How Stack Scanning Works

1. When a thread is registered, `stack_top` is set to the provided pointer
2. During GC, HashLink scans memory from `stack_top` down to the current stack pointer
3. Any pointer-sized values in this range that look like valid object pointers are treated as roots
4. Objects reachable from roots are kept alive; unreachable objects are collected

### Why the Internal Fix Works

By updating `stack_top` at the start of each HLFFI function:
1. The GC sees the current C function's stack frame
2. Local variables holding `vdynamic*` pointers are in the scanned range
3. Objects are properly kept alive during function execution

## Testing

The `test_gameloop` test verifies this works correctly:
- Runs 60+ frames in tight loops
- Creates/frees `hlffi_value` objects each frame
- Would crash with the old broken behavior
- Now passes in both Debug and Release builds

## References

- [HashLink Issue #752](https://github.com/HaxeFoundation/hashlink/issues/752) - Stack registration discussion
- `vendor/hashlink/src/gc.c` - GC implementation
- `vendor/hashlink/src/allocator.c` - Memory allocator with debug assertions
