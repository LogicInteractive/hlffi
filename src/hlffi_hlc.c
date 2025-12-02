/**
 * HLFFI HLC Mode Implementation
 *
 * This file provides HLC (HashLink/C) mode support for HLFFI.
 * In HLC mode, Haxe is compiled to C code instead of bytecode,
 * which means:
 * - No vm->module->code structure
 * - Types accessed via extern symbols (t$ClassName)
 * - Dynamic type resolution via Type.resolveClass()
 * - Method calls via Reflect.callMethod()
 *
 * This file is only compiled when HLFFI_HLC_MODE is defined.
 */

#include "hlffi_internal.h"

#ifdef HLFFI_HLC_MODE

#include <string.h>
#include <stdio.h>

/* Global HLC cache */
hlffi_hlc_cache g_hlc = {0};

/*
 * In HLC mode, types are exposed as extern symbols with the naming pattern:
 * t$ClassName (where . in package names becomes $)
 *
 * These are declared in the HLC-generated C code.
 * We declare them here as extern to access the Type and Reflect classes.
 */
extern hl_type t$Type;
extern hl_type t$Reflect;

/**
 * Create a HashLink string from a C string.
 * This is used internally for calling Haxe methods that expect String arguments.
 */
vdynamic* hlffi_hlc_create_string(const char* str) {
    if (!str) return NULL;

    HLFFI_UPDATE_STACK_TOP();

    int str_len = (int)strlen(str);

    /* Allocate UTF-16 buffer */
    uchar* ubuf = (uchar*)hl_gc_alloc_noptr((str_len + 1) << 1);
    if (!ubuf) return NULL;

    /* Convert UTF-8 to UTF-16 */
    hl_from_utf8(ubuf, str_len, str);

    /* Allocate vstring structure */
    vstring* vstr = (vstring*)hl_gc_alloc_raw(sizeof(vstring));
    if (!vstr) return NULL;

    vstr->bytes = ubuf;
    vstr->length = str_len;
    vstr->t = &hlt_bytes;

    return (vdynamic*)vstr;
}

/**
 * Initialize HLC support.
 *
 * This function sets up the cache with references to the Type and Reflect
 * classes, and pre-computes the hashes for commonly used field names.
 *
 * Must be called after hlffi_call_entry() has executed the entry point,
 * as the global class instances are initialized during entry point execution.
 */
hlffi_error_code hlffi_hlc_init(hlffi_vm* vm) {
    if (g_hlc.initialized) return HLFFI_OK;

    if (!vm) return HLFFI_ERROR_NULL_VM;

    if (!vm->entry_called) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "Entry point must be called before HLC init");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    HLFFI_UPDATE_STACK_TOP();

    /* === Access Type class via known HLC symbol === */
    g_hlc.type_class = &t$Type;

    if (!g_hlc.type_class->obj) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "Type class has no obj structure");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    if (!g_hlc.type_class->obj->global_value) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "Type class not initialized (no global_value)");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    g_hlc.type_global = *(vdynamic**)g_hlc.type_class->obj->global_value;
    if (!g_hlc.type_global) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED,
            "Type class global is NULL");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /* === Access Reflect class === */
    g_hlc.reflect_class = &t$Reflect;

    if (g_hlc.reflect_class->obj && g_hlc.reflect_class->obj->global_value) {
        g_hlc.reflect_global = *(vdynamic**)g_hlc.reflect_class->obj->global_value;
    }
    /* Note: Reflect.global may be NULL for some Reflect implementations */

    /* === Pre-compute hashes for performance === */
    g_hlc.hash_resolveClass = hl_hash_utf8("resolveClass");
    g_hlc.hash_createInstance = hl_hash_utf8("createInstance");
    g_hlc.hash_field = hl_hash_utf8("field");
    g_hlc.hash_setField = hl_hash_utf8("setField");
    g_hlc.hash_callMethod = hl_hash_utf8("callMethod");
    g_hlc.hash_allTypes = hl_hash_utf8("allTypes");
    g_hlc.hash_values = hl_hash_utf8("values");
    g_hlc.hash___type__ = hl_hash_utf8("__type__");
    g_hlc.hash___constructor__ = hl_hash_utf8("__constructor__");

    g_hlc.initialized = true;

    hlffi_set_error(vm, HLFFI_OK, NULL);
    return HLFFI_OK;
}

