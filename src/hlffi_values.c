/**
 * HLFFI Values & Static Members Implementation
 * Phase 3: Value boxing/unboxing, static field access, static method calls
 */

#include "hlffi_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Use hlffi_set_error from internal header, create local alias */
#define set_error hlffi_set_error

/* ========== VALUE BOXING ========== */

hlffi_value* hlffi_value_int(hlffi_vm* vm, int value) {
    if (!vm) return NULL;

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Box the integer */
    wrapped->hl_value = hl_alloc_dynamic(&hlt_i32);
    wrapped->hl_value->v.i = value;
    wrapped->is_rooted = false;

    return wrapped;
}

hlffi_value* hlffi_value_float(hlffi_vm* vm, double value) {
    if (!vm) return NULL;

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Box the float */
    wrapped->hl_value = hl_alloc_dynamic(&hlt_f64);
    wrapped->hl_value->v.d = value;
    wrapped->is_rooted = false;

    return wrapped;
}

hlffi_value* hlffi_value_bool(hlffi_vm* vm, bool value) {
    if (!vm) return NULL;

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Box the boolean */
    wrapped->hl_value = hl_alloc_dynamic(&hlt_bool);
    wrapped->hl_value->v.b = value;
    wrapped->is_rooted = false;

    return wrapped;
}

hlffi_value* hlffi_value_string(hlffi_vm* vm, const char* str) {
    if (!vm) return NULL;
    if (!str) return hlffi_value_null(vm);

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Create HashLink string using the dexutils.h pattern:
     * 1. Allocate UTF-16 buffer with hl_gc_alloc_noptr()
     * 2. Convert UTF-8 to UTF-16 using hl_from_utf8()
     * 3. Manually construct vstring with proper type
     * This pattern works reliably for method arguments (from dexutils.h).
     */
    int str_len = (int)strlen(str);
    uchar* ubuf = (uchar*)hl_gc_alloc_noptr((str_len + 1) << 1);  // UTF-16 needs 2 bytes per char
    if (!ubuf) {
        free(wrapped);
        return NULL;
    }

    hl_from_utf8(ubuf, str_len, str);  // Convert UTF-8 → UTF-16

    vstring* vstr = (vstring*)hl_gc_alloc_raw(sizeof(vstring));
    if (!vstr) {
        free(wrapped);
        return NULL;
    }

    vstr->bytes = ubuf;
    vstr->length = str_len;
    vstr->t = &hlt_bytes;

    wrapped->hl_value = (vdynamic*)vstr;
    wrapped->is_rooted = false;

    return wrapped;
}

hlffi_value* hlffi_value_null(hlffi_vm* vm) {
    if (!vm) return NULL;

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    wrapped->hl_value = NULL;
    wrapped->is_rooted = false;  /* NULL doesn't need rooting */

    return wrapped;
}

