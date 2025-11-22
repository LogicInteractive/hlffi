/**
 * HLFFI v3.0 - HashLink Foreign Function Interface
 *
 * Production-ready C/C++ library for embedding HashLink/Haxe into applications.
 *
 * Features:
 * - Clean C API with optional C++ wrappers
 * - Automatic GC root management (no manual dispose!)
 * - Two integration modes: Non-threaded (engine tick) and Threaded (dedicated thread)
 * - UV + haxe.EventLoop integration with weak symbols
 * - Hot reload support (HL 1.12+)
 * - Type-safe wrappers for common operations
 *
 * Platform: Windows (Visual Studio), cross-platform support planned
 * License: MIT (same as HashLink)
 */

#ifndef HLFFI_H
#define HLFFI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== VERSION ========== */

#define HLFFI_VERSION_MAJOR 3
#define HLFFI_VERSION_MINOR 0
#define HLFFI_VERSION_PATCH 0
#define HLFFI_VERSION_STRING "3.0.0"

/* ========== CORE TYPES ========== */

/**
 * Opaque VM handle.
 * Represents a HashLink virtual machine instance.
 * Only one VM per process is supported (HashLink limitation).
 */
typedef struct hlffi_vm hlffi_vm;

/**
 * Opaque type handle.
 * Represents a Haxe type for reflection.
 */
typedef struct hlffi_type hlffi_type;

/**
 * Opaque value handle.
 * Represents a Haxe value (object, primitive, etc.).
 */
typedef struct hlffi_value hlffi_value;

/* ========== ERROR CODES ========== */

typedef enum {
    HLFFI_OK = 0,

    /* VM lifecycle errors */
    HLFFI_ERROR_NULL_VM,
    HLFFI_ERROR_ALREADY_INITIALIZED,
    HLFFI_ERROR_NOT_INITIALIZED,
    HLFFI_ERROR_INIT_FAILED,
    HLFFI_ERROR_DESTROY_FAILED,

    /* Loading errors */
    HLFFI_ERROR_FILE_NOT_FOUND,
    HLFFI_ERROR_INVALID_BYTECODE,
    HLFFI_ERROR_MODULE_LOAD_FAILED,
    HLFFI_ERROR_MODULE_INIT_FAILED,

    /* Call errors */
    HLFFI_ERROR_ENTRY_POINT_NOT_FOUND,
    HLFFI_ERROR_TYPE_NOT_FOUND,
    HLFFI_ERROR_METHOD_NOT_FOUND,
    HLFFI_ERROR_FIELD_NOT_FOUND,
    HLFFI_ERROR_CALL_FAILED,
    HLFFI_ERROR_EXCEPTION_THROWN,

    /* Type errors */
    HLFFI_ERROR_INVALID_TYPE,
    HLFFI_ERROR_TYPE_MISMATCH,
    HLFFI_ERROR_NULL_VALUE,

    /* Hot reload errors */
    HLFFI_ERROR_RELOAD_NOT_SUPPORTED,
    HLFFI_ERROR_RELOAD_NOT_ENABLED,
    HLFFI_ERROR_RELOAD_FAILED,

    /* Threading errors */
    HLFFI_ERROR_THREAD_NOT_STARTED,
    HLFFI_ERROR_THREAD_ALREADY_RUNNING,
    HLFFI_ERROR_THREAD_START_FAILED,
    HLFFI_ERROR_THREAD_STOP_FAILED,
    HLFFI_ERROR_WRONG_THREAD,

    /* Event loop errors */
    HLFFI_ERROR_EVENTLOOP_NOT_FOUND,
    HLFFI_ERROR_EVENTLOOP_FAILED,

    /* Generic errors */
    HLFFI_ERROR_OUT_OF_MEMORY,
    HLFFI_ERROR_INVALID_ARGUMENT,
    HLFFI_ERROR_NOT_IMPLEMENTED,
    HLFFI_ERROR_UNKNOWN
} hlffi_error_code;

/* ========== INTEGRATION MODES ========== */

/**
 * Integration mode determines how HLFFI manages the VM lifecycle.
 *
 * NON_THREADED (recommended): Engine/host controls main loop
 *   - Call hlffi_update() every frame from host thread
 *   - Direct function calls, no synchronization overhead
 *   - Use for: Unreal, Unity, game engines, tools
 *
 * THREADED (advanced): Dedicated VM thread
 *   - Call hlffi_thread_start() to spawn thread
 *   - Thread-safe calls via hlffi_thread_call_sync()
 *   - Use for: Haxe code with blocking while loop (Android pattern)
 */
