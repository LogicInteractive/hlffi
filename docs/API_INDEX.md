# HLFFI API Index

> Quick reference for all 130 HLFFI functions with brief descriptions

---

## VM Lifecycle

```c
hlffi_vm* hlffi_create(void)
```
Allocate VM instance and initialize HashLink runtime. Returns VM handle or NULL on failure.

```c
hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv)
```
Initialize HashLink system with command-line arguments (call once per process). Sets up GC, registers main thread, and initializes libuv.

```c
hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path)
```
Load compiled .hl bytecode from file path. Returns HLFFI_OK on success or error code on failure.

```c
hlffi_error_code hlffi_load_memory(hlffi_vm* vm, const void* data, size_t size)
```
Load compiled .hl bytecode from memory buffer. Returns HLFFI_OK on success or error code on failure.

```c
hlffi_error_code hlffi_call_entry(hlffi_vm* vm)
```
Execute Haxe Main.main() entry point. Initializes globals and runs main function.

```c
void hlffi_destroy(hlffi_vm* vm)
```
Free VM instance (call only at process exit). Releases all HashLink resources.

```c
void hlffi_update_stack_top(void* stack_marker)
```
Update GC stack scanning limit for current thread. Call when switching stacks or in deep recursion.

```c
const char* hlffi_get_error(hlffi_vm* vm)
```
Get last error message (static string, do not free). Returns detailed error description.

```c
const char* hlffi_get_error_string(hlffi_error_code code)
```
Convert error code to human-readable string. Returns static error message for given code.

---

## Integration Modes

```c
hlffi_error_code hlffi_set_integration_mode(hlffi_vm* vm, hlffi_integration_mode mode)
```
Set integration mode (NON_THREADED or THREADED). Must call before hlffi_call_entry().

```c
hlffi_integration_mode hlffi_get_integration_mode(hlffi_vm* vm)
```
Get current integration mode. Returns HLFFI_NON_THREADED or HLFFI_THREADED.

---

## Event Loop

```c
hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time)
```
Process UV loop and Haxe EventLoop for one frame. Call every frame in NON_THREADED mode (processes timers, async ops).

```c
bool hlffi_has_pending_work(hlffi_vm* vm)
```
Check if VM has pending events/timers. Returns true if hlffi_update() should be called.

```c
hlffi_error_code hlffi_process_events(hlffi_vm* vm, hlffi_eventloop_type type)
```
Process specific event loop type (UV_LOOP or HAXE_EVENTLOOP). Lower-level control than hlffi_update().

```c
bool hlffi_has_pending_events(hlffi_vm* vm, hlffi_eventloop_type type)
```
Check if specific event loop has pending work. Returns true if events are queued for processing.

---

## Threading

```c
hlffi_error_code hlffi_thread_start(hlffi_vm* vm)
```
Start dedicated VM thread (THREADED mode only). Spawns background thread running event loop.

```c
hlffi_error_code hlffi_thread_stop(hlffi_vm* vm)
```
Stop VM thread gracefully. Waits for thread to finish current work.

```c
bool hlffi_thread_is_running(hlffi_vm* vm)
```
Check if VM thread is currently running. Returns true if thread is active.

```c
hlffi_error_code hlffi_thread_call_sync(hlffi_vm* vm, hlffi_thread_func func, void* userdata)
```
Execute function on VM thread and wait for completion (blocking). Returns when function finishes executing.

```c
hlffi_error_code hlffi_thread_call_async(hlffi_vm* vm, hlffi_thread_func func, void* userdata,
                                         hlffi_async_callback callback, void* callback_data)
```
Execute function on VM thread asynchronously (non-blocking). Returns immediately, callback invoked when complete.

```c
void hlffi_worker_register(void)
```
Register current C thread with HashLink GC. Must call before accessing HLFFI from worker threads.

```c
void hlffi_worker_unregister(void)
```
Unregister current thread from GC. Must call before worker thread exits.

```c
void hlffi_blocking_begin(void)
```
Notify GC that thread is starting external I/O operation. Allows GC to run while thread is blocked.

```c
void hlffi_blocking_end(void)
```
Notify GC that thread returned from external I/O. Must balance every hlffi_blocking_begin().

---

## Hot Reload

```c
hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable)
```
Enable/disable hot reload support (must call before load). Requires HashLink 1.12+ for module patching.

```c
bool hlffi_is_hot_reload_enabled(hlffi_vm* vm)
```
Check if hot reload is enabled. Returns true if hot reload was enabled before load.