/**
 * HLC implementation of type lookup.
 * Uses Type.resolveClass(name) to find a class by name.
 *
 * @param vm The HLFFI VM instance
 * @param name The fully qualified class name (e.g., "my.pkg.Player")
 * @return The hl_type* for the class, or NULL if not found
 */
hl_type* hlffi_hlc_find_type(hlffi_vm* vm, const char* name) {
    if (!vm || !name) return NULL;

    if (!g_hlc.initialized) {
        if (hlffi_hlc_init(vm) != HLFFI_OK) {
            return NULL;
        }
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Get resolveClass method from Type class */
    vclosure* resolve = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_resolveClass, &hlt_dyn);

    if (!resolve) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Type.resolveClass not found");
        return NULL;
    }

    /* Create String argument for the class name */
    vdynamic* name_str = hlffi_hlc_create_string(name);
    if (!name_str) {
        hlffi_set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY,
            "Failed to create string for class name");
        return NULL;
    }

    /* Call resolveClass(name) */
    vdynamic* args[1] = { name_str };
    bool isExc = false;
    vdynamic* cls = hl_dyn_call_safe(resolve, args, 1, &isExc);

    if (isExc || !cls) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Type not found: %s", name);
        hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, buf);
        return NULL;
    }

    /* Extract __type__ from Class<T> (BaseType.__type__ is hl_type*) */
    hl_type* result = (hl_type*)hl_dyn_getp(cls, g_hlc.hash___type__, &hlt_dyn);

    if (!result) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Could not extract __type__ from class: %s", name);
        hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, buf);
        return NULL;
    }

    return result;
}

/**
 * HLC implementation of object creation.
 * Uses Type.createInstance(cls, args) to create a new instance.
 *
 * @param vm The HLFFI VM instance
 * @param class_name The fully qualified class name
 * @param argc Number of constructor arguments
 * @param argv Constructor arguments (array of hlffi_value*)
 * @return A new hlffi_value* containing the instance, or NULL on error
 */
hlffi_value* hlffi_hlc_new(hlffi_vm* vm, const char* class_name, int argc, hlffi_value** argv) {
    if (!vm || !class_name) return NULL;

    if (!g_hlc.initialized) {
        if (hlffi_hlc_init(vm) != HLFFI_OK) {
            return NULL;
        }
    }

    HLFFI_UPDATE_STACK_TOP();

    /* First resolve the class using Type.resolveClass */
    vclosure* resolve = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_resolveClass, &hlt_dyn);

    if (!resolve) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Type.resolveClass not found");
        return NULL;
    }

    vdynamic* name_str = hlffi_hlc_create_string(class_name);
    if (!name_str) return NULL;

    vdynamic* resolve_args[1] = { name_str };
    bool isExc = false;
    vdynamic* cls = hl_dyn_call_safe(resolve, resolve_args, 1, &isExc);

    if (isExc || !cls) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Class not found: %s", class_name);
        hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, buf);
        return NULL;
    }

    /* Get createInstance method from Type class */
    vclosure* create = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_createInstance, &hlt_dyn);

    if (!create) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Type.createInstance not found");
        return NULL;
    }

    /* Build args array for Haxe (Array<Dynamic>) */
    varray* args_array = (varray*)hl_alloc_array(&hlt_dyn, argc);
    for (int i = 0; i < argc; i++) {
        hl_aptr(args_array, vdynamic*)[i] = argv && argv[i] ? argv[i]->hl_value : NULL;
    }

    /* Call createInstance(cls, args) */
    vdynamic* create_args[2] = { cls, (vdynamic*)args_array };
    isExc = false;
    vdynamic* instance = hl_dyn_call_safe(create, create_args, 2, &isExc);

    if (isExc) {
        hlffi_set_error(vm, HLFFI_ERROR_EXCEPTION_THROWN,
            "Exception in constructor");
        return NULL;
    }

    /* Wrap result in hlffi_value */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) {
        hlffi_set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY,
            "Failed to allocate hlffi_value");
        return NULL;
    }

    wrapped->hl_value = instance;
    wrapped->is_rooted = true;
    hl_add_root(&wrapped->hl_value);

    return wrapped;
}