typedef enum {
    HLFFI_MODE_NON_THREADED = 0,  /* Engine controls loop (default) */
    HLFFI_MODE_THREADED = 1        /* Dedicated VM thread */
} hlffi_integration_mode;

/* ========== EVENT LOOP TYPES ========== */

/**
 * Event loop type for hlffi_process_events().
 *
 * UV: libuv event loop (async I/O, HTTP, file watch, timers)
 * HAXE: haxe.EventLoop (haxe.Timer, haxe.MainLoop callbacks)
 * ALL: Both UV + Haxe (default for hlffi_update)
 */
typedef enum {
    HLFFI_EVENTLOOP_UV = 0,
    HLFFI_EVENTLOOP_HAXE = 1,
    HLFFI_EVENTLOOP_ALL = 2
} hlffi_eventloop_type;

/* ========== CALLBACKS ========== */

/**
 * Hot reload callback.
 * Called after a reload attempt (success or failure).
 *
 * @param vm VM instance
 * @param success true if reload succeeded, false if failed
 * @param userdata User-provided data (passed to hlffi_set_reload_callback)
 */
typedef void (*hlffi_reload_callback)(hlffi_vm* vm, bool success, void* userdata);

/**
 * Thread function callback.
 * Used with hlffi_thread_call_sync() and hlffi_thread_call_async().
 *
 * @param vm VM instance (safe to call hlffi_* functions)
 * @param userdata User-provided data
 */
typedef void (*hlffi_thread_func)(hlffi_vm* vm, void* userdata);

/**
 * Thread async callback.
 * Called when async thread operation completes.
 *
 * @param vm VM instance
 * @param result Result from thread function (or NULL)
 * @param userdata User-provided data
 */
typedef void (*hlffi_thread_async_callback)(hlffi_vm* vm, void* result, void* userdata);

/* ========== CORE VM LIFECYCLE ========== */

/**
 * Create a new VM instance.
 * Allocates VM structure but does not initialize HashLink runtime.
 *
 * @return VM instance, or NULL on allocation failure
 *
 * @note Only ONE VM per process is supported (HashLink limitation)
 * @note Call hlffi_init() to initialize the VM
 */
hlffi_vm* hlffi_create(void);

/**
 * Initialize HashLink runtime.
 * Sets up GC, registers main thread, prepares for module loading.
 *
 * @param vm VM instance
 * @param argc Argument count (pass 0 if no args)
 * @param argv Argument vector (pass NULL if no args)
 * @return HLFFI_OK on success, error code on failure
 *
 * @note This can only be called ONCE per process
 * @note Cannot be called again after hlffi_destroy()
 */
hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv);

/**
 * Load bytecode from file.
 * Loads .hl bytecode file from disk into memory.
 *
 * @param vm VM instance
 * @param path Path to .hl file
 * @return HLFFI_OK on success, error code on failure
 *
 * @note File must be valid HashLink bytecode
 * @note Call hlffi_call_entry() after loading to run main()
 */
hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path);

/**
 * Load bytecode from memory buffer.
 * Loads .hl bytecode from a memory buffer.
 *
 * @param vm VM instance
 * @param data Pointer to bytecode data
 * @param size Size of bytecode in bytes
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Buffer must contain valid HashLink bytecode
 * @note Buffer can be freed after this function returns
 */
hlffi_error_code hlffi_load_memory(hlffi_vm* vm, const void* data, size_t size);

/**
 * Call the entry point (main() function).
 * Runs the Haxe main() function and sets up global state.
 *
 * @param vm VM instance
 * @return HLFFI_OK on success, error code on failure
 *
 * @note This MUST be called even if main() is empty (sets up globals)
 * @note In NON_THREADED mode: Returns immediately (if Haxe has no while loop)
 * @note In THREADED mode: Called automatically by hlffi_thread_start()
 *
 * @warning If Haxe code has a while loop, this will block!
 *          Use THREADED mode or ensure Haxe main() returns
 */
hlffi_error_code hlffi_call_entry(hlffi_vm* vm);

/**
 * Destroy VM instance.
 * Frees VM resources and shuts down HashLink runtime.
 *
 * @param vm VM instance
 *
 * @warning Only call this at process exit!
 * @warning Cannot create a new VM after calling this (HashLink limitation)
 * @warning Use hot reload instead of destroy/create for code changes
 */
