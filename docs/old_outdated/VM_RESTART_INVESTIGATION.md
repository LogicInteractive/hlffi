# HashLink VM Restart Investigation Report

**Date**: 2025-11-22
**Critical Question**: Can HashLink VM be stopped and restarted in a C/C++ application?
**Answer**: ❌ **NO** - VM restart is NOT officially supported and has fundamental technical limitations

---

## Executive Summary

After thorough investigation of HashLink source code, GitHub issues, and community discussions, **VM restart (calling `hl_global_init()` after `hl_global_free()`) is NOT safe and NOT supported**. The implementation has multiple critical issues that prevent reliable reinitialization:

1. **No idempotent initialization** - `hl_gc_init()` lacks protective flags
2. **Incomplete cleanup** - `hl_gc_free()` doesn't fully reset static state
3. **Module system state leaks** - Static arrays never cleared
4. **Mutex reallocation leaks** - Calling init twice leaks memory

**Recommended Alternatives**:
- **Hot Reload** (official, preferred for development)
- **Plugin System** (runtime module loading, Nov 2025)
- **Process restart** (full isolation, most reliable)

---

## 1. Technical Evidence: Why Restart Fails

### 1.1 Core Issue: Non-Idempotent Initialization

**File**: `src/gc.c` ([GitHub source](https://github.com/HaxeFoundation/hashlink/blob/master/src/gc.c))

```c
// hl_global_init() calls hl_gc_init() which has NO protective flags:
static void hl_gc_init() {
    int i;
    for(i=0;i<1<<GC_LEVEL0_BITS;i++)
        hl_gc_page_map[i] = gc_level1_null;  // ⚠️ Resets page map
    gc_allocator_init();

    memset(&gc_threads,0,sizeof(gc_threads));  // ⚠️ Zeros thread state
    gc_threads.global_lock = hl_mutex_alloc(false);  // ⚠️ LEAK if called twice!
    gc_threads.exclusive_lock = hl_mutex_alloc(false);  // ⚠️ LEAK!

#ifdef HL_THREADS
    hl_add_root(&gc_threads.global_lock);
    hl_add_root(&gc_threads.exclusive_lock);
    mark_threads_done = hl_semaphore_alloc(0);  // ⚠️ LEAK!
#endif
}
```

**Problems if called twice:**
1. **Mutex leak** - Previous `global_lock` and `exclusive_lock` mutexes leaked (not freed before reallocating)
2. **Page map corruption** - Resets `hl_gc_page_map` while old allocations may still reference it
3. **Thread state corruption** - Zeros `gc_threads` structure, breaking active thread registrations
4. **Semaphore leak** - `mark_threads_done` reallocated without freeing previous instance

### 1.2 Incomplete Cleanup

```c
// hl_gc_free() is INCOMPLETE - doesn't cleanup everything:
static void hl_gc_free() {
#ifdef HL_THREADS
    hl_remove_root(&gc_threads.global_lock);  // Removes GC root only
    // ⚠️ MISSING: hl_mutex_free(gc_threads.global_lock)
    // ⚠️ MISSING: hl_mutex_free(gc_threads.exclusive_lock)
    // ⚠️ MISSING: hl_semaphore_free(mark_threads_done)
    // ⚠️ MISSING: Reset gc_threads to NULL/zero state
    // ⚠️ MISSING: Clear hl_gc_page_map
#endif
}
```

**Missing cleanup operations:**
- Mutexes not freed (`hl_mutex_free()` never called)
- Semaphores not freed
- Thread state not reset
- Page maps not cleared
- No reinitialization flag reset

**Result**: Calling `hl_global_init()` after `hl_global_free()` will allocate new resources without freeing old ones.

### 1.3 Module System State Leaks

**File**: `src/module.c` ([GitHub source](https://github.com/HaxeFoundation/hashlink/blob/master/src/module.c))

```c
// Static module tracking - NEVER CLEARED:
static hl_module **cur_modules = NULL;
static int modules_count = 0;

// hl_module_add() appends modules but never resets the array:
void hl_module_add(hl_module *m) {
    cur_modules = (hl_module**)realloc(cur_modules, sizeof(hl_module*) * (modules_count + 1));
    cur_modules[modules_count++] = m;
    // ⚠️ No mechanism to clear this array on restart
}
```

**Problem**: After `hl_global_free()`, `cur_modules` array still contains pointers to freed modules. On restart:
- Stale pointers remain in `cur_modules`
- `hl_module_resolve_symbol_full()` iterates over freed memory
- Potential segfaults or incorrect symbol resolution

**Missing**: A `hl_module_reset()` function to clear static state.

---

## 2. Historical Issues & Bug Reports

### 2.1 Issue #207: Unity Editor Crashes on Relaunch

**Link**: [GitHub Issue #207](https://github.com/HaxeFoundation/hashlink/issues/207) - "Are there any c api to stop runtime when be embed?"

**Problem Reported**:
> "When embedding libhl into Unity editor, everything runs okay at first time but when relaunched, the Unity editor crashes."
> "When I call hl_global_free(), unity crash..."

**Root Cause**: Mutex NULL pointer dereference in cleanup code.

**Fix Applied**: Added NULL checks in `hl_cache_free()` and `gc_global_lock()`:
```c
// Before fix - CRASH:
hl_mutex_free(hl_cache_lock);  // ⚠️ Crash if already freed

// After fix - SAFE:
if (hl_cache_lock != NULL) {  // ✅ NULL check added
    hl_mutex_free(hl_cache_lock);
}
```

**Lesson**: Even with bug fixes, restart was NOT the intended use case. The fix only prevented crashes, not enabled restart.

---

## 3. Official Solutions: Hot Reload & Plugins

HashLink provides two official alternatives to VM restart:

### 3.1 Hot Reload (Preferred for Development)

**Link**: [Hot Reload Wiki](https://github.com/HaxeFoundation/hashlink/wiki/Hot-Reload)

**What it does**: Runtime function patching without VM restart
- Detects changed functions in bytecode
- JIT-compiles new versions
- Patches all call sites to new implementations
- **Preserves runtime state** (objects, globals, etc.)

**How to use**:
```haxe
// Haxe code - check for changes in main loop:
function main() {
    while (true) {
        hl.Api.checkReload();  // Checks bytecode for changes
        update();
        render();
    }
}
```

```bash
# Enable hot reload mode:
hl --hot-reload game.hl

# Or in VSCode launch.json:
{
    "hotReload": true
}
```

**Limitations** (from wiki):
- ⚠️ **Cannot change class fields** (add/remove/reorder) - existing object memory layouts can't be remapped
- ⚠️ **New types don't auto-initialize** - static variables of new classes stay uninitialized
- ⚠️ **Experimental** - may have bugs, requires "tiny 2-3 lines" reproducible cases for debugging
- ⚠️ **JIT mode only** - doesn't work with HL/C compilation

**Verdict**: ✅ Works for function body changes, ❌ Not a full restart replacement

### 3.2 Plugin System (Runtime Module Loading)

**Link**: [GitHub Issue #179](https://github.com/HaxeFoundation/hashlink/issues/179) - "Possibility to load and execute hl bytecode in runtime"

**Status**: ✅ **Implemented November 2025** (bleeding-edge)

**What it does**: Load additional `.hl` modules at runtime
- Multiple JIT modules coexist in same VM
- Type sharing via `@:virtual` annotation
- Plugins can call core code and vice versa

**How to use**:
```haxe
// Haxe API (requires GIT Haxe and HL):
hl.Api.loadPlugin('mods/usermod.hl');

// Types shared via @:virtual:
@:virtual class Mod {
    public function onInit():Void;
}
```

**Limitations**:
- ⚠️ **Experimental** - API may change
- ⚠️ **Type compatibility challenges** - identical classes in different modules treated as different types
- ⚠️ **JIT mode only** - not available in HL/C
- ⚠️ **Requires bleeding-edge HL** (Nov 2025+)

**Verdict**: ✅ Adds modules dynamically, ❌ Doesn't restart VM or replace existing code

---

## 4. What Actually Happens When You Try to Restart

Based on source code analysis, here's the sequence of failures:

### Scenario: hlffi_close() then hlffi_create()

```c
// Attempt 1: Normal startup
hlffi_vm* vm = hlffi_create();
// → Calls hl_global_init()
// → Allocates mutexes: global_lock, exclusive_lock
// → Initializes page maps
// → Sets up GC roots

hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);
// → Modules added to cur_modules array

// Attempt 2: Cleanup and restart
hlffi_close(vm);
// → Calls hl_global_free()
// → Removes GC roots for mutexes
// → ⚠️ Mutexes NOT freed (leak)
// → ⚠️ Page maps NOT cleared
// → ⚠️ cur_modules array NOT cleared

hlffi_destroy(vm);
vm = hlffi_create();  // ⚠️ RESTART ATTEMPT
// → Calls hl_global_init() again
// → Reallocates NEW mutexes (old ones leaked)
// → Resets page map (but old pointers may exist)
// → Zeros gc_threads (breaks thread registrations)

hlffi_load_file(vm, "game2.hl");
// → Adds to cur_modules array (still has stale pointers from first run)
// → ⚠️ Symbol resolution may find old modules
// → ⚠️ Undefined behavior

hlffi_call_entry(vm);  // ⚠️ CRASH or undefined behavior likely
```

### Specific Failure Modes

1. **Memory Leaks**:
   - Each restart leaks: 2 mutexes + 1 semaphore (if threaded)
   - Repeated restarts → OOM (out of memory)

2. **Stale Pointers**:
   - `cur_modules` contains pointers to freed modules
   - Symbol resolution accesses freed memory → segfault

3. **Thread State Corruption**:
   - `memset(&gc_threads, 0, ...)` clears registered thread info
   - GC may scan invalid stack pointers → crash

4. **Mutex Double-Free** (if manually added):
   - If user calls `hl_mutex_free()` manually, then restart → crash on double-free
   - If user doesn't call it → leak

---

## 5. Evidence from Current hlffi.h Implementation

Your current library (`hlffi.h` v2.2.2) **attempts** restart:

```c
hlffi_vm* hlffi_create(void) {
    hlffi_vm* vm = (hlffi_vm*)calloc(1, sizeof(hlffi_vm));
    if (!vm) return NULL;
    hl_global_init();  // ⚠️ Not safe if called twice!
    hl_register_thread(NULL);
    vm->init = true;
    return vm;
}

void hlffi_close(hlffi_vm* vm) {
    if (!vm || !vm->init) return;
    hl_global_free();  // ⚠️ Incomplete cleanup
    vm->init = vm->ready = false;
}
```

**Why it "always failed"**:
- Creating second VM calls `hl_global_init()` twice → mutex leak
- `hl_global_free()` doesn't fully clean up → stale state
- Module array leaks across VMs → symbol resolution fails

**User's experience matches theory**: Restart doesn't work because it's fundamentally unsupported.

---

## 6. Recommended Approaches

### ✅ Option 1: Hot Reload (Best for Development)

**Use when**: Iterating on code during development

**Pros**:
- ✅ Official HashLink feature
- ✅ Fast (no VM restart overhead)
- ✅ Preserves runtime state
- ✅ Works in HL/JIT mode

**Cons**:
- ❌ Can't change class fields
- ❌ Experimental (may have bugs)
- ❌ Requires main loop integration

**Implementation**:
```c
// C wrapper for hot reload:
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable);
hlffi_error_code hlffi_check_reload(hlffi_vm* vm);  // Call in main loop

// Haxe code:
while (true) {
    hl.Api.checkReload();
    update();
}
```

### ✅ Option 2: Plugin System (Best for Mods/Extensions)

**Use when**: Loading user content or dynamic extensions

**Pros**:
- ✅ Official HashLink feature (Nov 2025)
- ✅ Multiple modules coexist
- ✅ Type sharing via @:virtual

**Cons**:
- ❌ Bleeding-edge (requires latest HL)
- ❌ Experimental API
- ❌ Type compatibility challenges

**Implementation**:
```c
// C wrapper for plugins:
hlffi_plugin* hlffi_load_plugin(hlffi_vm* vm, const char* path);
hlffi_error_code hlffi_unload_plugin(hlffi_plugin* plugin);
```

### ✅ Option 3: Process Restart (Most Reliable)

**Use when**: Need full isolation or production deployments

**Pros**:
- ✅ Complete cleanup (OS handles it)
- ✅ No static state issues
- ✅ Works with HL/C and HL/JIT
- ✅ Most predictable

**Cons**:
- ❌ Slower (full process restart)
- ❌ Loses all runtime state
- ❌ Requires IPC for communication

**Implementation**:
```c
// Parent process:
while (true) {
    pid_t child = fork();  // Unix
    if (child == 0) {
        // Child: run VM
        hlffi_vm* vm = hlffi_create();
        hlffi_load_file(vm, "game.hl");
        hlffi_call_entry(vm);
        exit(0);
    }
    waitpid(child, &status, 0);
    // Relaunch on crash/request
}
```

### ❌ Option 4: VM Restart (NOT RECOMMENDED)

**Current status**: Fundamentally broken, not supported

**To make it work** (theoretical, would require HashLink core changes):
1. Add initialization guard flag to `hl_gc_init()`:
   ```c
   static bool gc_initialized = false;
   if (gc_initialized) return;  // Prevent double-init
   gc_initialized = true;
   ```

2. Complete `hl_gc_free()` cleanup:
   ```c
   if (gc_threads.global_lock) {
       hl_mutex_free(gc_threads.global_lock);
       gc_threads.global_lock = NULL;
   }
   if (gc_threads.exclusive_lock) {
       hl_mutex_free(gc_threads.exclusive_lock);
       gc_threads.exclusive_lock = NULL;
   }
   if (mark_threads_done) {
       hl_semaphore_free(mark_threads_done);
       mark_threads_done = NULL;
   }
   gc_initialized = false;  // Allow re-init
   ```

3. Add module system cleanup:
   ```c
   void hl_module_reset() {
       free(cur_modules);
       cur_modules = NULL;
       modules_count = 0;
   }
   ```

4. Reset all static caches and state

**Effort**: Major HashLink core changes, likely weeks of work + extensive testing

**Verdict**: Not worth it when Hot Reload and Plugins exist.

---

## 7. Updated HLFFI v3.0 Master Plan Recommendations

### Remove Restart from Phase 1

**Current plan**:
```c
hlffi_error_code hlffi_shutdown(hlffi_vm* vm);  // ❌ Remove
hlffi_error_code hlffi_restart(hlffi_vm* vm);   // ❌ Remove
```

**Reason**: Misleading API - implies restart works when it doesn't.

**Replacement**: Document limitations clearly:
```c
// VM lifecycle - restart NOT supported
hlffi_vm* hlffi_create(void);           // ✅ Init VM once per process
void hlffi_destroy(hlffi_vm* vm);       // ✅ Cleanup (process exit only)

// For code changes, use hot reload instead:
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable);
hlffi_error_code hlffi_check_reload(hlffi_vm* vm);
```

### Update Phase 1: Lifecycle + Hot Reload

**Focus**:
1. ✅ Single VM lifecycle (init once, destroy on exit)
2. ✅ Hot reload for development iteration
3. ✅ Clear documentation of limitations
4. ❌ Remove restart/shutdown (not supported)

**New API**:
```c
// Single VM lifecycle
hlffi_vm* hlffi_create(void);
hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv);
hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path);
hlffi_error_code hlffi_call_entry(hlffi_vm* vm);
void hlffi_destroy(hlffi_vm* vm);  // Only safe at process exit

// Hot reload (alternative to restart)
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable);
hlffi_error_code hlffi_check_reload(hlffi_vm* vm);
void hlffi_set_reload_callback(hlffi_vm* vm, hlffi_reload_callback cb, void* userdata);
```

### Add Phase 9: Plugin System (Experimental)

Already planned, no changes needed. Plugin loading is the correct approach for runtime module addition.

---

## 8. Final Verdict

### Can HashLink VM be stopped and restarted?

**Answer**: ❌ **NO**

**Reasons**:
1. `hl_global_init()` is not idempotent (leaks mutexes/semaphores on second call)
2. `hl_global_free()` doesn't fully reset static state
3. Module system leaks `cur_modules` array across restarts
4. No reinitialization guards or cleanup mechanisms
5. Not an intended use case (Hot Reload is the official solution)

**Evidence**:
- ✅ Source code analysis (`gc.c`, `module.c`)
- ✅ GitHub Issue #207 (crashes on restart attempt)
- ✅ Official documentation recommends Hot Reload
- ✅ No restart examples in HashLink repo
- ✅ Community silence on restart (unsupported feature)

### What Should You Do Instead?

**For Development** (code iteration):
- ✅ Use **Hot Reload** - official, fast, preserves state
- Call `hl.Api.checkReload()` in main loop
- Enable with `hl --hot-reload`

**For Production** (mods/plugins):
- ✅ Use **Plugin System** (Nov 2025+) - runtime module loading
- Load with `hl.Api.loadPlugin()`
- Share types via `@:virtual`

**For Complete Isolation**:
- ✅ Use **Process Restart** - full OS-level cleanup
- Fork/exec new process, communicate via IPC

**Never** try to restart the VM by calling `hl_global_init()` after `hl_global_free()` - it will leak memory, corrupt state, and likely crash.

---

## 9. Sources & References

**GitHub Issues**:
- [Issue #207](https://github.com/HaxeFoundation/hashlink/issues/207) - Embedding crashes on relaunch
- [Issue #179](https://github.com/HaxeFoundation/hashlink/issues/179) - Runtime bytecode loading (Plugin System)

**Source Code**:
- [src/gc.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/gc.c) - `hl_global_init()` / `hl_global_free()`
- [src/module.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/module.c) - Module tracking (`cur_modules` leak)
- [src/main.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/main.c) - Canonical embedding example

**Documentation**:
- [Hot Reload Wiki](https://github.com/HaxeFoundation/hashlink/wiki/Hot-Reload) - Official hot reload guide
- [Embedding HashLink](https://github.com/HaxeFoundation/hashlink/wiki/Embedding-HashLink) - Embedding documentation

**Community Discussions**:
- [HashLink GUI & dynamic loading](https://groups.google.com/g/haxelang/c/7A8lTpPN3Xs) - Google Groups discussion

---

**Report Completed**: 2025-11-22
**Confidence Level**: 🔴 **HIGH** - Source code evidence + issue history + lack of official support
**Recommendation**: Remove restart from HLFFI v3.0, focus on Hot Reload integration
