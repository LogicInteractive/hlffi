# HLFFI API Reference - Error Handling

**[← Utilities](API_18_UTILITIES.md)** | **[Back to Index](API_REFERENCE.md)**

Error codes, error messages, and error handling patterns.

---

## Overview

HLFFI uses two error handling patterns:

1. **Error codes** - Functions return `hlffi_error_code`
2. **Error messages** - Functions return `NULL`/`false` and set error message via `hlffi_get_error()`

```c
// Pattern 1: Error codes
hlffi_error_code result = hlffi_load_file(vm, "game.hl");
if (result != HLFFI_OK) {
    fprintf(stderr, "Load failed: %s\n", hlffi_get_error_string(result));
}

// Pattern 2: Error messages
hlffi_value* val = hlffi_get_field(obj, "health");
if (!val) {
    fprintf(stderr, "Field access failed: %s\n", hlffi_get_error(vm));
}
```

---

## Quick Reference

### Error Functions

| Function | Purpose |
|----------|---------|
| `hlffi_get_error(vm)` | Get last error message |
| `hlffi_get_error_string(code)` | Convert error code to string |

### Error Codes

| Code | Meaning |
|------|---------|
| `HLFFI_OK` | Success |
| `HLFFI_ERROR_NULL_VM` | NULL VM parameter |
| `HLFFI_ERROR_ALREADY_INITIALIZED` | VM already initialized |
| `HLFFI_ERROR_NOT_INITIALIZED` | VM not initialized |
| `HLFFI_ERROR_LOAD_FAILED` | Bytecode load failed |
| `HLFFI_ERROR_ENTRY_POINT_NOT_FOUND` | No main() function |
| `HLFFI_ERROR_CALL_FAILED` | Function call failed |
| `HLFFI_ERROR_INVALID_MODE` | Invalid integration mode |

**Complete List:** See `include/hlffi.h` for all error codes

---

## Error Code Enum

```c
typedef enum {
    HLFFI_OK = 0,                          // Success
    HLFFI_ERROR_NULL_VM,                   // VM is NULL
    HLFFI_ERROR_ALREADY_INITIALIZED,       // hlffi_init() called twice
    HLFFI_ERROR_NOT_INITIALIZED,           // hlffi_init() not called
    HLFFI_ERROR_LOAD_FAILED,               // Failed to load bytecode
    HLFFI_ERROR_ENTRY_POINT_NOT_FOUND,     // No main() function
    HLFFI_ERROR_CALL_FAILED,               // Function call failed
    HLFFI_ERROR_INVALID_MODE,              // Invalid integration mode
    HLFFI_ERROR_THREAD_START_FAILED,       // Failed to start VM thread
    HLFFI_ERROR_THREAD_ALREADY_STARTED,    // Thread already running
    HLFFI_ERROR_THREAD_NOT_STARTED,        // Thread not running
    HLFFI_ERROR_OUT_OF_MEMORY,             // Memory allocation failed
    HLFFI_ERROR_INVALID_ARGUMENT,          // Invalid argument
    HLFFI_ERROR_TYPE_NOT_FOUND,            // Type not found
    HLFFI_ERROR_FIELD_NOT_FOUND,           // Field not found
    HLFFI_ERROR_METHOD_NOT_FOUND,          // Method not found
    HLFFI_ERROR_INVALID_TYPE,              // Wrong type
    HLFFI_ERROR_HOT_RELOAD_NOT_ENABLED,    // Hot reload disabled
    HLFFI_ERROR_HOT_RELOAD_FAILED          // Hot reload failed
} hlffi_error_code;
```

---

## Getting Error Messages

### Get Last Error

**Signature:**
```c
const char* hlffi_get_error(hlffi_vm* vm)
```

**Returns:** Last error message (static string, do not free)

**Example:**
```c
hlffi_value* field = hlffi_get_field(obj, "nonexistent");
if (!field) {
    fprintf(stderr, "Error: %s\n", hlffi_get_error(vm));
    // Output: "Error: Field 'nonexistent' not found in class 'Player'"
}
```

---

### Convert Error Code to String

**Signature:**
```c
const char* hlffi_get_error_string(hlffi_error_code code)
```

**Returns:** Human-readable error message (static string, do not free)

