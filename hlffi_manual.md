# hlffi — Developer Manual

*Version 2.2‑beta  ·  HashLink FFI single‑header*

---

## Table of Contents

1. [Introduction](#introduction)
2. [Design Philosophy](#design-philosophy)
3. [Getting Started](#getting-started)
4. [Feature Flags](#feature-flags)
5. [Error‑Handling Model](#error-handling-model)
6. [Lifecycle API](#lifecycle-api)
7. [Low‑Level Call API](#low-level-call-api)
8. [High‑Level Static Helpers](#high-level-static-helpers)
9. [Static Variable Access](#static-variable-access)
10. [Boxed Value Layer](#boxed-value-layer)
11. [Threading & Garbage Collection](#threading--garbage-collection)
12. [Unreal Engine Integration](#unreal-engine-integration)
13. [Memory‑Management Rules](#memory-management-rules)
14. [Performance Notes](#performance-notes)
15. [Building & Linking](#building--linking)
16. [Troubleshooting](#troubleshooting)
17. [Changelog](#changelog)
18. [License](#license)

---

## 1  Introduction

`hlffi` is a single‑header bridge that embeds the HashLink\*\*™\*\* VM inside any C or C++ host—console apps, game engines, plugins, scripting shells, unit‑test harnesses. It distills the entire HashLink public C API into a compact, ergonomic set of helpers while keeping the hot paths *as fast as hand‑written calls*.

Key goals:

* **Zero friction.** One header, one `#define`, compile.
* **Full feature coverage.** Every VM action—initialise, load byte‑code, execute, poke variables, collect GC—has a wrapper.
* **Opt‑in weight.** Keep it tiny by flipping `HLFFI_EXT_*` flags.
* **Cross‑toolchain.** GCC, Clang, MSVC, mingw, emscripten.

---

## 2  Design Philosophy

| Principle                          | Implementation                                                                                                                                |
| ---------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| *Tiny binary by default*           | All helpers are `static inline` when the compiler supports it. No CRT heap allocations on the call fast‑path.                                 |
| *Don’t hide HashLink*              | The real `hl.h` symbols are still fully usable side‑by‑side. `hlffi` merely layers convenience.                                               |
| *Fail loud in dev, silent in prod* | Enum error codes & detailed text in debug builds, and optional `*_bool` shims for shipping.                                                   |
| *No global state*                  | Everything lives on the `hlffi_vm` handle. Multiple VMs per process are supported (HashLink itself is global, but the header won’t stop you). |

---

## 3  Getting Started

```cpp
#define HLFFI_IMPLEMENTATION  // one TU only
#include "hlffi.h"

int main(int argc,char** argv){
    hlffi_vm* vm = hlffi_create();
    hlffi_init_bool(vm,"main.hl",argc,argv);
    hlffi_load_file_bool(vm,"game.hl",nullptr);
    hlffi_call_entry_bool(vm);
    hlffi_close(vm);
    hlffi_destroy(vm);
}
```

Minimal CMake snippet:

```cmake
add_executable(demo main.cpp)
find_library(HASHLINK hl PATHS /usr/lib /usr/local/lib)
if(NOT HASHLINK)
  message(FATAL_ERROR "HashLink library not found")
endif()
target_link_libraries(demo PRIVATE ${HASHLINK})
```

---

## 4  Feature Flags

All flags default to **1** (enabled) except Unreal.

| Flag               | Effect                  | Typical reason to turn off                      |
| ------------------ | ----------------------- | ----------------------------------------------- |
| `HLFFI_EXT_STATIC` | static‑method helpers   | You never need `Class.method()` convenience.    |
| `HLFFI_EXT_VAR`    | static variable Get/Set | Script interacts only via function calls.       |
| `HLFFI_EXT_VALUE`  | boxed variant helpers   | You expose raw `vdynamic*` to the host instead. |
| `HLFFI_EXT_ERRORS` | enum error codes        | You prefer raw HashLink error strings.          |
| `HLFFI_EXT_UNREAL` | UE5 log macros          | Not targeting Unreal.                           |

Example—compile a **micro‑build** for an embedded micro‑controller:

```cpp
#define HLFFI_EXT_STATIC 0
#define HLFFI_EXT_VAR    0
#define HLFFI_EXT_VALUE  0
#define HLFFI_EXT_ERRORS 0
#include "hlffi.h"
```

---

## 5  Error Handling Model

### 5.1 Enum Path (default)

```c
hlffi_result r = hlffi_load_file(vm, "bad.hl");
if(r != HLFFI_OK) {
    fprintf(stderr, "hlffi error %d: %s\n", r, hlffi_error(vm));
    exit(1);
}
```

* All public functions that can fail return a **negative enum value**.
* `hlffi_error(vm)` returns a human‑readable message (thread‑safe inside the VM instance).

### 5.2 Bool Shims

Every enum function has a `*_bool` inline that converts the return code to `true/false` and (optionally) fills a caller‑supplied `const char** errmsg`.

```c
const char* err;
if(!hlffi_load_file_bool(vm,"bad.hl",&err))
    printf("oops: %s\n", err);
```

---

## 6  Lifecycle API

| Function              | Description                                                                                                                                |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| `hlffi_create()`      | Allocate zero‑initialised VM handle (does **not** touch HL).                                                                               |
| `hlffi_init*()`       | One‑time HashLink global init, registers the main thread. Pass `main.hl` path **even if you never load it**, this satisfies HL path logic. |
| `hlffi_load_file*()`  | Read on-disk compiled byte‑code (`*.hl`) into memory and prepare the module.                                                               |
| `hlffi_load_mem*()`   | Same, but from a `void*` buffer (for zip‑packed games).                                                                                    |
| `hlffi_call_entry*()` | Invoke `Main.main()` or the custom entry defined at compile time.                                                                          |
| `hlffi_close()`       | Unregister thread, free HL globals.                                                                                                        |
| `hlffi_destroy()`     | Release the wrapper handle. Must be last.                                                                                                  |
| `hlffi_ready()`       | Returns `true` when `hlffi_load_*` finished successfully.                                                                                  |

### 6.1 Thread‑Safety

* All lifecycle functions must be called from the **same OS thread** that ran `hlffi_init` unless you wrap them in `hlffi_thread_attach/detach` (see §11).

---

## 7  Low-Level Call API

### 7.1 Lookup

```c
void* fun = hlffi_lookup(vm, "My.modFn", /*nargs=*/2);
```

* Performs a **hash lookup** inside the loaded module.
* Returns a `vclosure*` \*cast to void\*\*\*—store it and reuse, hashing is O(1) but caching is cheaper.

### 7.2 Call Macros

| Wrapper            | Equivalent HL call |
| ------------------ | ------------------ |
| `hlffi_call0(f)`   | `hl_call0(f)`      |
| `hlffi_call1(f,a)` | `hl_call1(f,a)`    |
| … up to 4          | …                  |

All wrappers inline directly to the HL C API with a narrow cast, **so they are as fast as calling HL yourself**.

### 7.3 Variadic

```c
hlffi_call_variadic(f, 3, arg0, arg1, arg2);
```

Internally allocates a small array on the stack with `alloca` and calls `hl_callN`.

---

## 8  High‑Level Static Helpers

If your Haxe code organises logic in **static classes** (Unity style), these wrappers remove the plumbing.

```cpp
// void Game.start(int w,int h);
hlffi_call_static_int(vm,"Game","start",1920);

int score = hlffi_call_static_ret_int(vm,"Game","score");
```

Return helper guarantees type safety—the result is **0** if the HL dynamic type mismatches.

---

## 9  Static Variable Access

Get or set any compile‑time `static var` in a Haxe class.

```c
double fps = hlffi_get_static_float(vm,"Engine","frameRate");
hlffi_set_static_float(vm,"Engine","frameRate", 144.0);
```

Remember that **HashLink strings are UTF‑16**—wrappers transparently convert between UTF‑8 (C side) and HL string.

---

## 10  Boxed Value Layer

`hlffi_value` is a tiny tagged union for hosts that don’t want to expose `vdynamic*` at their API boundary.

```c
hlffi_value* v = hlffi_val_string("hello");
print( hlffi_val_as_int(v) );   // 0 (fallback)
hlffi_val_free(v);
```

| Function                   | Allocates? | Notes                                                                                  |
| -------------------------- | ---------- | -------------------------------------------------------------------------------------- |
| `hlffi_val_int/float/bool` | yes        | Allocates both C wrapper **and** HL dynamic.                                           |
| `hlffi_val_string`         | yes        | Duplicates UTF‑8 into C heap; HL string lives in HL heap.                              |
| `hlffi_val_null`           | yes        | Kind = `HLFFI_T_NULL`, `dyn == NULL`.                                                  |
| `hlffi_val_free`           | yes        | Frees *only* the C struct (and its duplicate UTF‑8, if any). HL dynamic is GC‑managed. |

---

## 11  Threading & Garbage Collection

### 11.1 Multi‑Thread Basics

HashLink requires **each OS thread** that touches VM data to be registered:

```c
void worker(){
    hlffi_thread_attach();
    // safe HL calls...
    hlffi_thread_detach();
}
```

*Safe* means: call wrappers, read HL values, allocate HL memory. Pure C code obviously doesn’t need registration.

### 11.2 GC helpers

| Function                      | Description                                   |
| ----------------------------- | --------------------------------------------- |
| `hlffi_gc_stats(&obj,&bytes)` | Number of live objects and heap size.         |
| `hlffi_gc_collect()`          | Force a **major** collection (`hl_gc_major`). |

Use these to surface HL memory stats in your engine profiler.

---

## 12  Unreal Engine Integration

Enable at compile time:

```cpp
#define HLFFI_EXT_UNREAL 1
#include "hlffi.h"
```

Then call any static method with one macro line:

```cpp
HLFFI_UE_CALL(gVm,"Game","tick");
```

* On error the macro prints through `UE_LOG(LogTemp, Error, …)` and continues.
* `HLFFI_UE_CALL_FLOAT` takes one `double` argument.
* You can add more macro variants (int, string…) by copy‑pasting the pattern.

---

## 13  Memory Management Rules

| Return type                                              | Who frees?      | How                       |
| -------------------------------------------------------- | --------------- | ------------------------- |
| `hlffi_vm*`                                              | caller          | `hlffi_destroy()`         |
| `char*` from any `*_ret_string` or `*_get_static_string` | caller          | `free()` (uses `strdup`)  |
| `hlffi_value*`                                           | caller          | `hlffi_val_free()`        |
| `vdynamic*` in user code                                 | **HashLink GC** | keep a root or pass to HL |

Never mix CRT heaps—`hlffi` always uses the same `malloc/free` pair visible to your TU.

---

## 14  Performance Notes

| Scenario                              | Expected overhead                                                         |
| ------------------------------------- | ------------------------------------------------------------------------- |
| `hlffi_call0..4` vs direct `hl_callX` | **\~0%** (thin inline)                                                    |
| `hlffi_call_variadic`                 | \~3‑5× slower (va\_list + alloca)                                         |
| Static method helpers                 | \~40 ns extra (hash lookups) — cache class `hl_type*` manually if needed. |
| Boxed value create                    | 2‑3 allocations (C + HL). Avoid in tight loops.                           |

### Tips

* Cache `void* fun = hlffi_lookup(...)` once; call in hot loops.
* Turn off `HLFFI_EXT_ERRORS` for release to drop snprintfs.
* Prefer `hlffi_set_static_*` over reflection when updating game state every frame.

---

## 15  Building & Linking

### 15.1 Linux / macOS

```bash
g++ main.cpp -I/path/to/hashlink/include -lhl -o app
```

### 15.2 Windows (MSVC)

* Install [HashLink](https://hashlink.haxe.org/): copy `include/` & `lib/hl.lib`.
* Project → Properties → C/C++ → Additional Include Dirs → *hashlink\include*
* Linker → Input → Additional Dependencies → **hl.lib**

### 15.3 CMake Example

```cmake
add_library(hlffi INTERFACE)
add_library(HashLink::hl STATIC IMPORTED)
set_target_properties(HashLink::hl PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/thirdparty/hl.lib")

target_sources(hlffi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/hlffi.h)
target_link_libraries(hlffi INTERFACE HashLink::hl)
```

---

## 16  Troubleshooting

| Symptom                                         | Likely cause                  | Fix                                                    |
| ----------------------------------------------- | ----------------------------- | ------------------------------------------------------ |
| `HLFFI_E_INIT` on first call                    | Forgot `hlffi_init`           | Initialise once per process.                           |
| `HLFFI_E_LOAD` + msg “hl\_module\_alloc failed” | Byte‑code built with newer HL | Recompile `.hl` with your runtime version.             |
| Access violation in UE thread                   | Thread wasn’t registered      | Wrap code in `hlffi_thread_attach/detach`.             |
| Unicode garbage from strings                    | Mixing UTF‑8/UTF‑16           | Always pass UTF‑8 to API; wrapper converts internally. |

---


