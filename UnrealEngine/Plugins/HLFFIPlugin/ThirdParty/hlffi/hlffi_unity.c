/**
 * HLFFI Unity Build File for Unreal Engine
 *
 * This file includes all HLFFI source files for single-compilation-unit builds.
 * Include this file in exactly ONE .cpp file with HLFFI_IMPLEMENTATION defined.
 */

// Include all HLFFI source files in correct order
// (hlffi_internal.h is included by each .c file)

#include "src/hlffi_core.c"
#include "src/hlffi_lifecycle.c"
#include "src/hlffi_types.c"
#include "src/hlffi_values.c"
#include "src/hlffi_objects.c"
#include "src/hlffi_cache.c"
#include "src/hlffi_callbacks.c"
#include "src/hlffi_threading.c"
#include "src/hlffi_events.c"
#include "src/hlffi_reload.c"
#include "src/hlffi_integration.c"
#include "src/hlffi_enums.c"
#include "src/hlffi_abstracts.c"
#include "src/hlffi_maps.c"
#include "src/hlffi_bytes.c"
