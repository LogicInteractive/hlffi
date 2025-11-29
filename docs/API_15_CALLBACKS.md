# HLFFI API Reference - Callbacks & FFI

**[← Abstracts](API_14_ABSTRACTS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Exceptions →](API_16_EXCEPTIONS.md)**

Register C functions that can be called from Haxe code.

---

## Overview

HLFFI allows registering C callbacks that Haxe can call:

```haxe
// Haxe side:
@:hlNative("", "myCallback")
static function myCallback(x:Dynamic):Dynamic;
```

```c
// C side:
hlffi_value* my_callback(hlffi_vm* vm, int argc, hlffi_value** argv) {
    int x = hlffi_value_as_int(argv[0], 0);
    printf("Called from Haxe: %d\n", x);
    return hlffi_value_int(vm, x * 2);
}

hlffi_register_callback(vm, "myCallback", my_callback, 1);
```

---

## Quick Reference

| Function | Purpose |
|----------|---------|
| `hlffi_register_callback(vm, name, func, nargs)` | Register C callback (recommended) |
| `hlffi_register_callback_typed(...)` | Typed callback (has limitations) |
| `hlffi_get_callback(name)` | Get registered callback |
| `hlffi_unregister_callback(name)` | Remove callback |

**Complete Guide:** See `docs/PHASE6_COMPLETE.md`

---

## Callback Signature

All callbacks use this signature:

```c
typedef hlffi_value* (*hlffi_native_func)(
    hlffi_vm* vm,
    int argc,
    hlffi_value** argv
);
```

**Parameters:**
- `vm` - VM instance (for creating return values)
- `argc` - Number of arguments
- `argv` - Array of argument values

**Returns:** `hlffi_value*` (return value), or `hlffi_value_null(vm)` for void

---

## Registering Callbacks

### Recommended: Dynamic Arguments

**Signature:**
```c
bool hlffi_register_callback(
    hlffi_vm* vm,
    const char* name,
    hlffi_native_func func,
    int nargs
)
```

**Parameters:**
- `vm` - VM instance
- `name` - Callback name (matches Haxe `@:hlNative("", "name")`)
- `func` - C callback function
- `nargs` - Expected argument count

**Returns:** `true` on success

**Example:**
```c
hlffi_value* on_message(hlffi_vm* vm, int argc, hlffi_value** argv) {
    char* msg = hlffi_value_as_string(argv[0]);
    printf("Message: %s\n", msg);
    free(msg);
    return hlffi_value_null(vm);
}

// Register:
hlffi_register_callback(vm, "onMessage", on_message, 1);
```

**Haxe Side:**
```haxe
@:hlNative("", "onMessage")
static function onMessage(msg:Dynamic):Void;

// Call it:
onMessage("Hello from Haxe!");
```

---

### Typed Callbacks (Deprecated)

**⚠️ WARNING:** Typed callbacks have fundamental limitations. **Use `hlffi_register_callback()` with `Dynamic` types instead.**

**Signature:**
```c
bool hlffi_register_callback_typed(
    hlffi_vm* vm,
    const char* name,
    hlffi_native_func func,
    int nargs,
    hlffi_type** arg_types,
    hlffi_type* return_type
)
```

**Limitation:** HashLink cannot safely call native functions with non-dynamic signatures from bytecode. Use dynamic signatures and manual type conversion instead.

---

## Getting Registered Callbacks

**Signature:**
```c
hlffi_native_func hlffi_get_callback(const char* name)
```

**Returns:** Registered callback, or NULL if not found

**Example:**
```c
hlffi_native_func cb = hlffi_get_callback("onMessage");
if (cb) {
    printf("Callback 'onMessage' is registered\n");
}
```

---

## Unregistering Callbacks

**Signature:**
```c
bool hlffi_unregister_callback(const char* name)
```

**Returns:** `true` if callback was found and removed

**Example:**
```c
hlffi_unregister_callback("onMessage");
```

---

## Complete Example

