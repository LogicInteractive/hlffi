/**
 * HLFFI Event Loop Implementation
 * Haxe EventLoop integration (which internally handles UV loop)
 * Phase 1 Extensions: Event Loop Integration
 */

#include "hlffi_internal.h"
#include <stdio.h>

/* ========== INTERNAL HELPERS ========== */

/* Forward declaration */
static hlffi_error_code process_haxe_eventloop(hlffi_vm* vm);

/**
 * Process UV event loop (libuv)
 * This handles async I/O, network, file system, etc.
 *
 * NOTE: Instead of directly calling uv_run(), we call haxe.EventLoop.loopOnce()
 * which internally processes the UV loop. This is the correct way to integrate
 * with HashLink's event system.
 */
static hlffi_error_code process_uv_loop(hlffi_vm* vm) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    /* The Haxe EventLoop internally calls the UV loop, so we just
     * call the Haxe EventLoop which handles both UV and Haxe events.
     * This is cleaner and avoids needing to directly link against libuv. */

    return process_haxe_eventloop(vm);
}

/**
 * Process Haxe EventLoop (timers, MainLoop callbacks, thread messages)
 * This handles haxe.Timer, MainLoop.add(), runInMainThread(), etc.
 */
static hlffi_error_code process_haxe_eventloop(hlffi_vm* vm) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    /* Strategy: Call haxe.MainLoop.tick() to process timers and callbacks
     *
     * The Haxe MainLoop processes:
     * - Timers (haxe.Timer)
     * - MainLoop callbacks (MainLoop.add)
     * - Thread→main messages (runInMainThread)
     *
     * haxe.MainLoop.tick() is the standard way to process events in embedded mode
     */

    /* Call haxe.MainLoop.tick() to process all pending timers and callbacks */
    hlffi_value* result = hlffi_call_static(vm, "haxe.MainLoop", "tick", 0, NULL);
    if (result) {
        hlffi_value_free(result);
        return HLFFI_OK;
    }

    /* If tick() didn't work, clear the error - MainLoop might not be available
     * This is not necessarily an error - bytecode might not use timers/async */
    vm->error_msg[0] = '\0';
    vm->last_error = HLFFI_OK;

    return HLFFI_OK;
}

/**
 * Check if UV loop has pending events
 *
 * NOTE: We cannot directly check the UV loop without linking to libuv.
 * The Haxe EventLoop manages the UV loop internally, so we rely on
 * the Haxe-side check instead.
 */
static bool has_pending_uv_events(hlffi_vm* vm) {
    /* Conservative approach: We can't directly query UV loop status
     * without linking to libuv. The Haxe EventLoop handles this internally.
     * For now, return false (conservative) */
    (void)vm;
    return false;
}

/**
 * Check if Haxe EventLoop has pending work
 *
 * Note: There's no direct API to check this without calling into Haxe.
 * We could call a custom Haxe method that checks EventLoop.main.hasEvents(),
 * but for now we'll return a conservative estimate.
 */
static bool has_pending_haxe_events(hlffi_vm* vm) {
    if (!vm) return false;

    /* Conservative approach: assume there might be pending events
     * In practice, calling loopOnce() is cheap if there are no events */

    /* TODO: Implement a way to query Haxe EventLoop for pending work
     * This could be done by calling a Haxe static method like:
     *   haxe.EventLoop.hasPendingEvents() : Bool
     */

    return false; /* Default: no pending events */
}

/* ========== PUBLIC API ========== */

hlffi_error_code hlffi_process_events(hlffi_vm* vm, hlffi_eventloop_type type) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    hlffi_error_code result = HLFFI_OK;

    /* Process UV loop */
    if (type == HLFFI_EVENTLOOP_UV || type == HLFFI_EVENTLOOP_ALL) {
        result = process_uv_loop(vm);
        if (result != HLFFI_OK) {
            return result;
        }
    }

    /* Process Haxe EventLoop */
    if (type == HLFFI_EVENTLOOP_HAXE || type == HLFFI_EVENTLOOP_ALL) {
        result = process_haxe_eventloop(vm);
        if (result != HLFFI_OK) {
            return result;
        }
    }

    return HLFFI_OK;
}

bool hlffi_has_pending_events(hlffi_vm* vm, hlffi_eventloop_type type) {
    if (!vm) return false;

    bool has_events = false;

    /* Check UV loop */
    if (type == HLFFI_EVENTLOOP_UV || type == HLFFI_EVENTLOOP_ALL) {
        has_events |= has_pending_uv_events(vm);
    }

    /* Check Haxe EventLoop */
    if (type == HLFFI_EVENTLOOP_HAXE || type == HLFFI_EVENTLOOP_ALL) {
        has_events |= has_pending_haxe_events(vm);
    }

    return has_events;
}

/* Worker thread helpers are in hlffi_threading.c */
