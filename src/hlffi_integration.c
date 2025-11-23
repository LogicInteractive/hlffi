/**
 * HLFFI Integration Mode Implementation
 * Non-threaded and threaded mode support
 * Phase 1 implementation
 */

#include "../include/hlffi.h"
#include <hl.h>
#include <hlmodule.h>
#include <stdlib.h>
#include <string.h>

/* External VM structure (defined in hlffi_lifecycle.c) */
struct hlffi_vm;

/* ========== INTEGRATION MODE MANAGEMENT ========== */

hlffi_error_code hlffi_set_integration_mode(hlffi_vm* vm, hlffi_integration_mode mode) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    if (mode != HLFFI_MODE_NON_THREADED && mode != HLFFI_MODE_THREADED) {
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    /* TODO: In Phase 1, we only support NON_THREADED mode
     * THREADED mode will be implemented later */
    if (mode == HLFFI_MODE_THREADED) {
        return HLFFI_ERROR_NOT_IMPLEMENTED;
    }

    /* Note: vm->integration_mode = mode would require exposing the struct */
    /* For now, just validate the mode */

    return HLFFI_OK;
}

hlffi_integration_mode hlffi_get_integration_mode(hlffi_vm* vm) {
    if (!vm) return HLFFI_MODE_NON_THREADED;

    /* TODO: return vm->integration_mode */
    return HLFFI_MODE_NON_THREADED;
}

/* ========== NON-THREADED MODE (Mode 1) ========== */

hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    /* TODO: Process event loops
     * This will be implemented in hlffi_events.c
     * For now, just return success */

    (void)delta_time; /* Unused for now */

    return HLFFI_OK;
}

bool hlffi_has_pending_work(hlffi_vm* vm) {
    if (!vm) return false;

    /* TODO: Check if event loops have pending work
     * This will be implemented in hlffi_events.c
     * For now, return false */

    return false;
}

/* ========== THREADED MODE (Mode 2) ========== */

hlffi_error_code hlffi_thread_start(hlffi_vm* vm) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    /* TODO: Implement threaded mode
     * Will create dedicated VM thread with message queue */

    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

hlffi_error_code hlffi_thread_stop(hlffi_vm* vm) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    /* TODO: Implement threaded mode stop */

    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

hlffi_error_code hlffi_thread_call_sync(hlffi_vm* vm, hlffi_thread_func callback, void* userdata) {
    if (!vm) return HLFFI_ERROR_NULL_VM;
    if (!callback) return HLFFI_ERROR_INVALID_ARGUMENT;

    /* TODO: Implement sync call to VM thread */

    (void)userdata;

    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

hlffi_error_code hlffi_thread_call_async(hlffi_vm* vm, hlffi_thread_func callback,
                                          hlffi_thread_async_callback on_complete, void* userdata) {
    if (!vm) return HLFFI_ERROR_NULL_VM;
    if (!callback) return HLFFI_ERROR_INVALID_ARGUMENT;

    /* TODO: Implement async call to VM thread */

    (void)on_complete;
    (void)userdata;

    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

/* ========== WORKER THREAD HELPERS ========== */

void hlffi_worker_register(void) {
    /* TODO: Register worker thread with HashLink GC
     * Will call hl_register_thread() for the calling thread */
}

void hlffi_worker_unregister(void) {
    /* TODO: Unregister worker thread
     * Will call hl_unregister_thread() for the calling thread */
}

/* ========== BLOCKING OPERATION HELPERS ========== */

void hlffi_blocking_begin(void) {
    /* TODO: Mark beginning of blocking operation
     * Allows GC to know this thread may block */
}

void hlffi_blocking_end(void) {
    /* TODO: Mark end of blocking operation */
}
