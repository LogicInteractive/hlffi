# VM Restart Support (Experimental)

HLFFI supports restarting the HashLink VM within a single process. This is **experimental** and works around HashLink's design which assumes single-initialization per process.

## Overview

You can:
1. Create a VM, load bytecode, run it
2. Destroy the VM
3. Wait
4. Create a new VM, load bytecode, run it again

This is useful for:
- Hot reload scenarios
- Game level reloading
- Testing multiple configurations
- Long-running servers that need to refresh Haxe code

## How It Works

HashLink has two process-wide initializations that can't be safely repeated:

1. **`hl_global_init()`** - Initializes HashLink global state (GC, type system)
2. **`hl_register_thread()`** - Registers main thread with GC

HLFFI uses static flags to ensure these are called only once per process:

```c
// In hlffi_lifecycle.c
static bool g_hl_globals_initialized = false;
static bool g_main_thread_registered = false;
```

When you call `hlffi_init()` on a second VM, it skips these already-done initializations.

## Usage

### Non-Threaded Mode (Mode 1)

```c
for (int session = 0; session < 3; session++)
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Use the VM...
    hlffi_call_static(vm, "Game", "update", 0, NULL);

    hlffi_destroy(vm);

    // Wait before restart
    sleep(1);
}
```

### Threaded Mode (Mode 2) - Non-blocking main()

Haxe `main()` returns immediately, C host drives via sync calls:

```c
for (int session = 0; session < 3; session++)
{
    hlffi_vm* vm = hlffi_create();
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_thread_start(vm);  // Calls main(), then processes messages

    // Use the VM via sync calls...
    hlffi_thread_call_sync(vm, my_callback, data);

    hlffi_thread_stop(vm);
    hlffi_destroy(vm);

    sleep(1);
}
```

### Threaded Mode (Mode 2) - Blocking main()

Haxe `main()` runs a loop that eventually exits:

```haxe
class Game
{
    static var running = true;

    public static function main()
    {
        while (running)
        {
            // Game loop
            Sys.sleep(0.016);  // ~60 FPS
        }
        trace("main() exiting");
    }

    public static function shutdown()
    {
        running = false;  // Causes main() to exit
    }
}
```

```c
hlffi_thread_start(vm);

// Wait for game to finish or call shutdown
hlffi_thread_call_sync(vm, call_shutdown, NULL);

hlffi_thread_stop(vm);  // Will now succeed since main() exited
hlffi_destroy(vm);
```

## Limitations

### Experimental Status

This feature is experimental because:

1. **Not designed by HashLink** - HashLink assumes single initialization per process
2. **Memory accumulation** - Some HashLink internal state may not be fully released
3. **GC constraints** - The main thread registration persists across VM restarts
4. **Limited testing** - Works in tested scenarios but edge cases may exist

### Static State Persists

Haxe static variables reset on each `hlffi_load_file()` since it's a fresh module load:

```haxe
class Counter {
    static var count:Int = 0;  // Resets to 0 on each VM restart
}
```

### Blocking main() Must Exit

If Haxe `main()` has a blocking loop, it must eventually exit for `hlffi_thread_stop()` to complete:

```haxe
// GOOD: Loop that can exit
while (running) { ... }

// BAD: Infinite loop with no exit condition
while (true) { ... }  // VM thread can never cleanly stop
```

### Single VM at a Time

Only one VM should be active at any time. Don't create a second VM before destroying the first:

```c
// WRONG
hlffi_vm* vm1 = hlffi_create();
hlffi_vm* vm2 = hlffi_create();  // Bad - vm1 still exists

// CORRECT
hlffi_vm* vm1 = hlffi_create();
// use vm1...
hlffi_destroy(vm1);
hlffi_vm* vm2 = hlffi_create();  // OK - vm1 destroyed
```

## Test Files

See these test files for working examples:

- `test_vm_restart.c` - Non-threaded mode restart test
- `test_threaded_restart.c` - Threaded mode with C-driven counting
- `test_threaded_blocking.c` - Threaded mode with Haxe blocking loop
- `test/ThreadedCounter.hx` - Non-blocking Haxe example
- `test/ThreadedCounterBlocking.hx` - Blocking Haxe example

## Best Practices

1. **Always destroy before recreating** - Call `hlffi_destroy()` before creating a new VM
2. **Wait between restarts** - A small delay (100ms-1sec) helps ensure clean state
3. **Design for exit** - If using blocking main(), include a way to signal exit
4. **Monitor memory** - In long-running applications, watch for memory growth
5. **Test thoroughly** - Test your specific restart scenario before relying on it

## Future Work

- Investigate cleaner HashLink restart support
- Add `hlffi_is_restartable()` API to check restart capability
- Better memory cleanup between restarts
- Support for multiple concurrent VMs (if HashLink ever supports it)
