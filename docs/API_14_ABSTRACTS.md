# HLFFI API Reference - Abstracts

**[← Enums](API_13_ENUMS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Callbacks →](API_15_CALLBACKS.md)**

Work with Haxe abstract types (type wrappers over underlying types).

---

## Overview

**Haxe Abstracts** are compile-time wrappers that provide type safety without runtime overhead. They wrap primitive types, objects, or other abstracts.

```haxe
abstract UserId(Int) {
    public inline function new(id:Int) {
        this = id;
    }

    public inline function toInt():Int {
        return this;
    }
}

abstract Meters(Float) to Float from Float {}
```

**Key Point:** At runtime, abstracts are just their underlying type. HLFFI works with the underlying type directly.

---

## Quick Reference

| Function | Purpose |
|----------|---------|
| `hlffi_abstract_find(vm, name)` | Find abstract type by name |
| `hlffi_abstract_get_name(type)` | Get abstract type name |

**Complete Guide:** See `docs/PHASE2_COMPLETE.md`

---

## Finding Abstracts

**Signature:**
```c
hlffi_type* hlffi_abstract_find(hlffi_vm* vm, const char* name)
```

**Returns:** Abstract type, or NULL if not found

**Example:**
```c
hlffi_type* user_id = hlffi_abstract_find(vm, "UserId");
if (!user_id) {
    fprintf(stderr, "Abstract 'UserId' not found\n");
}
```

---

## Getting Abstract Name

**Signature:**
```c
char* hlffi_abstract_get_name(hlffi_type* type)
```

**Returns:** Abstract type name (**caller must free()**), or NULL if not abstract

**Example:**
```c
hlffi_type* abs = hlffi_abstract_find(vm, "UserId");
char* name = hlffi_abstract_get_name(abs);
printf("Abstract: %s\n", name);  // "UserId"
free(name);
```

---

## Working with Abstract Values

Abstracts are transparent at runtime - use the underlying type directly.

### Example: Int Abstract

**Haxe:**
```haxe
abstract UserId(Int) {
    public inline function new(id:Int) {
        this = id;
    }
}

class User {
    public static function getUserName(id:UserId):String {
        return "User" + id;
    }
}
```

**C:**
```c
// Create UserId (just an Int at runtime):
hlffi_value* user_id = hlffi_value_int(vm, 42);

// Call function expecting UserId:
hlffi_value* args[] = {user_id};
hlffi_value* name = hlffi_call_static(vm, "User", "getUserName", 1, args);

char* str = hlffi_value_as_string(name);
printf("Name: %s\n", str);  // "User42"
free(str);

hlffi_value_free(user_id);
hlffi_value_free(name);
```

---

### Example: Float Abstract

**Haxe:**
```haxe
abstract Meters(Float) to Float from Float {
    public inline function toKilometers():Float {
        return this / 1000.0;
    }
}

class Distance {
    public static function convert(m:Meters):Float {
        return m.toKilometers();
    }
}
```

**C:**
```c
// Create Meters (Float at runtime):
hlffi_value* meters = hlffi_value_float(vm, 5000.0);

// Call function:
hlffi_value* args[] = {meters};
hlffi_value* km = hlffi_call_static(vm, "Distance", "convert", 1, args);

double distance = hlffi_value_as_float(km, 0.0);
printf("Distance: %.2f km\n", distance);  // 5.00 km

hlffi_value_free(meters);
hlffi_value_free(km);
```

---

### Example: Object Abstract

**Haxe:**
```haxe
abstract Point({x:Float, y:Float}) {
    public inline function new(x:Float, y:Float) {
        this = {x: x, y: y};
    }

    public var x(get, never):Float;
    inline function get_x() return this.x;

    public var y(get, never):Float;
    inline function get_y() return this.y;
}
```

**C:**
```c
// Create underlying object:
hlffi_value* obj = hlffi_new(vm, "Point", 0, NULL);
hlffi_set_field_float(vm, obj, "x", 10.0);
hlffi_set_field_float(vm, obj, "y", 20.0);

// Pass to function expecting Point abstract:
hlffi_value* args[] = {obj};
hlffi_call_static(vm, "Graphics", "drawPoint", 1, args);

hlffi_value_free(obj);
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

    // Find abstract type:
    hlffi_type* user_id_type = hlffi_abstract_find(vm, "UserId");
    if (user_id_type) {
        char* name = hlffi_abstract_get_name(user_id_type);
        printf("Found abstract: %s\n", name);
        free(name);
    }

    // Use abstract (Int wrapper):
    hlffi_value* id = hlffi_value_int(vm, 123);
    hlffi_value* args[] = {id};
    hlffi_value* user = hlffi_call_static(vm, "User", "getById", 1, args);

    if (user) {
        char* name = hlffi_get_field_string(user, "name");
        printf("User: %s\n", name);
        free(name);
        hlffi_value_free(user);
    }

    hlffi_value_free(id);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
abstract UserId(Int) {
    public inline function new(id:Int) {
        this = id;
    }
}

class User {
    public var name:String;

    public function new(name:String) {
        this.name = name;
    }

    public static function getById(id:UserId):User {
        return new User("User" + id);
    }
}
```

---

## Best Practices

### 1. Use Underlying Type Directly

```c
// ✅ GOOD - Abstracts are transparent at runtime
abstract UserId(Int);

hlffi_value* id = hlffi_value_int(vm, 42);  // Directly create Int

// ❌ UNNECESSARY - No special construction needed
// There's no hlffi_abstract_new() - just use the underlying type
```

### 2. Type Safety in Haxe, Not C

```c
// Haxe provides type safety:
abstract UserId(Int) {}
abstract ProductId(Int) {}

// In C, both are just Int:
hlffi_value* user_id = hlffi_value_int(vm, 1);
hlffi_value* product_id = hlffi_value_int(vm, 2);

// Type safety enforced by Haxe compiler, not HLFFI
```

### 3. Check Underlying Type

```c
// ✅ GOOD - Verify what the abstract wraps
hlffi_type* abs = hlffi_abstract_find(vm, "UserId");
if (abs) {
    hlffi_type_kind kind = hlffi_type_get_kind(abs);
    // Check if underlying type is I32, F64, OBJ, etc.
}
```

---

## Common Abstract Patterns

### Numeric Wrappers

```c
// abstract Percentage(Float) from Float to Float
hlffi_value* pct = hlffi_value_float(vm, 0.75);  // 75%
```

### ID Types

```c
// abstract UserId(Int), abstract SessionId(Int)
hlffi_value* user_id = hlffi_value_int(vm, 123);
hlffi_value* session_id = hlffi_value_int(vm, 456);
```

### Enum Abstracts

```haxe
@:enum abstract Color(Int) {
    var Red = 0xFF0000;
    var Green = 0x00FF00;
    var Blue = 0x0000FF;
}
```

```c
// Just use Int values:
hlffi_value* red = hlffi_value_int(vm, 0xFF0000);
```

---

## Notes

- **No runtime overhead:** Abstracts are compile-time only
- **Use underlying type:** Create values using the wrapped type's constructor
- **Type safety in Haxe:** C code works with raw types; Haxe enforces abstract semantics
- **Inline methods:** Abstract methods are inlined at compile time (not accessible from C)

---

**[← Enums](API_13_ENUMS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Callbacks →](API_15_CALLBACKS.md)**
