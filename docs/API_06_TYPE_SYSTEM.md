# HLFFI API Reference - Type System & Reflection

**[← Hot Reload](API_05_HOT_RELOAD.md)** | **[Back to Index](API_REFERENCE.md)** | **[Values →](API_07_VALUES.md)**

The type system allows runtime introspection of Haxe types.

---

## Quick Reference

| Function | Purpose |
|----------|---------|
| `hlffi_find_type(vm, "Player")` | Find type by name |
| `hlffi_type_get_kind(type)` | Get kind (class/enum/primitive) |
| `hlffi_type_get_name(type)` | Get type name string |
| `hlffi_list_types(vm, callback, data)` | Enumerate all types |
| `hlffi_class_get_super(type)` | Get parent class |
| `hlffi_class_get_field_count(type)` | Count instance fields |
| `hlffi_class_get_field_name(type, idx)` | Get field name by index |
| `hlffi_class_get_field_type(type, idx)` | Get field type by index |
| `hlffi_class_get_method_count(type)` | Count methods |
| `hlffi_class_get_method_name(type, idx)` | Get method name by index |

**Complete Guide:** See `docs/PHASE2_COMPLETE.md`

---

## Example

```c
// Find a type:
hlffi_type* player_type = hlffi_find_type(vm, "Player");
if (!player_type) {
    fprintf(stderr, "Type 'Player' not found\n");
    return;
}

// Get type info:
const char* name = hlffi_type_get_name(player_type);
hlffi_type_kind kind = hlffi_type_get_kind(player_type);
printf("Type: %s (kind: %d)\n", name, kind);

// Get parent class:
hlffi_type* parent = hlffi_class_get_super(player_type);
if (parent) {
    printf("Extends: %s\n", hlffi_type_get_name(parent));
}

// Enumerate fields:
int field_count = hlffi_class_get_field_count(player_type);
printf("Fields (%d):\n", field_count);
for (int i = 0; i < field_count; i++) {
    const char* fname = hlffi_class_get_field_name(player_type, i);
    hlffi_type* ftype = hlffi_class_get_field_type(player_type, i);
    printf("  %s : %s\n", fname, hlffi_type_get_name(ftype));
}

// Enumerate methods:
int method_count = hlffi_class_get_method_count(player_type);
printf("Methods (%d):\n", method_count);
for (int i = 0; i < method_count; i++) {
    const char* mname = hlffi_class_get_method_name(player_type, i);
    printf("  %s()\n", mname);
}
```

---

## Type Kinds

```c
typedef enum {
    HLFFI_TYPE_VOID,
    HLFFI_TYPE_I32,      // Int
    HLFFI_TYPE_F64,      // Float
    HLFFI_TYPE_BOOL,
    HLFFI_TYPE_BYTES,
    HLFFI_TYPE_OBJ,      // Class/object
    HLFFI_TYPE_ARRAY,
    HLFFI_TYPE_ENUM,
    HLFFI_TYPE_ABSTRACT,
    // ... more types
} hlffi_type_kind;
```

---

## Enumerating All Types

```c
void type_visitor(hlffi_type* type, void* userdata) {
    hlffi_type_kind kind = hlffi_type_get_kind(type);
    const char* name = hlffi_type_get_name(type);
    
    if (kind == HLFFI_TYPE_OBJ) {
        printf("Class: %s\n", name);
    } else if (kind == HLFFI_TYPE_ENUM) {
        printf("Enum: %s\n", name);
    }
}

// Enumerate:
hlffi_list_types(vm, type_visitor, NULL);
```

---

**[← Hot Reload](API_05_HOT_RELOAD.md)** | **[Back to Index](API_REFERENCE.md)** | **[Values →](API_07_VALUES.md)**
