/**
 * HLFFI Phase 2 Test: Type System & Reflection
 * Tests type lookup, inspection, and class introspection
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper: Print type kind as string */
static const char* type_kind_to_string(hlffi_type_kind kind) {
    switch (kind) {
        case HLFFI_TYPE_VOID: return "void";
        case HLFFI_TYPE_UI8: return "ui8";
        case HLFFI_TYPE_UI16: return "ui16";
        case HLFFI_TYPE_I32: return "i32";
        case HLFFI_TYPE_I64: return "i64";
        case HLFFI_TYPE_F32: return "f32";
        case HLFFI_TYPE_F64: return "f64";
        case HLFFI_TYPE_BOOL: return "bool";
        case HLFFI_TYPE_BYTES: return "bytes";
        case HLFFI_TYPE_DYN: return "dynamic";
        case HLFFI_TYPE_FUN: return "function";
        case HLFFI_TYPE_OBJ: return "class";
        case HLFFI_TYPE_ARRAY: return "array";
        case HLFFI_TYPE_TYPE: return "type";
        case HLFFI_TYPE_REF: return "ref";
        case HLFFI_TYPE_VIRTUAL: return "virtual";
        case HLFFI_TYPE_DYNOBJ: return "dynobj";
        case HLFFI_TYPE_ABSTRACT: return "abstract";
        case HLFFI_TYPE_ENUM: return "enum";
        case HLFFI_TYPE_NULL: return "null";
        case HLFFI_TYPE_METHOD: return "method";
        case HLFFI_TYPE_STRUCT: return "struct";
        case HLFFI_TYPE_PACKED: return "packed";
        default: return "unknown";
    }
}

/* Helper: Print indentation */
static void print_indent(int level) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
}

/* Helper: Inspect a single type in detail */
static void inspect_type(hlffi_type* type, int indent_level) {
    hlffi_type_kind kind = hlffi_type_get_kind(type);
    const char* name = hlffi_type_get_name(type);

    print_indent(indent_level);
    printf("Type: %s (kind: %s)\n",
           name ? name : "<anonymous>",
           type_kind_to_string(kind));

    /* For class types, show detailed information */
    if (kind == HLFFI_TYPE_OBJ) {
        /* Show superclass */
        hlffi_type* super = hlffi_class_get_super(type);
        if (super) {
            const char* super_name = hlffi_type_get_name(super);
            print_indent(indent_level);
            printf("  Extends: %s\n", super_name ? super_name : "<unknown>");
        }

        /* Show fields */
        int field_count = hlffi_class_get_field_count(type);
        if (field_count > 0) {
            print_indent(indent_level);
            printf("  Fields (%d):\n", field_count);
            for (int i = 0; i < field_count; i++) {
                const char* field_name = hlffi_class_get_field_name(type, i);
                hlffi_type* field_type = hlffi_class_get_field_type(type, i);
                const char* field_type_name = hlffi_type_get_name(field_type);

                print_indent(indent_level);
                printf("    [%d] %s : %s\n",
                       i,
                       field_name ? field_name : "<unnamed>",
                       field_type_name ? field_type_name : "<unknown>");
            }
        }

        /* Show methods */
        int method_count = hlffi_class_get_method_count(type);
        if (method_count > 0) {
            print_indent(indent_level);
            printf("  Methods (%d):\n", method_count);
            for (int i = 0; i < method_count; i++) {
                const char* method_name = hlffi_class_get_method_name(type, i);

                print_indent(indent_level);
                printf("    [%d] %s()\n",
                       i,
                       method_name ? method_name : "<unnamed>");
            }
        }
    }
}

/* Callback for hlffi_list_types() */
static void type_enum_callback(hlffi_type* type, void* userdata) {
    int* count = (int*)userdata;
    (*count)++;

    hlffi_type_kind kind = hlffi_type_get_kind(type);
    const char* name = hlffi_type_get_name(type);

    /* Only show named types */
    if (name && strlen(name) > 0 && strcmp(name, "unknown") != 0) {
        printf("  [%d] %s (%s)\n",
               *count - 1,
               name,
               type_kind_to_string(kind));
    }
}

