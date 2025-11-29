# HLFFI API Reference - Exception Handling

**[← Callbacks](API_15_CALLBACKS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Performance →](API_17_PERFORMANCE.md)**

Safe error handling for Haxe exceptions and stack traces.

---

## Overview

Haxe code can throw exceptions. Use try-call functions to catch them safely:

```c
// Without exception handling (crashes on throw):
hlffi_call_static(vm, "Game", "loadLevel", 1, args);  // May crash

// With exception handling (safe):
hlffi_call_result res = hlffi_try_call_static(vm, "Game", "loadLevel", 1, args);
if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    fprintf(stderr, "Error: %s\n", msg);
    free(msg);
    hlffi_value_free(res.exception);
}
else
{
    // Use res.value
    hlffi_value_free(res.value);
}
```

---

## Quick Reference

| Function | Purpose |
|----------|---------|
| `hlffi_try_call_static(...)` | Call static method (exception-safe) |
| `hlffi_try_call_method(...)` | Call instance method (exception-safe) |
| `hlffi_get_exception_message(exc)` | Get exception message |
| `hlffi_get_exception_stack(exc)` | Get stack trace |
| `hlffi_is_exception(val)` | Check if value is exception |

**Complete Guide:** See `docs/PHASE6_COMPLETE.md`

---

## Call Result Type

```c

{
    hlffi_value* value;      // Return value (NULL if exception)
    hlffi_value* exception;  // Exception (NULL if success)
} hlffi_call_result;
```

**Usage Pattern:**
```c
hlffi_call_result res = hlffi_try_call_static(...);

if (res.exception)
{
    // Handle exception
    hlffi_value_free(res.exception);
}
else
{
    // Use res.value
    if (res.value)
    {
        hlffi_value_free(res.value);
    }
}
```

---

## Calling with Exception Safety

### Try Call Static Method

**Signature:**
```c
hlffi_call_result hlffi_try_call_static(
    hlffi_vm* vm,
    const char* class_name,
    const char* method_name,
    int argc,
    hlffi_value** argv
)
```

**Returns:** `hlffi_call_result` with either `value` or `exception` set

**Example:**
```c
hlffi_value* args[] = {hlffi_value_int(vm, 5)};
hlffi_call_result res = hlffi_try_call_static(vm, "Game", "loadLevel", 1, args);
hlffi_value_free(args[0]);

if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    fprintf(stderr, "Failed to load level: %s\n", msg);
    free(msg);
    hlffi_value_free(res.exception);
}
else
{
    printf("Level loaded successfully\n");
    if (res.value)
    {
        hlffi_value_free(res.value);
    }
}
```

---

### Try Call Instance Method

**Signature:**
```c
hlffi_call_result hlffi_try_call_method(
    hlffi_value* obj,
    const char* method_name,
    int argc,
    hlffi_value** argv
)
```

**Returns:** `hlffi_call_result` with either `value` or `exception` set

**Example:**
```c
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
hlffi_value* dmg = hlffi_value_int(vm, 100);
hlffi_value* args[] = {dmg};

hlffi_call_result res = hlffi_try_call_method(player, "takeDamage", 1, args);
hlffi_value_free(dmg);

if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    fprintf(stderr, "Error applying damage: %s\n", msg);
    free(msg);
    hlffi_value_free(res.exception);
}
else
{
    if (res.value)
    {
        hlffi_value_free(res.value);
    }
}

hlffi_value_free(player);
```

---

## Exception Inspection

### Get Exception Message

**Signature:**
```c
char* hlffi_get_exception_message(hlffi_value* exception)
```

**Returns:** Exception message string (**caller must free()**)

**Example:**
```c
if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    printf("Exception: %s\n", msg);
    free(msg);
}
```

---

### Get Stack Trace

**Signature:**
```c
char* hlffi_get_exception_stack(hlffi_value* exception)
```

**Returns:** Full stack trace string (**caller must free()**)

**Example:**
```c
if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    char* stack = hlffi_get_exception_stack(res.exception);

    fprintf(stderr, "Exception: %s\n", msg);
    fprintf(stderr, "Stack trace:\n%s\n", stack);

    free(msg);
    free(stack);
    hlffi_value_free(res.exception);
}
```

---

### Check if Value is Exception

**Signature:**
```c
bool hlffi_is_exception(hlffi_value* value)
```

**Returns:** `true` if value is an exception object

