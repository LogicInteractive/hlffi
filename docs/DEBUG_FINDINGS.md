# HLFFI Debug API - Investigation Findings

This document summarizes the findings from investigating VSCode HashLink debugger integration with HLFFI embedded VMs.

## Problem Statement

When using the VSCode HashLink debugger with HLFFI in NON_THREADED mode, breakpoints cause the program to exit instead of pausing execution. The same bytecode works perfectly with the official `hl.exe`.

## Test Results Summary

| Mode | Debugger Attached | Breakpoints Work | Notes |
|------|-------------------|------------------|-------|
| Official `hl.exe` | Yes | **Yes** | Reference implementation |
| HLFFI THREADED | Yes | **Yes** | Haxe runs in dedicated VM thread |
| HLFFI MANUAL_THREAD | Yes | **Yes** | Haxe runs in manually created C thread |
| HLFFI NON_THREADED | Yes | **No** | Haxe runs on main thread - **FAILS** |

## Key Finding

**Debugging works when Haxe code runs in a worker thread, but fails when running on the main thread.**

The common factor in working modes:
- THREADED mode: Uses `hlffi_thread_start()` which creates a dedicated VM thread
- MANUAL_THREAD mode: Uses `_beginthreadex()` to create a worker thread manually
- Both call `hl_register_thread(&stack_marker)` with a valid stack address

The failing mode:
- NON_THREADED: Runs Haxe directly on the main thread
- Main thread was registered early via `hl_register_thread(NULL)` in `hlffi_init()`

## Symptoms

When a breakpoint is hit in NON_THREADED mode:
1. Program executes partially (e.g., loop iterations 0-4)
2. Program exits cleanly (no crash, no exception printed)
3. `atexit()` handler IS called, indicating normal exit
4. VSCode debugger shows the process terminated

No exception handler catches anything - suggesting the debugger might be signaling the process to exit rather than an unhandled exception.

## What We Tested

### 1. File Path Setup
Added `hl_setup.file_path` with proper UTF-16 conversion for Windows.
**Result**: Did not fix the issue.

### 2. Exception Handlers
Added vectored exception handler to catch `EXCEPTION_BREAKPOINT`.
**Result**: Handler never triggered for breakpoints, suggesting debugger handles them before our handler.

### 3. Native Debugger Detection
Added wait loop for `IsDebuggerPresent()` to ensure native debugger was attached before running.
**Result**: Native debugger confirmed attached, still fails.

### 4. Thread Mode Comparison
Tested THREADED vs NON_THREADED vs MANUAL_THREAD.
**Result**: Worker threads work, main thread fails.

## Differences Between hl.exe and HLFFI

| Aspect | hl.exe | HLFFI NON_THREADED |
|--------|--------|-------------------|
| `hl_register_thread()` | Called with `&ctx` (stack address) | Called with `NULL` |
| `hl_setup.sys_args` | Set to argv | Not set |
| `hl_setup.load_plugin` | Set to callback | Not set |
| `hl_setup.resolve_type` | Set to callback | Not set |
| Entry point call | `hl_dyn_call_safe()` | `hl_dyn_call_safe()` (same) |

## Theories

### Theory 1: Main Thread Exception Handling
Windows may handle exceptions differently on the main thread vs worker threads when a debugger is attached via `DebugActiveProcess()`. The main thread might have special exception handling that interferes with breakpoint processing.

### Theory 2: Thread Registration Timing
In NON_THREADED mode, `hl_register_thread(NULL)` is called before the debugger attaches. In worker thread modes, `hl_register_thread(&stack_marker)` is called after debugger attachment. This timing difference might affect how the debugger tracks threads.

### Theory 3: Stack Top NULL
Passing `NULL` to `hl_register_thread()` sets `stack_top = NULL` in the thread info. While this is primarily for GC stack scanning, it might affect debugger behavior in ways we haven't identified.

## Workaround

**For debugging with HLFFI, use THREADED mode:**

```c
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "game.hl");
hlffi_debug_start(vm, 6112, true);  // Wait for debugger
hlffi_thread_start(vm);  // Runs in dedicated thread - breakpoints work!
```

NON_THREADED mode is still suitable for:
- Production builds without debugging
- HLC mode (which uses native debuggers anyway)
- Scenarios where you don't need Haxe-level debugging

## Test Commands

```bash
# THREADED mode (works)
test_debug.exe test\debug_test.hl --wait

# NON_THREADED mode (fails with debugger)
test_debug.exe test\debug_test.hl --non-threaded --wait

# MANUAL_THREAD mode (works - confirms worker thread theory)
test_debug.exe test\debug_test.hl --manual-thread --wait
```

## Files Modified During Investigation

- `test/jit_test/test_debug.c` - Added multi-mode support and diagnostics
- `src/hlffi_lifecycle.c` - Added `hl_setup.file_path` for debugger
- `.vscode/launch.json` - Debugger attach configuration

## Future Investigation

To fully understand and potentially fix NON_THREADED debugging:
1. Trace Windows debug events during breakpoint hit
2. Compare thread info structures between main and worker threads
3. Test delaying `hl_register_thread()` until after debugger attaches
4. Investigate if `hl_dyn_call_safe()` behaves differently on main thread

## Related Resources

- [HashLink Debugger Extension](https://github.com/vshaxe/hashlink-debugger)
- [HashLink Source - debugger.c](vendor/hashlink/src/debugger.c)
- [HashLink Source - debug.c](vendor/hashlink/src/std/debug.c)