void hlffi_destroy(hlffi_vm* vm);

/**
 * Get last error message.
 * Returns a human-readable error message for the last error.
 *
 * @param vm VM instance
 * @return Error message string (valid until next error)
 */
const char* hlffi_get_error(hlffi_vm* vm);

/* ========== INTEGRATION MODE SETUP ========== */

/**
 * Set integration mode.
 * Must be called BEFORE hlffi_call_entry().
 *
 * @param vm VM instance
 * @param mode Integration mode (NON_THREADED or THREADED)
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Default is NON_THREADED
 * @note Cannot be changed after hlffi_call_entry()
 */
hlffi_error_code hlffi_set_integration_mode(hlffi_vm* vm, hlffi_integration_mode mode);

/**
 * Get current integration mode.
 *
 * @param vm VM instance
 * @return Current integration mode
 */
hlffi_integration_mode hlffi_get_integration_mode(hlffi_vm* vm);

/* ========== MODE 1: NON-THREADED (Engine controls loop) ========== */

/**
 * Update VM (process events).
 * Call this EVERY FRAME from engine tick in NON_THREADED mode.
 *
 * Processes:
 * - libuv events (async I/O, HTTP, timers) - if UV loop exists
 * - haxe.EventLoop events (haxe.Timer callbacks) - if EventLoop exists
 *
 * @param vm VM instance
 * @param delta_time Frame delta time in seconds (optional, can be 0)
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Non-blocking - returns immediately after processing pending events
 * @note Uses weak symbols - only processes event loops that exist
 * @note Call from engine tick: Tick() { hlffi_update(vm, dt); }
 */
hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time);

/**
 * Check if VM has pending work.
 * Returns true if there are pending events to process.
 *
 * @param vm VM instance
 * @return true if pending work exists
 */
bool hlffi_has_pending_work(hlffi_vm* vm);

/* ========== MODE 2: THREADED (Dedicated VM thread) ========== */

/**
 * Start dedicated VM thread.
 * Spawns a thread and calls hlffi_call_entry() in the thread.
 * Use in THREADED mode when Haxe code has blocking while loop.
 *
 * @param vm VM instance
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Only use in THREADED mode
 * @note Entry point may block in the thread (not in host app)
 * @note All function calls must use hlffi_thread_call_*()
 */
hlffi_error_code hlffi_thread_start(hlffi_vm* vm);

/**
 * Stop dedicated VM thread.
 * Waits for thread to finish and cleans up.
 *
 * @param vm VM instance
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Blocks until thread exits
 */
hlffi_error_code hlffi_thread_stop(hlffi_vm* vm);

/**
 * Check if VM thread is running.
 *
 * @param vm VM instance
 * @return true if thread is running
 */
bool hlffi_thread_is_running(hlffi_vm* vm);

/**
 * Call function in VM thread (synchronous).
 * Queues a function call to the VM thread and blocks until complete.
 *
 * @param vm VM instance
 * @param func Function to call in VM thread
 * @param userdata User data passed to function
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Only use in THREADED mode
 * @note Blocks until function completes
 * @note Thread-safe
 */
hlffi_error_code hlffi_thread_call_sync(hlffi_vm* vm, hlffi_thread_func func, void* userdata);

/**
 * Call function in VM thread (asynchronous).
 * Queues a function call to the VM thread and returns immediately.
 *
 * @param vm VM instance
 * @param func Function to call in VM thread
 * @param callback Callback when function completes (optional)
 * @param userdata User data passed to function
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Only use in THREADED mode
 * @note Returns immediately
 * @note Thread-safe
 */
hlffi_error_code hlffi_thread_call_async(
    hlffi_vm* vm,
    hlffi_thread_func func,
    hlffi_thread_async_callback callback,
    void* userdata
);

/* ========== EVENT LOOP INTEGRATION (Advanced) ========== */

/**
 * Process specific event loop.
 * Low-level control over event processing.
 * Most users should use hlffi_update() instead.
 *
 * @param vm VM instance
 * @param type Event loop type (UV, HAXE, or ALL)
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Non-blocking - returns immediately
 * @note Uses weak symbols - no-op if event loop doesn't exist
 */
hlffi_error_code hlffi_process_events(hlffi_vm* vm, hlffi_eventloop_type type);

/**
 * Check if event loop has pending events.
 *
 * @param vm VM instance
 * @param type Event loop type (UV or HAXE)
 * @return true if pending events exist
 */
