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
    hlffi_error_code last_error;
    bool hl_initialized;
    bool thread_registered;
    bool module_loaded;
    bool entry_called;
    bool hot_reload_enabled;
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
    if (!vm || !vm->module || !vm->module->code || !class_name) return NULL;

    hl_code* code = vm->module->code;

    /* Hash the class name for comparison */
    int class_hash = hl_hash_utf8(class_name);

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char* type_name = hl_to_utf8(t->obj->name);
            int type_hash = type_name ? hl_hash_utf8(type_name) : 0;
            if (type_name && type_hash == class_hash) {
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
 *
 * Constructor calling follows the pattern from MASTER_PLAN.md Issue #253 workaround.
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

#ifdef HLFFI_DEBUG
    printf("[HLFFI] Allocated instance: %p\n", (void*)instance);
    printf("[HLFFI] Instance->t: %p (expected %p)\n", (void*)instance->t, (void*)class_type);
#endif

    /* Step 4: Find and call the constructor */
    /* First ensure runtime object is initialized */
    hl_runtime_obj* rt = hl_get_obj_proto(class_type);

#ifdef HLFFI_DEBUG
    printf("[HLFFI] Runtime object: %p\n", (void*)rt);
    if (rt) {
        printf("[HLFFI] rt->nlookup = %d, rt->lookup = %p\n", rt->nlookup, (void*)rt->lookup);
        printf("[HLFFI] rt->nmethods = %d, rt->methods = %p\n", rt->nmethods, (void*)rt->methods);
        printf("[HLFFI] rt->nbindings = %d, rt->bindings = %p\n", rt->nbindings, (void*)rt->bindings);
        printf("[HLFFI] class_type->obj->nbindings = %d, bindings = %p\n",
               class_type->obj->nbindings, (void*)class_type->obj->bindings);
        /* Dump raw bindings from type obj (pairs of fid, mid) */
        for (int i = 0; i < class_type->obj->nbindings; i++) {
            int fid = class_type->obj->bindings[i<<1];
            int mid = class_type->obj->bindings[(i<<1)|1];
            printf("[HLFFI] obj->bindings[%d]: fid=%d, mid=%d\n", i, fid, mid);
        }
    }
#endif

    /* Find constructor function */
    void* ctor_func = NULL;
    hl_type* ctor_type = NULL;

    /*
     * Strategy 1: Search in bindings (for classes with __constructor__ binding)
     */
    int ctor_hash = hl_hash_utf8("__constructor__");
    if (rt && rt->bindings) {
        for (int i = 0; i < rt->nbindings; i++) {
            hl_runtime_binding* b = &rt->bindings[i];
            if (b->fid == ctor_hash) {
                ctor_func = b->ptr;
                ctor_type = b->closure;
#ifdef HLFFI_DEBUG
                printf("[HLFFI] Found constructor in bindings: ptr=%p\n", ctor_func);
#endif
                break;
            }
        }
    }

    /*
     * Strategy 2: Search module functions for constructor
     * In HL bytecode, constructor is named "$ClassName.__constructor__"
     * The $ prefix indicates the Class type's static members
     */
    if (!ctor_func && vm->module && vm->module->code) {
        hl_code* code = vm->module->code;

        /* Build the expected constructor name: $ClassName */
        char expected_class_name[256];
        snprintf(expected_class_name, sizeof(expected_class_name), "$%s", class_name);

#ifdef HLFFI_DEBUG
        printf("[HLFFI] Searching for constructor '%s.__constructor__' in %d functions...\n",
               expected_class_name, code->nfunctions);
#endif

        /* Search through all functions */
        for (int i = 0; i < code->nfunctions; i++) {
            hl_function* f = &code->functions[i];
            hl_type_obj* fobj = fun_obj(f);
            const uchar* fname = fun_field_name(f);

            if (fobj && fname) {
                /* Check if this function belongs to our class and is named "__constructor__" */
                char obj_name[256];
                char field_name[256];
                utostr(obj_name, sizeof(obj_name), fobj->name);
                utostr(field_name, sizeof(field_name), fname);

#ifdef HLFFI_DEBUG
                if (i < 15 || strstr(obj_name, class_name) != NULL) {
                    printf("[HLFFI] func[%d]: %s.%s\n", i, obj_name, field_name);
                }
#endif

                if (strcmp(obj_name, expected_class_name) == 0 && strcmp(field_name, "__constructor__") == 0) {
                    /* Found the constructor function! */
                    /* f->findex is the actual function index in the module */
                    ctor_func = vm->module->functions_ptrs[f->findex];
                    ctor_type = f->type;
#ifdef HLFFI_DEBUG
                    printf("[HLFFI] Found constructor at code index %d, findex=%d: ptr=%p, type=%p\n",
                           i, f->findex, ctor_func, (void*)ctor_type);
#endif
                    break;
                }
            }
        }
    }

    /* Call constructor if found */
    if (ctor_func) {
#ifdef HLFFI_DEBUG
        printf("[HLFFI] Calling constructor: func=%p, type=%p\n", ctor_func, (void*)ctor_type);
        if (ctor_type && ctor_type->kind == HFUN) {
            printf("[HLFFI] Constructor type: nargs=%d\n", ctor_type->fun->nargs);
            for (int argi = 0; argi < ctor_type->fun->nargs; argi++) {
                printf("[HLFFI]   arg[%d] kind=%d\n", argi, ctor_type->fun->args[argi]->kind);
            }
            printf("[HLFFI] Instance type kind=%d\n", class_type->kind);
        }
#endif
        /*
         * The __constructor__ function is a JIT-compiled method.
         * For a no-arg constructor, signature is: (this:Player) -> Void
         *
         * We must call the JIT function directly, NOT through hl_dyn_call_safe,
         * because hl_dyn_call_safe is for closures with dynamic dispatch which
         * has different calling conventions.
         */

#ifdef HLFFI_DEBUG
        printf("[HLFFI] Calling constructor directly with this=%p\n", (void*)instance);
#endif

        /* For no-arg constructor, call directly via function pointer */
        if (argc == 0 && ctor_type && ctor_type->kind == HFUN && ctor_type->fun->nargs == 1) {
            /* Direct call: constructor(this) */
            typedef void (*ctor_fn)(vdynamic*);
            ctor_fn fn = (ctor_fn)ctor_func;
            fn((vdynamic*)instance);
#ifdef HLFFI_DEBUG
            printf("[HLFFI] Constructor completed successfully (direct call)\n");
#endif
        } else {
            /* For constructors with arguments, use hl_dyn_call_safe */
            int total_args = argc + 1;
            vdynamic** hl_args = (vdynamic**)alloca(total_args * sizeof(vdynamic*));
            hl_args[0] = (vdynamic*)instance;

            for (int i = 0; i < argc; i++) {
                hl_args[i + 1] = argv[i] ? argv[i]->hl_value : NULL;
            }

            vclosure cl;
            cl.t = ctor_type;
            cl.fun = ctor_func;
            cl.hasValue = 0;
            cl.value = NULL;

#ifdef HLFFI_DEBUG
            printf("[HLFFI] Calling with %d args via hl_dyn_call_safe\n", total_args);
#endif

            bool isException = false;
            vdynamic* result = hl_dyn_call_safe(&cl, hl_args, total_args, &isException);

            if (isException) {
                set_obj_error(vm, "Exception thrown in constructor");
#ifdef HLFFI_DEBUG
                if (result) {
                    printf("[HLFFI] Exception result: %p\n", (void*)result);
                    hl_print_uncaught_exception(result);
                }
#endif
                return NULL;
            }

#ifdef HLFFI_DEBUG
            printf("[HLFFI] Constructor completed successfully (dynamic call)\n");
#endif
        }
    }
#ifdef HLFFI_DEBUG
    else {
        printf("[HLFFI] No constructor found (ok for classes without explicit constructor)\n");
    }
#endif
    /* If no constructor found, that's OK - some classes may not have one */

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

    /* Build arguments array: [arg1, arg2, ...] */
    /* NOTE: 'this' is already bound in the closure, we only pass method arguments! */
    vdynamic** hl_args = NULL;

    if (argc > 0) {
        hl_args = (vdynamic**)alloca(argc * sizeof(vdynamic*));

        /* Copy user arguments */
        for (int i = 0; i < argc; i++) {
            hl_args[i] = argv[i] ? argv[i]->hl_value : NULL;
        }
    }

    /* Call method with exception handling */
    bool isException = false;
    vdynamic* result = hl_dyn_call_safe(method, hl_args, argc, &isException);

    if (isException) {
        return NULL;  /* Exception thrown */
    }

    /* Wrap result (including NULL for null strings/objects) */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    wrapped->hl_value = result;  /* Can be NULL for null strings/objects */
    wrapped->is_rooted = false;  /* Assume temporary result */

    return wrapped;
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

/* ========== CONVENIENCE API IMPLEMENTATIONS ========== */

/**
 * Convenience functions for direct field access without intermediate hlffi_value.
 * These eliminate the need to create/free hlffi_value wrappers for simple get/set operations.
 */

int hlffi_get_field_int(hlffi_value* obj, const char* field_name, int fallback) {
    hlffi_value* field = hlffi_get_field(obj, field_name);
    if (!field) return fallback;

    int value = hlffi_value_as_int(field, fallback);
    hlffi_value_free(field);
    return value;
}

float hlffi_get_field_float(hlffi_value* obj, const char* field_name, float fallback) {
    hlffi_value* field = hlffi_get_field(obj, field_name);
    if (!field) return fallback;

    float value = hlffi_value_as_float(field, fallback);
    hlffi_value_free(field);
    return value;
}

bool hlffi_get_field_bool(hlffi_value* obj, const char* field_name, bool fallback) {
    hlffi_value* field = hlffi_get_field(obj, field_name);
    if (!field) return fallback;

    bool value = hlffi_value_as_bool(field, fallback);
    hlffi_value_free(field);
    return value;
}

char* hlffi_get_field_string(hlffi_value* obj, const char* field_name) {
    hlffi_value* field = hlffi_get_field(obj, field_name);
    if (!field) return NULL;

    char* value = hlffi_value_as_string(field);
    hlffi_value_free(field);
    return value;  /* Caller must free() */
}

bool hlffi_set_field_int(hlffi_vm* vm, hlffi_value* obj, const char* field_name, int value) {
    if (!vm || !obj) return false;

    hlffi_value* temp = hlffi_value_int(vm, value);
    if (!temp) return false;

    bool result = hlffi_set_field(obj, field_name, temp);
    hlffi_value_free(temp);
    return result;
}

bool hlffi_set_field_float(hlffi_vm* vm, hlffi_value* obj, const char* field_name, float value) {
    if (!vm || !obj) return false;

    hlffi_value* temp = hlffi_value_float(vm, value);
    if (!temp) return false;

    bool result = hlffi_set_field(obj, field_name, temp);
    hlffi_value_free(temp);
    return result;
}

bool hlffi_set_field_bool(hlffi_vm* vm, hlffi_value* obj, const char* field_name, bool value) {
    if (!vm || !obj) return false;

    hlffi_value* temp = hlffi_value_bool(vm, value);
    if (!temp) return false;

    bool result = hlffi_set_field(obj, field_name, temp);
    hlffi_value_free(temp);
    return result;
}

bool hlffi_set_field_string(hlffi_vm* vm, hlffi_value* obj, const char* field_name, const char* value) {
    if (!vm || !obj) return false;

    hlffi_value* temp = hlffi_value_string(vm, value);
    if (!temp) return false;

    bool result = hlffi_set_field(obj, field_name, temp);
    hlffi_value_free(temp);
    return result;
}

/* Method call convenience functions */

bool hlffi_call_method_void(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv) {
    hlffi_value* result = hlffi_call_method(obj, method_name, argc, argv);
    if (result) {
        hlffi_value_free(result);
        return true;
    }
    return false;  /* Method failed or threw exception */
}

int hlffi_call_method_int(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv, int fallback) {
    hlffi_value* result = hlffi_call_method(obj, method_name, argc, argv);
    if (!result) return fallback;

    int value = hlffi_value_as_int(result, fallback);
    hlffi_value_free(result);
    return value;
}

float hlffi_call_method_float(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv, float fallback) {
    hlffi_value* result = hlffi_call_method(obj, method_name, argc, argv);
    if (!result) return fallback;

    float value = hlffi_value_as_float(result, fallback);
    hlffi_value_free(result);
    return value;
}

bool hlffi_call_method_bool(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv, bool fallback) {
    hlffi_value* result = hlffi_call_method(obj, method_name, argc, argv);
    if (!result) return fallback;

    bool value = hlffi_value_as_bool(result, fallback);
    hlffi_value_free(result);
    return value;
}

char* hlffi_call_method_string(hlffi_value* obj, const char* method_name, int argc, hlffi_value** argv) {
    hlffi_value* result = hlffi_call_method(obj, method_name, argc, argv);
    if (!result) return NULL;

    char* value = hlffi_value_as_string(result);
    hlffi_value_free(result);
    return value;  /* Caller must free() */
}
