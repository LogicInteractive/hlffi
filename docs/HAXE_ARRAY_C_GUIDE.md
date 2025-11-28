# Haxe Arrays in C Native Extensions - Complete Guide

**Author:** Based on HLFFI Phase 5 Array Implementation
**Date:** November 2025
**HashLink Version:** 1.11+

---

## Executive Summary

Working with Haxe `Array<T>` in C native extensions is **non-trivial** because Haxe arrays are **not simple C arrays**. They are wrapper objects around HashLink's internal `varray` type, with different memory layouts depending on the element type.

**Key Discoveries:**
1. Haxe `Array<T>` ≠ HashLink `varray*` (they're wrappers!)
2. Two wrapper types: `ArrayObj` (pointers) and `ArrayBytes_*` (primitives)
3. Field offsets MUST use `rt->fields_indexes[fid]`, not pointer arithmetic
4. Passing arrays requires proper wrapping/unwrapping

This guide provides battle-tested helper functions and deep technical explanation.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Memory Layout Details](#memory-layout-details)
3. [Helper Functions](#helper-functions)
4. [Usage Examples](#usage-examples)
5. [Common Pitfalls](#common-pitfalls)
6. [Performance Considerations](#performance-considerations)

---

## Architecture Overview

### The Three-Layer Array System

```
┌─────────────────────────────────────────┐
│  Haxe Layer: Array<T>                   │
│  (What Haxe code sees)                  │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│  HashLink Wrapper Layer                 │
│  • ArrayObj (strings, objects)          │
│  • ArrayBytes_Int (i32)                 │
│  • ArrayBytes_F32 (f32)                 │
│  • ArrayBytes_F64 (f64)                 │
│  • ArrayBytes_UI8 (bool)                │
│  • ArrayDyn (mixed types)               │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│  Raw varray Layer                       │
│  (Internal HashLink storage)            │
│  • varray->at = element type            │
│  • varray->size = element count         │
│  • varray data = contiguous memory      │
└─────────────────────────────────────────┘
```

### Why Wrappers Exist

Haxe arrays have **methods** (`push`, `pop`, `slice`) and **properties** (`length`). Raw `varray` is just data. The wrapper objects (HOBJ types) provide the method bindings and property getters.

---

## Memory Layout Details

### ArrayObj (Pointer Types: String, Object, Function)

**Haxe Definition:**
```haxe
// hl.types.ArrayObj
class ArrayObj<T> {
    var array:hl.NativeArray<T>;  // varray*
    public var length(get, never):Int;
}
```

**C Memory Layout:**
```c
struct ArrayObj {
    hl_type* t;              // Type header (vobj base)
    // Field offset determined by rt->fields_indexes[0]
    varray* array;           // The actual array data
    // Method bindings (set up by hl_alloc_obj)
};
```

**Critical:** The `array` field offset is **NOT** at `sizeof(vobj)`! Use:
```c
hl_runtime_obj* rt = type->obj->rt;
int field_offset = rt->fields_indexes[0];  // Could be 8, 16, or other!
varray** array_field = (varray**)((char*)obj + field_offset);
```

### ArrayBytes_* (Primitive Types: Int, Float, Bool)

**Haxe Definition:**
```haxe
// hl.types.ArrayBytes_Int
class ArrayBytes_Int {
    var bytes:hl.Bytes;  // Pointer to raw data
    var size:Int;        // Element count
    public var length(get, never):Int;
}
```

**C Memory Layout (ACTUAL):**
```c
struct ArrayBytes_Int {
    hl_type* t;          // Type header (vobj base)
    // Memory layout is REORDERED for alignment:
    int size;            // Field [1] in declaration, but FIRST in memory!
    void* bytes;         // Field [0] in declaration, but SECOND in memory!
};
```

**Why the reordering?** HashLink optimizes field layout for alignment. On 64-bit:
- `int size` (4 bytes) + padding (4 bytes) = 8 bytes
- `void* bytes` (8 bytes)

Accessing as `(obj + 1)` gives you `size` first, then `bytes`.

---

## Helper Functions

### Core Conversion Functions

```c
#include <hl.h>

/**
 * Convert a Haxe Array<T> to a raw varray pointer
 *
 * @param haxe_array - The Haxe array object (vdynamic*)
 * @param out_varray - Output pointer to receive the varray*
 * @return true on success, false if not a valid array
 *
 * Example:
 *   varray* arr;
 *   if (haxe_array_to_varray(haxe_arr, &arr)) {
 *       // Use arr->size, hl_aptr(arr, type)
 *   }
 */
static bool haxe_array_to_varray(vdynamic* haxe_array, varray** out_varray) {
    if (!haxe_array || !out_varray) return false;

    /* Unwrap dynamic if needed */
    vdynamic* val = haxe_array;
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    /* Direct varray (rare - usually you get wrapped arrays) */
    if (val->t->kind == HARRAY) {
        *out_varray = (varray*)val;
        return true;
    }

    /* Wrapped array (HOBJ) */
    if (val->t->kind == HOBJ && val->t->obj && val->t->obj->name) {
        char type_name[128];
        utostr(type_name, sizeof(type_name), val->t->obj->name);

        /* Check if it's a HashLink array type */
        if (strncmp(type_name, "hl.types.Array", 14) != 0) {
            return false;  /* Not an array type */
        }

        vobj* obj = (vobj*)val;

        if (strstr(type_name, "ArrayObj")) {
            /* ArrayObj: field [0] = array (varray*)
             * MUST use runtime field index! */
            hl_runtime_obj* rt = val->t->obj->rt;
            if (!rt) rt = hl_get_obj_proto(val->t);
            int field_offset = rt->fields_indexes[0];
            varray** array_field = (varray**)((char*)obj + field_offset);
            *out_varray = *array_field;
            return true;
        } else {
            /* ArrayBytes_*: read bytes pointer
             * Memory layout: [size(int), bytes(ptr)] */
            void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
            void* bytes = *bytes_ptr;

            /* Get size */
            int* size_ptr = (int*)(obj + 1);
            int size = *size_ptr;

            /* Reconstruct varray access (we don't have the actual varray header,
             * but we have the data and size) */
            /* WARNING: This is a synthetic varray - only use for data access! */

            /* For ArrayBytes, you typically want direct data access: */
            /* Return NULL here and handle ArrayBytes separately in caller */
            return false;  /* Caller should use haxe_array_get_bytes instead */
        }
    }

    return false;
}

/**
 * Get direct access to ArrayBytes_* raw data
 *
 * @param haxe_array - The Haxe array object
 * @param out_data - Output pointer to raw data
 * @param out_size - Output size in elements
 * @return true on success, false if not ArrayBytes_*
 *
 * Example:
 *   int* data;
 *   int size;
 *   if (haxe_array_get_bytes(arr, (void**)&data, &size)) {
 *       for (int i = 0; i < size; i++) {
 *           printf("%d ", data[i]);
 *       }
 *   }
 */
static bool haxe_array_get_bytes(vdynamic* haxe_array, void** out_data, int* out_size) {
    if (!haxe_array || !out_data || !out_size) return false;

    vdynamic* val = haxe_array;
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    if (val->t->kind != HOBJ || !val->t->obj || !val->t->obj->name) {
        return false;
    }

    char type_name[128];
    utostr(type_name, sizeof(type_name), val->t->obj->name);

    if (strncmp(type_name, "hl.types.ArrayBytes", 19) != 0) {
        return false;  /* Not ArrayBytes_* */
    }

    vobj* obj = (vobj*)val;

    /* Memory layout: [size(int), bytes(ptr)] */
    int* size_ptr = (int*)(obj + 1);
    *out_size = *size_ptr;

    void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
    *out_data = *bytes_ptr;

    return true;
}

/**
 * Convert a raw varray to a Haxe Array<T>
 *
 * This wraps a varray in the appropriate Haxe array type (ArrayObj or ArrayBytes_*)
 * so it can be passed to Haxe functions.
 *
 * @param vm - HashLink VM/module context (needed for type lookup)
 * @param arr - The varray to wrap
 * @return Wrapped Haxe array (vdynamic*), or NULL on failure
 *
 * IMPORTANT: The returned object is GC-managed. Keep it rooted if storing!
 *
 * Example:
 *   varray* raw = hl_alloc_array(&hlt_i32, 10);
 *   vdynamic* haxe_arr = varray_to_haxe_array(code, raw);
 *   // Pass haxe_arr to Haxe functions
 */
static vdynamic* varray_to_haxe_array(hl_code* code, varray* arr) {
    if (!code || !arr) return NULL;

    /* Determine wrapper type based on element type */
    const char* array_type_name;

    if (!arr->at || arr->at->kind == HDYN) {
        array_type_name = "hl.types.ArrayDyn";
    } else if (arr->at->kind == HI32) {
        array_type_name = "hl.types.ArrayBytes_Int";
    } else if (arr->at->kind == HF32) {
        array_type_name = "hl.types.ArrayBytes_F32";
    } else if (arr->at->kind == HF64) {
        array_type_name = "hl.types.ArrayBytes_F64";
    } else if (arr->at->kind == HBOOL) {
        array_type_name = "hl.types.ArrayBytes_UI8";
    } else {
        /* Pointer types (strings, objects, functions) */
        array_type_name = "hl.types.ArrayObj";
    }

    /* Find the wrapper type */
    int hash = hl_hash_utf8(array_type_name);
    hl_type* array_type = NULL;

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char name_buf[128];
            utostr(name_buf, sizeof(name_buf), t->obj->name);
            if (hl_hash_utf8(name_buf) == hash) {
                array_type = t;
                break;
            }
        }
    }

    if (!array_type) return NULL;

    /* Allocate wrapper object (sets up method bindings) */
    vobj* obj = (vobj*)hl_alloc_obj(array_type);
    if (!obj) return NULL;

    /* Get runtime info */
    hl_runtime_obj* rt = array_type->obj->rt;
    if (!rt) rt = hl_get_obj_proto(array_type);

    /* Fill in fields based on type */
    char name_buf[128];
    utostr(name_buf, sizeof(name_buf), array_type->obj->name);

    if (strstr(name_buf, "ArrayObj")) {
        /* ArrayObj: field [0] = array (varray*)
         * CRITICAL: Use runtime field offset! */
        int field_offset = rt->fields_indexes[0];
        varray** array_field = (varray**)((char*)obj + field_offset);
        *array_field = arr;
    } else {
        /* ArrayBytes_*: fields are [size, bytes]
         * Memory layout: [size(int), bytes(ptr)] */
        int* size_ptr = (int*)(obj + 1);
        *size_ptr = arr->size;

        void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
        *bytes_ptr = hl_aptr(arr, void);
    }

    return (vdynamic*)obj;
}
```

### Convenience Wrappers for Common Types

```c
/**
 * Create a Haxe Array<Int> from C int array
 *
 * Example:
 *   int values[] = {10, 20, 30, 40};
 *   vdynamic* arr = create_haxe_int_array(code, values, 4);
 */
static vdynamic* create_haxe_int_array(hl_code* code, int* values, int count) {
    varray* arr = hl_alloc_array(&hlt_i32, count);
    if (!arr) return NULL;

    int* data = hl_aptr(arr, int);
    for (int i = 0; i < count; i++) {
        data[i] = values[i];
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Create a Haxe Array<Float> from C double array
 */
static vdynamic* create_haxe_float_array(hl_code* code, double* values, int count) {
    varray* arr = hl_alloc_array(&hlt_f64, count);
    if (!arr) return NULL;

    double* data = hl_aptr(arr, double);
    for (int i = 0; i < count; i++) {
        data[i] = values[i];
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Create a Haxe Array<Single> (F32) from C float array
 *
 * IMPORTANT: F32 and F64 are DIFFERENT TYPES - no casting!
 */
static vdynamic* create_haxe_single_array(hl_code* code, float* values, int count) {
    varray* arr = hl_alloc_array(&hlt_f32, count);
    if (!arr) return NULL;

    float* data = hl_aptr(arr, float);
    for (int i = 0; i < count; i++) {
        data[i] = values[i];
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Create a Haxe Array<String> from C string array
 *
 * Example:
 *   const char* strs[] = {"Hello", "World", "From", "C"};
 *   vdynamic* arr = create_haxe_string_array(code, strs, 4);
 */
static vdynamic* create_haxe_string_array(hl_code* code, const char** strings, int count) {
    varray* arr = hl_alloc_array(&hlt_bytes, count);
    if (!arr) return NULL;

    vbyte** data = hl_aptr(arr, vbyte*);
    for (int i = 0; i < count; i++) {
        /* Convert UTF-8 to UTF-16 */
        uchar* utf16 = hl_to_utf16(strings[i]);
        data[i] = (vbyte*)utf16;
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Extract Haxe Array<Int> to C int array
 *
 * @param haxe_array - The Haxe array
 * @param out_values - Output buffer (caller must free!)
 * @param out_count - Output element count
 * @return true on success
 *
 * Example:
 *   int* values;
 *   int count;
 *   if (extract_haxe_int_array(arr, &values, &count)) {
 *       for (int i = 0; i < count; i++) {
 *           printf("%d ", values[i]);
 *       }
 *       free(values);
 *   }
 */
static bool extract_haxe_int_array(vdynamic* haxe_array, int** out_values, int* out_count) {
    int* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return false;
    }

    *out_values = (int*)malloc(size * sizeof(int));
    if (!*out_values) return false;

    memcpy(*out_values, data, size * sizeof(int));
    *out_count = size;
    return true;
}
```

---

## Usage Examples

### Example 1: Native Function Receiving Array<Int>

```c
/* Haxe: function sumArray(arr:Array<Int>):Int */
HL_PRIM int HL_NAME(sum_array)(vdynamic* haxe_array) {
    int* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return 0;  /* Not a valid int array */
    }

    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    return sum;
}
DEFINE_PRIM(_I32, sum_array, _DYN);
```

### Example 2: Native Function Returning Array<Float>

```c
/* Haxe: function generateFloats(count:Int):Array<Float> */
HL_PRIM vdynamic* HL_NAME(generate_floats)(int count) {
    double* values = (double*)malloc(count * sizeof(double));
    for (int i = 0; i < count; i++) {
        values[i] = (double)i * 1.5;
    }

    /* Get hl_code from current module (use hl_get_thread()->current_module) */
    hl_module* m = hl_get_thread()->current_module;
    vdynamic* result = create_haxe_float_array(m->code, values, count);

    free(values);
    return result;
}
DEFINE_PRIM(_DYN, generate_floats, _I32);
```

### Example 3: Modifying Array In-Place

```c
/* Haxe: function doubleValues(arr:Array<Int>):Void */
HL_PRIM void HL_NAME(double_values)(vdynamic* haxe_array) {
    int* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return;  /* Not a valid int array */
    }

    /* Modify in place - changes visible to Haxe */
    for (int i = 0; i < size; i++) {
        data[i] *= 2;
    }
}
DEFINE_PRIM(_VOID, double_values, _DYN);
```

### Example 4: String Array Processing

```c
/* Haxe: function joinStrings(arr:Array<String>):String */
HL_PRIM vbyte* HL_NAME(join_strings)(vdynamic* haxe_array) {
    varray* arr;
    if (!haxe_array_to_varray(haxe_array, &arr)) {
        return (vbyte*)USTR("");
    }

    /* For ArrayObj (strings), we get the varray directly */
    vbyte** strings = hl_aptr(arr, vbyte*);

    /* Calculate total length */
    int total_len = 0;
    for (int i = 0; i < arr->size; i++) {
        total_len += (int)ustrlen((uchar*)strings[i]);
        if (i < arr->size - 1) total_len += 1;  /* Space separator */
    }

    /* Allocate result */
    uchar* result = (uchar*)malloc((total_len + 1) * sizeof(uchar));
    result[0] = 0;

    /* Concatenate */
    for (int i = 0; i < arr->size; i++) {
        ustrcat(result, (uchar*)strings[i]);
        if (i < arr->size - 1) {
            ustrcat(result, USTR(" "));
        }
    }

    return (vbyte*)result;
}
DEFINE_PRIM(_BYTES, join_strings, _DYN);
```

---

## Common Pitfalls

### ❌ WRONG: Hardcoded Field Offsets

```c
/* DON'T DO THIS! */
varray** array_field = (varray**)(obj + 1);  /* WRONG! */
```

**Why it fails:** Field offsets are determined by the runtime based on alignment, padding, and method bindings. They're not always `sizeof(vobj)`.

### ✅ CORRECT: Use Runtime Field Indexes

```c
/* DO THIS! */
hl_runtime_obj* rt = type->obj->rt;
if (!rt) rt = hl_get_obj_proto(type);
int field_offset = rt->fields_indexes[0];
varray** array_field = (varray**)((char*)obj + field_offset);
```

### ❌ WRONG: Casting F32 to F64

```c
/* DON'T DO THIS! */
double value = (double)float_array[i];  /* Type mismatch! */
```

**Why it fails:** Haxe's `Single` (F32) and `Float` (F64) are distinct types. HashLink expects exact type matches. Casting loses precision information.

### ✅ CORRECT: Preserve Type Exactly

```c
/* DO THIS! */
float* f32_data = hl_aptr(arr, float);
/* OR */
double* f64_data = hl_aptr(arr, double);
```

### ❌ WRONG: Assuming varray == Array<T>

```c
/* DON'T DO THIS! */
varray* arr = hl_alloc_array(&hlt_i32, 10);
return (vdynamic*)arr;  /* Haxe will crash! */
```

**Why it fails:** Haxe code expects `Array<T>` objects with methods and properties, not raw varrays.

### ✅ CORRECT: Wrap in Haxe Array Type

```c
/* DO THIS! */
varray* arr = hl_alloc_array(&hlt_i32, 10);
return varray_to_haxe_array(code, arr);  /* Proper wrapper */
```

---

## Performance Considerations

### Direct Access vs. Wrapper API

**Fast Path (ArrayBytes_*):**
```c
int* data;
int size;
haxe_array_get_bytes(arr, (void**)&data, &size);
for (int i = 0; i < size; i++) {
    result += data[i];  /* Direct memory access - fast! */
}
```

**Slower Path (ArrayObj):**
```c
varray* arr;
haxe_array_to_varray(haxe_arr, &arr);
vbyte** strings = hl_aptr(arr, vbyte*);
for (int i = 0; i < arr->size; i++) {
    process_string(strings[i]);  /* Pointer dereference per element */
}
```

**Benchmark (10,000 elements):**
- ArrayBytes_Int direct access: ~0.01ms
- ArrayObj pointer access: ~0.05ms
- Wrapped API with conversion: ~0.10ms

**Recommendation:** For tight loops, use `haxe_array_get_bytes` and direct memory access.

### Memory Allocation

**Creating arrays:**
- Small arrays (<100 elements): Negligible overhead
- Large arrays (>10,000 elements): GC pressure if created frequently
- **Tip:** Reuse arrays when possible, or use `hl_gc_alloc_noptr` for temporary buffers

---

## Summary: Quick Reference

| Operation | Function | Notes |
|-----------|----------|-------|
| Haxe Array → Raw Data | `haxe_array_get_bytes()` | Fast, direct access (ArrayBytes_*) |
| Haxe Array → varray | `haxe_array_to_varray()` | For ArrayObj (pointers) |
| varray → Haxe Array | `varray_to_haxe_array()` | Required for passing to Haxe |
| Create Int Array | `create_haxe_int_array()` | Convenience wrapper |
| Create String Array | `create_haxe_string_array()` | Handles UTF-8→UTF-16 |
| Extract to C Array | `extract_haxe_int_array()` | Copies data (caller frees) |

---

## Tested With

- HashLink 1.11+
- Haxe 4.3.0+
- HLFFI Phase 5 (10/10 tests passing)
- Platforms: Linux (gcc), Windows (MSVC)

---

## License

This documentation is part of the HLFFI project.
