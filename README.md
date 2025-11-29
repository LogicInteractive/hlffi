
<img width="379" height="229" alt="HLFFI — HashLink FFI Library" src="https://github.com/user-attachments/assets/786155ad-73d4-41e3-acfb-a6af593cee07" />

---

**Embed Haxe/HashLink into any C/C++ application**

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C/C++](https://img.shields.io/badge/language-C%20%7C%20C%2B%2B-00599C)
![HashLink](https://img.shields.io/badge/HashLink-%F0%9F%94%A5-orange)

---

## Why HLFFI?

**The Problem**: You want to use [Haxe](https://haxe.org/) for game logic, scripting, or rapid prototyping in your C/C++ application. HashLink is Haxe's high-performance runtime, but embedding it requires understanding its internal APIs, managing GC roots manually, and dealing with threading complexities.

**The Solution**: HLFFI wraps all of that complexity into a simple, clean API:

```c
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Call Haxe functions from C
    hlffi_call_static(vm, "Game", "start", 0, NULL);

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

## What HLFFI Solves

| Challenge | Without HLFFI | With HLFFI |
|-----------|---------------|------------|
| **GC Root Management** | Manual `hl_add_root()` / `hl_remove_root()` calls | Automatic - just use `hlffi_value_free()` |
| **Type Conversions** | Raw `vdynamic*` manipulation | Type-safe `hlffi_value_*` helpers |
| **Error Handling** | Check global state, decode errors | Clear error codes + `hlffi_get_error()` |
| **Threading** | Complex GC registration | Simple `hlffi_worker_register()` |
| **Event Loops** | Manual UV + MainLoop integration | One call: `hlffi_update(vm, dt)` |
| **Hot Reload** | Not easily possible | Built-in `hlffi_reload_module()` |

## Use Cases

- **Game Engines** - Add Haxe scripting to Unreal, Unity, Godot, or custom engines
- **Plugin Systems** - Let users write plugins in Haxe
- **Rapid Prototyping** - Iterate on game logic in Haxe, hot reload without restart
- **Cross-Platform** - Use Haxe's extensive standard library from C

## Key Features

- **Clean C API** - No hidden state, explicit error codes, predictable behavior
- **Two Integration Modes** - Engine-controlled loop or dedicated Haxe thread
- **Hot Reload** - Update Haxe code without restarting your application
- **Complete Type System** - Arrays, Maps, Bytes, Enums, Callbacks
- **Automatic Memory Management** - GC roots handled for you

## Quick Example

**Haxe side** (`game.hx`):
```haxe
class Game
{
    public static var score:Int = 0;

    public static function addScore(points:Int):Void
    {
        score += points;
        trace('Score: $score');
    }
}
```

**C side**:
```c
// Call Haxe function with argument
hlffi_value* points = hlffi_value_int(vm, 100);
hlffi_value* args[] = {points};
hlffi_call_static(vm, "Game", "addScore", 1, args);
hlffi_value_free(points);

// Read Haxe static variable
int score = hlffi_get_static_int(vm, "Game", "score", 0);
printf("Score: %d\n", score);
```

## Getting Started

**Prerequisites**: Haxe, HashLink 1.12+, Visual Studio or GCC, CMake

```bash
git clone https://github.com/LogicInteractive/hlffi.git
cd hlffi
mkdir build && cd build
cmake .. -DHASHLINK_DIR=C:\HashLink
cmake --build . --config Release
```

Then follow the **[User Guide](docs/guide/GUIDE_01_GETTING_STARTED.md)** to create your first program.

## Documentation

| Resource | Description |
|----------|-------------|
| **[User Guide](docs/guide/README.md)** | Step-by-step tutorials (start here!) |
| **[API Reference](docs/API_INDEX.md)** | Complete function reference |
| **[Wiki](docs/wiki/HOME.md)** | Overview and prerequisites |

### Quick Links

- [Getting Started](docs/guide/GUIDE_01_GETTING_STARTED.md) - First program
- [Calling Haxe](docs/guide/GUIDE_02_CALLING_HAXE.md) - Functions and fields
- [Data Exchange](docs/guide/GUIDE_03_DATA_EXCHANGE.md) - Arrays, maps, enums
- [Advanced Topics](docs/guide/GUIDE_04_ADVANCED.md) - Threading, hot reload, callbacks

## Design Philosophy

HLFFI is built on these principles:

1. **Simplicity** - One header, clear naming, predictable behavior
2. **Safety** - Automatic GC roots, bounds checking, clear ownership
3. **Flexibility** - Works with any architecture (engine loop or dedicated thread)
4. **Performance** - Minimal overhead, optional caching for hot paths

## Requirements

- **HashLink** 1.12+ (1.14+ recommended)
- **Haxe** 4.2+
- **C Compiler** - MSVC, GCC, or Clang
- **Platform** - Windows (primary), Linux/macOS (planned)

## License

MIT License - Same as HashLink. See [LICENSE](LICENSE).

## Acknowledgements

- [HashLink](https://github.com/HaxeFoundation/hashlink) by Nicolas Cannasse / Haxe Foundation
- [AndroidBuildTools](https://github.com/LogicInteractive/AndroidBuildTools) for production patterns

---

**HLFFI v3.0.0** | Created by [LogicInteractive](https://github.com/LogicInteractive)

