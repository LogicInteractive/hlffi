# HLFFI User Guide - Part 3: Data Exchange

**Working with complex data types**

[← Calling Haxe](GUIDE_02_CALLING_HAXE.md) | [Next: Advanced Topics →](GUIDE_04_ADVANCED.md)

---

## Overview

This guide covers how to exchange complex data between C and Haxe:
- **Arrays** - Lists of values
- **Maps** - Key-value dictionaries
- **Bytes** - Binary data
- **Enums** - Algebraic data types

---

## Value Types Recap

Before diving in, remember how to create basic values:

```c
// Primitives
hlffi_value* i = hlffi_value_int(vm, 42);
hlffi_value* f = hlffi_value_float(vm, 3.14);
hlffi_value* b = hlffi_value_bool(vm, true);
hlffi_value* s = hlffi_value_string(vm, "hello");
hlffi_value* n = hlffi_value_null(vm);

// Don't forget to free!
hlffi_value_free(i);
hlffi_value_free(f);
hlffi_value_free(b);
hlffi_value_free(s);
hlffi_value_free(n);
```

And how to extract values:

```c
int i = hlffi_value_as_int(val, 0);       // 0 is fallback
double f = hlffi_value_as_float(val, 0.0);
bool b = hlffi_value_as_bool(val, false);
char* s = hlffi_value_as_string(val);     // Must free() this!
bool is_null = hlffi_value_is_null(val);
```

---

## Arrays

Arrays let you pass lists of data between C and Haxe.

### Creating an Array

```c
// Create an empty Int array with 5 slots
hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 5);

// Fill it with values
for (int i = 0; i < 5; i++)
{
    hlffi_value* val = hlffi_value_int(vm, i * 10);
    hlffi_array_set(vm, arr, i, val);
    hlffi_value_free(val);
}

// Cleanup when done
hlffi_value_free(arr);
```

### Common Element Types

| Haxe Type | C Type Constant |
|-----------|-----------------|
| `Int` | `&hlt_i32` |
| `Float` | `&hlt_f64` |
| `Bool` | `&hlt_bool` |
| `String` | `&hlt_bytes` |
| `Dynamic` | `NULL` |

### Reading from an Array

```c
// Get array length
int len = hlffi_array_length(arr);
printf("Array has %d elements\n", len);

// Read each element
for (int i = 0; i < len; i++)
{
    hlffi_value* elem = hlffi_array_get(vm, arr, i);
    int value = hlffi_value_as_int(elem, 0);
    printf("[%d] = %d\n", i, value);
    hlffi_value_free(elem);
}
```

### Passing Arrays to Haxe

**Haxe:**
```haxe
class Stats
{
    public static function sum(numbers:Array<Int>):Int
    {
        var total = 0;
        for (n in numbers) total += n;
        return total;
    }
}
```

**C:**
```c
// Create array with values [10, 20, 30]
hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 3);

hlffi_value* v1 = hlffi_value_int(vm, 10);
hlffi_value* v2 = hlffi_value_int(vm, 20);
hlffi_value* v3 = hlffi_value_int(vm, 30);

hlffi_array_set(vm, arr, 0, v1);
hlffi_array_set(vm, arr, 1, v2);
hlffi_array_set(vm, arr, 2, v3);

hlffi_value_free(v1);
hlffi_value_free(v2);
hlffi_value_free(v3);

// Pass to Haxe
hlffi_value* args[] = {arr};
hlffi_value* result = hlffi_call_static(vm, "Stats", "sum", 1, args);

int total = hlffi_value_as_int(result, 0);
printf("Sum: %d\n", total);  // 60

hlffi_value_free(result);
hlffi_value_free(arr);
```

### Getting Arrays from Haxe

**Haxe:**
```haxe
class Game
{
    public static function getScores():Array<Int>
    {
        return [100, 200, 150, 300];
    }
}
```

**C:**
```c
hlffi_value* scores = hlffi_call_static(vm, "Game", "getScores", 0, NULL);

int len = hlffi_array_length(scores);
printf("Got %d scores:\n", len);

for (int i = 0; i < len; i++)
{
    hlffi_value* elem = hlffi_array_get(vm, scores, i);
    printf("  Score %d: %d\n", i, hlffi_value_as_int(elem, 0));
    hlffi_value_free(elem);
}

hlffi_value_free(scores);
```

---

## Maps (Dictionaries)

Maps store key-value pairs.

### Creating a Map

