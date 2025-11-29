# Hot Reload Support

HLFFI supports hot reloading of Haxe bytecode at runtime without restarting the VM. This uses HashLink's built-in `hl_module_patch()` functionality.

## Overview

Hot reload allows you to:
- Change function implementations at runtime
- See code changes immediately without restart
- Preserve static variable state across reloads
- Get notified when reloads occur

## Quick Start

```c
// 1. Create VM and enable hot reload BEFORE loading
hlffi_vm* vm = hlffi_create();
hlffi_enable_hot_reload(vm, true);

// 2. Initialize and load bytecode
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);

// 3. Later, reload with new bytecode
hlffi_reload_module(vm, "game_updated.hl");
```

## API Reference

### hlffi_enable_hot_reload()

```c
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable);
```

Enable or disable hot reload mode. **Must be called before `hlffi_load_file()`**.

- `vm` - VM handle
- `enable` - true to enable, false to disable
- Returns `HLFFI_OK` on success, error code if module already loaded

### hlffi_reload_module()

```c
hlffi_error_code hlffi_reload_module(hlffi_vm* vm, const char* path);
```

Reload bytecode from a file, patching the running module.

- `vm` - VM handle
- `path` - Path to new .hl file (or NULL to reload original file)
- Returns `HLFFI_OK` on success

### hlffi_reload_module_memory()

```c
hlffi_error_code hlffi_reload_module_memory(hlffi_vm* vm, const void* data, size_t size);
```

Reload bytecode from memory buffer.

- `vm` - VM handle
- `data` - Bytecode data
- `size` - Size in bytes
- Returns `HLFFI_OK` on success

### hlffi_set_reload_callback()

```c
typedef void (*hlffi_reload_callback)(hlffi_vm* vm, bool changed, void* userdata);

void hlffi_set_reload_callback(hlffi_vm* vm, hlffi_reload_callback callback, void* userdata);
```

Register a callback to be notified after each reload.

- `vm` - VM handle
- `callback` - Function to call after reload
- `userdata` - User data passed to callback
- `changed` parameter is true if any code was actually modified

### hlffi_check_reload()

```c
bool hlffi_check_reload(hlffi_vm* vm);
```

Check if the loaded file has changed and auto-reload if so. Call this periodically (e.g., once per frame) for automatic hot reload during development.

- Returns true if reload occurred

### hlffi_is_hot_reload_enabled()

```c
bool hlffi_is_hot_reload_enabled(hlffi_vm* vm);
```

Check if hot reload is enabled for this VM.

## Important Behavior

### What Gets Reloaded

- **Function code** - Modified functions are patched in-place
- **New functions** - Added to the module

### What Does NOT Get Reloaded

- **Static variable values** - Preserved across reloads (not reinitialized!)
- **Instance data** - Existing objects keep their state
- **Type definitions** - Structural changes may cause issues

### Example: Static Variables

```haxe
// Version 1: game_v1.hl
class Game
{
    static var score:Int = 0;
    static var version:Int = 1;

    public static function getValue():Int
    {
        return 100;
    }
}
```

```haxe
// Version 2: game_v2.hl
class Game
{
    static var score:Int = 0;
    static var version:Int = 2;  // This initializer won't run!

    public static function getValue():Int
    {
        return 200;  // This WILL change
    }
}
```

After reload:
- `getValue()` returns 200 ✅ (code patched)
- `version` is still 1 ❌ (initializer not re-run)
- `score` is preserved ✅ (state maintained)

## Development Workflow

### Manual Reload

```c
// In your game loop or editor
if (user_pressed_reload_key())
{
    hlffi_error_code err = hlffi_reload_module(vm, "game.hl");
    if (err == HLFFI_OK)
    {
        printf("Reloaded successfully!\n");
    }
}
```

### Auto Reload (File Watching)

```c
// In your game loop - checks file modification time
void game_loop()
{
    while (running)
    {
        // Check for file changes once per frame
        if (hlffi_check_reload(vm))
        {
            printf("Auto-reloaded!\n");
        }

        // ... rest of game loop
    }
}
```

### With Reload Callback

```c
void on_reload(hlffi_vm* vm, bool changed, void* userdata)
{
    if (changed)
    {
        printf("[Hot Reload] Code updated!\n");
        // Optionally reinitialize game state
        hlffi_call_static(vm, "Game", "onReload", 0, NULL);
    }
}

// Setup
hlffi_set_reload_callback(vm, on_reload, NULL);
```

## Best Practices

1. **Enable early** - Call `hlffi_enable_hot_reload()` before loading any module

2. **Same class names** - V1 and V2 must use identical class/function names for patching to work

3. **Handle state carefully** - Static initializers don't re-run; use explicit reset functions if needed:
   ```haxe
   class Game
   {
       static var version = 1;

       public static function resetAfterReload()
       {
           version = 2;  // Explicitly update
       }
   }
   ```

4. **Avoid structural changes** - Adding/removing fields or changing types may cause issues

5. **Development only** - Hot reload has overhead; disable for production builds

## Limitations

- Hot reload must be enabled before the first `hlffi_load_file()` call
- Structural changes (new fields, type changes) may not work correctly
- Static initializers are not re-executed
- Requires HashLink 1.12+ (uses `hl_module_patch()`)

## Test Files

See these files for working examples:
- `test_hot_reload.c` - C test demonstrating hot reload
- `test/HotReload.hx` - Haxe test class
- `test/hot_reload_v1.hl` - Initial bytecode (getValue returns 100)
- `test/hot_reload_v2.hl` - Updated bytecode (getValue returns 200)

Run the test:
```bash
bin/x64/Debug/test_hot_reload.exe
```

Expected output:
```
=== Hot Reload Test ===

Enabling hot reload...
Loading hot_reload_v1.hl...

--- Before Reload ---
getValue() = 100 (expected 100)
getVersion() = 1 (expected 1)

--- Reloading with V2 ---
$HotReload.getValue has been modified [2]
[HotReload] 2 changes
[Callback] Reload completed for 'test', changed=true

--- After Reload ---
getValue() = 200 (expected 200)
getVersion() = 1 (statics persist!)

=== Results ===
SUCCESS: Hot reload worked correctly!
```
