# HLC (HashLink/C) Support for HLFFI

This document describes how to add HLC compilation support to HLFFI, enabling ARM and other non-x86 platforms.

## Background

### JIT vs HLC

| Mode | Description | Platforms |
|------|-------------|-----------|
| **JIT** | Load `.hl` bytecode at runtime, JIT compile | x86/x64 (Windows, Linux, Mac) |
| **HLC** | Compile Haxe → C → native binary | Any platform with C compiler (ARM, WASM, etc.) |

### Why HLC Support Matters

- **ARM devices**: Raspberry Pi, Android, iOS, Nintendo Switch, Apple Silicon
- **WebAssembly**: Browser deployment
- **Performance**: Native compilation can be faster than JIT
- **No JIT restrictions**: Some platforms prohibit JIT (iOS App Store)

## The Problem

HLFFI's type lookup currently relies on JIT-only structures:

```c
// Current code in hlffi_types.c - FAILS in HLC mode
hl_code* code = vm->module->code;  // NULL in HLC!
for (int i = 0; i < code->ntypes; i++) {
    hl_type* t = code->types + i;
    // ...
}
```

In HLC mode:
- No `hl_module` structure exists
- No `hl_code` structure exists
- No `code->types[]` array exists
- Types are compiled as static C structures

## The Solution

### Key Discovery: Type.allTypes Registry

Haxe's standard library maintains a runtime type registry that works in **both** JIT and HLC modes:

```haxe
// std/hl/_std/Type.hx
class Type {
    static var allTypes:hl.types.BytesMap;  // Registry of all types!

    public static function resolveClass(name:String):Class<Dynamic> {
        return allTypes.get(name.bytes);
    }
}
```

After `hl_entry_point()` runs, `Type.allTypes` is fully populated with all types.

### HLC Symbol Access

HLC generates stable C symbols for types following the pattern `t$<ClassName>`:

```c
// These symbols exist in HLC-compiled code
extern hl_type t$Type;    // The Type class
extern hl_type t$String;  // The String class
extern hl_type t$Player;  // User's Player class
```

### Architecture: Dual-Path Approach

**Keep existing JIT code** (simple, direct, proven) and **add HLC path** using Type.resolveClass:

```
┌─────────────────────────────────────────────────────────────┐
│                    hlffi_find_type()                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  #ifdef HLFFI_HLC_MODE                                      │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ HLC Path:                                            │   │
│  │ 1. extern hl_type t$Type (known symbol)              │   │
│  │ 2. Get Type.resolveClass method                      │   │
│  │ 3. Call resolveClass(name)                           │   │
│  │ 4. Extract __type__ from result                      │   │
│  └─────────────────────────────────────────────────────┘   │
│  #else                                                      │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ JIT Path (existing code):                            │   │
│  │ 1. Iterate code->types[]                             │   │
│  │ 2. Compare names                                     │   │
│  │ 3. Return matching hl_type*                          │   │
│  └─────────────────────────────────────────────────────┘   │
│  #endif                                                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Implementation Guide

### Step 1: Add HLC Mode Detection

In `hlffi_internal.h`:

```c
/*
 * HLC Mode Detection
 *
 * HLFFI_HLC_MODE should be defined when building for HLC targets.
 * This changes how type lookup works since there's no hl_code structure.
 */
#ifdef HLFFI_HLC_MODE
    /* HLC mode: No hl_module/hl_code, types are static C symbols */
    #define HLFFI_HAS_BYTECODE 0
#else
    /* JIT mode: Full hl_module/hl_code available */
    #define HLFFI_HAS_BYTECODE 1
#endif
```

### Step 2: Add HLC Type Lookup Cache

In `hlffi_internal.h` or a new `hlffi_hlc.c`:

```c
#ifdef HLFFI_HLC_MODE

/* Cached references for HLC type lookup (initialized once) */
typedef struct {
    hl_type* type_class;           /* The Type class's hl_type */
    vdynamic* type_class_global;   /* Type class global instance */
    int resolve_class_hash;        /* Hash of "resolveClass" */
    int type_field_hash;           /* Hash of "__type__" */
    bool initialized;
} hlffi_hlc_cache;

static hlffi_hlc_cache g_hlc_cache = {0};

#endif
```

### Step 3: Implement HLC Initialization

```c
#ifdef HLFFI_HLC_MODE

