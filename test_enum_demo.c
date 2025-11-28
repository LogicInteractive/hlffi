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
    if (hlffi_load_file(vm, "test/minimal_enum.hl") != HLFFI_OK) {
        fprintf(stderr, "Failed to load minimal_enum.hl: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    printf("\n=== C Side: Testing Enum Operations ===\n\n");

    /* Test 1: Get Color.Red from Haxe and inspect it */
    printf("--- Test 1: Color.Red (simple enum) ---\n");
    hlffi_value* red = hlffi_call_static(vm, "MinimalEnumTest", "createRed", 0, NULL);
    if (red) {
        int index = hlffi_enum_get_index(red);
        char* name = hlffi_enum_get_name(red);
        int nparams = hlffi_enum_get_param_count(red);

        printf("[C] Enum index: %d\n", index);
        printf("[C] Enum name: %s\n", name ? name : "NULL");
        printf("[C] Param count: %d\n", nparams);

        /* Test pattern matching by index */
        if (hlffi_enum_is(red, 0)) {
            printf("[C] Pattern match by index: is Red (0) ✓\n");
        }

        /* Test pattern matching by name */
        if (hlffi_enum_is_named(red, "Red")) {
            printf("[C] Pattern match by name: is 'Red' ✓\n");
        }

        free(name);
        hlffi_value_free(red);
    }

    /* Test 2: Get Status.Active from Haxe (has parameter) */
    printf("\n--- Test 2: Status.Active (with parameter) ---\n");
    hlffi_value* active = hlffi_call_static(vm, "MinimalEnumTest", "createActive", 0, NULL);
    if (active) {
        int index = hlffi_enum_get_index(active);
        char* name = hlffi_enum_get_name(active);
        int nparams = hlffi_enum_get_param_count(active);

        printf("[C] Enum index: %d\n", index);
        printf("[C] Enum name: %s\n", name ? name : "NULL");
        printf("[C] Param count: %d\n", nparams);

        /* Get the parameter value */
        if (nparams > 0) {
            hlffi_value* param = hlffi_enum_get_param(active, 0);
            if (param) {
                int val = hlffi_value_as_int(param, -1);
                printf("[C] Parameter value: %d\n", val);
                hlffi_value_free(param);
            }
        }

        /* Pattern matching */
        if (hlffi_enum_is(active, 0)) {
            printf("[C] Pattern match: is Active (0) ✓\n");
        }
        if (hlffi_enum_is_named(active, "Active")) {
            printf("[C] Pattern match by name: is 'Active' ✓\n");
        }

        free(name);
        hlffi_value_free(active);
    }

    /* Test 3: Get Status.Inactive from Haxe */
    printf("\n--- Test 3: Status.Inactive ---\n");
    hlffi_value* inactive = hlffi_call_static(vm, "MinimalEnumTest", "createInactive", 0, NULL);
    if (inactive) {
        int index = hlffi_enum_get_index(inactive);
        char* name = hlffi_enum_get_name(inactive);
        int nparams = hlffi_enum_get_param_count(inactive);

        printf("[C] Enum index: %d\n", index);
        printf("[C] Enum name: %s\n", name ? name : "NULL");
        printf("[C] Param count: %d (expected 0)\n", nparams);

        /* Pattern matching */
        if (hlffi_enum_is_named(inactive, "Inactive")) {
            printf("[C] Pattern match: is 'Inactive' ✓\n");
        }

        free(name);
        hlffi_value_free(inactive);
    }

    /* Test 4: Query enum type info */
    printf("\n--- Test 4: Enum Type Inspection ---\n");
    int construct_count = hlffi_enum_get_construct_count(vm, "Color");
    printf("[C] Color enum has %d constructors\n", construct_count);

    for (int i = 0; i < construct_count && i < 5; i++) {
        char* cname = hlffi_enum_get_construct_name(vm, "Color", i);
        printf("[C]   Constructor[%d]: %s\n", i, cname ? cname : "NULL");
        free(cname);
    }

    int status_count = hlffi_enum_get_construct_count(vm, "Status");
    printf("[C] Status enum has %d constructors\n", status_count);

    for (int i = 0; i < status_count && i < 5; i++) {
        char* cname = hlffi_enum_get_construct_name(vm, "Status", i);
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
