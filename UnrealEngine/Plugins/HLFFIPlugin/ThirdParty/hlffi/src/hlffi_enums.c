/**
 * hlffi_enums.c
 * Phase 5: Enum Support
 *
 * Implements enum value operations for Haxe enums
 * Supports pattern matching, constructor access, and parameter extraction
 */

#include "hlffi_internal.h"
#include <string.h>
#include <stdlib.h>

/* HashLink enum/string functions are available via hl.h included by hlffi_internal.h */

/* ========== ENUM TYPE QUERIES ========== */

/**
 * Get the number of constructors in an enum type.
 * Returns -1 if not an enum type.
 */
int hlffi_enum_get_construct_count(hlffi_vm* vm, const char* type_name) {
    if (!vm || !type_name) return -1;

    /* Look up the type */
    hl_type* t = (hl_type*)hlffi_find_type(vm, type_name);
    if (!t || t->kind != HENUM) return -1;

    return t->tenum->nconstructs;
}

/**
 * Get the name of a constructor by index.
 * Returns malloc'd UTF-8 string - caller must free()!
 */
char* hlffi_enum_get_construct_name(hlffi_vm* vm, const char* type_name, int index) {
    if (!vm || !type_name || index < 0) return NULL;

    /* Look up the type */
    hl_type* t = (hl_type*)hlffi_find_type(vm, type_name);
    if (!t || t->kind != HENUM) return NULL;

    if (index >= t->tenum->nconstructs) return NULL;

    /* Get constructor */
    hl_enum_construct* c = &t->tenum->constructs[index];

    /* Convert UTF-16 to UTF-8 */
    char* utf8 = hl_to_utf8(c->name);
    if (!utf8) return NULL;

    /* Make a copy since hl_to_utf8 may return static buffer */
    char* result = strdup(utf8);
    return result;
}

/* ========== ENUM VALUE ACCESS ========== */

/**
 * Get the constructor index from an enum value.
 * Returns -1 if not an enum.
 */
int hlffi_enum_get_index(hlffi_value* value) {
    if (!value || !value->hl_value) return -1;

    vdynamic* val = value->hl_value;
    if (!val->t || val->t->kind != HENUM) return -1;

    venum* e = (venum*)val;
    return e->index;
}

/**
 * Get the constructor name from an enum value.
 * Returns malloc'd UTF-8 string - caller must free()!
 */
char* hlffi_enum_get_name(hlffi_value* value) {
    if (!value || !value->hl_value) return NULL;

    vdynamic* val = value->hl_value;
    if (!val->t || val->t->kind != HENUM) return NULL;

    venum* e = (venum*)val;
    hl_enum_construct* c = &e->t->tenum->constructs[e->index];

    /* Convert UTF-16 to UTF-8 */
    char* utf8 = hl_to_utf8(c->name);
    if (!utf8) return NULL;

    /* Make a copy since hl_to_utf8 may return static buffer */
    char* result = strdup(utf8);
    return result;
}

/**
 * Get the number of parameters in an enum value.
 */
int hlffi_enum_get_param_count(hlffi_value* value) {
    if (!value || !value->hl_value) return -1;

    vdynamic* val = value->hl_value;
    if (!val->t || val->t->kind != HENUM) return -1;

    venum* e = (venum*)val;
    hl_enum_construct* c = &e->t->tenum->constructs[e->index];

    return c->nparams;
}

/**
 * Get a parameter from an enum value by index.
 * Returns a new hlffi_value - caller must free with hlffi_value_free()!
 *
 * NOTE: This creates a boxed dynamic value for the parameter.
 * For primitive types, use the typed accessors instead (see below).
 */
hlffi_value* hlffi_enum_get_param(hlffi_value* value, int param_index) {
    if (!value || !value->hl_value || param_index < 0) return NULL;

    vdynamic* val = value->hl_value;
    if (!val->t || val->t->kind != HENUM) return NULL;

    venum* e = (venum*)val;
    hl_enum_construct* c = &e->t->tenum->constructs[e->index];

    if (param_index >= c->nparams) return NULL;

    /* Get parameter pointer and type */
    void* param_ptr = (char*)e + c->offsets[param_index];
    hl_type* param_type = c->params[param_index];

    /* For now, use hl_make_dyn to box the value */
    extern vdynamic* hl_make_dyn(void* data, hl_type* t);
    vdynamic* param_dyn = hl_make_dyn(param_ptr, param_type);
    if (!param_dyn) return NULL;

    /* Wrap in hlffi_value */
    hlffi_value* result = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!result) return NULL;

    result->hl_value = param_dyn;
    result->is_rooted = false;

    return result;
}