/**
 * HLC implementation of static method calls.
 * Uses Reflect.field() and Reflect.callMethod() to call static methods.
 *
 * @param vm The HLFFI VM instance
 * @param class_name The fully qualified class name
 * @param method_name The method name to call
 * @param argc Number of arguments
 * @param argv Method arguments (array of hlffi_value*)
 * @return The return value wrapped in hlffi_value*, or NULL on error
 */
hlffi_value* hlffi_hlc_call_static(hlffi_vm* vm, const char* class_name,
                                   const char* method_name, int argc, hlffi_value** argv) {
    if (!vm || !class_name || !method_name) return NULL;

    if (!g_hlc.initialized) {
        if (hlffi_hlc_init(vm) != HLFFI_OK) {
            return NULL;
        }
    }

    HLFFI_UPDATE_STACK_TOP();

    /* First resolve the class using Type.resolveClass */
    vclosure* resolve = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_resolveClass, &hlt_dyn);

    if (!resolve) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Type.resolveClass not found");
        return NULL;
    }

    vdynamic* class_str = hlffi_hlc_create_string(class_name);
    if (!class_str) return NULL;

    vdynamic* resolve_args[1] = { class_str };
    bool isExc = false;
    vdynamic* cls = hl_dyn_call_safe(resolve, resolve_args, 1, &isExc);

    if (isExc || !cls) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Class not found: %s", class_name);
        hlffi_set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, buf);
        return NULL;
    }

    /* Get the static method via Reflect.field(cls, methodName) */
    vclosure* field_fn = (vclosure*)hl_dyn_getp(
        g_hlc.reflect_global, g_hlc.hash_field, &hlt_dyn);

    if (!field_fn) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Reflect.field not found");
        return NULL;
    }

    vdynamic* method_str = hlffi_hlc_create_string(method_name);
    if (!method_str) return NULL;

    vdynamic* field_args[2] = { cls, method_str };
    isExc = false;
    vdynamic* method = hl_dyn_call_safe(field_fn, field_args, 2, &isExc);

    if (isExc || !method) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Method not found: %s.%s", class_name, method_name);
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND, buf);
        return NULL;
    }

    /* Get Reflect.callMethod */
    vclosure* call_fn = (vclosure*)hl_dyn_getp(
        g_hlc.reflect_global, g_hlc.hash_callMethod, &hlt_dyn);

    if (!call_fn) {
        hlffi_set_error(vm, HLFFI_ERROR_METHOD_NOT_FOUND,
            "Reflect.callMethod not found");
        return NULL;
    }

    /* Build args array */
    varray* args_array = (varray*)hl_alloc_array(&hlt_dyn, argc);
    for (int i = 0; i < argc; i++) {
        hl_aptr(args_array, vdynamic*)[i] = argv && argv[i] ? argv[i]->hl_value : NULL;
    }

    /* Call Reflect.callMethod(null, method, args) for static call
     * First argument is null for static methods */
    vdynamic* call_args[3] = { NULL, method, (vdynamic*)args_array };
    isExc = false;
    vdynamic* result = hl_dyn_call_safe(call_fn, call_args, 3, &isExc);

    if (isExc) {
        hlffi_set_error(vm, HLFFI_ERROR_EXCEPTION_THROWN,
            "Exception in static method");
        return NULL;
    }

    /* Wrap result */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) {
        hlffi_set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY,
            "Failed to allocate hlffi_value");
        return NULL;
    }

    wrapped->hl_value = result;
    wrapped->is_rooted = false;

    return wrapped;
}

