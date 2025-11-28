# HLFFI v3.0 - API Reference Index

**Last Updated:** November 28, 2025
**Version:** 3.0.0

This is the main API reference for HLFFI. Each section is documented in detail in its own file.

---

## Table of Contents

### Core VM Management
1. **[VM Lifecycle](API_01_VM_LIFECYCLE.md)** - Create, initialize, load, run, destroy
   - `hlffi_create()`, `hlffi_init()`, `hlffi_load_file()`, `hlffi_call_entry()`, `hlffi_destroy()`
   - Error handling, GC stack management
   - ~10 functions

2. **[Integration Modes](API_02_INTEGRATION_MODES.md)** - NON_THREADED vs THREADED
   - `hlffi_set_integration_mode()`, `hlffi_get_integration_mode()`
   - Choosing between engine-controlled and VM-thread modes
   - ~2 functions

3. **[Event Loop Integration](API_03_EVENT_LOOP.md)** - Timers and async operations
   - `hlffi_update()`, `hlffi_process_events()`, `hlffi_has_pending_work()`
   - UV loop and Haxe EventLoop support
   - haxe.Timer, MainLoop.add() integration
   - ~5 functions

4. **[Threading](API_04_THREADING.md)** - Dedicated VM thread and worker threads
   - `hlffi_thread_start()`, `hlffi_thread_call_sync()`, `hlffi_thread_call_async()`
   - Worker thread registration, blocking operations
   - Message queue architecture
   - ~10 functions

5. **[Hot Reload](API_05_HOT_RELOAD.md)** - Runtime code updates
   - `hlffi_enable_hot_reload()`, `hlffi_reload_module()`, `hlffi_check_reload()`
   - Callback notifications, automatic file watching
   - ~7 functions

### Type System & Reflection
6. **[Type System & Reflection](API_06_TYPE_SYSTEM.md)** - Introspection
   - `hlffi_find_type()`, `hlffi_list_types()`, `hlffi_class_get_field_count()`
   - Enumerating types, fields, methods
   - Traversing class hierarchies
   - ~15 functions

### Values & Members
7. **[Value System](API_07_VALUES.md)** - Boxing and unboxing
   - `hlffi_value_int()`, `hlffi_value_string()`, `hlffi_value_as_int()`
   - Converting between C and Haxe types
   - Memory management, GC safety
   - ~12 functions

8. **[Static Members](API_08_STATIC_MEMBERS.md)** - Class-level access
   - `hlffi_get_static_field()`, `hlffi_set_static_field()`, `hlffi_call_static()`
   - Accessing static fields and methods
   - ~3 functions

9. **[Instance Members](API_09_INSTANCE_MEMBERS.md)** - Object manipulation
   - `hlffi_new()`, `hlffi_get_field()`, `hlffi_set_field()`, `hlffi_call_method()`
   - Convenience API (70% code reduction)
   - Object lifecycle management
   - ~15 functions

### Advanced Types
10. **[Arrays](API_10_ARRAYS.md)** - Array operations
    - Standard arrays, NativeArrays, struct arrays
    - Direct memory access, zero-copy operations
    - ~12 functions

11. **[Maps](API_11_MAPS.md)** - Hash tables/dictionaries
    - `hlffi_map_new()`, `hlffi_map_get()`, `hlffi_map_set()`, `hlffi_map_keys()`
    - IntMap, StringMap, ObjectMap support
    - ~8 functions

12. **[Bytes](API_12_BYTES.md)** - Binary data
    - `hlffi_bytes_new()`, `hlffi_bytes_from_data()`, `hlffi_bytes_get_ptr()`
    - Zero-copy access, blit operations
    - ~10 functions

13. **[Enums](API_13_ENUMS.md)** - Algebraic data types
    - `hlffi_enum_get_index()`, `hlffi_enum_get_param()`, `hlffi_enum_alloc()`
    - Constructor introspection, pattern matching support
    - ~10 functions

14. **[Abstracts](API_14_ABSTRACTS.md)** - Abstract types
    - `hlffi_is_abstract()`, `hlffi_abstract_find()`, `hlffi_value_is_abstract()`
    - Abstract type detection and handling
    - ~5 functions

### Interop & Error Handling
15. **[Callbacks & FFI](API_15_CALLBACKS.md)** - C ↔ Haxe callbacks
    - `hlffi_register_callback()`, `hlffi_get_callback()`, `hlffi_unregister_callback()`
    - Calling C functions from Haxe
    - Typed vs dynamic callbacks
    - ~5 functions

16. **[Exception Handling](API_16_EXCEPTIONS.md)** - Try/catch support
    - `hlffi_try_call_static()`, `hlffi_get_exception_message()`, `hlffi_get_exception_stack()`
    - Safe error handling, stack traces
    - ~7 functions

