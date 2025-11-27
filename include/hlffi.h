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
 * Opaque value handle - TEMPORARY CONVERSION WRAPPER.
 *
 * DESIGN INTENT: hlffi_value is for CONVERSION and PASSING data between
 * C and Haxe, NOT for long-term storage in C data structures.
 *
 * ✅ SAFE USAGE PATTERN (Temporary Conversion):
 *    hlffi_value* temp = hlffi_value_int(vm, 100);
 *    hlffi_call_method(obj, "setHealth", 1, &temp);
 *    hlffi_value_free(temp);  // Use immediately, then free
 *
 * ✅ SAFE: Extract and store native types:
 *    hlffi_value* hp = hlffi_get_field(obj, "health");
 *    int health = hlffi_value_as_int(hp, 0);  // Copy to C int
 *    hlffi_value_free(hp);
 *    // Now 'health' is a C-owned value, safe to store anywhere
 *
 * ✅ SAFE: Store Haxe objects created with hlffi_new():
 *    struct Game { hlffi_value* player; };
 *    game->player = hlffi_new(vm, "Player", 0, NULL);  // GC-rooted
 *    // This is safe because hlffi_new() adds an explicit GC root
 *    // Must call hlffi_value_free() when done to remove root
 *
 * ❌ UNSAFE: Storing temporary wrappers:
 *    struct Data { hlffi_value* temp; };  // DON'T DO THIS!
 *    data->temp = hlffi_value_int(vm, 100);  // NOT GC-rooted!
 *
 * ❌ UNSAFE: Global/static storage:
 *    static hlffi_value* g_value = NULL;  // DON'T DO THIS!
 *
 * MEMORY MANAGEMENT:
 * - ALWAYS call hlffi_value_free() when done (safe for all values)
 * - Values from hlffi_new() are GC-rooted (safe to store)
 * - Values from hlffi_value_int/float/bool/string() are NOT rooted (temporary)
 * - Values from hlffi_get_field/hlffi_call_method() are NOT rooted (temporary)
 * - Strings from hlffi_value_as_string() must be freed with free()
 *
 * GC SAFETY:
 * Non-rooted values rely on GC stack scanning for protection. They are safe
 * when stored in local (stack) variables and used immediately within the same
 * function. They become unsafe when stored in heap-allocated structs, globals,
 * or passed across async boundaries.
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

/**
 * Get error string from error code.
 *
 * @param code Error code
 * @return Human-readable error message
 */
const char* hlffi_get_error_string(hlffi_error_code code);

/* ========== GC STACK SCANNING FIX ========== */

/**
 * Update the GC stack top to current stack position.
 *
 * BACKGROUND:
 * HashLink's GC scans the C call stack to find object references. When embedding
 * HashLink, the initial stack_top pointer may point to heap memory instead of
 * the actual stack, causing incorrect GC behavior (crashes in Debug, corruption
 * in Release). See docs/GC_STACK_SCANNING.md for full details.
 *
 * CURRENT STATUS:
 * HLFFI now handles this internally - all HLFFI functions that allocate GC memory
 * automatically update stack_top. You typically don't need to call this manually.
 *
 * WHEN TO USE THIS:
 * Only if you encounter GC-related crashes and the internal fix is insufficient
 * (e.g., complex threading scenarios, or calling HashLink directly without HLFFI).
 *
 * @param stack_marker Address of a local variable on the call stack
 *
 * Usage:
 *   void my_update_loop(hlffi_vm* vm) {
 *       int stack_marker;
 *       hlffi_update_stack_top(&stack_marker);
 *       // ... now safe to call hlffi functions in loop ...
 *   }
 *
 * @note The stack_marker MUST be a local variable (on the stack), not heap-allocated
 * @note See: https://github.com/HaxeFoundation/hashlink/issues/752
 */
void hlffi_update_stack_top(void* stack_marker);

/**
 * Macro to update stack top using current stack frame.
 * Automatically creates a local variable and updates stack_top.
 *
 * CURRENT STATUS:
 * Not normally needed - HLFFI handles this internally. Provided as a fallback
 * if you encounter GC issues in edge cases.
 *
 * Usage:
 *   void my_update_loop(hlffi_vm* vm) {
 *       HLFFI_ENTER_SCOPE();  // Must be early in the function
 *       // ... now safe to call hlffi functions in loop ...
 *   }
 */
#define HLFFI_ENTER_SCOPE() \
    do { \
        int _hlffi_stack_marker; \
        hlffi_update_stack_top(&_hlffi_stack_marker); \
    } while(0)

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

