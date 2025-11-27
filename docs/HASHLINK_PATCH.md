# HashLink Patch: obj_resolve_field

## Overview

HLFFI requires a small modification to HashLink's `obj.c` to expose the `obj_resolve_field()` function for static field/method access.

**Patch File:** `hashlink_obj_resolve_field.patch` (root directory)

## What the Patch Does

**File Modified:** `vendor/hashlink/src/std/obj.c` (line 64)

**Change:**
```c
// Before (line 64):
static hl_field_lookup *obj_resolve_field( hl_type_obj *o, int hfield ) {

// After:
/* Made non-static for HLFFI static field/method access */
hl_field_lookup *obj_resolve_field( hl_type_obj *o, int hfield ) {
```

This makes the function accessible to HLFFI for:
- Static field access (`hlffi_get_static_field`, `hlffi_set_static_field`)
- Static method calls (`hlffi_call_static`)
- Dynamic field resolution by hash

## Applying the Patch

### Automatic (During Build)

The CMake build automatically applies this patch if it hasn't been applied:

```bash
cmake -B build -DHASHLINK_DIR=/path/to/hashlink
cmake --build build
```

### Manual Application

If building manually or the patch fails:

```bash
cd vendor/hashlink
patch -p1 < ../../hashlink_obj_resolve_field.patch
```

**Verify patch applied:**
```bash
grep -n "^hl_field_lookup \*obj_resolve_field" vendor/hashlink/src/std/obj.c
```

Should show line 65 without `static` keyword.

### Reverting the Patch

```bash
cd vendor/hashlink
patch -R -p1 < ../../hashlink_obj_resolve_field.patch
```

## Why Not Fork HashLink?

We use a patch instead of maintaining a fork because:

1. ✅ **Simpler maintenance** - No fork to keep synced with upstream
2. ✅ **Transparent** - Users see exactly what's changed
3. ✅ **Portable** - Works with any HashLink version
4. ✅ **Upstream-friendly** - Easy to submit as PR later

## Build System Integration

### CMake (Recommended)

The patch is automatically applied by CMake when building HashLink from source.

**CMakeLists.txt integration:**
```cmake
# Apply HashLink patch for obj_resolve_field
execute_process(
    COMMAND git apply --check ${CMAKE_SOURCE_DIR}/hashlink_obj_resolve_field.patch
    WORKING_DIRECTORY ${HASHLINK_DIR}
    RESULT_VARIABLE PATCH_CHECK
    OUTPUT_QUIET ERROR_QUIET
)
if(PATCH_CHECK EQUAL 0)
    execute_process(
        COMMAND git apply ${CMAKE_SOURCE_DIR}/hashlink_obj_resolve_field.patch
        WORKING_DIRECTORY ${HASHLINK_DIR}
    )
endif()
```

### Manual Build

If building without CMake:

```bash
# 1. Apply patch
cd vendor/hashlink
patch -p1 < ../../hashlink_obj_resolve_field.patch

# 2. Build HashLink
make

# 3. Build HLFFI
cd ../..
make
```

## Testing the Patch

Verify the patch works by running Phase 3 tests:

```bash
./test_static test/game.hl
```

**Expected output:**
```
--- Test 1: Get Static Int Field ---
✓ Game.score = 100

--- Test 2: Get Static String Field ---
✓ Game.playerName = "TestPlayer"
```

If these tests fail, the patch may not be applied correctly.

## Future: Upstream Contribution

This change is a candidate for upstreaming to HaxeFoundation/hashlink:

**Proposal:** Make `obj_resolve_field()` part of the public API (remove `static`)

**Benefits:**
- Enables FFI libraries like HLFFI to access field resolution
- No breaking changes (only adds visibility)
- Useful for other embedding use cases

**PR Status:** Not yet submitted
**Tracking:** TBD

## Troubleshooting

### "patch does not apply"

The HashLink version may have diverged. Check the obj.c file manually:

```bash
cd vendor/hashlink
git diff src/std/obj.c
```

If `obj_resolve_field` is already non-static, the patch is already applied or not needed.

### "obj_resolve_field undefined reference"

The patch wasn't applied before building. Apply it and rebuild:

```bash
cd vendor/hashlink
patch -p1 < ../../hashlink_obj_resolve_field.patch
make clean && make
```

### Patch conflicts with upstream

If HashLink upstream changes obj.c:

1. Update to latest HashLink: `cd vendor/hashlink && git pull`
2. Regenerate patch manually if needed
3. Report issue at: https://github.com/LogicInteractive/hlffi/issues
