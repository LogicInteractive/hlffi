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

bool hlffi_register_callback(hlffi_vm* vm, const char* name, hlffi_native_func func, int nargs) {
    if (!vm || !name || !func) {
        set_error(vm, "Invalid arguments to hlffi_register_callback");
        return false;
    }

    if (vm->callback_count >= MAX_CALLBACKS) {
        set_error(vm, "Maximum number of callbacks reached");
        return false;
    }

    /* Find or create callback entry */
    callback_entry* entry = NULL;
    for (int i = 0; i < vm->callback_count; i++) {
        if (strcmp(vm->callbacks[i].name, name) == 0) {
            entry = &vm->callbacks[i];
            break;
        }
    }

    if (!entry) {
        entry = &vm->callbacks[vm->callback_count++];
        memset(entry, 0, sizeof(callback_entry));
        strncpy(entry->name, name, sizeof(entry->name) - 1);
    }

    /* Store C function */
    entry->c_func = func;
    entry->nargs = nargs;

    /* Create HashLink closure wrapping the C function */
    /* TODO: Implement proper vclosure creation */
    /* This is complex and requires:
     * 1. Creating a hl_type for the function signature
     * 2. Creating a vclosure pointing to native_wrapper
     * 3. Setting up user_data to point to callback_entry
     * 4. Adding GC root
     */

    entry->hl_closure = NULL;  /* TODO */
    entry->is_rooted = false;

    return true;
}

hlffi_value* hlffi_get_callback(hlffi_vm* vm, const char* name) {
    if (!vm || !name) return NULL;

    /* Find callback */
    for (int i = 0; i < vm->callback_count; i++) {
        if (strcmp(vm->callbacks[i].name, name) == 0) {
            callback_entry* entry = &vm->callbacks[i];

            if (!entry->hl_closure) {
                set_error(vm, "Callback closure not created");
                return NULL;
            }

            /* Wrap closure in hlffi_value */
            hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
            if (!wrapped) return NULL;

            wrapped->hl_value = (vdynamic*)entry->hl_closure;
            wrapped->is_rooted = true;  /* Callback is GC-rooted */

            return wrapped;
        }
    }

    set_error(vm, "Callback not found");
    return NULL;
}

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

    /* Use existing hlffi_call_static but with exception handling */
    /* The key is using hl_dyn_call_safe which sets isException flag */

    /* TODO: This requires refactoring hlffi_call_static to expose the
     * hl_dyn_call_safe call so we can check isException.
     * For now, implement inline:
     */

    /* Find class type */
    /* Find method */
    /* Call with hl_dyn_call_safe */
    /* Check isException flag */

    if (out_result) *out_result = NULL;
    if (out_error) *out_error = "Not yet implemented";
    return HLFFI_CALL_ERROR;
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

    /* TODO: Implement with exception handling */

    if (out_result) *out_result = NULL;
    if (out_error) *out_error = "Not yet implemented";
    return HLFFI_CALL_ERROR;
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