**Example:**
```c
hlffi_error_code result = hlffi_load_file(vm, "missing.hl");
if (result != HLFFI_OK) {
    fprintf(stderr, "Load failed: %s\n", hlffi_get_error_string(result));
    // Output: "Load failed: Failed to load bytecode"
}
```

---

## Error Handling Patterns

### Pattern 1: Check Error Codes

For lifecycle functions:

```c
hlffi_vm* vm = hlffi_create();
if (!vm) {
    fprintf(stderr, "Failed to create VM\n");
    return 1;
}

hlffi_error_code result = hlffi_init(vm, argc, argv);
if (result != HLFFI_OK) {
    fprintf(stderr, "Init failed: %s\n", hlffi_get_error_string(result));
    hlffi_destroy(vm);
    return 1;
}

result = hlffi_load_file(vm, "game.hl");
if (result != HLFFI_OK) {
    fprintf(stderr, "Load failed: %s\n", hlffi_get_error_string(result));
    hlffi_destroy(vm);
    return 1;
}
```

---

### Pattern 2: Check NULL Returns

For value/field/method functions:

```c
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
if (!player) {
    fprintf(stderr, "Failed to create Player: %s\n", hlffi_get_error(vm));
    return;
}

hlffi_value* hp = hlffi_get_field(player, "health");
if (!hp) {
    fprintf(stderr, "Failed to get field: %s\n", hlffi_get_error(vm));
    hlffi_value_free(player);
    return;
}

hlffi_value_free(hp);
hlffi_value_free(player);
```

---

### Pattern 3: Check Boolean Returns

For setter/boolean functions:

```c
hlffi_value* val = hlffi_value_int(vm, 100);

if (!hlffi_set_field(obj, "health", val)) {
    fprintf(stderr, "Failed to set field: %s\n", hlffi_get_error(vm));
}

hlffi_value_free(val);
```

---

## Complete Example

```c
#include "hlffi.h"

int main(int argc, char** argv) {
    // Create VM:
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "FATAL: Failed to create VM\n");
        return 1;
    }

    // Initialize:
    hlffi_error_code result = hlffi_init(vm, argc, argv);
    if (result != HLFFI_OK) {
        fprintf(stderr, "Init error: %s\n", hlffi_get_error_string(result));
        hlffi_destroy(vm);
        return 1;
    }

    // Load bytecode:
    result = hlffi_load_file(vm, "game.hl");
    if (result != HLFFI_OK) {
        fprintf(stderr, "Load error: %s\n", hlffi_get_error_string(result));
        fprintf(stderr, "Details: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    // Call entry point:
    result = hlffi_call_entry(vm);
    if (result != HLFFI_OK) {
        fprintf(stderr, "Entry call error: %s\n", hlffi_get_error_string(result));
        hlffi_destroy(vm);
        return 1;
    }

    // Try to create object:
    hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
    if (!player) {
        fprintf(stderr, "Failed to create Player: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    // Try to access field:
    hlffi_value* hp = hlffi_get_field(player, "health");
    if (!hp) {
        fprintf(stderr, "Failed to get health field: %s\n", hlffi_get_error(vm));
        hlffi_value_free(player);
        hlffi_destroy(vm);
        return 1;
    }

    // Success:
    int health = hlffi_value_as_int(hp, 0);
    printf("Player health: %d\n", health);

    // Cleanup:
    hlffi_value_free(hp);
    hlffi_value_free(player);
    hlffi_destroy(vm);
    return 0;
}
```

---

## Best Practices

### 1. Always Check Return Values

```c
// ✅ GOOD - Check every call
hlffi_error_code result = hlffi_load_file(vm, "game.hl");
if (result != HLFFI_OK) {
    fprintf(stderr, "Error: %s\n", hlffi_get_error_string(result));
    return 1;
}

// ❌ BAD - Ignore errors
hlffi_load_file(vm, "game.hl");  // May fail silently
```

### 2. Clean Up on Error

```c
// ✅ GOOD - Free resources on error
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
if (!player) {
    fprintf(stderr, "Error: %s\n", hlffi_get_error(vm));
    hlffi_destroy(vm);  // Clean up VM
    return 1;
}

// ❌ BAD - Leak resources
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
if (!player) {
    return 1;  // VM leaked!
}
```

