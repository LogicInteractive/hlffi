# HLFFI API Reference - Arrays

**[← Instance Members](API_09_INSTANCE_MEMBERS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Maps →](API_11_MAPS.md)**

Create and manipulate Haxe arrays, including dynamic arrays and struct arrays.

---

## Quick Reference

### Dynamic Arrays

| Function | Purpose |
|----------|---------|
| `hlffi_array_new(vm, type, len)` | Create new array |
| `hlffi_array_length(arr)` | Get array length |
| `hlffi_array_get(vm, arr, idx)` | Get element at index |
| `hlffi_array_set(vm, arr, idx, val)` | Set element at index |
| `hlffi_array_push(vm, arr, val)` | Append element |

### Struct Arrays (Zero-Copy)

| Function | Purpose |
|----------|---------|
| `hlffi_array_new_struct(vm, type, len)` | Create struct array |
| `hlffi_array_get_struct(arr, idx)` | Get pointer to struct |
| `hlffi_array_set_struct(vm, arr, idx, ptr, size)` | Set struct by copy |

**Complete Guide:** See `docs/PHASE4_INSTANCE_MEMBERS.md`

---

## Dynamic Arrays

### Creating Arrays

**Signature:**
```c
hlffi_value* hlffi_array_new(hlffi_vm* vm, hl_type* element_type, int length)
```

**Parameters:**
- `vm` - VM instance
- `element_type` - Element type (`&hlt_i32`, `&hlt_f64`, `&hlt_bytes`, etc.), or `NULL` for dynamic
- `length` - Initial length (all elements initialized to default/null)

**Returns:** New array, or NULL on error

**Examples:**
```c
// Int array:
hlffi_value* int_arr = hlffi_array_new(vm, &hlt_i32, 10);

// String array:
hlffi_value* str_arr = hlffi_array_new(vm, &hlt_bytes, 5);

// Dynamic array (Array<Dynamic>):
hlffi_value* dyn_arr = hlffi_array_new(vm, NULL, 5);
```

---

### Getting Elements

**Signature:**
```c
hlffi_value* hlffi_array_get(hlffi_vm* vm, hlffi_value* arr, int index)
```

**Returns:** Element at index, or NULL if out of bounds

**Example:**
```c
hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 5);
hlffi_value* elem = hlffi_array_get(vm, arr, 0);
if (elem) {
    int value = hlffi_value_as_int(elem, 0);
    printf("arr[0] = %d\n", value);
    hlffi_value_free(elem);
}
```

---

### Setting Elements

**Signature:**
```c
bool hlffi_array_set(hlffi_vm* vm, hlffi_value* arr, int index, hlffi_value* value)
```

**Returns:** `true` on success, `false` if out of bounds

**Example:**
```c
hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 5);
hlffi_value* val = hlffi_value_int(vm, 42);
hlffi_array_set(vm, arr, 0, val);
hlffi_value_free(val);
```

---

### Getting Length

**Signature:**
```c
int hlffi_array_length(hlffi_value* arr)
```

**Returns:** Array length, or -1 if not an array

**Example:**
```c
int len = hlffi_array_length(arr);
for (int i = 0; i < len; i++) {
    hlffi_value* elem = hlffi_array_get(vm, arr, i);
    // ... use elem ...
    hlffi_value_free(elem);
}
```

---

### Appending Elements

**Signature:**
```c
bool hlffi_array_push(hlffi_vm* vm, hlffi_value* arr, hlffi_value* value)
```

**Description:** Append element to end of array (grows array by 1).

**Performance:** O(1) amortized, but slower than preallocating. **For large arrays, use `hlffi_array_new()` with size + `hlffi_array_set()`.**

**Example:**
```c
hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 0);
hlffi_value* val = hlffi_value_int(vm, 42);
hlffi_array_push(vm, arr, val);
hlffi_value_free(val);
```

---

## Struct Arrays (Zero-Copy Access)

Struct arrays store C structs directly in memory without boxing, enabling high-performance access.

### Creating Struct Arrays

**Signature:**
```c
hlffi_value* hlffi_array_new_struct(hlffi_vm* vm, hlffi_type* struct_type, int length)
```

**Parameters:**
- `vm` - VM instance
- `struct_type` - Type from `hlffi_find_type(vm, "StructName")`
- `length` - Number of structs

**Example:**
```c
// Haxe: @:struct class Vec3 { public var x:Float; public var y:Float; public var z:Float; }
hlffi_type* vec3_type = hlffi_find_type(vm, "Vec3");
hlffi_value* arr = hlffi_array_new_struct(vm, vec3_type, 100);
```

