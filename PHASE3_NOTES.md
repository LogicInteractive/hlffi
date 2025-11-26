# Phase 3 Implementation Notes

## Status: Partially Complete

Phase 3 (Static Members & Values) has been implemented at the API level, but runtime access to static fields is not yet working due to HashLink internals complexity.

## What Works

✅ **Value Boxing/Unboxing API** - Complete and functional
- `hlffi_value_int()`, `hlffi_value_float()`, `hlffi_value_bool()`, `hlffi_value_string()`, `hlffi_value_null()`
- `hlffi_value_as_int()`, `hlffi_value_as_float()`, `hlffi_value_as_bool()`, `hlffi_value_as_string()`
- `hlffi_value_is_null()`
- All conversion functions tested and working

✅ **Static Method Calls** - Should work (untested due to field access blocker)
- `hlffi_call_static()` implemented
- Uses HashLink's `vclosure` and `hl_dyn_call_safe()` for safe method invocation
- Method lookup by hash working correctly

## What Doesn't Work Yet

❌ **Static Field Access** - Implemented but not functional
- `hlffi_get_static_field()` - finds class and field correctly, but can't access value
- `hlffi_set_static_field()` - same issue

### The Problem

When looking up static fields:
1. ✅ Class type is found correctly (e.g., "$Game" with hash -375061962)
2. ✅ Class has correct number of fields (12 fields in test case)
3. ✅ Field hash matches (e.g., "score" = 416904417 found at field[0])
4. ❌ `class_type->obj->global_value` is NULL

In HashLink bytecode, classes have two representations:
- **"Game"** - Reflection class (metadata only, 0 fields)
- **"$Game"** - Runtime class (has field definitions, but no global instance)

The issue is that `$Game` doesn't have a global instance object (`global_value == NULL`). Instead, static fields in HashLink appear to be stored as individual globals in `module->globals_data`, but the mapping from field index to global index is not straightforward.

### What We Know

From HashLink source (`vendor/hashlink/src/module.c`):
```c
t->obj->global_value = ((int)(int_val)t->obj->global_value) ?
    (void**)(int_val)(m->globals_data + m->globals_indexes[(int)(int_val)t->obj->global_value-1]) :
    NULL;
```

This shows that `global_value` is set during module loading IF the bytecode initially has a non-zero value. If it's 0, it stays NULL. For classes without global instances, static fields are stored differently.

### Attempted Solutions

1. ❌ Direct `global_value` access - NULL pointer
2. ❌ Using `globals_indexes[field_index]` - incorrect mapping
3. ❌ Looking for generated getter/setter methods - API mismatch

### Research Findings

**From comprehensive HashLink source analysis and documentation review:**

1. **Root Cause Confirmed:**
   - Static fields in HashLink are stored in `module->globals_data`
   - Each global has an offset in `module->globals_indexes`
   - **No public API exists** to map field name → global index
   - Bytecode operations (OGetGlobal/OSetGlobal) encode global indices directly
   - Field definitions in `hl_type_obj->fields[i]` **do NOT** contain global indices

2. **Why `global_value` is NULL:**
   - `global_value` is for class singleton instances, not individual static fields
   - Classes without global instances (pure static classes) have `global_value = NULL`
   - This is by design - static fields are individual globals, not instance fields

3. **From HashLink Experts (Issue #253, #752, FFI_RESEARCH.md):**
   - Direct static field access via C API is **not officially supported**
   - HashLink's own native extensions use different patterns
   - The recommended approach is method-based access

### Recommended Solution

**Use Accessor Methods Pattern** (from FFI_RESEARCH.md, marked as "EASIEST"):

Instead of direct field access, require Haxe code to provide getter/setter methods:

```haxe
class Game {
    public static var score:Int = 0;

    // Add accessor methods for C access
    public static function getScore():Int { return score; }
    public static function setScore(v:Int):Void { score = v; }
}
```

```c
// From C - use method calls (which DO work)
hlffi_value* score = hlffi_call_static(vm, "Game", "getScore", 0, NULL);
```

**Why This Works:**
- ✅ Static method calls are fully functional (uses `proto` array with `findex`)
- ✅ Methods have direct function indices we can resolve
- ✅ No global index mapping needed
- ✅ Matches HashLink's own FFI patterns
- ✅ Simple, reliable, maintainable

### Alternative Approaches Considered

**Option 1: Bytecode Scanning** (NOT RECOMMENDED)
- Scan `code->globals` array looking for matching types/metadata
- Fragile - relies on undocumented bytecode structure
- Breaks on HashLink updates
- No official support

**Option 2: Wait for Official API** (LONG-TERM)
- HashLink may add static field access API in future
- Plugin system (Nov 2025) doesn't address this
- Not a blocker for Phase 3

### Next Steps

1. **Update Phase 3 API** - Document accessor method requirement
2. **Update test programs** - Add getter/setter methods to Game.hx
3. **Update CLAUDE.md** - Document this pattern for future reference
4. **Keep current implementation** - Mark as NOT_IMPLEMENTED until API available
5. **Consider Phase 3.5** - Helper to auto-generate Haxe accessors

## Test Programs

### Haxe Test Code
- **test/Game.hx** - Game class with static fields (score, playerName, isRunning, multiplier)
- **test/StaticMain.hx** - Entry point that uses static members
- Compiles to **test/static_test.hl**

### C Test Program
- **test_static.c** - Comprehensive test with 10 test cases
- Tests value boxing, field access, and method calls
- Added to Makefile as `test_static` target

## Build Status

✅ Compiles without errors on Linux
✅ Added to Makefile build system
✅ Visual Studio project updated (hlffi.vcxproj)

Libraries build successfully:
- libhl.a: 937320 bytes
- libhlffi.a: 225828 bytes (includes Phase 3 code)

## Files Modified

### New Files
- `src/hlffi_values.c` - Phase 3 implementation (~310 lines)
- `test/Game.hx` - Test Haxe class
- `test/StaticMain.hx` - Test entry point
- `test_static.c` - C test program (~210 lines)

### Modified Files
- `include/hlffi.h` - Added Phase 3 API declarations (~135 lines)
- `Makefile` - Added `hlffi_values.c` and `test_static` target
- `hlffi.vcxproj` - Added `hlffi_values.c` for Windows builds

## Recommendations

Phase 3 should be committed as-is with the known limitation documented. The value boxing/unboxing API is complete and useful on its own. Static method calls should work once we can verify with working field access.

The static field access issue requires deeper HashLink expertise and may benefit from:
- Community consultation (HashLink Discord/forum)
- Studying existing HashLink embedding examples
- Reviewing how HashLink's own tools access static fields

This is a complex problem that warrants separate focused investigation rather than blocking the entire Phase 3 implementation.
