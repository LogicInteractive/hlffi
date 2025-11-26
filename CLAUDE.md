# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

`hlffi` is a single-header C/C++ library that embeds the HashLink VM (Haxe runtime) into any C/C++ application. It provides a minimal, high-performance FFI (Foreign Function Interface) bridge between C/C++ host applications and Haxe/HashLink bytecode.

**Key characteristics:**
- Single header-only library ([hlffi.h](hlffi.h)) with implementation via `#define HLFFI_IMPLEMENTATION`
- Feature flags to enable/disable components (`HLFFI_EXT_STATIC`, `HLFFI_EXT_VAR`, `HLFFI_EXT_VALUE`, `HLFFI_EXT_ERRORS`, `HLFFI_EXT_UNREAL`)
- Two error handling modes: enum-based (`hlffi_result`) and boolean-based (`*_bool` functions)
- Wraps HashLink C API (`hl.h`, `hlmodule.h`) with ergonomic helpers

## Build System

### Building with CMake

```bash
# Configure with HashLink directory
cmake -B build -DHASHLINK_DIR="C:/HashLink"

# Build
cmake --build build

# Build with options
cmake -B build -DHLFFI_BUILD_EXAMPLES=ON -DHLFFI_BUILD_TESTS=ON
```

**Important CMake variables:**
- `HASHLINK_DIR` - Path to HashLink installation (required)
- `HLFFI_BUILD_SHARED` - Build shared library (default: OFF)
- `HLFFI_BUILD_EXAMPLES` - Build examples (default: ON)
- `HLFFI_BUILD_TESTS` - Build tests (default: ON)
- `HLFFI_ENABLE_HOT_RELOAD` - Enable hot reload (default: ON, requires HL 1.12+)

### Compiling Haxe Test Files

The [test/](test/) directory contains Haxe source files that compile to `.hl` bytecode for testing:

```bash
# Windows
cd test
build.bat

# Manual
haxe -hl hello.hl -main Main
```

## Architecture

### Core Design Patterns

1. **Single Header Implementation Pattern**
   - All code is in [hlffi.h](hlffi.h)
   - Implementation guarded by `#ifdef HLFFI_IMPLEMENTATION`
   - Use `static inline` for functions to avoid linkage issues

2. **Feature Flag System**
   - Each major feature controlled by `HLFFI_EXT_*` define
   - Defaults to all features enabled (except Unreal)
   - Compile-time elimination of unused code

3. **Dual Error Handling Modes**
   - **Enum mode** (`HLFFI_EXT_ERRORS=1`): Functions return `hlffi_result`, detailed errors via `hlffi_error(vm)`
   - **Bool mode**: Functions with `*_bool` suffix return `true/false`, optional `const char** errmsg` parameter
   - Both modes available simultaneously for migration flexibility

4. **VM Handle Pattern**
   - All state lives in `hlffi_vm*` handle
   - No global state in hlffi (though HashLink itself uses globals)
   - Supports multiple VM instances per process (with caveats from HashLink)

### Key Components

**Lifecycle API** (create → init → load → call → close → destroy):
- `hlffi_create()` - Allocate VM handle, call `hl_global_init()` and `hl_register_thread()`
- `hlffi_init_args()` - Initialize HashLink system with argc/argv
- `hlffi_load_file()` / `hlffi_load_mem()` - Load compiled `.hl` bytecode
- `hlffi_call_entry()` - Execute `Main.main()` or custom entry point
- `hlffi_close()` - Clean up HashLink runtime
- `hlffi_destroy()` - Free VM handle

**Low-Level Call API** (when you know function name and arity at compile time):
- `hlffi_lookup(vm, "functionName", nargs)` - Get function pointer via hash lookup
- `hlffi_call0..4(ptr, args...)` - Direct inline calls to `hl_call0..4` (~2ns overhead)
- `hlffi_call_variadic(ptr, argc, ...)` - Dynamic arity calls (3-5x slower)

**High-Level Static Helpers** (`HLFFI_EXT_STATIC`):
- `hlffi_call_static(vm, "ClassName", "methodName")` - Call static methods by name
- `hlffi_call_static_ret_int/float/string()` - Typed return value helpers
- Uses hash lookups internally (~40ns overhead)

**Static Variable Access** (`HLFFI_EXT_VAR`):
- `hlffi_get_static_int/float/string(vm, "Class", "varName")` - Read static variables
- `hlffi_set_static_*()` - Write static variables
- UTF-8 ↔ UTF-16 conversion handled automatically for strings

