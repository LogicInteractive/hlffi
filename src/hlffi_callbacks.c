/**
 * HLFFI Callbacks & Exceptions Implementation
 * Phase 6: Bidirectional C↔Haxe calls, exception handling, external blocking wrappers
 */

#include "../include/hlffi.h"
#include <hl.h>
#include <hlmodule.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_CALLBACKS 64

/* Callback entry storage */
typedef struct {
    char name[64];
    hlffi_native_func c_func;
    int nargs;
    vclosure* hl_closure;
    bool is_rooted;
} callback_entry;

/* Extended VM structure with callbacks */
struct hlffi_vm {
    /* HashLink module and code */
    hl_module* module;
    hl_code* code;

    /* Integration mode */
    hlffi_integration_mode integration_mode;

    /* Persistent context for stack_top */
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
    callback_entry callbacks[MAX_CALLBACKS];
    int callback_count;

    /* Phase 6: Exception storage */
    char exception_msg[512];
    char exception_stack[2048];
};

/* hlffi_value structure (also defined in other source files) */
struct hlffi_value {
    vdynamic* hl_value;
    bool is_rooted;  /* Track if we added a GC root */
};

/* ========== HELPER FUNCTIONS ========== */

static void set_error(hlffi_vm* vm, const char* msg) {
    if (!vm) return;
    if (msg) {
        strncpy(vm->error_msg, msg, sizeof(vm->error_msg) - 1);
        vm->error_msg[sizeof(vm->error_msg) - 1] = '\0';
    } else {
        vm->error_msg[0] = '\0';
    }
}

/* Store exception information from vdynamic */
static void store_exception(hlffi_vm* vm, vdynamic* exception) {
    if (!vm || !exception) return;

    /* Get exception as string */
    uchar* exc_str = hl_to_string(exception);
    if (exc_str) {
        char* utf8_msg = hl_to_utf8(exc_str);
        if (utf8_msg) {
            strncpy(vm->exception_msg, utf8_msg, sizeof(vm->exception_msg) - 1);
            vm->exception_msg[sizeof(vm->exception_msg) - 1] = '\0';
        }
    }

    /* Get stack trace using hl_print_uncaught_exception output */
    /* For now, just store the message - full stack trace requires more work */
    snprintf(vm->exception_stack, sizeof(vm->exception_stack),
             "Exception: %s\n(Full stack trace not yet implemented)",
             vm->exception_msg);
}

/* Native function wrapper - bridges C callback to HashLink */
static vdynamic* native_wrapper(void* user_data, vdynamic** args, int nargs) {
    callback_entry* entry = (callback_entry*)user_data;
    if (!entry || !entry->c_func) {
        return NULL;
    }

    /* Get VM from... wait, we need VM context */
    /* TODO: Need to pass VM context somehow */
    /* For now, we'll need to store VM pointer in callback_entry */

    /* Convert vdynamic** to hlffi_value** */
    hlffi_value** hlffi_args = NULL;
    if (nargs > 0) {
        hlffi_args = (hlffi_value**)malloc(nargs * sizeof(hlffi_value*));
        for (int i = 0; i < nargs; i++) {
            hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
            wrapped->hl_value = args[i];
            wrapped->is_rooted = false;  /* Don't root args - they're temporary */
            hlffi_args[i] = wrapped;
        }
    }

    /* Call C function */
    /* TODO: Need VM pointer - will add to callback_entry */
    hlffi_value* result = NULL; /* entry->c_func(entry->vm, nargs, hlffi_args); */

    /* Clean up arg wrappers */
    if (hlffi_args) {
        for (int i = 0; i < nargs; i++) {
            free(hlffi_args[i]);
        }
        free(hlffi_args);
    }

    /* Return result */
    return result ? result->hl_value : NULL;
}

/* ========== PUBLIC API ========== */

/* ========== CALLBACKS (NOT IMPLEMENTED - Phase 6 Future Work) ========== */

/*
 * CALLBACKS ARE NOT YET IMPLEMENTED
 *
 * Implementing C→Haxe callbacks requires:
 * 1. Dynamic hl_type creation for function signatures
 * 2. vclosure allocation and setup
 * 3. Native wrapper bridge with proper context passing
 * 4. GC root management for closures
 *
 * This is complex and deferred to Phase 6b.
 * Current priority: Exception handling (implemented below).
 *
 * Estimated implementation time: 3-4 hours
 */

