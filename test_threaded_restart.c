/**
 * HLFFI Threaded VM Restart Test
 *
 * Tests running VM in threaded mode where C drives the counting.
 * The Haxe incrementAndWait() method increments and sleeps 1 second.
 * After 5 calls, we stop the VM, wait 1 second, then restart.
 *
 * USAGE:
 *   test_threaded_restart test/threaded_counter.hl
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

/* Callback to increment counter and wait (1 sec sleep in Haxe) */
typedef struct {
    int counter_value;
} increment_result_t;

static void increment_and_wait_callback(hlffi_vm* vm, void* userdata) {
    increment_result_t* result = (increment_result_t*)userdata;
    hlffi_value* val = hlffi_call_static(vm, "ThreadedCounter", "incrementAndWait", 0, NULL);
    if (val) {
        result->counter_value = hlffi_value_as_int(val, -1);
        hlffi_value_free(val);
    }
}

/* Callback to get counter value */
static void get_counter_callback(hlffi_vm* vm, void* userdata) {
    increment_result_t* result = (increment_result_t*)userdata;
    hlffi_value* val = hlffi_call_static(vm, "ThreadedCounter", "getCounter", 0, NULL);
    if (val) {
        result->counter_value = hlffi_value_as_int(val, -1);
        hlffi_value_free(val);
    }
}

/* Callback to reset counter */
static void reset_callback(hlffi_vm* vm, void* userdata) {
    (void)userdata;
    hlffi_call_static(vm, "ThreadedCounter", "reset", 0, NULL);
}

/**
 * Run a single session:
 * - Start VM thread (Haxe main() returns immediately)
 * - Call incrementAndWait() 5 times (each sleeps 1 sec in Haxe)
 * - Stop VM thread
 */
int run_threaded_session(const char* hl_file, int session_num) {
    printf("\n========================================\n");
    printf("SESSION %d (Threaded)\n", session_num);
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

    /* Start VM thread - Haxe main() initializes and returns immediately */
    printf("[%d.5] Starting VM thread...\n", session_num);
    if (hlffi_thread_start(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to start thread: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 0;
    }
    printf("  OK - Thread running\n\n");

    /* Give VM thread time to initialize */
    sleep_ms(100);

    /* Reset counter (in case of restart with stale state) */
    printf("[%d.6] Resetting counter...\n", session_num);
    hlffi_thread_call_sync(vm, reset_callback, NULL);
    printf("  OK\n\n");

    /* Call incrementAndWait() 5 times - each call sleeps 1 sec in Haxe */
    printf("[%d.7] Counting loop (5 increments with 1s sleep each)...\n\n", session_num);
    for (int i = 0; i < 5; i++) {
        printf("  [Main] Calling incrementAndWait()...\n");
        increment_result_t result = { .counter_value = -1 };
        if (hlffi_thread_call_sync(vm, increment_and_wait_callback, &result) == HLFFI_OK) {
            printf("  [Main] Counter is now: %d (after 1 sec sleep)\n\n", result.counter_value);
        } else {
            fprintf(stderr, "  [Main] Call failed: %s\n", hlffi_get_error(vm));
        }
    }

    /* Get final counter value */
    printf("[%d.8] Getting final counter value...\n", session_num);
    increment_result_t final_result = { .counter_value = -1 };
    if (hlffi_thread_call_sync(vm, get_counter_callback, &final_result) == HLFFI_OK) {
        printf("  Final counter: %d\n", final_result.counter_value);
    }

    printf("\n[%d.9] Stopping VM thread...\n", session_num);
    if (hlffi_thread_stop(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to stop thread: %s\n", hlffi_get_error(vm));
    }
    printf("  OK - Thread stopped\n");

    /* Destroy VM */
    printf("[%d.10] Destroying VM...\n", session_num);
    hlffi_destroy(vm);
    printf("  OK\n");

    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <threaded_counter.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];

    printf("============================================\n");
    printf("Threaded VM Restart Test\n");
    printf("============================================\n");
    printf("Bytecode: %s\n", hl_file);
    printf("\nThis test starts VM in threaded mode. The C host calls\n");
    printf("incrementAndWait() 5 times (each sleeps 1 sec in Haxe).\n");
    printf("Then stops the VM, waits 1 second, and restarts.\n");

    /* Session 1 */
    if (!run_threaded_session(hl_file, 1)) {
        fprintf(stderr, "\nSession 1 failed!\n");
        return 1;
    }

    /* Wait 1 second between sessions */
    printf("\n--- Waiting 1 second before restart ---\n");
    sleep_ms(1000);

    /* Session 2 */
    if (!run_threaded_session(hl_file, 2)) {
        fprintf(stderr, "\nSession 2 failed!\n");
        return 1;
    }

    /* Wait 1 second between sessions */
    printf("\n--- Waiting 1 second before restart ---\n");
    sleep_ms(1000);

    /* Session 3 */
    if (!run_threaded_session(hl_file, 3)) {
        fprintf(stderr, "\nSession 3 failed!\n");
        return 1;
    }

    printf("\n============================================\n");
    printf("All 3 threaded sessions completed!\n");
    printf("============================================\n");

    return 0;
}