```c
hlffi_error_code hlffi_reload_module(hlffi_vm* vm, const char* path)
```
Reload bytecode from file (updates code, preserves globals). Returns HLFFI_OK on success.

```c
hlffi_error_code hlffi_reload_module_memory(hlffi_vm* vm, const void* data, size_t size)
```
Reload bytecode from memory buffer. Same as hlffi_reload_module() but from RAM.

```c
void hlffi_set_reload_callback(hlffi_vm* vm, hlffi_reload_callback callback, void* userdata)
```
Set callback invoked after successful reload. Callback receives VM and userdata.

```c
bool hlffi_check_reload(hlffi_vm* vm)
```
Check if bytecode file changed and reload if needed. Returns true if reload occurred (automatic file watching).

---

## Type System

```c
hlffi_type* hlffi_find_type(hlffi_vm* vm, const char* name)
```
Find type by fully-qualified name (e.g., "Player"). Returns type handle or NULL if not found.

```c
hlffi_type_kind hlffi_type_get_kind(hlffi_type* type)
```
Get type category (VOID, I32, F64, OBJ, etc.). Returns enum value indicating type kind.

```c
hlffi_error_code hlffi_list_types(hlffi_vm* vm, hlffi_type_callback callback, void* userdata)
```
Enumerate all types in loaded module. Calls callback for each type.

```c
hlffi_type* hlffi_class_get_super(hlffi_type* type)
```
Get parent class type. Returns superclass or NULL if no parent.

```c
int hlffi_class_get_field_count(hlffi_type* type)
```
Get number of fields in class. Returns field count or -1 on error.

```c
const char* hlffi_class_get_field_name(hlffi_type* type, int index)
```
Get field name by index. Returns static string (do not free).

```c
hlffi_type* hlffi_class_get_field_type(hlffi_type* type, int index)
```
Get field type by index. Returns type handle or NULL.

```c
int hlffi_class_get_method_count(hlffi_type* type)
```
Get number of methods in class. Returns method count or -1 on error.

```c
const char* hlffi_class_get_method_name(hlffi_type* type, int index)
```
Get method name by index. Returns static string (do not free).

```c
bool hlffi_is_jit_mode(void)
```
Check if running in JIT mode (vs interpreter). Returns true if JIT compiler is active.

---

## Value System

```c
hlffi_value* hlffi_value_int(hlffi_vm* vm, int value)
```
Box C int to Haxe value (temporary, not GC-rooted). Use immediately or pass to Haxe functions.

```c
hlffi_value* hlffi_value_float(hlffi_vm* vm, double value)
```
Box C double to Haxe Float value. Temporary value, free with hlffi_value_free().

```c
hlffi_value* hlffi_value_bool(hlffi_vm* vm, bool value)
```
Box C bool to Haxe Bool value. Temporary value for immediate use.

```c
hlffi_value* hlffi_value_string(hlffi_vm* vm, const char* str)
```
Box C UTF-8 string to Haxe String (converts to UTF-16). Copies string data, original can be freed.

```c
hlffi_value* hlffi_value_null(hlffi_vm* vm)
```
Create null value. Used for void returns or null parameters.

```c
void hlffi_value_free(hlffi_value* value)
```
Free value and remove GC root. Must call for all non-temporary values.

```c
int hlffi_value_as_int(hlffi_value* value, int fallback)
```
Unbox Haxe value to C int. Returns fallback if value is null or wrong type.

```c
double hlffi_value_as_float(hlffi_value* value, double fallback)
```
Unbox Haxe Float to C double. Returns fallback on error.

```c
bool hlffi_value_as_bool(hlffi_value* value, bool fallback)
```
Unbox Haxe Bool to C bool. Returns fallback on error.

```c
char* hlffi_value_as_string(hlffi_value* value)
```
Unbox Haxe String to C UTF-8 string (caller must free()). Converts UTF-16 to UTF-8, allocates new buffer.

```c
bool hlffi_value_is_null(hlffi_value* value)
```
Check if value is null. Returns true for null values.

---

## Static Members

```c
hlffi_value* hlffi_get_static_field(hlffi_vm* vm, const char* class_name, const char* field_name)
```
Get static field value by name. Returns value or NULL on error.

```c
hlffi_error_code hlffi_set_static_field(hlffi_vm* vm, const char* class_name,
                                        const char* field_name, hlffi_value* value)
```
Set static field value. Returns HLFFI_OK on success.

