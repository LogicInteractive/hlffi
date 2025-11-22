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

## Phase 1: VM Lifecycle
**Goal**: Full VM control - init, load, run, stop, restart
**Duration**: ~6 hours
**Deliverable**: Can load and execute HL bytecode, restart VM

### Features
- [ ] `hlffi_create()` - allocate VM
- [ ] `hlffi_init()` - initialize HashLink runtime + args
- [ ] `hlffi_load_file()` - load .hl from disk
- [ ] `hlffi_load_memory()` - load .hl from buffer
- [ ] `hlffi_call_entry()` - invoke Main.main()
- [ ] `hlffi_shutdown()` - cleanup runtime (prepare for restart)
- [ ] `hlffi_destroy()` - free VM
- [ ] `hlffi_restart()` - shutdown + reinit in one call
- [ ] Proper error codes for each step

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
    // ... more
} hlffi_error_code;

hlffi_vm* hlffi_create(void);
hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv);
hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path);
hlffi_error_code hlffi_load_memory(hlffi_vm* vm, const void* data, size_t size);
hlffi_error_code hlffi_call_entry(hlffi_vm* vm);
hlffi_error_code hlffi_shutdown(hlffi_vm* vm);  // NEW: allows restart
hlffi_error_code hlffi_restart(hlffi_vm* vm);   // NEW: convenience
void hlffi_destroy(hlffi_vm* vm);
const char* hlffi_get_error(hlffi_vm* vm);
```

### Test Cases
- Load valid bytecode
- Load invalid bytecode (should fail gracefully)
- Call entry multiple times (should fail)
- Shutdown and restart with different bytecode
- Shutdown without init (should fail safely)

### Example
```cpp
// 01_lifecycle.cpp
hlffi_vm* vm = hlffi_create();
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "test.hl");
hlffi_call_entry(vm);

// Later: restart with new bytecode
hlffi_shutdown(vm);
hlffi_init(vm, argc, argv);
hlffi_load_file(vm, "test2.hl");
hlffi_call_entry(vm);

hlffi_destroy(vm);
```

### Success Criteria
- ✓ Can load and run .hl bytecode
- ✓ Can restart VM without memory leaks
- ✓ All error paths tested and safe

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
- WASM: different calling conventions
- Android: JNI integration
- RPi: 32-bit ARM quirks
- Mobile: limited memory

---

## Timeline Estimate

| Phase | Duration | Cumulative |
|-------|----------|------------|
| 0: Foundation | 4h | 4h |
| 1: Lifecycle | 6h | 10h |
| 2: Type System | 8h | 18h |
| 3: Static Members | 6h | 24h |
| 4: Instance Members | 8h | 32h |
| 5: Advanced Values | 10h | 42h |
| 6: Callbacks/Exceptions | 8h | 50h |
| 7: Performance | 6h | 56h |
| 8: Cross-Platform | 8h | 64h |

**Total**: ~64 hours (8 working days)
**Usable after Phase 3**: ~24 hours (3 days)

---

## Deliverables per Phase

| Phase | What You Can Do |
|-------|-----------------|
| 0 | Build system works |
| 1 | Load & run bytecode, restart VM |
| 2 | Query all types and members |
| 3 | Call static methods, use static fields ✓ **USABLE** |
| 4 | Create objects, call methods ✓ **FULL FFI** |
| 5 | Work with arrays, maps, enums ✓ **COMPLETE** |
| 6 | Callbacks, exception handling ✓ **ROBUST** |
| 7 | Optimized, documented ✓ **PRODUCTION** |
| 8 | All platforms ✓ **UNIVERSAL** |

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
