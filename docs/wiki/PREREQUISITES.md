# Prerequisites

**What you need before using HLFFI**

[← Home](HOME.md) | [Next: Getting Started →](../guide/GUIDE_01_GETTING_STARTED.md)

---

## Required Software

### 1. Haxe Compiler

The Haxe compiler transforms your `.hx` source files into HashLink bytecode (`.hl` files).

**Download:** [haxe.org/download](https://haxe.org/download/)

**Verify installation:**
```bash
haxe --version
# Should output: 4.3.x or newer
```

**Windows:** Use the installer, which adds Haxe to your PATH.

**macOS:**
```bash
brew install haxe
```

**Linux (Ubuntu/Debian):**
```bash
sudo add-apt-repository ppa:haxe/releases
sudo apt update
sudo apt install haxe
```

---

### 2. HashLink Runtime

HashLink is the virtual machine that runs your compiled Haxe bytecode.

**Download:** [github.com/HaxeFoundation/hashlink/releases](https://github.com/HaxeFoundation/hashlink/releases)

**Version requirements:**
| HLFFI Feature | Minimum HashLink Version |
|---------------|--------------------------|
| Basic embedding | 1.11 |
| Hot reload | 1.12 |
| Full features | 1.14+ (recommended) |

**Windows:**
1. Download the Windows release (e.g., `hashlink-1.14-win64.zip`)
2. Extract to a folder (e.g., `C:\HashLink`)
3. Add to PATH (optional, but recommended)

**macOS:**
```bash
brew install hashlink
```

**Linux:**
```bash
# Build from source (recommended for latest version)
git clone https://github.com/HaxeFoundation/hashlink.git
cd hashlink
make
sudo make install
```

**Verify installation:**
```bash
hl --version
# Should output: 1.14 or similar
```

---

### 3. C/C++ Compiler

You need a C compiler to build your application with HLFFI.

**Windows - Visual Studio:**
- Download [Visual Studio](https://visualstudio.microsoft.com/) (Community edition is free)
- Install "Desktop development with C++" workload
- Use Developer Command Prompt or VS IDE

**Windows - MinGW:**
- Download [MSYS2](https://www.msys2.org/)
- Install: `pacman -S mingw-w64-x86_64-gcc`

**macOS:**
```bash
xcode-select --install
```

**Linux:**
```bash
sudo apt install build-essential  # Ubuntu/Debian
sudo dnf install gcc gcc-c++      # Fedora
```

---

### 4. CMake (Optional but Recommended)

CMake simplifies building HLFFI and your projects.

**Download:** [cmake.org/download](https://cmake.org/download/)

**Windows:** Use the installer.

**macOS:**
```bash
brew install cmake
```

**Linux:**
```bash
sudo apt install cmake
```

**Verify:**
```bash
cmake --version
# Should output: 3.16 or newer
```

---

## HLFFI Files

You need these files from the HLFFI repository:

| File | Purpose |
|------|---------|
| `include/hlffi.h` | Main header file |
| `src/hlffi.c` | Implementation (if not using pre-built library) |
| `lib/hlffi.lib` | Pre-built library (Windows) |
| `lib/libhlffi.a` | Pre-built library (Linux/macOS) |

**Get HLFFI:**
```bash
git clone https://github.com/your-repo/hlffi.git
```

---

## Directory Setup

A typical project structure:

```
my_project/
├── src/
│   └── main.c              # Your C code
├── haxe/
│   ├── Main.hx             # Your Haxe code
│   └── build.hxml          # Haxe build config
├── bin/
│   ├── game.hl             # Compiled bytecode (output)
│   └── my_app.exe          # Your executable (output)
├── lib/
│   └── hlffi/              # HLFFI library
│       ├── include/
│       │   └── hlffi.h
│       └── lib/
│           └── hlffi.lib
└── CMakeLists.txt          # Build configuration
```

---

## HashLink Files for Distribution

When distributing your application, include these HashLink files alongside your executable:

**Windows (required DLLs):**
```
libhl.dll       # Core HashLink library
fmt.hdll        # String formatting
ssl.hdll        # SSL/TLS (if using HTTPS)
sqlite.hdll     # SQLite (if using database)
# ... other .hdll files as needed
```

**Linux:**
```
libhl.so
# .hdll files as needed
```

**macOS:**
```
libhl.dylib
# .hdll files as needed
```

**Tip:** Only include the .hdll files your Haxe code actually uses.

---

## Environment Variables (Optional)

Set these for convenience:

**Windows (PowerShell):**
```powershell
$env:HASHLINK_DIR = "C:\HashLink"
$env:PATH += ";C:\HashLink"
```

**Windows (CMD):**
```cmd
set HASHLINK_DIR=C:\HashLink
set PATH=%PATH%;C:\HashLink
```

**Linux/macOS:**
```bash
export HASHLINK_DIR=/usr/local
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

---

## Verification Checklist

Run these commands to verify your setup:

```bash
# 1. Haxe compiler
haxe --version
# Expected: 4.x.x

# 2. HashLink runtime
hl --version
# Expected: 1.12 or newer

# 3. C compiler
gcc --version      # Linux/macOS/MinGW
cl                 # Windows MSVC (from Developer Command Prompt)
# Expected: version info

# 4. CMake (optional)
cmake --version
# Expected: 3.x.x
```

---

## Quick Test

Create a simple test to verify everything works:

**1. Create `test.hx`:**
```haxe
class Main
{
    public static function main()
    {
        trace("Haxe is working!");
    }
}
```

**2. Compile to bytecode:**
```bash
haxe -main Main -hl test.hl
```

**3. Run with HashLink:**
```bash
hl test.hl
# Should output: Main.hx:3: Haxe is working!
```

If this works, you're ready to use HLFFI!

---

## Troubleshooting

### "haxe: command not found"

Haxe is not in your PATH. Either:
- Add Haxe's bin directory to PATH
- Use the full path: `/path/to/haxe`

### "hl: command not found"

HashLink is not in your PATH. Either:
- Add HashLink directory to PATH
- Use the full path: `/path/to/hl`

### "Cannot find libhl.dll" (Windows)

Copy HashLink DLLs to your executable's directory, or add HashLink to PATH.

### "undefined reference to hl_*" (linking)

Your linker can't find HashLink. Check:
- Library path includes HashLink's lib directory
- You're linking against `libhl` (Linux) or `libhl.lib` (Windows)

### Haxe compile errors

Make sure you're using a compatible Haxe version. HLFFI works best with Haxe 4.2+.

---

## Platform-Specific Notes

### Windows

- Use **x64** builds (not x86) for best compatibility
- MSVC is recommended; MinGW works but may require tweaks
- Place DLLs next to your .exe or in PATH

### macOS

- Xcode command line tools are sufficient
- Homebrew installations work out of the box
- May need to sign/notarize for distribution

### Linux

- Building HashLink from source gives best results
- Ensure `libhl.so` is in library path (`LD_LIBRARY_PATH`)
- May need `sudo ldconfig` after installing

---

## Next Steps

Once your environment is set up:

**[→ Getting Started Guide](../guide/GUIDE_01_GETTING_STARTED.md)** - Create your first HLFFI program

---

## Summary

| Component | Purpose | Where to Get |
|-----------|---------|--------------|
| Haxe | Compile .hx to .hl | haxe.org |
| HashLink | Run .hl bytecode | hashlink.github.io |
| C Compiler | Build your app | VS, GCC, Clang |
| HLFFI | Bridge C ↔ Haxe | This repository |
| CMake | Build system (optional) | cmake.org |

---

[← Home](HOME.md) | [Next: Getting Started →](../guide/GUIDE_01_GETTING_STARTED.md)
