# HLFFI Map Operations Guide

## Overview

HLFFI provides full interoperability with Haxe `Map<K, V>` objects. Maps can be created in Haxe and manipulated from C, or vice versa, with automatic type conversion and method dispatch.

## Supported Map Types

All Haxe Map variants are supported:
- `Map<Int, V>` - Integer keys
- `Map<String, V>` - String keys
- `Map<K, V>` - Any key/value types

## API Functions

### Getting Maps from Haxe

```c
// Call a Haxe function that returns a Map
hlffi_value* map = hlffi_call_static(vm, "MyClass", "getMap", 0, NULL);
```

### Reading Values

```c
// Get value for a key
hlffi_value* key = hlffi_value_int(vm, 42);
hlffi_value* value = hlffi_map_get(vm, map, key);

if (value) {
    char* str = hlffi_value_as_string(value);
    printf("map[42] = %s\n", str);
    free(str);
    hlffi_value_free(value);
}

hlffi_value_free(key);
```

### Setting Values

```c
// Set a new key-value pair
hlffi_value* key = hlffi_value_int(vm, 99);
hlffi_value* val = hlffi_value_string(vm, "new value");

bool success = hlffi_map_set(vm, map, key, val);

hlffi_value_free(key);
hlffi_value_free(val);
```

### Checking Existence

```c
// Check if a key exists
hlffi_value* key = hlffi_value_int(vm, 42);
bool exists = hlffi_map_exists(vm, map, key);

printf("Key exists: %s\n", exists ? "yes" : "no");

hlffi_value_free(key);
```

### Removing Keys

```c
// Remove a key-value pair
hlffi_value* key = hlffi_value_int(vm, 42);
bool removed = hlffi_map_remove(vm, map, key);

hlffi_value_free(key);
```

### Iteration

```c
// Get iterator over keys
hlffi_value* keys_iter = hlffi_map_keys(vm, map);
// Use with Haxe iterator protocol

// Get iterator over values
hlffi_value* vals_iter = hlffi_map_values(vm, map);
// Use with Haxe iterator protocol
```

## Complete Example

```c
#include "hlffi.h"

int main() {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, argc, argv);
    hlffi_load_file(vm, "test.hl");
    hlffi_call_entry(vm);

    /* Get map from Haxe */
    hlffi_value* map = hlffi_call_static(vm, "MyClass", "createMap", 0, NULL);

    /* Read existing value */
    hlffi_value* key1 = hlffi_value_int(vm, 1);
    hlffi_value* val1 = hlffi_map_get(vm, map, key1);
    if (val1) {
        char* str = hlffi_value_as_string(val1);
        printf("map[1] = %s\n", str);
        free(str);
        hlffi_value_free(val1);
    }

    /* Add new entry */
    hlffi_value* key2 = hlffi_value_int(vm, 42);
    hlffi_value* val2 = hlffi_value_string(vm, "answer");
    hlffi_map_set(vm, map, key2, val2);

    /* Check existence */
    bool exists = hlffi_map_exists(vm, map, key2);
    printf("Key 42 exists: %s\n", exists ? "yes" : "no");

    /* Pass modified map back to Haxe */
    hlffi_value* args[] = {map};
    hlffi_value* result = hlffi_call_static(vm, "MyClass", "processMap", 1, args);

    /* Cleanup */
    hlffi_value_free(key1);
    hlffi_value_free(key2);
    hlffi_value_free(val2);
    hlffi_value_free(result);
    hlffi_value_free(map);

    hlffi_destroy(vm);
    return 0;
}
```

## Haxe Side Example

```haxe
class MyClass {
    public static function createMap():Map<Int, String> {
        var map = new Map<Int, String>();
        map.set(1, "one");
        map.set(2, "two");
        map.set(3, "three");
        return map;
    }

    public static function processMap(map:Map<Int, String>):String {
        var result = "";
        for (key in map.keys()) {
            result += map.get(key) + " ";
        }
        return result;
    }
}
```

## Implementation Details

### Method Dispatch

Maps operations use instance method calls via `hlffi_call_method()`:
- `hlffi_map_get(map, key)` → `map.get(key)`
- `hlffi_map_set(map, key, val)` → `map.set(key, val)`
- `hlffi_map_exists(map, key)` → `map.exists(key)`
- `hlffi_map_remove(map, key)` → `map.remove(key)`

### Memory Management

- Map objects are GC-managed by HashLink
- Keys and values passed to map functions must be freed by caller
- Values returned from `hlffi_map_get()` must be freed by caller
- The map itself is freed with `hlffi_value_free(map)`

### Type Conversions

Key and value types are automatically handled:
- Use `hlffi_value_int()`, `hlffi_value_string()`, etc. to create keys
- Use `hlffi_value_as_int()`, `hlffi_value_as_string()`, etc. to extract values
- Type mismatches will result in NULL returns or false from set operations

## Performance Notes

- Map operations have ~40ns overhead for method dispatch
- For hot loops, consider caching method pointers using `hlffi_lookup()`
- Maps are reference types - passing to/from Haxe is zero-copy

## Current Limitations

1. **Map Creation**: Maps should be created in Haxe, not C (for now)
2. **Map Size**: No direct size query - use Haxe's `Lambda.count(map)`
3. **Map Clear**: Not yet implemented - create a new map instead

## Testing

See `test_map_demo.c` and `test/MapTest.hx` for complete working examples.

Run the test:
```bash
make
./test_map_demo
```

## See Also

- `include/hlffi.h` - Full API declarations
- `src/hlffi_maps.c` - Implementation
- `test/MapTest.hx` - Haxe test cases
- `test_map_demo.c` - C integration tests