/* ========== PHASE 2: TYPE SYSTEM & REFLECTION ========== */

/**
 * Type kind enumeration.
 * Matches HashLink's hl_type_kind values.
 */
typedef enum {
    HLFFI_TYPE_VOID = 0,
    HLFFI_TYPE_UI8,
    HLFFI_TYPE_UI16,
    HLFFI_TYPE_I32,
    HLFFI_TYPE_I64,
    HLFFI_TYPE_F32,
    HLFFI_TYPE_F64,
    HLFFI_TYPE_BOOL,
    HLFFI_TYPE_BYTES,
    HLFFI_TYPE_DYN,
    HLFFI_TYPE_FUN,
    HLFFI_TYPE_OBJ,        // Class/object type
    HLFFI_TYPE_ARRAY,
    HLFFI_TYPE_TYPE,
    HLFFI_TYPE_REF,
    HLFFI_TYPE_VIRTUAL,
    HLFFI_TYPE_DYNOBJ,
    HLFFI_TYPE_ABSTRACT,
    HLFFI_TYPE_ENUM,
    HLFFI_TYPE_NULL,
    HLFFI_TYPE_METHOD,
    HLFFI_TYPE_STRUCT,
    HLFFI_TYPE_PACKED
} hlffi_type_kind;

/**
 * Type iterator callback.
 * Called for each type during hlffi_list_types().
 *
 * @param type Type being visited
 * @param userdata User-provided data
 */
typedef void (*hlffi_type_callback)(hlffi_type* type, void* userdata);

/**
 * Find type by name.
 * Searches loaded module for a type with the given name.
 *
 * @param vm VM instance
 * @param name Type name (e.g., "Player", "com.example.MyClass")
 * @return Type handle, or NULL if not found
 *
 * @note For packaged types, use full name: "com.example.Player"
 * @note Check hlffi_get_error() if NULL is returned
 * @note Type handle valid until module unloaded
 */
hlffi_type* hlffi_find_type(hlffi_vm* vm, const char* name);

/**
 * Get type kind.
 * Returns the kind of type (class, enum, primitive, etc.).
 *
 * @param type Type handle
 * @return Type kind enum value
 *
 * @note Returns HLFFI_TYPE_VOID if type is NULL
 */
hlffi_type_kind hlffi_type_get_kind(hlffi_type* type);

/**
 * Get type name.
 * Returns the fully-qualified name of the type.
 *
 * @param type Type handle
 * @return Type name string (UTF-8), or NULL if type is NULL
 *
 * @note String valid until type is unloaded
 * @note For objects: returns class name (e.g., "Player")
 * @note For primitives: returns kind name (e.g., "i32", "f64")
 */
const char* hlffi_type_get_name(hlffi_type* type);

/**
 * Enumerate all types in module.
 * Calls callback for each type in the loaded module.
 *
 * @param vm VM instance
 * @param callback Function to call for each type
 * @param userdata User data passed to callback
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Callback called for ALL types (primitives, classes, enums, etc.)
 * @note Use hlffi_type_get_kind() to filter types in callback
 */
hlffi_error_code hlffi_list_types(hlffi_vm* vm, hlffi_type_callback callback, void* userdata);

/* ========== CLASS INSPECTION ========== */

/**
 * Get superclass of a class type.
 * Returns the parent class, or NULL if no parent.
 *
 * @param type Class type handle
 * @return Parent class type, or NULL if none or not a class
 *
 * @note Only works for HLFFI_TYPE_OBJ types
 * @note Returns NULL for root classes (no parent)
 */
hlffi_type* hlffi_class_get_super(hlffi_type* type);

/**
 * Get number of fields in a class.
 * Returns count of instance fields (not including inherited fields).
 *
 * @param type Class type handle
 * @return Field count, or -1 if not a class type
 *
 * @note Only counts direct fields, not inherited ones
 * @note Use hlffi_class_get_super() to traverse hierarchy
 */
int hlffi_class_get_field_count(hlffi_type* type);

/**
 * Get field name by index.
 * Returns the name of the field at the given index.
 *
 * @param type Class type handle
 * @param index Field index (0 to field_count-1)
 * @return Field name (UTF-8), or NULL if invalid
 *
 * @note Index must be < hlffi_class_get_field_count()
 * @note String valid until type is unloaded
 */
const char* hlffi_class_get_field_name(hlffi_type* type, int index);