**Example:**
```c
if (hlffi_is_exception(res.value))
{
    printf("This is an exception\n");
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

    // Try calling method that may throw:
    hlffi_value* filename = hlffi_value_string(vm, "nonexistent.dat");
    hlffi_value* args[] = {filename};

    hlffi_call_result res = hlffi_try_call_static(
        vm, "FileLoader", "load", 1, args
    );
    hlffi_value_free(filename);

    if (res.exception)
    {
        // Extract error info:
        char* msg = hlffi_get_exception_message(res.exception);
        char* stack = hlffi_get_exception_stack(res.exception);

        fprintf(stderr, "=== Exception Caught ===\n");
        fprintf(stderr, "Message: %s\n", msg);
        fprintf(stderr, "\nStack trace:\n%s\n", stack);

        free(msg);
        free(stack);
        hlffi_value_free(res.exception);

        // Recovery logic:
        printf("Falling back to default file...\n");
        filename = hlffi_value_string(vm, "default.dat");
        hlffi_value* args2[] = {filename};
        hlffi_call_static(vm, "FileLoader", "load", 1, args2);
        hlffi_value_free(filename);

    }
else
{
        printf("File loaded successfully\n");
        if (res.value)
        {
            hlffi_value_free(res.value);
        }
    }

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class FileLoader
{
    public static function load(filename:String):Void
    {
        if (!sys.FileSystem.exists(filename))
        {
            throw 'File not found: $filename';
        }
        var content = sys.io.File.getContent(filename);
        trace('Loaded: $filename');
    }
}
```

---

## Best Practices

### 1. Use Try-Call for External Operations

```c
// ✅ GOOD - Catch file I/O exceptions
hlffi_call_result res = hlffi_try_call_static(vm, "File", "load", 1, args);
if (res.exception)
{
    // Handle error gracefully
}

// ❌ RISKY - May crash
hlffi_call_static(vm, "File", "load", 1, args);  // Crashes on throw
```

### 2. Always Free Exception Objects

```c
// ✅ GOOD - Free exception
if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    free(msg);
    hlffi_value_free(res.exception);  // MUST free
}

// ❌ BAD - Memory leak
if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    free(msg);
    // Missing: hlffi_value_free(res.exception);
}
```

### 3. Free Both Message and Stack

```c
// ✅ GOOD
char* msg = hlffi_get_exception_message(res.exception);
char* stack = hlffi_get_exception_stack(res.exception);
free(msg);
free(stack);

// ❌ BAD - Memory leak
char* msg = hlffi_get_exception_message(res.exception);
char* stack = hlffi_get_exception_stack(res.exception);
free(msg);
// Missing: free(stack);
```

### 4. Check Both Value and Exception

```c
// ✅ GOOD - Handle both cases
hlffi_call_result res = hlffi_try_call_static(...);
if (res.exception)
{
    // Handle exception
    hlffi_value_free(res.exception);
}
else
{
    // Use value
    if (res.value)
    {
        hlffi_value_free(res.value);
    }
}

// ❌ BAD - Only check exception
if (res.exception)
{
    hlffi_value_free(res.exception);
}
// Missing: free res.value if present
```

---

## Common Patterns

### Retry on Exception

```c
hlffi_call_result res = hlffi_try_call_static(vm, "Network", "connect", 0, NULL);

if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);
    fprintf(stderr, "Connection failed: %s\n", msg);
    fprintf(stderr, "Retrying...\n");
    free(msg);
    hlffi_value_free(res.exception);

    // Retry:
    res = hlffi_try_call_static(vm, "Network", "connect", 0, NULL);
}
```

### Log and Continue

```c
hlffi_call_result res = hlffi_try_call_static(vm, "Analytics", "track", 1, args);

if (res.exception)
{
    // Log but don't fail:
    char* msg = hlffi_get_exception_message(res.exception);
    fprintf(stderr, "Analytics error (non-fatal): %s\n", msg);
    free(msg);
    hlffi_value_free(res.exception);
}
else
{
    if (res.value) hlffi_value_free(res.value);
}

// Continue with main logic...
```

### Exception Type Checking

```c
if (res.exception)
{
    char* msg = hlffi_get_exception_message(res.exception);

    if (strstr(msg, "File not found"))
    {
        printf("Using default file...\n");
    } else if (strstr(msg, "Permission denied"))
    {
        fprintf(stderr, "Access error\n");
    }
else
{
        fprintf(stderr, "Unknown error: %s\n", msg);
    }

    free(msg);
    hlffi_value_free(res.exception);
}
```

---

## Performance Note

**Exception handling has ~5% overhead** compared to regular calls. Use `hlffi_try_call_*()` only when:
- Calling external I/O operations (file, network)
- Calling user-provided code
- Errors are expected and recoverable

For performance-critical hot paths where exceptions are impossible, use regular `hlffi_call_*()` functions.

---

**[← Callbacks](API_15_CALLBACKS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Performance →](API_17_PERFORMANCE.md)**