/* ========== ENUM CONSTRUCTION ========== */

/**
 * Create an enum value with no parameters.
 * Example: Option.None
 */
hlffi_value* hlffi_enum_alloc_simple(hlffi_vm* vm, const char* type_name, int index) {
    if (!vm || !type_name || index < 0) return NULL;

    HLFFI_UPDATE_STACK_TOP();

    /* Look up the type */
    hl_type* t = (hl_type*)hlffi_find_type(vm, type_name);
    if (!t || t->kind != HENUM) return NULL;

    if (index >= t->tenum->nconstructs) return NULL;

    /* Allocate enum */
    venum* e = hl_alloc_enum(t, index);
    if (!e) return NULL;

    /* Wrap in hlffi_value */
    hlffi_value* result = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!result) return NULL;

    result->hl_value = (vdynamic*)e;
    result->is_rooted = false; /* GC-managed */

    return result;
}

/**
 * Create an enum value with parameters.
 * Example: Option.Some(42)
 *
 * NOTE: For now, this is limited. To create enums with parameters,
 * call a Haxe function that returns the enum instead.
 * This function is provided for future expansion.
 */
hlffi_value* hlffi_enum_alloc(hlffi_vm* vm, const char* type_name, int index,
                               int nparam, hlffi_value** params) {
    if (!vm || !type_name || index < 0) return NULL;
    if (nparam > 0 && !params) return NULL;

    HLFFI_UPDATE_STACK_TOP();

    /* Look up the type */
    hl_type* t = (hl_type*)hlffi_find_type(vm, type_name);
    if (!t || t->kind != HENUM) return NULL;

    if (index >= t->tenum->nconstructs) return NULL;

    hl_enum_construct* c = &t->tenum->constructs[index];
    if (nparam != c->nparams) return NULL;

    /* Allocate enum */
    venum* e = hl_alloc_enum(t, index);
    if (!e) return NULL;

    /* For parameters, use hl_write_dyn to copy values */
    extern void hl_write_dyn(void* addr, hl_type* t, vdynamic* v, bool is_ptr);

    for (int i = 0; i < nparam; i++) {
        if (!params[i] || !params[i]->hl_value) {
            return NULL;
        }

        void* param_ptr = (char*)e + c->offsets[i];
        hl_type* param_type = c->params[i];

        /* Write the parameter value */
        hl_write_dyn(param_ptr, param_type, params[i]->hl_value, false);
    }

    /* Wrap in hlffi_value */
    hlffi_value* result = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!result) return NULL;

    result->hl_value = (vdynamic*)e;
    result->is_rooted = false; /* GC-managed */

    return result;
}

/* ========== PATTERN MATCHING ========== */

/**
 * Check if an enum value matches a specific constructor.
 */
bool hlffi_enum_is(hlffi_value* value, int index) {
    if (!value || !value->hl_value) return false;

    vdynamic* val = value->hl_value;
    if (!val->t || val->t->kind != HENUM) return false;

    venum* e = (venum*)val;
    return e->index == index;
}

/**
 * Check if an enum value matches a constructor by name.
 */
bool hlffi_enum_is_named(hlffi_value* value, const char* name) {
    if (!value || !value->hl_value || !name) return false;

    vdynamic* val = value->hl_value;
    if (!val->t || val->t->kind != HENUM) return false;

    venum* e = (venum*)val;
    hl_enum_construct* c = &e->t->tenum->constructs[e->index];

    /* Convert constructor name to UTF-8 and compare */
    char* construct_name = hl_to_utf8(c->name);
    if (!construct_name) return false;

    /* Compare using strcmp (both are UTF-8) */
    bool match = (strcmp(construct_name, name) == 0);

    return match;
}
