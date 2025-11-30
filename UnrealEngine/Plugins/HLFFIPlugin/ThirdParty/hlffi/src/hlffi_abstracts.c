/**
 * hlffi_abstracts.c
 * Phase 5: Abstract Type Support
 *
 * Haxe abstracts are compile-time wrappers around underlying types.
 * At runtime, they're transparent - just the underlying type.
 * This module provides utilities to identify and work with abstract types.
 */

#include "hlffi_internal.h"
#include <string.h>
#include <stdlib.h>

/* HashLink string functions are available via hl.h included by hlffi_internal.h */

/* ========== ABSTRACT TYPE QUERIES ========== */

/**
 * Check if a type is an abstract type.
 *
 * @param type The type to check
 * @return true if abstract, false otherwise
 */
bool hlffi_is_abstract(hlffi_type* type) {
    if (!type) return false;

    hl_type* t = (hl_type*)type;
    return t->kind == HABSTRACT;
}

/**
 * Get the name of an abstract type.
 *
 * @param type The abstract type
 * @return Abstract name (malloc'd, caller must free), or NULL if not abstract
 *
 * Example:
 *   hlffi_type* type = hlffi_find_type(vm, "MyAbstract");
 *   char* name = hlffi_abstract_get_name(type);
 *   printf("Abstract: %s\n", name);
 *   free(name);
 */
char* hlffi_abstract_get_name(hlffi_type* type) {
    if (!type) return NULL;

    hl_type* t = (hl_type*)type;
    if (t->kind != HABSTRACT || !t->abs_name) return NULL;

    /* Convert UTF-16 to UTF-8 */
    char* utf8 = hl_to_utf8(t->abs_name);
    if (!utf8) return NULL;

    /* Make a copy since hl_to_utf8 may return static buffer */
    char* result = strdup(utf8);
    return result;
}

/**
 * Find an abstract type by name.
 *
 * @param vm VM instance
 * @param name Abstract type name
 * @return Type handle, or NULL if not found or not abstract
 *
 * Example:
 *   hlffi_type* abs_type = hlffi_abstract_find(vm, "MyAbstract");
 *   if (abs_type) {
 *       // Work with abstract type
 *   }
 */
hlffi_type* hlffi_abstract_find(hlffi_vm* vm, const char* name) {
    if (!vm || !name) return NULL;

    hlffi_type* type = hlffi_find_type(vm, name);
    if (!type) return NULL;

    hl_type* t = (hl_type*)type;
    if (t->kind != HABSTRACT) return NULL;

    return type;
}

/* ========== ABSTRACT VALUE OPERATIONS ========== */

/**
 * Check if a value is of an abstract type.
 *
 * @param value The value to check
 * @return true if value is abstract type, false otherwise
 *
 * Note: At runtime, abstract values are just their underlying type.
 * This checks the static type information.
 */
bool hlffi_value_is_abstract(hlffi_value* value) {
    if (!value || !value->hl_value) return false;

    vdynamic* val = value->hl_value;
    if (!val->t) return false;

    return val->t->kind == HABSTRACT;
}

/**
 * Get the abstract type name from a value.
 *
 * @param value Value of abstract type
 * @return Abstract name (malloc'd, caller must free), or NULL
 *
 * Example:
 *   hlffi_value* val = hlffi_call_static(vm, "Test", "getAbstract", 0, NULL);
 *   char* abs_name = hlffi_value_get_abstract_name(val);
 *   printf("Abstract type: %s\n", abs_name);
 *   free(abs_name);
 */
char* hlffi_value_get_abstract_name(hlffi_value* value) {
    if (!value || !value->hl_value) return NULL;

    vdynamic* val = value->hl_value;
    if (!val->t || val->t->kind != HABSTRACT) return NULL;

    if (!val->t->abs_name) return NULL;

    /* Convert UTF-16 to UTF-8 */
    char* utf8 = hl_to_utf8(val->t->abs_name);
    if (!utf8) return NULL;

    /* Make a copy */
    char* result = strdup(utf8);
    return result;
}

/**
 * Work with abstract values.
 *
 * IMPORTANT: Haxe abstracts are transparent at runtime!
 * They're just compile-time wrappers around underlying types.
 *
 * To work with abstract values:
 * 1. Use normal hlffi_value_as_int/float/string for primitive abstracts
 * 2. Use hlffi_call_method for abstracts over objects
 * 3. The abstract type info is mainly for debugging/reflection
 *
 * Example (abstract over Int):
 *   hlffi_value* abstract_val = hlffi_call_static(vm, "Test", "getAbstractInt", 0, NULL);
 *   int value = hlffi_value_as_int(abstract_val, 0);  // Works directly!
 *
 * Example (abstract over object):
 *   hlffi_value* abstract_obj = hlffi_call_static(vm, "Test", "getAbstractObj", 0, NULL);
 *   hlffi_value* result = hlffi_call_method(abstract_obj, "doSomething", 0, NULL);
 */
