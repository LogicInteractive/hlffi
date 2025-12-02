# Struct Mismatch Bug Fix

**Date:** November 27, 2025
**Severity:** Critical (heap corruption)
**Status:** FIXED

## Symptom

When running `test_exceptions.exe`, a debug assertion failure occurred:

```
Debug Assertion Failed!
Program: test_exceptions.exe
File: minkernel\crts\ucrt\src\appcrt\heap\debug_heap.cpp
Line: 904
Expression: _CrtIsValidHeapPointer(block)
```

This indicates heap corruption detected by the MSVC debug CRT.

## Root Cause

The `struct hlffi_vm` was defined **differently** in different source files:

| File | Size | Extra Fields |
|------|------|--------------|
| `hlffi_lifecycle.c` | ~560 bytes | (base fields only) |
| `hlffi_values.c` | ~560 bytes | (base fields only) |
| `hlffi_types.c` | ~560 bytes | (base fields only) |
| `hlffi_objects.c` | ~560 bytes | (base fields only) |
| `hlffi_callbacks.c` | **~7KB** | `callbacks[64]`, `callback_count`, `exception_msg[512]`, `exception_stack[2048]` |

### The Problem

1. `hlffi_create()` in `hlffi_lifecycle.c` allocates memory using `calloc(1, sizeof(hlffi_vm))` with the **smaller** struct definition (~560 bytes)

2. `hlffi_try_call_static()` in `hlffi_callbacks.c` writes to `vm->exception_msg`, thinking it's at offset ~6KB

3. This writes **past the end** of the allocated buffer, corrupting the heap

4. When cleanup runs, the CRT debug heap detects the corruption

### Code Example

**Before (hlffi_lifecycle.c):**
```c
struct hlffi_vm {
    hl_module* module;
    hl_code* code;
    // ... ~560 bytes total
};

hlffi_vm* hlffi_create(void) {
    return calloc(1, sizeof(hlffi_vm));  // Allocates ~560 bytes
}
```

**Before (hlffi_callbacks.c):**
```c
struct hlffi_vm {
    hl_module* module;
    hl_code* code;
    // ... base fields ...
    callback_entry callbacks[64];  // +5KB
    int callback_count;
    char exception_msg[512];       // Writing here corrupts heap!
    char exception_stack[2048];
};

void store_exception(hlffi_vm* vm, ...) {
    strncpy(vm->exception_msg, ...);  // BOOM! Writing past allocated memory
}
```

## The Fix

Created a single shared header `src/hlffi_internal.h` containing the **complete** struct definition:

```c
// src/hlffi_internal.h
struct hlffi_vm {
    /* Base fields */
    hl_module* module;
    hl_code* code;
    hlffi_integration_mode integration_mode;
    void* stack_context;
    char error_msg[512];
    hlffi_error_code last_error;
    bool hl_initialized;
    bool thread_registered;
    bool module_loaded;
    bool entry_called;
    bool hot_reload_enabled;
    const char* loaded_file;

    /* Phase 6: Callback storage */
    hlffi_callback_entry callbacks[HLFFI_MAX_CALLBACKS];
    int callback_count;

    /* Phase 6: Exception storage */
    char exception_msg[512];
    char exception_stack[2048];
};
```

All source files now include this header instead of defining their own versions:

```c
// All .c files now use:
#include "hlffi_internal.h"
```

## Files Changed

- **Created:** `src/hlffi_internal.h` - Unified struct definitions
- **Modified:** `src/hlffi_lifecycle.c` - Use internal header
- **Modified:** `src/hlffi_values.c` - Use internal header
- **Modified:** `src/hlffi_types.c` - Use internal header
- **Modified:** `src/hlffi_objects.c` - Use internal header
- **Modified:** `src/hlffi_callbacks.c` - Use internal header

## Prevention

To prevent this bug from recurring:

1. **Never define structs in .c files** - Always use shared headers
2. **Single source of truth** - `hlffi_internal.h` is the only place `struct hlffi_vm` is defined
3. **Add new fields carefully** - When adding fields to VM struct, only modify `hlffi_internal.h`

## Test Results After Fix

All tests pass without heap assertions:

- ✅ `test_static.exe` - 10/10 tests
- ✅ `test_instance_basic.exe` - 10/10 tests
- ✅ `test_reflection.exe` - All tests
- ✅ `test_exceptions.exe` - 9/9 tests (no more heap corruption!)
