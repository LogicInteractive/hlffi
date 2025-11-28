# HLFFI API Reference

> Comprehensive API documentation organized by topic

<sub>**Version 3.0.0** · Last updated November 28, 2025</sub>

<details>
<summary><strong>Quick Navigation</strong></summary>

[VM Lifecycle](#vm-lifecycle) · [Integration](#integration-modes) · [Event Loop](#event-loop) · [Threading](#threading) · [Hot Reload](#hot-reload) · [Types](#type-system--reflection) · [Values](#value-system) · [Static](#static-members) · [Instance](#instance-members) · [Arrays](#arrays) · [Maps](#maps) · [Bytes](#bytes) · [Enums](#enums) · [Abstracts](#abstracts) · [Callbacks](#callbacks--ffi) · [Exceptions](#exceptions) · [Performance](#performance--caching) · [Utilities](#utilities--helpers) · [Errors](#error-handling)

</details>

---

### Core VM Management

#### VM Lifecycle
<sub>[API_01_VM_LIFECYCLE.md](API_01_VM_LIFECYCLE.md) · 10 functions</sub>

Create, initialize, load bytecode, run programs, and manage the HashLink VM instance.

**Key functions:** `hlffi_create()` · `hlffi_init()` · `hlffi_load_file()` · `hlffi_call_entry()` · `hlffi_destroy()`

**Topics:** Error handling · GC stack management · Process lifecycle

---

#### Integration Modes
<sub>[API_02_INTEGRATION_MODES.md](API_02_INTEGRATION_MODES.md) · 2 functions</sub>

Choose between NON_THREADED (engine controls loop) and THREADED (dedicated VM thread) execution modes.

**Key functions:** `hlffi_set_integration_mode()` · `hlffi_get_integration_mode()`

**Decision guide:** Use NON_THREADED for Unreal/Unity/Godot, THREADED for standalone apps

---

#### Event Loop Integration
<sub>[API_03_EVENT_LOOP.md](API_03_EVENT_LOOP.md) · 5 functions</sub>

Process UV loop and Haxe EventLoop for timers, `haxe.Timer`, and `MainLoop.add()` callbacks.

**Key functions:** `hlffi_update()` · `hlffi_process_events()` · `hlffi_has_pending_work()`

**Use case:** Call `hlffi_update()` every frame in NON_THREADED mode

---

#### Threading
<sub>[API_04_THREADING.md](API_04_THREADING.md) · 10 functions</sub>

Dedicated VM thread management, worker thread registration, and message queue architecture.

**Key functions:** `hlffi_thread_start()` · `hlffi_thread_call_sync()` · `hlffi_thread_call_async()` · `hlffi_worker_register()`

**Topics:** Sync/async calls · Worker threads · Blocking operations

---

#### Hot Reload
<sub>[API_05_HOT_RELOAD.md](API_05_HOT_RELOAD.md) · 7 functions</sub>

Runtime code updates without restarting. Reload bytecode while preserving static state.

**Key functions:** `hlffi_enable_hot_reload()` · `hlffi_reload_module()` · `hlffi_check_reload()`

**Features:** Callback notifications · Automatic file watching · Requires HashLink 1.12+

---

### Type System & Reflection

#### Type System & Reflection
<sub>[API_06_TYPE_SYSTEM.md](API_06_TYPE_SYSTEM.md) · 15 functions</sub>

Runtime type introspection, enumerate types/fields/methods, and traverse class hierarchies.

**Key functions:** `hlffi_find_type()` · `hlffi_list_types()` · `hlffi_class_get_field_count()`

**Use cases:** Dynamic object inspection · Serialization · Debug tools

---

### Values & Members

#### Value System
<sub>[API_07_VALUES.md](API_07_VALUES.md) · 12 functions</sub>

Box and unbox values between C and Haxe types. Memory management and GC safety.

**Key functions:** `hlffi_value_int()` · `hlffi_value_string()` · `hlffi_value_as_int()` · `hlffi_value_free()`

**Critical:** Temporary vs rooted values · UTF-8 ↔ UTF-16 conversion · Ownership rules

---

#### Static Members
<sub>[API_08_STATIC_MEMBERS.md](API_08_STATIC_MEMBERS.md) · 3 functions</sub>

Access class-level static fields and methods by name.

**Key functions:** `hlffi_get_static_field()` · `hlffi_set_static_field()` · `hlffi_call_static()`

**Precondition:** Must call `hlffi_call_entry()` first to initialize globals

---

#### Instance Members
<sub>[API_09_INSTANCE_MEMBERS.md](API_09_INSTANCE_MEMBERS.md) · 18 functions</sub>

Create objects, access fields/methods. Generic API + convenience helpers (70% code reduction).

**Key functions:** `hlffi_new()` · `hlffi_get_field()` · `hlffi_call_method()` · `hlffi_get_field_int()` (convenience)

**Topics:** Object lifecycle · Generic vs typed access · GC root management

---

### Advanced Types

#### Arrays
<sub>[API_10_ARRAYS.md](API_10_ARRAYS.md) · 12 functions</sub>

Dynamic arrays, NativeArrays, and struct arrays with zero-copy access.

**Key functions:** `hlffi_array_new()` · `hlffi_array_get()` · `hlffi_array_new_struct()` · `hlffi_array_get_struct()`

**Performance:** Preallocate for speed · Struct arrays for zero-copy

---

#### Maps
<sub>[API_11_MAPS.md](API_11_MAPS.md) · 9 functions</sub>

Hash tables and dictionaries. Map<K,V> operations.

**Key functions:** `hlffi_map_new()` · `hlffi_map_set()` · `hlffi_map_get()` · `hlffi_map_keys()`

**Types:** IntMap · StringMap · ObjectMap · Dynamic maps

---

#### Bytes
<sub>[API_12_BYTES.md](API_12_BYTES.md) · 11 functions</sub>

Binary data operations and byte buffers with zero-copy access.

**Key functions:** `hlffi_bytes_new()` · `hlffi_bytes_from_data()` · `hlffi_bytes_get_ptr()` · `hlffi_bytes_blit()`

**Use cases:** File I/O · Network protocols · Binary serialization

---

#### Enums
<sub>[API_13_ENUMS.md](API_13_ENUMS.md) · 10 functions</sub>

Algebraic data types with constructors and pattern matching support.

**Key functions:** `hlffi_enum_get_index()` · `hlffi_enum_get_param()` · `hlffi_enum_alloc()` · `hlffi_enum_is_named()`

**Patterns:** Option types · Result types · Pattern matching

---

#### Abstracts
<sub>[API_14_ABSTRACTS.md](API_14_ABSTRACTS.md) · 5 functions</sub>

Abstract type wrappers and underlying type access.

**Key functions:** `hlffi_is_abstract()` · `hlffi_abstract_find()` · `hlffi_abstract_get_name()`

**Note:** Abstracts are transparent at runtime - work with underlying types directly

---

### Interop & Error Handling

#### Callbacks & FFI
<sub>[API_15_CALLBACKS.md](API_15_CALLBACKS.md) · 5 functions</sub>

Register C callbacks callable from Haxe. C ↔ Haxe function interop.

**Key functions:** `hlffi_register_callback()` · `hlffi_get_callback()` · `hlffi_unregister_callback()`

**Recommendation:** Use Dynamic types in Haxe, avoid typed callbacks

---

#### Exceptions
<sub>[API_16_EXCEPTIONS.md](API_16_EXCEPTIONS.md) · 7 functions</sub>

Try-catch support and stack traces for safe error handling.

**Key functions:** `hlffi_try_call_static()` · `hlffi_try_call_method()` · `hlffi_get_exception_message()` · `hlffi_get_exception_stack()`

**Overhead:** ~5% performance cost - use for external I/O and error-prone operations

---

### Performance & Utilities

#### Performance & Caching
<sub>[API_17_PERFORMANCE.md](API_17_PERFORMANCE.md) · 5 functions</sub>

Method caching for 60x speedup on hot paths.

**Key functions:** `hlffi_cache_static_method()` · `hlffi_call_cached()` · `hlffi_cache_free()`

**Benchmark:** 40ns → 0.7ns per call · Cache hot paths only

---

#### Utilities & Helpers
<sub>[API_18_UTILITIES.md](API_18_UTILITIES.md) · 8 functions</sub>

Worker thread helpers, blocking operation guards, C++ RAII wrappers.

**Key functions:** `hlffi_worker_register()` · `hlffi_blocking_begin()` · C++ `WorkerGuard` / `BlockingGuard`

**Use cases:** Multi-threaded access · External I/O · File/network operations

---

#### Error Handling
<sub>[API_19_ERROR_HANDLING.md](API_19_ERROR_HANDLING.md)</sub>

Error codes, error messages, and error handling patterns.

**Key functions:** `hlffi_get_error()` · `hlffi_get_error_string()`

**Patterns:** Check return values · Clean up on error · Log for debugging

---

## Quick Start Guide

New to HLFFI? Follow this learning path:

1. **[VM Lifecycle](API_01_VM_LIFECYCLE.md)** - Learn basic setup and teardown
2. **[Integration Modes](API_02_INTEGRATION_MODES.md)** - Choose your integration pattern
3. **[Value System](API_07_VALUES.md)** - Understand boxing/unboxing
4. **[Static Members](API_08_STATIC_MEMBERS.md)** - Call your first Haxe function
5. **[Instance Members](API_09_INSTANCE_MEMBERS.md)** - Create and use objects

---

## Complete Guides

For in-depth tutorials and comprehensive guides:

- [PHASE3_COMPLETE.md](PHASE3_COMPLETE.md) - Static members guide (1000+ lines)
- [PHASE4_INSTANCE_MEMBERS.md](PHASE4_INSTANCE_MEMBERS.md) - Instance API guide
- [PHASE5_COMPLETE.md](PHASE5_COMPLETE.md) - Maps, Bytes, Enums, Arrays
- [PHASE6_COMPLETE.md](PHASE6_COMPLETE.md) - Exceptions & Callbacks
- [PHASE7_COMPLETE.md](PHASE7_COMPLETE.md) - Performance & Caching
- [HOT_RELOAD.md](HOT_RELOAD.md) - Hot reload comprehensive guide
- [TIMERS_ASYNC_THREADING.md](TIMERS_ASYNC_THREADING.md) - Event loop & threading

---

## Function Summary

| Category | Functions | Focus |
|----------|-----------|-------|
| VM Lifecycle | 10 | Core setup and management |
| Integration & Events | 7 | Modes and event processing |
| Threading | 10 | Multi-threading support |
| Hot Reload | 7 | Runtime updates |
| Type System | 15 | Reflection and introspection |
| Values & Members | 33 | Boxing, fields, methods |
| Advanced Types | 47 | Arrays, Maps, Bytes, Enums |
| Callbacks & Exceptions | 12 | Interop and safety |
| Performance & Utilities | 13 | Optimization and helpers |
| **Total** | **154** | **Complete API** |

---

## API Conventions

**Function Naming**
- `hlffi_*` prefix for all functions
- `hlffi_value_*` for value operations
- `hlffi_*_field` / `hlffi_*_method` for member access
- `hlffi_thread_*` for threading
- Type-specific: `hlffi_enum_*`, `hlffi_map_*`, `hlffi_bytes_*`

**Memory Management**
- Values from `hlffi_value_*()`: Temporary, not GC-rooted
- Values from `hlffi_new()`: GC-rooted, must call `hlffi_value_free()`
- Strings from `*_as_string()`: Caller must `free()` with C free
- VM handle: Caller must call `hlffi_destroy()` (at process exit only)

**Thread Safety**
- Most functions: Not thread-safe (call from main thread)
- Threading functions: Thread-safe (documented per function)
- Worker threads: Must call `hlffi_worker_register()` first

**Error Handling**
- Functions return `hlffi_error_code` or `bool`
- Use `hlffi_get_error(vm)` for detailed error messages
- Use `hlffi_try_call_*()` for exception-safe calls

---

## Platform Support

| Platform | Status | Build System |
|----------|--------|--------------|
| Linux | Fully tested | CMake / Makefile |
| Windows (MSVC) | Fully tested | Visual Studio 2022 |
| macOS | Should work (not tested) | CMake / Makefile |
| Mobile (Android/iOS) | Requires HL/C mode | TBD |
| WebAssembly | Requires investigation | TBD |

---

## Additional Resources

- **[API_INDEX.md](API_INDEX.md)** - Quick function reference (130 functions at a glance)
- **Issues:** Report bugs at project repository
- **Examples:** See `examples/` directory for working code
- **Tests:** See `test/` directory for comprehensive test coverage

---

<sub>**HLFFI v3.0.0** · MIT License · Requires HashLink 1.11+ (1.12+ for hot reload)</sub>
