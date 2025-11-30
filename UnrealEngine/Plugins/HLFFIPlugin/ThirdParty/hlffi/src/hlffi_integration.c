/**
 * HLFFI Integration Mode Implementation
 * Non-threaded and threaded mode support
 * Phase 1 implementation
 */

#include "hlffi_internal.h"
#include <stdlib.h>
#include <string.h>

/* ========== INTEGRATION MODE MANAGEMENT ========== */

hlffi_error_code hlffi_set_integration_mode(hlffi_vm* vm, hlffi_integration_mode mode) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    if (mode != HLFFI_MODE_NON_THREADED && mode != HLFFI_MODE_THREADED) {
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    vm->integration_mode = mode;

    return HLFFI_OK;
}

hlffi_integration_mode hlffi_get_integration_mode(hlffi_vm* vm) {
    if (!vm) return HLFFI_MODE_NON_THREADED;

    return vm->integration_mode;
}

/* ========== NON-THREADED MODE (Mode 1) ========== */

hlffi_error_code hlffi_update(hlffi_vm* vm, float delta_time) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    /* Process both UV and Haxe event loops
     * This processes:
     * - UV async I/O, network, file system (libuv)
     * - Haxe timers, MainLoop callbacks, thread messages (EventLoop)
     */
    hlffi_error_code result = hlffi_process_events(vm, HLFFI_EVENTLOOP_ALL);
    if (result != HLFFI_OK) {
        return result;
    }

    /* delta_time is provided for user convenience but not used internally
     * Host application can pass it to their own update methods */
    (void)delta_time;

    return HLFFI_OK;
}

bool hlffi_has_pending_work(hlffi_vm* vm) {
    if (!vm) return false;

    /* Check if either UV or Haxe event loops have pending work */
    return hlffi_has_pending_events(vm, HLFFI_EVENTLOOP_ALL);
}

/* ========== THREADED MODE (Mode 2) ========== */
/* Thread functions are implemented in hlffi_threading.c */
/* Blocking helpers are implemented in hlffi_callbacks.c */
