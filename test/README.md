# HLFFI Test Files

Simple Haxe test programs for validating HLFFI functionality.

## Prerequisites

1. **Haxe Compiler**: Download from https://haxe.org/download/
2. **HLFFI Built**: Build the Visual Studio solution first

## Building the Test Files

### Windows

```cmd
build.bat
```

This compiles `Main.hx` to `hello.hl` bytecode.

### Manual Build

```cmd
haxe -hl hello.hl -main Main
```

## Running the Tests

### With HashLink Standalone

If you have HashLink installed:

```cmd
hl hello.hl
```

### With HLFFI Examples

Using the HLFFI hello_world example:

```cmd
..\bin\x64\Debug\example_hello_world.exe hello.hl
```

Or in Release mode:

```cmd
..\bin\x64\Release\example_hello_world.exe hello.hl
```

## What the Test Does

The `Main.hx` test program:

1. ✅ Prints trace messages
2. ✅ Tests integer, float, and string operations
3. ✅ Tests array creation and access
4. ✅ Tests Map (hash table) operations

This validates:
- Basic VM initialization
- Standard library functions
- Memory allocation
- Garbage collection
- Type system

## Expected Output

```
Hello from Haxe!
HLFFI v3.0 - HashLink FFI Library
Testing basic VM functionality...
Integer: 42
Float: 3.14
String: Hello World
Array: [1,2,3,4,5]
Array length: 5
Map keys: [one,two,three]
Map values: [1,2,3]
Test completed successfully!
```

## Troubleshooting

**Error: "haxe: command not found"**
- Install Haxe from https://haxe.org/download/
- Add Haxe to your PATH

**Error: "hl: command not found"**
- Install HashLink from https://hashlink.haxe.org/
- Or use HLFFI examples instead

**Error: "example_hello_world.exe not found"**
- Build the Visual Studio solution first
- Check that the example built successfully

## Adding More Tests

To add more test files:

1. Create `YourTest.hx` with a `static function main()`
2. Compile with: `haxe -hl yourtest.hl -main YourTest`
3. Run with: `..\bin\x64\Debug\example_hello_world.exe yourtest.hl`

## Integration Tests

For more complex tests (HTTP, timers, etc.), see the integration test examples in the HLFFI documentation.
