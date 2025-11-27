/**
 * HLFFI Types Implementation
 * Phase 2: Type System & Reflection
 */

#include "hlffi_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Use hlffi_set_error from internal header, create local alias */
#define set_error hlffi_set_error

/* ========== TYPE LOOKUP ========== */

hlffi_type* hlffi_find_type(hlffi_vm* vm, const char* name) {
    if (!vm) return NULL;
    if (!vm->module || !vm->module->code) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "VM not initialized or no bytecode loaded");
        return NULL;
    }
    if (!name) {
        set_error(vm, HLFFI_ERROR_INVALID_TYPE, "Type name is NULL");
        return NULL;
    }

    hl_code* code = vm->module->code;

    /* Hash the search name once */
    int target_hash = hl_hash_utf8(name);

    /* Search all types in loaded module */
    for (int i = 0; i < code->ntypes; i++) {
        hl_type *t = code->types + i;

        /* For object types, compare names */
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            /* Convert UTF-16 name to UTF-8 */
            char* type_name_utf8 = hl_to_utf8(t->obj->name);
            if (type_name_utf8) {
                /* Hash and compare */
                int type_hash = hl_hash_utf8(type_name_utf8);
                if (type_hash == target_hash) {
                    /* Found it! */
                    return (hlffi_type*)t;
                }
            }
        }
        /* For enum types */
        else if (t->kind == HENUM && t->tenum && t->tenum->name) {
            char* type_name_utf8 = hl_to_utf8(t->tenum->name);
            if (type_name_utf8) {
                int type_hash = hl_hash_utf8(type_name_utf8);
                if (type_hash == target_hash) {
                    return (hlffi_type*)t;
                }
            }
        }
        /* For abstract types */
        else if (t->kind == HABSTRACT && t->abs_name) {
            char* type_name_utf8 = hl_to_utf8(t->abs_name);
            if (type_name_utf8) {
                int type_hash = hl_hash_utf8(type_name_utf8);
                if (type_hash == target_hash) {
                    return (hlffi_type*)t;
                }
            }
        }
    }

    /* Type not found */
    char error_buf[256];
    snprintf(error_buf, sizeof(error_buf), "Type not found: %s", name);
    set_error(vm, HLFFI_ERROR_TYPE_NOT_FOUND, error_buf);
    return NULL;
}

/* ========== TYPE INSPECTION ========== */

hlffi_type_kind hlffi_type_get_kind(hlffi_type* type) {
    if (!type) return HLFFI_TYPE_VOID;

    hl_type* hl_t = (hl_type*)type;

    /* Map HashLink type kinds to HLFFI type kinds */
    switch (hl_t->kind) {
        case HVOID: return HLFFI_TYPE_VOID;
        case HUI8: return HLFFI_TYPE_UI8;
        case HUI16: return HLFFI_TYPE_UI16;
        case HI32: return HLFFI_TYPE_I32;
        case HI64: return HLFFI_TYPE_I64;
        case HF32: return HLFFI_TYPE_F32;
        case HF64: return HLFFI_TYPE_F64;
        case HBOOL: return HLFFI_TYPE_BOOL;
        case HBYTES: return HLFFI_TYPE_BYTES;
        case HDYN: return HLFFI_TYPE_DYN;
        case HFUN: return HLFFI_TYPE_FUN;
        case HOBJ: return HLFFI_TYPE_OBJ;
        case HARRAY: return HLFFI_TYPE_ARRAY;
        case HTYPE: return HLFFI_TYPE_TYPE;
        case HREF: return HLFFI_TYPE_REF;
        case HVIRTUAL: return HLFFI_TYPE_VIRTUAL;
        case HDYNOBJ: return HLFFI_TYPE_DYNOBJ;
        case HABSTRACT: return HLFFI_TYPE_ABSTRACT;
        case HENUM: return HLFFI_TYPE_ENUM;
        case HNULL: return HLFFI_TYPE_NULL;
        case HMETHOD: return HLFFI_TYPE_METHOD;
        case HSTRUCT: return HLFFI_TYPE_STRUCT;
        case HPACKED: return HLFFI_TYPE_PACKED;
        default: return HLFFI_TYPE_VOID;
    }
}