/**
 * Get field type by index.
 * Returns the type of the field at the given index.
 *
 * @param type Class type handle
 * @param index Field index (0 to field_count-1)
 * @return Field type handle, or NULL if invalid
 *
 * @note Index must be < hlffi_class_get_field_count()
 */
hlffi_type* hlffi_class_get_field_type(hlffi_type* type, int index);

/**
 * Get number of methods in a class.
 * Returns count of methods (not including inherited methods).
 *
 * @param type Class type handle
 * @return Method count, or -1 if not a class type
 *
 * @note Only counts direct methods, not inherited ones
 */
int hlffi_class_get_method_count(hlffi_type* type);

/**
 * Get method name by index.
 * Returns the name of the method at the given index.
 *
 * @param type Class type handle
 * @param index Method index (0 to method_count-1)
 * @return Method name (UTF-8), or NULL if invalid
 *
 * @note Index must be < hlffi_class_get_method_count()
 * @note String valid until type is unloaded
 */
const char* hlffi_class_get_method_name(hlffi_type* type, int index);

/* ========== PHASE 3: STATIC MEMBERS & VALUES ========== */

/**
 * Create integer value.
 *
 * @param vm VM instance
 * @param value Integer value
 * @return Boxed value handle
 *
 * @note Value is GC-managed, valid until unreachable
 */
hlffi_value* hlffi_value_int(hlffi_vm* vm, int value);

/**
 * Create float value.
 *
 * @param vm VM instance
 * @param value Float value
 * @return Boxed value handle
 */
hlffi_value* hlffi_value_float(hlffi_vm* vm, double value);

/**
 * Create boolean value.
 *
 * @param vm VM instance
 * @param value Boolean value
 * @return Boxed value handle
 */
hlffi_value* hlffi_value_bool(hlffi_vm* vm, bool value);

/**
 * Create string value.
 *
 * @param vm VM instance
 * @param str UTF-8 string (will be converted to UTF-16)
 * @return Boxed value handle, or NULL if string is NULL
 */
hlffi_value* hlffi_value_string(hlffi_vm* vm, const char* str);

/**
 * Create null value.
 *
 * @param vm VM instance
 * @return Boxed null value
 */
hlffi_value* hlffi_value_null(hlffi_vm* vm);

/**
 * Free a value handle.
 *
 * This removes the GC root and frees the wrapper struct.
 * IMPORTANT: Always use this function instead of free() on hlffi_value pointers.
 *
 * @param value Value handle to free (can be NULL)
 */
void hlffi_value_free(hlffi_value* value);

/**
 * Extract integer from value.
 *
 * @param value Value handle
 * @param fallback Fallback value if conversion fails
 * @return Integer value, or fallback if not an integer
 */
int hlffi_value_as_int(hlffi_value* value, int fallback);

/**
 * Extract float from value.
 *
 * @param value Value handle
 * @param fallback Fallback value if conversion fails
 * @return Float value, or fallback if not a float
 */
double hlffi_value_as_float(hlffi_value* value, double fallback);

/**
 * Extract boolean from value.
 *
 * @param value Value handle
 * @param fallback Fallback value if conversion fails
 * @return Boolean value, or fallback if not a boolean
 */
bool hlffi_value_as_bool(hlffi_value* value, bool fallback);

/**
 * Extract string from value.
 * Returns UTF-8 string converted from HashLink's UTF-16.
 *
 * @param value Value handle
 * @return UTF-8 string, or NULL if not a string
 *
 * @note MEMORY OWNERSHIP: Caller must free() the returned string with free()
 * @note Returns NULL if value is NULL or not a string
 * @note The string is a COPY - safe to store and use after freeing hlffi_value
 *
 * Example:
 *   hlffi_value* name = hlffi_get_field(obj, "name");
 *   char* str = hlffi_value_as_string(name);
 *   hlffi_value_free(name);  // Can free value immediately
 *   // str is still valid - it's an independent copy
 *   printf("Name: %s\n", str);
 *   free(str);  // Must free the string when done
 */
char* hlffi_value_as_string(hlffi_value* value);

/**
 * Check if value is null.
 *
 * @param value Value handle
 * @return true if value is null or NULL pointer
 */
bool hlffi_value_is_null(hlffi_value* value);

/**
 * Get static field value.
 *
 * @param vm VM instance
 * @param class_name Class name (e.g., "Game")
 * @param field_name Field name (e.g., "score")
 * @return Field value, or NULL on error
 *
 * @note Check hlffi_get_error() if NULL is returned
 * @note Entry point must be called before accessing static fields
 */
hlffi_value* hlffi_get_static_field(hlffi_vm* vm, const char* class_name, const char* field_name);

