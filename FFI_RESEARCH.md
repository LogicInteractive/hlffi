# HashLink FFI - Comprehensive Research Document

**Research Date**: 2025-11-22
**Purpose**: Comprehensive knowledge base for building HLFFI v3.0
**Sources**: Official HashLink docs, GitHub issues, community forums, existing projects

---

## Table of Contents

1. [Overview](#overview)
2. [Existing Projects & Implementations](#existing-projects--implementations)
3. [Official C API Reference](#official-c-api-reference)
4. [Memory Model & Garbage Collection](#memory-model--garbage-collection)
5. [Type System](#type-system)
6. [Calling Conventions](#calling-conventions)
7. [Threading & Concurrency](#threading--concurrency)
8. [Common Patterns & Best Practices](#common-patterns--best-practices)
9. [Known Issues & Gotchas](#known-issues--gotchas)
10. [Code Examples](#code-examples)
11. [Key Takeaways for HLFFI v3.0](#key-takeaways-for-hlffi-v30)

---

## Overview

### What is HashLink FFI?

Foreign Function Interface (FFI) for HashLink enables:
- **Embedding**: Running HashLink VM inside C/C++ applications
- **Native Extensions**: Calling C/C++ from Haxe code
- **Bidirectional Communication**: C ↔ Haxe function calls

### Two Main Use Cases

| Use Case | Direction | Purpose |
|----------|-----------|---------|
| **Embedding** | C → Haxe | Game engines, tools embedding Haxe scripts |
| **Native Extensions** | Haxe → C | Performance-critical code, hardware access |

**Our Focus**: Embedding (C → Haxe) for HLFFI v3.0

---

## Existing Projects & Implementations

### 1. MECore Game Engine
- **GitHub**: [datee/MECore](https://github.com/datee/MECore)
- **Architecture**: C++ engine core + Haxe scripting via HashLink
- **Stack**: Vulkan 1.3 + Jolt Physics + HashLink VM
- **Language Split**: 75.7% C++, 22.7% Haxe
- **Status**: Educational/experimental (not production-ready)
- **Key Insight**: Uses compiled scripting layer approach (HL/C)
- **Structure**:
  - `api/` - FFI/binding declarations
  - `core/` - C++ engine implementation
  - `extension/` - Plugin system

**Lessons Learned**:
- CMake-based build with custom code generation
- Separates API layer from core implementation
- Likely uses HL/C mode for compilation

### 2. Official HashLink Examples

**Location**: [HashLink Repository](https://github.com/HaxeFoundation/hashlink)

Key examples:
- `src/main.c` - **Primary embedding reference**
- `libs/fmt/fmt.c` - FFI example ([Haxe API](https://github.com/HaxeFoundation/haxe/blob/development/std/hl/Format.hx))
- `libs/ui/ui_win.c` - Windows UI native extension
- `libs/uv/uv.c` - libuv integration

### 3. Heaps.io Game Engine
- **Purpose**: Cross-platform game framework
- **Integration**: Uses HashLink as primary runtime
- **Documentation**: [Hello HashLink](https://heaps.io/documentation/hello-hashlink.html)

### 4. Community Tools

**hlbc** - [Gui-Yom/hlbc](https://github.com/Gui-Yom/hlbc)
- HashLink bytecode disassembler/analyzer/decompiler
- Useful for understanding bytecode structure

**Khabind** - [Binding C++ to Kha](https://gist.github.com/zicklag/6c2828cda4e8282c850d97ac8a3a0de5)
- Method for binding C++ libraries to Haxe (Krom/HTML5/HashLink targets)

---

## Official C API Reference

### Core Headers

**Primary Header**: `hl.h` - Complete C API
**Documentation**: [C API Documentation](https://github.com/HaxeFoundation/hashlink/wiki/C-API-Documentation)

### Platform Detection

```c
// Version
#define HL_VERSION 0x011000  // 1.16.0

// Platforms
HL_WIN, HL_MAC, HL_LINUX, HL_ANDROID, HL_IOS,
HL_EMSCRIPTEN, HL_PS, HL_NX, HL_XBO, HL_XBS

// Architecture
HL_64        // 64-bit mode
HL_WSIZE     // Pointer size (4 or 8)
```

### Type System

#### Type Kinds Enum
```c
typedef enum {
    HVOID, HUI8, HUI16, HI32, HI64, HF32, HF64, HBOOL, HBYTES, HDYN,
    HFUN, HOBJ, HARRAY, HTYPE, HREF, HVIRTUAL, HDYNOBJ, HABSTRACT,
    HENUM, HNULL, HMETHOD, HSTRUCT, HPACKED, HGUID
} hl_type_kind;
```

#### Core Structures

```c
typedef struct {
    hl_type *t;           // Type pointer
    union {
        bool b;           // HBOOL
        int i;            // HI32
        float f;          // HF32
        double d;         // HF64
        vbyte *bytes;     // HBYTES
        void *ptr;        // Various pointer types
        int64 i64;        // HI64
    } v;
} vdynamic;

typedef struct {
    hl_type *t;
    void (*fun)(void);    // Function pointer
    void *value;          // Captured context
    int hasValue;
} vclosure;

typedef struct {
    hl_type *at;          // Array element type
    int size;             // Number of elements
    // Elements follow in memory
} varray;

typedef struct {
    hl_type_obj *obj;     // Object type info
    // Fields follow based on obj layout
} vobj;

typedef struct {
    hl_type *t;
    // Field lookup hash table follows
} vdynobj;
```

#### Predefined Types

```c
// Available as global constants
hlt_void, hlt_i32, hlt_i64, hlt_f64, hlt_f32,
hlt_dyn, hlt_array, hlt_bytes, hlt_dynobj, hlt_bool, hlt_abstract
```

### Memory Allocation

#### GC Allocation Functions

```c
// Opaque memory (no pointers) - for strings/binary data
#define hl_gc_alloc_noptr(size)

// Raw memory (may contain HL pointers) - for arrays of values
#define hl_gc_alloc_raw(size)

// Memory with finalizer callback
#define hl_gc_alloc_finalizer(size)

// Typed allocation
#define hl_gc_alloc(type, size)
```

**Memory Kind Flags**:
```c
MEM_KIND_DYNAMIC    // 0 - vdynamic compatible
MEM_KIND_RAW        // 1 - can contain pointers
MEM_KIND_NOPTR      // 2 - no pointers (not scanned by GC)
MEM_KIND_FINALIZER  // 3 - has finalizer callback
```

#### HL Value Allocation

```c
varray*    hl_alloc_array(hl_type *t, int size);
vdynamic*  hl_alloc_dynamic(hl_type *t);
vobj*      hl_alloc_obj(hl_type *t);
venum*     hl_alloc_enum(hl_type *t, int index);
vvirtual*  hl_alloc_virtual(hl_type *t);
vdynobj*   hl_alloc_dynobj(void);
vbyte*     hl_alloc_bytes(int size);
vclosure*  hl_alloc_closure_void(hl_type *t, void *fvalue);
vclosure*  hl_alloc_closure_ptr(hl_type *fullt, void *fvalue, void *ptr);
```

### Dynamic Field Access

#### Field Getters
```c
int       hl_dyn_geti(vdynamic *d, int hfield, hl_type *t);
int64     hl_dyn_geti64(vdynamic *d, int hfield);
void*     hl_dyn_getp(vdynamic *d, int hfield, hl_type *t);
float     hl_dyn_getf(vdynamic *d, int hfield);
double    hl_dyn_getd(vdynamic *d, int hfield);
```

#### Field Setters
```c
void hl_dyn_seti(vdynamic *d, int hfield, hl_type *t, int value);
void hl_dyn_seti64(vdynamic *d, int hfield, int64 value);
void hl_dyn_setp(vdynamic *d, int hfield, hl_type *t, void *ptr);
void hl_dyn_setf(vdynamic *d, int hfield, float f);
void hl_dyn_setd(vdynamic *d, int hfield, double v);
```

#### Field Hashing
```c
int hl_hash_utf8(const char *str);      // Hash field name
const char* hl_field_name(int hash);     // Reverse lookup (debug only)

// Example
int field_hash = hl_hash_utf8("myField");
int value = hl_dyn_geti(obj, field_hash, &hlt_i32);
```

### Function Calling

#### Direct Calling (Fast Path)
```c
// Macro-based (zero overhead)
hl_call0(ret, closure)
hl_call1(ret, closure, type1, value1)
hl_call2(ret, closure, type1, value1, type2, value2)
hl_call3(ret, closure, type1, value1, type2, value2, type3, value3)
hl_call4(ret, closure, type1, value1, type2, value2, type3, value3, type4, value4)

// Example
vdynamic *result = hl_call0(vdynamic*, my_closure);
```

#### Dynamic Calling (Slow Path)
```c
vdynamic* hl_dyn_call(vclosure *c, vdynamic **args, int nargs);
vdynamic* hl_dyn_call_safe(vclosure *c, vdynamic **args, int nargs, bool *isException);

// Example
vdynamic *args[2] = {hl_alloc_dynamic(&hlt_i32), hl_alloc_dynamic(&hlt_i32)};
args[0]->v.i = 5;
args[1]->v.i = 3;
vdynamic *result = hl_dyn_call(closure, args, 2);
```

### String Handling

```c
// UTF-16 (UCS-2) to UTF-8
char* hl_to_utf8(const uchar *str);

// UTF-8 to UTF-16
uchar* hl_from_utf8(const char *str);

// Dynamic to string
uchar* hl_to_string(vdynamic *v);

// Allocate string with formatting
vstring* hl_alloc_strbytes(const uchar *msg, ...);
```

**Important**: HashLink strings are UTF-16 (16-bit unicode), not UTF-8!

### Type Utilities

```c
int  hl_type_size(hl_type *t);              // Get type size in bytes
bool hl_is_dynamic(hl_type *t);             // Check if type is dynamic
bool hl_same_type(hl_type *a, hl_type *b);  // Type equality
bool hl_safe_cast(hl_type *t, hl_type *to); // Check cast safety
char* hl_type_str(hl_type *t);              // Type to string (debug)
```

---

## Memory Model & Garbage Collection

### GC Architecture

**Type**: Mark-and-not-sweep collector based on Immix
**Implementation**: `src/alloc.c`
**Page Size**: 64 KB aligned blocks
**Line Size**: 128 bytes within blocks

**Documentation**:
- [HashLink GC Blog](https://haxe.org/blog/hashlink-gc/)
- [GC Notes](https://github.com/HaxeFoundation/hashlink/wiki/Notes-on-Garbage-Collector)

### Memory Organization

```
┌─────────────────────────┐
│  GC Heap                │
├─────────────────────────┤
│  Blocks (64 KB each)    │
│  ├─ Lines (128 bytes)   │
│  ├─ Lines (128 bytes)   │
│  └─ ...                 │
└─────────────────────────┘
```

**Block Ownership**:
- Blocks are owned by threads
- Most common allocations don't require synchronization
- Thread-safe by design

### Precision

**Mostly Precise GC**: Uses type system metadata (`mark_bits`) to identify pointers
- Prevents false positives from numeric values
- Efficient mark phase using CPU bit counting

### GC Roots

```c
// Add a root (prevents GC)
void hl_add_root(void *ptr);

// Remove a root
void hl_remove_root(void *ptr);

// Check if pointer is GC-managed
bool hl_is_gc_ptr(void *ptr);

// Get allocation size
int hl_gc_get_memsize(void *ptr);
```

**Usage Pattern** (for stored closures):
```c
vclosure *stored_callback = NULL;

void set_callback(vclosure *cb) {
    if (stored_callback) hl_remove_root(&stored_callback);
    stored_callback = cb;
    if (cb) hl_add_root(&stored_callback);  // Prevent GC
}
```

### GC Triggering

```c
void hl_gc_major(void);  // Force major collection
```

### Blocking/Unblocking

**Critical for Plugin/Threading**:
```c
void hl_blocking(bool b);
bool hl_is_blocking(void);
```

**Usage** (when exiting/entering HL scope):
```c
// Leaving HL scope (e.g., calling external code)
hl_blocking(true);
external_blocking_function();
hl_blocking(false);
```

**Why**: Informs GC that thread is outside HL control

---

## Type System

### Finding Types

#### By Name (with package support)
```c
// Search all types in loaded module
for (int i = 0; i < code->ntypes; i++) {
    hl_type *t = code->types[i];
    if (t->kind == HOBJ) {
        const char *name = hl_to_utf8(t->obj->name);
        if (strcmp(name, "MyClass") == 0) {
            // Found it!
        }
    }
}
```

### Object Type Information

```c
typedef struct {
    const uchar *name;
    hl_type super;           // Parent class
    hl_runtime_obj *rt;      // Runtime info
    int nfields;             // Field count
    int nproto;              // Proto field count
    int nbindings;           // Binding count
    hl_obj_field *fields;    // Field definitions
    hl_obj_proto *proto;     // Methods
    int *bindings;           // Binding indices
    void **global_value;     // Static/global instance
} hl_type_obj;

typedef struct {
    const uchar *name;
    hl_type *t;              // Field type
    int hashed_name;         // Hash of field name
} hl_obj_field;

typedef struct {
    const uchar *name;
    hl_type *t;              // Method type
    int findex;              // Function index
    int hashed_name;
    int pindex;              // Virtual index (-1 for final)
} hl_obj_proto;
```

### Class Instance Creation

**Pattern from Issue #253**:
```c
// 1. Find class type
hl_type *class_type = find_type_by_name(code, "ClassA");

// 2. Get global type (constructor expects this)
vdynamic *global = *(vdynamic**)class_type->obj->global_value;

// 3. Allocate instances
vdynamic *inst_main = hl_alloc_obj(class_type);
vdynamic *inst_global = hl_alloc_obj(global->t);

// 4. Find and call constructor
int constructor_hash = hl_hash_utf8("new");
hl_field_lookup *lookup = obj_resolve_field(class_type->obj, constructor_hash);
vclosure *constructor = (vclosure*)lookup->field_value;

// 5. Call constructor (workaround for type mismatch)
hl_dyn_call_constructor(constructor, inst_main, NULL, 0);
```

**Important**: Call entry point before accessing global values (otherwise NULL)!

### Method Lookup

```c
// Using field hash
int method_hash = hl_hash_utf8("methodName");
hl_field_lookup *lookup = obj_resolve_field(type->obj, method_hash);

// Call method
vclosure *method = (vclosure*)lookup->field_value;
vdynamic *result = hl_dyn_call(method, args, nargs);
```

---

## Calling Conventions

### Fast Path (Compile-Time Known Arity)

**Use when**: Function signature known at compile time

```c
vclosure *func = /* lookup */;

// 0 arguments
vdynamic *result = hl_call0(vdynamic*, func);

// 1 argument (int)
int arg = 42;
vdynamic *result = hl_call1(vdynamic*, func, &hlt_i32, arg);

// 2 arguments (int, float)
int arg1 = 42;
float arg2 = 3.14f;
vdynamic *result = hl_call2(vdynamic*, func, &hlt_i32, arg1, &hlt_f32, arg2);
```

**Performance**: ~0-2 ns overhead (essentially free)

### Slow Path (Runtime Arity)

**Use when**: Argument count/types determined at runtime

```c
vclosure *func = /* lookup */;

// Build argument array
vdynamic *args[3];
args[0] = hl_alloc_dynamic(&hlt_i32);
args[0]->v.i = 42;
args[1] = hl_alloc_dynamic(&hlt_f32);
args[1]->v.f = 3.14f;
args[2] = hl_alloc_dynamic(&hlt_bytes);
args[2]->v.bytes = hl_alloc_bytes(100);

// Call
vdynamic *result = hl_dyn_call(func, args, 3);
```

**Performance**: ~10-20 ns overhead (still fast but measurable)

### Type Coercion

`hl_dyn_call()` automatically coerces types:
- Widens numeric types (int → float)
- Boxes primitives to dynamic
- Unboxes dynamic to concrete types

### Safe Calling (with Exception Handling)

```c
bool is_exception = false;
vdynamic *result = hl_dyn_call_safe(func, args, nargs, &is_exception);

if (is_exception) {
    // Handle exception
    const char *msg = hl_to_utf8(hl_to_string(result));
    printf("Exception: %s\n", msg);
}
```

---

## Threading & Concurrency

### Thread Registration

**CRITICAL**: Every thread that touches HL must register!

```c
// Main thread (embedding)
void *stack_top = NULL;
hl_register_thread(&stack_top);

// Worker thread
void worker_thread(void *param) {
    void *stack_top = NULL;
    hl_register_thread(&stack_top);

    // ... HL operations ...

    hl_unregister_thread();
}
```

### Stack Top Requirement

**Issue #752 Insight**: Stack top must remain valid during GC

**Problem**: Plugin initialization function stack goes out of scope

**Solution**: Use persistent stack variable from host's main():
```c
// Host application main()
int main(int argc, char **argv) {
    void *stack_top_storage = NULL;  // Persistent!
    init_plugin(&stack_top_storage);
    // ...
}

// Plugin init
void init_plugin(void **stack_ref) {
    hl_register_thread(stack_ref);
}
```

### Synchronization Primitives

#### Mutex
```c
hl_mutex *mutex = hl_mutex_alloc(bool gc_thread);
hl_mutex_acquire(mutex);
hl_mutex_release(mutex);
bool acquired = hl_mutex_try_acquire(mutex);
hl_mutex_free(mutex);
```

#### Semaphore
```c
hl_semaphore *sem = hl_semaphore_alloc(int initial_value);
hl_semaphore_acquire(sem);
bool acquired = hl_semaphore_try_acquire(sem, timeout);
hl_semaphore_release(sem);
hl_semaphore_free(sem);
```

#### Condition Variable
```c
hl_condition *cond = hl_condition_alloc();
hl_condition_acquire(cond);
hl_condition_wait(cond);
bool timeout = hl_condition_timed_wait(cond, double timeout);
hl_condition_signal(cond);      // Wake one
hl_condition_broadcast(cond);   // Wake all
hl_condition_release(cond);
hl_condition_free(cond);
```

#### Thread-Local Storage
```c
hl_tls *tls = hl_tls_alloc(bool gc_value);
hl_tls_set(tls, void *value);
void *value = hl_tls_get(tls);
hl_tls_free(tls);
```

### Thread Creation
```c
hl_thread_id thread = hl_thread_start(void *callback, void *param, bool withGC);
hl_thread_id current = hl_thread_current();
hl_thread_yield();
```

---

## Common Patterns & Best Practices

### 1. Embedding Initialization Pattern

**From `main.c` reference**:

```c
// Step 1: Initialize globals
hl_global_init();

// Step 2: Register main thread
void *stack_top = NULL;
hl_register_thread(&stack_top);

// Step 3: Initialize sys (argc/argv)
hl_sys_init(argv, argc, NULL);

// Step 4: Load bytecode
unsigned char *bytecode = read_file("game.hl", &bytecode_size);
hl_code *code = NULL;
if (!hl_code_read(bytecode, bytecode_size, &code)) {
    fprintf(stderr, "Failed to load bytecode\n");
    exit(1);
}

// Step 5: Allocate and initialize module
hl_module *module = hl_module_alloc(code);
if (!hl_module_init(module, NULL, NULL)) {
    fprintf(stderr, "Module init failed\n");
    exit(1);
}

// Step 6: Build entry point closure
int entry_index = code->entrypoint;
int func_index = module->functions_indexes[entry_index];
hl_type *entry_type = code->functions[entry_index].type;
void *entry_ptr = module->functions_ptrs[func_index];
vclosure *entry = hl_alloc_closure_void(entry_type, entry_ptr);

// Step 7: Call entry point (REQUIRED!)
hl_call0(void, entry);

// ... Application runs ...

// Cleanup
hl_global_free();
```

**CRITICAL**: Entry point call is NOT optional - sets up global state!

### 2. Native Extension Pattern

**From native extension tutorial**:

```c
// Extension header
#define HL_NAME(n) mylib_##n
#include <hl.h>

// Define function
HL_PRIM int HL_NAME(add)(int a, int b) {
    return a + b;
}

// Register with HL
DEFINE_PRIM(_I32, add, _I32 _I32);
```

**Haxe side**:
```haxe
@:hlNative("mylib")
class MyLib {
    public static function add(a:Int, b:Int):Int {
        return 0;  // Implementation in C
    }
}
```

### 3. Callback Storage Pattern

```c
// Store HL callback from C
vclosure *render_callback = NULL;

HL_PRIM void HL_NAME(set_render_callback)(vclosure *cb) {
    if (render_callback) hl_remove_root(&render_callback);
    render_callback = cb;
    if (cb) hl_add_root(&render_callback);
}

// Call from C later
void on_render() {
    if (render_callback) {
        hl_dyn_call(render_callback, NULL, 0);
    }
}
```

### 4. Dynamic Object Creation Pattern

```c
// Build dynamic object in C
HL_PRIM vdynamic* HL_NAME(create_result)(int width, int height) {
    vdynobj *obj = hl_alloc_dynobj();

    // Set integer field
    hl_dyn_seti(obj, hl_hash_utf8("width"), &hlt_i32, width);
    hl_dyn_seti(obj, hl_hash_utf8("height"), &hlt_i32, height);

    // Set bytes field
    vbyte *data = hl_alloc_bytes(width * height * 4);
    hl_dyn_setp(obj, hl_hash_utf8("data"), &hlt_bytes, data);

    return (vdynamic*)obj;
}
```

### 5. String Conversion Pattern

```c
// C string → HL string
const char *c_str = "Hello";
uchar *hl_str = hl_from_utf8(c_str);

// HL string → C string
uchar *hl_str = /* from HL */;
char *c_str = hl_to_utf8(hl_str);

// Dynamic value → C string
vdynamic *dyn = /* from HL */;
uchar *hl_str = hl_to_string(dyn);
char *c_str = hl_to_utf8(hl_str);
```

### 6. Array Access Pattern

```c
// Get array
varray *arr = (varray*)dyn->v.ptr;

// Access elements
for (int i = 0; i < arr->size; i++) {
    void *element = ((void**)arr)[i + 2];  // +2 to skip header
    // Or use type-specific access
    int value = ((int*)arr)[i + 2];
}

// Create array
varray *arr = hl_alloc_array(&hlt_i32, 100);
int *elements = (int*)arr + 2;
for (int i = 0; i < 100; i++) {
    elements[i] = i;
}
```

---

## Known Issues & Gotchas

### 1. Entry Point is Mandatory

**Issue**: Cannot skip calling entry point
**Why**: Sets up global state and static initializers
**Solution**: Always call entry point, even if empty

```c
// Even with no Haxe Main.main(), call it!
hl_call0(void, entry_closure);
```

### 2. Global Values are NULL Before Entry

**Issue #253**: Accessing `global_value` before entry point returns NULL
**Solution**: Call entry point first, then access globals

```c
// WRONG
vdynamic *global = *(vdynamic**)type->obj->global_value;  // NULL!

// RIGHT
hl_call0(void, entry_closure);  // Initialize
vdynamic *global = *(vdynamic**)type->obj->global_value;  // Now valid
```

### 3. Stack Top Must Remain Valid

**Issue #752**: Plugin initialization stack variables go out of scope
**Why**: GC scans stack using provided pointer
**Solution**: Use persistent stack variable from host application

```c
// BAD - stack variable dies after init returns
void plugin_init() {
    void *stack_top = NULL;
    hl_register_thread(&stack_top);  // ⚠️ Dangling pointer!
}

// GOOD - persistent storage
static void *g_stack_top = NULL;
void plugin_init() {
    hl_register_thread(&g_stack_top);
}
```

### 4. Constructor Type Mismatch

**Issue #253**: Constructor expects global type, not class type
**Workaround**: Allocate both instances, swap in correct one

```c
vdynamic *inst_class = hl_alloc_obj(class_type);
vdynamic *inst_global = hl_alloc_obj(global->t);
hl_dyn_call_constructor(constructor, inst_class, NULL, 0);
// Use inst_class (not inst_global)
```

### 5. String Encoding

**Gotcha**: HL strings are UTF-16, not UTF-8!

```c
// WRONG - treating as UTF-8
char *str = (char*)hl_string_ptr;  // ⚠️ Garbage!

// RIGHT - convert to UTF-8
char *str = hl_to_utf8(hl_string_ptr);
```

### 6. Closure Lifetime

**Gotcha**: Closures passed to C can be GC'd
**Solution**: Add GC root if storing

```c
vclosure *stored = NULL;

void store_closure(vclosure *c) {
    stored = c;
    hl_add_root(&stored);  // Prevent GC
}

void clear_closure() {
    hl_remove_root(&stored);
    stored = NULL;
}
```

### 7. Thread Blocking

**Gotcha**: Forgetting to call `hl_blocking()` causes GC issues
**When**: Calling external blocking code from HL thread

```c
// WRONG
external_blocking_call();  // GC might run, doesn't know we're blocked

// RIGHT
hl_blocking(true);
external_blocking_call();
hl_blocking(false);
```

### 8. Hot Reload Limitations

**From HL docs**:
- ⚠️ Cannot add/remove/reorder class fields
- ⚠️ HL/JIT only (not HL/C)
- ⚠️ Experimental (may have bugs)
- ✓ Function body changes work
- ✓ New functions work

### 9. Field Access by Name

**Gotcha**: Field names must be hashed, not used directly

```c
// WRONG
hl_dyn_geti(obj, "myField", &hlt_i32);  // ⚠️ Won't compile

// RIGHT
int hash = hl_hash_utf8("myField");
hl_dyn_geti(obj, hash, &hlt_i32);
```

### 10. Multiple VMs Not Officially Supported

**Status**: HashLink uses global state internally
**Workaround**: Possible with careful setup, but not recommended
**Our Approach**: Single VM per process (simpler, safer)

---

## Code Examples

### Complete Embedding Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <hl.h>

typedef struct {
    hl_code *code;
    hl_module *module;
    vclosure *entry;
    bool initialized;
} my_vm;

my_vm* create_vm() {
    my_vm *vm = calloc(1, sizeof(my_vm));

    // Global init (once per process)
    hl_global_init();

    // Register main thread
    void *stack_top = NULL;
    hl_register_thread(&stack_top);

    vm->initialized = true;
    return vm;
}

bool load_bytecode(my_vm *vm, const char *path) {
    // Read file
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char *data = malloc(size);
    fread(data, 1, size, fp);
    fclose(fp);

    // Parse bytecode
    if (!hl_code_read(data, size, &vm->code)) {
        free(data);
        return false;
    }
    free(data);

    // Allocate module
    vm->module = hl_module_alloc(vm->code);
    if (!vm->module) return false;

    // Initialize module (JIT compile)
    if (!hl_module_init(vm->module, NULL, NULL)) return false;

    // Build entry closure
    int ep = vm->code->entrypoint;
    int pf = vm->module->functions_indexes[ep];
    hl_type *tp = vm->code->functions[ep].type;
    void *fptr = vm->module->functions_ptrs[pf];
    vm->entry = hl_alloc_closure_void(tp, fptr);

    return vm->entry != NULL;
}

bool call_entry(my_vm *vm, int argc, char **argv) {
    if (!vm->entry) return false;

    // Init sys with args
    hl_sys_init((void*)argv, argc, NULL);

    // Call entry point
    hl_call0(void, vm->entry);

    return true;
}

void* find_function(my_vm *vm, const char *name, int nargs) {
    int hash = hl_hash_utf8(name);

    for (int i = 0; i < vm->code->nfunctions; i++) {
        hl_function *f = &vm->code->functions[i];
        if (f->name == hash && f->nargs == nargs) {
            int idx = vm->module->functions_indexes[i];
            return vm->module->functions_ptrs[idx];
        }
    }

    return NULL;
}

void destroy_vm(my_vm *vm) {
    if (!vm) return;

    if (vm->initialized) {
        hl_global_free();
    }

    free(vm);
}

// Usage
int main(int argc, char **argv) {
    my_vm *vm = create_vm();

    if (!load_bytecode(vm, "game.hl")) {
        fprintf(stderr, "Failed to load bytecode\n");
        return 1;
    }

    if (!call_entry(vm, argc, argv)) {
        fprintf(stderr, "Failed to call entry\n");
        return 1;
    }

    // Find and call static method
    void *update_func = find_function(vm, "Game.update", 0);
    if (update_func) {
        hl_call0(void, (vclosure*)update_func);
    }

    destroy_vm(vm);
    return 0;
}
```

### Native Extension Example

```c
// mymath.c
#define HL_NAME(n) mymath_##n
#include <hl.h>
#include <math.h>

// Add two numbers
HL_PRIM double HL_NAME(add)(double a, double b) {
    return a + b;
}

// Compute distance
HL_PRIM double HL_NAME(distance)(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx*dx + dy*dy);
}

// Create a point object
HL_PRIM vdynamic* HL_NAME(make_point)(double x, double y) {
    vdynobj *obj = hl_alloc_dynobj();
    hl_dyn_setd(obj, hl_hash_utf8("x"), x);
    hl_dyn_setd(obj, hl_hash_utf8("y"), y);
    return (vdynamic*)obj;
}

// Register functions
DEFINE_PRIM(_F64, add, _F64 _F64);
DEFINE_PRIM(_F64, distance, _F64 _F64 _F64 _F64);
DEFINE_PRIM(_DYN, make_point, _F64 _F64);
```

```haxe
// MyMath.hx
@:hlNative("mymath")
class MyMath {
    public static function add(a:Float, b:Float):Float {
        return 0;
    }

    public static function distance(x1:Float, y1:Float, x2:Float, y2:Float):Float {
        return 0;
    }

    public static function makePoint(x:Float, y:Float):Dynamic {
        return null;
    }
}

class Main {
    static function main() {
        var sum = MyMath.add(5, 3);
        trace('5 + 3 = $sum');

        var dist = MyMath.distance(0, 0, 3, 4);
        trace('Distance: $dist');

        var pt = MyMath.makePoint(10, 20);
        trace('Point: ${pt.x}, ${pt.y}');
    }
}
```

---

## Key Takeaways for HLFFI v3.0

### 1. Architecture Decisions

✅ **Header-only + compiled** - Matches our plan
✅ **C core + C++ wrappers** - Best of both worlds
✅ **GC-integrated values** - No manual memory management needed
✅ **Return code errors** - Industry standard, works everywhere

### 2. Critical Implementation Details

**Must-Have**:
- Entry point calling in lifecycle (Phase 1)
- GC root management for stored values (Phase 4)
- Field name hashing for object access (Phase 4)
- Thread registration helpers (Phase 1)
- UTF-16 ↔ UTF-8 string conversion (Phase 3)

**Performance**:
- Fast path (hl_call0-4) for known arity
- Slow path (hl_dyn_call) for runtime arity
- Type lookup caching critical (Phase 7)

**Safety**:
- Stack top must be persistent
- Blocking/unblocking for external calls
- Exception-safe calling variants

### 3. API Design Insights

**From `hl.h` Analysis**:
- Clean separation: allocation / calling / conversion
- Macro-based fast paths
- Type-safe wrappers for dynamic ops
- Consistent naming: `hl_<category>_<action>`

**From Community Issues**:
- Need helper for class instance creation (not trivial)
- Need helper for method lookup (hashing not obvious)
- Need clear documentation of gotchas

### 4. Missing Pieces in Current Landscape

**What doesn't exist but should**:
1. **High-level class API** - Current approach is too low-level
2. **Automatic root management** - Users forget hl_add_root
3. **Type-safe wrappers** - Too much manual coercion
4. **Restart support** - No clean way to reload bytecode
5. **Multi-module support** - Plugin system incomplete

**HLFFI v3.0 can fill these gaps!**

### 5. Phase Priority Adjustments

Based on research:

**Phase 1 (Lifecycle)**: Add restart support ✅ Already planned
**Phase 2 (Type System)**: More critical than expected (needed for Phase 3)
**Phase 3 (Static)**: Need automatic root management
**Phase 4 (Instance)**: Most complex - needs careful API design
**Phase 9 (Plugins)**: Experimental but feasible

### 6. Testing Strategy

**Must test**:
- Entry point edge cases
- Global value access timing
- GC stress testing (roots, collection cycles)
- Thread registration/unregistration
- String encoding conversions
- Hot reload failure modes

### 7. Documentation Priorities

**Critical docs**:
- Gotcha list (from this document)
- Migration from direct HL API
- Performance best practices
- Threading guide
- Memory model explanation

### 8. Reference Implementations to Study

1. **`src/main.c`** - Canonical embedding example
2. **`libs/fmt/fmt.c`** - Native extension pattern
3. **`libs/ui/ui_win.c`** - Callback storage pattern
4. **Issue #253** - Class instantiation workaround
5. **Issue #752** - Plugin threading pattern

### 9. Bytecode Analysis Tools

**Purpose for FFI Development**: Understanding bytecode structure helps design better FFI APIs, enables runtime code generation, and provides debugging insights.

#### hlbc (Rust) - Most Comprehensive
- **GitHub**: [Gui-Yom/hlbc](https://github.com/Gui-Yom/hlbc)
- **Language**: Rust
- **License**: MIT
- **Features**:
  - Complete bytecode disassembler
  - Decompiler (partial - loops incomplete)
  - Visual GUI for bytecode exploration
  - Bytecode assembler/reassembler
  - Indexing and search capabilities
- **Architecture**: Arena pattern for graph management
- **Use Cases**: Analyzing Northgard, Dune: Spice Wars, Wartales
- **FFI Insight**: Potential C API mentioned for future development
- **Structure**:
  ```
  crates/hlbc/        - Core library
  crates/cli/         - Command-line tools
  crates/decompiler/  - Decompilation engine
  crates/gui/         - Visual explorer
  crates/indexing/    - Search/indexing
  ```

#### dashlink (Haxe) - Programmatic Manipulation
- **GitHub**: [steviegt6/dashlink](https://github.com/steviegt6/dashlink)
- **Language**: Haxe (100%)
- **License**: MIT
- **Features**:
  - Disassembler, assembler, inspector
  - Create bytecode from scratch
  - Dynamic code rewriting at runtime
  - Programmatic bytecode creation
- **Based On**: Adapted from hlbc (Rust)
- **Use Cases**:
  - Reverse-engineering Haxe apps
  - Runtime code modification
  - Bootstrapping external assemblies
- **FFI Insight**: Enables dynamic binding generation, runtime code manipulation
- **Key Advantage**: Pure Haxe - integrates directly with HL ecosystem

#### crashlink (Python) - Modding Tool
- **GitHub**: [N3rdL0rd/crashlink](https://github.com/N3rdL0rd/crashlink)
- **Language**: Pure Python
- **License**: Not specified
- **Features**:
  - Zero dependencies
  - IDAPython compatible
  - Scriptable interface for value modification
  - Control flow graph generation (requires Graphviz)
  - Reserialize modified bytecode
- **CLI**: hlbc-compatible mode
- **Status**: Active development, breaking changes possible
- **Installation**: pip install crashlink
- **FFI Insight**: Python integration for bytecode analysis
- **Key Advantage**: Pure Python, works everywhere

#### Comparison Matrix

| Tool | Language | Completeness | GUI | Reassembly | FFI Relevance |
|------|----------|--------------|-----|------------|---------------|
| **hlbc** | Rust | High | ✅ Yes | ✅ Yes | Future C API |
| **dashlink** | Haxe | Medium | ❌ No | ✅ Yes | Direct HL integration |
| **crashlink** | Python | Medium | ❌ No | ✅ Yes | Python scripting |

**Recommendation for HLFFI v3.0**:
- Study **hlbc** source for bytecode structure understanding
- Reference **dashlink** for Haxe-native bytecode manipulation patterns
- Use **crashlink** for quick Python-based analysis during development

---

## Sources & References

**Complete Source List** - All resources used in this research document

---

### 1. Official HashLink Documentation

#### Core Documentation
- [HashLink Homepage](https://hashlink.haxe.org/)
- [HashLink GitHub Repository](https://github.com/HaxeFoundation/hashlink)
- [Embedding HashLink](https://github.com/HaxeFoundation/hashlink/wiki/Embedding-HashLink)
- [C API Documentation](https://github.com/HaxeFoundation/hashlink/wiki/C-API-Documentation)
- [Native Extension Tutorial](https://github.com/HaxeFoundation/hashlink/wiki/HashLink-native-extension-tutorial)

#### Advanced Topics
- [Hot Reload Wiki](https://github.com/HaxeFoundation/hashlink/wiki/Hot-Reload)
- [Notes on Garbage Collector](https://github.com/HaxeFoundation/hashlink/wiki/Notes-on-Garbage-Collector)

#### Blog Posts & Deep Dives
- [HashLink GC Redesign](https://haxe.org/blog/hashlink-gc/) - Immix GC implementation
- [HashLink In Depth - Part 1](https://haxe.org/blog/hashlink-indepth/) - VM architecture
- [HashLink In Depth - Part 2](https://haxe.org/blog/hashlink-in-depth-p2/) - Type system & JIT

#### Haxe Manual
- [Getting Started with HashLink](https://haxe.org/manual/target-hl-getting-started.html)
- [HashLink/C Compilation](https://haxe.org/manual/target-hl-c-compilation.html)

---

### 2. GitHub Issues (Critical for FFI Development)

#### Embedding & FFI
- [Issue #253](https://github.com/HaxeFoundation/hashlink/issues/253) - **Initializing classes and calling functions from C++** ⭐
  - Constructor type mismatch workaround
  - Global vs class type handling
  - Method lookup via field hashing
- [Issue #752](https://github.com/HaxeFoundation/hashlink/issues/752) - **Embedding without stack_top access** ⭐
  - Plugin/dynamic library threading
  - Stack pointer persistence requirement
  - Blocking/unblocking patterns
- [Issue #33](https://github.com/HaxeFoundation/hashlink/issues/33) - Plans for embedding API
- [Issue #46](https://github.com/HaxeFoundation/hashlink/issues/46) - How to call Haxe function from native code

#### Runtime & Modules
- [Issue #179](https://github.com/HaxeFoundation/hashlink/issues/179) - **Runtime bytecode loading** ⭐
  - Multiple JIT modules
  - Type sharing between modules
  - Plugin system foundation
- [Issue #492](https://github.com/HaxeFoundation/hashlink/issues/492) - Building library for embedding & FFI

#### Memory & Types
- [Issue #161](https://github.com/HaxeFoundation/hashlink/issues/161) - hl_alloc_dynobj usage
- [Issue #145](https://github.com/HaxeFoundation/hashlink/issues/145) - JIT Hot Reload implementation

---

### 3. HashLink Source Code (Primary Reference)

#### Core C API Headers
- [src/hl.h](https://github.com/HaxeFoundation/hashlink/blob/master/src/hl.h) - **Complete C API** ⭐
- [src/hlmodule.h](https://github.com/HaxeFoundation/hashlink/blob/master/src/hlmodule.h) - Module definitions
- [src/gc.h](https://github.com/HaxeFoundation/hashlink/blob/master/src/gc.h) - GC internals

#### Implementation Examples
- [src/main.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/main.c) - **Canonical embedding example** ⭐
- [src/alloc.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/alloc.c) - GC implementation
- [src/code.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/code.c) - Bytecode loading
- [src/module.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/module.c) - Module initialization
- [src/jit.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/jit.c) - JIT compiler

#### Standard Library Native Extensions
- [libs/fmt/fmt.c](https://github.com/HaxeFoundation/hashlink/blob/master/libs/fmt/fmt.c) - **Format FFI example** ⭐
- [libs/ui/ui_win.c](https://github.com/HaxeFoundation/hashlink/blob/master/libs/ui/ui_win.c) - Windows UI callbacks
- [libs/uv/uv.c](https://github.com/HaxeFoundation/hashlink/blob/master/libs/uv/uv.c) - libuv integration
- [src/std/string.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/std/string.c) - String utilities
- [src/std/fun.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/std/fun.c) - Function utilities

---

### 4. Community Forums & Discussions

#### Google Groups (Haxe Lang)
- [Unofficial HashLink Thread](https://groups.google.com/g/haxelang/c/5Jk_efynbro) - FFI examples in fmt.c
- [Haxe FFI Library Discussion](https://groups.google.com/g/haxelang/c/kdbSdacKpiM)
- [HXCPP: Passing function pointer to C++](https://groups.google.com/g/haxelang/c/mdcWGaUojfs)
- [Use hxcpp to build library](https://groups.google.com/g/haxelang/c/Rs8aJUPql0w)
- [HashLink GUI & dynamic loading](https://groups.google.com/g/haxelang/c/7A8lTpPN3Xs)
- [Compiling HashLink on OSX](https://groups.google.com/g/haxelang/c/RO7vq8nwd-8)
- [CFFI example problems](https://groups.google.com/g/haxelang/c/KufDsP11BYg)
- [HashLink bytecode portability](https://groups.google.com/g/haxelang/c/frV0fLixvm8)

#### Haxe Community
- [Haxe Community Forum](https://community.haxe.org/)
- [Hashlink debugger in VS Code](https://community.openfl.org/t/hashlink-debugger-in-vs-code-asset-libraries-not-loading/14354)

---

### 5. Tutorials & Blog Posts

#### Technical Tutorials
- [Working with C (Aramallo Blog)](https://aramallo.com/blog/hashlink/calling-c.html) - Practical C integration
- [How to compile HL/C (Gist)](https://gist.github.com/Yanrishatum/d69ed72e368e35b18cbfca726d81279a) - Compilation guide
- [Khabind (Gist)](https://gist.github.com/zicklag/6c2828cda4e8282c850d97ac8a3a0de5) - Binding C++ to Kha/HashLink

#### Framework Documentation
- [Hello HashLink (Heaps.io)](https://heaps.io/documentation/hello-hashlink.html)
- [Heaps Wiki: Hello HashLink](https://github.com/HeapsIO/heaps/wiki/Hello-HashLink)

---

### 6. Projects Using HashLink FFI

#### Game Engines & Frameworks
- [MECore Game Engine](https://github.com/datee/MECore) ⭐
  - C++ engine + Haxe scripting
  - Vulkan 1.3 + Jolt Physics
  - 75.7% C++, 22.7% Haxe
  - Educational example of HL/C integration
- [Heaps.io](https://heaps.io/) - Cross-platform game framework
- [Kha](https://github.com/Kode/Kha) - Multi-platform game engine

#### Game Titles (Using HashLink)
- **Northgard** (Shiro Games)
- **Dune: Spice Wars** (Shiro Games)
- **Wartales** (Shiro Games)
- **Dead Cells** (Motion Twin)
- **Evoland** series (Shiro Games)

---

### 7. Bytecode Analysis & Development Tools

#### Disassemblers/Decompilers
- [hlbc](https://github.com/Gui-Yom/hlbc) - **Rust, most comprehensive** ⭐
  - Complete disassembler, decompiler, assembler
  - GUI bytecode explorer
  - Indexing and search
  - Used for game analysis (Northgard, etc.)
- [dashlink](https://github.com/steviegt6/dashlink) - **Haxe, programmatic** ⭐
  - Pure Haxe implementation
  - Create bytecode from scratch
  - Runtime code modification
  - Based on hlbc
- [crashlink](https://github.com/N3rdL0rd/crashlink) - **Python, modding**
  - Pure Python, zero dependencies
  - IDAPython compatible
  - CFG generation (Graphviz)
  - pip install crashlink

#### Other Tools
- [Callfunc](https://lib.haxe.org/p/callfunc/) - FFI library using libffi
- [haxe-sys](https://github.com/Aurel300/haxe-sys) - Asynchronous system API

---

### 8. Related FFI Technologies

#### libffi
- [libffi GitHub](https://github.com/libffi/libffi)
- [libffi docs](https://sourceware.org/libffi/)

#### Language Bindings
- [Ruby FFI](https://github.com/ffi/ffi) - Reference FFI implementation
- [Python ctypes](https://docs.python.org/3/library/ctypes.html)

---

### 9. Academic & Research

#### Papers & Theses
- [Modern garbage collector for HashLink](https://www.imperial.ac.uk/media/imperial-college/faculty-of-engineering/computing/public/1920-ug-projects/Aurel-B%C3%ADl%C3%BD.pdf) - Imperial College thesis on HL GC

---

### 10. Platform-Specific Resources

#### Windows
- [Visual Studio HashLink Setup](https://github.com/HaxeFoundation/hashlink/wiki/HashLink-native-extension-tutorial#visual-studio-setup)

#### Web/WASM
- [OpenFL devlog: HashLink/C compilation](https://joshblog.net/2024/openfl-devlog-hashlink-c-compilation/)
- [WebAssembly support discussion](https://groups.google.com/g/haxelang/c/Pcm38LPFjW0)

---

### 11. Releases & Changelogs

- [HashLink Releases](https://github.com/HaxeFoundation/hashlink/releases)
- [Latest: HashLink 1.14](https://github.com/HaxeFoundation/hashlink/releases/latest)
- [HashLink Commits](https://github.com/HaxeFoundation/hashlink/commits/master)

---

### 11.5. Recent Commits (2024-2025) - FFI Relevant

**Source**: [HashLink master commits](https://github.com/HaxeFoundation/hashlink/commits/master)

#### 🔌 Plugin System (NEW!)
- **Nov 1, 2025**: "added plugin system runtime support" ⭐
  - **Major addition**: Runtime plugin infrastructure
  - Enables dynamic module loading at runtime
  - Foundation for Phase 9 (Plugin/Module System)
- **Nov 1, 2025**: "perform structural check when resolving plugin types for compatibility"
  - Type safety for plugin loading
  - Addresses type sharing challenges mentioned in Issue #179

#### 🔍 API Enhancements
- **Nov 10, 2025**: "added value_address and type_data_size"
  - New introspection capabilities
  - Useful for FFI debugging and analysis
- **Oct 26, 2025**: "added missing ToSFloat from i64"
  - Extended numeric conversions
  - Important for i64 ↔ float FFI bridges
- **Oct 26, 2025**: "added bsort_i64"
  - 64-bit sorting support
  - Array operations enhancement

#### ⚡ Threading & Performance
- **Oct 17, 2025**: "reduce sleep to 1ms (faster wakeup)"
  - Thread scheduling optimization
  - Better responsiveness for embedded use cases
- **Oct 11, 2025**: "handle EINTR on select() and use SA_RESTART for profiling signals"
  - Signal handling reliability (Unix)
  - Critical for robust production FFI
- **Nov 8, 2025**: "improve windows sys_sleep precision"
  - Windows-specific sleep accuracy
  - Important for Phase 1 (Windows first target)

#### 💾 Memory Optimization
- **Nov 10, 2025**: "optimize allocation of small null<int>"
  - GC allocation overhead reduction
  - Performance improvement for nullable types

#### 🔧 Module Loading Extensions
- **Nov 5, 2025**: "added uv create_loop"
  - libuv integration expansion
  - Async I/O for embedded scenarios
- **Nov 9, 2025**: "[uv] Add wrappers for all libuv functions"
  - Complete libuv API exposure
  - Event loop integration patterns

**Key Insights for HLFFI v3.0**:
1. **Plugin system is NOW available** (Nov 2025) - Phase 9 timing is perfect!
2. **Type checking for plugins** - Addresses major concern from research
3. **Introspection APIs** - `value_address` and `type_data_size` useful for debugging FFI
4. **Windows optimizations** - Aligns with our Windows-first target
5. **Threading improvements** - Makes embedded use cases more robust

---

### 12. Index by Topic

#### 🔥 Critical Reading (Must Read Before Implementation)
1. [src/main.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/main.c) - Embedding pattern
2. [src/hl.h](https://github.com/HaxeFoundation/hashlink/blob/master/src/hl.h) - Complete API
3. [Issue #253](https://github.com/HaxeFoundation/hashlink/issues/253) - Class instantiation
4. [Issue #752](https://github.com/HaxeFoundation/hashlink/issues/752) - Threading gotcha
5. [C API Documentation](https://github.com/HaxeFoundation/hashlink/wiki/C-API-Documentation) - Official API docs

#### 🧠 Deep Understanding (Recommended)
- [HashLink In Depth - Part 2](https://haxe.org/blog/hashlink-in-depth-p2/) - Type system
- [GC Redesign Blog](https://haxe.org/blog/hashlink-gc/) - Memory model
- [GC Notes Wiki](https://github.com/HaxeFoundation/hashlink/wiki/Notes-on-Garbage-Collector) - GC internals
- [Native Extension Tutorial](https://github.com/HaxeFoundation/hashlink/wiki/HashLink-native-extension-tutorial) - FFI patterns

#### 🔧 Practical Implementation
- [libs/fmt/fmt.c](https://github.com/HaxeFoundation/hashlink/blob/master/libs/fmt/fmt.c) - FFI example
- [MECore](https://github.com/datee/MECore) - Real-world usage
- [Working with C (Aramallo)](https://aramallo.com/blog/hashlink/calling-c.html) - Tutorial

#### 🐛 Troubleshooting
- [Issue #253](https://github.com/HaxeFoundation/hashlink/issues/253) - Constructor issues
- [Issue #752](https://github.com/HaxeFoundation/hashlink/issues/752) - Thread registration
- [Issue #161](https://github.com/HaxeFoundation/hashlink/issues/161) - Dynamic objects

---

**Document Version**: 1.1
**Last Updated**: 2025-11-22
**Next Review**: Before Phase 0 implementation
**Sources Count**: 75+ references across 12 categories
