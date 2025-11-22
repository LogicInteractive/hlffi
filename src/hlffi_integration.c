/**
 * HLFFI Integration Mode Implementation
 * Setup and management of integration modes
 * Phase 1 implementation
 */

#include "hlffi.h"

/* TODO: Phase 1 - Implement integration mode functions */

hlffi_error_code hlffi_set_integration_mode(hlffi_vm* vm, hlffi_integration_mode mode) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

hlffi_integration_mode hlffi_get_integration_mode(hlffi_vm* vm) {
    return HLFFI_MODE_NON_THREADED;
}

hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

bool hlffi_has_pending_work(hlffi_vm* vm) {
    return false;
}
