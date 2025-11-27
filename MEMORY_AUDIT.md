# HLFFI Memory Management Audit

## Executive Summary
**Status**: ⚠️ CRITICAL ISSUES FOUND - Potential memory leaks and dangling pointers

## Issues Found

### 🔴 CRITICAL: GC-managed allocations without roots
**Location**: `src/hlffi_values.c`, `src/hlffi_objects.c`

**Problem**: Functions allocate GC-managed memory but don't protect it with GC roots.

**Affected Functions**:
1. `hlffi_value_int()` - Line 109: `hl_alloc_dynamic(&hlt_i32)` with `is_rooted = false`
2. `hlffi_value_float()` - Line 125: `hl_alloc_dynamic(&hlt_f32)` with `is_rooted = false`
3. `hlffi_value_bool()` - Line 141: `hl_alloc_dynamic(&hlt_bool)` with `is_rooted = false`
4. `hlffi_get_field()` - Lines 193, 201, 209, 217: `hl_alloc_dynamic()` with `is_rooted = false`

**Risk**: If GC runs before caller uses these values, they could be collected, causing:
- Dangling pointers
- Use-after-free crashes
- Corrupted data

**Why this might work anyway**: The `HLFFI_UPDATE_STACK_TOP()` macro updates the GC's stack scanning range. If these values are stored in local variables on the C stack, the GC will see them during stack scanning and keep them alive. However, this is fragile and depends on:
- Values being stored in stack variables (not registers)
- GC stack scanning being correct
- No GC cycles between allocation and stack storage

**Recommendation**: Either:
- **Option A**: Add GC roots to all allocated values (safest)
- **Option B**: Document that values must be immediately stored in stack variables
- **Option C**: Use a different allocation strategy for primitives

---

### 🟡 MEDIUM: Inconsistent free() API
**Location**: Test code and throughout codebase

**Problem**: Users must know which free function to call:
- `hlffi_value_free()` for rooted values (from `hlffi_new()`)
- `free()` for non-rooted values (from `hlffi_get_field()`, `hlffi_call_method()`, `hlffi_value_int()`)

**Example from test**:
```c
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
// ... use player ...
hlffi_value_free(player);  // Correct - removes GC root

hlffi_value* health = hlffi_get_field(player, "health");
// ... use health ...
free(health);  // Also correct - no GC root to remove
```

**Risk**:
- Confusing API - users might call wrong free function
- Memory leaks if `hlffi_value_free()` not called on rooted values
- The test code is actually correct, but this is error-prone

**Recommendation**: Make API consistent:
- **Option A**: Always use `hlffi_value_free()` (it safely handles both cases)
- **Option B**: Rename to `hlffi_value_release()` to avoid confusion with `free()`
- **Option C**: Document clearly which functions return rooted vs non-rooted values

---

### 🟡 MEDIUM: String memory must be freed by caller
**Location**: `src/hlffi_values.c:271, 278, 287`

**Problem**: `hlffi_value_as_string()` returns `strdup()`-allocated memory.

**Code**:
```c
char* utf8 = hl_to_utf8(hl_str->bytes);
return utf8 ? strdup(utf8) : NULL;  // Caller must free!
```

**Risk**: Memory leak if caller doesn't free the string.

**Current status**: Test code correctly frees strings with `free(name_str)`.

**Documentation**: ⚠️ NOT DOCUMENTED in header file

**Recommendation**: Add to header documentation:
```c
/**
 * Convert value to C string (UTF-8).
 *
 * @param value Value handle
 * @return Dynamically allocated string, or NULL if not a string
 * @note CALLER MUST FREE returned string with free()
 */
char* hlffi_value_as_string(hlffi_value* value);
```

---

### 🟢 GOOD: GC root management for objects
**Location**: `src/hlffi_objects.c:150`

**Code**:
```c
hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
wrapped->hl_value = (vdynamic*)instance;
wrapped->is_rooted = true;
hl_add_root(&wrapped->hl_value);  /* Keep object alive! */
```