/**
 * Initialize HLC type lookup.
 * Must be called after hlffi_call_entry().
 */
static hlffi_error_code hlffi_hlc_init(hlffi_vm* vm) {
    if (g_hlc_cache.initialized) return HLFFI_OK;

    /* Access the Type class via known HLC symbol */
    extern hl_type t$Type;
    g_hlc_cache.type_class = &t$Type;

    /* Verify Type class is initialized */
    if (!g_hlc_cache.type_class->obj ||
        !g_hlc_cache.type_class->obj->global_value) {
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /* Get Type class global instance (where static methods live) */
    g_hlc_cache.type_class_global =
        *(vdynamic**)g_hlc_cache.type_class->obj->global_value;

    if (!g_hlc_cache.type_class_global) {
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /* Cache field hashes for performance */
    g_hlc_cache.resolve_class_hash = hl_hash_utf8("resolveClass");
    g_hlc_cache.type_field_hash = hl_hash_utf8("__type__");
    g_hlc_cache.initialized = true;

    return HLFFI_OK;
}

#endif
```

### Step 4: Implement Unified hlffi_find_type

```c
hlffi_type* hlffi_find_type(hlffi_vm* vm, const char* name) {
    if (!vm) return NULL;
    if (!name) {
        hlffi_set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Type name is NULL");
        return NULL;
    }

#ifdef HLFFI_HLC_MODE
    /*========================================
     * HLC MODE: Use Type.resolveClass()
     *========================================*/

    /* Ensure HLC lookup is initialized */
    if (!g_hlc_cache.initialized) {
        if (hlffi_hlc_init(vm) != HLFFI_OK) {
            hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
                "HLC type lookup not initialized - call hlffi_call_entry() first");
            return NULL;
        }
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Get resolveClass method from Type class */
    vclosure* resolve_method = (vclosure*)hl_dyn_getp(
        g_hlc_cache.type_class_global,
        g_hlc_cache.resolve_class_hash,
        &hlt_dyn
    );

    if (!resolve_method) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Type.resolveClass not found");
        return NULL;
    }

    /* Create Haxe String from C string */
    int name_len = (int)strlen(name);
    uchar* name_utf16 = hl_from_utf8((vbyte*)name);

    /* Allocate String object */
    vdynamic* name_str = hl_alloc_dynamic(&hlt_bytes);
    name_str->v.ptr = name_utf16;

    /* Call Type.resolveClass(name) */
    vdynamic* args[1] = { name_str };
    bool isExc = false;
    vdynamic* cls_result = hl_dyn_call_safe(resolve_method, args, 1, &isExc);

    if (isExc || !cls_result) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Type not found: %s", name);
        hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, error_buf);
        return NULL;
    }

    /* Extract hl_type* from Class<T> result
     * Class extends BaseType which has __type__ field */
    hl_type* found_type = (hl_type*)hl_dyn_getp(
        cls_result,
        g_hlc_cache.type_field_hash,
        &hlt_dyn
    );

    return (hlffi_type*)found_type;

#else
    /*========================================
     * JIT MODE: Iterate code->types[] (existing)
     *========================================*/

    if (!vm->module || !vm->module->code) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "VM not initialized or no bytecode loaded");
        return NULL;
    }

    hl_code* code = vm->module->code;
    int target_hash = hl_hash_utf8(name);

    /* Search all types in loaded module */
    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;

        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char* type_name_utf8 = hl_to_utf8(t->obj->name);
            if (type_name_utf8) {
                int type_hash = hl_hash_utf8(type_name_utf8);
                if (type_hash == target_hash) {
                    return (hlffi_type*)t;
                }
            }
        }
        else if (t->kind == HENUM && t->tenum && t->tenum->name) {
            char* type_name_utf8 = hl_to_utf8(t->tenum->name);
            if (type_name_utf8) {
                int type_hash = hl_hash_utf8(type_name_utf8);
                if (type_hash == target_hash) {
                    return (hlffi_type*)t;
                }
            }
        }
        else if (t->kind == HABSTRACT && t->abs_name) {
            char* type_name_utf8 = hl_to_utf8(t->abs_name);
            if (type_name_utf8) {
                int type_hash = hl_hash_utf8(type_name_utf8);
                if (type_hash == target_hash) {
                    return (hlffi_type*)t;
                }
            }
        }
    }

    char error_buf[256];
    snprintf(error_buf, sizeof(error_buf), "Type not found: %s", name);
    hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, error_buf);
    return NULL;

