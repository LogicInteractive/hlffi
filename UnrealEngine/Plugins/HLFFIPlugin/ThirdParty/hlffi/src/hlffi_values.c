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

hlffi_value* hlffi_value_f32(hlffi_vm* vm, float value) {
    if (!vm) return NULL;

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    /* Box the f32 */
    wrapped->hl_value = hl_alloc_dynamic(&hlt_f32);
    wrapped->hl_value->v.f = value;
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

float hlffi_value_as_f32(hlffi_value* value, float fallback) {
    if (!value || !value->hl_value) return fallback;

    vdynamic* v = value->hl_value;

    /* Check type by kind */
    if (v->t->kind == HF32) {
        return v->v.f;  /* 32-bit float - no conversion */
    } else if (v->t->kind == HF64) {
        return (float)v->v.d;  /* Downcast from F64 if needed */
    } else if (v->t->kind == HI32) {
        return (float)v->v.i;  /* Allow int->float conversion */
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

/* ========== PHASE 5: ARRAY OPERATIONS ========== */

/**
 * Helper: Find Haxe Array wrapper type by element type
 * Returns hl.types.ArrayBytes_Int, ArrayBytes_F64, ArrayObj, etc.
 */
static hl_type* find_haxe_array_type(hlffi_vm* vm, hl_type* element_type) {
    if (!vm || !vm->module || !vm->module->code) {
        return NULL;
    }

    hl_code* code = vm->module->code;
    const char* array_type_name = NULL;

    /* Determine the Haxe Array type name based on element type */
    if (!element_type || element_type->kind == HDYN) {
        array_type_name = "hl.types.ArrayDyn";
    } else if (element_type->kind == HI32) {
        array_type_name = "hl.types.ArrayBytes_Int";
    } else if (element_type->kind == HF32) {
        array_type_name = "hl.types.ArrayBytes_F32";
    } else if (element_type->kind == HF64) {
        array_type_name = "hl.types.ArrayBytes_F64";
    } else if (element_type->kind == HBOOL) {
        array_type_name = "hl.types.ArrayBytes_UI8";
    } else {
        /* Strings, objects, functions are all pointers - use ArrayObj */
        /* NOTE: HBYTES (strings) use ArrayObj, not ArrayBytes_String */
        array_type_name = "hl.types.ArrayObj";
    }

    /* Search for the type in the module */
    int hash = hl_hash_utf8(array_type_name);
    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), t->obj->name);
            if (hl_hash_utf8(type_name) == hash) {
                return t;
            }
        }
    }

    return NULL;
}

/**
 * Helper: Wrap a varray as a Haxe Array object
 * Converts C-created HARRAY to Haxe Array<T> (HOBJ)
 */
static vdynamic* wrap_varray_as_haxe_array(hlffi_vm* vm, varray* arr) {
    if (!vm || !arr) return NULL;

    /* Find the appropriate Haxe Array type */
    hl_type* array_type = find_haxe_array_type(vm, arr->at);
    if (!array_type) {
        set_error(vm, HLFFI_ERROR_TYPE_MISMATCH,
                  "Could not find Haxe Array type for element type");
        return NULL;
    }

    /* Ensure runtime object is initialized */
    hl_runtime_obj* rt = hl_get_obj_proto(array_type);
    if (!rt) {
        set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Failed to initialize Array type");
        return NULL;
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Allocate the Array object */
    vobj* obj = (vobj*)hl_alloc_obj(array_type);
    if (!obj) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate Array object");
        return NULL;
    }

    /* ArrayObj vs ArrayBytes_* have different field structures:
     * - ArrayObj: single "array" field (varray*)
     * - ArrayBytes_*: "bytes" and "size" fields
     */
    char type_name[128];
    utostr(type_name, sizeof(type_name), array_type->obj->name);

    if (strstr(type_name, "ArrayObj")) {
        /* ArrayObj: single field [0] = array (varray*)
         * Use runtime's field index to get correct offset */
        int field_offset = rt->fields_indexes[0];  /* field 0 is "array" */
        varray** array_field = (varray**)((char*)obj + field_offset);
        *array_field = arr;
    } else {
        /* ArrayBytes_*: fields [0] = bytes, [1] = size
         * But memory layout is [size(int), bytes(ptr)] due to alignment */
        int* size_ptr = (int*)(obj + 1);
        *size_ptr = arr->size;

        void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
        *bytes_ptr = hl_aptr(arr, void);
    }

    return (vdynamic*)obj;
}

