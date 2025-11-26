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

/* HashLink internal function - declared as extern to access it
 * Defined as static in vendor/hashlink/src/std/obj.c:64
 * But exists in compiled libhl.a and can be accessed via extern declaration
 * Pattern discovered from working FFI code - this is the KEY to static field access!
 */
extern hl_field_lookup* obj_resolve_field(hl_type_obj *o, int hfield);

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

    /* Check type by kind, not just pointer equality
     * This handles both direct type pointers and type kinds
     */
    if (v->t->kind == HI32) {
        return v->v.i;
    } else if (v->t->kind == HF64) {
        return (int)v->v.d;  /* Allow float->int conversion */
    } else if (v->t->kind == HBOOL) {
        return v->v.b ? 1 : 0;
    }

    return fallback;
}

double hlffi_value_as_float(hlffi_value* value, double fallback) {
    if (!value || !value->hl_value) return fallback;

    vdynamic* v = value->hl_value;

    /* Check type by kind */
    if (v->t->kind == HF64) {
        return v->v.d;
    } else if (v->t->kind == HI32) {
        return (double)v->v.i;  /* Allow int->float conversion */
    }

    return fallback;
}

bool hlffi_value_as_bool(hlffi_value* value, bool fallback) {
    if (!value || !value->hl_value) return fallback;

    vdynamic* v = value->hl_value;

    /* Check type by kind */
    if (v->t->kind == HBOOL) {
        return v->v.b;
    } else if (v->t->kind == HI32) {
        return v->v.i != 0;  /* Non-zero int is true */
    }

    return fallback;
}

char* hlffi_value_as_string(hlffi_value* value) {
    if (!value || !value->hl_value) return NULL;

    vdynamic* v = value->hl_value;

    /* HashLink strings can be either HBYTES or direct vstring
     * Check for string type using hl_is_dynamic which handles both cases
     */
    if (v->t->kind == HBYTES) {
        /* Direct bytes/string type */
        vstring* hl_str = (vstring*)v;
        if (hl_str->bytes) {
            /* Convert UTF-16 to UTF-8 */
            char* utf8 = hl_to_utf8(hl_str->bytes);
            /* hl_to_utf8 returns a static buffer, so we need to strdup */
            return utf8 ? strdup(utf8) : NULL;
        }
    } else if (v->t->kind == HDYN) {
        /* Dynamic value - might be a boxed string */
        vdynamic* str_dyn = hl_dyn_castp(v, v->t, &hlt_bytes);
        if (str_dyn && str_dyn->t->kind == HBYTES) {
            vstring* hl_str = (vstring*)str_dyn;
            if (hl_str->bytes) {
                char* utf8 = hl_to_utf8(hl_str->bytes);
                return utf8 ? strdup(utf8) : NULL;
            }
        }
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

    /* Hash the class name (NO $ prefix for global_value access) */
    int class_hash = hl_hash_utf8(class_name);
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

    /* Get global instance - must be non-NULL for static field access!
     * Pattern from working FFI code:
     * Classes need a global singleton instance for field access to work.
     * This is created automatically by HashLink for classes with static fields/methods.
     */
    if (!class_type->obj->global_value) {
        char error_buf[512];
        snprintf(error_buf, sizeof(error_buf),
                "Class '%s' has no global instance. Entry point must be called first to initialize globals.",
                class_name);
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, error_buf);
        return NULL;
    }

    vdynamic* global = *(vdynamic**)class_type->obj->global_value;
    if (!global) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "Global instance is NULL - entry point not called");
        return NULL;
    }

    /* Resolve field using obj_resolve_field (the KEY function!)
     * This maps the field hash to a field_lookup structure with the actual field offset.
     */
    hl_field_lookup* lookup = obj_resolve_field(global->t->obj, field_hash);
    if (!lookup) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Field not found: %s.%s", class_name, field_name);
        set_error(vm, HLFFI_ERROR_FIELD_NOT_FOUND, error_buf);
        return NULL;
    }

    /* Get field value using hl_dyn_getp with the correct field type from lookup */
    vdynamic* field_value = (vdynamic*)hl_dyn_getp(global, lookup->hashed_name, lookup->t);

    /* Wrap and return */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate hlffi_value");
        return NULL;
    }
    wrapped->hl_value = field_value;

    return wrapped;
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

    /* Hash the class name (NO $ prefix for global_value access) */
    int class_hash = hl_hash_utf8(class_name);
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

    /* Get global instance */
    if (!class_type->obj->global_value) {
        char error_buf[512];
        snprintf(error_buf, sizeof(error_buf),
                "Class '%s' has no global instance. Entry point must be called first to initialize globals.",
                class_name);
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, error_buf);
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    vdynamic* global = *(vdynamic**)class_type->obj->global_value;
    if (!global) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "Global instance is NULL - entry point not called");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /* Resolve field */
    hl_field_lookup* lookup = obj_resolve_field(global->t->obj, field_hash);
    if (!lookup) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Field not found: %s.%s", class_name, field_name);
        set_error(vm, HLFFI_ERROR_FIELD_NOT_FOUND, error_buf);
        return HLFFI_ERROR_FIELD_NOT_FOUND;
    }

    /* Set field value using hl_dyn_setp
     * The field type from lookup can be used for type checking if needed
     */
    hl_dyn_setp(global, lookup->hashed_name, lookup->t, value->hl_value);

    return HLFFI_OK;
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

    /* Hash the class name (NO $ prefix - use regular class for global_value access) */
    int class_hash = hl_hash_utf8(class_name);
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

    /* Get global instance for method access */
    if (!class_type->obj->global_value) {
        char error_buf[512];
        snprintf(error_buf, sizeof(error_buf),
                "Class '%s' has no global instance. Entry point must be called first.",
                class_name);
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, error_buf);
        return NULL;
    }

    vdynamic* global = *(vdynamic**)class_type->obj->global_value;
    if (!global) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "Global instance is NULL - entry point not called");
        return NULL;
    }

    /* Resolve method as a field on the global object (KEY pattern from working FFI code!)
     * Methods are stored as closure fields, not in the proto array
     */
    hl_field_lookup* lookup = obj_resolve_field(global->t->obj, method_hash);
    if (!lookup) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Method not found: %s.%s", class_name, method_name);
        set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND, error_buf);
        return NULL;
    }

    /* Get the method closure from the global object */
    vclosure* method = (vclosure*)hl_dyn_getp(global, lookup->hashed_name, &hlt_dyn);
    if (!method) {
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Method is NULL: %s.%s", class_name, method_name);
        set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND, error_buf);
        return NULL;
    }

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

    /* Call the method closure with exception handling
     * The method variable already contains a valid vclosure from hl_dyn_getp
     */
    bool isExc = false;
    vdynamic* result = hl_dyn_call_safe(method, hl_args, argc, &isExc);

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
