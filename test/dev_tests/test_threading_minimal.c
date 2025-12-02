/**
 * Minimal threading test to isolate segfault
 */

#include "include/hlffi.h"
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <threading_simple.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Minimal Threading Test ===\n");
    printf("Bytecode: %s\n\n", argv[1]);

    /* Step 1: Create VM */
    printf("Step 1: Creating VM...\n");
    fflush(stdout);
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        printf("FAILED: hlffi_create() returned NULL\n");
        return 1;
    }
    printf("  OK: VM created at %p\n", (void*)vm);
    fflush(stdout);

    /* Step 2: Set mode */
    printf("Step 2: Setting THREADED mode...\n");
    fflush(stdout);
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    printf("  OK\n");
    fflush(stdout);

    /* Step 3: Initialize */
    printf("Step 3: Initializing VM...\n");
    fflush(stdout);
    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        printf("FAILED: hlffi_init() returned %d: %s\n", err, hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("  OK\n");
    fflush(stdout);

    /* Step 4: Load file */
    printf("Step 4: Loading bytecode...\n");
    fflush(stdout);
    err = hlffi_load_file(vm, argv[1]);
    if (err != HLFFI_OK) {
        printf("FAILED: hlffi_load_file() returned %d: %s\n", err, hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("  OK\n");
    fflush(stdout);

    /* Step 5: Start thread */
    printf("Step 5: Starting thread...\n");
    fflush(stdout);
    err = hlffi_thread_start(vm);
    if (err != HLFFI_OK) {
        printf("FAILED: hlffi_thread_start() returned %d: %s\n", err, hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("  OK: Thread started\n");
    fflush(stdout);

    /* Wait a bit */
    printf("Step 6: Waiting 500ms for thread to initialize...\n");
    fflush(stdout);
    sleep_ms(500);
    printf("  OK\n");
    fflush(stdout);

    /* Step 7: Check thread state */
    printf("Step 7: Checking thread state...\n");
    fflush(stdout);
    bool running = hlffi_thread_is_running(vm);
    printf("  Thread running: %s\n", running ? "yes" : "no");
    fflush(stdout);

    /* Step 8: Stop thread */
    printf("Step 8: Stopping thread...\n");
    fflush(stdout);
    err = hlffi_thread_stop(vm);
    if (err != HLFFI_OK) {
        printf("FAILED: hlffi_thread_stop() returned %d: %s\n", err, hlffi_get_error(vm));
    } else {
        printf("  OK: Thread stopped\n");
    }
    fflush(stdout);

    /* Step 9: Cleanup */
    printf("Step 9: Destroying VM...\n");
    fflush(stdout);
    hlffi_destroy(vm);
    printf("  OK\n");

    printf("\n=== Test Complete ===\n");
    return 0;
}
