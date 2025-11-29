# HLFFI API Reference - Utilities & Helpers

**[← Performance](API_17_PERFORMANCE.md)** | **[Back to Index](API_REFERENCE.md)** | **[Error Handling →](API_19_ERROR_HANDLING.md)**

Helper functions for threading, GC integration, and debugging.

---

## Quick Reference

### Worker Thread Management

| Function | Purpose |
|----------|---------|
| `hlffi_worker_register()` | Register worker thread with GC |
| `hlffi_worker_unregister()` | Unregister worker thread |

### Blocking Operations

| Function | Purpose |
|----------|---------|
| `hlffi_blocking_begin()` | Mark start of external I/O |
| `hlffi_blocking_end()` | Mark end of external I/O |

### C++ RAII Guards

| Class | Purpose |
|-------|---------|
| `hlffi::WorkerGuard` | Auto register/unregister worker thread |
| `hlffi::BlockingGuard` | Auto blocking begin/end |

**Complete Guide:** See `docs/TIMERS_ASYNC_THREADING.md`

---

## Worker Thread Management

Worker threads are C threads that need to access HLFFI/HashLink data structures.

### Register Worker Thread

**Signature:**
```c
void hlffi_worker_register(void)
```

**Description:** Register current thread with HashLink GC. **Must call before accessing HLFFI from worker thread.**

**Example:**
```c
void* worker_thread(void* arg)
{
    hlffi_worker_register();  // MUST call first

    // Safe to use HLFFI now:
    hlffi_value* result = hlffi_call_static(vm, "Task", "process", 0, NULL);
    hlffi_value_free(result);

    hlffi_worker_unregister();  // MUST call before exit
    return NULL;
}

pthread_t thread;
pthread_create(&thread, NULL, worker_thread, NULL);
```

---

### Unregister Worker Thread

**Signature:**
```c
void hlffi_worker_unregister(void)
```

**Description:** Unregister current thread from HashLink GC. **Must call before thread exits.**

**Example:**
```c
void* worker_thread(void* arg)
{
    hlffi_worker_register();

    // ... work ...

    hlffi_worker_unregister();  // MUST call
    return NULL;
}
```

**⚠️ IMPORTANT:** Always balance `register` with `unregister`. Unbalanced calls cause memory leaks or crashes.

---

## Blocking Operation Guards

When calling external I/O (file, network, sleep, etc.), notify HashLink GC to prevent blocking.

### Mark Blocking Start

**Signature:**
```c
void hlffi_blocking_begin(void)
```

**Description:** Notify GC that thread is leaving HashLink control for external I/O. **Must balance with `hlffi_blocking_end()`.**

**Example:**
```c
hlffi_blocking_begin();  // Leaving HL control
sleep(1);  // External blocking operation
hlffi_blocking_end();    // Back under HL control
```

---

### Mark Blocking End

**Signature:**
```c
void hlffi_blocking_end(void)
```

**Description:** Notify GC that thread has returned to HashLink control.

**Example:**
```c
hlffi_blocking_begin();
FILE* f = fopen("data.txt", "r");  // External I/O
// ... read file ...
fclose(f);
hlffi_blocking_end();
```

---

## When to Use Blocking Guards

### ✅ Use Guards For:
- File I/O (`fopen`, `fread`, `fwrite`)
- Network I/O (`recv`, `send`, `connect`)
- Sleep/delay operations (`sleep`, `usleep`, `nanosleep`)
- External library calls (database, HTTP, etc.)
- System calls that may block

### ❌ Don't Use Guards For:
- CPU-only operations (math, string processing)
- HLFFI function calls
- Memory allocation
- Pure computation

---

## Complete Example: Worker Thread

```c
#include "hlffi.h"
#include <pthread.h>


{
    hlffi_vm* vm;
    int task_id;
} task_data;

void* worker_thread(void* arg)
{
    task_data* data = (task_data*)arg;

    // Register with GC:
    hlffi_worker_register();

    printf("Worker thread started (task %d)\n", data->task_id);

    // Call Haxe function:
    hlffi_value* id = hlffi_value_int(data->vm, data->task_id);
    hlffi_value* args[] = {id};
    hlffi_value* result = hlffi_call_static(
        data->vm, "TaskProcessor", "process", 1, args
    );
    hlffi_value_free(id);

    if (result)
    {
        int status = hlffi_value_as_int(result, 0);
        printf("Task %d completed with status %d\n", data->task_id, status);
        hlffi_value_free(result);
    }

    // Unregister before exit:
    hlffi_worker_unregister();

    free(data);
    return NULL;
}

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Spawn 4 worker threads:
    pthread_t threads[4];
    for (int i = 0; i < 4; i++)
    {
        task_data* data = malloc(sizeof(task_data));
        data->vm = vm;
        data->task_id = i;
        pthread_create(&threads[i], NULL, worker_thread, data);
    }

    // Wait for workers:
    for (int i = 0; i < 4; i++)
    {
        pthread_join(threads[i], NULL);
    }

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

---

## Complete Example: Blocking I/O

```c
#include "hlffi.h"