```c
// Create Map<String, Int>
hlffi_value* scores = hlffi_map_new(vm, &hlt_bytes, &hlt_i32);

// Add entries
hlffi_value* key1 = hlffi_value_string(vm, "Alice");
hlffi_value* val1 = hlffi_value_int(vm, 100);
hlffi_map_set(vm, scores, key1, val1);
hlffi_value_free(key1);
hlffi_value_free(val1);

hlffi_value* key2 = hlffi_value_string(vm, "Bob");
hlffi_value* val2 = hlffi_value_int(vm, 150);
hlffi_map_set(vm, scores, key2, val2);
hlffi_value_free(key2);
hlffi_value_free(val2);

// Don't forget to free the map
hlffi_value_free(scores);
```

### Reading from a Map

```c
// Look up a value
hlffi_value* key = hlffi_value_string(vm, "Alice");
hlffi_value* val = hlffi_map_get(vm, scores, key);

if (val)
{
    int score = hlffi_value_as_int(val, 0);
    printf("Alice's score: %d\n", score);
    hlffi_value_free(val);
}
else
{
    printf("Alice not found\n");
}

hlffi_value_free(key);
```

### Checking if Key Exists

```c
hlffi_value* key = hlffi_value_string(vm, "Charlie");

if (hlffi_map_exists(vm, scores, key))
{
    printf("Charlie is in the map\n");
}
else
{
    printf("Charlie is not in the map\n");
}

hlffi_value_free(key);
```

### Iterating Over a Map

```c
// Get all keys
hlffi_value* keys = hlffi_map_keys(vm, scores);
int count = hlffi_array_length(keys);

printf("Players (%d):\n", count);

for (int i = 0; i < count; i++)
{
    // Get key
    hlffi_value* key = hlffi_array_get(vm, keys, i);
    char* name = hlffi_value_as_string(key);

    // Get value for this key
    hlffi_value* val = hlffi_map_get(vm, scores, key);
    int score = hlffi_value_as_int(val, 0);

    printf("  %s: %d\n", name, score);

    free(name);
    hlffi_value_free(key);
    hlffi_value_free(val);
}

hlffi_value_free(keys);
```

### Map Operations

```c
// Get size
int size = hlffi_map_size(scores);

// Remove entry
hlffi_value* key = hlffi_value_string(vm, "Alice");
hlffi_map_remove(vm, scores, key);
hlffi_value_free(key);

// Clear all entries
hlffi_map_clear(scores);
```

---

## Bytes (Binary Data)

Bytes are used for raw binary data - files, network packets, images, etc.

### Creating Bytes

```c
// Create empty buffer
hlffi_value* buf = hlffi_bytes_new(vm, 1024);  // 1 KB

// Create from existing data
unsigned char data[] = {0x48, 0x45, 0x4C, 0x4C, 0x4F};  // "HELLO"
hlffi_value* bytes = hlffi_bytes_from_data(vm, data, 5);

// Create from string
hlffi_value* text = hlffi_bytes_from_string(vm, "Hello World");

hlffi_value_free(buf);
hlffi_value_free(bytes);
hlffi_value_free(text);
```

### Reading Bytes

```c
// Get length
int len = hlffi_bytes_get_length(bytes);

// Get single byte
int first_byte = hlffi_bytes_get(bytes, 0);  // 0-255 or -1 on error

// Get direct pointer (zero-copy!)
unsigned char* ptr = (unsigned char*)hlffi_bytes_get_ptr(bytes);
for (int i = 0; i < len; i++) {
    printf("%02X ", ptr[i]);
}
printf("\n");
```

### Writing Bytes

```c
// Set single byte
hlffi_bytes_set(bytes, 0, 0xFF);

// Direct pointer access (fastest)
unsigned char* ptr = (unsigned char*)hlffi_bytes_get_ptr(bytes);
ptr[0] = 0x00;
ptr[1] = 0xFF;

// Fill range with value
hlffi_bytes_fill(bytes, 0, 10, 0x00);  // Fill first 10 bytes with 0
```

### Converting to String

```c
// Convert bytes to C string
char* str = hlffi_bytes_to_string(bytes, len);
printf("As string: %s\n", str);
free(str);  // Must free!
```

### Example: File-like Operations

```c
// Simulate reading a file header
hlffi_value* file_data = hlffi_bytes_from_data(vm, raw_file, file_size);

// Read magic number (first 4 bytes)
unsigned char* ptr = (unsigned char*)hlffi_bytes_get_ptr(file_data);
uint32_t magic = *(uint32_t*)ptr;

if (magic == 0x464C4548)  // "HELF"
{
    printf("Valid file format!\n");
}

hlffi_value_free(file_data);
```

---

## Enums

Haxe enums are algebraic data types - they can have multiple constructors with different parameters.

### Haxe Side

