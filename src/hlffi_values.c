/**
 * HLFFI Values & Static Members Implementation
 * Phase 3: Value boxing/unboxing, static field access, static method calls
 */

#include "../include/hlffi.h"
#include <hl.h>
#include <hlmodule.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Forward declaration of VM structure */
struct hlffi_vm {
    hl_module* module;
    hl_code* code;
    hlffi_integration_mode integration_mode;
    void* stack_context;
    char error_msg[512];
    hlffi_error_code last_error;
    bool hl_initialized;
    bool thread_registered;
    bool module_loaded;
    bool entry_called;
    bool hot_reload_enabled;
    const char* loaded_file;
};

/* hlffi_value wraps a HashLink vdynamic* */
struct hlffi_value {
    vdynamic* hl_value;
};

/* Helper: Set error */
static void set_error(hlffi_vm* vm, hlffi_error_code code, const char* msg) {
    if (!vm) return;
    vm->last_error = code;
    if (msg) {
        strncpy(vm->error_msg, msg, sizeof(vm->error_msg) - 1);
        vm->error_msg[sizeof(vm->error_msg) - 1] = '\0';
    } else {
        vm->error_msg[0] = '\0';
    }
}

/* ========== VALUE BOXING ========== */

hlffi_value* hlffi_value_int(hlffi_vm* vm, int value) {
    if (!vm) return NULL;

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Box the integer */
    wrapped->hl_value = hl_alloc_dynamic(&hlt_i32);
    wrapped->hl_value->v.i = value;

    return wrapped;
}

hlffi_value* hlffi_value_float(hlffi_vm* vm, double value) {
    if (!vm) return NULL;

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Box the float */
    wrapped->hl_value = hl_alloc_dynamic(&hlt_f64);
    wrapped->hl_value->v.d = value;

    return wrapped;
}

hlffi_value* hlffi_value_bool(hlffi_vm* vm, bool value) {
    if (!vm) return NULL;

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Box the boolean */
    wrapped->hl_value = hl_alloc_dynamic(&hlt_bool);
    wrapped->hl_value->v.b = value;

    return wrapped;
}

hlffi_value* hlffi_value_string(hlffi_vm* vm, const char* str) {
    if (!vm) return NULL;
    if (!str) return hlffi_value_null(vm);

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Use hl_alloc_strbytes which allocates and converts UTF-8 to UTF-16 */
    vdynamic* hl_str = hl_alloc_strbytes((const uchar*)str);
    if (!hl_str) {
        free(wrapped);
        return NULL;
    }

    wrapped->hl_value = hl_str;

    return wrapped;
}

hlffi_value* hlffi_value_null(hlffi_vm* vm) {
    if (!vm) return NULL;

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    wrapped->hl_value = NULL;

    return wrapped;
}

/* ========== VALUE UNBOXING ========== */

int hlffi_value_as_int(hlffi_value* value, int fallback) {
    if (!value || !value->hl_value) return fallback;

    vdynamic* v = value->hl_value;

    /* Check type and extract */
    if (v->t == &hlt_i32) {
        return v->v.i;
    } else if (v->t == &hlt_f64) {
        return (int)v->v.d;  /* Allow float->int conversion */
    } else if (v->t == &hlt_bool) {
        return v->v.b ? 1 : 0;
    }

    return fallback;
}

double hlffi_value_as_float(hlffi_value* value, double fallback) {
    if (!value || !value->hl_value) return fallback;

    vdynamic* v = value->hl_value;

    /* Check type and extract */
    if (v->t == &hlt_f64) {
        return v->v.d;
    } else if (v->t == &hlt_i32) {
        return (double)v->v.i;  /* Allow int->float conversion */
    }

    return fallback;
}

bool hlffi_value_as_bool(hlffi_value* value, bool fallback) {
    if (!value || !value->hl_value) return fallback;

    vdynamic* v = value->hl_value;

    /* Check type and extract */
    if (v->t == &hlt_bool) {
        return v->v.b;
    } else if (v->t == &hlt_i32) {
        return v->v.i != 0;  /* Non-zero int is true */
    }

    return fallback;
}