hlffi_value* load_file_callback(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    char* filename = hlffi_value_as_string(argv[0]);

    // Notify GC we're doing external I/O:
    hlffi_blocking_begin();

    FILE* f = fopen(filename, "r");
    if (!f)
    {
        hlffi_blocking_end();
        free(filename);
        return hlffi_value_null(vm);
    }

    // Read file (may block on disk I/O):
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = '\0';
    fclose(f);

    hlffi_blocking_end();  // Back under HL control

    // Create result:
    hlffi_value* result = hlffi_value_string(vm, data);

    free(filename);
    free(data);
    return result;
}

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);

    // Register callback that does blocking I/O:
    hlffi_register_callback(vm, "loadFile", load_file_callback, 1);

    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
@:hlNative("", "loadFile")
static function loadFile(filename:Dynamic):Dynamic;

class Main
{
    public static function main()
    {
        var content = loadFile("data.txt");
        trace('Loaded: $content');
    }
}
```

---

## C++ RAII Guards

For C++ projects, use RAII guards for automatic cleanup:

### WorkerGuard

**Signature:**
```cpp
namespace hlffi
{
    class WorkerGuard
    {
        WorkerGuard();   // Calls hlffi_worker_register()
        ~WorkerGuard();  // Calls hlffi_worker_unregister()
    };
}
```

**Example:**
```cpp
#include "hlffi.h"

void* worker_thread(void* arg)
{
    hlffi::WorkerGuard guard;  // Auto register

    // Safe to use HLFFI:
    hlffi_call_static(vm, "Task", "process", 0, NULL);

    return NULL;
}  // Auto unregister on scope exit
```

---

### BlockingGuard

**Signature:**
```cpp
namespace hlffi
{
    class BlockingGuard
    {
        BlockingGuard();   // Calls hlffi_blocking_begin()
        ~BlockingGuard();  // Calls hlffi_blocking_end()
    };
}
```

**Example:**
```cpp
#include "hlffi.h"

hlffi_value* my_callback(hlffi_vm* vm, int argc, hlffi_value** argv)
{
    hlffi::BlockingGuard guard;  // Auto begin

    // External I/O:
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return hlffi_value_null(vm);
}  // Auto end on scope exit
```

---

## Best Practices

### 1. Always Balance Register/Unregister

```c
// ✅ GOOD - Balanced
void* worker(void* arg)
{
    hlffi_worker_register();
    // ... work ...
    hlffi_worker_unregister();
    return NULL;
}

// ❌ BAD - Unbalanced
void* worker(void* arg)
{
    hlffi_worker_register();
    // ... work ...
    // Missing: hlffi_worker_unregister();
    return NULL;  // Memory leak!
}
```

### 2. Always Balance Blocking Begin/End

```c
// ✅ GOOD - Balanced
hlffi_blocking_begin();
sleep(1);
hlffi_blocking_end();

// ❌ BAD - Unbalanced
hlffi_blocking_begin();
sleep(1);
// Missing: hlffi_blocking_end();
```

### 3. Use RAII in C++

```cpp
// ✅ GOOD - Exception-safe
void* worker(void* arg)
{
    hlffi::WorkerGuard guard;  // Auto cleanup on any exit path

    if (error) throw std::runtime_error("Error");

    return NULL;
}  // Always calls unregister

// ❌ RISKY - Manual cleanup
void* worker(void* arg)
{
    hlffi_worker_register();

    if (error)
    {
        hlffi_worker_unregister();  // Must remember every exit path
        throw std::runtime_error("Error");
    }

    hlffi_worker_unregister();
    return NULL;
}
```

### 4. Guard Only External I/O

```c
// ✅ GOOD - Guard actual I/O
hlffi_blocking_begin();
FILE* f = fopen("data.txt", "r");
fread(buf, 1, 1024, f);
fclose(f);
hlffi_blocking_end();

// ❌ UNNECESSARY - Don't guard CPU work
hlffi_blocking_begin();  // Not needed
int sum = 0;
for (int i = 0; i < 1000; i++)
{
    sum += i;
}
hlffi_blocking_end();
```

---

## Thread Safety Notes

- **Worker registration:** Thread-local (per-thread registration)
- **Blocking guards:** Thread-local (per-thread state)
- **RAII guards:** Non-copyable (move-only in C++11)

---

## Performance Impact

- **Worker registration:** ~100ns (one-time per thread)
- **Blocking guards:** ~50ns per begin/end pair
- **Negligible** for actual I/O operations (which are microseconds+)

---

**[← Performance](API_17_PERFORMANCE.md)** | **[Back to Index](API_REFERENCE.md)** | **[Error Handling →](API_19_ERROR_HANDLING.md)**