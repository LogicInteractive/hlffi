# HLFFI User Guide - Part 1: Getting Started

**Your first steps with HLFFI**

[Next: Calling Haxe from C →](GUIDE_02_CALLING_HAXE.md)

---

## What is HLFFI?

HLFFI is a C/C++ library that lets you **embed Haxe code** inside your C/C++ application. Think of it as a bridge between a C/C++ program being the host, and Haxe/HashLink being the client.

**Use cases:**
- Add scripting to your game engine
- Use Haxe libraries from C/C++ applications
- Prototype game logic in Haxe, integrate into C++ engine
- Build plugins with Haxe for C/C++ hosts

---

## Prerequisites

Before starting, you need:

1. **HashLink** - The Haxe runtime (download from [hashlink.github.io](https://hashlink.github.io))
2. **Haxe** - The Haxe compiler (download from [haxe.org](https://haxe.org))
3. **A C/C++ compiler** - MSVC, GCC, or Clang
4. **CMake** (optional) - For building examples

---

## Your First HLFFI Program

Let's create a simple program that loads and runs Haxe code.

### Step 1: Write the Haxe Code

Create a file called `hello.hx`:

```haxe
class Main
{
    public static function main()
    {
        trace("Hello from Haxe!");
    }
}
```

### Step 2: Compile to HashLink Bytecode

Open a terminal and run:

```bash
haxe -main Main -hl hello.hl
```

This creates `hello.hl` - the bytecode file your C program will load.

### Step 3: Write the C Program

Create `main.c`:

```c
#include <stdio.h>
#include "hlffi.h"

int main(int argc, char** argv)
{
    // 1. Create VM
    hlffi_vm* vm = hlffi_create();
    if (!vm)
    {
        printf("Failed to create VM\n");
        return 1;
    }

    // 2. Initialize
    if (hlffi_init(vm, argc, argv) != HLFFI_OK)
    {
        printf("Failed to init: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    // 3. Load bytecode
    if (hlffi_load_file(vm, "hello.hl") != HLFFI_OK)
    {
        printf("Failed to load: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    // 4. Run main()
    if (hlffi_call_entry(vm) != HLFFI_OK)
    {
        printf("Failed to run: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    // 5. Cleanup
    hlffi_close(vm);
    hlffi_destroy(vm);

    printf("Done!\n");
    return 0;
}
```

### Step 4: Build and Run

**Windows (MSVC):**
```bash
cl main.c /I path\to\hlffi\include /link path\to\hlffi.lib
main.exe
```

**Linux/macOS (GCC):**
```bash
gcc main.c -I path/to/hlffi/include -L path/to/hlffi -lhlffi -o main
./main
```

**Output:**
```
Hello from Haxe!
Done!
```

---

## Understanding the Lifecycle

Every HLFFI program follows this pattern:

```
create → init → load → call_entry → [use VM] → close → destroy
```

| Step | Function | What it does |
|------|----------|--------------|
| 1 | `hlffi_create()` | Allocate VM structure |
| 2 | `hlffi_init()` | Initialize HashLink runtime |
| 3 | `hlffi_load_file()` | Load your .hl bytecode |
| 4 | `hlffi_call_entry()` | Run Haxe `main()` function |
| 5 | *use the VM* | Call functions, access data |
| 6 | `hlffi_close()` | Clean up VM state |
| 7 | `hlffi_destroy()` | Free all resources |

**Important:** Always call `hlffi_close()` before `hlffi_destroy()`.

---

## Error Handling

HLFFI uses error codes. Always check return values:

```c
hlffi_error_code result = hlffi_load_file(vm, "game.hl");

if (result != HLFFI_OK)
{
    // Get human-readable error message
    printf("Error: %s\n", hlffi_get_error_string(result));

    // Get detailed error from VM
    printf("Details: %s\n", hlffi_get_error(vm));
}
```

Common error codes:

| Code | Meaning |
|------|---------|
| `HLFFI_OK` | Success |
| `HLFFI_ERROR_FILE_NOT_FOUND` | .hl file doesn't exist |
| `HLFFI_ERROR_INVALID_BYTECODE` | File is not valid bytecode |
| `HLFFI_ERROR_ENTRY_POINT_NOT_FOUND` | No `main()` function |

---

## A More Complete Example

Here's a template you can use for any HLFFI project:

```c
#include <stdio.h>
#include "hlffi.h"

int main(int argc, char** argv)
{
    hlffi_vm* vm = NULL;
    int exit_code = 0;

    // Create
    vm = hlffi_create();
    if (!vm)
    {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    // Initialize
    hlffi_error_code err = hlffi_init(vm, argc, argv);
    if (err != HLFFI_OK)
    {
        fprintf(stderr, "Init failed: %s\n", hlffi_get_error_string(err));
        exit_code = 1;
        goto cleanup;
    }

    // Load
    err = hlffi_load_file(vm, "game.hl");
    if (err != HLFFI_OK)
    {
        fprintf(stderr, "Load failed: %s\n", hlffi_get_error(vm));
        exit_code = 1;
        goto cleanup;
    }

    // Run entry point
    err = hlffi_call_entry(vm);
    if (err != HLFFI_OK)
    {
        fprintf(stderr, "Entry failed: %s\n", hlffi_get_error(vm));
        exit_code = 1;
        goto cleanup;
    }

    // ========================================
    // Your code here: call Haxe functions, etc.
    // ========================================

    printf("Success!\n");

cleanup:
    if (vm)
    {
        hlffi_close(vm);
        hlffi_destroy(vm);
    }
    return exit_code;
}
```

---

## Project Structure

A typical HLFFI project looks like this:

```
my_project/
├── src/
│   └── main.c           # Your C code
├── haxe/
│   ├── Main.hx          # Haxe entry point
│   ├── Game.hx          # Your Haxe classes
│   └── build.hxml       # Haxe build config
├── bin/
│   ├── game.hl          # Compiled bytecode
│   └── my_app.exe       # Your executable
└── CMakeLists.txt       # Build configuration
```

**build.hxml example:**
```hxml
-main Main
-hl ../bin/game.hl
-cp .
```

Build Haxe with:
```bash
cd haxe
haxe build.hxml
```

---

## Quick Reference

### Minimum Working Example

```c
#include "hlffi.h"

int main()
{
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // ... use the VM ...

    hlffi_close(vm);
    hlffi_destroy(vm);
    return 0;
}
```

### Include Header

```c
#include "hlffi.h"
```

### Check Version

```c
printf("HLFFI version: %s\n", HLFFI_VERSION_STRING);  // "3.0.0"
```

---

## What's Next?

Now that you can load and run Haxe code, let's learn how to:
- Call Haxe functions from C
- Pass data between C and Haxe
- Access Haxe objects and fields

**[Next: Calling Haxe from C →](GUIDE_02_CALLING_HAXE.md)**

---

## Troubleshooting

### "Failed to load: File not found"

- Check the path to your .hl file
- Make sure you compiled with `haxe -hl output.hl`
- Use absolute path if relative path doesn't work

### "Entry point not found"

- Your Haxe code needs a `public static function main()`
- Make sure it's in a class called `Main` (or specify `-main YourClass`)

### Crashes on startup

- Make sure HashLink DLLs are in the same folder as your executable
- On Windows: `libhl.dll`, `fmt.hdll`, etc.

### "Invalid bytecode"

- Recompile your Haxe code with matching HashLink version
- Check you're loading a `.hl` file, not a `.hx` file

---

**[Next: Calling Haxe from C →](GUIDE_02_CALLING_HAXE.md)**
