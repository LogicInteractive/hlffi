/**
 * HLFFI Event Loop Implementation
 * UV and haxe.EventLoop integration with weak symbols
 * Phase 1 implementation
 */

#include "hlffi.h"

/* TODO: Phase 1 - Implement event loop integration */

hlffi_error_code hlffi_process_events(hlffi_vm* vm, hlffi_eventloop_type type) {
    return HLFFI_ERROR_NOT_IMPLEMENTED;
}

bool hlffi_has_pending_events(hlffi_vm* vm, hlffi_eventloop_type type) {
    return false;
}

void hlffi_worker_register(void) {
    /* TODO: Register worker thread with HashLink */
}

void hlffi_worker_unregister(void) {
    /* TODO: Unregister worker thread */
}

void hlffi_blocking_begin(void) {
    /* TODO: Call hl_blocking(true) */
}

void hlffi_blocking_end(void) {
    /* TODO: Call hl_blocking(false) */
}