```c
hlffi_value* hlffi_call_static(hlffi_vm* vm, const char* class_name, const char* method_name,
                               int argc, hlffi_value** argv)
```
Call static method by name. Returns result value or NULL on error.

---

## Instance Members

```c
hlffi_value* hlffi_new(hlffi_vm* vm, const char* class_name, int argc, hlffi_value** argv)
```
Create new object instance (GC-rooted, must free). Calls constructor with given arguments.

```c
hlffi_value* hlffi_get_field(hlffi_value* obj, const char* field_name)
```
Get instance field value. Returns value or NULL on error.

```c
bool hlffi_set_field(hlffi_value* obj, const char* field_name, hlffi_value* value)
```
Set instance field value. Returns true on success.

```c
int hlffi_get_field_int(hlffi_value* obj, const char* field_name, int fallback)
```
Get int field with fallback (convenience). Returns field value or fallback on error.

```c
float hlffi_get_field_float(hlffi_value* obj, const char* field_name, float fallback)
```
Get float field with fallback. Returns field value or fallback.

```c
bool hlffi_get_field_bool(hlffi_value* obj, const char* field_name, bool fallback)
```
Get bool field with fallback. Returns field value or fallback.

```c
char* hlffi_get_field_string(hlffi_value* obj, const char* field_name)
```
Get string field (caller must free()). Returns UTF-8 string or NULL on error.

```c
bool hlffi_set_field_int(hlffi_vm* vm, hlffi_value* obj, const char* field_name, int value)
```
Set int field (convenience). Returns true on success.

```c
bool hlffi_set_field_float(hlffi_vm* vm, hlffi_value* obj, const char* field_name, float value)
```
Set float field. Returns true on success.

```c
bool hlffi_set_field_bool(hlffi_vm* vm, hlffi_value* obj, const char* field_name, bool value)
```
Set bool field. Returns true on success.

```c
bool hlffi_set_field_string(hlffi_vm* vm, hlffi_value* obj, const char* field_name, const char* value)
```
Set string field from C UTF-8 string. Returns true on success.

```c
hlffi_value* hlffi_call_method(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv)
```
Call instance method. Returns result or NULL on error.

```c
bool hlffi_call_method_void(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv)
```
Call void method (convenience). Returns true on success.

```c
int hlffi_call_method_int(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv, int fallback)
```
Call method returning int. Returns result or fallback on error.

```c
float hlffi_call_method_float(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv, float fallback)
```
Call method returning float. Returns result or fallback.

```c
bool hlffi_call_method_bool(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv, bool fallback)
```
Call method returning bool. Returns result or fallback.

```c
char* hlffi_call_method_string(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv)
```
Call method returning string (caller must free()). Returns UTF-8 string or NULL.

```c
bool hlffi_is_instance_of(hlffi_value* obj, const char* class_name)
```
Check if object is instance of class. Returns true if obj is of type class_name.

---

## Arrays

```c
hlffi_value* hlffi_array_new(hlffi_vm* vm, hl_type* element_type, int length)
```
Create dynamic array with initial length. Element type NULL for Dynamic arrays.

```c
int hlffi_array_length(hlffi_value* arr)
```
Get array length. Returns element count or -1 on error.

```c
hlffi_value* hlffi_array_get(hlffi_vm* vm, hlffi_value* arr, int index)
```
Get array element at index. Returns element value or NULL if out of bounds.

```c
bool hlffi_array_set(hlffi_vm* vm, hlffi_value* arr, int index, hlffi_value* value)
```
Set array element at index. Returns true on success.

```c
bool hlffi_array_push(hlffi_vm* vm, hlffi_value* arr, hlffi_value* value)
```
Append element to array (grows by 1). Returns true on success.

```c
hlffi_value* hlffi_native_array_new(hlffi_vm* vm, hl_type* element_type, int length)
```
Create NativeArray for zero-copy access. Fixed size, direct memory access.

```c
void* hlffi_native_array_get_ptr(hlffi_value* arr)
```
Get direct pointer to NativeArray data. Zero-copy access to array memory.

```c
hlffi_value* hlffi_array_new_struct(hlffi_vm* vm, hlffi_type* struct_type, int length)
```
Create array of structs (@:struct classes). Fixed-size, zero-copy struct storage.

```c
void* hlffi_array_get_struct(hlffi_value* arr, int index)
```
Get pointer to struct at index (zero-copy). Returns direct pointer, modify in-place.