bool hlffi_register_callback(hlffi_vm* vm, const char* name, hlffi_native_func func, int nargs) {
    (void)name; (void)func; (void)nargs;  /* Suppress unused warnings */

    if (!vm) return false;
    set_error(vm, "Callbacks not yet implemented - Phase 6 future work");
    return false;
}

hlffi_value* hlffi_get_callback(hlffi_vm* vm, const char* name) {
    (void)name;  /* Suppress unused warning */

    if (!vm) return NULL;
    set_error(vm, "Callbacks not yet implemented - Phase 6 future work");
    return NULL;
}

/* ========== EXCEPTION HANDLING (IMPLEMENTED) ========== */

hlffi_call_result hlffi_try_call_static(
    hlffi_vm* vm,
    const char* class_name,
    const char* method_name,
    int argc,
    hlffi_value** argv,
    hlffi_value** out_result,
    const char** out_error
) {
    if (!vm || !class_name || !method_name) {
        if (out_error) *out_error = "Invalid arguments";
        if (out_result) *out_result = NULL;
        return HLFFI_CALL_ERROR;
    }

    /* Clear previous exception state */
    vm->exception_msg[0] = '\0';
    vm->exception_stack[0] = '\0';

    /* Call the normal hlffi_call_static */
    /* The function already uses hl_dyn_call_safe internally */
    hlffi_value* result = hlffi_call_static(vm, class_name, method_name, argc, argv);

    /* Check if an exception occurred by looking at the error code */
    if (!result && vm->last_error == HLFFI_ERROR_EXCEPTION_THROWN) {
        /* Exception was thrown */
        /* Note: The actual exception object was returned by hl_dyn_call_safe,
         * but hlffi_call_static currently doesn't store it.
         * For now, we return the error message from the VM.
         */
        if (out_result) *out_result = NULL;
        if (out_error) {
            /* Store exception message */
            const char* err_msg = hlffi_get_error(vm);
            if (err_msg && err_msg[0]) {
                strncpy(vm->exception_msg, err_msg, sizeof(vm->exception_msg) - 1);
                vm->exception_msg[sizeof(vm->exception_msg) - 1] = '\0';
                *out_error = vm->exception_msg;
            } else {
                *out_error = "Exception thrown (no message)";
            }
        }
        return HLFFI_CALL_EXCEPTION;
    } else if (!result) {
        /* Regular error (not exception) */
        if (out_result) *out_result = NULL;
        if (out_error) {
            const char* err_msg = hlffi_get_error(vm);
            *out_error = err_msg ? err_msg : "Unknown error";
        }
        return HLFFI_CALL_ERROR;
    }

    /* Success */
    if (out_result) *out_result = result;
    if (out_error) *out_error = NULL;
    return HLFFI_CALL_OK;
}

hlffi_call_result hlffi_try_call_method(
    hlffi_value* obj,
    const char* method_name,
    int argc,
    hlffi_value** argv,
    hlffi_value** out_result,
    const char** out_error
) {
    if (!obj || !method_name) {
        if (out_error) *out_error = "Invalid arguments";
        if (out_result) *out_result = NULL;
        return HLFFI_CALL_ERROR;
    }

    /* Get VM from obj (we need to store it in hlffi_value) */
    /* For now, we can't distinguish exceptions from errors without VM context */
    /* This is a limitation - we need VM pointer in hlffi_value for proper exception handling */

    /* Call the normal hlffi_call_method */
    hlffi_value* result = hlffi_call_method(obj, method_name, argc, argv);

    if (!result) {
        /* Could be exception or error, but we can't distinguish without VM */
        if (out_result) *out_result = NULL;
        if (out_error) *out_error = "Method call failed (cannot distinguish exception without VM context)";
        return HLFFI_CALL_ERROR;
    }

    /* Success */
    if (out_result) *out_result = result;
    if (out_error) *out_error = NULL;
    return HLFFI_CALL_OK;
}

const char* hlffi_get_exception_message(hlffi_vm* vm) {
    if (!vm) return NULL;
    return vm->exception_msg[0] ? vm->exception_msg : NULL;
}

const char* hlffi_get_exception_stack(hlffi_vm* vm) {
    if (!vm) return NULL;
    return vm->exception_stack[0] ? vm->exception_stack : NULL;
}

void hlffi_blocking_begin(void) {
    /* Notify GC that we're entering blocking code */
    hl_blocking(true);
}

void hlffi_blocking_end(void) {
    /* Notify GC that we're back under HL control */
    hl_blocking(false);
}