hlffi_value* hlffi_array_new(hlffi_vm* vm, hl_type* element_type, int length) {
    if (!vm) return NULL;
    if (length < 0) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Array length must be >= 0");
        return NULL;
    }

    /* Use default element type if not specified */
    if (!element_type) {
        element_type = &hlt_dyn;  /* Dynamic array by default */
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Allocate array */
    varray* arr = hl_alloc_array(element_type, length);
    if (!arr) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate array");
        return NULL;
    }

    /* Initialize all elements to null/zero */
    if (element_type->kind == HDYN || element_type->kind == HOBJ ||
        element_type->kind == HBYTES || element_type->kind == HFUN) {
        /* Pointer types - initialize to NULL */
        vdynamic** data = (vdynamic**)hl_aptr(arr, vdynamic*);
        for (int i = 0; i < length; i++) {
            data[i] = NULL;
        }
    } else {
        /* Value types - zero out */
        void* data = hl_aptr(arr, void);
        int elem_size = hl_type_size(element_type);
        memset(data, 0, length * elem_size);
    }

    /* Wrap the varray as a Haxe Array object for compatibility with Haxe code */
    vdynamic* result = wrap_varray_as_haxe_array(vm, arr);
    if (!result) {
        /* If wrapping fails (e.g., module not loaded), fall back to raw varray */
        result = (vdynamic*)arr;
    }

    /* Wrap in hlffi_value */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;
    wrapped->hl_value = result;
    wrapped->is_rooted = false;

    return wrapped;
}

int hlffi_array_length(hlffi_value* arr) {
    if (!arr || !arr->hl_value) return -1;

    vdynamic* val = arr->hl_value;

    /* Debug: print actual type */

    /* Arrays can be wrapped in dynamic values */
    if (val->t->kind == HDYN && val->v.ptr) {
        /* Unwrap dynamic */
        val = (vdynamic*)val->v.ptr;
    }

    /* Check if this is actually an array */
    /* Note: Haxe Arrays can be HOBJ (11) or HARRAY (12) depending on context */
    if (val->t->kind == HOBJ) {
        /* For objects, check if it's a Haxe Array by looking at type name */
        if (val->t->obj && val->t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), val->t->obj->name);

            /* Check if this is a HashLink Array type (e.g., "hl.types.ArrayBytes_Int") */
            if (strncmp(type_name, "hl.types.Array", 14) == 0) {
                vobj* obj = (vobj*)val;

                if (strstr(type_name, "ArrayObj")) {
                    /* ArrayObj: single field [0] = array (varray*)
                     * Use runtime's field index for correct offset */
                    hl_runtime_obj* rt = val->t->obj->rt;
                    if (!rt) rt = hl_get_obj_proto(val->t);
                    int field_offset = rt->fields_indexes[0];
                    varray** array_field = (varray**)((char*)obj + field_offset);
                    varray* arr = *array_field;
                    return arr ? arr->size : 0;
                } else {
                    /* ArrayBytes_*: memory layout is [size(int), bytes(ptr)] */
                    int* size_ptr = (int*)(obj + 1);
                    return *size_ptr;
                }
            }
        }
        return -1;
    } else if (val->t->kind == HARRAY) {
        /* Direct varray */
        varray* array = (varray*)val;
        return array->size;
    } else {
        return -1;
    }
}

