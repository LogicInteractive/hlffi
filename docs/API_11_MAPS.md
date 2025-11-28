# HLFFI API Reference - Maps (Hash Tables)

**[← Arrays](API_10_ARRAYS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Bytes →](API_12_BYTES.md)**

Create and manipulate Haxe maps (hash tables / dictionaries).

---

## Quick Reference

| Function | Purpose |
|----------|---------|
| `hlffi_map_new(vm, key_type, val_type)` | Create new map |
| `hlffi_map_set(vm, map, key, value)` | Insert/update key-value pair |
| `hlffi_map_get(vm, map, key)` | Get value by key |
| `hlffi_map_exists(vm, map, key)` | Check if key exists |
| `hlffi_map_remove(vm, map, key)` | Remove key-value pair |
| `hlffi_map_keys(vm, map)` | Get array of all keys |
| `hlffi_map_values(vm, map)` | Get array of all values |
| `hlffi_map_size(map)` | Get number of entries |
| `hlffi_map_clear(map)` | Remove all entries |

**Complete Guide:** See `docs/PHASE4_INSTANCE_MEMBERS.md`

---

## Creating Maps

**Signature:**
```c
hlffi_value* hlffi_map_new(hlffi_vm* vm, hl_type* key_type, hl_type* value_type)
```

**Parameters:**
- `vm` - VM instance
- `key_type` - Key type (`&hlt_i32`, `&hlt_bytes`, etc.)
- `value_type` - Value type

**Returns:** New map, or NULL on error

**Common Map Types:**

| Key Type | Value Type | Haxe Equivalent |
|----------|------------|-----------------|
| `&hlt_i32` | `&hlt_bytes` | `Map<Int, String>` |
| `&hlt_bytes` | `&hlt_i32` | `Map<String, Int>` |
| `&hlt_i32` | `&hlt_i32` | `Map<Int, Int>` |
| `NULL` | `NULL` | `Map<Dynamic, Dynamic>` |

**Examples:**
```c
// Map<Int, String>:
hlffi_value* map = hlffi_map_new(vm, &hlt_i32, &hlt_bytes);

// Map<String, Int>:
hlffi_value* scores = hlffi_map_new(vm, &hlt_bytes, &hlt_i32);

// Map<Dynamic, Dynamic>:
hlffi_value* dyn_map = hlffi_map_new(vm, NULL, NULL);
```

---

## Setting Values

**Signature:**
```c
bool hlffi_map_set(hlffi_vm* vm, hlffi_value* map, hlffi_value* key, hlffi_value* value)
```

**Description:** Insert or update key-value pair. If key exists, value is replaced.

**Returns:** `true` on success

**Example:**
```c
hlffi_value* map = hlffi_map_new(vm, &hlt_bytes, &hlt_i32);

hlffi_value* key = hlffi_value_string(vm, "player1");
hlffi_value* val = hlffi_value_int(vm, 1000);
hlffi_map_set(vm, map, key, val);
hlffi_value_free(key);
hlffi_value_free(val);
```

---

## Getting Values

**Signature:**
```c
hlffi_value* hlffi_map_get(hlffi_vm* vm, hlffi_value* map, hlffi_value* key)
```

**Returns:** Value for key, or NULL if key doesn't exist

**Note:** NULL can also mean the value is null. Use `hlffi_map_exists()` to distinguish.

**Example:**
```c
hlffi_value* key = hlffi_value_string(vm, "player1");
hlffi_value* val = hlffi_map_get(vm, map, key);

if (val) {
    int score = hlffi_value_as_int(val, 0);
    printf("Score: %d\n", score);
    hlffi_value_free(val);
} else {
    printf("Key not found\n");
}

hlffi_value_free(key);
```

---

## Checking Existence

**Signature:**
```c
bool hlffi_map_exists(hlffi_vm* vm, hlffi_value* map, hlffi_value* key)
```

**Returns:** `true` if key exists, `false` otherwise

**Use Case:** Distinguish between missing key and null value.

**Example:**
```c
hlffi_value* key = hlffi_value_string(vm, "player1");

if (hlffi_map_exists(vm, map, key)) {
    hlffi_value* val = hlffi_map_get(vm, map, key);
    if (hlffi_value_is_null(val)) {
        printf("Key exists with null value\n");
    } else {
        printf("Key exists with value: %d\n", hlffi_value_as_int(val, 0));
    }
    hlffi_value_free(val);
} else {
    printf("Key does not exist\n");
}

hlffi_value_free(key);
```

---

## Removing Entries

**Signature:**
```c
bool hlffi_map_remove(hlffi_vm* vm, hlffi_value* map, hlffi_value* key)
```

**Returns:** `true` if key was removed, `false` if key didn't exist

**Example:**
```c
hlffi_value* key = hlffi_value_string(vm, "player1");

if (hlffi_map_remove(vm, map, key)) {
    printf("Key removed\n");
} else {
    printf("Key not found\n");
}

hlffi_value_free(key);
```

---

## Getting All Keys

**Signature:**
```c
hlffi_value* hlffi_map_keys(hlffi_vm* vm, hlffi_value* map)
```

**Returns:** Array of all keys (use `hlffi_array_length()` to get count)

**Example:**
```c
hlffi_value* keys = hlffi_map_keys(vm, map);
int count = hlffi_array_length(keys);

printf("Keys (%d):\n", count);
for (int i = 0; i < count; i++) {
    hlffi_value* key = hlffi_array_get(vm, keys, i);
    char* key_str = hlffi_value_as_string(key);
    printf("  %s\n", key_str);
    free(key_str);
    hlffi_value_free(key);
}

hlffi_value_free(keys);
```