int main(int argc, char** argv) {
    printf("=== HLFFI Phase 2 Test: Type System & Reflection ===\n\n");

    /* Check arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    const char* bytecode_path = argv[1];
    printf("Loading bytecode: %s\n\n", bytecode_path);

    /* Create VM */
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }

    /* Initialize VM */
    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: VM init failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Load bytecode */
    err = hlffi_load_file(vm, bytecode_path);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to load bytecode: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    printf("✓ VM initialized and bytecode loaded\n\n");

    /* TEST 1: List all types */
    printf("--- Test 1: List All Types ---\n");
    int type_count = 0;
    err = hlffi_list_types(vm, type_enum_callback, &type_count);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: hlffi_list_types() failed: %s\n", hlffi_get_error(vm));
    } else {
        printf("\nTotal types enumerated: %d\n", type_count);
    }
    printf("\n");

    /* TEST 2: Find specific types */
    printf("--- Test 2: Find Specific Types ---\n");

    /* Try to find common types that might be in hello.hl */
    const char* test_types[] = {
        "Main",        // Main class from test program
        "String",      // HashLink built-in
        "Array",       // HashLink built-in
        "haxe.io.Bytes", // Common Haxe type
        "NonExistentType"  // Should fail
    };

    for (int i = 0; i < sizeof(test_types) / sizeof(test_types[0]); i++) {
        printf("\nSearching for type: %s\n", test_types[i]);
        hlffi_type* found = hlffi_find_type(vm, test_types[i]);

        if (found) {
            printf("✓ Found!\n");
            inspect_type(found, 1);
        } else {
            printf("✗ Not found: %s\n", hlffi_get_error(vm));
        }
    }
    printf("\n");

    /* TEST 3: Inspect type hierarchy */
    printf("--- Test 3: Type Hierarchy Inspection ---\n");

    /* Find Main class and walk up the hierarchy */
    hlffi_type* main_type = hlffi_find_type(vm, "Main");
    if (main_type) {
        printf("\nType hierarchy for Main:\n");
        hlffi_type* current = main_type;
        int depth = 0;

        while (current) {
            print_indent(depth);
            const char* name = hlffi_type_get_name(current);
            printf("└─ %s\n", name ? name : "<anonymous>");

            current = hlffi_class_get_super(current);
            depth++;

            /* Safety: prevent infinite loops */
            if (depth > 10) {
                printf("   (max depth reached)\n");
                break;
            }
        }
    } else {
        printf("Main type not found, trying to find any class type...\n");
    }
    printf("\n");

    /* TEST 4: Error handling */
    printf("--- Test 4: Error Handling ---\n");

    /* Test NULL VM */
    hlffi_type* result = hlffi_find_type(NULL, "String");
    printf("hlffi_find_type(NULL, \"String\"): %s\n", result ? "returned type (unexpected)" : "returned NULL (correct)");

    /* Test NULL name */
    result = hlffi_find_type(vm, NULL);
    printf("hlffi_find_type(vm, NULL): %s\n", result ? "returned type (unexpected)" : "returned NULL (correct)");
    printf("  Error: %s\n", hlffi_get_error(vm));

    /* Test invalid callback */
    err = hlffi_list_types(vm, NULL, NULL);
    printf("hlffi_list_types(vm, NULL, NULL): %s\n",
           err == HLFFI_OK ? "OK (unexpected)" : "failed (correct)");
    printf("  Error: %s\n", hlffi_get_error(vm));

    printf("\n");

    /* Cleanup */
    printf("--- Cleanup ---\n");
    hlffi_destroy(vm);
    printf("✓ VM destroyed\n\n");

    printf("=== All Phase 2 Tests Complete ===\n");
    return 0;
}
