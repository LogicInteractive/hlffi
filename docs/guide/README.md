# HLFFI User Guide

**A step-by-step guide to embedding Haxe in your C/C++ application**

---

## What is HLFFI?

HLFFI (HashLink Foreign Function Interface) is a C/C++ library that lets you embed the Haxe/HashLink runtime into any application. Use Haxe for game logic, scripting, or any code that benefits from Haxe's productivity, while keeping your core engine in C/C++.

---

## Guide Contents

| Part | Topic | Description |
|------|-------|-------------|
| **[1. Getting Started](GUIDE_01_GETTING_STARTED.md)** | Setup & Basics | Create your first HLFFI program |
| **[2. Calling Haxe](GUIDE_02_CALLING_HAXE.md)** | Functions & Objects | Call methods, access fields, create objects |
| **[3. Data Exchange](GUIDE_03_DATA_EXCHANGE.md)** | Complex Types | Arrays, maps, bytes, enums |
| **[4. Advanced Topics](GUIDE_04_ADVANCED.md)** | Pro Features | Hot reload, callbacks, threading, performance |

---

## Quick Start

### 1. Write Haxe Code

```haxe
// hello.hx
class Main
{
    public static function main()
    {
        trace("Hello from Haxe!");
    }
}
```

### 2. Compile to HashLink

```bash
haxe -main Main -hl hello.hl
```

### 3. Load from C

```c
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "hello.hl");
    hlffi_call_entry(vm);

    // Your Haxe code is now running!

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

---

## What You'll Learn

### Part 1: Getting Started
- Install prerequisites
- Create and compile Haxe bytecode
- Load bytecode from C
- Handle errors
- Understand the VM lifecycle

### Part 2: Calling Haxe from C
- Call static methods
- Read/write static variables
- Create Haxe objects
- Access instance fields
- Call instance methods

### Part 3: Data Exchange
- Work with arrays
- Use maps (dictionaries)
- Handle binary data (bytes)
- Pattern match on enums
- Type checking

### Part 4: Advanced Topics
- Integration modes (threaded vs non-threaded)
- Hot reload for fast iteration
- Register C callbacks for Haxe to call
- Worker thread safety
- Performance optimization with caching
- VM restart capability

---

## Prerequisites

- **HashLink** - [hashlink.github.io](https://hashlink.github.io)
- **Haxe** - [haxe.org](https://haxe.org)
- **C/C++ Compiler** - MSVC, GCC, or Clang

---

## Further Reading

After completing this guide, explore:

- [API Reference](../API_REFERENCE.md) - Complete API documentation
- [Examples](../../examples/) - Working code examples
- [Dev Docs](../dev/) - Internal development documentation

---

**[Start with Part 1: Getting Started →](GUIDE_01_GETTING_STARTED.md)**