---

## Getting All Values

**Signature:**
```c
hlffi_value* hlffi_map_values(hlffi_vm* vm, hlffi_value* map)
```

**Returns:** Array of all values (same order as `hlffi_map_keys()`)

**Example:**
```c
hlffi_value* values = hlffi_map_values(vm, map);
int len = hlffi_array_length(values);

for (int i = 0; i < len; i++) {
    hlffi_value* val = hlffi_array_get(vm, values, i);
    int score = hlffi_value_as_int(val, 0);
    printf("Value: %d\n", score);
    hlffi_value_free(val);
}

hlffi_value_free(values);
```

---

## Getting Size

**Signature:**
```c
int hlffi_map_size(hlffi_value* map)
```

**Returns:** Number of key-value pairs, or -1 if not a map

**Example:**
```c
int size = hlffi_map_size(map);
printf("Map has %d entries\n", size);
```

---

## Clearing All Entries

**Signature:**
```c
bool hlffi_map_clear(hlffi_value* map)
```

**Description:** Remove all key-value pairs (map becomes empty).

**Returns:** `true` on success

**Example:**
```c
hlffi_map_clear(map);
printf("Size after clear: %d\n", hlffi_map_size(map));  // 0
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

    // Create Map<String, Int> for player scores:
    hlffi_value* scores = hlffi_map_new(vm, &hlt_bytes, &hlt_i32);

    // Insert scores:
    hlffi_value* p1_key = hlffi_value_string(vm, "Alice");
    hlffi_value* p1_val = hlffi_value_int(vm, 1000);
    hlffi_map_set(vm, scores, p1_key, p1_val);
    hlffi_value_free(p1_key);
    hlffi_value_free(p1_val);

    hlffi_value* p2_key = hlffi_value_string(vm, "Bob");
    hlffi_value* p2_val = hlffi_value_int(vm, 1500);
    hlffi_map_set(vm, scores, p2_key, p2_val);
    hlffi_value_free(p2_key);
    hlffi_value_free(p2_val);

    // Get score:
    hlffi_value* key = hlffi_value_string(vm, "Alice");
    hlffi_value* val = hlffi_map_get(vm, scores, key);
    if (val) {
        printf("Alice's score: %d\n", hlffi_value_as_int(val, 0));
        hlffi_value_free(val);
    }
    hlffi_value_free(key);

    // Enumerate all scores:
    hlffi_value* keys = hlffi_map_keys(vm, scores);
    hlffi_value* values = hlffi_map_values(vm, scores);
    int count = hlffi_array_length(keys);

    printf("All scores (%d):\n", count);
    for (int i = 0; i < count; i++) {
        hlffi_value* k = hlffi_array_get(vm, keys, i);
        hlffi_value* v = hlffi_array_get(vm, values, i);

        char* name = hlffi_value_as_string(k);
        int score = hlffi_value_as_int(v, 0);
        printf("  %s: %d\n", name, score);

        free(name);
        hlffi_value_free(k);
        hlffi_value_free(v);
    }

    hlffi_value_free(keys);
    hlffi_value_free(values);

    // Cleanup:
    hlffi_value_free(scores);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class Main {
    public static function main() {
        var scores = new Map<String, Int>();
        scores["Alice"] = 1000;
        scores["Bob"] = 1500;
    }
}
```

---

## Best Practices

### 1. Check Existence for Null Values

```c
// ✅ GOOD - Distinguish missing key from null value
if (hlffi_map_exists(vm, map, key)) {
    hlffi_value* val = hlffi_map_get(vm, map, key);
    if (hlffi_value_is_null(val)) {
        printf("Key exists, value is null\n");
    }
    hlffi_value_free(val);
} else {
    printf("Key doesn't exist\n");
}

// ❌ AMBIGUOUS
hlffi_value* val = hlffi_map_get(vm, map, key);
if (!val) {
    // Could be missing key OR null value
}
```

### 2. Free Keys and Values

```c
// ✅ GOOD - Free all temporary values
hlffi_value* key = hlffi_value_string(vm, "player1");
hlffi_value* val = hlffi_value_int(vm, 100);
hlffi_map_set(vm, map, key, val);
hlffi_value_free(key);
hlffi_value_free(val);

// ❌ BAD - Memory leak
hlffi_value* key = hlffi_value_string(vm, "player1");
hlffi_value* val = hlffi_value_int(vm, 100);
hlffi_map_set(vm, map, key, val);
// Missing: hlffi_value_free(key); hlffi_value_free(val);
```

### 3. Use Enumeration for Iteration

```c
// ✅ GOOD - Enumerate keys and values together
hlffi_value* keys = hlffi_map_keys(vm, map);
hlffi_value* values = hlffi_map_values(vm, map);
int len = hlffi_array_length(keys);

for (int i = 0; i < len; i++) {
    hlffi_value* k = hlffi_array_get(vm, keys, i);
    hlffi_value* v = hlffi_array_get(vm, values, i);
    // ... process ...
    hlffi_value_free(k);
    hlffi_value_free(v);
}

hlffi_value_free(keys);
hlffi_value_free(values);
```

---

**[← Arrays](API_10_ARRAYS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Bytes →](API_12_BYTES.md)**