/**
 * HLC implementation of static field get.
 * Uses Reflect.field(cls, fieldName) to get a static field value.
 */
hlffi_value* hlffi_hlc_get_static_field(hlffi_vm* vm, const char* class_name, const char* field_name) {
    if (!vm || !class_name || !field_name) return NULL;

    if (!g_hlc.initialized) {
        if (hlffi_hlc_init(vm) != HLFFI_OK) {
            return NULL;
        }
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Resolve the class */
    vclosure* resolve = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_resolveClass, &hlt_dyn);

    if (!resolve) return NULL;

    vdynamic* class_str = hlffi_hlc_create_string(class_name);
    if (!class_str) return NULL;

    vdynamic* resolve_args[1] = { class_str };
    bool isExc = false;
    vdynamic* cls = hl_dyn_call_safe(resolve, resolve_args, 1, &isExc);

    if (isExc || !cls) return NULL;

    /* Get field via Reflect.field */
    vclosure* field_fn = (vclosure*)hl_dyn_getp(
        g_hlc.reflect_global, g_hlc.hash_field, &hlt_dyn);

    if (!field_fn) return NULL;

    vdynamic* field_str = hlffi_hlc_create_string(field_name);
    if (!field_str) return NULL;

    vdynamic* field_args[2] = { cls, field_str };
    isExc = false;
    vdynamic* value = hl_dyn_call_safe(field_fn, field_args, 2, &isExc);

    if (isExc) return NULL;

    /* Wrap result */
    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;

    wrapped->hl_value = value;
    wrapped->is_rooted = false;

    return wrapped;
}

/**
 * HLC implementation of static field set.
 * Uses Reflect.setField(cls, fieldName, value) to set a static field.
 */
hlffi_error_code hlffi_hlc_set_static_field(hlffi_vm* vm, const char* class_name,
                                             const char* field_name, hlffi_value* value) {
    if (!vm || !class_name || !field_name) return HLFFI_ERROR_INVALID_ARGUMENT;

    if (!g_hlc.initialized) {
        hlffi_error_code err = hlffi_hlc_init(vm);
        if (err != HLFFI_OK) return err;
    }

    HLFFI_UPDATE_STACK_TOP();

    /* Resolve the class */
    vclosure* resolve = (vclosure*)hl_dyn_getp(
        g_hlc.type_global, g_hlc.hash_resolveClass, &hlt_dyn);

    if (!resolve) return HLFFI_ERROR_METHOD_NOT_FOUND;

    vdynamic* class_str = hlffi_hlc_create_string(class_name);
    if (!class_str) return HLFFI_ERROR_OUT_OF_MEMORY;

    vdynamic* resolve_args[1] = { class_str };
    bool isExc = false;
    vdynamic* cls = hl_dyn_call_safe(resolve, resolve_args, 1, &isExc);

    if (isExc || !cls) return HLFFI_ERROR_TYPE_NOT_FOUND;

    /* Get Reflect.setField */
    vclosure* setfield_fn = (vclosure*)hl_dyn_getp(
        g_hlc.reflect_global, g_hlc.hash_setField, &hlt_dyn);

    if (!setfield_fn) return HLFFI_ERROR_METHOD_NOT_FOUND;

    vdynamic* field_str = hlffi_hlc_create_string(field_name);
    if (!field_str) return HLFFI_ERROR_OUT_OF_MEMORY;

    /* Call Reflect.setField(cls, fieldName, value) */
    vdynamic* set_args[3] = { cls, field_str, value ? value->hl_value : NULL };
    isExc = false;
    hl_dyn_call_safe(setfield_fn, set_args, 3, &isExc);

    if (isExc) return HLFFI_ERROR_EXCEPTION_THROWN;

    return HLFFI_OK;
}

#endif /* HLFFI_HLC_MODE */
