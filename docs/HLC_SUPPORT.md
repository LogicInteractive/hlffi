# HLC (HashLink/C) Support for HLFFI

Complete guide for adding HLC compilation support to HLFFI, enabling ARM and other non-x86 platforms.

## Table of Contents

1. [Background](#background)
2. [Architecture Overview](#architecture-overview)
3. [Functions Requiring HLC Support](#functions-requiring-hlc-support)
4. [Implementation Details](#implementation-details)
5. [New Functions for HLC](#new-functions-for-hlc)
6. [Build System Integration](#build-system-integration)
7. [Usage Guide](#usage-guide)
8. [Testing](#testing)

---

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

### Core Architectural Difference

**JIT Mode:**
```
.hl bytecode → hl_code_read() → hl_module_alloc() → hl_module_init() → runtime
                                      ↓
                              code->types[]      ← Type metadata
                              code->functions[]  ← Function metadata
                              module->functions_ptrs[] ← JIT'd code
```

**HLC Mode:**
```
.hx source → haxe --hl output.c → gcc → native binary
                                           ↓
                                   Static hl_type symbols (t$ClassName)
                                   Static function symbols
                                   No hl_code/hl_module structures
```

---

## Architecture Overview

### The Problem

HLFFI relies on JIT-only structures that don't exist in HLC:

```c
// These work in JIT, fail in HLC
vm->module          // NULL in HLC
vm->module->code    // NULL in HLC
code->types[]       // Doesn't exist in HLC
code->functions[]   // Doesn't exist in HLC
code->ntypes        // Doesn't exist in HLC
```

### The Solution: Type.allTypes Registry

Haxe's standard library maintains a runtime type registry that works in **both** modes:

```haxe
// std/hl/_std/Type.hx - works in JIT and HLC!
class Type {
    static var allTypes:hl.types.BytesMap;

    public static function resolveClass(name:String):Class<Dynamic> {
        return allTypes.get(name.bytes);
    }

    public static function createInstance<T>(cl:Class<T>, args:Array<Dynamic>):T {
        // Creates instances dynamically
    }
}
```

### HLC Symbol Naming Convention

HLC generates stable C symbols following the pattern `t$<ClassName>`:

```c
extern hl_type t$Type;           // Type class
extern hl_type t$String;         // String class
extern hl_type t$Array;          // Array class
extern hl_type t$my$pkg$Player;  // my.pkg.Player class
```

### Dual-Path Strategy

Keep existing JIT code (proven, simple) and add HLC path using Haxe reflection:

```
┌─────────────────────────────────────────────────────────────┐
│                      HLFFI Function                         │
├─────────────────────────────────────────────────────────────┤
│  #ifdef HLFFI_HLC_MODE                                      │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ HLC: Use Type.resolveClass / Reflect.* APIs         │   │
│  └─────────────────────────────────────────────────────┘   │
│  #else                                                      │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ JIT: Use code->types[] / code->functions[] (existing)│   │
│  └─────────────────────────────────────────────────────┘   │
│  #endif                                                     │
└─────────────────────────────────────────────────────────────┘
```

---

## Functions Requiring HLC Support

### Category 1: Functions Needing HLC Alternatives

These functions use `vm->module->code` and need completely different implementations for HLC:

| Function | File | JIT Implementation | HLC Implementation |
|----------|------|-------------------|-------------------|
| `hlffi_find_type()` | hlffi_types.c | Scan `code->types[]` | `Type.resolveClass()` |
| `hlffi_list_types()` | hlffi_types.c | Iterate `code->types[]` | `Type.allTypes.values()` |
| `hlffi_new()` | hlffi_objects.c | Find ctor in `code->functions[]` | `Type.createInstance()` |
| `hlffi_call_static()` | hlffi_values.c | Find in `code->functions[]` | `Reflect.callMethod()` |
| `hlffi_get_static_field()` | hlffi_values.c | Find class, access field | `Reflect.field()` |
| `hlffi_set_static_field()` | hlffi_values.c | Find class, set field | `Reflect.setField()` |
| `hlffi_call_entry()` | hlffi_lifecycle.c | `code->entrypoint` lookup | `hl_entry_point()` direct |
| `hlffi_load_file()` | hlffi_lifecycle.c | Parse & JIT bytecode | No-op (code is linked) |
| `hlffi_load_memory()` | hlffi_lifecycle.c | Parse & JIT bytecode | No-op (code is linked) |
| `find_type_by_name()` | hlffi_objects.c | Scan `code->types[]` | Use cached Type class |
| `hlffi_cached_call_init()` | hlffi_cache.c | Find function ptr | Use reflection |

### Category 2: Functions That Work Unchanged

These use runtime APIs (`hl_dyn_*`) that work identically in both modes:

| Function | File | Why It Works |
|----------|------|--------------|
| `hlffi_get_field()` | hlffi_objects.c | Uses `hl_dyn_getp()` on object |
| `hlffi_set_field()` | hlffi_objects.c | Uses `hl_dyn_setp()` on object |
| `hlffi_call_method()` | hlffi_objects.c | Uses `hl_dyn_call_safe()` |
| `hlffi_is_instance_of()` | hlffi_objects.c | Compares `obj->t` |
| `hlffi_value_int/float/bool/string()` | hlffi_values.c | Creates vdynamic |
| `hlffi_value_as_int/float/bool/string()` | hlffi_values.c | Reads vdynamic |
| `hlffi_value_free()` | hlffi_values.c | Frees wrapper |
| All `hlffi_get_field_*()` | hlffi_objects.c | Wrapper around hlffi_get_field |
| All `hlffi_set_field_*()` | hlffi_objects.c | Wrapper around hlffi_set_field |
| All `hlffi_call_method_*()` | hlffi_objects.c | Wrapper around hlffi_call_method |
| `hlffi_process_events()` | hlffi_events.c | Uses runtime event APIs |
| `hlffi_enum_*()` | hlffi_enums.c | Uses hl_type metadata |
| `hlffi_abstract_*()` | hlffi_abstracts.c | Uses hl_type metadata |

### Category 3: Functions That Must Be Disabled

These features are impossible in HLC:

| Function | File | HLC Behavior |
|----------|------|--------------|
| `hlffi_enable_hot_reload()` | hlffi_reload.c | Return `HLFFI_ERROR_RELOAD_NOT_SUPPORTED` |
| `hlffi_reload_module()` | hlffi_reload.c | Return `HLFFI_ERROR_RELOAD_NOT_SUPPORTED` |
| `hlffi_reload_module_memory()` | hlffi_reload.c | Return `HLFFI_ERROR_RELOAD_NOT_SUPPORTED` |
| `hlffi_check_reload()` | hlffi_reload.c | Return `false` |
| `hlffi_is_hot_reload_enabled()` | hlffi_reload.c | Return `false` |

---

## Implementation Details

### 1. HLC Mode Detection and Cache

Add to `hlffi_internal.h`:

```c
/*============================================================================
 * HLC Mode Support
 *============================================================================*/

#ifdef HLFFI_HLC_MODE
    #define HLFFI_HAS_BYTECODE 0
#else
    #define HLFFI_HAS_BYTECODE 1
#endif

#ifdef HLFFI_HLC_MODE

/**
 * Cached references for HLC operations.
 * Initialized once after hlffi_call_entry().
 */
typedef struct {
    /* Type class access */
    hl_type* type_class;           /* hl_type for Type class */
    vdynamic* type_global;         /* Type class global instance */

    /* Reflect class access */
    hl_type* reflect_class;        /* hl_type for Reflect class */
    vdynamic* reflect_global;      /* Reflect class global instance */

    /* Pre-computed hashes */
    int hash_resolveClass;
    int hash_createInstance;
    int hash_field;
    int hash_setField;
    int hash_callMethod;
    int hash_allTypes;
    int hash_values;
    int hash___type__;
    int hash___constructor__;

    bool initialized;
} hlffi_hlc_cache;

/* Global HLC cache - initialized by hlffi_hlc_init() */
extern hlffi_hlc_cache g_hlc;

/**
 * Initialize HLC support.
 * Called automatically by functions that need it, after entry point.
 */
hlffi_error_code hlffi_hlc_init(hlffi_vm* vm);

/**
 * Check if HLC is initialized.
 */
static inline bool hlffi_hlc_is_ready(void) {
    return g_hlc.initialized;
}

#endif /* HLFFI_HLC_MODE */
```

### 2. HLC Initialization Implementation

Create `hlffi_hlc.c`:

```c
#include "hlffi_internal.h"

#ifdef HLFFI_HLC_MODE

/* Global HLC cache */
hlffi_hlc_cache g_hlc = {0};

hlffi_error_code hlffi_hlc_init(hlffi_vm* vm) {
    if (g_hlc.initialized) return HLFFI_OK;

    if (!vm || !vm->entry_called) {
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    HLFFI_UPDATE_STACK_TOP();

    /*=== Access Type class via known HLC symbol ===*/
    extern hl_type t$Type;
    g_hlc.type_class = &t$Type;

    if (!g_hlc.type_class->obj || !g_hlc.type_class->obj->global_value) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "Type class not initialized");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    g_hlc.type_global = *(vdynamic**)g_hlc.type_class->obj->global_value;
    if (!g_hlc.type_global) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "Type class global is NULL");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /*=== Access Reflect class ===*/
    extern hl_type t$Reflect;
    g_hlc.reflect_class = &t$Reflect;

    if (g_hlc.reflect_class->obj && g_hlc.reflect_class->obj->global_value) {
        g_hlc.reflect_global = *(vdynamic**)g_hlc.reflect_class->obj->global_value;
    }

    /*=== Pre-compute hashes for performance ===*/
    g_hlc.hash_resolveClass = hl_hash_utf8("resolveClass");
    g_hlc.hash_createInstance = hl_hash_utf8("createInstance");
    g_hlc.hash_field = hl_hash_utf8("field");
    g_hlc.hash_setField = hl_hash_utf8("setField");
    g_hlc.hash_callMethod = hl_hash_utf8("callMethod");
    g_hlc.hash_allTypes = hl_hash_utf8("allTypes");
    g_hlc.hash_values = hl_hash_utf8("values");
    g_hlc.hash___type__ = hl_hash_utf8("__type__");
    g_hlc.hash___constructor__ = hl_hash_utf8("__constructor__");

    g_hlc.initialized = true;
    return HLFFI_OK;
}

#endif /* HLFFI_HLC_MODE */
```

### 3. Type Lookup (hlffi_find_type)

```c
hlffi_type* hlffi_find_type(hlffi_vm* vm, const char* name) {
    if (!vm || !name) return NULL;

#ifdef HLFFI_HLC_MODE
    /*=== HLC: Use Type.resolveClass() ===*/

    if (!g_hlc.initialized && hlffi_hlc_init(vm) != HLFFI_OK) {
        return NULL;
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Get resolveClass method */
    vclosure* resolve = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_resolveClass, &hlt_dyn);
    if (!resolve) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Type.resolveClass not found");
        return NULL;
    }

    /* Create String argument */
    vdynamic* name_str = hlffi_create_hl_string(name);
    if (!name_str) return NULL;

    /* Call resolveClass(name) */
    vdynamic* args[1] = { name_str };
    bool isExc = false;
    vdynamic* cls = hl_dyn_call_safe(resolve, args, 1, &isExc);

    if (isExc || !cls) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Type not found: %s", name);
        hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, buf);
        return NULL;
    }

    /* Extract __type__ from Class (BaseType.__type__ is hl_type*) */
    hl_type* result = (hl_type*)hl_dyn_getp(cls, g_hlc.hash___type__, &hlt_dyn);
    return (hlffi_type*)result;

#else
    /*=== JIT: Existing code->types[] scan ===*/

    if (!vm->module || !vm->module->code) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "VM not initialized");
        return NULL;
    }

    hl_code* code = vm->module->code;
    int target_hash = hl_hash_utf8(name);

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;

        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char* tname = hl_to_utf8(t->obj->name);
            if (tname && hl_hash_utf8(tname) == target_hash) {
                return (hlffi_type*)t;
            }
        }
        else if (t->kind == HENUM && t->tenum && t->tenum->name) {
            char* tname = hl_to_utf8(t->tenum->name);
            if (tname && hl_hash_utf8(tname) == target_hash) {
                return (hlffi_type*)t;
            }
        }
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "Type not found: %s", name);
    hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, buf);
    return NULL;

#endif
}
```

### 4. Object Creation (hlffi_new)

```c
hlffi_value* hlffi_new(hlffi_vm* vm, const char* class_name,
                       int argc, hlffi_value** argv) {
    if (!vm || !class_name) return NULL;

    if (!vm->entry_called) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "Entry point must be called first");
        return NULL;
    }

    HLFFI_UPDATE_STACK_TOP();

#ifdef HLFFI_HLC_MODE
    /*=== HLC: Use Type.createInstance() ===*/

    if (!g_hlc.initialized && hlffi_hlc_init(vm) != HLFFI_OK) {
        return NULL;
    }

    /* First resolve the class */
    vclosure* resolve = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_resolveClass, &hlt_dyn);

    vdynamic* name_str = hlffi_create_hl_string(class_name);
    vdynamic* resolve_args[1] = { name_str };
    bool isExc = false;
    vdynamic* cls = hl_dyn_call_safe(resolve, resolve_args, 1, &isExc);

    if (isExc || !cls) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Class not found: %s", class_name);
        hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, buf);
        return NULL;
    }

    /* Get createInstance method */
    vclosure* create = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_createInstance, &hlt_dyn);

    if (!create) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Type.createInstance not found");
        return NULL;
    }

    /* Build args array for Haxe */
    varray* args_array = (varray*)hl_alloc_array(&hlt_dyn, argc);
    for (int i = 0; i < argc; i++) {
        hl_aptr(args_array, vdynamic*)[i] = argv[i] ? argv[i]->hl_value : NULL;
    }

    /* Call createInstance(cls, args) */
    vdynamic* create_args[2] = { cls, (vdynamic*)args_array };
    isExc = false;
    vdynamic* instance = hl_dyn_call_safe(create, create_args, 2, &isExc);

    if (isExc) {
        hlffi_set_error(vm, HLFFI_ERROR_EXCEPTION_THROWN,
            "Exception in constructor");
        return NULL;
    }

    /* Wrap result */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    wrapped->hl_value = instance;
    wrapped->is_rooted = true;
    hl_add_root(&wrapped->hl_value);

    return wrapped;

#else
    /*=== JIT: Existing implementation ===*/
    /* ... existing hlffi_new code ... */
#endif
}
```

### 5. Static Method Calls (hlffi_call_static)

```c
hlffi_value* hlffi_call_static(hlffi_vm* vm, const char* class_name,
                               const char* method_name,
                               int argc, hlffi_value** argv) {
    if (!vm || !class_name || !method_name) return NULL;

    HLFFI_UPDATE_STACK_TOP();

#ifdef HLFFI_HLC_MODE
    /*=== HLC: Use Reflect.callMethod() ===*/

    if (!g_hlc.initialized && hlffi_hlc_init(vm) != HLFFI_OK) {
        return NULL;
    }

    /* Resolve class first */
    vclosure* resolve = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_resolveClass, &hlt_dyn);

    vdynamic* class_str = hlffi_create_hl_string(class_name);
    vdynamic* resolve_args[1] = { class_str };
    bool isExc = false;
    vdynamic* cls = hl_dyn_call_safe(resolve, resolve_args, 1, &isExc);

    if (isExc || !cls) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Class not found: %s", class_name);
        hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, buf);
        return NULL;
    }

    /* Get the static method via Reflect.field(cls, methodName) */
    vclosure* field_fn = (vclosure*)hl_dyn_getp(
        g_hlc.reflect_global, g_hlc.hash_field, &hlt_dyn);

    vdynamic* method_str = hlffi_create_hl_string(method_name);
    vdynamic* field_args[2] = { cls, method_str };
    isExc = false;
    vdynamic* method = hl_dyn_call_safe(field_fn, field_args, 2, &isExc);

    if (isExc || !method) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Method not found: %s.%s",
                 class_name, method_name);
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND, buf);
        return NULL;
    }

    /* Call Reflect.callMethod(null, method, args) for static call */
    vclosure* call_fn = (vclosure*)hl_dyn_getp(
        g_hlc.reflect_global, g_hlc.hash_callMethod, &hlt_dyn);

    varray* args_array = (varray*)hl_alloc_array(&hlt_dyn, argc);
    for (int i = 0; i < argc; i++) {
        hl_aptr(args_array, vdynamic*)[i] = argv[i] ? argv[i]->hl_value : NULL;
    }

    vdynamic* call_args[3] = { NULL, method, (vdynamic*)args_array };
    isExc = false;
    vdynamic* result = hl_dyn_call_safe(call_fn, call_args, 3, &isExc);

    if (isExc) {
        hlffi_set_error(vm, HLFFI_ERROR_EXCEPTION_THROWN,
            "Exception in static method");
        return NULL;
    }

    /* Wrap result */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    wrapped->hl_value = result;
    wrapped->is_rooted = false;

    return wrapped;

#else
    /*=== JIT: Existing implementation ===*/
    /* ... existing hlffi_call_static code ... */
#endif
}
```

### 6. Entry Point (hlffi_call_entry)

```c
hlffi_error_code hlffi_call_entry(hlffi_vm* vm) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

#ifdef HLFFI_HLC_MODE
    /*=== HLC: Call hl_entry_point() directly ===*/

    if (!vm->hl_initialized) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "VM not initialized");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /* In HLC, entry point is a known symbol */
    extern void hl_entry_point(void);

    /* Setup closure for safe call */
    hl_type_fun tf = { 0 };
    hl_type clt = { 0 };
    vclosure cl = { 0 };

    tf.ret = &hlt_void;
    clt.kind = HFUN;
    clt.fun = &tf;
    cl.t = &clt;
    cl.fun = hl_entry_point;

    bool isExc = false;
    vdynamic* ret = hl_dyn_call_safe(&cl, NULL, 0, &isExc);

    if (isExc) {
        hlffi_set_error(vm, HLFFI_ERROR_EXCEPTION_THROWN,
            "Exception in entry point");
        hl_print_uncaught_exception(ret);
        return HLFFI_ERROR_EXCEPTION_THROWN;
    }

    vm->entry_called = true;
    vm->module_loaded = true;  /* In HLC, code is always "loaded" */

    return HLFFI_OK;

#else
    /*=== JIT: Existing implementation ===*/

    if (!vm->module_loaded) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "No module loaded");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    hl_code* code = vm->module->code;
    int entry_index = code->entrypoint;

    vclosure cl;
    cl.t = code->functions[vm->module->functions_indexes[entry_index]].type;
    cl.fun = vm->module->functions_ptrs[entry_index];
    cl.hasValue = 0;

    bool isExc = false;
    vdynamic* ret = hl_dyn_call_safe(&cl, NULL, 0, &isExc);

    if (isExc) {
        hlffi_set_error(vm, HLFFI_ERROR_EXCEPTION_THROWN,
            "Exception in entry point");
        hl_print_uncaught_exception(ret);
        return HLFFI_ERROR_EXCEPTION_THROWN;
    }

    vm->entry_called = true;
    return HLFFI_OK;

#endif
}
```

### 7. Module Loading (hlffi_load_file)

```c
hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path) {
#ifdef HLFFI_HLC_MODE
    /*=== HLC: No loading needed, code is statically linked ===*/

    if (!vm) return HLFFI_ERROR_NULL_VM;

    /* In HLC mode, just mark as loaded */
    vm->module_loaded = true;
    vm->loaded_file = path;  /* Store for reference */

    return HLFFI_OK;

#else
    /*=== JIT: Existing bytecode loading ===*/
    /* ... existing implementation ... */
#endif
}

hlffi_error_code hlffi_load_memory(hlffi_vm* vm, const void* data, size_t size) {
#ifdef HLFFI_HLC_MODE
    /* HLC: No loading from memory */
    (void)data;
    (void)size;

    if (!vm) return HLFFI_ERROR_NULL_VM;
    vm->module_loaded = true;

    return HLFFI_OK;

#else
    /*=== JIT: Existing implementation ===*/
    /* ... existing implementation ... */
#endif
}
```

### 8. Hot Reload (Disabled in HLC)

```c
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable) {
#ifdef HLFFI_HLC_MODE
    (void)enable;
    if (!vm) return HLFFI_ERROR_NULL_VM;
    hlffi_set_error(vm, HLFFI_ERROR_RELOAD_NOT_SUPPORTED,
        "Hot reload not supported in HLC mode");
    return HLFFI_ERROR_RELOAD_NOT_SUPPORTED;
#else
    /* ... existing implementation ... */
#endif
}

hlffi_error_code hlffi_reload_module(hlffi_vm* vm, const char* path) {
#ifdef HLFFI_HLC_MODE
    (void)path;
    if (!vm) return HLFFI_ERROR_NULL_VM;
    return HLFFI_ERROR_RELOAD_NOT_SUPPORTED;
#else
    /* ... existing implementation ... */
#endif
}

bool hlffi_is_hot_reload_enabled(hlffi_vm* vm) {
#ifdef HLFFI_HLC_MODE
    (void)vm;
    return false;
#else
    return vm && vm->hot_reload_enabled;
#endif
}

bool hlffi_check_reload(hlffi_vm* vm) {
#ifdef HLFFI_HLC_MODE
    (void)vm;
    return false;
#else
    /* ... existing implementation ... */
#endif
}
```

---

## New Functions for HLC

### Helper: Create Haxe String

```c
/**
 * Create a Haxe String from a C string.
 * Used internally for calling Haxe methods.
 */
static vdynamic* hlffi_create_hl_string(const char* str) {
    if (!str) return NULL;

    int len = (int)strlen(str);
    uchar* utf16 = hl_from_utf8((vbyte*)str);

    /* Allocate String object */
    vstring* s = (vstring*)hl_alloc_obj(&hlt_bytes);
    s->bytes = (vbyte*)utf16;
    s->length = len;

    return (vdynamic*)s;
}
```

### Query: Is HLC Mode

```c
/**
 * Check if running in HLC mode.
 */
bool hlffi_is_hlc_mode(void) {
#ifdef HLFFI_HLC_MODE
    return true;
#else
    return false;
#endif
}
```

### Query: Features Available

```c
/**
 * Check if hot reload is available (false in HLC).
 */
bool hlffi_hot_reload_available(void) {
#ifdef HLFFI_HLC_MODE
    return false;
#else
    return true;
#endif
}

/**
 * Check if bytecode loading is available (false in HLC).
 */
bool hlffi_bytecode_loading_available(void) {
#ifdef HLFFI_HLC_MODE
    return false;
#else
    return true;
#endif
}
```

---

## Build System Integration

### CMake Configuration

```cmake
# CMakeLists.txt additions

option(HLFFI_HLC_MODE "Build for HLC (HashLink/C) instead of JIT" OFF)

if(HLFFI_HLC_MODE)
    add_definitions(-DHLFFI_HLC_MODE)
    message(STATUS "HLFFI: Building for HLC mode")

    # Disable features not available in HLC
    set(HLFFI_ENABLE_HOT_RELOAD OFF CACHE BOOL "" FORCE)

    # HLC requires these additional sources
    list(APPEND HLFFI_SOURCES src/hlffi_hlc.c)
else()
    message(STATUS "HLFFI: Building for JIT mode")
endif()

# Source files
set(HLFFI_SOURCES
    src/hlffi_lifecycle.c
    src/hlffi_types.c
    src/hlffi_values.c
    src/hlffi_objects.c
    src/hlffi_events.c
    src/hlffi_callbacks.c
    src/hlffi_enums.c
    src/hlffi_abstracts.c
    src/hlffi_maps.c
    src/hlffi_bytes.c
    src/hlffi_cache.c
    src/hlffi_integration.c
    src/hlffi_threading.c
)

if(NOT HLFFI_HLC_MODE)
    list(APPEND HLFFI_SOURCES src/hlffi_reload.c)
endif()

if(HLFFI_HLC_MODE)
    list(APPEND HLFFI_SOURCES src/hlffi_hlc.c)
endif()
```

### Building for Different Targets

**JIT Mode (default):**
```bash
cmake -B build -DHASHLINK_DIR="/path/to/hashlink"
cmake --build build
```

**HLC Mode:**
```bash
cmake -B build-hlc \
    -DHLFFI_HLC_MODE=ON \
    -DHASHLINK_DIR="/path/to/hashlink"
cmake --build build-hlc
```

**Cross-compile for ARM:**
```bash
cmake -B build-arm \
    -DHLFFI_HLC_MODE=ON \
    -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake \
    -DHASHLINK_DIR="/path/to/hashlink"
cmake --build build-arm
```

---

## Usage Guide

### JIT Mode (Existing Workflow)

```c
#include "hlffi.h"

int main(int argc, char** argv) {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, argc, argv);

    // Load bytecode
    hlffi_load_file(vm, "game.hl");

    // Run entry point
    hlffi_call_entry(vm);

    // Use HLFFI APIs...
    hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);

    hlffi_destroy(vm);
    return 0;
}
```

### HLC Mode (New Workflow)

```c
#define HLFFI_HLC_MODE  // Or set via compiler flag
#include "hlffi.h"

int main(int argc, char** argv) {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, argc, argv);

    // No load needed - code is statically linked!
    // hlffi_load_file() is a no-op in HLC mode

    // Run entry point (calls hl_entry_point)
    hlffi_call_entry(vm);

    // Use HLFFI APIs - works identically!
    hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);

    hlffi_destroy(vm);
    return 0;
}
```

### Compiling HLC Application

```bash
# 1. Compile Haxe to C
haxe -hl output/main.c -main Main

# 2. Compile everything together
gcc -o myapp \
    -DHLFFI_HLC_MODE \
    -I/path/to/hlffi/include \
    -I/path/to/hashlink/src \
    your_app.c \
    output/main.c \
    output/hl/*.c \
    /path/to/hashlink/src/hlc_main.c \
    /path/to/hlffi/lib/libhlffi_hlc.a \
    -lhl -lm -lpthread
```

---

## Testing

### Compile-Time Mode Check

```c
#include "hlffi.h"
#include <stdio.h>

int main() {
    printf("HLFFI Mode: %s\n", hlffi_is_hlc_mode() ? "HLC" : "JIT");
    printf("Hot reload available: %s\n",
           hlffi_hot_reload_available() ? "yes" : "no");
    printf("Bytecode loading available: %s\n",
           hlffi_bytecode_loading_available() ? "yes" : "no");
    return 0;
}
```

### Functionality Test

```c
void test_hlffi(void) {
    hlffi_vm* vm = hlffi_create();
    assert(vm != NULL);

    hlffi_init(vm, 0, NULL);

    #ifndef HLFFI_HLC_MODE
    hlffi_load_file(vm, "test.hl");
    #endif

    hlffi_error_code err = hlffi_call_entry(vm);
    assert(err == HLFFI_OK);

    // Type lookup should work in both modes
    hlffi_type* t = hlffi_find_type(vm, "TestClass");
    assert(t != NULL);

    // Object creation should work in both modes
    hlffi_value* obj = hlffi_new(vm, "TestClass", 0, NULL);
    assert(obj != NULL);

    // Field access should work in both modes
    hlffi_set_field_int(vm, obj, "value", 42);
    int val = hlffi_get_field_int(obj, "value", 0);
    assert(val == 42);

    hlffi_value_free(obj);
    hlffi_destroy(vm);

    printf("All tests passed!\n");
}
```

---

## Summary

### Functions by Category

| Category | Count | Examples |
|----------|-------|----------|
| Need HLC alternative | 11 | `hlffi_find_type`, `hlffi_new`, `hlffi_call_static` |
| Work unchanged | 25+ | `hlffi_get_field`, `hlffi_call_method`, `hlffi_value_*` |
| Disabled in HLC | 5 | `hlffi_reload_*`, `hlffi_enable_hot_reload` |
| New for HLC | 4 | `hlffi_hlc_init`, `hlffi_is_hlc_mode`, etc. |

### Key Implementation Points

1. **Compile-time switch**: `#ifdef HLFFI_HLC_MODE`
2. **Bootstrap**: `extern hl_type t$Type` and `extern hl_type t$Reflect`
3. **Type lookup**: `Type.resolveClass(name)` → `BaseType.__type__`
4. **Object creation**: `Type.createInstance(cls, args)`
5. **Static calls**: `Reflect.field()` + `Reflect.callMethod()`
6. **Entry point**: Direct `hl_entry_point()` call
7. **Caching**: Pre-compute hashes, cache class references

### Files to Modify/Create

| File | Action |
|------|--------|
| `hlffi_internal.h` | Add HLC cache structure and macros |
| `hlffi_hlc.c` | **NEW** - HLC initialization |
| `hlffi_types.c` | Add HLC path to `hlffi_find_type`, `hlffi_list_types` |
| `hlffi_objects.c` | Add HLC path to `hlffi_new`, `find_type_by_name` |
| `hlffi_values.c` | Add HLC path to static field/method functions |
| `hlffi_lifecycle.c` | Add HLC path to `hlffi_call_entry`, `hlffi_load_*` |
| `hlffi_reload.c` | Return errors in HLC mode |
| `hlffi_cache.c` | Add HLC path for cached calls |
| `CMakeLists.txt` | Add `HLFFI_HLC_MODE` option |

---

## References

- [HashLink/C Compilation](https://haxe.org/manual/target-hl-c-compilation.html)
- [HashLink GitHub](https://github.com/HaxeFoundation/hashlink)
- [Working with C in HashLink](https://aramallo.com/blog/hashlink/calling-c.html)
- [Haxe Type API](https://api.haxe.org/Type.html)
- [Haxe Reflect API](https://api.haxe.org/Reflect.html)
