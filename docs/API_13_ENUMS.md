# HLFFI API Reference - Enums (Algebraic Data Types)

**[← Bytes](API_12_BYTES.md)** | **[Back to Index](API_REFERENCE.md)** | **[Abstracts →](API_14_ABSTRACTS.md)**

Work with Haxe enums (algebraic data types with constructors and parameters).

---

## Quick Reference

### Introspection

| Function | Purpose |
|----------|---------|
| `hlffi_enum_get_construct_count(vm, name)` | Get number of constructors |
| `hlffi_enum_get_construct_name(vm, name, idx)` | Get constructor name by index |
| `hlffi_enum_get_index(value)` | Get constructor index of value |
| `hlffi_enum_get_name(value)` | Get constructor name of value |
| `hlffi_enum_get_param_count(value)` | Get number of parameters |
| `hlffi_enum_get_param(value, idx)` | Get parameter value |

### Construction

| Function | Purpose |
|----------|---------|
| `hlffi_enum_alloc_simple(vm, type, idx)` | Create enum (no parameters) |
| `hlffi_enum_alloc(vm, type, idx, argc, argv)` | Create enum with parameters |

### Checking

| Function | Purpose |
|----------|---------|
| `hlffi_enum_is(value, idx)` | Check if value is constructor index |
| `hlffi_enum_is_named(value, name)` | Check if value is constructor name |

**Complete Guide:** See `docs/PHASE4_INSTANCE_MEMBERS.md`

---

## Haxe Enum Overview

```haxe
enum Option
{
    None;           // Constructor 0, no parameters
    Some(value:Int);  // Constructor 1, 1 parameter
}

enum Result<T, E>
{
    Ok(value:T);
    Err(error:E);
}
```

---

## Type Introspection

### Get Constructor Count

**Signature:**
```c
int hlffi_enum_get_construct_count(hlffi_vm* vm, const char* type_name)
```

**Returns:** Number of constructors, or -1 on error

**Example:**
```c
int count = hlffi_enum_get_construct_count(vm, "Option");
printf("Option has %d constructors\n", count);  // 2 (None, Some)
```

---

### Get Constructor Name by Index

**Signature:**
```c
char* hlffi_enum_get_construct_name(hlffi_vm* vm, const char* type_name, int index)
```

**Returns:** Constructor name (**caller must free()**), or NULL on error

**Example:**
```c
char* name0 = hlffi_enum_get_construct_name(vm, "Option", 0);
char* name1 = hlffi_enum_get_construct_name(vm, "Option", 1);

printf("Constructors: %s, %s\n", name0, name1);  // "None, Some"

free(name0);
free(name1);
```

---

## Value Inspection

### Get Constructor Index

**Signature:**
```c
int hlffi_enum_get_index(hlffi_value* value)
```

**Returns:** Constructor index, or -1 if not enum

**Example:**
```c
// Given: Option.Some(42)
int idx = hlffi_enum_get_index(opt);
printf("Constructor index: %d\n", idx);  // 1 (Some)
```

---

### Get Constructor Name

**Signature:**
```c
char* hlffi_enum_get_name(hlffi_value* value)
```

**Returns:** Constructor name (**caller must free()**), or NULL if not enum

**Example:**
```c
char* name = hlffi_enum_get_name(opt);
printf("Constructor: %s\n", name);  // "Some"
free(name);
```

---

### Get Parameter Count

**Signature:**
```c
int hlffi_enum_get_param_count(hlffi_value* value)
```

**Returns:** Number of parameters, or -1 if not enum

**Example:**
```c
int nparam = hlffi_enum_get_param_count(opt);
printf("Parameters: %d\n", nparam);  // 1 for Some(value)
```

---

### Get Parameter Value

**Signature:**
```c
hlffi_value* hlffi_enum_get_param(hlffi_value* value, int param_index)
```

**Returns:** Parameter value, or NULL if out of bounds

**Example:**
```c
// Given: Option.Some(42)
hlffi_value* param = hlffi_enum_get_param(opt, 0);
if (param)
{
    int value = hlffi_value_as_int(param, 0);
    printf("Some(%d)\n", value);  // Some(42)
    hlffi_value_free(param);
}
```

---

## Creating Enums

### Create Simple Enum (No Parameters)

**Signature:**
```c
hlffi_value* hlffi_enum_alloc_simple(hlffi_vm* vm, const char* type_name, int index)
```

**Returns:** Enum value, or NULL on error

**Example:**
```c
// Create Option.None (index 0):
hlffi_value* none = hlffi_enum_alloc_simple(vm, "Option", 0);
```

---

### Create Enum with Parameters

**Signature:**
```c
hlffi_value* hlffi_enum_alloc(hlffi_vm* vm, const char* type_name, int index,
                              int argc, hlffi_value** argv)
```

**Parameters:**
- `type_name` - Enum type name
- `index` - Constructor index
- `argc` - Number of parameters
- `argv` - Array of parameter values

**Returns:** Enum value, or NULL on error

**Example:**
```c
// Create Option.Some(42):
hlffi_value* val = hlffi_value_int(vm, 42);
hlffi_value* params[] = {val};
hlffi_value* some = hlffi_enum_alloc(vm, "Option", 1, 1, params);
hlffi_value_free(val);
```

---

## Checking Constructors

### Check by Index