/**
 * Set static field value.
 *
 * @param vm VM instance
 * @param class_name Class name
 * @param field_name Field name
 * @param value New value
 * @return HLFFI_OK on success, error code on failure
 *
 * @note Entry point must be called before accessing static fields
 */
hlffi_error_code hlffi_set_static_field(hlffi_vm* vm, const char* class_name, const char* field_name, hlffi_value* value);

/**
 * Call static method.
 *
 * @param vm VM instance
 * @param class_name Class name (e.g., "Game")
 * @param method_name Method name (e.g., "start")
 * @param argc Number of arguments
 * @param argv Argument array (can be NULL if argc=0)
 * @return Return value, or NULL on error/void return
 *
 * @note Check hlffi_get_error() if NULL is returned
 * @note Entry point must be called before calling static methods
 */
hlffi_value* hlffi_call_static(hlffi_vm* vm, const char* class_name, const char* method_name, int argc, hlffi_value** argv);

/* ========== PHASE 4: INSTANCE MEMBERS (OBJECTS) ========== */

/**
 * Create a new instance of a class (call constructor).
 *
 * Creates a new object instance by calling the class constructor.
 * The object is automatically GC-rooted and must be freed with hlffi_value_free().
 *
 * @param vm VM instance
 * @param class_name Fully qualified class name (e.g., "Player", "com.example.MyClass")
 * @param argc Number of constructor arguments
 * @param argv Array of argument values (can be NULL if argc == 0)
 * @return New object instance, or NULL on error
 *
 * @note Entry point must be called before creating instances
 * @note Object is GC-rooted - call hlffi_value_free() when done
 * @note Check hlffi_get_error() if NULL is returned
 *
 * Example:
 *   // Player has: new(name:String)
 *   hlffi_value* name = hlffi_value_string(vm, "Hero");
 *   hlffi_value* player = hlffi_new(vm, "Player", 1, &name);
 *   // ... use player ...
 *   hlffi_value_free(player);
 */
hlffi_value* hlffi_new(hlffi_vm* vm, const char* class_name, int argc, hlffi_value** argv);

/**
 * Get an instance field value.
 *
 * Retrieves the value of an object's instance field by name.
 * Works for all field types (primitives, objects, strings, etc.).
 *
 * @param obj Object instance
 * @param field_name Field name (UTF-8)
 * @return Field value, or NULL on error
 *
 * @note Check hlffi_get_error() if NULL is returned
 * @note Returned value does NOT need to be freed (it's a borrowed reference)
 *
 * Example:
 *   hlffi_value* health = hlffi_get_field(player, "health");
 *   int hp = hlffi_value_as_int(health);
 */
hlffi_value* hlffi_get_field(hlffi_value* obj, const char* field_name);

/**
 * Set an instance field value.
 *
 * Sets the value of an object's instance field by name.
 * Works for all field types (primitives, objects, strings, etc.).
 *
 * @param obj Object instance
 * @param field_name Field name (UTF-8)
 * @param value New value to set
 * @return true on success, false on error
 *
 * @note Check hlffi_get_error() if false is returned
 *
 * Example:
 *   hlffi_value* new_health = hlffi_value_int(vm, 50);
 *   hlffi_set_field(player, "health", new_health);
 *   hlffi_value_free(new_health);
 */
bool hlffi_set_field(hlffi_value* obj, const char* field_name, hlffi_value* value);

/**
 * Call an instance method.
 *
 * Calls a method on an object instance with the given arguments.
 *
 * @param obj Object instance
 * @param method_name Method name (UTF-8)
 * @param argc Number of arguments
 * @param argv Array of argument values (can be NULL if argc == 0)
 * @return Return value, or NULL on error/void return
 *
 * @note Check hlffi_get_error() if NULL is returned
 * @note Returned value should be freed with hlffi_value_free()
 *
 * Example:
 *   hlffi_value* damage = hlffi_value_int(vm, 25);
 *   hlffi_call_method(player, "takeDamage", 1, &damage);
 *   hlffi_value_free(damage);
 */
hlffi_value* hlffi_call_method(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv);

/**
 * Check if a value is an instance of a given class.
 *
 * @param obj Object instance to check
 * @param class_name Class name to check against
 * @return true if obj is an instance of class_name, false otherwise
 *
 * Example:
 *   if (hlffi_is_instance_of(value, "Player")) {
 *       // Safe to use as Player
 *   }
 */
bool hlffi_is_instance_of(hlffi_value* obj, const char* class_name);

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
