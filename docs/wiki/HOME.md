# HLFFI - HashLink Foreign Function Interface

**Embed Haxe in your C/C++ applications**

---

## What is HLFFI?

HLFFI is a C/C++ library that embeds the [HashLink](https://hashlink.github.io/) virtual machine into any application. It provides a clean, production-ready API for running [Haxe](https://haxe.org/) code from C/C++ hosts.

```c
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Call Haxe functions
    hlffi_call_static(vm, "Game", "start", 0, NULL);

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

---

## Why HLFFI?

### The Problem

You want to use Haxe for game logic, scripting, or rapid prototyping, but your engine or application is written in C/C++. HashLink is Haxe's high-performance runtime, but embedding it requires understanding its internal APIs.

### The Solution

HLFFI wraps HashLink's internals into a simple, consistent API:

- **One header file** - Just `#include "hlffi.h"`
- **Automatic memory management** - GC roots handled for you
- **Type-safe wrappers** - No need to understand HashLink's type system
- **Two integration modes** - Works with any application architecture
- **Hot reload** - Update code without restarting
- **Production ready** - Tested and documented

---

## Key Features

| Feature | Description |
|---------|-------------|
| **Simple API** | Create → Init → Load → Run → Destroy |
| **Call Haxe from C** | Static methods, instance methods, fields |
| **Call C from Haxe** | Register callbacks, Haxe calls your code |
| **Data exchange** | Primitives, strings, arrays, maps, bytes, enums |
| **Hot reload** | Reload code without restarting (HL 1.12+) |
| **Threading** | Non-threaded (engine loop) or threaded (dedicated) |
| **VM restart** | Full reset within single process (experimental) |

---

## Use Cases

### Game Engines

Embed Haxe scripting in your C++ game engine:

```c
// Every frame
void engine_tick(float dt)
{
    hlffi_update(vm, dt);
    hlffi_call_static(vm, "Game", "update", 0, NULL);
}
```

### Plugin Systems

Let users write plugins in Haxe:

```c
// Load user plugin
hlffi_load_file(vm, "user_plugin.hl");
hlffi_call_entry(vm);

// Call plugin function
hlffi_call_static(vm, "Plugin", "onLoad", 0, NULL);
```

### Prototyping

Prototype logic in Haxe, integrate into C++ later:

```haxe
// Iterate quickly in Haxe
class AI
{
    public static function think(entity:Dynamic):Dynamic
    {
        // Easy to change, hot reload
        return Decision.Attack;
    }
}
```

### Cross-Platform Libraries

Use Haxe's cross-platform libraries from C:

```c
// Use Haxe's HTTP, JSON, etc.
hlffi_call_static(vm, "Http", "request", 1, args);
```

---

## How It Works

```
┌─────────────────────────────────────────────────┐
│                 Your C/C++ App                   │
│                                                  │
│  ┌──────────────────────────────────────────┐   │
│  │                  HLFFI                    │   │
│  │                                           │   │
│  │   hlffi_call_static()                    │   │
│  │   hlffi_get_field()                      │   │
│  │   hlffi_new()                            │   │
│  │   ...                                    │   │
│  └────────────────┬─────────────────────────┘   │
│                   │                              │
│  ┌────────────────▼─────────────────────────┐   │
│  │            HashLink VM                    │   │
│  │                                           │   │
│  │   ┌───────────────────────────────┐      │   │
│  │   │      Your Haxe Code           │      │   │
│  │   │      (game.hl bytecode)       │      │   │
│  │   └───────────────────────────────┘      │   │
│  └──────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
```

1. Your C/C++ app includes HLFFI
2. HLFFI manages the HashLink VM
3. HashLink runs your compiled Haxe bytecode
4. HLFFI provides the bridge for data and function calls

---

## Quick Links

### Getting Started
- [Prerequisites](PREREQUISITES.md) - What you need to install
- [User Guide Part 1](../guide/GUIDE_01_GETTING_STARTED.md) - Your first program

### Documentation
- [User Guide](../guide/README.md) - Step-by-step tutorials
- [API Reference](../API_REFERENCE.md) - Complete API documentation

### Topics
- [VM Lifecycle](../API_01_VM_LIFECYCLE.md) - Create, init, load, destroy
- [Calling Haxe](../guide/GUIDE_02_CALLING_HAXE.md) - Functions and fields
- [Hot Reload](../API_05_HOT_RELOAD.md) - Update code without restart
- [Threading](../API_04_THREADING.md) - Multi-threaded integration

---

## Version

**HLFFI v3.0.0**

- Requires: HashLink 1.12+ (for hot reload) or 1.11+ (basic features)
- Platforms: Windows (MSVC), Linux/macOS support planned
- License: MIT (same as HashLink)

---

## Next Steps

**[→ Prerequisites](PREREQUISITES.md)** - Install what you need to get started