**Signature:**
```c
bool hlffi_enum_is(hlffi_value* value, int index)
```

**Returns:** `true` if value is constructor at index

**Example:**
```c
if (hlffi_enum_is(opt, 0))
{
    printf("Is None\n");
} else if (hlffi_enum_is(opt, 1))
{
    printf("Is Some\n");
}
```

---

### Check by Name

**Signature:**
```c
bool hlffi_enum_is_named(hlffi_value* value, const char* name)
```

**Returns:** `true` if value is constructor with name

**Example:**
```c
if (hlffi_enum_is_named(opt, "Some"))
{
    hlffi_value* val = hlffi_enum_get_param(opt, 0);
    printf("Some(%d)\n", hlffi_value_as_int(val, 0));
    hlffi_value_free(val);
} else if (hlffi_enum_is_named(opt, "None"))
{
    printf("None\n");
}
```

---

## Complete Example

```c
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Enumerate constructors:
    int count = hlffi_enum_get_construct_count(vm, "Option");
    printf("Option constructors (%d):\n", count);
    for (int i = 0; i < count; i++)
    {
        char* name = hlffi_enum_get_construct_name(vm, "Option", i);
        printf("  [%d] %s\n", i, name);
        free(name);
    }

    // Create Option.None:
    hlffi_value* none = hlffi_enum_alloc_simple(vm, "Option", 0);
    char* name = hlffi_enum_get_name(none);
    printf("Created: %s\n", name);  // "None"
    free(name);

    // Create Option.Some(42):
    hlffi_value* val = hlffi_value_int(vm, 42);
    hlffi_value* params[] = {val};
    hlffi_value* some = hlffi_enum_alloc(vm, "Option", 1, 1, params);
    hlffi_value_free(val);

    // Inspect Some:
    if (hlffi_enum_is_named(some, "Some"))
    {
        hlffi_value* param = hlffi_enum_get_param(some, 0);
        printf("Some(%d)\n", hlffi_value_as_int(param, 0));  // Some(42)
        hlffi_value_free(param);
    }

    // Pattern matching:
    hlffi_value* opt = some;  // Could be either None or Some
    if (hlffi_enum_is(opt, 0))
    {
        printf("Matched None\n");
    } else if (hlffi_enum_is(opt, 1))
    {
        hlffi_value* v = hlffi_enum_get_param(opt, 0);
        printf("Matched Some(%d)\n", hlffi_value_as_int(v, 0));
        hlffi_value_free(v);
    }

    // Cleanup:
    hlffi_value_free(none);
    hlffi_value_free(some);
    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
enum Option
{
    None;
    Some(value:Int);
}

class Main
{
    public static function main()
    {
        var opt1 = Option.None;
        var opt2 = Option.Some(42);

        switch (opt2)
        {
            case None:
                trace("None");
            case Some(v):
                trace('Some($v)');
        }
    }
}
```

---

## Best Practices

### 1. Use Pattern Matching

```c
// ✅ GOOD - Check constructor before accessing params
if (hlffi_enum_is_named(result, "Ok"))
{
    hlffi_value* value = hlffi_enum_get_param(result, 0);
    printf("Success: %d\n", hlffi_value_as_int(value, 0));
    hlffi_value_free(value);
} else if (hlffi_enum_is_named(result, "Err"))
{
    hlffi_value* err = hlffi_enum_get_param(result, 0);
    char* msg = hlffi_value_as_string(err);
    printf("Error: %s\n", msg);
    free(msg);
    hlffi_value_free(err);
}

// ❌ BAD - Assume constructor
hlffi_value* value = hlffi_enum_get_param(result, 0);  // Could be Err!
```

### 2. Free Constructor Names

```c
// ✅ GOOD
char* name = hlffi_enum_get_name(opt);
printf("%s\n", name);
free(name);  // MUST free

// ❌ BAD - Memory leak
char* name = hlffi_enum_get_name(opt);
printf("%s\n", name);
// Missing: free(name);
```

### 3. Free Parameters

```c
// ✅ GOOD
hlffi_value* param = hlffi_enum_get_param(opt, 0);
if (param)
{
    // ... use param ...
    hlffi_value_free(param);
}

// ❌ BAD - Memory leak
hlffi_value* param = hlffi_enum_get_param(opt, 0);
// ... use param ...
// Missing: hlffi_value_free(param);
```

---

## Common Patterns

### Option Type

```c
// Check for value:
if (hlffi_enum_is_named(opt, "Some"))
{
    hlffi_value* val = hlffi_enum_get_param(opt, 0);
    printf("Has value: %d\n", hlffi_value_as_int(val, 0));
    hlffi_value_free(val);
}
else
{
    printf("No value\n");
}
```

### Result Type

```c
// Handle success/error:
if (hlffi_enum_is_named(res, "Ok"))
{
    hlffi_value* val = hlffi_enum_get_param(res, 0);
    printf("Success\n");
    hlffi_value_free(val);
}
else
{
    hlffi_value* err = hlffi_enum_get_param(res, 0);
    char* msg = hlffi_value_as_string(err);
    fprintf(stderr, "Error: %s\n", msg);
    free(msg);
    hlffi_value_free(err);
}
```

---

**[← Bytes](API_12_BYTES.md)** | **[Back to Index](API_REFERENCE.md)** | **[Abstracts →](API_14_ABSTRACTS.md)**