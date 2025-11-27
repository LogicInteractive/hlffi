# HLFFI TODO List

This file tracks known issues, limitations, and planned improvements for the HLFFI library.

## High Priority

### Complete C→Haxe Array Passing (Phase 5)
**Status:** Nearly Complete (90% - 9/10 tests passing)
**Issue:** Passing C-created arrays TO Haxe methods fails due to type incompatibility.

**What Works:**
- ✅ C-created arrays (`hlffi_array_new`) - full CRUD operations
- ✅ Array length detection for both C and Haxe arrays
- ✅ Element access (`hlffi_array_get`) for Haxe Array objects (FIXED!)
- ✅ Tests 1-9 passing

**What Doesn't Work:**
- ❌ Passing C-created arrays (varray) to Haxe methods expecting Array<T> objects
- ❌ Test 10 failing

**Root Cause (Test 10):**
- C-created arrays use raw `varray` type (HARRAY)
- Haxe methods expect `hl.types.ArrayBytes_T` wrapper objects (HOBJ)
- Would need array type conversion/wrapping to pass C arrays to Haxe
- The reverse direction (Haxe→C) works perfectly after discovering field layout

**Solution Implemented (Tests 1-9):**
- ✅ Discovered HashLink optimizes field layout in memory
- ✅ Field names [bytes, size] but memory layout is [size(int), bytes(ptr)]
- ✅ Read size from first 4 bytes, bytes pointer from offset sizeof(void*)
- ✅ Cast bytes pointer directly to typed data (int*, double*, etc.)
- ✅ Supports Int and Float arrays from Haxe

**Next Steps:**
1. Implement array wrapper conversion for C→Haxe direction (Test 10)
2. Add support for String, Bool, and Dynamic arrays
3. Consider convenience helpers for common array operations

**Files to modify:**
- `src/hlffi_values.c` - Fix `hlffi_array_get` and `hlffi_array_set`
- `test_arrays.c` - All tests should pass

---

### Fix Typed Callbacks (Phase 6)
**Status:** Experimental / Non-functional
**Issue:** `hlffi_register_callback_typed()` crashes when callbacks with primitive arguments are invoked.

**Root Cause:**
- Wrapper functions expect `vdynamic*` pointers for all arguments
- Typed closures pass primitive types (Int/Float/Bool) as raw values
- Stack layout mismatch causes segmentation faults

**Current Workaround:**
Use `hlffi_register_callback()` with Dynamic types in Haxe.

**Proposed Solutions:**
1. **Create typed wrapper functions** for common type combinations:
   - `wrapper_ii_i` for `(Int,Int)->Int`
   - `wrapper_fff_f` for `(Float,Float,Float)->Float`
   - `wrapper_sis_s` for `(String,Int,String)->String`
   - etc.

2. **Dynamic wrapper generation** using libffi or similar:
   - Generate wrappers at runtime based on type signature
   - More flexible but adds dependency

3. **Hybrid approach**:
   - Pre-define wrappers for most common signatures
   - Fall back to Dynamic for complex cases

**Files to modify:**
- `src/hlffi_callbacks.c` - Add typed wrapper functions
- `include/hlffi.h` - Update documentation when fixed

**Test coverage:** All infrastructure exists in `test_callbacks.c` and `test_typed_minimal.c`

---

## Medium Priority

### Performance Optimizations
- [ ] Cache function lookups to avoid repeated hash table searches
- [ ] Optimize string conversion (UTF-8 ↔ UTF-16)
- [ ] Pool allocations for frequently created/destroyed values

### Error Handling
- [ ] More detailed error messages with context
- [ ] Error codes for different failure modes
- [ ] Optional error callback mechanism

### Documentation
- [ ] Add more examples to README
- [ ] Create API reference documentation
- [ ] Add performance benchmarks

---

## Low Priority

### Additional Features
- [ ] Support for more than 4 callback arguments
- [ ] Hot reload improvements
- [ ] Better debugging tools/introspection
- [ ] GC tuning utilities

### Build System
- [ ] CMake improvements for cross-platform builds
- [ ] Package manager integration (vcpkg, conan)
- [ ] CI/CD pipeline

---

## Completed
- ✅ Phase 1: Basic VM lifecycle
- ✅ Phase 2: Static method calls
- ✅ Phase 3: Static variable access
- ✅ Phase 4: Value boxing/unboxing
- ✅ Phase 4: Object instance methods
- 🟢 Phase 5: Array operations (9/10 tests passing - C arrays + Haxe→C arrays working)
- ✅ Phase 6: Dynamic callbacks (working)
- ✅ Phase 6: Callback unregistration

---

## Notes

**Typed Callbacks Decision:**
The typed callback API was implemented as an experiment to enable type-safe callbacks
in Haxe. While the registration works correctly, the runtime invocation has fundamental
issues that require significant refactoring. The Dynamic callback approach works well
and is recommended for production use.

For questions or to contribute fixes, see the GitHub repository.