char* hlffi_value_as_string(hlffi_value* value) {
    if (!value || !value->hl_value) return NULL;

    vdynamic* v = value->hl_value;

    /* Check if it's a bytes type (strings are represented as bytes in HashLink) */
    if (v->t->kind == HBYTES) {
        vstring* hl_str = (vstring*)v;
        /* Convert UTF-16 to UTF-8 */
        char* utf8 = hl_to_utf8(hl_str->bytes);
        /* hl_to_utf8 returns a static buffer, so we need to strdup */
        return utf8 ? strdup(utf8) : NULL;
    }

    return NULL;
}

bool hlffi_value_is_null(hlffi_value* value) {
    return !value || !value->hl_value;
}

/* ========== STATIC FIELD ACCESS ========== */

hlffi_value* hlffi_get_static_field(hlffi_vm* vm, const char* class_name, const char* field_name) {
    if (!vm) return NULL;
    if (!vm->module || !vm->module->code) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "VM not initialized or no bytecode loaded");
        return NULL;
    }
    if (!class_name || !field_name) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Class name or field name is NULL");
        return NULL;
    }

    /* Hash the names - prepend $ for runtime class */
    char runtime_class_name[256];
    snprintf(runtime_class_name, sizeof(runtime_class_name), "$%s", class_name);
    int class_hash = hl_hash_utf8(runtime_class_name);
    int field_hash = hl_hash_utf8(field_name);

    /* Find the class type */
    hl_code* code = vm->module->code;
    hl_type* class_type = NULL;

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char* type_name = hl_to_utf8(t->obj->name);
            int type_hash = type_name ? hl_hash_utf8(type_name) : 0;
            if (type_name && type_hash == class_hash) {
                class_type = t;
                break;
            }
        }
    }

    if (!class_type) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Class not found: %s", class_name);
        set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, error_buf);
        return NULL;
    }

    /* Find the static field */
    for (int i = 0; i < class_type->obj->nfields; i++) {
        hl_obj_field* field = &class_type->obj->fields[i];
        if (field->hashed_name == field_hash) {
            /* TODO: Static field runtime access not yet working
             * The class type and field are found correctly, but accessing the value fails.
             * Issue: class_type->obj->global_value is NULL for classes without global instances.
             * HashLink stores static fields as individual globals in module->globals_data,
             * but the mapping from field index to global index is not straightforward.
             * Need to investigate proper HashLink API for static field access.
             */
            set_error(vm, HLFFI_ERROR_NOT_IMPLEMENTED,
                     "Static field access not yet implemented - runtime lookup mechanism needs investigation");
            return NULL;
        }
    }

    char error_buf[256];
    snprintf(error_buf, sizeof(error_buf), "Field not found: %s.%s", class_name, field_name);
    set_error(vm, HLFFI_ERROR_FIELD_NOT_FOUND, error_buf);
    return NULL;
}

