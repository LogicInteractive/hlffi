# HLFFI v3.0 — Master Implementation Plan

**Clean redesign of HashLink FFI library**
**Target Platform**: Windows (primary), cross-platform later
**Architecture**: Single-header C/C++ library with phased feature rollout

---

## Design Principles

1. **Test-Driven**: Each feature ships with working example + test
2. **Incremental**: Each phase is usable on its own
3. **Type-Safe**: Leverage C++ when beneficial, keep C compatibility
4. **Zero-Overhead**: Fast paths inline to raw HL calls
5. **Clear Errors**: Every failure returns actionable error message
6. **Single VM**: Optimize for one VM per process (multi-VM = future)
7. **Memory-Safe**: No leaks, clear ownership rules

---

## Improvements Over Existing FFI Libraries

HLFFI v3.0 builds on lessons learned from existing HashLink embedding projects:

### Compared to hashlink-embed (Ruby FFI Library)

| Feature | hashlink-embed (Ruby) | HLFFI v3.0 (C/C++) | Advantage |
|---------|----------------------|-------------------|-----------|
| **Memory Management** | Manual `dispose()` required | Automatic GC root tracking | ✅ No manual cleanup, safer |
| **Object Direction** | One-way (Haxe → Ruby only) | Bidirectional (C ↔ Haxe) | ✅ True FFI, C structs to Haxe |
| **Performance** | Ruby interpreter overhead | Direct C API calls | ✅ Zero overhead fast path |
| **Type Conversions** | Partial (no f32, no writes) | Complete (all types, read/write) | ✅ Full type coverage |
| **Error Handling** | Exceptions (Ruby style) | Return codes + messages | ✅ Works in C and C++ |
| **Platform Support** | Requires Ruby runtime | Standalone C/C++ | ✅ Universal compatibility |
| **API Style** | Method chaining proxies | Both C (simple) & C++ (elegant) | ✅ Best of both worlds |