hlffi_value* hlffi_array_get(hlffi_vm* vm, hlffi_value* arr, int index) {
    if (!vm || !arr || !arr->hl_value) return NULL;

    vdynamic* val = arr->hl_value;

    /* Arrays can be wrapped in dynamic values */
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    varray* array = NULL;

    /* Handle both HARRAY (raw varray) and HOBJ (Haxe Array object) */
    if (val->t->kind == HARRAY) {
        array = (varray*)val;
    } else if (val->t->kind == HOBJ) {
        /* Check if this is a Haxe Array object */
        if (val->t->obj && val->t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), val->t->obj->name);

            if (strncmp(type_name, "hl.types.Array", 14) == 0) {
                vobj* obj = (vobj*)val;

                /* ArrayObj has different structure than ArrayBytes_* */
                if (strstr(type_name, "ArrayObj")) {
                    /* ArrayObj: single field [0] = array (varray*)
                     * Use runtime's field index for correct offset */
                    hl_runtime_obj* rt = val->t->obj->rt;
                    if (!rt) rt = hl_get_obj_proto(val->t);
                    int field_offset = rt->fields_indexes[0];
                    varray** array_field = (varray**)((char*)obj + field_offset);
                    array = *array_field;
                    /* Continue to normal varray access below */
                } else {
                    /* ArrayBytes_*: fields with optimized memory layout */
                    /* Field names: [0]="bytes", [1]="size" but memory: [0]=size(int), [1]=bytes(ptr) */

                    /* Read size - first field is int (4 bytes) */
                    int* size_ptr = (int*)(obj + 1);
                    int size = *size_ptr;

                    /* Read bytes pointer - second field, aligned to pointer size */
                    void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
                    void* bytes = *bytes_ptr;

                    if (bytes && index >= 0 && index < size) {
                    /* Access bytes as raw typed data based on array element type */
                    if (strstr(type_name, "_Int")) {
                        int* data = (int*)bytes;
                        return hlffi_value_int(vm, data[index]);
                    } else if (strstr(type_name, "_F32")) {
                        float* data = (float*)bytes;
                        return hlffi_value_f32(vm, data[index]);
                    } else if (strstr(type_name, "_F64")) {
                        double* data = (double*)bytes;
                        return hlffi_value_float(vm, data[index]);
                    } else if (strstr(type_name, "_UI8")) {
                        unsigned char* data = (unsigned char*)bytes;
                        return hlffi_value_bool(vm, data[index] != 0);
                    } else if (strstr(type_name, "Dyn") || strstr(type_name, "Obj")) {
                        /* Dynamic or object arrays store vdynamic* pointers */
                        vdynamic** data = (vdynamic**)bytes;
                        vdynamic* elem = data[index];

                        if (!elem) {
                            return hlffi_value_null(vm);
                        }

                        /* Wrap the element */
                        hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
                        if (!wrapped) return NULL;
                        wrapped->hl_value = elem;
                        wrapped->is_rooted = false;
                        return wrapped;
                        }
                        /* TODO: Add support for String, Bool arrays */
                    }
                    set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Array index out of bounds or unsupported type");
                    return NULL;
                }  /* End of else (ArrayBytes_*) */
            }  /* End of if Array type */
        }
    }

    if (!array) {
        set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Value is not an array");
        return NULL;
    }

    /* Bounds check */
    if (index < 0 || index >= array->size) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Array index out of bounds");
        return NULL;
    }

    hl_type* elem_type = array->at;

    /* Get element based on type */
    if (elem_type->kind == HI32) {
        int* data = (int*)hl_aptr(array, int);
        return hlffi_value_int(vm, data[index]);
    } else if (elem_type->kind == HF32) {
        float* data = (float*)hl_aptr(array, float);
        return hlffi_value_f32(vm, data[index]);
    } else if (elem_type->kind == HF64) {
        double* data = (double*)hl_aptr(array, double);
        return hlffi_value_float(vm, data[index]);
    } else if (elem_type->kind == HBOOL) {
        bool* data = (bool*)hl_aptr(array, bool);
        return hlffi_value_bool(vm, data[index]);
    } else {
        /* Pointer types (dynamic, object, string, etc.) */
        vdynamic** data = (vdynamic**)hl_aptr(array, vdynamic*);
        vdynamic* elem = data[index];

        if (!elem) {
            return hlffi_value_null(vm);
        }

        /* Wrap the element */
        hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
        if (!wrapped) return NULL;
        wrapped->hl_value = elem;
        wrapped->is_rooted = false;
        return wrapped;
    }
}

