# HashLink Debugger Architecture

This document explains how the VSCode HashLink debugger works, based on investigation of the HashLink source code and HLFFI integration.

## Overview

The HashLink debugger enables source-level debugging of Haxe code running in the HashLink JIT VM. It allows:
- Setting breakpoints in `.hx` source files
- Stepping through code (step over, step into, step out)
- Inspecting variables and call stacks
- Evaluating expressions

**Important**: This debugger only works with JIT mode (`.hl` bytecode). HLC (HashLink/C) compiled code uses native debuggers like Visual Studio or GDB.

## Architecture Components

```
┌─────────────────────────────────────────────────────────────────┐
│                        VSCode IDE                                │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │          HashLink Debugger Extension                     │    │
│  │  - Parses .hl bytecode for debug symbols                │    │
│  │  - Manages breakpoints, stepping, variable display      │    │
│  │  - Communicates via Debug Adapter Protocol (DAP)        │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ TCP Connection (port 6112)
                              │ Custom binary protocol
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    HashLink Process                              │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │              Debug Server Thread                         │    │
│  │  - Accepts TCP connection from VSCode                   │    │
│  │  - Sends module info (types, functions, addresses)      │    │
│  │  - Receives breakpoint/step commands                    │    │
│  │  - Reports execution state changes                      │    │
│  │                                                         │    │
│  │  Source: vendor/hashlink/src/debugger.c                │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│                              │ DebugActiveProcess (Windows)      │
│                              │ ptrace (Linux/Mac)                │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                  JIT Code Memory                         │    │
│  │  - Contains compiled machine code                       │    │
│  │  - Breakpoints written as INT3 (0xCC) instructions     │    │
│  │  - Exception handlers catch breakpoint exceptions       │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

## Two-Stage Debugger Attachment

The HashLink debugger uses a two-stage attachment process:

### Stage 1: TCP Connection

1. Host application calls `hl_module_debug(module, port, wait)`
2. HashLink creates a debug server thread listening on the specified port
3. VSCode debugger extension connects via TCP
4. Server sends module metadata:
   - JIT code base addresses
   - Type information
   - Function metadata with source file/line mappings
   - Global variable addresses

### Stage 2: Native Debugger Attachment

1. VSCode extension calls Windows `DebugActiveProcess()` to attach as a native debugger
2. This allows the extension to:
   - Write `INT3` (0xCC) breakpoint instructions to JIT code via `WriteProcessMemory()`
   - Receive `EXCEPTION_BREAKPOINT` notifications
   - Read/write process memory for variable inspection

```c
// From vendor/hashlink/src/debugger.c
// Server sets this flag when TCP client connects
hl_setup.is_debugger_attached = true;

// VSCode extension then calls DebugActiveProcess() to attach natively
// This enables WriteProcessMemory for setting breakpoints
```

## Key Data Structures

### hl_setup (Global Configuration)

```c
// From vendor/hashlink/src/hl.h
typedef struct {
    // File path for debugger to locate source
    const void *file_path;

    // System arguments
    void **sys_args;
    int sys_nargs;

    // Debugger state flags
    bool is_debugger_enabled;   // hl_module_debug() was called
    bool is_debugger_attached;  // TCP client connected

    // Callbacks
    void *load_plugin;
    void *resolve_type;
    void *reload_check;
} hl_setup_struct;

