# HLFFI API Index

> Complete reference for all 130 HLFFI functions

<details>
<summary><strong>Quick Navigation</strong></summary>

[VM Lifecycle](#vm-lifecycle) · [Integration](#integration-modes) · [Event Loop](#event-loop) · [Threading](#threading) · [Hot Reload](#hot-reload) · [Types](#type-system) · [Values](#value-system) · [Static](#static-members) · [Instance](#instance-members) · [Arrays](#arrays) · [Maps](#maps) · [Bytes](#bytes) · [Enums](#enums) · [Abstracts](#abstracts) · [Callbacks](#callbacks--ffi) · [Exceptions](#exception-handling) · [Performance](#performance--caching)

</details>

---

### VM Lifecycle
<sub>9 functions · Create, initialize, load, and manage the HashLink VM</sub>

| Function | Description |
|----------|-------------|
| `hlffi_create()` | Allocate VM instance and initialize HashLink runtime |
| `hlffi_init(vm, argc, argv)` | Initialize HashLink system with command-line arguments (call once per process) |
| `hlffi_load_file(vm, path)` | Load compiled .hl bytecode from file |
| `hlffi_load_memory(vm, data, size)` | Load compiled .hl bytecode from memory buffer |
| `hlffi_call_entry(vm)` | Execute Haxe Main.main() entry point |
| `hlffi_destroy(vm)` | Free VM instance (call only at process exit) |
| `hlffi_update_stack_top(marker)` | Update GC stack scanning limit for current thread |
| `hlffi_get_error(vm)` | Get last error message (static string) |
| `hlffi_get_error_string(code)` | Convert error code to human-readable string |

<details>
<summary>View details</summary>

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

</details>

---

### Integration Modes
<sub>2 functions · Choose between NON_THREADED and THREADED execution</sub>

| Function | Description |
|----------|-------------|
| `hlffi_set_integration_mode(vm, mode)` | Set integration mode (must call before entry point) |
| `hlffi_get_integration_mode(vm)` | Get current integration mode |

---

### Event Loop
<sub>4 functions · Process UV loop and Haxe EventLoop for timers and async operations</sub>

| Function | Description |
|----------|-------------|
| `hlffi_update(vm, delta_time)` | Process UV loop and Haxe EventLoop for one frame |
| `hlffi_has_pending_work(vm)` | Check if VM has pending events/timers |
| `hlffi_process_events(vm, type)` | Process specific event loop type (UV_LOOP or HAXE_EVENTLOOP) |
| `hlffi_has_pending_events(vm, type)` | Check if specific event loop has pending work |

---

### Threading
<sub>9 functions · VM thread management and worker thread support</sub>

| Function | Description |
|----------|-------------|
| `hlffi_thread_start(vm)` | Start dedicated VM thread (THREADED mode only) |
| `hlffi_thread_stop(vm)` | Stop VM thread gracefully |
| `hlffi_thread_is_running(vm)` | Check if VM thread is currently running |
| `hlffi_thread_call_sync(vm, func, userdata)` | Execute function on VM thread and wait for completion |
| `hlffi_thread_call_async(vm, func, userdata, callback, data)` | Execute function on VM thread asynchronously |
| `hlffi_worker_register()` | Register current C thread with HashLink GC |
| `hlffi_worker_unregister()` | Unregister current thread from GC |
| `hlffi_blocking_begin()` | Notify GC that thread is starting external I/O |
| `hlffi_blocking_end()` | Notify GC that thread returned from external I/O |

---

### Hot Reload
<sub>6 functions · Runtime code updates without restart</sub>

| Function | Description |
|----------|-------------|
| `hlffi_enable_hot_reload(vm, enable)` | Enable/disable hot reload support (must call before load) |
| `hlffi_is_hot_reload_enabled(vm)` | Check if hot reload is enabled |
| `hlffi_reload_module(vm, path)` | Reload bytecode from file (updates code, preserves globals) |
| `hlffi_reload_module_memory(vm, data, size)` | Reload bytecode from memory buffer |
| `hlffi_set_reload_callback(vm, callback, userdata)` | Set callback invoked after successful reload |
| `hlffi_check_reload(vm)` | Check if bytecode file changed and reload if needed |

---

### Type System
<sub>10 functions · Type introspection and reflection</sub>

| Function | Description |
|----------|-------------|
| `hlffi_find_type(vm, name)` | Find type by fully-qualified name |
| `hlffi_type_get_kind(type)` | Get type category (VOID, I32, F64, OBJ, etc.) |
| `hlffi_list_types(vm, callback, userdata)` | Enumerate all types in loaded module |
| `hlffi_class_get_super(type)` | Get parent class type |
| `hlffi_class_get_field_count(type)` | Get number of fields in class |
| `hlffi_class_get_field_name(type, index)` | Get field name by index |
| `hlffi_class_get_field_type(type, index)` | Get field type by index |
| `hlffi_class_get_method_count(type)` | Get number of methods in class |
| `hlffi_class_get_method_name(type, index)` | Get method name by index |
| `hlffi_is_jit_mode()` | Check if running in JIT mode (vs interpreter) |

---

### Value System
<sub>11 functions · Boxing and unboxing between C and Haxe types</sub>

| Function | Description |
|----------|-------------|
| `hlffi_value_int(vm, value)` | Box C int to Haxe value (temporary, not GC-rooted) |
| `hlffi_value_float(vm, value)` | Box C double to Haxe Float value |
| `hlffi_value_bool(vm, value)` | Box C bool to Haxe Bool value |
| `hlffi_value_string(vm, str)` | Box C UTF-8 string to Haxe String (converts to UTF-16) |
| `hlffi_value_null(vm)` | Create null value |
| `hlffi_value_free(value)` | Free value and remove GC root |
| `hlffi_value_as_int(value, fallback)` | Unbox Haxe value to C int |
| `hlffi_value_as_float(value, fallback)` | Unbox Haxe Float to C double |
| `hlffi_value_as_bool(value, fallback)` | Unbox Haxe Bool to C bool |
| `hlffi_value_as_string(value)` | Unbox Haxe String to C UTF-8 string (caller must free) |
| `hlffi_value_is_null(value)` | Check if value is null |

---

### Static Members
<sub>3 functions · Access static fields and methods</sub>

| Function | Description |
|----------|-------------|
| `hlffi_get_static_field(vm, class, field)` | Get static field value by name |
| `hlffi_set_static_field(vm, class, field, value)` | Set static field value |
| `hlffi_call_static(vm, class, method, argc, argv)` | Call static method by name |

---

### Instance Members
<sub>16 functions · Create objects and access instance fields/methods</sub>

| Function | Description |
|----------|-------------|
| `hlffi_new(vm, class, argc, argv)` | Create new object instance (GC-rooted, must free) |
| `hlffi_get_field(obj, field)` | Get instance field value |
| `hlffi_set_field(obj, field, value)` | Set instance field value |
| `hlffi_get_field_int(obj, field, fallback)` | Get int field with fallback (convenience) |
| `hlffi_get_field_float(obj, field, fallback)` | Get float field with fallback |
| `hlffi_get_field_bool(obj, field, fallback)` | Get bool field with fallback |
| `hlffi_get_field_string(obj, field)` | Get string field (caller must free) |
| `hlffi_set_field_int(vm, obj, field, value)` | Set int field (convenience) |
| `hlffi_set_field_float(vm, obj, field, value)` | Set float field |
| `hlffi_set_field_bool(vm, obj, field, value)` | Set bool field |
| `hlffi_set_field_string(vm, obj, field, value)` | Set string field from C UTF-8 string |
| `hlffi_call_method(obj, method, argc, argv)` | Call instance method |
| `hlffi_call_method_void(obj, method, argc, argv)` | Call void method (convenience) |
| `hlffi_call_method_int(obj, method, argc, argv, fallback)` | Call method returning int |
| `hlffi_call_method_float(obj, method, argc, argv, fallback)` | Call method returning float |
| `hlffi_call_method_bool(obj, method, argc, argv, fallback)` | Call method returning bool |
| `hlffi_call_method_string(obj, method, argc, argv)` | Call method returning string (caller must free) |
| `hlffi_is_instance_of(obj, class)` | Check if object is instance of class |

---

### Arrays
<sub>12 functions · Dynamic arrays, NativeArrays, and struct arrays</sub>

| Function | Description |
|----------|-------------|
| `hlffi_array_new(vm, type, length)` | Create dynamic array with initial length |
| `hlffi_array_length(arr)` | Get array length |
| `hlffi_array_get(vm, arr, index)` | Get array element at index |
| `hlffi_array_set(vm, arr, index, value)` | Set array element at index |
| `hlffi_array_push(vm, arr, value)` | Append element to array (grows by 1) |
| `hlffi_native_array_new(vm, type, length)` | Create NativeArray for zero-copy access |
| `hlffi_native_array_get_ptr(arr)` | Get direct pointer to NativeArray data |
| `hlffi_array_new_struct(vm, type, length)` | Create array of structs (@:struct classes) |
| `hlffi_array_get_struct(arr, index)` | Get pointer to struct at index (zero-copy) |
| `hlffi_array_set_struct(vm, arr, index, ptr, size)` | Copy struct into array at index |
| `hlffi_native_array_new_struct(vm, type, length)` | Create NativeArray of structs |
| `hlffi_native_array_get_struct_ptr(arr)` | Get pointer to first struct in NativeArray |

---

### Maps
<sub>9 functions · Hash tables and dictionaries</sub>

| Function | Description |
|----------|-------------|
| `hlffi_map_new(vm, key_type, value_type)` | Create hash map with key/value types |
| `hlffi_map_set(vm, map, key, value)` | Insert or update key-value pair |
| `hlffi_map_get(vm, map, key)` | Get value for key |
| `hlffi_map_exists(vm, map, key)` | Check if key exists in map |
| `hlffi_map_remove(vm, map, key)` | Remove key-value pair |
| `hlffi_map_keys(vm, map)` | Get array of all keys |
| `hlffi_map_values(vm, map)` | Get array of all values |
| `hlffi_map_size(map)` | Get number of entries |
| `hlffi_map_clear(map)` | Remove all entries |

---

### Bytes
<sub>11 functions · Binary data and byte buffers</sub>

| Function | Description |
|----------|-------------|
| `hlffi_bytes_new(vm, size)` | Allocate byte buffer (zero-initialized) |
| `hlffi_bytes_from_data(vm, data, size)` | Create bytes from C data (copies data) |
| `hlffi_bytes_from_string(vm, str)` | Create bytes from C string (no null terminator) |
| `hlffi_bytes_get_ptr(bytes)` | Get direct pointer to byte buffer |
| `hlffi_bytes_get_length(bytes)` | Get byte buffer length |
| `hlffi_bytes_blit(dst, dst_pos, src, src_pos, len)` | Copy bytes from src to dst |
| `hlffi_bytes_compare(a, a_pos, b, b_pos, len)` | Compare byte ranges |
| `hlffi_bytes_to_string(bytes, length)` | Convert bytes to C string (caller must free) |
| `hlffi_bytes_get(bytes, index)` | Get single byte at index |
| `hlffi_bytes_set(bytes, index, value)` | Set single byte at index |
| `hlffi_bytes_fill(bytes, pos, len, value)` | Fill byte range with value |

---

### Enums
<sub>10 functions · Algebraic data types with constructors</sub>

| Function | Description |
|----------|-------------|
| `hlffi_enum_get_construct_count(vm, type_name)` | Get number of enum constructors |
| `hlffi_enum_get_construct_name(vm, type_name, index)` | Get constructor name by index (caller must free) |
| `hlffi_enum_get_index(value)` | Get constructor index of enum value |
| `hlffi_enum_get_name(value)` | Get constructor name of enum value (caller must free) |
| `hlffi_enum_get_param_count(value)` | Get number of constructor parameters |
| `hlffi_enum_get_param(value, param_index)` | Get constructor parameter at index |
| `hlffi_enum_alloc_simple(vm, type_name, index)` | Create enum with no parameters |
| `hlffi_enum_alloc(vm, type_name, index, nparams, params)` | Create enum with constructor parameters |
| `hlffi_enum_is(value, index)` | Check if enum is specific constructor index |
| `hlffi_enum_is_named(value, name)` | Check if enum is specific constructor name |

---

### Abstracts
<sub>5 functions · Abstract type detection and handling</sub>

| Function | Description |
|----------|-------------|
| `hlffi_is_abstract(type)` | Check if type is abstract |
| `hlffi_abstract_get_name(type)` | Get abstract type name (caller must free) |
| `hlffi_abstract_find(vm, name)` | Find abstract type by name |
| `hlffi_value_is_abstract(value)` | Check if value is abstract type |
| `hlffi_value_get_abstract_name(value)` | Get abstract name of value (caller must free) |

---

### Callbacks & FFI
<sub>4 functions · C ↔ Haxe interop</sub>

| Function | Description |
|----------|-------------|
| `hlffi_register_callback(vm, name, func, nargs)` | Register C callback callable from Haxe |
| `hlffi_register_callback_typed(vm, name, func, nargs, arg_types, ret_type)` | Register typed callback (deprecated - use Dynamic) |
| `hlffi_get_callback(vm, name)` | Get registered callback function |
| `hlffi_unregister_callback(vm, name)` | Remove registered callback |

---

### Exception Handling
<sub>7 functions · Try-catch support and stack traces</sub>

| Function | Description |
|----------|-------------|
| `hlffi_try_call_static(vm, class, method, argc, argv)` | Call static method with exception safety |
| `hlffi_try_call_method(obj, method, argc, argv)` | Call instance method with exception safety |
| `hlffi_get_exception_message(exception)` | Get exception message (caller must free) |
| `hlffi_get_exception_stack(exception)` | Get exception stack trace (caller must free) |
| `hlffi_is_exception(value)` | Check if value is exception object |
| `hlffi_has_exception(vm)` | Check if VM has pending exception |
| `hlffi_clear_exception(vm)` | Clear pending exception |

---

### Performance & Caching
<sub>5 functions · Method caching for 60x speedup</sub>

| Function | Description |
|----------|-------------|
| `hlffi_cache_static_method(vm, class, method)` | Cache static method for 60x faster calls |
| `hlffi_call_cached(cached, argc, argv)` | Call cached static method |
| `hlffi_cached_call_free(cached)` | Free cached method handle |
| `hlffi_cache_instance_method(vm, class, method)` | Cache instance method (NOT YET IMPLEMENTED) |
| `hlffi_call_cached_method(cached, instance, argc, argv)` | Call cached instance method (NOT YET IMPLEMENTED) |

---

<sub>**130 total functions** · See individual API docs for detailed examples</sub>