---

### Getting Struct Pointers (Zero-Copy)

**Signature:**
```c
void* hlffi_array_get_struct(hlffi_value* arr, int index)
```

**Returns:** Direct pointer to struct in array, or NULL if out of bounds

**Example:**
```c
// C struct matching Haxe @:struct class:
typedef struct { double x, y, z; } Vec3;

Vec3* v = (Vec3*)hlffi_array_get_struct(arr, 0);
if (v) {
    v->x = 1.0;  // Modify in-place (no boxing!)
    v->y = 2.0;
    v->z = 3.0;
}
```

---

### Setting Structs

**Signature:**
```c
bool hlffi_array_set_struct(hlffi_vm* vm, hlffi_value* arr, int index, void* struct_ptr, int struct_size)
```

**Description:** Copy struct data into array at index.

**Example:**
```c
Vec3 v = {1.0, 2.0, 3.0};
hlffi_array_set_struct(vm, arr, 0, &v, sizeof(Vec3));
```

---

## Complete Example

```c
#include "hlffi.h"

int main() {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Create and populate int array:
    hlffi_value* scores = hlffi_array_new(vm, &hlt_i32, 3);

    hlffi_value* v1 = hlffi_value_int(vm, 100);
    hlffi_value* v2 = hlffi_value_int(vm, 200);
    hlffi_value* v3 = hlffi_value_int(vm, 300);

    hlffi_array_set(vm, scores, 0, v1);
    hlffi_array_set(vm, scores, 1, v2);
    hlffi_array_set(vm, scores, 2, v3);

    hlffi_value_free(v1);
    hlffi_value_free(v2);
    hlffi_value_free(v3);

    // Read back:
    int len = hlffi_array_length(scores);
    printf("Scores (%d):\n", len);
    for (int i = 0; i < len; i++) {
        hlffi_value* elem = hlffi_array_get(vm, scores, i);
        int score = hlffi_value_as_int(elem, 0);
        printf("  [%d] = %d\n", i, score);
        hlffi_value_free(elem);
    }

    // Struct array (zero-copy):
    typedef struct { double x, y, z; } Vec3;
    hlffi_type* vec_type = hlffi_find_type(vm, "Vec3");
    hlffi_value* positions = hlffi_array_new_struct(vm, vec_type, 10);

    // Write directly:
    Vec3* pos = (Vec3*)hlffi_array_get_struct(positions, 0);
    pos->x = 10.0;
    pos->y = 20.0;
    pos->z = 30.0;

    // Cleanup:
    hlffi_value_free(scores);
    hlffi_value_free(positions);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
@:struct class Vec3 {
    public var x:Float;
    public var y:Float;
    public var z:Float;

    public function new(x:Float, y:Float, z:Float) {
        this.x = x; this.y = y; this.z = z;
    }
}
```

---

## Best Practices

### 1. Preallocate for Performance

```c
// ✅ GOOD - Preallocate known size
hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 1000);
for (int i = 0; i < 1000; i++) {
    hlffi_value* val = hlffi_value_int(vm, i);
    hlffi_array_set(vm, arr, i, val);
    hlffi_value_free(val);
}

// ❌ SLOWER - Growing with push
hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 0);
for (int i = 0; i < 1000; i++) {
    hlffi_value* val = hlffi_value_int(vm, i);
    hlffi_array_push(vm, arr, val);  // Reallocation overhead
    hlffi_value_free(val);
}
```

### 2. Use Struct Arrays for Performance-Critical Data

```c
// ✅ GOOD - Zero-copy struct access
hlffi_value* arr = hlffi_array_new_struct(vm, vec3_type, 1000);
Vec3* v = (Vec3*)hlffi_array_get_struct(arr, 0);
v->x = 10.0;  // Direct memory write

// ❌ SLOWER - Boxed values
hlffi_value* arr = hlffi_array_new(vm, vec3_obj_type, 1000);
hlffi_value* v = hlffi_array_get(vm, arr, 0);
hlffi_set_field_float(vm, v, "x", 10.0);  // Boxing overhead
hlffi_value_free(v);
```

### 3. Free Returned Elements

```c
// ✅ GOOD - Free elements from hlffi_array_get()
hlffi_value* elem = hlffi_array_get(vm, arr, 0);
// ... use elem ...
hlffi_value_free(elem);

// ❌ BAD - Memory leak
hlffi_value* elem = hlffi_array_get(vm, arr, 0);
// Missing: hlffi_value_free(elem);
```

---

**[← Instance Members](API_09_INSTANCE_MEMBERS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Maps →](API_11_MAPS.md)**
