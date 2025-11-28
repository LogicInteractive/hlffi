# Phase 5: Complex Data Types - Complete

**Date**: 2025-11-28
**Status**: Complete
**Files**: `src/hlffi_maps.c`, `src/hlffi_bytes.c`, `src/hlffi_enums.c`, `src/hlffi_objects.c`

---

## Overview

Phase 5 implements support for complex Haxe data types in HLFFI:
- **Maps** - `Map<Int, V>`, `Map<String, V>` with full CRUD operations
- **Bytes** - `haxe.io.Bytes` for binary data exchange
- **Enums** - Haxe enums with constructor parameters

All three subsystems are fully tested and working.

---

## Components

### 1. Maps (`hlffi_maps.c`)

Full interoperability with Haxe Maps via instance method calls:

```c
// Get map from Haxe
hlffi_value* map = hlffi_call_static(vm, "MyClass", "getMap", 0, NULL);

// Operations
hlffi_value* val = hlffi_map_get(vm, map, key);
hlffi_map_set(vm, map, key, value);
bool exists = hlffi_map_exists(vm, map, key);
bool removed = hlffi_map_remove(vm, map, key);

// Iteration
hlffi_value* keys = hlffi_map_keys(vm, map);
hlffi_value* vals = hlffi_map_values(vm, map);
```

**Test**: `test_map_demo.c` + `test/MapTest.hx`

### 2. Bytes (`hlffi_bytes.c`)

Binary data exchange between C and Haxe:

```c
// Create bytes from C buffer
hlffi_value* bytes = hlffi_bytes_from_data(vm, data, length);

// Get raw pointer for zero-copy access
uint8_t* ptr = hlffi_bytes_get_data(bytes);
int len = hlffi_bytes_get_length(bytes);

// Modify in place
ptr[0] = 0xFF;

// Pass to Haxe - changes visible immediately
hlffi_value* args[] = {bytes};
hlffi_call_static(vm, "MyClass", "processBytes", 1, args);
```

**Test**: `test_bytes_demo.c` + `test/BytesTest.hx`

### 3. Enums (`hlffi_enums.c`)

Access Haxe enum values and their constructor parameters:

```c
// Get enum from Haxe
hlffi_value* state = hlffi_call_static(vm, "MyClass", "getState", 0, NULL);

// Check enum constructor name
char* name = hlffi_enum_get_constructor(state);  // e.g., "Running"
free(name);

// Access constructor parameters
hlffi_value* param = hlffi_enum_get_param(state, 0);
int speed = hlffi_value_as_int(param, 0);
hlffi_value_free(param);
```

**Test**: `test_enum_demo.c` + `test/EnumTest.hx`

---

## Bug Fix: StringMap Key Encoding

### Issue

`StringMap.exists()` (and other string-keyed operations) always returned `false` for existing keys, while `IntMap` worked correctly.

### Root Cause

`hlffi_value_string()` creates strings with type `HBYTES`, but HashLink's StringMap methods expect arguments of type `HOBJ` (String object). When `hlffi_call_method()` passed these HBYTES strings to the map, the type mismatch caused lookups to fail.

### Fix

Added type conversion in `hlffi_call_method()` (`src/hlffi_objects.c:556-575`) to convert HBYTES arguments to String objects when the method signature expects an HOBJ of type "String":

```c
/* TYPE CONVERSION: Convert HBYTES to String objects if method expects HOBJ String */
if (argc > 0 && method->t->kind == HFUN) {
    for (int i = 0; i < argc && i < method->t->fun->nargs; i++) {
        hl_type* expected_type = method->t->fun->args[i];
        vdynamic* arg = hl_args[i];

        if (arg && expected_type->kind == HOBJ && arg->t->kind == HBYTES) {
            char type_name_buf[128];
            if (expected_type->obj && expected_type->obj->name) {
                utostr(type_name_buf, sizeof(type_name_buf), expected_type->obj->name);
                if (strcmp(type_name_buf, "String") == 0) {
                    vstring* bytes_str = (vstring*)arg;
                    bytes_str->t = expected_type;
                    hl_args[i] = (vdynamic*)bytes_str;
                }
            }
        }
    }
}
```

This mirrors the existing conversion code in `hlffi_call_static()`.

### Verification

All map operations now work correctly for both IntMap and StringMap:

| Test | Before Fix | After Fix |
|------|------------|-----------|
| IntMap.get(2) | "two" | "two" |
| IntMap.exists(2) | true | true |
| IntMap.exists(99) | false | false |
| StringMap.get("b") | 20 | 20 |
| StringMap.exists("b") | **false** | **true** |

---

## Test Results

All Phase 5 tests pass:

```
test_map_demo.exe    - PASS (IntMap, StringMap get/set/exists)
test_bytes_demo.exe  - PASS (create, modify, pass to Haxe)
test_enum_demo.exe   - PASS (constructor name, parameters)
```

---

## API Summary

### Map Operations
| Function | Description |
|----------|-------------|
| `hlffi_map_get(vm, map, key)` | Get value for key |
| `hlffi_map_set(vm, map, key, val)` | Set key-value pair |
| `hlffi_map_exists(vm, map, key)` | Check if key exists |
| `hlffi_map_remove(vm, map, key)` | Remove key |
| `hlffi_map_keys(vm, map)` | Get key iterator |
| `hlffi_map_values(vm, map)` | Get value iterator |

### Bytes Operations
| Function | Description |
|----------|-------------|
| `hlffi_bytes_from_data(vm, ptr, len)` | Create Bytes from C buffer |
| `hlffi_bytes_get_data(bytes)` | Get raw data pointer |
| `hlffi_bytes_get_length(bytes)` | Get byte length |

### Enum Operations
| Function | Description |
|----------|-------------|
| `hlffi_enum_get_constructor(val)` | Get constructor name |
| `hlffi_enum_get_param(val, idx)` | Get constructor parameter |
| `hlffi_enum_get_index(val)` | Get enum index |

---

## Memory Management

| Type | Allocation | Cleanup |
|------|------------|---------|
| Map values | GC-managed | `hlffi_value_free()` wrapper |
| Bytes data | GC-managed | `hlffi_value_free()` wrapper |
| Enum values | GC-managed | `hlffi_value_free()` wrapper |
| `char*` returns | `strdup()` | `free()` |

---

## Known Limitations

1. **Map Creation**: Maps must be created in Haxe, not C
2. **Map Size**: No direct size query - use `Lambda.count()` in Haxe
3. **Enum Creation**: Enums must be created in Haxe, not C

---

## See Also

- [MAPS_GUIDE.md](MAPS_GUIDE.md) - Detailed map usage guide
- [PHASE4_INSTANCE_MEMBERS.md](PHASE4_INSTANCE_MEMBERS.md) - Instance/method infrastructure
- `test/MapTest.hx`, `test/BytesTest.hx`, `test/EnumTest.hx` - Haxe test classes
