/**
 * HLFFI Threaded VM Restart Test - BLOCKING version
 *
 * Tests running VM in threaded mode where Haxe main() runs a blocking loop
 * that counts to 5 with 1 second sleep, then exits.
 * After main() completes, we stop the thread, wait 1 second, and restart.
 *
 * USAGE:
 *   test_threaded_blocking test/threaded_counter_blocking.hl
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

/**
 * Run a single session:
 * - Start VM thread (Haxe main() runs blocking loop, exits at count 5)
 * - Wait for Haxe to finish counting
 * - Stop VM thread
 */
int run_blocking_session(const char* hl_file, int session_num) {
    printf("\n========================================\n");
    printf("SESSION %d (Blocking)\n", session_num);
    printf("========================================\n\n");

    /* Create VM */
    printf("[%d.1] Creating VM...\n", session_num);
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 0;
    }
    printf("  OK: VM at %p\n", (void*)vm);

    /* Set THREADED mode */
    printf("[%d.2] Setting THREADED mode...\n", session_num);
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);

    /* Initialize */
    printf("[%d.3] Initializing VM...\n", session_num);
    if (hlffi_init(vm, 0, NULL) != HLFFI_OK) {
        fprintf(stderr, "Failed to init: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 0;
    }
    printf("  OK\n");

    /* Load bytecode */
    printf("[%d.4] Loading bytecode...\n", session_num);
    if (hlffi_load_file(vm, hl_file) != HLFFI_OK) {
        fprintf(stderr, "Failed to load: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 0;
    }
    printf("  OK\n");

    /* Start VM thread - Haxe main() runs blocking loop (counts to 5) */
    printf("[%d.5] Starting VM thread (Haxe will count to 5 then exit)...\n", session_num);
    if (hlffi_thread_start(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to start thread: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 0;
    }
    printf("  OK - Thread running, Haxe counting...\n\n");

    /* Wait for Haxe main() to finish (should take ~5 seconds) */
    printf("[%d.6] Waiting for Haxe to finish counting...\n", session_num);
    for (int i = 0; i < 8; i++) {  /* Wait up to 8 seconds */
        sleep_ms(1000);
        printf("  [Main thread] %d second(s) elapsed\n", i + 1);
    }

    printf("\n[%d.7] Stopping VM thread...\n", session_num);
    if (hlffi_thread_stop(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to stop thread: %s\n", hlffi_get_error(vm));
    }
    printf("  OK - Thread stopped\n");

    /* Destroy VM */
    printf("[%d.8] Destroying VM...\n", session_num);
    hlffi_destroy(vm);
    printf("  OK\n");

    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <threaded_counter_blocking.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];

    printf("============================================\n");
    printf("Threaded VM Restart Test (BLOCKING)\n");
    printf("============================================\n");
    printf("Bytecode: %s\n", hl_file);
    printf("\nThis test starts VM in threaded mode. Haxe main() runs a\n");
    printf("blocking loop that counts to 5 with Sys.sleep(1), then exits.\n");
    printf("After completion, VM is stopped, wait 1 sec, then restart.\n");

    /* Session 1 */
    if (!run_blocking_session(hl_file, 1)) {
        fprintf(stderr, "\nSession 1 failed!\n");
        return 1;
    }

    /* Wait 1 second between sessions */
    printf("\n--- Waiting 1 second before restart ---\n");
    sleep_ms(1000);

    /* Session 2 */
    if (!run_blocking_session(hl_file, 2)) {
        fprintf(stderr, "\nSession 2 failed!\n");
        return 1;
    }

    /* Wait 1 second between sessions */
    printf("\n--- Waiting 1 second before restart ---\n");
    sleep_ms(1000);

    /* Session 3 */
    if (!run_blocking_session(hl_file, 3)) {
        fprintf(stderr, "\nSession 3 failed!\n");
        return 1;
    }

    printf("\n============================================\n");
    printf("All 3 blocking sessions completed!\n");
    printf("============================================\n");

    return 0;
}
