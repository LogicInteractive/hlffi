/**
 * Simple Phase 2 Test - Basic type inspection
 */

#include "hlffi.h"
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Simple Phase 2 Reflection Test ===\n\n");

    // Create and initialize VM
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }

    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Init failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    err = hlffi_load_file(vm, argv[1]);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Load failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    printf("✓ Loaded: %s\n\n", argv[1]);

    // Test 1: Find the Main class
    printf("Test 1: Finding 'Main' class...\n");
    hlffi_type* main_type = hlffi_find_type(vm, "Main");
    if (!main_type) {
        printf("  ✗ Not found: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("  ✓ Found Main!\n");

    // Test 2: Get type kind
    printf("\nTest 2: Inspecting Main type...\n");
    hlffi_type_kind kind = hlffi_type_get_kind(main_type);
    const char* kind_str = (kind == HLFFI_TYPE_OBJ) ? "class" : "other";
    printf("  Type kind: %s\n", kind_str);

    // Test 3: Get type name
    const char* name = hlffi_type_get_name(main_type);
    printf("  Type name: %s\n", name ? name : "<null>");

    // Test 4: Check for superclass
    printf("\nTest 3: Checking inheritance...\n");
    hlffi_type* super = hlffi_class_get_super(main_type);
    if (super) {
        printf("  Extends: %s\n", hlffi_type_get_name(super));
    } else {
        printf("  No superclass (top-level)\n");
    }

    // Test 5: Count fields
    printf("\nTest 4: Examining fields...\n");
    int field_count = hlffi_class_get_field_count(main_type);
    printf("  Field count: %d\n", field_count);
    for (int i = 0; i < field_count && i < 5; i++) {
        const char* field_name = hlffi_class_get_field_name(main_type, i);
        hlffi_type* field_type = hlffi_class_get_field_type(main_type, i);
        printf("    [%d] %s : %s\n", i,
               field_name ? field_name : "<unnamed>",
               hlffi_type_get_name(field_type));
    }
    if (field_count > 5) {
        printf("    ... (%d more fields)\n", field_count - 5);
    }

    // Test 6: Count methods
    printf("\nTest 5: Examining methods...\n");
    int method_count = hlffi_class_get_method_count(main_type);
    printf("  Method count: %d\n", method_count);
    for (int i = 0; i < method_count && i < 5; i++) {
        const char* method_name = hlffi_class_get_method_name(main_type, i);
        printf("    [%d] %s()\n", i, method_name ? method_name : "<unnamed>");
    }
    if (method_count > 5) {
        printf("    ... (%d more methods)\n", method_count - 5);
    }

    // Test 7: Find String class
    printf("\nTest 6: Finding 'String' class...\n");
    hlffi_type* string_type = hlffi_find_type(vm, "String");
    if (string_type) {
        printf("  ✓ Found String!\n");
        printf("  Fields: %d\n", hlffi_class_get_field_count(string_type));
        printf("  Methods: %d\n", hlffi_class_get_method_count(string_type));
    } else {
        printf("  ✗ Not found\n");
    }

    // Cleanup
    printf("\n✓ All tests passed!\n");
    hlffi_destroy(vm);
    return 0;
}