```c
#include "hlffi.h"

// Callback: Add two numbers
hlffi_value* native_add(hlffi_vm* vm, int argc, hlffi_value** argv) {
    int a = hlffi_value_as_int(argv[0], 0);
    int b = hlffi_value_as_int(argv[1], 0);
    int result = a + b;
    printf("C: %d + %d = %d\n", a, b, result);
    return hlffi_value_int(vm, result);
}

// Callback: Print message
hlffi_value* native_print(hlffi_vm* vm, int argc, hlffi_value** argv) {
    char* msg = hlffi_value_as_string(argv[0]);
    printf("C: %s\n", msg);
    free(msg);
    return hlffi_value_null(vm);
}

int main() {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);

    // Register callbacks BEFORE loading bytecode:
    hlffi_register_callback(vm, "nativeAdd", native_add, 2);
    hlffi_register_callback(vm, "nativePrint", native_print, 1);

    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);  // Haxe will call callbacks

    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class Main {
    // Declare native callbacks:
    @:hlNative("", "nativeAdd")
    static function nativeAdd(a:Dynamic, b:Dynamic):Dynamic;

    @:hlNative("", "nativePrint")
    static function nativePrint(msg:Dynamic):Void;

    public static function main() {
        // Call C from Haxe:
        var result = nativeAdd(10, 20);
        trace('Result from C: $result');  // 30

        nativePrint("Hello from Haxe!");
    }
}
```

---

## Best Practices

### 1. Register Before Loading Bytecode

```c
// ✅ GOOD - Register before load
hlffi_register_callback(vm, "onEvent", my_callback, 1);
hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);

// ❌ BAD - Register after load
hlffi_load_file(vm, "game.hl");
hlffi_register_callback(vm, "onEvent", my_callback, 1);  // Too late!
```

### 2. Use Dynamic Types in Haxe

```haxe
// ✅ GOOD - Dynamic arguments
@:hlNative("", "callback")
static function callback(arg:Dynamic):Dynamic;

// ❌ BAD - Typed arguments (doesn't work reliably)
@:hlNative("", "callback")
static function callback(arg:Int):Int;
```

### 3. Free Strings from Arguments

```c
// ✅ GOOD
hlffi_value* callback(hlffi_vm* vm, int argc, hlffi_value** argv) {
    char* str = hlffi_value_as_string(argv[0]);
    printf("%s\n", str);
    free(str);  // MUST free
    return hlffi_value_null(vm);
}

// ❌ BAD - Memory leak
hlffi_value* callback(hlffi_vm* vm, int argc, hlffi_value** argv) {
    char* str = hlffi_value_as_string(argv[0]);
    printf("%s\n", str);
    // Missing: free(str);
    return hlffi_value_null(vm);
}
```

### 4. Return hlffi_value_null() for Void

```c
// ✅ GOOD - Always return a value
hlffi_value* callback(hlffi_vm* vm, int argc, hlffi_value** argv) {
    // ... do work ...
    return hlffi_value_null(vm);  // For void return
}

// ❌ BAD - Don't return NULL directly
hlffi_value* callback(hlffi_vm* vm, int argc, hlffi_value** argv) {
    // ... do work ...
    return NULL;  // Undefined behavior
}
```

---

## Common Patterns

### Event Callback

```c
hlffi_value* on_player_hit(hlffi_vm* vm, int argc, hlffi_value** argv) {
    int damage = hlffi_value_as_int(argv[0], 0);
    char* attacker = hlffi_value_as_string(argv[1]);

    printf("Player hit by %s for %d damage\n", attacker, damage);

    free(attacker);
    return hlffi_value_null(vm);
}

hlffi_register_callback(vm, "onPlayerHit", on_player_hit, 2);
```

### Computation Callback

```c
hlffi_value* calculate_score(hlffi_vm* vm, int argc, hlffi_value** argv) {
    int kills = hlffi_value_as_int(argv[0], 0);
    int deaths = hlffi_value_as_int(argv[1], 1);

    int score = kills * 100 - deaths * 50;
    return hlffi_value_int(vm, score);
}

hlffi_register_callback(vm, "calculateScore", calculate_score, 2);
```

---

## Threading Considerations

**⚠️ Callbacks execute on VM thread:**
- In NON_THREADED mode: Same thread as `hlffi_update()`
- In THREADED mode: Dedicated VM thread

**If accessing C state from callback:**
- Use proper synchronization (mutexes)
- Or use `hlffi_blocking_begin/end()` for external I/O

**Example:**
```c
hlffi_value* blocking_callback(hlffi_vm* vm, int argc, hlffi_value** argv) {
    hlffi_blocking_begin();  // Allow VM to continue
    sleep(1);  // External I/O
    hlffi_blocking_end();    // Resume VM operations
    return hlffi_value_null(vm);
}
```

---

**[← Abstracts](API_14_ABSTRACTS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Exceptions →](API_16_EXCEPTIONS.md)**