### 3. Provide Context in Error Messages

```c
// ✅ GOOD - Contextual error message
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
if (!player) {
    fprintf(stderr, "Failed to create Player object: %s\n", hlffi_get_error(vm));
}

// ❌ LESS HELPFUL - Generic message
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
if (!player) {
    fprintf(stderr, "Error: %s\n", hlffi_get_error(vm));
}
```

### 4. Log Errors for Debugging

```c
// ✅ GOOD - Log errors to file
FILE* log = fopen("hlffi_errors.log", "a");
hlffi_error_code result = hlffi_load_file(vm, "game.hl");
if (result != HLFFI_OK) {
    fprintf(log, "[%s] Load error: %s\n", timestamp(), hlffi_get_error(vm));
    fclose(log);
}
```

---

## Common Error Scenarios

### Bytecode Load Failure

```c
hlffi_error_code result = hlffi_load_file(vm, "game.hl");
if (result == HLFFI_ERROR_LOAD_FAILED) {
    fprintf(stderr, "Failed to load bytecode:\n");
    fprintf(stderr, "  - Check file exists: game.hl\n");
    fprintf(stderr, "  - Check file is valid .hl bytecode\n");
    fprintf(stderr, "  - Check HashLink version compatibility\n");
    fprintf(stderr, "Details: %s\n", hlffi_get_error(vm));
}
```

### Type Not Found

```c
hlffi_type* player_type = hlffi_find_type(vm, "Player");
if (!player_type) {
    fprintf(stderr, "Type 'Player' not found:\n");
    fprintf(stderr, "  - Check class name spelling\n");
    fprintf(stderr, "  - Ensure class is compiled into bytecode\n");
    fprintf(stderr, "  - Check you called hlffi_call_entry() first\n");
}
```

### Method Not Found

```c
hlffi_value* result = hlffi_call_static(vm, "Game", "start", 0, NULL);
if (!result) {
    fprintf(stderr, "Method call failed:\n");
    fprintf(stderr, "  - Check method name spelling\n");
    fprintf(stderr, "  - Verify method is public and static\n");
    fprintf(stderr, "  - Ensure correct argument count\n");
    fprintf(stderr, "Details: %s\n", hlffi_get_error(vm));
}
```

---

## Error Recovery Strategies

### Retry on Failure

```c
int retries = 3;
hlffi_error_code result;

for (int i = 0; i < retries; i++) {
    result = hlffi_load_file(vm, "game.hl");
    if (result == HLFFI_OK) break;

    fprintf(stderr, "Load failed (attempt %d/%d): %s\n",
            i + 1, retries, hlffi_get_error(vm));
    sleep(1);
}

if (result != HLFFI_OK) {
    fprintf(stderr, "All retry attempts failed\n");
    return 1;
}
```

### Fallback Values

```c
hlffi_value* config = hlffi_get_static_field(vm, "Config", "maxPlayers");
int max_players;

if (!config) {
    fprintf(stderr, "Warning: Config.maxPlayers not found, using default\n");
    max_players = 16;  // Fallback
} else {
    max_players = hlffi_value_as_int(config, 16);
    hlffi_value_free(config);
}
```

### Graceful Degradation

```c
// Try advanced feature:
hlffi_value* result = hlffi_call_static(vm, "Graphics", "renderHDR", 0, NULL);

if (!result) {
    // Fall back to basic rendering:
    fprintf(stderr, "HDR not available, using standard rendering\n");
    result = hlffi_call_static(vm, "Graphics", "renderStandard", 0, NULL);
}

if (result) hlffi_value_free(result);
```

---

## Debugging Tips

### Enable Verbose Errors

```c
// Set environment variable:
setenv("HLFFI_VERBOSE", "1", 1);

// Now all errors include stack traces and detailed info
```

### Check HashLink Errors

```c
// HLFFI wraps HashLink errors - check both:
hlffi_error_code result = hlffi_load_file(vm, "game.hl");
if (result != HLFFI_OK) {
    fprintf(stderr, "HLFFI error: %s\n", hlffi_get_error_string(result));
    fprintf(stderr, "HL error: %s\n", hlffi_get_error(vm));
}
```

---

**[← Utilities](API_18_UTILITIES.md)** | **[Back to Index](API_REFERENCE.md)**