**Boxed Value Layer** (`HLFFI_EXT_VALUE`):
- `hlffi_value` - Tagged union wrapper around `vdynamic*`
- `hlffi_val_int/float/bool/string/null()` - Create boxed values
- `hlffi_val_as_*()` - Extract typed values with fallback
- Use for API boundaries; avoid in tight loops (2-3 allocations per value)

**Threading & GC**:
- `hlffi_thread_attach()` / `hlffi_thread_detach()` - Register threads that touch HL data
- `hlffi_gc_stats()`, `hlffi_gc_collect()` - Memory management utilities

**Unreal Engine Integration** (`HLFFI_EXT_UNREAL=1`):
- `HLFFI_UE_CALL()` macros for one-liner calls with `UE_LOG` integration

### Memory Management Rules

| Type | Ownership | How to Free |
|------|-----------|-------------|
| `hlffi_vm*` | Caller | `hlffi_destroy(vm)` |
| `char*` from `*_ret_string` or `*_get_static_string` | Caller | `free()` (uses `strdup`) |
| `hlffi_value*` | Caller | `hlffi_val_free()` |
| `vdynamic*` | HashLink GC | Keep root or pass to HL |

**Critical**: Never mix CRT heaps. Always use the same `malloc/free` pair.

## Development Workflow

### Making Changes to hlffi.h

1. The entire library is in [hlffi.h](hlffi.h)
2. Structure:
   - Lines 1-40: Header guard, includes, HashLink headers
   - Lines 41-55: Feature flags
   - Lines 56-148: Type definitions, function prototypes
   - Lines 156+: Implementation (guarded by `HLFFI_IMPLEMENTATION`)

3. When adding features:
   - Add prototype before `#ifdef HLFFI_IMPLEMENTATION`
   - Add implementation after
   - Use `static inline` when possible
   - Guard with appropriate `HLFFI_EXT_*` flag if optional

### Testing Changes

1. Build examples: `cmake --build build --target examples`
2. Compile Haxe test: `cd test && build.bat`
3. Run example: `bin\x64\Debug\example_hello_world.exe test\hello.hl`

### Performance Considerations

- `hlffi_call0..4`: ~0% overhead (thin inline over `hl_callX`)
- `hlffi_call_variadic`: ~3-5x slower (uses `alloca` and `va_list`)
- Static method helpers: ~40ns (hash lookups) - cache with `hlffi_lookup()` for hot loops
- Boxed values: 2-3 allocations - avoid in tight loops

## Common Patterns

### Embedding HashLink in a C++ Application

```cpp
#define HLFFI_IMPLEMENTATION
#include "hlffi.h"

hlffi_vm* vm = hlffi_create();
if (!hlffi_init_args(vm, argc, argv)) { /* error */ }
if (hlffi_load_file(vm, "game.hl") != HLFFI_OK) {
    fprintf(stderr, "%s\n", hlffi_error(vm));
}
hlffi_call_entry(vm);
hlffi_close(vm);
hlffi_destroy(vm);
```

### Calling Haxe Functions from C++

```cpp
// Low-level (cache the pointer)
void* spawn = hlffi_lookup(vm, "Player.spawn", 2);
hlffi_call2(spawn, x_arg, y_arg);

// High-level (convenience)
hlffi_call_static_int(vm, "Game", "setScore", 100);
int score = hlffi_call_static_ret_int(vm, "Game", "getScore");
```

### Multi-threaded Access

```cpp
void worker_thread() {
    hlffi_thread_attach();
    // Safe to call HL functions here
    hlffi_thread_detach();
}
```

## Documentation

- [README.md](README.md) - Quick start, features, API overview
- [hlffi_manual.md](hlffi_manual.md) - Complete developer manual (17 sections, ~350 lines)
- [test/README.md](test/README.md) - Test file documentation

## Important Notes

- **HashLink strings are UTF-16** - hlffi handles UTF-8 ↔ UTF-16 conversion transparently
- **Thread safety**: All lifecycle functions must run on same thread as `hlffi_init_args()` unless wrapped in `hlffi_thread_attach/detach`
- **Version compatibility**: `.hl` bytecode must match HashLink runtime version (recompile if needed)
- **Feature flag changes require recompilation**: All TUs including `hlffi.h` must use same flags
