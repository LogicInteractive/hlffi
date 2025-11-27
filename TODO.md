# HLFFI TODO List

This file tracks known issues, limitations, and planned improvements for the HLFFI library.

## High Priority

### Phase 5: Advanced Value Types
**Status:** 🟡 **Partial (25% - Arrays Complete)**

**Completed Components:**
- ✅ **Arrays** - Full bidirectional C↔Haxe support (10/10 tests passing)
  - C-created arrays automatically wrapped as Haxe Array<T> objects
  - Support for Int, Float, Dynamic, and String array types
  - Operations: create, get, set, push, length, bounds checking

**Remaining Components:**
- ❌ **Maps** - Hash maps / dictionaries (not started)
  - API: `hlffi_map_new()`, `hlffi_map_get/set()`, `hlffi_map_exists()`, `hlffi_map_keys()`
  - Support both Map<String, T> and Map<Int, T>
  - Handle Haxe's Map implementation (IntMap, StringMap, ObjectMap)

- ❌ **Enums** - Construct, match, extract enum values (not started)
  - API: `hlffi_enum_construct()`, `hlffi_enum_index()`, `hlffi_enum_name()`, `hlffi_enum_get_param()`
  - Support enum constructors with parameters
  - Pattern matching support

- ❌ **Bytes** - Binary data I/O (not started)
  - API: `hlffi_bytes_new()`, `hlffi_bytes_length()`, `hlffi_bytes_read/write()`
  - Efficient binary data handling
  - Compatible with Haxe's Bytes type

- ❌ **Null handling** - Proper null value support (not started)
  - Distinguish between null and default values
  - Null-safe APIs

**Array Implementation Details:**
Discovered and implemented HashLink's optimized field memory layout:
- Field names: [bytes, size]
- Memory layout: [size(int), bytes(ptr)] - reordered for alignment
- Direct memory access bypasses broken `hl_dyn_getp` for array field access

**Solution Implemented:**
1. `find_haxe_array_type()` - Locates hl.types.ArrayBytes_* types in module
2. `wrap_varray_as_haxe_array()` - Converts C varrays to Haxe Array objects
3. Updated `hlffi_array_new()` - Creates wrapped arrays by default
4. Updated `hlffi_array_get/set/push()` - Handle both HARRAY and HOBJ types
5. Updated `hlffi_array_length()` - Direct memory read for wrapped arrays

**Supported Array Types:**
- ✅ hl.types.ArrayBytes_Int (i32)
- ✅ hl.types.ArrayBytes_F64 (f64)
- ✅ hl.types.ArrayDyn (dynamic/mixed types)
- ⚠️ hl.types.ArrayBytes_String (fallback to varray)
- ⚠️ hl.types.ArrayBytes_UI8 (bool - fallback to varray)

**Test Coverage:** All 10 tests passing
1. ✅ Create empty array
2. ✅ Create array with length
3. ✅ Set and get int array elements
4. ✅ Float array operations
5. ✅ String array operations
6. ✅ Dynamic array operations
7. ✅ Array push
8. ✅ Bounds checking
9. ✅ Get array from Haxe (Haxe→C)
10. ✅ Pass array to Haxe (C→Haxe) - **THE BREAKTHROUGH!**

**Files Modified:**
- `src/hlffi_values.c` - Complete array implementation with wrapping
- `include/hlffi.h` - Array API declarations
- `test_arrays.c` - Comprehensive test suite
- `test/Arrays.hx` - Haxe test methods

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
- 🟡 Phase 5: Advanced Value Types (25% - Arrays complete, Maps/Enums/Bytes pending)
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