bool hlffi_has_pending_events(hlffi_vm* vm, hlffi_eventloop_type type);

/* ========== HOT RELOAD ========== */

/**
 * Enable/disable hot reload.
 * Allows reloading changed bytecode without restart.
 *
 * @param vm VM instance
 * @param enable true to enable, false to disable
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Requires HashLink 1.12+
 * @note Only works in JIT mode (not HL/C)
 * @note Must call before hlffi_call_entry()
 */
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable);

/**
 * Check if hot reload is enabled.
 *
 * @param vm VM instance
 * @return true if hot reload is enabled
 */
bool hlffi_is_hot_reload_enabled(hlffi_vm* vm);

/**
 * Reload module from file.
 * Reloads changed bytecode without restarting VM.
 *
 * @param vm VM instance
 * @param path Path to new .hl file
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Hot reload must be enabled
 * @note Preserves runtime state
 * @note Triggers reload callback if set
 */
hlffi_error_code hlffi_reload_module(hlffi_vm* vm, const char* path);

/**
 * Reload module from memory.
 *
 * @param vm VM instance
 * @param data Pointer to new bytecode
 * @param size Size of bytecode in bytes
 * @return HLFFI_OK on success, error code on failure
 */
hlffi_error_code hlffi_reload_module_memory(hlffi_vm* vm, const void* data, size_t size);

/**
 * Set reload callback.
 * Called after each reload attempt (success or failure).
 *
 * @param vm VM instance
 * @param callback Callback function (or NULL to clear)
 * @param userdata User data passed to callback
 */
void hlffi_set_reload_callback(hlffi_vm* vm, hlffi_reload_callback callback, void* userdata);

/* ========== WORKER THREAD HELPERS ========== */

/**
 * Register worker thread.
 * Call from worker thread BEFORE using any HLFFI functions.
 * Use for background work FROM Haxe (e.g., sys.thread.Thread).
 *
 * @note Must call hlffi_worker_unregister() when done
 * @note Each thread must register separately
 */
void hlffi_worker_register(void);

/**
 * Unregister worker thread.
 * Call when worker thread is done using HLFFI.
 */
void hlffi_worker_unregister(void);

/**
 * Begin external blocking operation.
 * Call before blocking I/O from Haxe code.
 * Balances hl_blocking() to prevent GC issues.
 *
 * @note Must call hlffi_blocking_end() after operation!
 * @note Calls must be balanced (begin/end pairs)
 */
void hlffi_blocking_begin(void);

/**
 * End external blocking operation.
 * Call after blocking I/O completes.
 */
void hlffi_blocking_end(void);

/* ========== UTILITIES ========== */

/**
 * Get HLFFI version string.
 *
 * @return Version string (e.g., "3.0.0")
 */
const char* hlffi_get_version(void);

/**
 * Get HashLink version string.
 *
 * @return HashLink version string
 */
const char* hlffi_get_hl_version(void);

/**
 * Check if running in JIT mode.
 *
 * @return true if JIT mode, false if HL/C mode
 */
bool hlffi_is_jit_mode(void);

#ifdef __cplusplus
}

/* ========== C++ RAII GUARDS ========== */

namespace hlffi {

/**
 * RAII guard for blocking operations.
 * Automatically calls hlffi_blocking_begin/end.
 *
 * Usage:
 *   {
 *       hlffi::BlockingGuard guard;
 *       // ... blocking I/O ...
 *   } // Automatically calls hlffi_blocking_end()
 */
class BlockingGuard {
public:
    BlockingGuard() { hlffi_blocking_begin(); }
    ~BlockingGuard() { hlffi_blocking_end(); }

    BlockingGuard(const BlockingGuard&) = delete;
    BlockingGuard& operator=(const BlockingGuard&) = delete;
};

/**
 * RAII guard for worker threads.
 * Automatically calls hlffi_worker_register/unregister.
 *
 * Usage:
 *   void worker_thread() {
 *       hlffi::WorkerGuard guard;
 *       // ... use HLFFI ...
 *   } // Automatically unregisters
 */
class WorkerGuard {
public:
    WorkerGuard() { hlffi_worker_register(); }
    ~WorkerGuard() { hlffi_worker_unregister(); }

    WorkerGuard(const WorkerGuard&) = delete;
    WorkerGuard& operator=(const WorkerGuard&) = delete;
};

} // namespace hlffi

#endif /* __cplusplus */

#endif /* HLFFI_H */
