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
    int file_time;  /* Last modification time for auto-reload check */
    hlffi_reload_callback reload_callback;
    void* reload_userdata;

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
 * Internal helper to update GC stack_top before any GC allocation.
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
 * Update stack_top to point to the caller's stack frame before any GC allocation.
 * We use a static inline function so the stack_marker lives in the caller's frame.
 *
 * See: docs/GC_STACK_SCANNING.md
 * See: https://github.com/HaxeFoundation/hashlink/issues/752
 */
static inline void hlffi_internal_update_stack_top(void* stack_addr) {
    hl_thread_info* t = hl_get_thread();
    if (t) {
        t->stack_top = stack_addr;
    }
}

/*
 * CRITICAL: This macro updates stack_top to the current function's stack.
 * The variable declaration happens in the CALLER'S scope (not inside do-while).
 * This ensures stack_top points to valid memory for the duration of the calling function.
 *
 * Usage: Place at the START of any function that calls HashLink APIs.
 */
#define HLFFI_UPDATE_STACK_TOP() \
    int _hlffi_stack_marker_; \
    hlffi_internal_update_stack_top(&_hlffi_stack_marker_)

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

/* HashLink internal function for field lookup.
 *
 * This function is normally static in vendor/hashlink/src/std/obj.c, but can be
 * exported via a patch to libhl. If the patched libhl is available, it will be used
 * via extern linkage. Otherwise, we provide a fallback implementation below.
 *
 * The fallback uses hl_lookup_find() which IS exported from libhl.
 */
#ifdef HLFFI_HAS_PATCHED_LIBHL
extern hl_field_lookup* obj_resolve_field(hl_type_obj *o, int hfield);
#else
/* Fallback implementation using exported hl_lookup_find() */
static inline hl_field_lookup* obj_resolve_field(hl_type_obj *o, int hfield) {
    hl_runtime_obj *rt = o->rt;
    do {
        hl_field_lookup *f = hl_lookup_find(rt->lookup, rt->nlookup, hfield);
        if (f) return f;
        rt = rt->parent;
    } while (rt);
    return NULL;
}
#endif

/* ========== HLC MODE SUPPORT ========== */

/*
 * HLC (HashLink/C) mode compiles Haxe to C instead of bytecode.
 * In HLC mode:
 * - No vm->module->code (bytecode doesn't exist)
 * - Types accessed via extern symbols: t$ClassName
 * - Use Type.resolveClass() and Reflect.* APIs for dynamic access
 * - Hot reload is impossible (code is statically linked)
 *
 * Define HLFFI_HLC_MODE at compile time to enable HLC support.
 */

#ifdef HLFFI_HLC_MODE
    #define HLFFI_HAS_BYTECODE 0
#else
    #define HLFFI_HAS_BYTECODE 1
#endif

#ifdef HLFFI_HLC_MODE

/**
 * Cached references for HLC operations.
 * Initialized once after hlffi_call_entry().
 *
 * In HLC mode, we use Haxe's Type and Reflect APIs to dynamically
 * resolve classes and call methods. This cache stores the necessary
 * references and pre-computed hashes for performance.
 */
typedef struct {
    /* Type class access */
    hl_type* type_class;           /* hl_type for Type class */
    vdynamic* type_global;         /* Type class global instance */

    /* Reflect class access */
    hl_type* reflect_class;        /* hl_type for Reflect class */
    vdynamic* reflect_global;      /* Reflect class global instance */

    /* Pre-computed field hashes for performance */
    int hash_resolveClass;
    int hash_createInstance;
    int hash_field;
    int hash_setField;
    int hash_callMethod;
    int hash_allTypes;
    int hash_values;
    int hash___type__;
    int hash___constructor__;

    bool initialized;
} hlffi_hlc_cache;

/* Global HLC cache - initialized by hlffi_hlc_init() */
extern hlffi_hlc_cache g_hlc;

/**
 * Initialize HLC support.
 * Called automatically by functions that need it, after entry point.
 * Returns HLFFI_OK on success.
 */
hlffi_error_code hlffi_hlc_init(hlffi_vm* vm);

/**
 * Check if HLC is initialized.
 */
static inline bool hlffi_hlc_is_ready(void) {
    return g_hlc.initialized;
}

/**
 * Create a HashLink string from a C string.
 * Used internally for calling Haxe methods that expect String arguments.
 */
vdynamic* hlffi_hlc_create_string(const char* str);

#endif /* HLFFI_HLC_MODE */

/* ========== MODE QUERY FUNCTIONS ========== */

/**
 * Check if running in HLC mode.
 */
static inline bool hlffi_is_hlc_mode(void) {
#ifdef HLFFI_HLC_MODE
    return true;
#else
    return false;
#endif
}

/**
 * Check if hot reload is available (false in HLC).
 */
static inline bool hlffi_hot_reload_available(void) {
#ifdef HLFFI_HLC_MODE
    return false;
#else
    return true;
#endif
}

/**
 * Check if bytecode loading is available (false in HLC).
 */
static inline bool hlffi_bytecode_loading_available(void) {
#ifdef HLFFI_HLC_MODE
    return false;
#else
    return true;
#endif
}

#endif /* HLFFI_INTERNAL_H */