```c
bool hlffi_array_set_struct(hlffi_vm* vm, hlffi_value* arr, int index, void* struct_ptr, int struct_size)
```
Copy struct into array at index. Copies struct data from ptr.

```c
hlffi_value* hlffi_native_array_new_struct(hlffi_vm* vm, hlffi_type* struct_type, int length)
```
Create NativeArray of structs. Zero-copy fixed-size struct array.

```c
void* hlffi_native_array_get_struct_ptr(hlffi_value* arr)
```
Get pointer to first struct in NativeArray. Direct pointer to array memory.

---

## Maps

```c
hlffi_value* hlffi_map_new(hlffi_vm* vm, hl_type* key_type, hl_type* value_type)
```
Create hash map with key/value types. Use NULL for Dynamic types.

```c
bool hlffi_map_set(hlffi_vm* vm, hlffi_value* map, hlffi_value* key, hlffi_value* value)
```
Insert or update key-value pair. Returns true on success.

```c
hlffi_value* hlffi_map_get(hlffi_vm* vm, hlffi_value* map, hlffi_value* key)
```
Get value for key. Returns value or NULL if not found.

```c
bool hlffi_map_exists(hlffi_vm* vm, hlffi_value* map, hlffi_value* key)
```
Check if key exists in map. Returns true if key present.

```c
bool hlffi_map_remove(hlffi_vm* vm, hlffi_value* map, hlffi_value* key)
```
Remove key-value pair. Returns true if key was removed.

```c
hlffi_value* hlffi_map_keys(hlffi_vm* vm, hlffi_value* map)
```
Get array of all keys. Returns array of keys.

```c
hlffi_value* hlffi_map_values(hlffi_vm* vm, hlffi_value* map)
```
Get array of all values. Order matches hlffi_map_keys().

```c
int hlffi_map_size(hlffi_value* map)
```
Get number of entries. Returns entry count or -1 on error.

```c
bool hlffi_map_clear(hlffi_value* map)
```
Remove all entries. Returns true on success.

---

## Bytes

```c
hlffi_value* hlffi_bytes_new(hlffi_vm* vm, int size)
```
Allocate byte buffer (zero-initialized). Returns bytes object or NULL.

```c
hlffi_value* hlffi_bytes_from_data(hlffi_vm* vm, const void* data, int size)
```
Create bytes from C data (copies data). Returns bytes object containing copy.

```c
hlffi_value* hlffi_bytes_from_string(hlffi_vm* vm, const char* str)
```
Create bytes from C string (no null terminator). Copies string contents.

```c
void* hlffi_bytes_get_ptr(hlffi_value* bytes)
```
Get direct pointer to byte buffer. Zero-copy access for in-place modification.

```c
int hlffi_bytes_get_length(hlffi_value* bytes)
```
Get byte buffer length. Returns size in bytes.

```c
bool hlffi_bytes_blit(hlffi_value* dst, int dst_pos, hlffi_value* src, int src_pos, int len)
```
Copy bytes from src to dst. Returns true on success.

```c
int hlffi_bytes_compare(hlffi_value* a, int a_pos, hlffi_value* b, int b_pos, int len)
```
Compare byte ranges. Returns <0, 0, >0 like memcmp().

```c
char* hlffi_bytes_to_string(hlffi_value* bytes, int length)
```
Convert bytes to C string (caller must free()). Null-terminates and returns copy.

```c
int hlffi_bytes_get(hlffi_value* bytes, int index)
```
Get single byte at index. Returns byte value (0-255) or -1 on error.

```c
bool hlffi_bytes_set(hlffi_value* bytes, int index, int value)
```
Set single byte at index. Returns true on success.

```c
bool hlffi_bytes_fill(hlffi_value* bytes, int pos, int len, int value)
```
Fill byte range with value. Returns true on success.

---

## Enums

```c
int hlffi_enum_get_construct_count(hlffi_vm* vm, const char* type_name)
```
Get number of enum constructors. Returns constructor count or -1 on error.

```c
char* hlffi_enum_get_construct_name(hlffi_vm* vm, const char* type_name, int index)
```
Get constructor name by index (caller must free()). Returns constructor name or NULL.

```c
int hlffi_enum_get_index(hlffi_value* value)
```
Get constructor index of enum value. Returns index or -1 if not enum.

```c
char* hlffi_enum_get_name(hlffi_value* value)
```
Get constructor name of enum value (caller must free()). Returns name or NULL.

```c
int hlffi_enum_get_param_count(hlffi_value* value)
```
Get number of constructor parameters. Returns param count or -1.