extern hl_setup_struct hl_setup;
```

### Debug Server Thread

HashLink creates a dedicated thread for the debug server:

```c
// Simplified from vendor/hashlink/src/debugger.c
void hl_module_debug(hl_module *m, int port, bool wait) {
    // Create TCP server socket
    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    bind(server, ...port...);
    listen(server, 1);

    // Create debug thread
    hl_thread_start(debug_thread_func, ...);

    if (wait) {
        // Block until debugger connects
        while (!hl_setup.is_debugger_attached) {
            Sleep(10);
        }
    }
}
```

## Breakpoint Mechanism

### How Breakpoints Work

1. **VSCode sends breakpoint request** via TCP with source file + line number
2. **Debug server maps to JIT address** using debug symbols in `.hl` bytecode
3. **VSCode writes INT3** (0xCC) to that address via `WriteProcessMemory()`
4. **When CPU hits INT3**, Windows raises `EXCEPTION_BREAKPOINT`
5. **Debug server catches exception**, notifies VSCode, waits for continue command
6. **VSCode restores original byte**, single-steps, re-inserts breakpoint

```
Original JIT code:        After breakpoint set:
┌──────────────────┐      ┌──────────────────┐
│ 0x48 0x89 0x5C   │      │ 0xCC 0x89 0x5C   │  ← INT3 replaces first byte
│ 0x24 0x08 ...    │      │ 0x24 0x08 ...    │
└──────────────────┘      └──────────────────┘
```

### Exception Handling

HashLink sets up a vectored exception handler to catch breakpoints:

```c
// From vendor/hashlink/src/std/debug.c
static LONG CALLBACK debug_handler(PEXCEPTION_POINTERS info) {
    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) {
        // Notify debug server
        // Wait for continue command
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

// Handler is installed when debugger is enabled
AddVectoredExceptionHandler(1, debug_handler);
```

## Safe Call Wrapper

HashLink's `hl_dyn_call_safe()` provides exception handling for Haxe code:

```c
// From vendor/hashlink/src/std/debug.c
vdynamic* hl_dyn_call_safe(vclosure *cl, vdynamic **args, int nargs, bool *isException) {
    // On first call, checks IsDebuggerPresent()
    // If false, installs its own exception handler
    // This may conflict with the debugger's exception handling!

    __try {
        return hl_dyn_call(cl, args, nargs);
    } __except(exception_filter(...)) {
        *isException = true;
        return exception_value;
    }
}
```

## Debug Protocol Messages

The TCP protocol uses a custom binary format:

### Messages from Server to Client

| Message | Description |
|---------|-------------|
| `MODULE_INFO` | JIT addresses, types, functions |
| `THREAD_STOPPED` | Execution paused (breakpoint/step) |
| `THREAD_STARTED` | Thread began execution |
| `THREAD_EXITED` | Thread terminated |

### Messages from Client to Server

| Message | Description |
|---------|-------------|
| `SET_BREAKPOINT` | Set breakpoint at file:line |
| `REMOVE_BREAKPOINT` | Remove breakpoint |
| `CONTINUE` | Resume execution |
| `STEP_OVER` | Step over current line |
| `STEP_INTO` | Step into function call |
| `STEP_OUT` | Step out of current function |
| `GET_STACKTRACE` | Request call stack |
| `GET_VARIABLES` | Request variable values |
| `EVALUATE` | Evaluate expression |

## Compiling with Debug Symbols

Debug information is only included when compiling with `--debug`:

```bash
# With debug symbols (required for debugger)
haxe --debug -hl game.hl -main Main

# Without debug symbols (smaller, no debugging)
haxe -hl game.hl -main Main
```

The `--debug` flag includes:
- Source file paths
- Line number mappings
- Local variable names
- Function parameter names

## HLFFI Integration

### Starting the Debugger

```c
// After loading bytecode, before calling entry point
hlffi_error_code err = hlffi_debug_start(vm, 6112, true);  // wait=true blocks
if (err != HLFFI_OK) {
    printf("Failed to start debugger: %s\n", hlffi_get_error_string(err));
}

// Now run Haxe code
hlffi_call_entry(vm);  // Breakpoints will pause here
```

### Checking Debugger Status

```c
if (hlffi_debug_is_attached(vm)) {
    printf("Debugger connected!\n");
}
```

### Stopping the Debugger

```c
hlffi_debug_stop(vm);  // Clean shutdown before destroy
hlffi_destroy(vm);
```

## Thread Registration

All threads that execute Haxe code must be registered with HashLink:

```c
// Register thread with GC (required before any HL calls)
int stack_marker;
hl_register_thread(&stack_marker);

// ... execute Haxe code ...

// Unregister when done
hl_unregister_thread();
```

The stack marker address is used by the GC to scan the stack for GC roots.

## VSCode Configuration

### launch.json for Attach Mode

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Attach to HashLink",
            "request": "attach",
            "type": "hl",
            "port": 6112,
            "cwd": "${workspaceFolder}/src",
            "program": "${workspaceFolder}/bin/game.hl"
        }
    ]
}
```

### Key Settings

| Setting | Description |
|---------|-------------|
| `port` | TCP port matching `hlffi_debug_start()` |
| `cwd` | Working directory for source file resolution |
| `program` | Path to `.hl` file (for debug symbols) |
| `stopOnEntry` | Pause at program start |

## Debugging Workflow

1. **Compile Haxe with debug symbols**
   ```bash
   haxe --debug -hl game.hl -main Main
   ```

2. **Start host application with debugger**
   ```c
   hlffi_debug_start(vm, 6112, false);  // Don't wait
   hlffi_call_entry(vm);
   ```

3. **Attach VSCode debugger**
   - Open VSCode with HashLink Debugger extension
   - Use "Attach" configuration
   - Set breakpoints in `.hx` files

4. **Debug**
   - Breakpoints pause execution
   - Inspect variables in VSCode
   - Step through code

## Limitations

1. **JIT Mode Only**: HLC compiled code cannot use this debugger
2. **Single Module**: One `.hl` file per debug session
3. **Thread Complexity**: See [DEBUG_FINDINGS.md](DEBUG_FINDINGS.md) for main thread issues
4. **No Edit and Continue**: Must restart to apply code changes

## Related Files

| File | Description |
|------|-------------|
| `vendor/hashlink/src/debugger.c` | Debug server implementation |
| `vendor/hashlink/src/std/debug.c` | Debug utilities, exception handling |
| `vendor/hashlink/src/hl.h` | `hl_setup` struct definition |
| `src/hlffi_lifecycle.c` | HLFFI debug API implementation |

## References

- [HashLink Debugger Extension](https://github.com/vshaxe/hashlink-debugger)
- [Debug Adapter Protocol](https://microsoft.github.io/debug-adapter-protocol/)
- [HashLink Source](https://github.com/HaxeFoundation/hashlink)
