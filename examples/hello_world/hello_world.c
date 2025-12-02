/*
 * HLFFI Example: Hello World
 *
 * This example demonstrates the basic VM lifecycle:
 * 1. Create VM
 * 2. Initialize HashLink runtime
 * 3. Load bytecode file
 * 4. Call entry point
 * 5. Cleanup
 */

#include <hlffi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    printf("=== HLFFI Hello World Example ===\n\n");

    // Check for bytecode file argument
    const char* hl_file = "hello.hl";
    if (argc > 1) {
        hl_file = argv[1];
    }

    printf("HLFFI Version: %s\n", hlffi_get_version());
    printf("HashLink Version: %s\n", hlffi_get_hl_version());
    printf("Loading: %s\n\n", hl_file);

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
    const char* init_args[] = {"hello_world"};
    hlffi_error_code err = hlffi_init(vm, 1, (char**)init_args);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to initialize VM: %s\n",
                hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }
    printf("    Runtime initialized\n");

    // Step 3: Load bytecode file
    printf("[3] Loading bytecode file: %s\n", hl_file);
    err = hlffi_load_file(vm, hl_file);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to load bytecode: %s\n",
                hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }
    printf("    Bytecode loaded successfully\n");

    // Step 4: Set integration mode (non-threaded)
    printf("[4] Setting integration mode (NON_THREADED)...\n");
    err = hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to set integration mode: %s\n",
                hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }
    printf("    Integration mode set\n");

    // Step 5: Call entry point
    printf("[5] Calling Haxe main() entry point...\n");
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

    // Step 6: Process event loops (if Haxe code uses timers/async)
    printf("[6] Processing event loops...\n");
    float delta_time = 0.016f; // 60 FPS
    int frames = 0;
    int max_frames = 10; // Process 10 frames worth of events

    while (frames < max_frames && hlffi_has_pending_work(vm)) {
        err = hlffi_update(vm, delta_time);
        if (err != HLFFI_OK) {
            fprintf(stderr, "ERROR: Failed to update: %s\n",
                    hlffi_get_error_string(err));
            break;
        }
        frames++;
    }
    printf("    Processed %d frame(s)\n", frames);

    // Step 7: Cleanup
    printf("[7] Cleaning up VM...\n");
    hlffi_destroy(vm);
    printf("    VM destroyed\n");

    printf("\n=== Example completed successfully ===\n");
    return 0;
}
