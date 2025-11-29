# HLFFI API Reference - Threading

**[← Event Loop](API_03_EVENT_LOOP.md)** | **[Back to Index](API_REFERENCE.md)** | **[Hot Reload →](API_05_HOT_RELOAD.md)**

Threading support enables **THREADED mode**, where the VM runs in a dedicated thread and the host application communicates via message queue.

---

## Overview

**Use THREADED mode when:**
- Haxe code has blocking `while(true)` loop
- Android pattern (Haxe controls main loop)
- Server applications
- Background processing

**Architecture:**
- Main thread queues messages to VM thread
- VM thread processes messages between event loop ticks
- Supports synchronous (blocking) and asynchronous (non-blocking) calls
- Message queue: 256 messages, circular buffer with mutex/condition variables

**Complete Guide:** See `docs/TIMERS_ASYNC_THREADING.md`

---

## Functions

### Thread Management

#### `hlffi_thread_start()`

**Signature:**
```c
hlffi_error_code hlffi_thread_start(hlffi_vm* vm)
```

**Description:**
Spawns a dedicated VM thread and calls `hlffi_call_entry()` in that thread.

**Returns:**
- `HLFFI_OK` - Thread started
- `HLFFI_ERROR_THREAD_ALREADY_RUNNING` - Already started
- `HLFFI_ERROR_THREAD_START_FAILED` - Thread creation failed

**Example:**
```c
hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
hlffi_error_code err = hlffi_thread_start(vm);
if (err != HLFFI_OK)
{
    fprintf(stderr, "Thread start failed\n");
}
// VM now running in background thread
```

---

#### `hlffi_thread_stop()`

**Signature:**
```c
hlffi_error_code hlffi_thread_stop(hlffi_vm* vm)
```

**Description:**
Stops the VM thread gracefully. **Blocks** until thread exits.

**Example:**
```c
printf("Stopping VM thread...\n");
hlffi_thread_stop(vm);  // Blocks until thread exits
printf("Thread stopped\n");
```

---

#### `hlffi_thread_is_running()`

**Signature:**
```c
bool hlffi_thread_is_running(hlffi_vm* vm)
```

**Description:**
Checks if the VM thread is currently running.

---

### Thread Calls

#### `hlffi_thread_call_sync()`

**Signature:**
```c
hlffi_error_code hlffi_thread_call_sync(
    hlffi_vm* vm,
    hlffi_thread_func func,
    void* userdata
)
```

**Description:**
Calls a function in the VM thread **synchronously**. Blocks until complete.

**Function Signature:**
```c
typedef void (*hlffi_thread_func)(hlffi_vm* vm, void* userdata);
```

**Example:**
```c
void set_score_callback(hlffi_vm* vm, void* userdata)
{
    int score = *(int*)userdata;
    hlffi_value* arg = hlffi_value_int(vm, score);
    hlffi_call_static(vm, "Game", "setScore", 1, &arg);
    hlffi_value_free(arg);
}

// Main thread:
int score = 100;
hlffi_thread_call_sync(vm, set_score_callback, &score);  // Blocks
printf("Score updated\n");
```

---

#### `hlffi_thread_call_async()`

**Signature:**
```c
hlffi_error_code hlffi_thread_call_async(
    hlffi_vm* vm,
    hlffi_thread_func func,
    hlffi_thread_async_callback callback,
    void* userdata
)
```

**Description:**
Calls a function in the VM thread **asynchronously**. Returns immediately.

**Callback Signature:**
```c
typedef void (*hlffi_thread_async_callback)(hlffi_vm* vm, void* result, void* userdata);
```

**Example:**
```c
void increment_callback(hlffi_vm* vm, void* userdata)
{
    hlffi_call_static(vm, "Game", "incrementScore", 0, NULL);
}

void on_complete(hlffi_vm* vm, void* result, void* userdata)
{
    printf("Increment complete\n");
}

// Main thread:
hlffi_thread_call_async(vm, increment_callback, on_complete, NULL);
printf("Message queued, continuing...\n");  // Immediate
```

