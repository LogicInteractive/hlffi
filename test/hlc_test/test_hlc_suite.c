/**
 * HLFFI HLC Mode Test Suite Runner
 *
 * This test runs the comprehensive HlcTestSuite which tests:
 * - Basic types (int, float, string, bool)
 * - Arrays (int, float, string, operations)
 * - Maps (get, set, iterate, remove)
 * - String operations
 * - Math functions
 * - Classes and instances
 * - Static members
 * - Enums
 * - Exception handling
 */

#include <hlffi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    printf("==============================================\n");
    printf("  HLFFI HLC Test Suite Runner\n");
    printf("==============================================\n\n");

    printf("HLFFI Version: %s\n", hlffi_get_version());
    printf("HashLink Version: %s\n", hlffi_get_hl_version());

#ifndef HLFFI_HLC_MODE
    printf("\nERROR: This test requires HLC mode!\n");
    printf("Build with -DHLFFI_HLC_MODE=ON\n");
    return 1;
#endif

    printf("Mode: HLC (HashLink/C) - code is statically linked\n\n");

    /* Step 1: Create VM */
    printf("[1] Creating VM...\n");
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }

    /* Step 2: Initialize */
    printf("[2] Initializing HashLink runtime...\n");
    hlffi_error_code err = hlffi_init(vm, argc, argv);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to init: %s\n", hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }

    /* Step 3: Load (no-op in HLC mode) */
    printf("[3] Loading module...\n");
    err = hlffi_load_file(vm, NULL);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Load failed: %s\n", hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }

    /* Step 4: Run test suite */
    printf("[4] Running HLC Test Suite...\n");
    printf("\n");

    err = hlffi_call_entry(vm);

    printf("\n");
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Test suite failed: %s\n", hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }

    /* Step 5: Cleanup */
    printf("[5] Cleaning up...\n");
    hlffi_destroy(vm);

    printf("\n=== HLC Test Suite Runner Complete ===\n");
    return 0;
}