✅ Correct: Objects created with `hlffi_new()` are properly GC-rooted.
✅ Correct: `hlffi_value_free()` properly removes roots.

---

### 🟢 GOOD: Proper cleanup in hlffi_value_free()
**Location**: `src/hlffi_values.c:195-205`

**Code**:
```c
void hlffi_value_free(hlffi_value* value) {
    if (!value) return;

    if (value->is_rooted && value->hl_value) {
        hl_remove_root(&value->hl_value);
    }

    free(value);
}
```

✅ Correct: Handles both rooted and non-rooted values safely.
✅ Correct: NULL-safe.

---

## Memory Ownership Rules (Current Implementation)

| Function | Returns | GC Root? | Free With |
|----------|---------|----------|-----------|
| `hlffi_new()` | Object instance | ✅ YES | `hlffi_value_free()` |
| `hlffi_value_int/float/bool()` | Boxed primitive | ❌ NO | `free()` or `hlffi_value_free()` |
| `hlffi_value_string()` | String object | ❌ NO | `free()` or `hlffi_value_free()` |
| `hlffi_get_field()` | Field value | ❌ NO | `free()` or `hlffi_value_free()` |
| `hlffi_call_method()` | Return value | ❌ NO | `free()` or `hlffi_value_free()` |
| `hlffi_value_as_string()` | char* | N/A | `free()` REQUIRED |

**Note**: `hlffi_value_free()` works for all cases (checks `is_rooted` flag), so it's safe to always use it.

---

## Recommendations

### Immediate Actions (Critical)

1. **Document memory ownership** in `include/hlffi.h`:
   - Which functions return values that need freeing
   - Whether to use `free()` or `hlffi_value_free()`
   - String return values require `free()`

2. **Add safety documentation** for GC-allocated primitives:
   ```c
   /**
    * Create an integer value.
    *
    * @note The returned value uses GC-managed memory. Store in a
    *       stack variable immediately to ensure it stays alive.
    * @note Free with hlffi_value_free() or free() when done.
    */
   ```

3. **Fix test code consistency**:
   - Change all `free(value)` to `hlffi_value_free(value)` for safety
   - This works for both rooted and non-rooted values

### Long-term Actions (Recommended)

4. **Consider adding GC roots to all values**:
   ```c
   hlffi_value* hlffi_value_int(hlffi_vm* vm, int value) {
       // ...
       wrapped->is_rooted = true;  // Changed from false
       hl_add_root(&wrapped->hl_value);  // Add root for safety
       return wrapped;
   }
   ```
   **Pros**: Safer, no dangling pointer risk
   **Cons**: Slight performance cost, more GC roots to track

5. **Add memory leak detection test**:
   - Create test that allocates many values in a loop
   - Run with valgrind or AddressSanitizer
   - Verify no leaks after freeing all values

---

## Test Code Review

The test code in `test_instance_basic.c` is **mostly correct** but has inconsistent free() usage:

✅ Correctly frees strings: `if (name_str) free(name_str);`
✅ Correctly uses `hlffi_value_free()` for player object
✅ Correctly uses `hlffi_value_free()` for created values (damage, new_health)

⚠️ Uses `free()` directly for field/method returns (works but inconsistent):
- `free(health)` - from `hlffi_get_field()`
- `free(result)` - from `hlffi_call_method()`

**Recommendation**: Change all to `hlffi_value_free()` for consistency and safety.

---

## Conclusion

The memory management is **functionally correct** but:
1. ⚠️ Relies on GC stack scanning for safety (fragile)
2. ⚠️ API is confusing (when to use free() vs hlffi_value_free())
3. ⚠️ Documentation is missing
4. ⚠️ Could cause issues in optimized builds (values in registers)

**Risk Level**: MEDIUM-HIGH
- No leaks detected in current test code
- But could fail in edge cases or with optimization
- Potential for user errors due to confusing API

**Action Required**: Documentation improvements and test code cleanup recommended before production use.
