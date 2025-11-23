/**
 * HLFFI Hot Reload Implementation
 * Runtime code reloading without restart
 * Phase 1 implementation
 */

#include "hlffi.h"

/* TODO: Phase 1 - Implement hot reload */

hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

bool hlffi_is_hot_reload_enabled(hlffi_vm* vm) {
    return false;
}

hlffi_error_code hlffi_reload_module(hlffi_vm* vm, const char* path) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

hlffi_error_code hlffi_reload_module_memory(hlffi_vm* vm, const void* data, size_t size) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

void hlffi_set_reload_callback(hlffi_vm* vm, hlffi_reload_callback callback, void* userdata) {
    /* TODO: Store callback */
}
