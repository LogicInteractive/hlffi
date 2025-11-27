/**
 * hlffi_objects.c
 * Phase 4: Instance Members (Objects)
 *
 * Implements:
 * - hlffi_new() - Create class instances (call constructors)
 * - hlffi_get_field() - Get instance fields
 * - hlffi_set_field() - Set instance fields
 * - hlffi_call_method() - Call instance methods
 * - hlffi_is_instance_of() - Type checking
 */

#include "../include/hlffi.h"
#include <hl.h>
#include <hlmodule.h>
#include <string.h>
#include <stdlib.h>

/* ========== INTERNAL GC STACK FIX (from Phase 3) ========== */

/**
 * Update GC stack_top before any GC allocation.
 * This is critical for proper stack scanning when HLFFI is embedded.
 * See: docs/GC_STACK_SCANNING.md
 */
#define HLFFI_UPDATE_STACK_TOP() \
    do { \
        hl_thread_info* _t = hl_get_thread(); \
        if (_t) { \
            int _stack_marker; \
            _t->stack_top = &_stack_marker; \
        } \
    } while(0)

/* HashLink internal function for field resolution */
HL_API hl_field_lookup* obj_resolve_field(hl_type_obj* o, int hfield);

/* Internal VM structure (defined in hlffi_lifecycle.c) */
struct hlffi_vm {
    hl_module* module;
    hl_code* code;
    hlffi_integration_mode integration_mode;
    void* stack_context;
    char error_msg[512];
    bool entry_called;
    const char* loaded_file;
};

/* hlffi_value wraps a HashLink vdynamic* with GC root protection */
struct hlffi_value {
    vdynamic* hl_value;
    bool is_rooted;  /* Track if we added a GC root */
};

/* Helper: Set error message */
static void set_obj_error(hlffi_vm* vm, const char* msg) {
    if (vm) {
        strncpy(vm->error_msg, msg, sizeof(vm->error_msg) - 1);
        vm->error_msg[sizeof(vm->error_msg) - 1] = '\0';
    }
}

/* Helper: Find type by name */
static hl_type* find_type_by_name(hlffi_vm* vm, const char* class_name) {
    if (!vm || !vm->code || !class_name) return NULL;

    for (int i = 0; i < vm->code->ntypes; i++) {
        hl_type* t = &vm->code->types[i];
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char name_buf[256];
            utostr(name_buf, sizeof(name_buf), t->obj->name);
            if (strcmp(name_buf, class_name) == 0) {
                return t;
            }
        }
    }
    return NULL;
}

/* ========== OBJECT CREATION ========== */

/**
 * Create a new instance of a class (call constructor).
 *
 * Implementation notes:
 * 1. Entry point must be called first (global_value is NULL otherwise)
 * 2. Constructor is a method named "new"
 * 3. We must allocate the object THEN call the constructor on it
 * 4. Object is automatically GC-rooted for safety
 */