void hlffi_value_free(hlffi_value* value) {
    if (!value) return;

    /* Remove GC root if we added one */
    if (value->is_rooted && value->hl_value) {
        hl_remove_root(&value->hl_value);
    }

    /* Free the wrapper struct */
    free(value);
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
    } else if (v->t->kind == HF32) {
        return (double)v->v.f;  /* 32-bit float */
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

    /* HashLink strings can be HBYTES (raw vstring), HOBJ (String class), or HDYN */
    if (v->t->kind == HBYTES) {
        /* Direct bytes/string type */
        vstring* hl_str = (vstring*)v;
        if (hl_str->bytes) {
            char* utf8 = hl_to_utf8(hl_str->bytes);
            return utf8 ? strdup(utf8) : NULL;
        }
    } else if (v->t->kind == HOBJ) {
        /* String object (kind=11) - use hl_to_string for proper conversion */
        uchar* utf16_str = hl_to_string(v);
        if (utf16_str) {
            char* utf8 = hl_to_utf8(utf16_str);
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

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

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

    /* Get field value using the appropriate accessor based on field type.
     * IMPORTANT: Primitive types (int, float, bool) are returned inline by hl_dyn_get*,
     * not as vdynamic pointers. We must use the correct accessor and box the result.
     */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate hlffi_value");
        return NULL;
    }

    switch (lookup->t->kind) {
        case HI32:
        case HUI8:
        case HUI16: {
            /* Integer types - use hl_dyn_geti and box the result */
            int val = hl_dyn_geti(global, lookup->hashed_name, lookup->t);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_i32);
            wrapped->hl_value->v.i = val;
            break;
        }
        case HI64: {
            /* 64-bit integer */
            int64 val = hl_dyn_geti64(global, lookup->hashed_name);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_i64);
            wrapped->hl_value->v.i64 = val;
            break;
        }
        case HF32: {
            /* 32-bit float - use hl_dyn_getf */
            float val = hl_dyn_getf(global, lookup->hashed_name);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_f32);
            wrapped->hl_value->v.f = val;
            break;
        }
        case HF64: {
            /* 64-bit float - use hl_dyn_getd */
            double val = hl_dyn_getd(global, lookup->hashed_name);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_f64);
            wrapped->hl_value->v.d = val;
            break;
        }
        case HBOOL: {
            /* Boolean - use hl_dyn_geti and treat as bool */
            int val = hl_dyn_geti(global, lookup->hashed_name, lookup->t);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_bool);
            wrapped->hl_value->v.b = (val != 0);
            break;
        }
        default: {
            /* Pointer types (objects, strings, etc.) - use hl_dyn_getp */
            vdynamic* field_value = (vdynamic*)hl_dyn_getp(global, lookup->hashed_name, lookup->t);
            wrapped->hl_value = field_value;
            break;
        }
    }
    wrapped->is_rooted = false;

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

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

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

    /* Set field value using the appropriate setter based on field type.
     * IMPORTANT: Primitive types (int, float, bool) need their specific setters.
     * We extract the primitive value from the boxed hlffi_value.
     */
    switch (lookup->t->kind) {
        case HI32:
        case HUI8:
        case HUI16: {
            /* Integer types - extract int value and use hl_dyn_seti */
            int val = hlffi_value_as_int(value, 0);
            hl_dyn_seti(global, lookup->hashed_name, lookup->t, val);
            break;
        }
        case HI64: {
            /* 64-bit integer */
            if (value->hl_value && value->hl_value->t->kind == HI64) {
                hl_dyn_seti64(global, lookup->hashed_name, value->hl_value->v.i64);
            } else {
                /* Fallback: convert from int */
                int64 val = (int64)hlffi_value_as_int(value, 0);
                hl_dyn_seti64(global, lookup->hashed_name, val);
            }
            break;
        }
        case HF32: {
            /* 32-bit float - use hl_dyn_setf */
            float val = (float)hlffi_value_as_float(value, 0.0);
            hl_dyn_setf(global, lookup->hashed_name, val);
            break;
        }
        case HF64: {
            /* 64-bit float - use hl_dyn_setd */
            double val = hlffi_value_as_float(value, 0.0);
            hl_dyn_setd(global, lookup->hashed_name, val);
            break;
        }
        case HBOOL: {
            /* Boolean - use hl_dyn_seti with 0/1 */
            int val = hlffi_value_as_bool(value, false) ? 1 : 0;
            hl_dyn_seti(global, lookup->hashed_name, lookup->t, val);
            break;
        }
        default: {
            /* Pointer types (objects, strings, etc.) - use hl_dyn_setp */
            hl_dyn_setp(global, lookup->hashed_name, lookup->t, value->hl_value);
            break;
        }
    }

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

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

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

    /* TYPE CONVERSION: Convert HBYTES to String objects if method expects HOBJ String */
    if (argc > 0 && method->t->kind == HFUN) {
        for (int i = 0; i < argc && i < method->t->fun->nargs; i++) {
            hl_type* expected_type = method->t->fun->args[i];
            vdynamic* arg = hl_args[i];

            if (arg && expected_type->kind == HOBJ && arg->t->kind == HBYTES) {
                char type_name_buf[128];
                if (expected_type->obj && expected_type->obj->name) {
                    utostr(type_name_buf, sizeof(type_name_buf), expected_type->obj->name);
                    if (strcmp(type_name_buf, "String") == 0) {
                        vstring* bytes_str = (vstring*)arg;
                        bytes_str->t = expected_type;
                        hl_args[i] = (vdynamic*)bytes_str;
                    }
                }
            }
        }
    }

    /* Call the method closure with exception handling */
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
    wrapped->is_rooted = false;

    return wrapped;
}