bool hlffi_array_set(hlffi_vm* vm, hlffi_value* arr, int index, hlffi_value* value) {
    if (!vm || !arr || !arr->hl_value) return false;

    vdynamic* val = arr->hl_value;

    /* Arrays can be wrapped in dynamic values */
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    varray* array = NULL;

    /* Handle both HARRAY (raw varray) and HOBJ (Haxe Array object) */
    if (val->t->kind == HARRAY) {
        array = (varray*)val;
    } else if (val->t->kind == HOBJ) {
        /* Check if this is a Haxe Array object */
        if (val->t->obj && val->t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), val->t->obj->name);

            if (strncmp(type_name, "hl.types.Array", 14) == 0) {
                vobj* obj = (vobj*)val;

                /* ArrayObj has different structure than ArrayBytes_* */
                if (strstr(type_name, "ArrayObj")) {
                    /* ArrayObj: single field [0] = array (varray*)
                     * Use runtime's field index for correct offset */
                    hl_runtime_obj* rt = val->t->obj->rt;
                    if (!rt) rt = hl_get_obj_proto(val->t);
                    int field_offset = rt->fields_indexes[0];
                    varray** array_field = (varray**)((char*)obj + field_offset);
                    array = *array_field;
                    /* Continue to normal varray access below */
                } else {
                    /* ArrayBytes_*: use direct memory access */
                    /* Memory layout: [size(int), bytes(ptr)] */

                    /* Read size - first field */
                    int* size_ptr = (int*)(obj + 1);
                    int size = *size_ptr;

                    /* Read bytes pointer - second field, aligned to pointer size */
                    void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
                    void* bytes = *bytes_ptr;

                    /* Bounds check */
                    if (index < 0 || index >= size) {
                        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Array index out of bounds");
                        return false;
                    }

                    /* Set element based on array type */
                    if (strstr(type_name, "_Int")) {
                        int* data = (int*)bytes;
                        data[index] = hlffi_value_as_int(value, 0);
                        return true;
                    } else if (strstr(type_name, "_F32")) {
                        float* data = (float*)bytes;
                        data[index] = hlffi_value_as_f32(value, 0.0f);
                        return true;
                    } else if (strstr(type_name, "_F64")) {
                        double* data = (double*)bytes;
                        data[index] = hlffi_value_as_float(value, 0.0);
                        return true;
                    } else if (strstr(type_name, "_UI8")) {
                        unsigned char* data = (unsigned char*)bytes;
                        data[index] = hlffi_value_as_bool(value, false) ? 1 : 0;
                        return true;
                    } else if (strstr(type_name, "Dyn") || strstr(type_name, "Obj")) {
                        vdynamic** data = (vdynamic**)bytes;
                        data[index] = value ? value->hl_value : NULL;
                        return true;
                    }

                    set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Unsupported array element type");
                    return false;
                }  /* End of else (ArrayBytes_*) */
            }  /* End of if Array type */
        }
    }

    if (!array) {
        set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Value is not an array");
        return false;
    }

    /* Bounds check */
    if (index < 0 || index >= array->size) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Array index out of bounds");
        return false;
    }

    hl_type* elem_type = array->at;

    /* Set element based on type */
    if (elem_type->kind == HI32) {
        int* data = (int*)hl_aptr(array, int);
        data[index] = hlffi_value_as_int(value, 0);
    } else if (elem_type->kind == HF32) {
        float* data = (float*)hl_aptr(array, float);
        data[index] = hlffi_value_as_f32(value, 0.0f);
    } else if (elem_type->kind == HF64) {
        double* data = (double*)hl_aptr(array, double);
        data[index] = hlffi_value_as_float(value, 0.0);
    } else if (elem_type->kind == HBOOL) {
        bool* data = (bool*)hl_aptr(array, bool);
        data[index] = hlffi_value_as_bool(value, false);
    } else {
        /* Pointer types */
        vdynamic** data = (vdynamic**)hl_aptr(array, vdynamic*);
        data[index] = value ? value->hl_value : NULL;
    }

    return true;
}

bool hlffi_array_push(hlffi_vm* vm, hlffi_value* arr, hlffi_value* value) {
    /* Note: HashLink arrays are fixed-size, so push requires creating a new array
     * This is a convenience function that creates a new array with size+1 and copies data */
    if (!vm || !arr || !arr->hl_value) return false;

    vdynamic* val = arr->hl_value;

    /* Arrays can be wrapped in dynamic values */
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    /* Extract array info - handle both HARRAY (raw) and HOBJ (wrapped) */
    int old_size;
    hl_type* elem_type;
    void* old_data;

    if (val->t->kind == HARRAY) {
        /* Raw varray */
        varray* old_array = (varray*)val;
        old_size = old_array->size;
        elem_type = old_array->at;
        old_data = hl_aptr(old_array, void);
    } else if (val->t->kind == HOBJ) {
        /* Wrapped Haxe Array - extract fields */
        if (val->t->obj && val->t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), val->t->obj->name);

            if (strncmp(type_name, "hl.types.Array", 14) == 0) {
                vobj* obj = (vobj*)val;

                /* Read size - first field */
                int* size_ptr = (int*)(obj + 1);
                old_size = *size_ptr;

                /* Read bytes pointer - second field */
                void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
                old_data = *bytes_ptr;

                /* Determine element type from array type name */
                if (strstr(type_name, "_Int")) {
                    elem_type = &hlt_i32;
                } else if (strstr(type_name, "_F64")) {
                    elem_type = &hlt_f64;
                } else {
                    elem_type = &hlt_dyn;  /* Dynamic or object array */
                }
            } else {
                set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Value is not an array");
                return false;
            }
        } else {
            set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Value is not an array");
            return false;
        }
    } else {
        set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Value is not an array");
        return false;
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Allocate new varray with size+1 */
    varray* new_varray = hl_alloc_array(elem_type, old_size + 1);
    if (!new_varray) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate new array");
        return false;
    }

    /* Copy old data */
    int elem_size = hl_type_size(elem_type);
    void* new_data = hl_aptr(new_varray, void);
    memcpy(new_data, old_data, old_size * elem_size);

    /* Add new element at the end */
    if (elem_type->kind == HI32) {
        int* data = (int*)new_data;
        data[old_size] = hlffi_value_as_int(value, 0);
    } else if (elem_type->kind == HF64) {
        double* data = (double*)new_data;
        data[old_size] = hlffi_value_as_float(value, 0.0);
    } else if (elem_type->kind == HBOOL) {
        bool* data = (bool*)new_data;
        data[old_size] = hlffi_value_as_bool(value, false);
    } else {
        vdynamic** data = (vdynamic**)new_data;
        data[old_size] = value ? value->hl_value : NULL;
    }

    /* Wrap the new varray as a Haxe Array object */
    vdynamic* new_wrapped = wrap_varray_as_haxe_array(vm, new_varray);
    if (!new_wrapped) {
        /* If wrapping fails, fall back to raw varray */
        new_wrapped = (vdynamic*)new_varray;
    }

    /* Replace old array with new one */
    arr->hl_value = new_wrapped;

    return true;
}

