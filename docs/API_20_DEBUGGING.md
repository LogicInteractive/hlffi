# API Reference: Debugging

HLFFI supports debugging Haxe code running in embedded VMs using the VSCode HashLink Debugger extension.

## Overview

The debug API enables the HashLink debugger protocol, allowing you to:
- Set breakpoints in Haxe source files
- Step through code line by line
- Inspect variables and call stacks
- Debug Haxe code running inside Unreal Engine, custom game engines, or any C/C++ host

**Requirements:**
- JIT mode only (not HLC - use native debuggers for HLC)
- Haxe compiled with `--debug` flag
- VSCode with [HashLink Debugger](https://marketplace.visualstudio.com/items?itemName=HaxeFoundation.haxe-hl) extension

## Functions

### hlffi_debug_start

```c
hlffi_error_code hlffi_debug_start(hlffi_vm* vm, int port, bool wait);
```

Start the HashLink debugger server.

**Parameters:**
- `vm` - The VM instance
- `port` - TCP port to listen on (default: 6112)
- `wait` - If true, block until debugger connects before returning

**Returns:** `HLFFI_OK` on success, error code on failure

**When to call:** After `hlffi_load_file()` but before `hlffi_call_entry()`

**Example:**
```c
hlffi_load_file(vm, "game.hl");
hlffi_debug_start(vm, 6112, false);  // Start server, don't wait
hlffi_call_entry(vm);                // Run - attach debugger anytime
```

### hlffi_debug_stop

```c
void hlffi_debug_stop(hlffi_vm* vm);
```

Stop the debugger server. Call during shutdown before `hlffi_destroy()`.

### hlffi_debug_is_attached

```c
bool hlffi_debug_is_attached(hlffi_vm* vm);
```

Check if a debugger is currently connected.

**Returns:** `true` if debugger is attached, `false` otherwise

## Usage Patterns

### Basic Usage

```c
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, 0, NULL);
hlffi_load_file(vm, "game.hl");  // Compiled with: haxe --debug -hl game.hl

// Start debug server
if (hlffi_debug_start(vm, 6112, false) == HLFFI_OK) {
    printf("Debugger listening on port 6112\n");
}

hlffi_call_entry(vm);

// ... game loop ...

hlffi_debug_stop(vm);
hlffi_destroy(vm);
```

### Wait for Debugger (Debug Startup Code)

```c
// Block until debugger connects - useful for debugging initialization
hlffi_debug_start(vm, 6112, true);  // Blocks here
hlffi_call_entry(vm);               // Debugger already attached
```

### Unreal Engine Integration

```cpp
void UHaxeSubsystem::Initialize() {
    vm = hlffi_create();
    hlffi_init(vm, 0, nullptr);
    hlffi_load_file(vm, "Content/Haxe/game.hl");

#if WITH_EDITOR
    // Enable debugging in Play-In-Editor
    if (GIsEditor) {
        hlffi_debug_start(vm, 6112, false);
        UE_LOG(LogHaxe, Log, TEXT("HashLink debugger on port 6112"));
    }
#endif

    hlffi_call_entry(vm);
}

void UHaxeSubsystem::Deinitialize() {
    hlffi_debug_stop(vm);
    hlffi_destroy(vm);
}
```

## VSCode Configuration

### launch.json

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Attach to Embedded HashLink",
            "request": "attach",
            "type": "hl",
            "port": 6112,
            "cwd": "${workspaceFolder}"
        }
    ]
}
```

### Workflow

1. **Compile Haxe with debug info:**
   ```bash
   haxe --debug -hl game.hl -main Main
   ```

2. **Start your C/C++ application** (it calls `hlffi_debug_start`)

3. **In VSCode:** Run the "Attach to Embedded HashLink" configuration

4. **Set breakpoints** in your `.hx` files

5. **Execution pauses** when breakpoints are hit

## Error Codes

| Code | Meaning |
|------|---------|
| `HLFFI_OK` | Debug server started successfully |
| `HLFFI_ERROR_DEBUG_START_FAILED` | Failed to start (port in use?) |
| `HLFFI_ERROR_DEBUG_NOT_AVAILABLE` | HLC mode - use native debugger |
| `HLFFI_ERROR_NOT_INITIALIZED` | Module not loaded yet |
| `HLFFI_ERROR_ALREADY_INITIALIZED` | Debugger already started |

## Notes

### JIT vs HLC

| Mode | Debugger | How to Debug |
|------|----------|--------------|
| **JIT** (`.hl` bytecode) | VSCode HashLink Debugger | Use `hlffi_debug_start()` |
| **HLC** (compiled to C) | Visual Studio / GDB | Native C debugging |

HLC compiles Haxe to C code, so you debug at the C level with native debuggers. The HashLink debugger only works with JIT bytecode.

### Breakpoint Behavior

When a debugger is attached and execution hits a breakpoint:
- **Execution pauses automatically** - this is the default HashLink behavior
- You can inspect variables, step through code, etc.
- No special configuration needed beyond attaching the debugger

### Multiple Instances

If running multiple embedded VMs (e.g., multiple PIE instances), use different ports:
```c
hlffi_debug_start(vm1, 6112, false);
hlffi_debug_start(vm2, 6113, false);
```

## Test Program

A standalone debug test is included:

```bash
# Build
cmake --build build-jit --target test_debug

# Run (interactive - waits for you to attach debugger)
test_debug.exe path/to/debug_test.hl

# Run with --wait (blocks until debugger connects)
test_debug.exe path/to/debug_test.hl --wait

# Run non-interactive (for CI/testing)
test_debug.exe path/to/debug_test.hl -n
```

The test Haxe file `test/DebugTest.hx` provides good breakpoint targets for testing.