---

### Worker Threads

#### `hlffi_worker_register()`

**Signature:**
```c
void hlffi_worker_register(void)
```

**Description:**
Registers a worker thread with the HashLink GC. **Call from worker thread BEFORE using HLFFI**.

**Example:**
```c
void* worker_thread(void* arg)
{
    hlffi_worker_register();  // MUST call first
    
    hlffi_vm* vm = (hlffi_vm*)arg;
    hlffi_call_static(vm, "Game", "doWork", 0, NULL);
    
    hlffi_worker_unregister();  // MUST call before exit
    return NULL;
}

pthread_t thread;
pthread_create(&thread, NULL, worker_thread, vm);
```

---

#### `hlffi_worker_unregister()`

**Signature:**
```c
void hlffi_worker_unregister(void)
```

**Description:**
Unregisters worker thread from HashLink GC.

---

### Blocking Operations

#### `hlffi_blocking_begin()` / `hlffi_blocking_end()`

**Signatures:**
```c
void hlffi_blocking_begin(void)
void hlffi_blocking_end(void)
```

**Description:**
Notifies GC that thread is entering/exiting external blocking operation (file I/O, network, sleep).

**Example:**
```c
hlffi_value* save_file_callback(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    const char* path = hlffi_value_as_string(argv[0]);
    
    hlffi_blocking_begin();  // Notify GC
    FILE* f = fopen(path, "w");
    fwrite(...);  // Potentially long I/O
    fclose(f);
    hlffi_blocking_end();  // Back under HL control
    
    return hlffi_value_null(vm);
}
```

---

## Complete Example

```c
#include "hlffi.h"
#include <pthread.h>

void set_player_name(hlffi_vm* vm, void* userdata)
{
    const char* name = (const char*)userdata;
    hlffi_value* arg = hlffi_value_string(vm, name);
    hlffi_call_static(vm, "Game", "setPlayerName", 1, &arg);
    hlffi_value_free(arg);
}

int main()
{
    // Setup THREADED mode:
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    hlffi_thread_start(vm);  // Haxe runs in background
    
    // Main thread does other work:
    while (!should_quit())
    {
        // Synchronous call:
        hlffi_thread_call_sync(vm, set_player_name, "Hero");
        
        // Async call (fire-and-forget):
        hlffi_thread_call_async(vm, increment_callback, NULL, NULL);
        
        handle_events();
        sleep_ms(16);
    }
    
    // Cleanup:
    hlffi_thread_stop(vm);
    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class Main
{
    public static function main()
    {
        // Blocking loop - runs in VM thread
        while (true)
        {
            Game.update();
            Game.render();
            Sys.sleep(0.016);
        }
    }
}
```

---

## Best Practices

### 1. Balance Register/Unregister

```c
// ✅ GOOD
void* worker(void* arg)
{
    hlffi_worker_register();
    // ... work ...
    hlffi_worker_unregister();
    return NULL;
}

// ❌ BAD
void* worker(void* arg)
{
    hlffi_worker_register();
    // ... work ...
    // Missing unregister!
    return NULL;
}
```

### 2. Balance Blocking Begin/End

```c
// ✅ GOOD
hlffi_blocking_begin();
long_io_operation();
hlffi_blocking_end();

// ❌ BAD
hlffi_blocking_begin();
long_io_operation();
// Missing blocking_end!
```

### 3. Use C++ RAII Wrappers

```cpp
// C++ automatic management:
void* worker(void* arg)
{
    hlffi::WorkerGuard guard;  // Auto register/unregister
    // ... work ...
    return NULL;  // Unregister automatic
}

hlffi_value* callback(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    hlffi::BlockingGuard guard;  // Auto begin/end
    long_io_operation();
    return hlffi_value_null(vm);
}  // Auto end
```

---

**[← Event Loop](API_03_EVENT_LOOP.md)** | **[Back to Index](API_REFERENCE.md)** | **[Hot Reload →](API_05_HOT_RELOAD.md)**