```haxe
enum Option<T>
{
    None;
    Some(value:T);
}

enum Result<T, E>
{
    Ok(value:T);
    Err(error:E);
}
```

### Checking Enum Constructors

```c
// Get the constructor index (0 = None, 1 = Some for Option)
int idx = hlffi_enum_get_index(opt);

// Check by index
if (hlffi_enum_is(opt, 0))
{
    printf("It's None\n");
}
else if (hlffi_enum_is(opt, 1))
{
    printf("It's Some\n");
}

// Check by name (easier to read)
if (hlffi_enum_is_named(opt, "None"))
{
    printf("Got None\n");
}
else if (hlffi_enum_is_named(opt, "Some"))
{
    printf("Got Some\n");
}
```

### Getting Enum Parameters

```c
// If it's Some(value), get the value
if (hlffi_enum_is_named(opt, "Some"))
{
    hlffi_value* param = hlffi_enum_get_param(opt, 0);  // First parameter
    int value = hlffi_value_as_int(param, 0);
    printf("Some(%d)\n", value);
    hlffi_value_free(param);
}
```

### Creating Enums

```c
// Create Option.None (no parameters)
hlffi_value* none = hlffi_enum_alloc_simple(vm, "Option", 0);

// Create Option.Some(42)
hlffi_value* val = hlffi_value_int(vm, 42);
hlffi_value* params[] = {val};
hlffi_value* some = hlffi_enum_alloc(vm, "Option", 1, 1, params);
hlffi_value_free(val);

hlffi_value_free(none);
hlffi_value_free(some);
```

### Pattern Matching Example

**Haxe:**
```haxe
enum Response
{
    Success(data:String);
    Error(code:Int, message:String);
    Pending;
}

class Api
{
    public static function fetch():Response
    {
        // Returns one of the enum variants
        return Response.Success("Hello!");
    }
}
```

**C:**
```c
hlffi_value* response = hlffi_call_static(vm, "Api", "fetch", 0, NULL);

if (hlffi_enum_is_named(response, "Success"))
{
    hlffi_value* data = hlffi_enum_get_param(response, 0);
    char* str = hlffi_value_as_string(data);
    printf("Success: %s\n", str);
    free(str);
    hlffi_value_free(data);
}
else if (hlffi_enum_is_named(response, "Error"))
{
    hlffi_value* code_val = hlffi_enum_get_param(response, 0);
    hlffi_value* msg_val = hlffi_enum_get_param(response, 1);

    int code = hlffi_value_as_int(code_val, 0);
    char* msg = hlffi_value_as_string(msg_val);

    printf("Error %d: %s\n", code, msg);

    free(msg);
    hlffi_value_free(code_val);
    hlffi_value_free(msg_val);
}
else if (hlffi_enum_is_named(response, "Pending"))
{
    printf("Still pending...\n");
}

hlffi_value_free(response);
```

---

## Type Checking

You can check the type of values at runtime.

### Check Object Type

```c
// Check if object is instance of a class
if (hlffi_is_instance_of(obj, "Player"))
{
    printf("It's a Player!\n");
}

if (hlffi_is_instance_of(obj, "Entity"))
{
    printf("It's some kind of Entity\n");
}
```

### Get Type Information

```c
// Find a type by name
hlffi_type* player_type = hlffi_find_type(vm, "Player");

if (player_type)
{
    // Get type name
    const char* name = hlffi_type_get_name(player_type);
    printf("Found type: %s\n", name);

    // Get field count
    int fields = hlffi_class_get_field_count(player_type);
    printf("Has %d fields\n", fields);
}
```

---

## Complete Example

Here's a full example demonstrating data exchange:

**Haxe: inventory.hx**
```haxe
enum ItemRarity
{
    Common;
    Rare;
    Epic;
    Legendary;
}

class Item
{
    public var name:String;
    public var rarity:ItemRarity;
    public var value:Int;

    public function new(name:String, rarity:ItemRarity, value:Int)
    {
        this.name = name;
        this.rarity = rarity;
        this.value = value;
    }
}

class Inventory
{
    public static var items:Array<Item> = [];

    public static function addItem(item:Item):Void
    {
        items.push(item);
        trace('Added: ${item.name}');
    }

    public static function getItems():Array<Item>
    {
        return items;
    }

    public static function getTotalValue():Int
    {
        var total = 0;
        for (item in items)
        {
            total += item.value;
        }
        return total;
    }
}

class Main
{
    public static function main()
    {
        trace("Inventory system loaded");
    }
}
```

