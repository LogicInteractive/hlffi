# HLFFI TODO List

This file tracks known issues, limitations, and planned improvements for the HLFFI library.

## High Priority

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
- ✅ Phase 5: Object instance methods
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
