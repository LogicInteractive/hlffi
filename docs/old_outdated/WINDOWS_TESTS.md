# HLFFI Windows Test Results

## Test Environment

- **Platform**: Windows (win32)
- **Build Configuration**: Debug/Release x64
- **Build System**: CMake + Visual Studio

## Test Summary

All Phase 5 and Phase 6 tests pass on Windows:

| Test | Result | Description |
|------|--------|-------------|
| test_callbacks | 14/14 | C→Haxe callbacks |
| test_exceptions | 9/9 | Exception handling & stack traces |
| test_arrays | 10/10 | Array operations |
| test_map_demo | All pass | IntMap and StringMap interop |
| test_bytes_demo | All pass | hl.Bytes and haxe.io.Bytes |
| test_enum_demo | All pass | Enum inspection |

## Test Details

### test_callbacks (14/14 passed)

Tests C callback registration and invocation from Haxe:

1. Register 0-arg callback (Void->Void)
2. Register 1-arg callback (String->Void)
3. Register 2-arg callback ((Int,Int)->Int)
4. Register 3-arg callback ((Int,Int,Int)->Int)
5. Get registered callback
6. Invoke 1-arg callback from Haxe
7. Invoke 2-arg callback from Haxe
8. Invoke 0-arg callback multiple times
9. Invoke 3-arg callback from Haxe
10. Reject invalid callback arity (>4)
11. Reject duplicate callback name
12. Get non-existent callback returns NULL
13. Unregister callback
14. Unregister non-existent callback returns false

### test_exceptions (9/9 passed)

Tests exception handling and stack trace extraction:

1. Safe method call succeeds
2. Exception caught
3. Custom exception caught
4. Conditional no-throw succeeds
5. Conditional throw caught
6. Division by zero caught
7. Safe division succeeds
8. Exception message extractable
9. Regular error distinguished from exception

### test_arrays (10/10 passed)

Tests array operations between C and Haxe:

- Array creation, get/set operations
- Array length queries
- Passing arrays to/from Haxe
- Type coercion

### test_map_demo

Tests Map interop:

- IntMap: get, set, exists
- StringMap: get, set, exists (with HBYTES→String conversion fix)
- Modify map from C and pass back to Haxe

### test_bytes_demo

Tests binary data handling:

- hl.Bytes operations
- haxe.io.Bytes operations
- Blit and compare operations

### test_enum_demo

Tests enum inspection:

- Enum type lookup
- Enum value inspection
- Enum parameters extraction

## Build Instructions

### Building Tests

```bash
# Configure
cmake -B build -DHASHLINK_DIR="C:/HashLink" -DHLFFI_BUILD_TESTS=ON

# Build all tests (Debug)
cmake --build build --config Debug

# Build all tests (Release)
cmake --build build --config Release
```

### Running Tests

```bash
# From project root
bin\x64\Debug\test_callbacks.exe test\callbacks.hl
bin\x64\Debug\test_exceptions.exe test\exceptions.hl
bin\x64\Debug\test_arrays.exe test\arrays.hl
bin\x64\Debug\test_map_demo.exe
bin\x64\Debug\test_bytes_demo.exe
bin\x64\Debug\test_enum_demo.exe
```

## Known Issues Fixed

### Warning Cleanup (November 2024)

Fixed compiler/linker warnings:

1. **C4273 dll linkage warnings**: Removed redundant extern declarations from source files that conflicted with HL_API decorators in hl.h

2. **LNK4006 duplicate symbol warnings**: Removed duplicate function implementations from `hlffi_integration.c` (thread and blocking functions were already in `hlffi_threading.c` and `hlffi_callbacks.c`)

3. **C4013 undefined function warnings**: Replaced non-existent `hl_bytes_blit` and `hl_bytes_compare` with standard C equivalents (`memmove` and `memcmp`)

### StringMap Key Fix

Fixed StringMap.exists() returning false for existing keys by adding HBYTES→String type conversion in `hlffi_call_method()`.

## See Also

- [PHASE5_COMPLETE.md](PHASE5_COMPLETE.md) - Phase 5 documentation
- [PHASE6_COMPLETE.md](PHASE6_COMPLETE.md) - Phase 6 documentation
- [MAPS_GUIDE.md](MAPS_GUIDE.md) - Map operations guide
