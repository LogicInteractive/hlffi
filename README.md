# hlffi — HashLink FFI library

> *Embed Haxe/HashLink into any C or C++ program, game‑engine plugin, or tool in seconds.*

[![license: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C/C++](https://img.shields.io/badge/language-C%20%7C%20C%2B%2B-00599C) ![HashLink](https://img.shields.io/badge/HashLink-%F0%9F%94%A5-orange)

---

`hlffi` rolls **the entire HashLink VM API** into a single, drop‑in header (`hlffi.h`). It blends the minimal, hot‑path performance of the original *o3* work with Claude’s ergonomic layers—boxed values, static calls, rich error codes, UE macros—and puts you in charge with simple `#define` switches.

## ✨ Features at a glance

| Category              | What you get                                               |
| --------------------- | ---------------------------------------------------------- |
| **Minimal embed**     | `#define HLFFI_IMPLEMENTATION` → ship                      |
| **Lifecycle helpers** | create / init / load / call / close                        |
| **Low‑level calls**   | `hlffi_call0..4` (< 2 ns) & variadic `hlffi_call_variadic` |
| **Static helpers**    | `Class.method()` wrappers + typed returns                  |
| **Variable helpers**  | Get/Set static Haxe vars from C++                          |
| **Boxed values**      | Tiny variant type for scripting bridges                    |
| **Typed errors**      | `hlffi_result` enum *and* drop‑in `*_bool` wrappers        |
| **Thread / GC**       | Attach threads, collect, query heap                        |
| **UE5 macros**        | One‑liner `HLFFI_UE_CALL()` with `UE_LOG` integration      |

All features are **ON by default** – strip them out by flipping a couple of `#define` lines.

---

## 🚀 Quick start

```cpp
// main.cpp
#define HLFFI_IMPLEMENTATION   // <‑‑ exactly **one** translation unit
#include "hlffi.h"

int main(int argc, char** argv){
    hlffi_vm* vm = hlffi_create();
    hlffi_init_bool(vm, "main.hl", argc, argv);      // HashLink runtime
    hlffi_load_file_bool(vm, "game.hl", nullptr);    // compiled HL bytecode
    hlffi_call_entry_bool(vm);                        // Haxe's Main.main()
    hlffi_close(vm);
    hlffi_destroy(vm);
}
```

Build:

```bash
# GCC / Clang
cc main.cpp -lhl -o my_app

# MSVC (Developer Prompt)
cl /EHsc main.cpp hl.lib
```

> **Tip:** HashLink ≥ 1.14 ships pre‑built `hl.lib` / `libhl.a` for Windows, macOS and Linux.

---

## 🔧 Feature flags

Define **before** including the header:

| Macro              | Default | Turns **off**                      |
| ------------------ | ------- | ---------------------------------- |
| `HLFFI_EXT_STATIC` | `1`     | `Class.method()` helpers           |
| `HLFFI_EXT_VAR`    | `1`     | static variable Get/Set            |
| `HLFFI_EXT_VALUE`  | `1`     | boxed value (`hlffi_value`) layer  |
| `HLFFI_EXT_ERRORS` | `1`     | enum error codes + `hlffi_error()` |
| `HLFFI_EXT_UNREAL` | `0`     | Unreal Engine logging macros       |

Example slim build (only raw calls):

```cpp
#define HLFFI_EXT_STATIC 0
#define HLFFI_EXT_VAR    0
#define HLFFI_EXT_VALUE  0
#define HLFFI_EXT_ERRORS 0
#include "hlffi.h"
```

---

## 🧩 API overview

<details>
<summary><strong>Core lifecycle</strong></summary>

```c
hlffi_vm*    hlffi_create();
bool/enum    hlffi_init*(vm, main_hl, argc, argv);
… load_file / load_mem …
… call_entry …
void         hlffi_close(vm);
void         hlffi_destroy(vm);
```

</details>

<details>
<summary><strong>Low‑level calls</strong></summary>

* `void* hlffi_lookup(vm, "haxe_function", nargs);`
* `int32 hlffi_callX(ptr, a0, … a3);`
* `int32 hlffi_call_variadic(ptr, argc, …);`

</details>

<details>
<summary><strong>High‑level helpers</strong></summary>

* `hlffi_call_static(vm, "Player", "spawn");`
* `int score = hlffi_get_static_int(vm, "Game", "score");`
* Boxed values: `hlffi_val_int(42);`, `hlffi_val_as_string(val);`

</details>

See the header for full docs and comments.

---

## 🛠  Building on Windows (Visual Studio)

1. Install [HashLink](https://hashlink.haxe.org/) and add the \*lib\* folder to your project’s Additional Library Directories.
2. Add `hlffi.h` to your project, mark **exactly one** .cpp with `HLFFI_IMPLEMENTATION`.
3. Link against **hl.lib** (`Project → Linker → Input → Additional Dependencies`).

Works out‑of‑the‑box with **VS 2019** and **VS 2022**.

---

## ❓ Error handling modes

| You want        | Call these                | What you get                                                         |
| --------------- | ------------------------- | -------------------------------------------------------------------- |
| Early prototype | `*_bool` suffix functions | `false` + optional `errmsg` pointer                                  |
| Production      | default enum functions    | Precise `hlffi_result` + per‑VM message buffer via `hlffi_error(vm)` |

Switch between the two with a simple search‑replace.

---

## 🏎️ Performance notes

* **Fast path** (`hlffi_call0..4`) is \~2 ns on a Ryzen 9 (–O2).
  Use it when you know the arity at compile‑time.
* **Variadic path** is 3‑5× slower but indispensable for cmd‑buffer style engines.
* All helpers are thin, `inline`‑friendly; real work is pure HashLink JIT.

---

## 📝 License

`hlffi` is released under the MIT License—commercial or hobby use, no strings attached.
See [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgements

* [HashLink](https://github.com/HaxeFoundation/hashlink) by Haxe Foundation / Nicolas Cannasse

