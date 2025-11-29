# HLFFI API Reference - Hot Reload

**[← Threading](API_04_THREADING.md)** | **[Back to Index](API_REFERENCE.md)** | **[Type System →](API_06_TYPE_SYSTEM.md)**

Hot reload allows reloading changed bytecode without restarting the VM.

---

## Overview

**Requirements:**
- HashLink 1.12+ (uses `hl_module_patch()`)
- JIT mode (not HL/C)

**Key Features:**
- In-place code patching
- Function code updated, static variables **persist**
- No VM restart required
- Optional callback for reload notifications
- Automatic file change detection

**Complete Guide:** See `docs/HOT_RELOAD.md`

---

## Functions

### `hlffi_enable_hot_reload()`

**Signature:**
```c
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable)
```

**Description:**
Enables or disables hot reload. **Must be called BEFORE `hlffi_load_file()`**.

**Parameters:**
- `vm` - VM instance
- `enable` - `true` to enable, `false` to disable

**Returns:**
- `HLFFI_OK` - Hot reload configured
- `HLFFI_ERROR_RELOAD_NOT_SUPPORTED` - HashLink too old or HL/C mode

**Example:**
```c
hlffi_enable_hot_reload(vm, true);
hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);

// Later, during development:
hlffi_reload_module(vm, "game.hl");
```

---

### `hlffi_is_hot_reload_enabled()`

**Signature:**
```c
bool hlffi_is_hot_reload_enabled(hlffi_vm* vm)
```

**Description:**
Checks if hot reload is enabled.

**Returns:** `true` if enabled, `false` otherwise

---

### `hlffi_reload_module()`

**Signature:**
```c
hlffi_error_code hlffi_reload_module(hlffi_vm* vm, const char* path)
```

**Description:**
Reloads bytecode from file. Updates function implementations, **preserves static variables**.

**Parameters:**
- `vm` - VM instance
- `path` - Path to new .hl file

**Returns:**
- `HLFFI_OK` - Reload successful
- `HLFFI_ERROR_RELOAD_NOT_ENABLED` - Hot reload not enabled
- `HLFFI_ERROR_RELOAD_FAILED` - Reload failed

**Example:**
```c
if (hlffi_reload_module(vm, "game_v2.hl") == HLFFI_OK)
{
    printf("Reloaded successfully\n");
}
else
{
    fprintf(stderr, "Reload failed: %s\n", hlffi_get_error(vm));
}
```

**What Gets Updated:**
- ✅ Function implementations
- ✅ Method bodies
- ❌ Static variables (they persist!)
- ❌ Type definitions (cannot change structure)

---

### `hlffi_reload_module_memory()`

**Signature:**
```c
hlffi_error_code hlffi_reload_module_memory(hlffi_vm* vm, const void* data, size_t size)
```

**Description:**
Reloads bytecode from memory buffer.

**Parameters:**
- `vm` - VM instance
- `data` - Pointer to new bytecode
- `size` - Size in bytes

---

### `hlffi_set_reload_callback()`

**Signature:**
```c
void hlffi_set_reload_callback(
    hlffi_vm* vm,
    hlffi_reload_callback callback,
    void* userdata
)
```

**Description:**
Sets a callback to be notified when reload completes.

**Callback Signature:**
```c
typedef void (*hlffi_reload_callback)(hlffi_vm* vm, bool success, void* userdata);
```

**Example:**
```c
void on_reload(hlffi_vm* vm, bool success, void* userdata)
{
    if (success)
    {
        printf("✓ Reload successful!\n");
    }
else
{
        printf("✗ Reload failed: %s\n", hlffi_get_error(vm));
    }
}

hlffi_set_reload_callback(vm, on_reload, NULL);
```

---

### `hlffi_check_reload()`

**Signature:**
```c
bool hlffi_check_reload(hlffi_vm* vm)
```