hlffi_value* hlffi_new(hlffi_vm* vm, const char* class_name, int argc, hlffi_value** argv) {
    if (!vm) return NULL;
    if (!class_name) {
        set_obj_error(vm, "Class name is NULL");
        return NULL;
    }

    /* Entry point must be called first (Phase 3 discovery!) */
    if (!vm->entry_called) {
        set_obj_error(vm, "Entry point must be called before creating instances");
        return NULL;
    }

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning (Phase 3 fix!) */

    /* Step 1: Find the class type */
    hl_type* class_type = find_type_by_name(vm, class_name);
    if (!class_type) {
        char errmsg[256];
        snprintf(errmsg, sizeof(errmsg), "Class not found: %s", class_name);
        set_obj_error(vm, errmsg);
        return NULL;
    }

    /* Step 2: Check if global_value exists (should be non-NULL after entry) */
    if (!class_type->obj->global_value) {
        char errmsg[256];
        snprintf(errmsg, sizeof(errmsg), "Class %s has no global_value (entry point not called?)", class_name);
        set_obj_error(vm, errmsg);
        return NULL;
    }

    /* Step 3: Allocate the object instance */
    vobj* instance = (vobj*)hl_alloc_obj(class_type);
    if (!instance) {
        set_obj_error(vm, "Failed to allocate object instance");
        return NULL;
    }

    /* Step 4: Find and call the constructor if argc > 0 or constructor exists */
    int ctor_hash = hl_hash_utf8("new");
    hl_field_lookup* ctor_lookup = obj_resolve_field(class_type->obj, ctor_hash);

    if (ctor_lookup && ctor_lookup->t->kind == HFUN) {
        /* Constructor found - call it */
        vclosure* ctor = (vclosure*)hl_dyn_getp((vdynamic*)instance, ctor_hash, &hlt_dyn);

        if (ctor) {
            /* Build arguments array: [this, arg1, arg2, ...] */
            vdynamic** hl_args = NULL;
            int total_args = argc + 1;  /* +1 for 'this' */

            if (total_args > 0) {
                hl_args = (vdynamic**)alloca(total_args * sizeof(vdynamic*));
                hl_args[0] = (vdynamic*)instance;  /* 'this' pointer */

                /* Copy user arguments */
                for (int i = 0; i < argc; i++) {
                    hl_args[i + 1] = argv[i] ? argv[i]->hl_value : NULL;
                }
            }

            /* Call constructor (void return) */
            bool isException = false;
            hl_dyn_call_safe(ctor, hl_args, total_args, &isException);

            if (isException) {
                set_obj_error(vm, "Exception thrown during constructor");
                return NULL;
            }
        }
    }

    /* Step 5: Wrap in hlffi_value with GC root */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) {
        set_obj_error(vm, "Failed to allocate value wrapper");
        return NULL;
    }

    wrapped->hl_value = (vdynamic*)instance;
    wrapped->is_rooted = true;
    hl_add_root(&wrapped->hl_value);  /* Keep object alive! */

    return wrapped;
}

/* ========== INSTANCE FIELD ACCESS ========== */

/**
 * Get an instance field value.
 * Uses the same type-specific accessor pattern from Phase 3 static fields.
 */
hlffi_value* hlffi_get_field(hlffi_value* obj, const char* field_name) {
    if (!obj || !obj->hl_value) return NULL;
    if (!field_name) return NULL;

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

    /* Get object type */
    vdynamic* vobj_dyn = obj->hl_value;
    if (vobj_dyn->t->kind != HOBJ) {
        return NULL;  /* Not an object */
    }

    /* Resolve field by hash */
    int field_hash = hl_hash_utf8(field_name);
    hl_field_lookup* lookup = obj_resolve_field(vobj_dyn->t->obj, field_hash);

    if (!lookup) {
        return NULL;  /* Field not found */
    }

    /* Use type-specific getter (Phase 3 pattern!) */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    wrapped->is_rooted = false;  /* Borrowed reference from object */

    switch (lookup->t->kind) {
        case HI32:
        case HUI8:
        case HUI16: {
            /* Integer types - use hl_dyn_geti */
            int val = hl_dyn_geti(vobj_dyn, lookup->hashed_name, lookup->t);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_i32);
            wrapped->hl_value->v.i = val;
            break;
        }

        case HF64: {
            /* Double type - use hl_dyn_getd */
            double val = hl_dyn_getd(vobj_dyn, lookup->hashed_name);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_f64);
            wrapped->hl_value->v.d = val;
            break;
        }

        case HF32: {
            /* Float type - use hl_dyn_getf */
            float val = hl_dyn_getf(vobj_dyn, lookup->hashed_name);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_f32);
            wrapped->hl_value->v.f = val;
            break;
        }

        case HBOOL: {
            /* Boolean type - use hl_dyn_geti */
            int val = hl_dyn_geti(vobj_dyn, lookup->hashed_name, lookup->t);
            wrapped->hl_value = hl_alloc_dynamic(&hlt_bool);
            wrapped->hl_value->v.b = (val != 0);
            break;
        }

        default: {
            /* Pointer types (objects, strings, etc.) - use hl_dyn_getp */
            vdynamic* field_value = (vdynamic*)hl_dyn_getp(vobj_dyn, lookup->hashed_name, lookup->t);
            wrapped->hl_value = field_value;
            break;
        }
    }

    return wrapped;
}

/**
 * Set an instance field value.
 * Uses the same type-specific setter pattern from Phase 3.
 */
