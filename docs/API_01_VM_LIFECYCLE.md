# HLFFI API Reference - VM Lifecycle

**[← Back to API Index](API_REFERENCE.md)**

This section covers the VM lifecycle functions - the core create/init/load/run/destroy pattern for embedding HashLink.

---

## Table of Contents

1. [Overview](#overview)
2. [Lifecycle Pattern](#lifecycle-pattern)
3. [Functions](#functions)
   - [`hlffi_create()`](#hlffi_create)
   - [`hlffi_init()`](#hlffi_init)
   - [`hlffi_load_file()`](#hlffi_load_file)
   - [`hlffi_load_memory()`](#hlffi_load_memory)
   - [`hlffi_call_entry()`](#hlffi_call_entry)
   - [`hlffi_close()`](#hlffi_close)
   - [`hlffi_destroy()`](#hlffi_destroy)
   - [`hlffi_get_error()`](#hlffi_get_error)
   - [`hlffi_get_error_string()`](#hlffi_get_error_string)
   - [`hlffi_update_stack_top()`](#hlffi_update_stack_top)
4. [VM Restart (Experimental)](#vm-restart-experimental)
5. [Best Practices](#best-practices)
6. [Common Pitfalls](#common-pitfalls)

---

## Overview

The VM lifecycle manages the HashLink virtual machine from creation to destruction. These functions must be called in the correct order:

**create → init → load → call_entry → [use VM] → close → destroy**

**Important Notes:**
- Only **ONE** VM active at a time (HashLink limitation)
- VM restart IS supported (experimental) - see [VM_RESTART.md](VM_RESTART.md)
- Entry point **must** be called before accessing static members
- All lifecycle functions are **NOT thread-safe** (call from main thread)

---

## Lifecycle Pattern

### Basic Pattern

```c
// 1. Create VM handle
hlffi_vm* vm = hlffi_create();
if (!vm)
{
    fprintf(stderr, "Failed to create VM\n");
    return -1;
}

// 2. Initialize HashLink runtime
hlffi_error_code err = hlffi_init(vm, 0, NULL);
if (err != HLFFI_OK)
{
    fprintf(stderr, "Init failed: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);
    return -1;
}

// 3. Load bytecode
err = hlffi_load_file(vm, "game.hl");
if (err != HLFFI_OK)
{
    fprintf(stderr, "Load failed: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);
    return -1;
}

// 4. Call entry point (initializes globals)
err = hlffi_call_entry(vm);
if (err != HLFFI_OK)
{
    fprintf(stderr, "Entry call failed: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);
    return -1;
}

// 5. Use the VM (call functions, access data)
hlffi_call_static(vm, "Game", "start", 0, NULL);

// 6. Close VM (cleanup before destroy)
hlffi_close(vm);

// 7. Destroy VM
hlffi_destroy(vm);
```

### With Arguments

```c
// Pass command-line arguments to Haxe:
hlffi_error_code err = hlffi_init(vm, argc, argv);
```

### Error Handling Pattern

```c
hlffi_error_code err = hlffi_load_file(vm, "game.hl");
if (err != HLFFI_OK)
{
    // Get error code name:
    fprintf(stderr, "Error %d: %s\n", err, hlffi_get_error_string(err));

    // Get detailed message:
    fprintf(stderr, "Details: %s\n", hlffi_get_error(vm));
}
```

---

## Functions

### `hlffi_create()`

**Signature:**
```c
hlffi_vm* hlffi_create(void)
```

**Description:**
Allocates a new VM instance structure. Does not initialize the HashLink runtime yet.

**Returns:**
- `hlffi_vm*` - VM instance handle
- `NULL` - Allocation failure

**Thread Safety:** ❌ Not thread-safe

**Memory:** Caller must call `hlffi_destroy()` to free (at process exit only)

**Example:**
```c
hlffi_vm* vm = hlffi_create();
if (!vm)
{
    fprintf(stderr, "Failed to create VM\n");
    return -1;
}
```

**Notes:**
- Only allocates the VM structure
- Does not touch HashLink runtime yet
- Safe to call `hlffi_destroy()` if init fails

**See Also:** [`hlffi_destroy()`](#hlffi_destroy)

---

### `hlffi_init()`

**Signature:**
```c
hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv)
```

**Description:**
Initializes the HashLink runtime. Sets up the garbage collector, registers the main thread, and prepares for bytecode loading. HashLink global state is initialized only once per process (subsequent calls skip already-done initializations).

**Parameters:**
- `vm` - VM instance from `hlffi_create()`
- `argc` - Argument count (pass 0 if no args)
- `argv` - Argument vector (pass NULL if no args)

**Returns:**
- `HLFFI_OK` - Initialization successful
- `HLFFI_ERROR_NULL_VM` - vm is NULL
- `HLFFI_ERROR_ALREADY_INITIALIZED` - Already called
- `HLFFI_ERROR_INIT_FAILED` - HashLink initialization failed

**Thread Safety:** ❌ Must be called from main thread

**Example:**
```c
// Without arguments:
hlffi_error_code err = hlffi_init(vm, 0, NULL);
if (err != HLFFI_OK)
{
    fprintf(stderr, "Init failed: %s\n", hlffi_get_error(vm));
    return -1;
}

// With command-line arguments:
int main(int argc, char** argv)
{
    hlffi_vm* vm = hlffi_create();
    hlffi_error_code err = hlffi_init(vm, argc, argv);
    // Haxe code can now access Sys.args()
}
```

**Notes:**
- HashLink global state (`hl_global_init()`, `hl_register_thread()`) is initialized only once
- Subsequent VM creations reuse the existing global state (enables VM restart)
- Registers main thread with GC
- Sets up memory allocator

**See Also:** [`hlffi_create()`](#hlffi_create), [`hlffi_load_file()`](#hlffi_load_file)

---

### `hlffi_load_file()`

**Signature:**
```c
hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path)
```

**Description:**
Loads HashLink bytecode from a `.hl` file on disk.

**Parameters:**
- `vm` - Initialized VM instance
- `path` - Path to `.hl` file (absolute or relative)

**Returns:**
- `HLFFI_OK` - File loaded successfully
- `HLFFI_ERROR_NULL_VM` - vm is NULL
- `HLFFI_ERROR_NOT_INITIALIZED` - VM not initialized
- `HLFFI_ERROR_FILE_NOT_FOUND` - File doesn't exist
- `HLFFI_ERROR_INVALID_BYTECODE` - Not valid HashLink bytecode
- `HLFFI_ERROR_MODULE_LOAD_FAILED` - Load failed

**Thread Safety:** ❌ Not thread-safe

**Precondition:** Must call `hlffi_init()` first

**Example:**
```c
hlffi_error_code err = hlffi_load_file(vm, "game.hl");
if (err != HLFFI_OK)
{
    fprintf(stderr, "Load failed: %s\n", hlffi_get_error(vm));
    return -1;
}

// Relative path:
err = hlffi_load_file(vm, "../assets/game.hl");

// Absolute path:
err = hlffi_load_file(vm, "/opt/game/game.hl");
```

**Notes:**
- Bytecode must match HashLink version (recompile if needed)
- File can be in any location accessible to the process
- For embedded bytecode, use `hlffi_load_memory()`
- Loads entire file into memory

**Common Errors:**
- `HLFFI_ERROR_FILE_NOT_FOUND` - Check file path, permissions
- `HLFFI_ERROR_INVALID_BYTECODE` - Recompile with matching HL version
- `HLFFI_ERROR_MODULE_LOAD_FAILED` - Check file is valid .hl format

**See Also:** [`hlffi_load_memory()`](#hlffi_load_memory), [`hlffi_call_entry()`](#hlffi_call_entry)

---

### `hlffi_load_memory()`

**Signature:**
```c
hlffi_error_code hlffi_load_memory(hlffi_vm* vm, const void* data, size_t size)
```

**Description:**
Loads HashLink bytecode from a memory buffer. Useful for embedded resources or encrypted assets.

**Parameters:**
- `vm` - Initialized VM instance
- `data` - Pointer to bytecode data
- `size` - Size of bytecode in bytes

**Returns:**
- `HLFFI_OK` - Bytecode loaded successfully
- `HLFFI_ERROR_NULL_VM` - vm is NULL
- `HLFFI_ERROR_NOT_INITIALIZED` - VM not initialized
- `HLFFI_ERROR_INVALID_BYTECODE` - Invalid data
- `HLFFI_ERROR_MODULE_LOAD_FAILED` - Load failed

**Thread Safety:** ❌ Not thread-safe

**Memory:** Buffer can be freed after this function returns

**Example:**
```c
// Embedded bytecode (e.g., from xxd -i game.hl):
unsigned char game_hl[] =
{
    0x48, 0x4c, 0x42, 0x03, // HLB header
    // ... rest of bytecode ...
};
unsigned int game_hl_len = sizeof(game_hl);

hlffi_error_code err = hlffi_load_memory(vm, game_hl, game_hl_len);
if (err != HLFFI_OK)
{
    fprintf(stderr, "Load from memory failed: %s\n", hlffi_get_error(vm));
    return -1;
}

// From dynamically allocated buffer:
void* bytecode = load_encrypted_asset("game.hl.enc");
size_t size = get_asset_size("game.hl.enc");
err = hlffi_load_memory(vm, bytecode, size);
free(bytecode);  // Safe to free after loading
```

**Use Cases:**
- Embedding bytecode in executable (via linker or resource compiler)
- Loading encrypted/compressed bytecode
- Loading from custom asset system
- Avoiding file I/O on restricted platforms

**Notes:**
- HashLink makes internal copy of the data
- Safe to free buffer immediately after calling
- Data must be valid HashLink bytecode (starts with "HLB" signature)

**See Also:** [`hlffi_load_file()`](#hlffi_load_file)

---

### `hlffi_call_entry()`

**Signature:**
```c
hlffi_error_code hlffi_call_entry(hlffi_vm* vm)
```

**Description:**
Calls the entry point (Haxe `main()` function). **This MUST be called even if main() is empty**, as it initializes static globals.

**Parameters:**
- `vm` - VM instance with loaded bytecode

**Returns:**
- `HLFFI_OK` - Entry point executed successfully
- `HLFFI_ERROR_NULL_VM` - vm is NULL
- `HLFFI_ERROR_NOT_INITIALIZED` - VM not initialized
- `HLFFI_ERROR_ENTRY_POINT_NOT_FOUND` - No main() function

**Thread Safety:** ❌ Not thread-safe (or use THREADED mode)

**Blocking:** ⚠️ **Blocks if Haxe has while loop!** Use THREADED mode for blocking code

**Example (Non-Threaded):**
```c
// Haxe: function main() { trace("Hello"); }
hlffi_error_code err = hlffi_call_entry(vm);
if (err != HLFFI_OK)
{
    fprintf(stderr, "Entry call failed: %s\n", hlffi_get_error(vm));
}
// Returns immediately if Haxe main() returns
```

**Example (Threaded Mode for Blocking Code):**
```c
// Haxe: function main() { while(true) { /* blocking loop */ } }
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
hlffi_thread_start(vm);  // Calls entry point in separate thread
// Main thread continues immediately
```

**Critical Notes:**
- Must be called before accessing static members
- In NON_THREADED mode: Returns when Haxe main() returns
- In THREADED mode: Called automatically by `hlffi_thread_start()`
- Initializes static globals (required even for empty main())
- If main() has infinite loop, use THREADED mode

**What It Does:**
1. Resolves entry point symbol
2. Calls Haxe `main()` function
3. Initializes static class variables
4. Returns when main() completes (or blocks if infinite loop)

**See Also:** `hlffi_set_integration_mode()`, `hlffi_thread_start()`, [Integration Modes](API_02_INTEGRATION_MODES.md)

---

### `hlffi_close()`

**Signature:**
```c
void hlffi_close(hlffi_vm* vm)
```

**Description:**
Cleans up the VM state before destruction. Releases module resources and prepares for destroy.

**Parameters:**
- `vm` - VM instance to close

**Thread Safety:** ❌ Not thread-safe

**Example:**
```c
// Before destroying VM:
hlffi_close(vm);
hlffi_destroy(vm);
vm = NULL;
```

**Notes:**
- Should be called before `hlffi_destroy()`
- Safe to call even if init/load failed
- Clears the `init` and `ready` flags on the VM

**See Also:** [`hlffi_destroy()`](#hlffi_destroy)

---

### `hlffi_destroy()`

**Signature:**
```c
void hlffi_destroy(hlffi_vm* vm)
```

**Description:**
Destroys the VM instance and frees its memory. Call `hlffi_close()` first.

**Parameters:**
- `vm` - VM instance to destroy

**Thread Safety:** ❌ Not thread-safe

**Example:**
```c
// Proper shutdown:
hlffi_close(vm);
hlffi_destroy(vm);
vm = NULL;

// After destroy, you CAN create a new VM (experimental):
vm = hlffi_create();
hlffi_init(vm, 0, NULL);
// ... load and use new VM
```

**Notes:**
- Call `hlffi_close()` before this function
- Safe to call even if init/load failed
- **VM restart IS supported** - you can create a new VM after destroy (experimental)
- For hot reload (no restart needed), see [Hot Reload](API_05_HOT_RELOAD.md)
- For VM restart patterns, see [VM_RESTART.md](VM_RESTART.md)

**See Also:** [`hlffi_close()`](#hlffi_close), [`hlffi_create()`](#hlffi_create), [VM Restart](#vm-restart-experimental)

---

### `hlffi_get_error()`

**Signature:**
```c
const char* hlffi_get_error(hlffi_vm* vm)
```

**Description:**
Returns a human-readable error message for the last error that occurred on this VM.

**Parameters:**
- `vm` - VM instance

**Returns:**
- `const char*` - Error message string (valid until next error)
- Empty string if no error

**Thread Safety:** ⚠️ Error message is per-VM, not thread-safe

**Memory:** Do not free the returned string (static buffer)

**Example:**
```c
if (hlffi_load_file(vm, "game.hl") != HLFFI_OK)
{
    fprintf(stderr, "Error: %s\n", hlffi_get_error(vm));
}

// Check if there's an error:
const char* err = hlffi_get_error(vm);
if (err && *err)
{
    printf("Last error: %s\n", err);
}
```

**Notes:**
- Error message is cleared on successful operations
- Message is stored in VM structure
- Not thread-safe if multiple threads access same VM
- Use `hlffi_get_error_string()` for error code to string conversion

**See Also:** [`hlffi_get_error_string()`](#hlffi_get_error_string), `hlffi_error_code`

---

### `hlffi_get_error_string()`

**Signature:**
```c
const char* hlffi_get_error_string(hlffi_error_code code)
```

**Description:**
Converts an error code to a human-readable string.

**Parameters:**
- `code` - Error code enum value

**Returns:**
- `const char*` - Error message string (static)

**Thread Safety:** ✅ Thread-safe (static strings)

**Example:**
```c
hlffi_error_code err = hlffi_load_file(vm, "game.hl");
if (err != HLFFI_OK)
{
    fprintf(stderr, "Error %d: %s\n", err, hlffi_get_error_string(err));
    fprintf(stderr, "Details: %s\n", hlffi_get_error(vm));
}

// Print all possible errors:
for (int i = HLFFI_OK; i <= HLFFI_ERROR_UNKNOWN; i++)
{
    printf("%d: %s\n", i, hlffi_get_error_string(i));
}
```

**Notes:**
- Returns generic message for error code
- Use `hlffi_get_error()` for context-specific details
- Safe to use from any thread
- Useful for logging/debugging

**See Also:** [`hlffi_get_error()`](#hlffi_get_error), [Error Handling](API_19_ERROR_HANDLING.md)

---

### `hlffi_update_stack_top()`

**Signature:**
```c
void hlffi_update_stack_top(void* stack_marker)
```

**Description:**
Updates the GC stack top pointer to the current stack position. **Typically not needed** - HLFFI handles this internally.

**Parameters:**
- `stack_marker` - Address of a local variable on the stack

**Thread Safety:** ✅ Thread-safe (per-thread GC state)

**Example:**
```c
void my_game_loop(hlffi_vm* vm)
{
    int stack_marker;  // Must be local variable
    hlffi_update_stack_top(&stack_marker);

    // Now safe to call HLFFI functions in loop
    while (running)
    {
        hlffi_update(vm, delta_time);
    }
}

// Macro version:
void my_game_loop(hlffi_vm* vm)
{
    HLFFI_ENTER_SCOPE();  // Creates stack_marker and calls update_stack_top

    while (running)
    {
        hlffi_update(vm, delta_time);
    }
}
```

**When to Use:**
- Only if you encounter GC-related crashes
- Complex threading scenarios
- Calling HashLink directly without HLFFI wrappers
- Deep call stacks in game loops

**When NOT to Use:**
- Normal HLFFI usage (already handled internally)
- Simple applications
- When using HLFFI wrapper functions (they handle it)

**Background:**
HashLink's GC scans the C call stack to find object references. When embedding HashLink, the initial `stack_top` may point to heap memory instead of the actual stack, causing crashes. HLFFI handles this automatically in most cases, but this function is provided as a fallback.

**How It Works:**
1. Takes address of local variable (guaranteed to be on stack)
2. Updates GC's `stack_top` to this address
3. GC now scans from correct stack location
4. Prevents false positives/negatives in GC marking

**See Also:** `HLFFI_ENTER_SCOPE()` macro, [GC_STACK_SCANNING.md](GC_STACK_SCANNING.md)

---

## VM Restart (Experimental)

HLFFI supports restarting the HashLink VM within a single process. This is **experimental** but tested and working.

### How It Works

HashLink has two process-wide initializations:
1. `hl_global_init()` - Initializes HashLink global state (GC, type system)
2. `hl_register_thread()` - Registers main thread with GC

HLFFI uses static flags to ensure these are called only once per process. When you create a new VM after destroying one, it reuses the existing global state.

### Basic Restart Pattern

```c
for (int session = 0; session < 3; session++)
{
    // Create and use VM
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Use the VM...
    hlffi_call_static(vm, "Game", "update", 0, NULL);

    // Cleanup
    hlffi_close(vm);
    hlffi_destroy(vm);

    // Brief pause before restart (recommended)
    sleep(1);
}
```

### Threaded Mode Restart

```c
for (int session = 0; session < 3; session++)
{
    hlffi_vm* vm = hlffi_create();
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_thread_start(vm);

    // Use via sync calls...
    hlffi_thread_call_sync(vm, my_callback, data);

    hlffi_thread_stop(vm);
    hlffi_close(vm);
    hlffi_destroy(vm);
    sleep(1);
}
```

### Limitations

1. **Experimental** - Not officially supported by HashLink
2. **Memory accumulation** - Some internal state may not be fully released
3. **Single VM at a time** - Don't create a second VM before destroying the first
4. **Static variables reset** - Haxe statics reset on each `hlffi_load_file()`

### When to Use

| Scenario | Recommended Approach |
|----------|---------------------|
| Code iteration during development | Hot Reload |
| Full state reset | VM Restart |
| Level/scene reload | VM Restart or Hot Reload |
| Long-running server refresh | VM Restart |

For complete details, see [VM_RESTART.md](VM_RESTART.md).

---

## Best Practices

### 1. Always Check Return Codes

```c
// ✅ GOOD
hlffi_error_code err = hlffi_init(vm, 0, NULL);
if (err != HLFFI_OK)
{
    fprintf(stderr, "Init failed: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);
    return -1;
}

// ❌ BAD
hlffi_init(vm, 0, NULL);  // Ignoring error
```

### 2. Call Entry Point Before Using VM

```c
// ✅ GOOD
hlffi_call_entry(vm);  // Initialize globals
int score = hlffi_get_static_int(vm, "Game", "score", 0);  // Now safe

// ❌ BAD
int score = hlffi_get_static_int(vm, "Game", "score", 0);  // Globals not initialized!
```

### 3. Use Hot Reload for Code Updates

```c
// ✅ PREFERRED - Hot reload for code updates (preserves state)
hlffi_enable_hot_reload(vm, true);
// ... later ...
hlffi_reload_module(vm, "game.hl");

// ✅ ALSO WORKS - VM restart (experimental, resets state)
hlffi_close(vm);
hlffi_destroy(vm);
sleep(1);  // Brief pause recommended
vm = hlffi_create();
hlffi_init(vm, 0, NULL);
hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);
```

### 4. Close Before Destroy

```c
// ✅ GOOD
hlffi_close(vm);
hlffi_destroy(vm);

// ❌ BAD - Missing close
hlffi_destroy(vm);  // May not clean up properly
```

### 5. Handle Threading Properly

```c
// ✅ GOOD - Threaded mode for blocking code
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
hlffi_thread_start(vm);  // Calls entry in thread

// ❌ BAD - Non-threaded mode with blocking code
hlffi_call_entry(vm);  // BLOCKS FOREVER if main() has while(true)
```

---

## Common Pitfalls

### 1. Forgetting to Call Entry Point

**Problem:**
```c
hlffi_load_file(vm, "game.hl");
// Missing: hlffi_call_entry(vm);
int score = hlffi_get_static_int(vm, "Game", "score", 0);  // CRASH!
```

**Symptoms:** Crashes, null pointers, uninitialized static fields

**Solution:** Always call `hlffi_call_entry()` after loading

---

### 2. Forgetting to Close Before Destroy

**Problem:**
```c
hlffi_destroy(vm);  // Missing hlffi_close()!
vm = hlffi_create();
hlffi_init(vm, 0, NULL);
```

**Symptoms:** Potential memory leaks or undefined behavior

**Solution:** Always call `hlffi_close()` before `hlffi_destroy()`:
```c
hlffi_close(vm);
hlffi_destroy(vm);
```

**Note:** VM restart IS supported (experimental). See [VM_RESTART.md](VM_RESTART.md)

---

### 3. Blocking in Non-Threaded Mode

**Problem:**
```c
// Haxe: while(true) { /* game loop */ }
hlffi_call_entry(vm);  // BLOCKS FOREVER!
```

**Symptoms:** Program hangs, unresponsive

**Solution:** Use THREADED mode for blocking Haxe code

---

### 4. Wrong Bytecode Version

**Problem:**
```c
// .hl compiled with HL 1.11, running with HL 1.12
hlffi_load_file(vm, "old.hl");  // ERROR!
```

**Symptoms:** `HLFFI_ERROR_INVALID_BYTECODE`, crashes

**Solution:** Recompile Haxe code with matching HashLink version

---

### 5. Ignoring Error Messages

**Problem:**
```c
if (hlffi_load_file(vm, "game.hl") != HLFFI_OK)
{
    printf("Failed!\n");  // No details!
}
```

**Symptoms:** Hard to debug failures

**Solution:** Always print error messages:
```c
if (hlffi_load_file(vm, "game.hl") != HLFFI_OK)
{
    fprintf(stderr, "Load failed: %s\n", hlffi_get_error(vm));
}
```

---

**[← Back to API Index](API_REFERENCE.md)** | **[Next: Integration Modes →](API_02_INTEGRATION_MODES.md)**
