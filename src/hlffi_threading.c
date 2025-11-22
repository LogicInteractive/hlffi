/**
 * HLFFI Threading Implementation
 * Dedicated VM thread with message queue
 * Phase 1 implementation
 */

#include "hlffi.h"

/* TODO: Phase 1 - Implement threaded mode */

hlffi_error_code hlffi_thread_start(hlffi_vm* vm) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

hlffi_error_code hlffi_thread_stop(hlffi_vm* vm) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

bool hlffi_thread_is_running(hlffi_vm* vm) {
    return false;
}

hlffi_error_code hlffi_thread_call_sync(hlffi_vm* vm, hlffi_thread_func func, void* userdata) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

hlffi_error_code hlffi_thread_call_async(hlffi_vm* vm, hlffi_thread_func func,
                                          hlffi_thread_async_callback callback, void* userdata) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}
