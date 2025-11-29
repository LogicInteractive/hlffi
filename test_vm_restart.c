/**
 * Test VM restart behavior
 *
 * HashLink uses global state, so restarting VMs may not work cleanly.
 * This test explores what happens when we:
 * 1. Create VM, do stuff, destroy it
 * 2. Wait
 * 3. Create a new VM, do stuff, destroy it
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

static void do_some_haxe_stuff(hlffi_vm* vm, int iteration) {
    printf("  Calling ThreadingSimple.incrementCounter()...\n");
    hlffi_value* result = hlffi_call_static(vm, "ThreadingSimple", "incrementCounter", 0, NULL);
    if (result) {
        hlffi_value_free(result);
    }

    printf("  Calling ThreadingSimple.setValue(%d)...\n", iteration * 100);
    hlffi_value* arg = hlffi_value_int(vm, iteration * 100);
    hlffi_value* args[] = { arg };
    result = hlffi_call_static(vm, "ThreadingSimple", "setValue", 1, args);
    if (result) {
        hlffi_value_free(result);
    }
    hlffi_value_free(arg);

    printf("  Calling ThreadingSimple.getCounter()...\n");
    result = hlffi_call_static(vm, "ThreadingSimple", "getCounter", 0, NULL);
    if (result) {
        printf("  Counter = %d\n", hlffi_value_as_int(result, -1));
        hlffi_value_free(result);
    }
}

int run_vm_session(const char* hl_file, int session_num) {
    printf("\n========================================\n");
    printf("SESSION %d\n", session_num);
    printf("========================================\n\n");

    /* Step 1: Create VM */
    printf("[%d.1] Creating VM...\n", session_num);
    fflush(stdout);
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        printf("FAILED: hlffi_create() returned NULL\n");
        return 0;
    }
    printf("  OK: VM at %p\n", (void*)vm);

    /* Step 2: Initialize */
    printf("[%d.2] Initializing VM...\n", session_num);
    fflush(stdout);
    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        printf("FAILED: hlffi_init() returned %d: %s\n", err, hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 0;
    }
    printf("  OK\n");

    /* Step 3: Load bytecode */
    printf("[%d.3] Loading bytecode...\n", session_num);
    fflush(stdout);
    err = hlffi_load_file(vm, hl_file);
    if (err != HLFFI_OK) {
        printf("FAILED: hlffi_load_file() returned %d: %s\n", err, hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 0;
    }
    printf("  OK\n");

    /* Step 4: Call entry point */
    printf("[%d.4] Calling entry point...\n", session_num);
    fflush(stdout);
    err = hlffi_call_entry(vm);
    if (err != HLFFI_OK) {
        printf("FAILED: hlffi_call_entry() returned %d: %s\n", err, hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 0;
    }
    printf("  OK\n");

    /* Step 5: Do some Haxe stuff */
    printf("[%d.5] Doing Haxe stuff...\n", session_num);
    fflush(stdout);
    do_some_haxe_stuff(vm, session_num);
    printf("  OK\n");

    /* Step 6: Destroy VM */
    printf("[%d.6] Destroying VM...\n", session_num);
    fflush(stdout);
    hlffi_destroy(vm);
    printf("  OK\n");

    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <threading_simple.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];

    printf("============================================\n");
    printf("VM Restart Test\n");
    printf("============================================\n");
    printf("Bytecode: %s\n", hl_file);
    printf("\nThis test creates, uses, and destroys the VM multiple times\n");
    printf("to see if HashLink's global state allows clean restarts.\n");
    fflush(stdout);

    /* Session 1 */
    if (!run_vm_session(hl_file, 1)) {
        printf("\nSession 1 FAILED\n");
        return 1;
    }

    /* Wait */
    printf("\n--- Waiting 1 second before restart ---\n");
    fflush(stdout);
    sleep_ms(1000);

    /* Session 2 */
    if (!run_vm_session(hl_file, 2)) {
        printf("\nSession 2 FAILED\n");
        return 1;
    }

    /* Wait */
    printf("\n--- Waiting 1 second before restart ---\n");
    fflush(stdout);
    sleep_ms(1000);

    /* Session 3 */
    if (!run_vm_session(hl_file, 3)) {
        printf("\nSession 3 FAILED\n");
        return 1;
    }

    printf("\n============================================\n");
    printf("All 3 sessions completed!\n");
    printf("============================================\n");

    return 0;
}
