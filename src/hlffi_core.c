/**
 * HLFFI Core Implementation
 * Version information and utilities
 */

#include "hlffi.h"
#include <stdio.h>
#include <string.h>

/* ========== VERSION ========== */

const char* hlffi_get_version(void) {
    return HLFFI_VERSION_STRING;
}

const char* hlffi_get_hl_version(void) {
    /* TODO: Query HashLink version */
    return "unknown";
}

bool hlffi_is_jit_mode(void) {
    /* TODO: Detect JIT vs HL/C mode */
#ifdef HLML_JIT
    return true;
#else
    return false;
#endif
}