bool hlffi_set_field(hlffi_value* obj, const char* field_name, hlffi_value* value) {
    if (!obj || !obj->hl_value) return false;
    if (!field_name || !value) return false;

    /* Get object type */
    vdynamic* vobj_dyn = obj->hl_value;
    if (vobj_dyn->t->kind != HOBJ) {
        return false;  /* Not an object */
    }

    /* Resolve field by hash */
    int field_hash = hl_hash_utf8(field_name);
    hl_field_lookup* lookup = obj_resolve_field(vobj_dyn->t->obj, field_hash);

    if (!lookup) {
        return false;  /* Field not found */
    }

    /* Use type-specific setter (Phase 3 pattern!) */
    switch (lookup->t->kind) {
        case HI32:
        case HUI8:
        case HUI16:
        case HBOOL: {
            /* Integer/bool types - use hl_dyn_seti */
            int ival = value->hl_value ? value->hl_value->v.i : 0;
            hl_dyn_seti(vobj_dyn, lookup->hashed_name, lookup->t, ival);
            break;
        }

        case HF64: {
            /* Double type - use hl_dyn_setd */
            double dval = value->hl_value ? value->hl_value->v.d : 0.0;
            hl_dyn_setd(vobj_dyn, lookup->hashed_name, dval);
            break;
        }

        case HF32: {
            /* Float type - use hl_dyn_setf */
            float fval = value->hl_value ? value->hl_value->v.f : 0.0f;
            hl_dyn_setf(vobj_dyn, lookup->hashed_name, fval);
            break;
        }

        default: {
            /* Pointer types - use hl_dyn_setp */
            hl_dyn_setp(vobj_dyn, lookup->hashed_name, lookup->t, value->hl_value);
            break;
        }
    }

    return true;
}

/* ========== INSTANCE METHOD CALLS ========== */

/**
 * Call an instance method.
 * Similar to hlffi_call_static but on an instance instead of class.
 */
hlffi_value* hlffi_call_method(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv) {
    if (!obj || !obj->hl_value) return NULL;
    if (!method_name) return NULL;

    HLFFI_UPDATE_STACK_TOP();  /* Fix GC stack scanning */

    /* Get object type */
    vdynamic* vobj_dyn = obj->hl_value;
    if (vobj_dyn->t->kind != HOBJ) {
        return NULL;  /* Not an object */
    }

    /* Find method by hash */
    int method_hash = hl_hash_utf8(method_name);
    vclosure* method = (vclosure*)hl_dyn_getp(vobj_dyn, method_hash, &hlt_dyn);

    if (!method) {
        return NULL;  /* Method not found */
    }

    /* Build arguments array: [this, arg1, arg2, ...] */
    vdynamic** hl_args = NULL;
    int total_args = argc + 1;  /* +1 for 'this' */

    if (total_args > 0) {
        hl_args = (vdynamic**)alloca(total_args * sizeof(vdynamic*));
        hl_args[0] = vobj_dyn;  /* 'this' pointer */

        /* Copy user arguments */
        for (int i = 0; i < argc; i++) {
            hl_args[i + 1] = argv[i] ? argv[i]->hl_value : NULL;
        }
    }

    /* Call method with exception handling */
    bool isException = false;
    vdynamic* result = hl_dyn_call_safe(method, hl_args, total_args, &isException);

    if (isException) {
        return NULL;  /* Exception thrown */
    }

    /* Wrap result */
    if (result) {
        hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
        if (!wrapped) return NULL;

        wrapped->hl_value = result;
        wrapped->is_rooted = false;  /* Assume temporary result */

        return wrapped;
    }

    return NULL;  /* Void return or NULL */
}

/* ========== TYPE CHECKING ========== */

/**
 * Check if a value is an instance of a given class.
 */
bool hlffi_is_instance_of(hlffi_value* obj, const char* class_name) {
    if (!obj || !obj->hl_value) return false;
    if (!class_name) return false;

    vdynamic* vobj_dyn = obj->hl_value;
    if (vobj_dyn->t->kind != HOBJ) {
        return false;  /* Not an object */
    }

    /* Compare class name */
    if (vobj_dyn->t->obj && vobj_dyn->t->obj->name) {
        char name_buf[256];
        utostr(name_buf, sizeof(name_buf), vobj_dyn->t->obj->name);
        return strcmp(name_buf, class_name) == 0;
    }

    return false;
}
