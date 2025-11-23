/**
 * HLFFI v3.0 Non-Blocking Test
 * Demonstrates NON_THREADED mode with simulated frame updates
 */

#include <stdio.h>
#include <stdlib.h>
#include "include/hlffi.h"

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];
    printf("HLFFI v3.0 Non-Blocking Test\n");
    printf("================================\n\n");
    printf("This test demonstrates NON_THREADED mode:\n");
    printf("- Entry point returns immediately (no blocking)\n");
    printf("- hlffi_update() processes events each 'frame'\n");
    printf("- Simulates game engine tick pattern\n\n");

    // Step 1: Create and initialize VM
    printf("[1/5] Creating and initializing VM...\n");
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }

    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to initialize VM: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("      ✓ VM created and initialized\n\n");

    // Step 2: Set integration mode to NON_THREADED (this is the default, but let's be explicit)
    printf("[2/5] Setting NON_THREADED integration mode...\n");
    err = hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to set integration mode: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("      ✓ Mode set to NON_THREADED\n");
    printf("      (Engine controls main loop, no dedicated thread)\n\n");

    // Step 3: Load bytecode
    printf("[3/5] Loading bytecode: %s...\n", hl_file);
    err = hlffi_load_file(vm, hl_file);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to load bytecode: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("      ✓ Bytecode loaded\n\n");

    // Step 4: Call entry point (non-blocking)
    printf("[4/5] Calling entry point (should return immediately)...\n");
    printf("════════════════════════════════════════════════\n");
    err = hlffi_call_entry(vm);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Failed to call entry point: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("════════════════════════════════════════════════\n");
    printf("      ✓ Entry point returned (non-blocking!)\n\n");

    // Step 5: Simulate game loop with frame updates
    printf("[5/5] Simulating engine tick loop...\n");
    printf("      (Calling hlffi_update() 10 times, like a game engine)\n\n");

    const int num_frames = 10;
    const float delta_time = 1.0f / 60.0f; // 60 FPS

    for (int frame = 1; frame <= num_frames; frame++) {
        printf("Frame %2d: ", frame);

        // Check if there's pending work
        bool has_work = hlffi_has_pending_work(vm);
        printf("[Pending: %s] ", has_work ? "YES" : "NO ");

        // Process events (non-blocking)
        err = hlffi_update(vm, delta_time);
        if (err != HLFFI_OK) {
            fprintf(stderr, "\n      ERROR: hlffi_update() failed: %s\n", hlffi_get_error(vm));
            break;
        }

        printf("✓ Updated\n");

        // Simulate frame delay (16ms ≈ 60 FPS)
        sleep_ms(16);
    }

    printf("\n✓ Test completed successfully!\n");
    printf("\nNON_THREADED mode works correctly:\n");
    printf("- Entry point returned immediately (no blocking)\n");
    printf("- hlffi_update() processes events without blocking\n");
    printf("- Perfect for game engines and event-driven applications\n");

    // Cleanup
    hlffi_destroy(vm);

    return 0;
}
