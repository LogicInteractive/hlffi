# Phase 4: Instance Members Implementation Notes

**Date**: 2025-11-27
**Status**: Complete
**File**: `src/hlffi_objects.c`

---

## Overview

Phase 4 implements full instance member support for HLFFI:
- Creating class instances with `hlffi_new()`
- Reading/writing instance fields
- Calling instance methods
- GC root management for object lifetime

---

## Key Findings

### 1. Constructor Location in HashLink Bytecode

Haxe constructors are **not** stored as a method named "new" on the class. Instead:

- **Constructor function name**: `$ClassName.__constructor__`
- **Location**: In `module->code->functions` array
- **The `$` prefix**: Indicates the Class type's static members (not the instance)

Example for `Player` class:
```
$Player.__constructor__   // Constructor function
$Player.create            // Static factory method
$Player.main              // Static main method
Player.takeDamage         // Instance method (no $ prefix)
```

### 2. Function Index vs Array Index

**Critical discovery**: The array position `i` in `code->functions[i]` is NOT the function pointer index.

```c
// WRONG - uses array position
void* fn = vm->module->functions_ptrs[i];

// CORRECT - uses f->findex
hl_function* f = &code->functions[i];
void* fn = vm->module->functions_ptrs[f->findex];
```

The `f->findex` field contains the actual index into `functions_ptrs` where the JIT-compiled function pointer lives.

### 3. Constructor Calling Convention

HashLink constructors are JIT-compiled methods with signature:
```
(this: ClassName) -> Void
```

**For no-arg constructors**, call directly via function pointer:
```c
typedef void (*ctor_fn)(vdynamic*);
ctor_fn fn = (ctor_fn)ctor_func;
fn((vdynamic*)instance);
```

**Do NOT use `hl_dyn_call_safe`** for simple constructors - it has different calling conventions for closures/dynamic dispatch and causes access violations.

For constructors with arguments, `hl_dyn_call_safe` can be used with a properly constructed `vclosure`.

### 4. hl_function Union Structure

The `hl_function` struct has a union for `field`:
```c
typedef struct {
    hl_type *type;
    int findex;
    int nregs;
    int nops;
    hl_opcode *ops;
    int *debug;
    union {
        struct {
            hl_type_obj *obj;      // When field.name exists
            const uchar *name;
        };
        hl_native *native;        // For native functions
    } field;
    int ref;
} hl_function;
```

Access via helper macros:
```c
#define fun_obj(f) ((f)->field.obj)
#define fun_field_name(f) ((f)->field.name)
```

### 5. Field Access Implementation

Fields are accessed via `hl_obj_lookup`:
```c
int field_hash = hl_hash_utf8(field_name);
hl_field_lookup* lookup = hl_obj_lookup(instance, field_hash, &rt);
```

The lookup returns field offset and type for reading/writing.

**Reading**: Use `hl_obj_get(instance, lookup)`
**Writing**: Use `hl_obj_set(instance, lookup, value)` (specific to type)

### 6. Method Resolution

Methods are resolved via `obj_resolve_field`:
```c
vclosure* method = (vclosure*)obj_resolve_field((vdynamic*)instance, field_hash, &rt);
```

This returns a closure that can be called with `hl_dyn_call_safe`.

---

## API Summary

### Instance Creation
```c
// Create instance (calls constructor automatically)
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);

// Free when done (removes GC root)
hlffi_value_free(player);
```

### Field Access - Low Level
```c
// Read field (returns hlffi_value* - must free wrapper)
hlffi_value* health = hlffi_get_field(player, "health");
int hp = hlffi_value_as_int(health, 0);
free(health);

// Write field
hlffi_value* new_health = hlffi_value_int(vm, 50);
hlffi_set_field(player, "health", new_health);
hlffi_value_free(new_health);
```

### Field Access - Convenience API (Recommended)
```c
// Read - no manual free needed for primitives
int hp = hlffi_get_field_int(player, "health", -1);
float dmg = hlffi_get_field_float(player, "damage", 0.0f);
bool alive = hlffi_get_field_bool(player, "isAlive", false);
char* name = hlffi_get_field_string(player, "name");  // must free()

// Write
hlffi_set_field_int(vm, player, "health", 100);
hlffi_set_field_float(vm, player, "damage", 25.5f);
hlffi_set_field_bool(vm, player, "isAlive", true);
hlffi_set_field_string(vm, player, "name", "Hero");
```

### Method Calls - Low Level
```c
hlffi_value* result = hlffi_call_method(player, "getHealth", 0, NULL);
int hp = hlffi_value_as_int(result, 0);
free(result);
```

### Method Calls - Convenience API (Recommended)
```c
// Void method
hlffi_call_method_void(player, "takeDamage", 1, &damage_arg);

// Methods with return values
int hp = hlffi_call_method_int(player, "getHealth", 0, NULL, -1);
float f = hlffi_call_method_float(player, "getSpeed", 0, NULL, 0.0f);
bool b = hlffi_call_method_bool(player, "isAlive", 0, NULL, false);
char* s = hlffi_call_method_string(player, "getName", 0, NULL);  // must free()
```

---

## Memory Ownership Rules

| Function | Returns | Who Frees? |
|----------|---------|------------|
| `hlffi_new()` | `hlffi_value*` | `hlffi_value_free()` |
| `hlffi_get_field()` | `hlffi_value*` | `free()` (wrapper only) |
| `hlffi_get_field_int/float/bool()` | primitive | nothing |
| `hlffi_get_field_string()` | `char*` | `free()` |
| `hlffi_call_method()` | `hlffi_value*` | `free()` |
| `hlffi_call_method_int/float/bool()` | primitive | nothing |
| `hlffi_call_method_string()` | `char*` | `free()` |

---

## Constructor Search Strategy

The implementation uses two strategies to find constructors:

### Strategy 1: Runtime Bindings
```c
int ctor_hash = hl_hash_utf8("__constructor__");
if (rt && rt->bindings) {
    for (int i = 0; i < rt->nbindings; i++) {
        if (rt->bindings[i].fid == ctor_hash) {
            // Found in bindings
        }
    }
}
```

### Strategy 2: Module Functions (Primary)
```c
char expected_class_name[256];
snprintf(expected_class_name, sizeof(expected_class_name), "$%s", class_name);

for (int i = 0; i < code->nfunctions; i++) {
    hl_function* f = &code->functions[i];
    hl_type_obj* fobj = fun_obj(f);
    const uchar* fname = fun_field_name(f);

    // Convert to UTF-8 and compare
    if (strcmp(obj_name, expected_class_name) == 0 &&
        strcmp(field_name, "__constructor__") == 0) {
        // Found! Use f->findex for function pointer
        ctor_func = vm->module->functions_ptrs[f->findex];
    }
}
```

---

## Type Checking

```c
// Check if value is instance of class
bool is_player = hlffi_is_instance_of(player, "Player");
```

---

## GC Integration

Objects created via `hlffi_new()` are automatically GC-rooted:
- `hl_add_root()` called during creation
- `hl_remove_root()` called during `hlffi_value_free()`

This prevents the GC from collecting objects while C code holds references.

---

## Test Coverage

All functionality verified in `test_instance_basic.c`:
1. Instance creation with constructor
2. Primitive field read (int)
3. String field read
4. Primitive field write
5. Void method call with parameter
6. Method call returning int
7. Method call returning bool
8. Method call returning string
9. Type checking (`is_instance_of`)
10. GC root cleanup

All 10 tests pass.