**C: main.c**
```c
#include <stdio.h>
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "inventory.hl");
    hlffi_call_entry(vm);

    // Create items with different rarities

    // Item 1: Common sword
    hlffi_value* name1 = hlffi_value_string(vm, "Iron Sword");
    hlffi_value* rarity1 = hlffi_enum_alloc_simple(vm, "ItemRarity", 0);  // Common
    hlffi_value* value1 = hlffi_value_int(vm, 50);
    hlffi_value* args1[] = {name1, rarity1, value1};
    hlffi_value* sword = hlffi_new(vm, "Item", 3, args1);
    hlffi_value_free(name1);
    hlffi_value_free(rarity1);
    hlffi_value_free(value1);

    // Item 2: Epic armor
    hlffi_value* name2 = hlffi_value_string(vm, "Dragon Armor");
    hlffi_value* rarity2 = hlffi_enum_alloc_simple(vm, "ItemRarity", 2);  // Epic
    hlffi_value* value2 = hlffi_value_int(vm, 500);
    hlffi_value* args2[] = {name2, rarity2, value2};
    hlffi_value* armor = hlffi_new(vm, "Item", 3, args2);
    hlffi_value_free(name2);
    hlffi_value_free(rarity2);
    hlffi_value_free(value2);

    // Add items to inventory
    hlffi_value* add_args1[] = {sword};
    hlffi_call_static(vm, "Inventory", "addItem", 1, add_args1);

    hlffi_value* add_args2[] = {armor};
    hlffi_call_static(vm, "Inventory", "addItem", 1, add_args2);

    // Get total value
    hlffi_value* total = hlffi_call_static(vm, "Inventory", "getTotalValue", 0, NULL);
    printf("Total inventory value: %d gold\n", hlffi_value_as_int(total, 0));
    hlffi_value_free(total);

    // Get and display all items
    hlffi_value* items = hlffi_call_static(vm, "Inventory", "getItems", 0, NULL);
    int count = hlffi_array_length(items);

    printf("\nInventory (%d items):\n", count);
    for (int i = 0; i < count; i++)
    {
        hlffi_value* item = hlffi_array_get(vm, items, i);

        // Get item fields
        hlffi_value* name_val = hlffi_get_field(item, "name");
        hlffi_value* rarity_val = hlffi_get_field(item, "rarity");
        hlffi_value* value_val = hlffi_get_field(item, "value");

        char* name = hlffi_value_as_string(name_val);
        char* rarity = hlffi_enum_get_name(rarity_val);
        int value = hlffi_value_as_int(value_val, 0);

        printf("  - %s (%s) - %d gold\n", name, rarity, value);

        free(name);
        free(rarity);
        hlffi_value_free(name_val);
        hlffi_value_free(rarity_val);
        hlffi_value_free(value_val);
        hlffi_value_free(item);
    }

    // Cleanup
    hlffi_value_free(items);
    hlffi_value_free(sword);
    hlffi_value_free(armor);
    hlffi_close(vm);
    hlffi_destroy(vm);

    return 0;
}
```

**Output:**
```
Inventory system loaded
Added: Iron Sword
Added: Dragon Armor
Total inventory value: 550 gold

Inventory (2 items):
  - Iron Sword (Common) - 50 gold
  - Dragon Armor (Epic) - 500 gold
```

---

## Quick Reference

### Arrays
```c
hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, size);
int len = hlffi_array_length(arr);
hlffi_value* elem = hlffi_array_get(vm, arr, index);
hlffi_array_set(vm, arr, index, value);
hlffi_array_push(vm, arr, value);
```

### Maps
```c
hlffi_value* map = hlffi_map_new(vm, key_type, val_type);
hlffi_map_set(vm, map, key, value);
hlffi_value* val = hlffi_map_get(vm, map, key);
bool exists = hlffi_map_exists(vm, map, key);
hlffi_map_remove(vm, map, key);
int size = hlffi_map_size(map);
```

### Bytes
```c
hlffi_value* bytes = hlffi_bytes_new(vm, size);
hlffi_value* bytes = hlffi_bytes_from_data(vm, data, len);
void* ptr = hlffi_bytes_get_ptr(bytes);
int len = hlffi_bytes_get_length(bytes);
```

### Enums
```c
bool is_variant = hlffi_enum_is_named(val, "VariantName");
hlffi_value* param = hlffi_enum_get_param(val, index);
hlffi_value* enum_val = hlffi_enum_alloc_simple(vm, "EnumName", index);
```

---

## What's Next?

Now that you can exchange complex data, let's learn about:
- Integration modes (threaded vs non-threaded)
- Hot reload for fast iteration
- C callbacks from Haxe
- Performance optimization

**[Next: Advanced Topics →](GUIDE_04_ADVANCED.md)**
