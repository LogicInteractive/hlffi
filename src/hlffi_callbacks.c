/**
 * HLFFI Callbacks & Exceptions Implementation
 * Phase 6: Bidirectional C↔Haxe calls, exception handling, external blocking wrappers
 */

#include "hlffi_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

/* Helper: Convert vdynamic** args to hlffi_value** */
static hlffi_value** convert_args(hlffi_vm* vm, vdynamic** hl_args, int nargs) {
    if (nargs == 0) return NULL;

    hlffi_value** hlffi_args = (hlffi_value**)malloc(nargs * sizeof(hlffi_value*));
    if (!hlffi_args) return NULL;

    for (int i = 0; i < nargs; i++) {
        hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
        if (!wrapped) {
            /* Cleanup on allocation failure */
            for (int j = 0; j < i; j++) {
                free(hlffi_args[j]);
            }
            free(hlffi_args);
            return NULL;
        }
        wrapped->hl_value = hl_args[i];
        wrapped->is_rooted = false;  /* Don't root args - they're temporary */
        hlffi_args[i] = wrapped;
    }
    return hlffi_args;
}

/* Helper: Free converted args */
static void free_args(hlffi_value** args, int nargs) {
    if (!args) return;
    for (int i = 0; i < nargs; i++) {
        free(args[i]);
    }
    free(args);
}

/* Native function wrappers - bridge C callback to HashLink calling conventions
 * HashLink uses specific calling conventions: fun(arg1, arg2, ...) not fun(closure, args[])
 * We need separate wrappers for each arity (0-4 args) */

static vdynamic* native_wrapper0(hlffi_callback_entry* entry) {
    if (!entry || !entry->c_func || !entry->vm) return NULL;
    hlffi_value* result = entry->c_func(entry->vm, 0, NULL);
    return result ? result->hl_value : NULL;
}

static vdynamic* native_wrapper1(hlffi_callback_entry* entry, vdynamic* a0) {
    if (!entry || !entry->c_func || !entry->vm) return NULL;
    vdynamic* hl_args[] = {a0};
    hlffi_value** args = convert_args(entry->vm, hl_args, 1);
    if (!args) return NULL;
    hlffi_value* result = entry->c_func(entry->vm, 1, args);
    free_args(args, 1);
    return result ? result->hl_value : NULL;
}

static vdynamic* native_wrapper2(hlffi_callback_entry* entry, vdynamic* a0, vdynamic* a1) {
    if (!entry || !entry->c_func || !entry->vm) return NULL;
    vdynamic* hl_args[] = {a0, a1};
    hlffi_value** args = convert_args(entry->vm, hl_args, 2);
    if (!args) return NULL;
    hlffi_value* result = entry->c_func(entry->vm, 2, args);
    free_args(args, 2);
    return result ? result->hl_value : NULL;
}

static vdynamic* native_wrapper3(hlffi_callback_entry* entry, vdynamic* a0, vdynamic* a1, vdynamic* a2) {
    if (!entry || !entry->c_func || !entry->vm) return NULL;
    vdynamic* hl_args[] = {a0, a1, a2};
    hlffi_value** args = convert_args(entry->vm, hl_args, 3);
    if (!args) return NULL;
    hlffi_value* result = entry->c_func(entry->vm, 3, args);
    free_args(args, 3);
    return result ? result->hl_value : NULL;
}

static vdynamic* native_wrapper4(hlffi_callback_entry* entry, vdynamic* a0, vdynamic* a1, vdynamic* a2, vdynamic* a3) {
    if (!entry || !entry->c_func || !entry->vm) return NULL;
    vdynamic* hl_args[] = {a0, a1, a2, a3};
    hlffi_value** args = convert_args(entry->vm, hl_args, 4);
    if (!args) return NULL;
    hlffi_value* result = entry->c_func(entry->vm, 4, args);
    free_args(args, 4);
    return result ? result->hl_value : NULL;
}

/* Get wrapper function for given arity */
static void* get_wrapper_for_arity(int nargs) {
    switch (nargs) {
        case 0: return (void*)native_wrapper0;
        case 1: return (void*)native_wrapper1;
        case 2: return (void*)native_wrapper2;
        case 3: return (void*)native_wrapper3;
        case 4: return (void*)native_wrapper4;
        default: return NULL;  /* >4 args not supported yet */
    }
}

/* ========== PUBLIC API ========== */

/* ========== CALLBACKS (Phase 6b Implementation) ========== */

/* Helper: Create a function type for callback with specified arity
 * This creates a dynamic hl_type representing: (closure_value, arg0, arg1, ...) -> vdynamic
 *
 * IMPORTANT: HashLink's hl_alloc_closure_ptr requires the function type to have
 * at least 1 argument (the closure value). So for a callback with N args,
 * we create a function type with N+1 args (closure value + N actual args). */