**Key Innovations**:
1. **Automatic GC Roots**: We add/remove roots automatically - users never call `dispose()`
2. **Bidirectional Objects**: Pass C structs to Haxe, not just primitives
3. **Complete Type System**: Full support for arrays, maps, enums, bytes (read AND write)
4. **Zero Overhead**: Fast path inlines to raw HL calls (~0-2ns)
5. **Both C and C++**: C for compatibility, C++ for elegance (like Ruby's method chaining)

**Example Comparison**:

```ruby
# hashlink-embed (Ruby) - manual disposal required
vm = HashLink::VM.new("game.hl")
player = vm.game.Player.new("Hero")
player.takeDamage(25)
player.dispose()  # ⚠️ User must remember this!
vm.dispose()      # ⚠️ Easy to forget = memory leak
```

```cpp
// HLFFI v3.0 (C++) - automatic cleanup
hlffi::VM vm("game.hl");
auto player = vm.call_static<hlffi::Object>("Player", "new", "Hero");
player.call_method("takeDamage", 25);
// ✅ Automatic GC root management - no disposal needed!
```

### Compared to Direct HL API Usage

| Aspect | Direct hl.h API | HLFFI v3.0 | Advantage |
|--------|----------------|-----------|-----------|
| **Complexity** | ~100+ functions to learn | ~30 core functions | ✅ Easier learning curve |
| **Type Safety** | Manual type coercion | Automatic conversion | ✅ Fewer runtime errors |
| **Constructor Call** | Issue #253 workaround needed | Helper function handles it | ✅ Just works |
| **GC Roots** | Manual add/remove | Automatic tracking | ✅ No memory leaks |
| **Error Handling** | Raw HL errors | Typed error codes + messages | ✅ Actionable diagnostics |
| **Documentation** | Scattered wiki pages | Comprehensive manual | ✅ One-stop reference |

---

## Design Decisions ✅ CONFIRMED

These architectural choices guide the entire implementation:

### 1. Library Distribution: **Both Header-Only + Compiled**
- **Header-only mode**: `#define HLFFI_IMPLEMENTATION` (single-file distribution)
- **Compiled library mode**: Separate hlffi.lib for faster user builds
- **Rationale**: Best of both worlds - quick prototyping + production builds

### 2. API Language: **C Core + C++ Wrappers**
- **C API**: `hlffi_*` functions (universal compatibility)
- **C++ API**: `hl::*` namespace with templates, RAII, type safety
- **Both coexist**: User chooses based on project needs
- **Rationale**: Maximum compatibility + modern convenience

```c
// C API - works everywhere
hlffi_error_code err = hlffi_call_static(vm, "Game", "start", 0, NULL);
if (err != HLFFI_OK) {
    printf("Error: %s\n", hlffi_get_error(vm));
}
```

```cpp
// C++ API - type-safe, RAII (optional)
#ifdef __cplusplus
namespace hl {
    auto result = call_static<int>(vm, "Game", "getScore");
    // No manual memory management needed
}
#endif
```

### 3. Error Handling: **Return Codes**
- **All C functions return**: `hlffi_error_code` enum
- **Error details**: Retrieved via `hlffi_get_error(vm)`
- **Thread-safe**: Each VM has its own error buffer
- **No exceptions**: Simpler, predictable, zero overhead
- **Rationale**: Works in C and C++, no ABI issues, explicit control flow

```c
typedef enum {
    HLFFI_OK = 0,
    HLFFI_ERR_NULL_VM,
    HLFFI_ERR_NOT_INIT,
    HLFFI_ERR_FILE_NOT_FOUND,
    HLFFI_ERR_INVALID_BYTECODE,
    HLFFI_ERR_TYPE_NOT_FOUND,
    HLFFI_ERR_METHOD_NOT_FOUND,
    HLFFI_ERR_FIELD_NOT_FOUND,
    HLFFI_ERR_INVALID_ARGS,
    HLFFI_ERR_EXCEPTION,
    // ... more specific codes
} hlffi_error_code;
```

### 4. Value Lifetime: **GC-Integrated**
- **No manual free**: HashLink GC manages all `hlffi_value*` lifetimes
- **Automatic roots**: Library maintains GC roots for live values
- **Scope-based**: Values live as long as they're reachable from C
- **No ref counting**: Zero user burden
- **Rationale**: Safest, matches Haxe semantics, prevents leaks

```c
// User code - no memory management!
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
hlffi_value* health = hlffi_get_field(player, "health");
// Values automatically GC'd when no longer referenced
// No hlffi_value_free() needed!
```

**GC Root Management (Internal)**
- Library automatically adds/removes GC roots
- Values returned to C are pinned until scope exit (C++) or manual release (C)
- Optional `hlffi_value_release()` for advanced C users

### 5. Naming Convention: **hlffi_** prefix + hl:: namespace**
- **C functions**: `hlffi_*` (e.g., `hlffi_call_static`)
- **C++ namespace**: `hl::*` (e.g., `hl::call_static<T>`)
- **Types**: `hlffi_type`, `hlffi_value`, `hlffi_vm`
- **Enums**: `HLFFI_*` (e.g., `HLFFI_OK`, `HLFFI_TYPE_I32`)
- **Rationale**: Clear prefix prevents collisions, C++ namespace is clean

### 6. Runtime Mode: **Both HL/JIT + HL/C**
- **HL/JIT**: Bytecode + JIT compilation (default, supports hot reload)
- **HL/C**: Compiled to native C (better performance, ARM/mobile/WASM)
- **Both supported**: API works seamlessly with either mode
- **Runtime detection**: Library detects which mode is active
- **Rationale**: JIT for development (hot reload), C for production (performance)

```c
// Same code works with both modes!
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);

// Hot reload only available in HL/JIT mode
if (hlffi_is_hot_reload_available(vm)) {
    hlffi_enable_hot_reload(vm, true);
}
```

**Mode Comparison:**

| Feature | HL/JIT | HL/C |
|---------|--------|------|
| Hot Reload | ✅ Yes | ❌ No |
| Performance | Good | Excellent |
| Platforms | x64 desktop | All (ARM, WASM, etc) |
| Startup Time | Fast | Faster |
| Binary Size | Smaller | Larger |
| Use Case | Development | Production/Mobile |

---

## Phase 0: Foundation & Architecture
**Goal**: Build system, core types, test harness
**Duration**: ~4 hours
**Deliverable**: Compiles on Windows, runs "hello world"

### Features
- [x] Clean repository structure
- [ ] CMake build system (MSVC primary)
- [ ] Core `hlffi_vm` struct design
- [ ] Error handling system (enum + messages)
- [ ] Basic type definitions
- [ ] Test harness (simple C++ test runner)
- [ ] Example: minimal init/shutdown

### Files Created
```
hlffi/
├── include/
│   ├── hlffi.h           // Main C API header (can be header-only)
│   └── hlffi.hpp         // C++ wrapper (optional, Phase 3+)
├── src/
│   └── hlffi.c           // Implementation (for compiled lib mode)
├── tests/
│   ├── test_main.cpp     // Test runner
│   ├── test_phase0.cpp   // Foundation tests
│   └── test_data/
│       └── simple.hl     // Test bytecode
├── examples/
│   ├── 00_hello_c.c      // Minimal C example
│   ├── 00_hello_cpp.cpp  // Minimal C++ example
│   └── haxe/
│       └── Simple.hx     // Test Haxe code
├── CMakeLists.txt        // Build system
├── MASTER_PLAN.md        // This file
└── README.md             // User documentation
```

**Build System Features:**
- Option to build as header-only or compiled library
- Automatic HashLink detection
- Test target with Haxe compilation
- Example targets

### Success Criteria
- ✓ Builds with MSVC on Windows
- ✓ Example runs without crashes
- ✓ Test harness can run basic assertions

---

## Phase 1: VM Lifecycle + Hot Reload 🔥
**Goal**: Single-instance VM lifecycle with hot reload for code changes
**Duration**: ~8 hours (extended for hot reload)
**Deliverable**: Can load and execute HL bytecode, hot reload code changes without restart

### Features

**Core Lifecycle:**
- [ ] `hlffi_create()` - allocate VM
- [ ] `hlffi_init()` - initialize HashLink runtime + args
- [ ] `hlffi_load_file()` - load .hl from disk
- [ ] `hlffi_load_memory()` - load .hl from buffer
- [ ] `hlffi_call_entry()` - invoke Main.main()
- [ ] `hlffi_destroy()` - free VM (ONLY safe at process exit)

**⚠️ VM Restart NOT Supported:**
- HashLink VM can only be initialized ONCE per process
- Calling `hlffi_destroy()` then `hlffi_create()` again will crash
- For code changes during development, use **Hot Reload** instead
- For complete isolation, restart the entire process (fork/exec)

**Hot Reload (NEW! HL 1.12+):**
- [ ] `hlffi_enable_hot_reload()` - enable hot reload mode
- [ ] `hlffi_reload_module()` - reload changed bytecode at runtime
- [ ] `hlffi_set_reload_callback()` - callback on successful reload
- [ ] Detect and reload changed functions without restart
- [ ] Preserve runtime state during reload
- [ ] Handle reload failures gracefully

### API Design
```c
typedef enum {
    HLFFI_OK = 0,
    HLFFI_ERR_NULL_VM,
    HLFFI_ERR_ALREADY_INIT,
    HLFFI_ERR_NOT_INIT,
    HLFFI_ERR_FILE_NOT_FOUND,
    HLFFI_ERR_INVALID_BYTECODE,
    HLFFI_ERR_MODULE_INIT_FAILED,
    HLFFI_ERR_RELOAD_FAILED,
    HLFFI_ERR_RELOAD_NOT_SUPPORTED,
    // ... more
} hlffi_error_code;

// Core lifecycle (single-instance only)
hlffi_vm* hlffi_create(void);
hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv);
hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path);
hlffi_error_code hlffi_load_memory(hlffi_vm* vm, const void* data, size_t size);
hlffi_error_code hlffi_call_entry(hlffi_vm* vm);
void hlffi_destroy(hlffi_vm* vm);  // Only call at process exit
const char* hlffi_get_error(hlffi_vm* vm);

// Hot reload (HL 1.12+)
typedef void (*hlffi_reload_callback)(hlffi_vm* vm, bool success, void* userdata);

hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable);
bool hlffi_is_hot_reload_enabled(hlffi_vm* vm);
hlffi_error_code hlffi_reload_module(hlffi_vm* vm, const char* path);
hlffi_error_code hlffi_reload_module_memory(hlffi_vm* vm, const void* data, size_t size);
void hlffi_set_reload_callback(hlffi_vm* vm, hlffi_reload_callback cb, void* userdata);
```

### ⚠️ Critical Implementation Notes

**Stack Top Persistence (Issue #752)**:
```c
// CRITICAL: Stack top storage must remain valid during VM lifetime!
typedef struct {
    hl_code *code;
    hl_module *module;
    void *stack_top;  // ⭐ Persistent storage for thread registration
    bool initialized;
    char error_buffer[512];
} hlffi_vm;

hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv) {
    // Use VM-owned storage (not local variable!)
    hl_register_thread(&vm->stack_top);
    // ...
}
```

**Why**: Plugin/DLL scenarios fail if stack variable goes out of scope. GC scans stack using provided pointer, so it must outlive the thread registration.

**Entry Point is Mandatory (Gotcha #1)**:
```c
hlffi_error_code hlffi_call_entry(hlffi_vm* vm) {
    // This MUST be called even if entry point is empty!
    // Sets up global state and static initializers
    hl_call0(void, vm->entry_closure);
}
```

**Why**: Global values (`obj->global_value`) are NULL until entry point runs. Static class members won't work without this.

**VM Restart NOT Supported** (See: VM_RESTART_INVESTIGATION.md):
```c
// ❌ WRONG - This will crash!
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);
hlffi_destroy(vm);

// ❌ Second init will crash due to:
// 1. Non-idempotent hl_global_init() (leaks mutexes)
// 2. Incomplete hl_global_free() (doesn't reset static state)
// 3. Stale module pointers in cur_modules array
vm = hlffi_create();  // CRASH or undefined behavior
hlffi_init(vm, argc, argv);

// ✅ CORRECT - Use hot reload instead
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
hlffi_enable_hot_reload(vm, true);
hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);

// Later: reload code without restart
hlffi_reload_module(vm, "game.hl");  // ✅ Safe, preserves state
```

**Why**: HashLink core has fundamental design limitations:
- `hl_gc_init()` allocates mutexes without guarding against double-init
- `hl_gc_free()` doesn't free mutexes or reset page maps
- Module system static state (`cur_modules`) never cleared
- Result: memory leaks, stale pointers, crashes

**Alternatives**:
- **Hot Reload**: Change code without restart (development)
- **Process Restart**: Fork/exec for complete isolation (production)
- **Plugin System**: Runtime module loading (Phase 9)

### Test Cases

**Core Lifecycle:**
- Load valid bytecode
- Load invalid bytecode (should fail gracefully)
- Call entry multiple times (should fail)
- Destroy without init (should fail safely)
- ⚠️ **No restart test** - restart is not supported

**Hot Reload:**
- Enable hot reload before loading bytecode
- Modify and reload a simple function
- Reload with syntax errors (should fail gracefully)
- Reload with class structure changes (should fail with clear error)
- Reload callback receives success/failure notifications
- State preservation across reload

### Example

**Basic Lifecycle:**
```cpp
// 01_lifecycle.cpp - Single VM instance
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);

// VM runs for lifetime of process
// ... application runs ...

// Only destroy at process exit
hlffi_destroy(vm);
```

**Hot Reload:**
```cpp
// 01_hot_reload.cpp
void on_reload(hlffi_vm* vm, bool success, void* userdata) {
    printf("Reload %s\n", success ? "succeeded" : "failed");
    if (!success) {
        printf("Error: %s\n", hlffi_get_error(vm));
    }
}

hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);

// Enable hot reload
hlffi_enable_hot_reload(vm, true);
hlffi_set_reload_callback(vm, on_reload, NULL);

hlffi_load_file(vm, "game.hl");
hlffi_call_entry(vm);

// Game loop
while (running) {
    // Check if bytecode changed (e.g., file watcher)
    if (bytecode_changed) {
        // Reload without stopping the game!
        hlffi_reload_module(vm, "game.hl");
    }

    // Continue running with updated code
    hlffi_call_static(vm, "Game", "update", 0, NULL);
}

hlffi_destroy(vm);
```

### Success Criteria
- ✓ Can load and run .hl bytecode
- ✓ Single VM instance works reliably
- ✓ Can hot reload changed functions without restart
- ✓ Hot reload preserves runtime state
- ✓ Hot reload failures handled gracefully
- ✓ All error paths tested and safe
- ✓ VM restart limitation clearly documented

### Hot Reload Limitations (per HL docs)
- ⚠️ Cannot add/remove/reorder class fields
- ⚠️ Experimental feature - may have bugs
- ⚠️ Requires HL 1.12 or later
- ✓ Works for function body changes
- ✓ Works for new functions
- ✓ JIT mode only (not HL/C)

---

## Phase 2: Type System & Reflection
**Goal**: Find and inspect types, classes, enums
**Duration**: ~8 hours
**Deliverable**: Can query all types in loaded module

### Features
- [ ] `hlffi_find_type(name)` - get type by full name
- [ ] `hlffi_type_get_kind()` - class/enum/abstract/typedef
- [ ] `hlffi_type_get_name()` - get type name
- [ ] `hlffi_list_types()` - enumerate all types
- [ ] `hlffi_class_get_super()` - get parent class
- [ ] `hlffi_class_list_fields()` - list all fields
- [ ] `hlffi_class_list_methods()` - list all methods
- [ ] Handle package names: "com.example.MyClass"

### API Design
```c
typedef struct hlffi_type hlffi_type;

typedef enum {
    HLFFI_TYPE_VOID,
    HLFFI_TYPE_I32,
    HLFFI_TYPE_F64,
    HLFFI_TYPE_BOOL,
    HLFFI_TYPE_BYTES,
    HLFFI_TYPE_DYN,
    HLFFI_TYPE_OBJ,    // class/instance
    HLFFI_TYPE_ARRAY,
    HLFFI_TYPE_ENUM,
    // ... more
} hlffi_type_kind;

hlffi_type* hlffi_find_type(hlffi_vm* vm, const char* name);
hlffi_type_kind hlffi_type_get_kind(hlffi_type* type);
const char* hlffi_type_get_name(hlffi_type* type);

// Iteration
typedef void (*hlffi_type_callback)(hlffi_type* type, void* userdata);
void hlffi_list_types(hlffi_vm* vm, hlffi_type_callback cb, void* userdata);

// Class inspection
hlffi_type* hlffi_class_get_super(hlffi_type* cls);
int hlffi_class_get_field_count(hlffi_type* cls);
const char* hlffi_class_get_field_name(hlffi_type* cls, int index);
hlffi_type* hlffi_class_get_field_type(hlffi_type* cls, int index);
```

### Test Cases
- Find simple class "MyClass"
- Find packaged class "com.example.Player"
- List all types in module
- Get superclass chain
- List fields and methods

### Example
```cpp
// 02_reflection.cpp
hlffi_type* player = hlffi_find_type(vm, "Player");
printf("Type: %s\n", hlffi_type_get_name(player));

hlffi_type* super = hlffi_class_get_super(player);
printf("Extends: %s\n", hlffi_type_get_name(super));

int field_count = hlffi_class_get_field_count(player);
for(int i = 0; i < field_count; i++) {
    printf("Field: %s\n", hlffi_class_get_field_name(player, i));
}
```

### Success Criteria
- ✓ Can find any type by name
- ✓ Can enumerate all types in module
- ✓ Reflection data matches Haxe source

---

## Phase 3: Static Members (Methods & Fields)
**Goal**: Call static methods, get/set static fields
**Duration**: ~6 hours
**Deliverable**: Full static member access

### Features
- [ ] `hlffi_call_static()` - call static method (variadic args)
- [ ] `hlffi_get_static_field()` - get static field value
- [ ] `hlffi_set_static_field()` - set static field value
- [ ] Type-safe wrappers (int, float, bool, string, object)
- [ ] Automatic type conversion
- [ ] Handle null returns safely

### API Design
```c
// Generic static call
hlffi_value* hlffi_call_static(
    hlffi_vm* vm,
    const char* class_name,
    const char* method_name,
    int argc,
    hlffi_value** args
);

// Type-safe helpers (C++)
int hlffi_call_static_int(hlffi_vm* vm, const char* cls, const char* method);
double hlffi_call_static_float(hlffi_vm* vm, const char* cls, const char* method);
const char* hlffi_call_static_string(hlffi_vm* vm, const char* cls, const char* method);

// Static fields
hlffi_value* hlffi_get_static_field(hlffi_vm* vm, const char* cls, const char* field);
void hlffi_set_static_field(hlffi_vm* vm, const char* cls, const char* field, hlffi_value* val);
```

### Test Haxe Code
```haxe
class TestStatic {
    public static var counter:Int = 0;
    public static var name:String = "test";

    public static function increment():Void {
        counter++;
    }

    public static function add(a:Int, b:Int):Int {
        return a + b;
    }

    public static function greet(name:String):String {
        return "Hello, " + name;
    }
}
```

### Example
```cpp
// 03_static.cpp
// Call static method
hlffi_call_static(vm, "TestStatic", "increment", 0, NULL);

// Get static field
hlffi_value* counter = hlffi_get_static_field(vm, "TestStatic", "counter");
printf("Counter: %d\n", hlffi_value_as_int(counter));

// Set static field
hlffi_value* new_name = hlffi_value_string("Alice");
hlffi_set_static_field(vm, "TestStatic", "name", new_name);

// Call with args
hlffi_value* args[2] = {hlffi_value_int(5), hlffi_value_int(3)};
hlffi_value* result = hlffi_call_static(vm, "TestStatic", "add", 2, args);
printf("5 + 3 = %d\n", hlffi_value_as_int(result));
```

### Success Criteria
- ✓ Can call all static methods (0-4+ args)
- ✓ Can get/set all static field types
- ✓ Type conversions work correctly
- ✓ No memory leaks

---

## Phase 4: Instance Members (Objects)
**Goal**: Create objects, call methods, access fields
**Duration**: ~8 hours
**Deliverable**: Full instance member access

### Features
- [ ] `hlffi_new()` - create class instance (call constructor)
- [ ] `hlffi_call_method()` - call instance method
- [ ] `hlffi_get_field()` - get instance field
- [ ] `hlffi_set_field()` - set instance field
- [ ] Object lifetime management (GC roots)
- [ ] Type checking (instanceof)

### API Design
```c
// Object creation
hlffi_value* hlffi_new(
    hlffi_vm* vm,
    const char* class_name,
    int argc,
    hlffi_value** args
);

// Instance methods
hlffi_value* hlffi_call_method(
    hlffi_value* obj,
    const char* method_name,
    int argc,
    hlffi_value** args
);

// Instance fields
hlffi_value* hlffi_get_field(hlffi_value* obj, const char* field_name);
void hlffi_set_field(hlffi_value* obj, const char* field_name, hlffi_value* value);

// Type checking
bool hlffi_is_instance_of(hlffi_value* obj, const char* class_name);
```

### ⚠️ Critical Implementation Notes

**Constructor Type Mismatch (Issue #253)**:
```c
// CRITICAL: Direct constructor call fails due to type system quirk!
hlffi_value* hlffi_new(hlffi_vm* vm, const char* class_name, int argc, hlffi_value** args) {
    // 1. Find class type
    hl_type *class_type = find_type_by_name(vm, class_name);

    // 2. Get global type (constructor expects this, not class type!)
    vdynamic *global = *(vdynamic**)class_type->obj->global_value;

    // 3. Allocate instance with class type (correct for instance)
    vobj *instance = hl_alloc_obj(class_type);

    // 4. Find constructor
    int ctor_hash = hl_hash_utf8("new");
    vclosure *ctor = find_method(class_type, ctor_hash);

    // 5. Call constructor (workaround: pass both types)
    vdynamic *args_with_this[argc + 1];
    args_with_this[0] = (vdynamic*)instance;
    // ... copy args ...
    hl_dyn_call(ctor, args_with_this, argc + 1);

    return wrap_value(instance);  // Wrap with GC root
}
```

**Why**: Constructor signature expects global type, but instance must use class type. Must workaround type mismatch.

**Automatic GC Root Management**:
```c
// IMPROVEMENT OVER RUBY LIBRARY: Automatic disposal!
typedef struct {
    vdynamic *hl_value;
    bool is_rooted;
} hlffi_value;

hlffi_value* wrap_value(vdynamic *v) {
    hlffi_value *wrapped = malloc(sizeof(hlffi_value));
    wrapped->hl_value = v;
    wrapped->is_rooted = true;
    hl_add_root(&wrapped->hl_value);  // ⭐ Automatic GC root
    return wrapped;
}

void hlffi_value_release(hlffi_value *v) {
    if (v && v->is_rooted) {
        hl_remove_root(&v->hl_value);  // ⭐ Automatic cleanup
        v->is_rooted = false;
    }
    free(v);
}
```

**Why**: hashlink-embed Ruby library requires manual `dispose()` - users forget and leak memory. We do it automatically!

**Global Values Initialization Order**:
```c
// CRITICAL: Entry point must be called before accessing objects!
hlffi_value* hlffi_new(hlffi_vm* vm, const char* class_name, ...) {
    if (!vm->entry_called) {
        // Set error: "Entry point must be called first"
        return NULL;
    }
    // ... proceed with construction
}
```

**Why**: `obj->global_value` is NULL before entry point runs (Gotcha #2).

### Test Haxe Code
```haxe
class Player {
    public var name:String;
    public var health:Int;

    public function new(name:String) {
        this.name = name;
        this.health = 100;
    }

    public function takeDamage(amount:Int):Void {
        health -= amount;
    }

    public function isAlive():Bool {
        return health > 0;
    }
}
```

### Example
```cpp
// 04_instances.cpp
// Create player
hlffi_value* name = hlffi_value_string("Hero");
hlffi_value* player = hlffi_new(vm, "Player", 1, &name);

// Get field
hlffi_value* health = hlffi_get_field(player, "health");
printf("Health: %d\n", hlffi_value_as_int(health));

// Call method
hlffi_value* damage = hlffi_value_int(25);
hlffi_call_method(player, "takeDamage", 1, &damage);

// Check result
hlffi_value* alive = hlffi_call_method(player, "isAlive", 0, NULL);
printf("Alive: %s\n", hlffi_value_as_bool(alive) ? "yes" : "no");
```

### Success Criteria
- ✓ Can create instances with constructors
- ✓ Can call instance methods
- ✓ Can get/set instance fields
- ✓ Objects survive GC correctly

---

## Phase 5: Advanced Value Types
**Goal**: Arrays, maps, enums, bytes
**Duration**: ~10 hours
**Deliverable**: Can work with all Haxe value types

### Features
- [ ] Arrays: create, get/set elements, length, push/pop
- [ ] Maps: create, get/set, exists, keys
- [ ] Enums: construct, match, extract values
- [ ] Bytes: create, read/write, length
- [ ] Null handling (proper null values)
- [ ] Type conversion safety

### API Design
```c
// Arrays
hlffi_value* hlffi_array_new(hlffi_vm* vm, hlffi_type* element_type, int length);
int hlffi_array_length(hlffi_value* arr);
hlffi_value* hlffi_array_get(hlffi_value* arr, int index);
void hlffi_array_set(hlffi_value* arr, int index, hlffi_value* val);
void hlffi_array_push(hlffi_value* arr, hlffi_value* val);

// Maps
hlffi_value* hlffi_map_new(hlffi_vm* vm);
hlffi_value* hlffi_map_get(hlffi_value* map, hlffi_value* key);
void hlffi_map_set(hlffi_value* map, hlffi_value* key, hlffi_value* val);
bool hlffi_map_exists(hlffi_value* map, hlffi_value* key);

// Enums
hlffi_value* hlffi_enum_construct(
    hlffi_vm* vm,
    const char* enum_name,
    const char* constructor_name,
    int argc,
    hlffi_value** args
);
int hlffi_enum_index(hlffi_value* enum_val);
const char* hlffi_enum_name(hlffi_value* enum_val);
hlffi_value* hlffi_enum_get_param(hlffi_value* enum_val, int index);

// Bytes
hlffi_value* hlffi_bytes_new(hlffi_vm* vm, int size);
int hlffi_bytes_length(hlffi_value* bytes);
void hlffi_bytes_read(hlffi_value* bytes, int pos, void* dst, int len);
void hlffi_bytes_write(hlffi_value* bytes, int pos, const void* src, int len);
```

### Test Haxe Code
```haxe
enum Action {
    Move(x:Int, y:Int);
    Attack(target:String);
    Idle;
}

class Game {
    public static function testArray():Array<Int> {
        return [1, 2, 3, 4, 5];
    }

    public static function testMap():Map<String, Int> {
        var m = new Map();
        m.set("score", 100);
        m.set("lives", 3);
        return m;
    }

    public static function testEnum():Action {
        return Move(10, 20);
    }
}
```

### Example
```cpp
// 05_advanced_values.cpp
// Array
hlffi_value* arr = hlffi_call_static(vm, "Game", "testArray", 0, NULL);
printf("Length: %d\n", hlffi_array_length(arr));
hlffi_value* first = hlffi_array_get(arr, 0);
printf("First: %d\n", hlffi_value_as_int(first));

// Map
hlffi_value* map = hlffi_call_static(vm, "Game", "testMap", 0, NULL);
hlffi_value* key = hlffi_value_string("score");
hlffi_value* score = hlffi_map_get(map, key);
printf("Score: %d\n", hlffi_value_as_int(score));

// Enum
hlffi_value* action = hlffi_call_static(vm, "Game", "testEnum", 0, NULL);
printf("Action: %s\n", hlffi_enum_name(action));
hlffi_value* x = hlffi_enum_get_param(action, 0);
printf("X: %d\n", hlffi_value_as_int(x));
```

### Success Criteria
- ✓ Can create and manipulate arrays
- ✓ Can work with maps
- ✓ Can construct and match enums
- ✓ Bytes I/O works correctly

---

## Phase 6: Callbacks & Exceptions
**Goal**: C→Haxe callbacks, exception handling
**Duration**: ~8 hours
**Deliverable**: Bidirectional C↔Haxe calls, safe error handling

### Features
- [ ] Register C function as Haxe callback
- [ ] Call C function from Haxe
- [ ] Try/catch for Haxe exceptions
- [ ] Get exception message/stack
- [ ] Custom error handlers

### API Design
```c
// Callbacks: C functions callable from Haxe
typedef hlffi_value* (*hlffi_native_func)(hlffi_vm* vm, int argc, hlffi_value** args);

void hlffi_register_callback(
    hlffi_vm* vm,
    const char* name,
    hlffi_native_func func,
    int nargs
);

// Exception handling
typedef enum {
    HLFFI_CALL_OK,
    HLFFI_CALL_EXCEPTION,
    HLFFI_CALL_ERROR
} hlffi_call_result;

hlffi_call_result hlffi_try_call_static(
    hlffi_vm* vm,
    const char* cls,
    const char* method,
    int argc,
    hlffi_value** args,
    hlffi_value** out_result,
    const char** out_error
);

const char* hlffi_get_exception_message(hlffi_vm* vm);
const char* hlffi_get_exception_stack(hlffi_vm* vm);
```

### ⚠️ Critical Implementation Notes

**External Blocking Call Wrapper (Gotcha #7)**:
```c
// CRITICAL: Notify GC when calling external blocking code!
typedef void (*hlffi_external_func)(void* userdata);

void hlffi_call_external_blocking(
    hlffi_vm* vm,
    hlffi_external_func func,
    void* userdata
) {
    hl_blocking(true);   // ⭐ Tell GC we're leaving HL control
    func(userdata);      // Call external code (file I/O, network, etc.)
    hl_blocking(false);  // ⭐ Back under HL control
}

// Example usage:
void save_file(void* path) {
    FILE* f = fopen((const char*)path, "w");
    fwrite(...);  // Potentially long operation
    fclose(f);
}

// In callback from Haxe:
hlffi_value* on_save(hlffi_vm* vm, int argc, hlffi_value** args) {
    const char* path = hlffi_value_as_string(args[0]);
    hlffi_call_external_blocking(vm, save_file, (void*)path);
    return hlffi_value_null(vm);
}
```

**Why**: GC needs to know when thread is blocked outside HL. Prevents GC from waiting on blocked thread.

**Callback GC Root Management**:
```c
// Store C callbacks for Haxe to call
typedef struct {
    vclosure *hl_closure;
    hlffi_native_func c_func;
    int nargs;
} hlffi_callback_entry;

void hlffi_register_callback(hlffi_vm* vm, const char* name, hlffi_native_func func, int nargs) {
    // Create HL closure wrapping C function
    vclosure *closure = create_native_closure(func, nargs);

    // Add GC root - callback must survive!
    hl_add_root(&vm->callbacks[index].hl_closure);

    // Store for later retrieval by Haxe
    vm->callbacks[index].hl_closure = closure;
    vm->callbacks[index].c_func = func;
}
```

**Why**: Callbacks stored for Haxe to call later must be GC-rooted (Common Pattern #3).

**Bidirectional Object Passing** (Improvement over Ruby library):
```c
// Ruby library limitation: Can't pass Ruby objects to Haxe
// HLFFI v3.0: Support native C structs wrapped as Haxe objects

typedef struct {
    int x, y;
} Point;

// Wrap C struct as Haxe dynamic object
hlffi_value* wrap_c_struct(Point* pt) {
    vdynobj *obj = hl_alloc_dynobj();
    hl_dyn_seti(obj, hl_hash_utf8("x"), &hlt_i32, pt->x);
    hl_dyn_seti(obj, hl_hash_utf8("y"), &hlt_i32, pt->y);
    return wrap_value((vdynamic*)obj);
}
```

**Why**: Enables true bidirectional FFI, unlike Ruby library's one-way limitation.

### Test Haxe Code
```haxe
class Callbacks {
    public static var onEvent:(String)->Void;

    public static function triggerEvent(msg:String):Void {
        if(onEvent != null) onEvent(msg);
    }

    public static function mightThrow(shouldThrow:Bool):Void {
        if(shouldThrow) throw "Something went wrong!";
    }
}
```

### Example
```cpp
// 06_callbacks.cpp
// Define C callback
hlffi_value* on_event(hlffi_vm* vm, int argc, hlffi_value** args) {
    const char* msg = hlffi_value_as_string(args[0]);
    printf("Event from Haxe: %s\n", msg);
    return hlffi_value_null(vm);
}

// Register it
hlffi_register_callback(vm, "onEvent", on_event, 1);
hlffi_set_static_field(vm, "Callbacks", "onEvent",
    hlffi_get_callback(vm, "onEvent"));

// Trigger from Haxe
hlffi_value* msg = hlffi_value_string("Hello from C!");
hlffi_call_static(vm, "Callbacks", "triggerEvent", 1, &msg);

// Handle exceptions
hlffi_value* result;
const char* error;
hlffi_value* arg = hlffi_value_bool(true);
hlffi_call_result res = hlffi_try_call_static(
    vm, "Callbacks", "mightThrow", 1, &arg, &result, &error
);
if(res == HLFFI_CALL_EXCEPTION) {
    printf("Exception: %s\n", error);
}
```

### Success Criteria
- ✓ Can call C from Haxe
- ✓ Exceptions caught and reported
- ✓ Stack traces available

---

## Phase 7: Performance & Polish
**Goal**: Optimize, cache, benchmark, document
**Duration**: ~6 hours
**Deliverable**: Production-ready library

### Features
- [ ] Method/field lookup caching
- [ ] Benchmark suite
- [ ] Performance comparison vs raw HL API
- [ ] Memory profiling
- [ ] Complete documentation
- [ ] Migration guide from old hlffi
- [ ] Best practices guide

### Optimizations
- Cache type lookups
- Cache method/field indices
- Fast paths for common operations
- Reduce allocations
- SIMD for bulk conversions (future)

### Benchmarks
- Static method call overhead
- Instance method call overhead
- Field access overhead
- Array operations
- Type lookup cost
- Caching effectiveness

### Documentation
- Complete API reference
- Tutorial for each phase
- Common patterns
- Troubleshooting guide
- Performance tips

### Success Criteria
- ✓ Overhead < 5% vs raw HL calls
- ✓ Zero memory leaks (valgrind clean)
- ✓ Complete examples for all features
- ✓ API documentation 100% coverage

---

## Phase 8: Cross-Platform (Future)
**Goal**: Support all platforms
**Duration**: ~8 hours
**Deliverable**: Works on WebAssembly, Android, RPi

### Platforms
- [ ] WebAssembly (emscripten)
- [ ] Android (NDK)
- [ ] Raspberry Pi (ARM)
- [ ] Linux
- [ ] macOS
- [ ] iOS

### Platform-Specific Challenges
- WASM: different calling conventions, requires HL/C mode
- Android: JNI integration, HL/C recommended
- RPi: 32-bit ARM quirks, HL/C required
- Mobile: limited memory, HL/C for best performance

### HL/JIT vs HL/C per Platform
| Platform | HL/JIT | HL/C | Recommended |
|----------|--------|------|-------------|
| Windows x64 | ✅ | ✅ | JIT (dev), C (prod) |
| Linux x64 | ✅ | ✅ | JIT (dev), C (prod) |
| macOS x64 | ✅ | ✅ | JIT (dev), C (prod) |
| WebAssembly | ❌ | ✅ | C only |
| Android ARM | ❌ | ✅ | C only |
| Raspberry Pi | ❌ | ✅ | C only |
| iOS | ❌ | ✅ | C only |

---

## Phase 9: Plugin/Module System 🔌 (Experimental)
**Goal**: Dynamic module loading at runtime
**Duration**: ~6 hours
**Deliverable**: Load and unload HL modules dynamically
**Status**: ⚠️ Bleeding-edge HL feature (experimental)

### Overview
HashLink's bleeding-edge version supports loading multiple JIT modules at runtime, enabling:
- **Game mods**: Load user-created content
- **Plugin architecture**: Extend app without recompilation
- **Dynamic extensions**: Add features at runtime
- **Hot-swappable modules**: Replace modules on-the-fly

**Current Status** (per [Issue #179](https://github.com/HaxeFoundation/hashlink/issues/179)):
- Multiple JIT modules can now be loaded
- Type sharing between modules in development
- Experimental, API may change

### Features
- [ ] `hlffi_load_plugin()` - load additional .hl module
- [ ] `hlffi_unload_plugin()` - unload plugin module
- [ ] `hlffi_list_plugins()` - enumerate loaded plugins
- [ ] Type resolution across modules
- [ ] Symbol lookup in specific modules
- [ ] Plugin isolation/sandboxing options
- [ ] Plugin dependency management

### API Design
```c
typedef struct hlffi_plugin hlffi_plugin;

// Plugin management
hlffi_plugin* hlffi_load_plugin(hlffi_vm* vm, const char* path);
hlffi_error_code hlffi_unload_plugin(hlffi_plugin* plugin);
bool hlffi_is_plugin_loaded(hlffi_plugin* plugin);
const char* hlffi_plugin_get_name(hlffi_plugin* plugin);

// Cross-module calls
hlffi_value* hlffi_call_plugin_static(
    hlffi_plugin* plugin,
    const char* class_name,
    const char* method_name,
    int argc,
    hlffi_value** args
);

// Type sharing (experimental)
hlffi_type* hlffi_plugin_find_type(hlffi_plugin* plugin, const char* name);
hlffi_error_code hlffi_plugin_share_type(hlffi_plugin* from, hlffi_plugin* to, const char* type_name);
```

### Example Use Case: Game Modding
```haxe
// Core game (core.hl)
class ModLoader {
    public static var mods:Array<Mod> = [];

    public static function loadMod(path:String):Void {
        // Loaded via FFI from C side
    }
}

// User mod (usermod.hl)
class MyMod implements Mod {
    public function onInit():Void {
        trace("Mod loaded!");
    }

    public function onUpdate(dt:Float):Void {
        // Custom mod logic
    }
}
```

```cpp
// C++ host code
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "core.hl");

// Load user mods dynamically
hlffi_plugin* mod1 = hlffi_load_plugin(vm, "mods/usermod1.hl");
hlffi_plugin* mod2 = hlffi_load_plugin(vm, "mods/usermod2.hl");

// Call into mods
hlffi_call_plugin_static(mod1, "MyMod", "onInit", 0, NULL);

// Later: unload mod
hlffi_unload_plugin(mod1);
```

### Challenges to Solve
1. **Type Compatibility**: Ensure types match between core and plugins
2. **Symbol Conflicts**: Handle duplicate class/function names
3. **Memory Safety**: Prevent crashes when unloading active modules
4. **Version Compatibility**: Ensure plugin bytecode version matches
5. **GC Integration**: Track references across module boundaries

### Test Cases
- Load simple plugin module
- Call function in plugin from core
- Share type between core and plugin
- Unload plugin and verify cleanup
- Handle plugin load failures
- Multiple plugins with dependencies

### Success Criteria
- ✓ Can load additional .hl modules at runtime
- ✓ Can call functions across modules
- ✓ Can unload modules safely
- ✓ Type sharing works between modules
- ✓ No memory leaks when loading/unloading

### Limitations
- ⚠️ HL/JIT only (not HL/C)
- ⚠️ Experimental API, may change
- ⚠️ HashLink bleeding-edge version required
- ⚠️ Type compatibility requires careful design

---

## Timeline Estimate

| Phase | Duration | Cumulative |
|-------|----------|------------|
| 0: Foundation | 4h | 4h |
| 1: Lifecycle + Hot Reload 🔥 | 8h | 12h |
| 2: Type System | 8h | 20h |
| 3: Static Members | 6h | 26h |
| 4: Instance Members | 8h | 34h |
| 5: Advanced Values | 10h | 44h |
| 6: Callbacks/Exceptions | 8h | 52h |
| 7: Performance | 6h | 58h |
| 8: Cross-Platform (HL/C) | 8h | 66h |
| 9: Plugin System 🔌 (Optional) | 6h | 72h |

**Total**: ~72 hours (9 working days)
**Usable after Phase 3**: ~26 hours (3+ days)
**Full-featured after Phase 7**: ~58 hours (7+ days)
**With all experimental features**: ~72 hours (9 days)

---

## Deliverables per Phase

| Phase | What You Can Do |
|-------|-----------------|
| 0 | Build system works, compiles on Windows |
| 1 | Load & run bytecode, restart VM, **hot reload** 🔥 |
| 2 | Query all types and members via reflection |
| 3 | Call static methods, use static fields ✓ **USABLE** |
| 4 | Create objects, call instance methods ✓ **FULL FFI** |
| 5 | Work with arrays, maps, enums, bytes ✓ **COMPLETE** |
| 6 | Callbacks (C↔Haxe), exception handling ✓ **ROBUST** |
| 7 | Optimized, benchmarked, documented ✓ **PRODUCTION** |
| 8 | All platforms (HL/JIT + HL/C) ✓ **UNIVERSAL** |
| 9 | Dynamic plugin/module loading ✓ **EXTENSIBLE** |

---

## Implementation Strategy

### Repository Structure
```
hlffi/
├── include/hlffi.h        // C API (header-only capable)
├── include/hlffi.hpp      // C++ wrappers (Phase 3+)
├── src/hlffi.c            // Compiled lib implementation
├── tests/                 // Test suite per phase
├── examples/              // Working examples per phase
├── docs/                  // API documentation
└── CMakeLists.txt         // Build system
```

### Git Workflow
- **Main branch**: Stable releases only
- **Feature branches**: `phase-N-*` for each phase
- **Tags**: `v3.0-phase-N` for each completed phase
- **Each phase**: Merge to main when tests pass

### Testing Strategy
- **Unit tests**: Each function has test coverage
- **Integration tests**: End-to-end scenarios
- **Example validation**: All examples compile and run
- **Memory tests**: Valgrind/sanitizers clean (Phase 7)

### Documentation
- **API docs**: Inline comments + generated reference
- **Tutorials**: One per phase with full code
- **Migration guide**: From v2.x to v3.0
- **Best practices**: Common patterns and pitfalls

---

## Next Steps

**Ready to start Phase 0!** 🚀

Confirmed decisions:
- ✅ Library: Header-only + compiled
- ✅ API: C core + C++ wrappers
- ✅ Errors: Return codes
- ✅ Values: GC-integrated
- ✅ Target: Windows first

**Let's begin implementation!**