/* ========== NativeArray Support ========== */

hlffi_value* hlffi_native_array_new(hlffi_vm* vm, hl_type* element_type, int length) {
    if (!vm) return NULL;
    if (length < 0) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Array length must be >= 0");
        return NULL;
    }

    /* Use default element type if not specified */
    if (!element_type) {
        element_type = &hlt_dyn;
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Allocate varray (raw NativeArray) */
    varray* arr = hl_alloc_array(element_type, length);
    if (!arr) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate native array");
        return NULL;
    }

    /* Initialize all elements to null/zero */
    if (element_type->kind == HDYN || element_type->kind == HOBJ ||
        element_type->kind == HBYTES || element_type->kind == HFUN) {
        /* Pointer types - initialize to NULL */
        vdynamic** data = (vdynamic**)hl_aptr(arr, vdynamic*);
        for (int i = 0; i < length; i++) {
            data[i] = NULL;
        }
    } else {
        /* Value types - zero out */
        void* data = hl_aptr(arr, void);
        int elem_size = hl_type_size(element_type);
        memset(data, 0, length * elem_size);
    }

    /* Return unwrapped varray (no Haxe Array object wrapper) */
    /* Wrap in hlffi_value for API consistency */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;
    wrapped->hl_value = (vdynamic*)arr;
    wrapped->is_rooted = false;

    return wrapped;
}

void* hlffi_native_array_get_ptr(hlffi_value* arr) {
    if (!arr || !arr->hl_value) return NULL;

    vdynamic* val = arr->hl_value;

    /* Unwrap dynamic if needed */
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    /* Must be HARRAY (varray) for NativeArray */
    if (val->t->kind != HARRAY) {
        return NULL;
    }

    varray* array = (varray*)val;
    return hl_aptr(array, void);
}

/* ========== Struct Array Support ========== */

hlffi_value* hlffi_array_new_struct(hlffi_vm* vm, hlffi_type* struct_type, int length) {
    if (!vm || !struct_type) return NULL;

    hl_type* hl_struct_type = (hl_type*)struct_type;

    /* Verify it's a struct type */
    if (hl_struct_type->kind != HSTRUCT && hl_struct_type->kind != HOBJ) {
        set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Type is not a struct");
        return NULL;
    }

    /* For Array<Struct>, elements are vdynamic* pointers (wrapped)
     * So we create an array of type HDYN */
    return hlffi_array_new(vm, &hlt_dyn, length);
}

