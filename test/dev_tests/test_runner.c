/**
 * Simple C test to load and run Haxe bytecode using HLFFI
 *
 * Build: gcc -o test_runner test_runner.c -Iinclude -Ivendor/hashlink/src -Lbin -lhlffi -lhl -ldl -lm -lpthread
 * Run: ./test_runner test/hello.hl
 */

#include <stdio.h>
#include <stdlib.h>
#include <hlffi.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];
    printf("HLFFI Test Runner\n");
    printf("================\n\n");

    // Show versions
    printf("HLFFI version: %s\n", hlffi_get_version());
    printf("HashLink version: %s\n", hlffi_get_hl_version());
    printf("Loading bytecode: %s\n\n", hl_file);

    // Step 1: Create VM
    printf("[1/5] Creating VM...\n");
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }
    printf("      VM created successfully\n\n");

    // Step 2: Initialize HashLink runtime
    printf("[2/5] Initializing HashLink runtime...\n");
    const char* init_args[] = {"test_runner"};
    hlffi_error_code err = hlffi_init(vm, 1, (char**)init_args);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to initialize: %s\n", hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }
    printf("      Runtime initialized\n\n");

    // Step 3: Load bytecode file
    printf("[3/5] Loading bytecode from %s...\n", hl_file);
    err = hlffi_load_file(vm, hl_file);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to load bytecode: %s\n", hlffi_get_error_string(err));
        fprintf(stderr, "       VM error: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("      Bytecode loaded successfully\n\n");

    // Step 4: Call entry point (main function)
    printf("[4/5] Calling entry point (main)...\n");
    printf("--------------------------------------------------\n");
    err = hlffi_call_entry(vm);
    printf("--------------------------------------------------\n");
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Entry point failed: %s\n", hlffi_get_error_string(err));
        fprintf(stderr, "       VM error: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("      Entry point completed successfully\n\n");

    // Step 5: Cleanup
    printf("[5/5] Cleaning up...\n");
    hlffi_destroy(vm);
    printf("      VM destroyed\n\n");

    printf("✓ Test completed successfully!\n");
    return 0;
}