hlffi_error_code hlffi_set_static_field(hlffi_vm* vm, const char* class_name, const char* field_name, hlffi_value* value) {
    if (!vm) return HLFFI_ERROR_NULL_VM;
    if (!vm->module || !vm->module->code) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "VM not initialized or no bytecode loaded");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }
    if (!class_name || !field_name || !value) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Class name, field name, or value is NULL");
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    /* Hash the names - prepend $ for runtime class */
    char runtime_class_name[256];
    snprintf(runtime_class_name, sizeof(runtime_class_name), "$%s", class_name);
    int class_hash = hl_hash_utf8(runtime_class_name);
    int field_hash = hl_hash_utf8(field_name);

    /* Find the class type */
    hl_code* code = vm->module->code;
    hl_type* class_type = NULL;

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char* type_name = hl_to_utf8(t->obj->name);
            if (type_name && hl_hash_utf8(type_name) == class_hash) {
                class_type = t;
                break;
            }
        }
    }

    if (!class_type) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Class not found: %s", class_name);
        set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, error_buf);
        return HLFFI_ERROR_TYPE_NOT_FOUND;
    }

    /* Find the static field */
    for (int i = 0; i < class_type->obj->nfields; i++) {
        hl_obj_field* field = &class_type->obj->fields[i];
        if (field->hashed_name == field_hash) {
            /* TODO: Static field runtime access not yet working (see hlffi_get_static_field for details) */
            set_error(vm, HLFFI_ERROR_NOT_IMPLEMENTED,
                     "Static field access not yet implemented - runtime lookup mechanism needs investigation");
            return HLFFI_ERROR_NOT_IMPLEMENTED;
        }
    }

    char error_buf[256];
    snprintf(error_buf, sizeof(error_buf), "Field not found: %s.%s", class_name, field_name);
    set_error(vm, HLFFI_ERROR_FIELD_NOT_FOUND, error_buf);
    return HLFFI_ERROR_FIELD_NOT_FOUND;
}

/* ========== STATIC METHOD CALLS ========== */

hlffi_value* hlffi_call_static(hlffi_vm* vm, const char* class_name, const char* method_name, int argc, hlffi_value** argv) {
    if (!vm) return NULL;
    if (!vm->module || !vm->module->code) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "VM not initialized or no bytecode loaded");
        return NULL;
    }
    if (!class_name || !method_name) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Class name or method name is NULL");
        return NULL;
    }

    /* Hash the names - prepend $ for runtime class */
    char runtime_class_name[256];
    snprintf(runtime_class_name, sizeof(runtime_class_name), "$%s", class_name);
    int class_hash = hl_hash_utf8(runtime_class_name);
    int method_hash = hl_hash_utf8(method_name);

    /* Find the class type */
    hl_code* code = vm->module->code;
    hl_type* class_type = NULL;

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char* type_name = hl_to_utf8(t->obj->name);
            if (type_name && hl_hash_utf8(type_name) == class_hash) {
                class_type = t;
                break;
            }
        }
    }

    if (!class_type) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Class not found: %s", class_name);
        set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, error_buf);
        return NULL;
    }

    /* Find the static method */
    int func_index = -1;
    for (int i = 0; i < class_type->obj->nproto; i++) {
        hl_obj_proto* method = &class_type->obj->proto[i];
        if (method->hashed_name == method_hash) {
            func_index = method->findex;
            break;
        }
    }

    if (func_index < 0) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Method not found: %s.%s", class_name, method_name);
        set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND, error_buf);
        return NULL;
    }

    /* Get the function type */
    hl_function* func = &code->functions[vm->module->functions_indexes[func_index]];

    /* Prepare arguments - unbox hlffi_value** to vdynamic** */
    vdynamic** hl_args = NULL;
    if (argc > 0) {
        hl_args = (vdynamic**)malloc(sizeof(vdynamic*) * argc);
        if (!hl_args) {
            set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate argument array");
            return NULL;
        }
        for (int i = 0; i < argc; i++) {
            hl_args[i] = argv[i] ? argv[i]->hl_value : NULL;
        }
    }

    /* Setup closure for function call */
    vclosure cl;
    cl.t = func->type;
    cl.fun = vm->module->functions_ptrs[func_index];
    cl.hasValue = 0;

    /* Call the function with exception handling */
    bool isExc = false;
    vdynamic* result = hl_dyn_call_safe(&cl, hl_args, argc, &isExc);

    /* Free argument array */
    if (hl_args) free(hl_args);

    /* Check for exception */
    if (isExc) {
        set_error(vm, HLFFI_ERROR_EXCEPTION_THROWN, "Exception thrown during function call");
        return NULL;
    }

    /* Wrap result */
    if (!result) {
        /* Void return or null */
        return hlffi_value_null(vm);
    }

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;
    wrapped->hl_value = result;

    return wrapped;
}