static hl_type* create_callback_function_type(int nargs) {
    /* Allocate hl_type structure */
    hl_type* type = (hl_type*)malloc(sizeof(hl_type));
    if (!type) return NULL;
    memset(type, 0, sizeof(hl_type));

    /* Set kind to HFUN (function) */
    type->kind = HFUN;

    /* Allocate hl_type_fun structure */
    hl_type_fun* tfun = (hl_type_fun*)malloc(sizeof(hl_type_fun));
    if (!tfun) {
        free(type);
        return NULL;
    }
    memset(tfun, 0, sizeof(hl_type_fun));

    /* Function signature: (closure_value, arg0, ..., argN-1) -> vdynamic
     * Total args = 1 (closure) + nargs (actual args) */
    int total_args = 1 + nargs;
    tfun->nargs = total_args;
    tfun->ret = &hlt_dyn;  /* Return type: vdynamic (flexible) */
    tfun->parent = type;   /* Back-reference to parent type */

    /* Allocate args array */
    tfun->args = (hl_type**)malloc(total_args * sizeof(hl_type*));
    if (!tfun->args) {
        free(tfun);
        free(type);
        return NULL;
    }

    /* All args are vdynamic (first is closure value, rest are actual args) */
    for (int i = 0; i < total_args; i++) {
        tfun->args[i] = &hlt_dyn;
    }

    /* Link function descriptor to type */
    type->fun = tfun;

    return type;
}

/* Helper: Free function type */
static void free_function_type(hl_type* type) {
    if (!type) return;
    if (type->fun) {
        if (type->fun->args) {
            free(type->fun->args);
        }
        free(type->fun);
    }
    free(type);
}

bool hlffi_register_callback(hlffi_vm* vm, const char* name, hlffi_native_func func, int nargs) {
    if (!vm) return false;
    if (!name || !func) {
        set_error(vm, "Invalid callback name or function");
        return false;
    }
    if (nargs < 0 || nargs > 4) {
        set_error(vm, "Callback arity must be 0-4 arguments");
        return false;
    }
    if (vm->callback_count >= HLFFI_MAX_CALLBACKS) {
        set_error(vm, "Maximum number of callbacks reached");
        return false;
    }

    /* Check for duplicate name */
    for (int i = 0; i < vm->callback_count; i++) {
        if (strcmp(vm->callbacks[i].name, name) == 0) {
            set_error(vm, "Callback with this name already registered");
            return false;
        }
    }

    /* Get the callback entry slot */
    hlffi_callback_entry* entry = &vm->callbacks[vm->callback_count];

    /* Store callback info */
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->c_func = func;
    entry->nargs = nargs;
    entry->vm = vm;
    entry->is_rooted = false;
    entry->hl_closure = NULL;

    /* Create function type */
    hl_type* func_type = create_callback_function_type(nargs);
    if (!func_type) {
        set_error(vm, "Failed to create callback function type");
        return false;
    }

    /* Get wrapper function for this arity */
    void* wrapper_func = get_wrapper_for_arity(nargs);
    if (!wrapper_func) {
        free_function_type(func_type);
        set_error(vm, "Unsupported callback arity (max 4 args)");
        return false;
    }

    /* Update GC stack_top before allocation */
    HLFFI_UPDATE_STACK_TOP();

    /* Create closure using hl_alloc_closure_ptr
     * This creates a closure that wraps our C function */
    vclosure* closure = hl_alloc_closure_ptr(func_type, wrapper_func, entry);
    if (!closure) {
        free_function_type(func_type);
        set_error(vm, "Failed to allocate closure");
        return false;
    }

    /* Add GC root to prevent collection */
    hl_add_root(&entry->hl_closure);
    entry->hl_closure = closure;
    entry->is_rooted = true;

    /* Increment callback count */
    vm->callback_count++;

    return true;
}

hlffi_value* hlffi_get_callback(hlffi_vm* vm, const char* name) {
    if (!vm || !name) return NULL;

    /* Find callback by name */
    for (int i = 0; i < vm->callback_count; i++) {
        if (strcmp(vm->callbacks[i].name, name) == 0) {
            hlffi_callback_entry* entry = &vm->callbacks[i];

            /* Wrap the closure in hlffi_value */
            hlffi_value* value = (hlffi_value*)malloc(sizeof(hlffi_value));
            if (!value) {
                set_error(vm, "Failed to allocate value wrapper");
                return NULL;
            }

            value->hl_value = (vdynamic*)entry->hl_closure;
            value->is_rooted = true;  /* Already rooted in callback table */

            return value;
        }
    }

    set_error(vm, "Callback not found");
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
