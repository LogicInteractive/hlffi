/**
 * Enum Demo - Shows Enum operations from C
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    printf("==========================================\n");
    printf("  Phase 5: Enum Demo - Haxe ↔ C\n");
    printf("==========================================\n\n");

    /* Initialize VM */
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    if (hlffi_init(vm, argc, argv) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM\n");
        hlffi_destroy(vm);
        return 1;
    }

    /* Load bytecode */
    if (hlffi_load_file(vm, "test/enum_test.hl") != HLFFI_OK) {
        fprintf(stderr, "Failed to load enum_test.hl: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Call Haxe main() - minimal, no output */
    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry point: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    printf("\n=== C Side: Testing Enum Operations ===\n\n");

    /* Test 1: Get Option.Some from Haxe and inspect it */
    printf("--- Test 1: Option.Some ---\n");
    hlffi_value* some = hlffi_call_static(vm, "EnumTest", "createSome", 0, NULL);
    if (some) {
        int index = hlffi_enum_get_index(some);
        char* name = hlffi_enum_get_name(some);
        int nparams = hlffi_enum_get_param_count(some);

        printf("[C] Enum index: %d\n", index);
        printf("[C] Enum name: %s\n", name ? name : "NULL");
        printf("[C] Param count: %d\n", nparams);

        /* Get the parameter value */
        if (nparams > 0) {
            hlffi_value* param = hlffi_enum_get_param(some, 0);
            if (param) {
                int val = hlffi_value_as_int(param, -1);
                printf("[C] Parameter value: %d\n", val);
                hlffi_value_free(param);
            }
        }

        /* Test pattern matching by index */
        if (hlffi_enum_is(some, 0)) {
            printf("[C] Pattern match by index: is Some (0) ✓\n");
        }

        /* Test pattern matching by name */
        if (hlffi_enum_is_named(some, "Some")) {
            printf("[C] Pattern match by name: is 'Some' ✓\n");
        }

        free(name);
        hlffi_value_free(some);
    }

    /* Test 2: Get Option.None from Haxe */
    printf("\n--- Test 2: Option.None ---\n");
    hlffi_value* none = hlffi_call_static(vm, "EnumTest", "createNone", 0, NULL);
    if (none) {
        int index = hlffi_enum_get_index(none);
        char* name = hlffi_enum_get_name(none);
        int nparams = hlffi_enum_get_param_count(none);

        printf("[C] Enum index: %d\n", index);
        printf("[C] Enum name: %s\n", name ? name : "NULL");
        printf("[C] Param count: %d\n", nparams);

        /* Pattern matching */
        if (hlffi_enum_is(none, 1)) {
            printf("[C] Pattern match: is None (1) ✓\n");
        }
        if (hlffi_enum_is_named(none, "None")) {
            printf("[C] Pattern match by name: is 'None' ✓\n");
        }

        free(name);
        hlffi_value_free(none);
    }

    /* Test 3: Get Result.Ok from Haxe */
    printf("\n--- Test 3: Result.Ok ---\n");
    hlffi_value* ok = hlffi_call_static(vm, "EnumTest", "createOk", 0, NULL);
    if (ok) {
        char* name = hlffi_enum_get_name(ok);
        int nparams = hlffi_enum_get_param_count(ok);

        printf("[C] Enum name: %s\n", name ? name : "NULL");
        printf("[C] Param count: %d\n", nparams);

        /* Get string parameter */
        if (nparams > 0) {
            hlffi_value* param = hlffi_enum_get_param(ok, 0);
            if (param) {
                char* str = hlffi_value_as_string(param);
                printf("[C] Parameter value: %s\n", str ? str : "NULL");
                free(str);
                hlffi_value_free(param);
            }
        }

        free(name);
        hlffi_value_free(ok);
    }

    /* Test 4: Get simple enum (Color.Red) */
    printf("\n--- Test 4: Color.Red (simple enum) ---\n");
    hlffi_value* red = hlffi_call_static(vm, "EnumTest", "createRed", 0, NULL);
    if (red) {
        char* name = hlffi_enum_get_name(red);
        int nparams = hlffi_enum_get_param_count(red);

        printf("[C] Enum name: %s\n", name ? name : "NULL");
        printf("[C] Param count: %d (expected 0)\n", nparams);

        /* Pattern matching */
        if (hlffi_enum_is_named(red, "Red")) {
            printf("[C] Pattern match: is 'Red' ✓\n");
        }

        free(name);
        hlffi_value_free(red);
    }

    /* Test 5: Query enum type info */
    printf("\n--- Test 5: Enum Type Inspection ---\n");
    int construct_count = hlffi_enum_get_construct_count(vm, "Color");
    printf("[C] Color enum has %d constructors\n", construct_count);

    for (int i = 0; i < construct_count && i < 5; i++) {
        char* cname = hlffi_enum_get_construct_name(vm, "Color", i);
        printf("[C]   Constructor[%d]: %s\n", i, cname ? cname : "NULL");
        free(cname);
    }

    /* Cleanup */
    hlffi_destroy(vm);

    printf("\n==========================================\n");
    printf("  ✓ Enum tests complete!\n");
    printf("  ✓ Demonstrated pattern matching\n");
    printf("  ✓ Showed parameter extraction\n");
    printf("  ✓ Demonstrated type inspection\n");
    printf("==========================================\n");

    return 0;
}