```c
hlffi_value* hlffi_enum_get_param(hlffi_value* value, int param_index)
```
Get constructor parameter at index. Returns parameter value or NULL.

```c
hlffi_value* hlffi_enum_alloc_simple(hlffi_vm* vm, const char* type_name, int index)
```
Create enum with no parameters. Returns enum value or NULL.

```c
hlffi_value* hlffi_enum_alloc(hlffi_vm* vm, const char* type_name, int index, int nparams, hlffi_value** params)
```
Create enum with constructor parameters. Returns enum value or NULL.

```c
bool hlffi_enum_is(hlffi_value* value, int index)
```
Check if enum is specific constructor index. Returns true if matches.

```c
bool hlffi_enum_is_named(hlffi_value* value, const char* name)
```
Check if enum is specific constructor name. Returns true if matches.

---

## Abstracts

```c
bool hlffi_is_abstract(hlffi_type* type)
```
Check if type is abstract. Returns true for abstract types.

```c
char* hlffi_abstract_get_name(hlffi_type* type)
```
Get abstract type name (caller must free()). Returns name or NULL if not abstract.

```c
hlffi_type* hlffi_abstract_find(hlffi_vm* vm, const char* name)
```
Find abstract type by name. Returns type or NULL if not found.

```c
bool hlffi_value_is_abstract(hlffi_value* value)
```
Check if value is abstract type. Returns true for abstract values.

```c
char* hlffi_value_get_abstract_name(hlffi_value* value)
```
Get abstract name of value (caller must free()). Returns name or NULL.

---

## Callbacks & FFI

```c
bool hlffi_register_callback(hlffi_vm* vm, const char* name, hlffi_native_func func, int nargs)
```
Register C callback callable from Haxe. Returns true on success (register before load).

```c
bool hlffi_register_callback_typed(hlffi_vm* vm, const char* name, hlffi_native_func func,
                                    int nargs, hlffi_type** arg_types, hlffi_type* ret_type)
```
Register typed callback (deprecated - use Dynamic). Returns true on success.

```c
hlffi_value* hlffi_get_callback(hlffi_vm* vm, const char* name)
```
Get registered callback function. Returns callback or NULL if not registered.

```c
bool hlffi_unregister_callback(hlffi_vm* vm, const char* name)
```
Remove registered callback. Returns true if callback was found.

---

## Exception Handling

```c
hlffi_call_result hlffi_try_call_static(hlffi_vm* vm, const char* class_name, const char* method_name,
                                        int argc, hlffi_value** argv)
```
Call static method with exception safety. Returns result with value OR exception set.

```c
hlffi_call_result hlffi_try_call_method(hlffi_value* obj, const char* method_name,
                                        int argc, hlffi_value** argv)
```
Call instance method with exception safety. Returns result with value OR exception set.

```c
char* hlffi_get_exception_message(hlffi_value* exception)
```
Get exception message (caller must free()). Returns error message string.

```c
char* hlffi_get_exception_stack(hlffi_value* exception)
```
Get exception stack trace (caller must free()). Returns full stack trace.

```c
bool hlffi_is_exception(hlffi_value* value)
```
Check if value is exception object. Returns true for exceptions.

```c
bool hlffi_has_exception(hlffi_vm* vm)
```
Check if VM has pending exception. Returns true if exception occurred.

```c
void hlffi_clear_exception(hlffi_vm* vm)
```
Clear pending exception. Resets exception state.

---

## Performance & Caching

```c
hlffi_cached_call* hlffi_cache_static_method(hlffi_vm* vm, const char* class_name, const char* method_name)
```
Cache static method for 60x faster calls. Returns cache handle or NULL on error.

```c
hlffi_value* hlffi_call_cached(hlffi_cached_call* cached, int argc, hlffi_value** argv)
```
Call cached static method. Returns result value.

```c
void hlffi_cached_call_free(hlffi_cached_call* cached)
```
Free cached method handle. Must call to avoid memory leak.

```c
hlffi_cached_call* hlffi_cache_instance_method(hlffi_vm* vm, const char* class_name, const char* method_name)
```
Cache instance method (NOT YET IMPLEMENTED). Returns NULL currently.

```c
hlffi_value* hlffi_call_cached_method(hlffi_cached_call* cached, hlffi_value* instance,
                                      int argc, hlffi_value** argv)
```
Call cached instance method (NOT YET IMPLEMENTED). Returns NULL currently.

---

**Total Functions:** 130