### Performance & Utilities
17. **[Performance & Caching](API_17_PERFORMANCE.md)** - Optimization
    - `hlffi_cache_static_method()`, `hlffi_call_cached()`
    - 60x speedup for hot paths
    - Benchmark results
    - ~5 functions

18. **[Utilities & Helpers](API_18_UTILITIES.md)** - Version info, GC helpers
    - `hlffi_get_version()`, `hlffi_get_hl_version()`, `hlffi_is_jit_mode()`
    - Blocking operation guards
    - C++ RAII wrappers
    - ~8 functions

19. **[Error Handling](API_19_ERROR_HANDLING.md)** - Error codes and messages
    - `hlffi_error_code` enum (all error codes)
    - `hlffi_get_error()`, `hlffi_get_error_string()`
    - Error handling patterns

---

## Quick Start

New to HLFFI? Start with these sections in order:

1. **[VM Lifecycle](API_01_VM_LIFECYCLE.md)** - Learn the basic setup
2. **[Integration Modes](API_02_INTEGRATION_MODES.md)** - Choose your integration pattern
3. **[Values](API_07_VALUES.md)** - Understand boxing/unboxing
4. **[Static Members](API_08_STATIC_MEMBERS.md)** - Call your first Haxe function
5. **[Instance Members](API_09_INSTANCE_MEMBERS.md)** - Create and use objects

## Complete Guides

For in-depth tutorials and walkthroughs, see:
- `docs/PHASE3_COMPLETE.md` - Static members guide (1000+ lines)
- `docs/PHASE4_INSTANCE_MEMBERS.md` - Instance API guide
- `docs/PHASE5_COMPLETE.md` - Maps, Bytes, Enums, Arrays
- `docs/PHASE6_COMPLETE.md` - Exceptions & Callbacks
- `docs/PHASE7_COMPLETE.md` - Performance & Caching
- `docs/HOT_RELOAD.md` - Hot reload guide
- `docs/TIMERS_ASYNC_THREADING.md` - Event loop & threading guide

## Function Count Summary

| Category | Functions |
|----------|-----------|
| VM Lifecycle | ~10 |
| Integration & Events | ~7 |
| Threading | ~10 |
| Hot Reload | ~7 |
| Type System | ~15 |
| Values & Members | ~30 |
| Advanced Types (Arrays, Maps, Bytes, Enums) | ~40 |
| Callbacks & Exceptions | ~12 |
| Performance & Utilities | ~13 |
| **Total** | **~150+** |

---

## API Conventions

### Function Naming
- `hlffi_*` prefix for all functions
- `hlffi_value_*` for value operations
- `hlffi_*_field` for field access
- `hlffi_call_*` for method calls
- `hlffi_thread_*` for threading
- `hlffi_enum_*` / `hlffi_map_*` / `hlffi_bytes_*` for type-specific ops

### Memory Management
- **Values from `hlffi_value_*()`**: Not GC-rooted, temporary use only
- **Values from `hlffi_new()`**: GC-rooted, must call `hlffi_value_free()`
- **Strings from `*_as_string()`**: Caller must `free()` with C free
- **VM handle**: Caller must call `hlffi_destroy()` (at process exit only)

### Thread Safety
- Most functions: ❌ Not thread-safe (call from main thread)
- Threading functions: ✅ Thread-safe (documented per function)
- Worker threads: Must call `hlffi_worker_register()` first

### Error Handling
- Functions return `hlffi_error_code` or `bool`
- Use `hlffi_get_error(vm)` for detailed error messages
- Use `hlffi_try_call_*()` for exception-safe calls

---

## Platform Support

| Platform | Status | Build System |
|----------|--------|--------------|
| Linux | ✅ Fully tested | Makefile |
| Windows (MSVC) | ✅ Fully tested | Visual Studio 2022 |
| macOS | ⚠️ Should work (not tested) | Makefile (BSD/Unix) |
| Mobile (Android/iOS) | ❌ Requires HL/C mode | TBD |
| WebAssembly | ❌ Requires investigation | TBD |

---

## Version Information

**HLFFI Version:** 3.0.0
**HashLink Version Required:** 1.12+ (for hot reload), 1.11+ (core features)
**License:** MIT (same as HashLink)

---

## Support & Resources

- **Issues**: Report bugs at project repository
- **Examples**: See `examples/` directory for working code
- **Tests**: See `test/` directory for comprehensive test coverage
- **Guides**: See `docs/PHASE*_COMPLETE.md` for detailed walkthroughs

---

**Next:** Start with [VM Lifecycle](API_01_VM_LIFECYCLE.md) to learn the basics.
