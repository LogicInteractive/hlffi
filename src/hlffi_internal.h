/**
 * HLFFI Internal Header
 *
 * Contains internal structure definitions shared across all HLFFI source files.
 * This header is NOT part of the public API.
 */

#ifndef HLFFI_INTERNAL_H
#define HLFFI_INTERNAL_H

#include "../include/hlffi.h"
#include <hl.h>
#include <hlmodule.h>
#include <string.h>

/* Maximum number of registered callbacks */
#define HLFFI_MAX_CALLBACKS 64

/* Callback entry storage */
typedef struct {
    char name[64];
    hlffi_native_func c_func;
    int nargs;
    vclosure* hl_closure;
    bool is_rooted;
    struct hlffi_vm* vm;  /* VM pointer for wrapper access */
} hlffi_callback_entry;

/**
 * Internal VM structure.
 *
 * IMPORTANT: This definition MUST be kept in sync across all source files!
 * All HLFFI source files must include this header instead of defining
 * their own struct hlffi_vm.
 */
struct hlffi_vm {
    /* HashLink module and code */
    hl_module* module;
    hl_code* code;

    /* Integration mode */
    hlffi_integration_mode integration_mode;

    /* Persistent context for stack_top (CRITICAL: must persist!) */
    /* This is passed to hl_register_thread and MUST remain valid */
    void* stack_context;

    /* Error state */
    char error_msg[512];
    hlffi_error_code last_error;

    /* Initialization flags */
    bool hl_initialized;
    bool thread_registered;
    bool module_loaded;
    bool entry_called;

    /* Hot reload support */
    bool hot_reload_enabled;
    const char* loaded_file;

    /* Phase 6: Callback storage */
    hlffi_callback_entry callbacks[HLFFI_MAX_CALLBACKS];
    int callback_count;

    /* Phase 6: Exception storage */
    char exception_msg[512];
    char exception_stack[2048];

    /* Phase 1: Threading state (for THREADED mode) */
    void* thread_handle;        /* pthread_t* */
    void* thread_mutex;         /* pthread_mutex_t* */
    void* thread_cond_var;      /* pthread_cond_t* */
    void* thread_response_cond; /* pthread_cond_t* for sync responses */
    void* message_queue;        /* hlffi_thread_message_queue* */
    bool thread_running;
    bool thread_should_stop;
};

/**
 * Internal value structure.
 *
 * IMPORTANT: This definition MUST be kept in sync across all source files!
 */
struct hlffi_value {
    vdynamic* hl_value;
    bool is_rooted;  /* Track if we added a GC root */
};

/* ========== INTERNAL GC STACK FIX ========== */

/**
 * Internal macro to update GC stack_top before any GC allocation.
 * This ensures proper stack scanning when HLFFI is embedded.
 *
 * THE PROBLEM:
 * When hl_register_thread() was called in hlffi_init(), we passed a pointer
 * to heap memory (vm->stack_context). The GC uses stack_top to scan the call
 * stack for roots, but heap memory isn't the stack. This caused:
 *   - Debug builds: allocator.c(369) : FATAL ERROR : assert
 *   - Release builds: Random crashes, memory corruption
 *
 * THE FIX:
 * Update stack_top to point to the current stack frame before any GC allocation.
 * This way the GC scans the actual call stack where vdynamic* pointers live.
 *
 * See: docs/GC_STACK_SCANNING.md
 * See: https://github.com/HaxeFoundation/hashlink/issues/752
 */
#define HLFFI_UPDATE_STACK_TOP() \
    do { \
        hl_thread_info* _t = hl_get_thread(); \
        if (_t) { \
            int _stack_marker; \
            _t->stack_top = &_stack_marker; \
        } \
    } while(0)

/* ========== INTERNAL HELPER DECLARATIONS ========== */

/**
 * Internal helper to set error state on VM.
 */
static inline void hlffi_set_error(hlffi_vm* vm, hlffi_error_code code, const char* msg) {
    if (!vm) return;
    vm->last_error = code;
    if (msg) {
        strncpy(vm->error_msg, msg, sizeof(vm->error_msg) - 1);
        vm->error_msg[sizeof(vm->error_msg) - 1] = '\0';
    } else {
        vm->error_msg[0] = '\0';
    }
}

/* HashLink internal function - declared as extern to access it
 * Defined as static in vendor/hashlink/src/std/obj.c:64
 * But exported via our patch in libhl and can be accessed via extern declaration
 * Pattern discovered from working FFI code - this is the KEY to static field access!
 */
extern hl_field_lookup* obj_resolve_field(hl_type_obj *o, int hfield);

#endif /* HLFFI_INTERNAL_H */