const char* hlffi_type_get_name(hlffi_type* type) {
    if (!type) return NULL;

    hl_type* hl_t = (hl_type*)type;

    /* For object types */
    if (hl_t->kind == HOBJ && hl_t->obj && hl_t->obj->name) {
        return hl_to_utf8(hl_t->obj->name);
    }
    /* For enum types */
    else if (hl_t->kind == HENUM && hl_t->tenum && hl_t->tenum->name) {
        return hl_to_utf8(hl_t->tenum->name);
    }
    /* For abstract types */
    else if (hl_t->kind == HABSTRACT && hl_t->abs_name) {
        return hl_to_utf8(hl_t->abs_name);
    }
    /* For primitive types, return kind name */
    else {
        switch (hl_t->kind) {
            case HVOID: return "void";
            case HUI8: return "ui8";
            case HUI16: return "ui16";
            case HI32: return "i32";
            case HI64: return "i64";
            case HF32: return "f32";
            case HF64: return "f64";
            case HBOOL: return "bool";
            case HBYTES: return "bytes";
            case HDYN: return "dynamic";
            case HFUN: return "function";
            case HARRAY: return "array";
            case HTYPE: return "type";
            case HREF: return "ref";
            case HVIRTUAL: return "virtual";
            case HDYNOBJ: return "dynobj";
            case HNULL: return "null";
            case HMETHOD: return "method";
            case HSTRUCT: return "struct";
            case HPACKED: return "packed";
            default: return "unknown";
        }
    }
}

hlffi_error_code hlffi_list_types(hlffi_vm* vm, hlffi_type_callback callback, void* userdata) {
    if (!vm) return HLFFI_ERROR_NULL_VM;
    if (!vm->module || !vm->module->code) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "VM not initialized or no bytecode loaded");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }
    if (!callback) {
        set_error(vm, HLFFI_ERROR_INVALID_TYPE, "Callback is NULL");
        return HLFFI_ERROR_INVALID_TYPE;
    }

    hl_code* code = vm->module->code;

    /* Iterate all types and call callback */
    for (int i = 0; i < code->ntypes; i++) {
        hl_type *t = code->types + i;
        callback((hlffi_type*)t, userdata);
    }

    return HLFFI_OK;
}

/* ========== CLASS INSPECTION ========== */

hlffi_type* hlffi_class_get_super(hlffi_type* type) {
    if (!type) return NULL;

    hl_type* hl_t = (hl_type*)type;

    /* Only works for object types */
    if (hl_t->kind != HOBJ || !hl_t->obj) return NULL;

    /* Return superclass (NULL if no parent) */
    return (hlffi_type*)hl_t->obj->super;
}

int hlffi_class_get_field_count(hlffi_type* type) {
    if (!type) return -1;

    hl_type* hl_t = (hl_type*)type;

    /* Only works for object types */
    if (hl_t->kind != HOBJ || !hl_t->obj) return -1;

    return hl_t->obj->nfields;
}

const char* hlffi_class_get_field_name(hlffi_type* type, int index) {
    if (!type) return NULL;

    hl_type* hl_t = (hl_type*)type;

    /* Validate object type */
    if (hl_t->kind != HOBJ || !hl_t->obj) return NULL;

    /* Validate index */
    if (index < 0 || index >= hl_t->obj->nfields) return NULL;

    /* Get field */
    hl_obj_field* field = &hl_t->obj->fields[index];
    if (!field->name) return NULL;

    /* Convert UTF-16 to UTF-8 */
    return hl_to_utf8(field->name);
}

hlffi_type* hlffi_class_get_field_type(hlffi_type* type, int index) {
    if (!type) return NULL;

    hl_type* hl_t = (hl_type*)type;

    /* Validate object type */
    if (hl_t->kind != HOBJ || !hl_t->obj) return NULL;

    /* Validate index */
    if (index < 0 || index >= hl_t->obj->nfields) return NULL;

    /* Get field type */
    hl_obj_field* field = &hl_t->obj->fields[index];
    return (hlffi_type*)field->t;
}

int hlffi_class_get_method_count(hlffi_type* type) {
    if (!type) return -1;

    hl_type* hl_t = (hl_type*)type;

    /* Only works for object types */
    if (hl_t->kind != HOBJ || !hl_t->obj) return -1;

    return hl_t->obj->nproto;
}

const char* hlffi_class_get_method_name(hlffi_type* type, int index) {
    if (!type) return NULL;

    hl_type* hl_t = (hl_type*)type;

    /* Validate object type */
    if (hl_t->kind != HOBJ || !hl_t->obj) return NULL;

    /* Validate index */
    if (index < 0 || index >= hl_t->obj->nproto) return NULL;

    /* Get method (proto) */
    hl_obj_proto* method = &hl_t->obj->proto[index];
    if (!method->name) return NULL;

    /* Convert UTF-16 to UTF-8 */
    return hl_to_utf8(method->name);
}
