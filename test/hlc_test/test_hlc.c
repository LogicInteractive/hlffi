/**
 * HLFFI HLC Mode Test
 *
 * This test demonstrates HLFFI running in HLC (HashLink/C) mode.
 * In HLC mode, Haxe code is compiled to C and statically linked,
 * rather than running through the JIT VM.
 */

#include <hlffi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    printf("=== HLFFI HLC Mode Test ===\n\n");

    printf("HLFFI Version: %s\n", hlffi_get_version());
    printf("HashLink Version: %s\n", hlffi_get_hl_version());

#ifdef HLFFI_HLC_MODE
    printf("Mode: HLC (HashLink/C) - code is statically linked\n\n");
#else
    printf("Mode: JIT (bytecode) - this test requires HLC mode!\n");
    printf("Build with -DHLFFI_HLC_MODE=ON\n");
    return 1;
#endif

    // Step 1: Create VM
    printf("[1] Creating VM...\n");
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }
    printf("    VM created successfully\n");

    // Step 2: Initialize HashLink runtime
    printf("[2] Initializing HashLink runtime...\n");
    hlffi_error_code err = hlffi_init(vm, argc, argv);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to initialize VM: %s\n",
                hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }
    printf("    Runtime initialized\n");

    // Step 3: In HLC mode, load_file is a no-op (code is statically linked)
    printf("[3] Loading module (no-op in HLC mode)...\n");
    err = hlffi_load_file(vm, NULL);  // path is ignored in HLC mode
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: hlffi_load_file failed: %s\n",
                hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }
    printf("    Module marked as loaded\n");

    // Step 4: Call entry point (calls hl_entry_point() in HLC mode)
    printf("[4] Calling Haxe entry point...\n");
    printf("----------------------------------------\n");
    err = hlffi_call_entry(vm);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to call entry point: %s\n",
                hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }
    printf("----------------------------------------\n");
    printf("    Entry point returned successfully\n");

    // Step 5: Test HLC-specific features
    printf("[5] Testing HLC mode characteristics...\n");

#ifdef HLFFI_HLC_MODE
    printf("    HLC Mode: enabled (code statically linked)\n");
    printf("    Hot reload: not available in HLC mode\n");
    printf("    Bytecode loading: not available in HLC mode\n");
#else
    printf("    JIT Mode: enabled (bytecode loaded at runtime)\n");
#endif

    // Step 6: Cleanup
    printf("[6] Cleaning up VM...\n");
    hlffi_destroy(vm);
    printf("    VM destroyed\n");

    printf("\n=== HLC Test completed successfully ===\n");
    return 0;
}
