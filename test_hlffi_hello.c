/**
 * HLFFI v3.0 Hello World Test
 * Simple test that loads and runs Haxe bytecode using HLFFI API
 */

#include <stdio.h>
#include <stdlib.h>
#include "include/hlffi.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];
    printf("HLFFI v3.0 Hello World Test\n");
    printf("============================\n\n");
    printf("Loading: %s\n\n", hl_file);

    // Step 1: Create VM
    printf("[1/4] Creating VM...\n");
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }
    printf("      ✓ VM created\n\n");

    // Step 2: Initialize VM
    printf("[2/4] Initializing VM...\n");
    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to initialize VM (code: %d)\n", err);
        return 1;
    }
    printf("      ✓ VM initialized\n\n");

    // Step 3: Load bytecode
    printf("[3/4] Loading bytecode...\n");
    err = hlffi_load_file(vm, hl_file);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to load bytecode (code: %d)\n", err);
        return 1;
    }
    printf("      ✓ Bytecode loaded\n\n");

    // Step 4: Call entry point
    printf("[4/4] Calling entry point...\n");
    printf("════════════════════════════════════════════════\n");
    err = hlffi_call_entry(vm);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to call entry point (code: %d)\n", err);
        return 1;
    }
    printf("════════════════════════════════════════════════\n");
    printf("      ✓ Entry point executed\n\n");

    printf("✓ Test completed successfully!\n");
    printf("\nThe Haxe bytecode executed correctly using HLFFI v3.0\n");

    return 0;
}