void* hlffi_array_get_struct(hlffi_value* arr, int index) {
    if (!arr || !arr->hl_value) return NULL;

    vdynamic* val = arr->hl_value;

    /* Unwrap dynamic if needed */
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    varray* array = NULL;

    /* Handle both HARRAY and HOBJ */
    if (val->t->kind == HARRAY) {
        array = (varray*)val;
    } else if (val->t->kind == HOBJ) {
        /* Extract varray from Haxe Array object */
        if (val->t->obj && val->t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), val->t->obj->name);

            if (strncmp(type_name, "hl.types.Array", 14) == 0) {
                vobj* obj = (vobj*)val;

                if (strstr(type_name, "ArrayObj") || strstr(type_name, "Dyn")) {
                    /* ArrayObj: single field [0] = array (varray*) */
                    hl_runtime_obj* rt = val->t->obj->rt;
                    if (!rt) rt = hl_get_obj_proto(val->t);
                    int field_offset = rt->fields_indexes[0];
                    varray** array_field = (varray**)((char*)obj + field_offset);
                    array = *array_field;
                }
            }
        }
    }

    if (!array) return NULL;

    /* Bounds check */
    if (index < 0 || index >= array->size) {
        return NULL;
    }

    /* Get vdynamic* wrapper at index */
    vdynamic** data = (vdynamic**)hl_aptr(array, vdynamic*);
    vdynamic* wrapper = data[index];

    if (!wrapper) return NULL;

    /* For structs stored in arrays, they're wrapped in vdynamic
     * The struct data is in wrapper->v.ptr for small structs, or wrapper itself for large ones */
    if (wrapper->t->kind == HSTRUCT) {
        /* Struct data follows the vdynamic header */
        return (void*)((char*)wrapper + sizeof(vdynamic));
    } else if (wrapper->t->kind == HOBJ) {
        /* Object-style struct - data follows vobj header */
        return (void*)((vobj*)wrapper + 1);
    } else {
        /* Generic vdynamic - data is in v.ptr */
        return wrapper->v.ptr;
    }
}

bool hlffi_array_set_struct(hlffi_vm* vm, hlffi_value* arr, int index, void* struct_ptr, int struct_size) {
    if (!vm || !arr || !arr->hl_value || !struct_ptr) return false;

    vdynamic* val = arr->hl_value;

    /* Unwrap dynamic if needed */
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    varray* array = NULL;

    /* Handle both HARRAY and HOBJ */
    if (val->t->kind == HARRAY) {
        array = (varray*)val;
    } else if (val->t->kind == HOBJ) {
        /* Extract varray from Haxe Array object */
        if (val->t->obj && val->t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), val->t->obj->name);

            if (strncmp(type_name, "hl.types.Array", 14) == 0) {
                vobj* obj = (vobj*)val;

                if (strstr(type_name, "ArrayObj") || strstr(type_name, "Dyn")) {
                    /* ArrayObj: single field [0] = array (varray*) */
                    hl_runtime_obj* rt = val->t->obj->rt;
                    if (!rt) rt = hl_get_obj_proto(val->t);
                    int field_offset = rt->fields_indexes[0];
                    varray** array_field = (varray**)((char*)obj + field_offset);
                    array = *array_field;
                }
            }
        }
    }

    if (!array) {
        set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Value is not an array");
        return false;
    }

    /* Bounds check */
    if (index < 0 || index >= array->size) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Array index out of bounds");
        return false;
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Allocate vdynamic wrapper + struct data
     * We need to allocate space for both the vdynamic header and the struct data */
    vdynamic* wrapper = (vdynamic*)hl_gc_alloc_raw(sizeof(vdynamic) + struct_size);
    if (!wrapper) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate struct wrapper");
        return false;
    }

    /* Copy struct data after vdynamic header */
    void* dest = (char*)wrapper + sizeof(vdynamic);
    memcpy(dest, struct_ptr, struct_size);

    /* Store pointer in wrapper */
    wrapper->v.ptr = dest;

    /* Store wrapper in array */
    vdynamic** data = (vdynamic**)hl_aptr(array, vdynamic*);
    data[index] = wrapper;

    return true;
}

hlffi_value* hlffi_native_array_new_struct(hlffi_vm* vm, hlffi_type* struct_type, int length) {
    if (!vm || !struct_type) return NULL;

    hl_type* hl_struct_type = (hl_type*)struct_type;

    /* Verify it's a struct type */
    if (hl_struct_type->kind != HSTRUCT && hl_struct_type->kind != HOBJ) {
        set_error(vm, HLFFI_ERROR_TYPE_MISMATCH, "Type is not a struct");
        return NULL;
    }

    /* For NativeArray<Struct>, elements are stored directly (no wrapping)
     * Create varray with struct type */
    return hlffi_native_array_new(vm, hl_struct_type, length);
}

void* hlffi_native_array_get_struct_ptr(hlffi_value* arr) {
    /* Same as hlffi_native_array_get_ptr - returns direct pointer to struct array data */
    return hlffi_native_array_get_ptr(arr);
}