**Description:**
Checks if the loaded file has changed and automatically reloads if needed.

**Returns:** `true` if reload occurred, `false` otherwise

**Example (Game Loop):**
```c
while (running)
{
    hlffi_check_reload(vm);  // Auto-reload on file change
    hlffi_update(vm, dt);
    render();
}
```

---

## Complete Example

```c
#include "hlffi.h"

void on_reload(hlffi_vm* vm, bool success, void* userdata)
{
    if (success)
    {
        printf("✓ Code reloaded successfully\n");
        // Re-initialize if needed
        hlffi_call_static(vm, "Game", "onReload", 0, NULL);
    }
else
{
        printf("✗ Reload failed: %s\n", hlffi_get_error(vm));
    }
}

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    
    // Enable hot reload BEFORE loading:
    hlffi_enable_hot_reload(vm, true);
    hlffi_set_reload_callback(vm, on_reload, NULL);
    
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);
    
    // Development loop:
    while (!should_quit())
    {
        // Auto-reload when file changes:
        hlffi_check_reload(vm);
        
        // Or manual reload on keypress:
        if (key_pressed('R'))
        {
            hlffi_reload_module(vm, "game.hl");
        }
        
        hlffi_update(vm, dt);
        render();
    }

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class Game
{
    public static var score:Int = 0;  // Persists across reloads
    
    public static function getValue():Int
    {
        return 100;  // Change to 200, recompile, reload
    }
    
    public static function onReload()
    {
        trace('Code reloaded! Score still: $score');
    }
}
```

---

## Best Practices

### 1. Enable Before Loading

```c
// ✅ GOOD
hlffi_enable_hot_reload(vm, true);
hlffi_load_file(vm, "game.hl");

// ❌ BAD
hlffi_load_file(vm, "game.hl");
hlffi_enable_hot_reload(vm, true);  // Too late!
```

### 2. Use Callback for Notifications

```c
// ✅ GOOD - Know when reload completes
hlffi_set_reload_callback(vm, on_reload, NULL);

// Can reinitialize state if needed
void on_reload(hlffi_vm* vm, bool success, void* userdata)
{
    if (success)
    {
        reinit_game_state();
    }
}
```

### 3. Handle Static Variables

```haxe
// Static variables PERSIST across reloads
class Game
{
    public static var initialized:Bool = false;
    
    public static function init()
    {
        if (!initialized)
        {
            // First init
            initialized = true;
        }
else
{
            // Already initialized (hot reload occurred)
            // Don't re-init!
        }
    }
}
```

---

## Limitations

### Cannot Change Type Structure

```haxe
// ✅ CAN hot reload:
class Player
{
    var x:Float;  // Same fields
    var y:Float;
    
    public function move()
    {
        x += 10;  // ← Can change implementation
    }
}

// ❌ CANNOT hot reload:
class Player
{
    var x:Float;
    var y:Float;
    var z:Float;  // ← Added new field - requires full restart
}
```

### Static Variables Don't Reset

```haxe
class Game
{
    public static var score:Int = 1000;  // Initial value
}

// After hot reload, score is STILL whatever it was at runtime
// NOT reset to 1000!
```

**Workaround:** Add explicit reset function:
```haxe
public static function resetScore()
{
    score = 1000;
}
```

---

## Troubleshooting

### Reload Fails Silently

**Check:**
- HashLink version is 1.12+
- Not using HL/C mode (JIT only)
- `hlffi_enable_hot_reload()` was called before load

### Changes Not Reflected

**Try:**
- Verify file is actually recompiled
- Check callback is called
- Ensure .hl file timestamp changed

### Crashes After Reload

**Possible Causes:**
- Changed type structure (fields added/removed)
- ABI incompatibility
- Need full restart instead

---

**[← Threading](API_04_THREADING.md)** | **[Back to Index](API_REFERENCE.md)** | **[Type System →](API_06_TYPE_SYSTEM.md)**