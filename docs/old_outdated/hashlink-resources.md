# HashLink External Resources

This document contains links to useful HashLink blog posts and tutorials.

## Aramallo's HashLink Blog Series

### Working with C
**URL:** https://aramallo.com/blog/hashlink/calling-c.html

**Summary:**
This article explains how to call C code from Haxe when targeting HashLink.

**Key Topics:**
- **@:hlNative macro** - Tells the Haxe compiler that a class or method exists in the VM's natives table
  - The name in parenthesis will be used when generating the corresponding call in C
  - This is C11, not C++, so there are no classes
  - The final symbol name is a combination of the Haxe class name and method name
- **Mixed implementation** - You can have parts of your class implemented in Haxe, which greatly reduces the amount of C code you need to maintain
- **Note:** The standard Haxe `extern` keyword doesn't work for HashLink

**Related to HLFFI:** This covers the **opposite direction** from HLFFI:
- Aramallo's post: Haxe → C (calling native C from Haxe code)
- HLFFI: C → Haxe (embedding Haxe VM and calling Haxe from C/C++)

---

### Compiling to HLC
**URL:** https://aramallo.com/blog/hashlink/compiling-hlc.html

**Summary:**
This article covers compiling Haxe to C11 using HashLink's HLC compiler.

**Key Topics:**
- HashLink can compile programs to either **bytecode (.hl)** or to **C11 (.c)**
- HLC compilation allows ahead-of-time compilation for better performance
- Differences between bytecode and native compilation modes

**Related to HLFFI:** Understanding HLC compilation helps when:
- Debugging FFI issues (HLC produces readable C code)
- Optimizing performance-critical Haxe code called from C
- Understanding HashLink's internal structure

---

### Embedding the Hashlink VM
**URL:** https://aramallo.com/blog/hashlink/index.html

**Summary:**
Overview of embedding the HashLink virtual machine in applications.

**Related to HLFFI:** This is directly related to HLFFI's core use case - embedding the HashLink VM to run Haxe bytecode from C/C++ applications.

---

## Additional Resources

### Official HashLink Documentation
- **HashLink GitHub:** https://github.com/HaxeFoundation/hashlink
- **API Documentation:** See `vendor/hashlink/src/hl.h` for complete API reference

### HLFFI Project Documentation
- [README.md](../README.md) - Quick start and overview
- [hlffi_manual.md](hlffi_manual.md) - Complete developer manual
- [CLAUDE.md](../CLAUDE.md) - Project structure and development guide
- [dexutils.h](dexutils.h) - Production FFI implementation reference