#endif /* HLFFI_HLC_MODE */
}
```

### Step 5: Update hlffi_list_types for HLC

```c
hlffi_error_code hlffi_list_types(hlffi_vm* vm, hlffi_type_callback callback, void* userdata) {
    if (!vm) return HLFFI_ERROR_NULL_VM;
    if (!callback) return HLFFI_ERROR_INVALID_ARGUMENT;

#ifdef HLFFI_HLC_MODE
    /*========================================
     * HLC MODE: Iterate Type.allTypes.values()
     *========================================*/

    if (!g_hlc_cache.initialized) {
        if (hlffi_hlc_init(vm) != HLFFI_OK) {
            return HLFFI_ERROR_NOT_INITIALIZED;
        }
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Get allTypes field from Type class */
    int allTypes_hash = hl_hash_utf8("allTypes");
    vdynamic* all_types_map = hl_dyn_getp(
        g_hlc_cache.type_class_global,
        allTypes_hash,
        &hlt_dyn
    );

    if (!all_types_map) {
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /* Get values() method from BytesMap */
    int values_hash = hl_hash_utf8("values");
    vclosure* values_method = (vclosure*)hl_dyn_getp(
        all_types_map, values_hash, &hlt_dyn);

    if (!values_method) {
        return HLFFI_ERROR_METHOD_NOT_FOUND;
    }

    /* Call values() to get array of all registered types */
    bool isExc = false;
    varray* values = (varray*)hl_dyn_call_safe(values_method, NULL, 0, &isExc);

    if (isExc || !values) {
        return HLFFI_ERROR_CALL_FAILED;
    }

    /* Iterate values array, extract __type__ from each BaseType */
    for (int i = 0; i < values->size; i++) {
        vdynamic* base_type = hl_aptr(values, vdynamic*)[i];
        if (base_type) {
            hl_type* t = (hl_type*)hl_dyn_getp(
                base_type,
                g_hlc_cache.type_field_hash,
                &hlt_dyn
            );
            if (t) {
                callback((hlffi_type*)t, userdata);
            }
        }
    }

    return HLFFI_OK;

#else
    /*========================================
     * JIT MODE: Iterate code->types[] (existing)
     *========================================*/

    if (!vm->module || !vm->module->code) {
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    hl_code* code = vm->module->code;

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        callback((hlffi_type*)t, userdata);
    }

    return HLFFI_OK;

#endif /* HLFFI_HLC_MODE */
}
```

### Step 6: Update Other Affected Functions

Functions that use `vm->module->code` need HLC alternatives:

| Function | JIT Implementation | HLC Implementation |
|----------|-------------------|-------------------|
| `hlffi_find_type` | Scan `code->types[]` | `Type.resolveClass()` |
| `hlffi_list_types` | Iterate `code->types[]` | `Type.allTypes.values()` |
| `hlffi_new` | Find ctor in `code->functions[]` | Use `Type.createInstance()` |
| `hlffi_call_static` | Find method in `code->functions[]` | Use `Reflect.callMethod()` |
| `hlffi_reload_*` | `hl_module_patch()` | Return `HLFFI_ERROR_RELOAD_NOT_SUPPORTED` |

### Step 7: Update CMake Build System

```cmake
# In CMakeLists.txt

option(HLFFI_HLC_MODE "Build for HLC (HashLink/C) instead of JIT" OFF)

if(HLFFI_HLC_MODE)
    add_definitions(-DHLFFI_HLC_MODE)
    message(STATUS "HLFFI: Building for HLC mode (ARM/cross-platform)")

    # Disable features not available in HLC
    set(HLFFI_ENABLE_HOT_RELOAD OFF CACHE BOOL "" FORCE)
else()
    message(STATUS "HLFFI: Building for JIT mode (x86/x64)")
endif()
```

## Usage Guide

### Building for JIT (Default)

```bash
cmake -B build -DHASHLINK_DIR="/path/to/hashlink"
cmake --build build
```

### Building for HLC

```bash
# Cross-compile for ARM
cmake -B build-arm \
    -DHLFFI_HLC_MODE=ON \
    -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake \
    -DHASHLINK_DIR="/path/to/hashlink"
cmake --build build-arm
```

### Linking HLC Application

```bash
# 1. Compile Haxe to C
haxe -hl output/main.c -main Main

# 2. Compile with HLFFI (HLC mode)
gcc -o myapp \
    -DHLFFI_HLC_MODE \
    -I/path/to/hlffi/include \
    -I/path/to/hashlink/src \
    output/main.c \
    output/hl/*.c \
    /path/to/hashlink/src/hlc_main.c \
    -lhl -lm
```

## API Differences

### Functions That Work Identically

These use runtime APIs that are the same in both modes:

- `hlffi_create()` / `hlffi_destroy()`
- `hlffi_init()`
- `hlffi_call_entry()`
- `hlffi_get_field()` / `hlffi_set_field()`
- `hlffi_call_method()`
- `hlffi_value_*()` functions
- GC functions

### Functions With Different Behavior

| Function | JIT | HLC |
|----------|-----|-----|
| `hlffi_load_file()` | Loads .hl bytecode | Returns error (code is linked) |
| `hlffi_load_memory()` | Loads bytecode from buffer | Returns error |
| `hlffi_enable_hot_reload()` | Enables hot reload | Returns error |
| `hlffi_reload_module()` | Patches running module | Returns error |

### HLC-Only Initialization

In HLC mode, there's no bytecode to load. Initialization is simpler:

```c
// JIT mode
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "game.hl");  // Load bytecode
hlffi_call_entry(vm);

// HLC mode
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
// No load needed - code is statically linked
hlffi_call_entry(vm);  // Calls hl_entry_point()
```

## Technical Details

### How Type.resolveClass Works

```
┌──────────────────────────────────────────────────────────┐
│                  Type.resolveClass("Player")             │
├──────────────────────────────────────────────────────────┤
│ 1. Convert "Player" to UTF-16 bytes                      │
│ 2. Look up in Type.allTypes BytesMap (O(1) hash lookup)  │
│ 3. Return Class<Player> object (extends BaseType)        │
│ 4. Class.__type__ contains raw hl_type* pointer          │
└──────────────────────────────────────────────────────────┘
```

### HLC Symbol Naming Convention

HLC generates stable C symbols:

| Haxe Type | HLC Symbol |
|-----------|------------|
| `Type` | `t$Type` |
| `String` | `t$String` |
| `my.package.Player` | `t$my$package$Player` |

### BaseType Structure

```c
// Haxe: class BaseType
struct BaseType {
    hl_type* $type;              // Standard HL object header
    hl_type* __type__;           // The wrapped type (what we need!)
    vdynamic* __meta__;          // Metadata
    varray* __implementedBy__;   // Implementing types
};

// Haxe: class Class extends BaseType
struct Class {
    // BaseType fields...
    vbyte* __name__;             // Class name
    vclosure* __constructor__;   // Constructor function
};
```

## Limitations

### Hot Reload Not Supported

HLC compiles to static native code - there's no way to patch it at runtime.

```c
#ifdef HLFFI_HLC_MODE
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable) {
    return HLFFI_ERROR_RELOAD_NOT_SUPPORTED;
}
#endif
```

### Type Enumeration Differences

- **JIT**: Returns ALL types including primitives and internal types
- **HLC**: Returns only types registered in `Type.allTypes` (user classes, enums)

### Initialization Timing

In HLC mode, `hlffi_find_type()` requires `hlffi_call_entry()` to have been called first (to populate `Type.allTypes`). This is the same requirement as JIT mode.

## Testing

### Verify HLC Build

```c
#include "hlffi.h"

int main() {
    #ifdef HLFFI_HLC_MODE
    printf("HLFFI running in HLC mode\n");
    #else
    printf("HLFFI running in JIT mode\n");
    #endif

    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_call_entry(vm);

    // This should work in both modes
    hlffi_type* player = hlffi_find_type(vm, "Player");
    printf("Found Player type: %p\n", player);

    hlffi_destroy(vm);
    return 0;
}
```

## References

- [HashLink/C Compilation](https://haxe.org/manual/target-hl-c-compilation.html)
- [HashLink GitHub](https://github.com/HaxeFoundation/hashlink)
- [Working with C in HashLink](https://aramallo.com/blog/hashlink/calling-c.html)
