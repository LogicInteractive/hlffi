/**
 * HLFFI Phase 1 - Threading Tests
 *
 * Tests Mode 2 (THREADED) integration with dedicated VM thread
 *
 * NOTE: HashLink does not support multiple VMs per process.
 * All tests share a single VM instance.
 *
 * USAGE:
 *   bin/x64/Debug/test_threading test/threading_simple.hl
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

/* Test result tracking */
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) printf("\n[TEST] %s\n", name); fflush(stdout)
#define PASS() do { tests_passed++; printf("[PASS]\n"); fflush(stdout); } while(0)
#define FAIL(msg, ...) do { tests_failed++; printf("[FAIL] " msg "\n", ##__VA_ARGS__); fflush(stdout); } while(0)
#define CHECK(cond, msg, ...) do { if (!(cond)) { FAIL(msg, ##__VA_ARGS__); return 0; } } while(0)

/* Callback functions for thread tests */
static void increment_counter_callback(hlffi_vm* vm, void* userdata) {
    hlffi_call_static(vm, "ThreadingSimple", "incrementCounter", 0, NULL);
}

static void set_value_callback(hlffi_vm* vm, void* userdata) {
    int value = *(int*)userdata;
    hlffi_value* arg = hlffi_value_int(vm, value);
    hlffi_value* args[] = { arg };
    hlffi_call_static(vm, "ThreadingSimple", "setValue", 1, args);
    hlffi_value_free(arg);
}

static void expensive_op_callback(hlffi_vm* vm, void* userdata) {
    int iterations = *(int*)userdata;
    hlffi_value* arg = hlffi_value_int(vm, iterations);
    hlffi_value* args[] = { arg };
    hlffi_value* result = hlffi_call_static(vm, "ThreadingSimple", "expensiveOperation", 1, args);
    if (result) {
        printf("    Expensive operation result: %d\n", hlffi_value_as_int(result, 0));
        hlffi_value_free(result);
    }
    hlffi_value_free(arg);
}

static volatile int async_completed = 0;

static void async_completion_callback(hlffi_vm* vm, void* result, void* userdata) {
    (void)vm;
    (void)result;
    (void)userdata;
    async_completed = 1;
    printf("    Async callback completed\n");
    fflush(stdout);
}

/* ========== TEST 1: Thread Start/Stop ========== */
int test_thread_start_stop(hlffi_vm* vm) {
    TEST("Thread Start/Stop");

    printf("  Starting thread...\n"); fflush(stdout);
    CHECK(hlffi_thread_start(vm) == HLFFI_OK, "Failed to start thread: %s", hlffi_get_error(vm));
    CHECK(hlffi_thread_is_running(vm), "Thread not running after start");

    printf("  Waiting for thread to initialize...\n"); fflush(stdout);
    sleep_ms(200);

    printf("  Stopping thread...\n"); fflush(stdout);
    CHECK(hlffi_thread_stop(vm) == HLFFI_OK, "Failed to stop thread");
    CHECK(!hlffi_thread_is_running(vm), "Thread still running after stop");

    PASS();
    return 1;
}

/* ========== TEST 2: Restart Thread ========== */
int test_thread_restart(hlffi_vm* vm) {
    TEST("Thread Restart");

    printf("  Starting thread again...\n"); fflush(stdout);
    CHECK(hlffi_thread_start(vm) == HLFFI_OK, "Failed to restart thread: %s", hlffi_get_error(vm));
    CHECK(hlffi_thread_is_running(vm), "Thread not running after restart");

    sleep_ms(100);

    PASS();
    return 1;
}

/* ========== TEST 3: Synchronous Calls ========== */
int test_sync_calls(hlffi_vm* vm) {
    TEST("Synchronous Calls");

    printf("  Calling incrementCounter()...\n"); fflush(stdout);
    CHECK(hlffi_thread_call_sync(vm, increment_counter_callback, NULL) == HLFFI_OK, "Sync call failed");

    printf("  Calling incrementCounter() again...\n"); fflush(stdout);
    CHECK(hlffi_thread_call_sync(vm, increment_counter_callback, NULL) == HLFFI_OK, "Second sync call failed");

    int value = 42;
    printf("  Calling setValue(42)...\n"); fflush(stdout);
    CHECK(hlffi_thread_call_sync(vm, set_value_callback, &value) == HLFFI_OK, "Sync call with param failed");

    PASS();
    return 1;
}

/* ========== TEST 4: Asynchronous Calls ========== */
int test_async_calls(hlffi_vm* vm) {
    TEST("Asynchronous Calls");

    async_completed = 0;
    printf("  Calling incrementCounter() async...\n"); fflush(stdout);
    CHECK(hlffi_thread_call_async(vm, increment_counter_callback, async_completion_callback, NULL) == HLFFI_OK,
          "Async call failed");

    /* Wait for completion */
    int timeout = 0;
    while (!async_completed && timeout < 100) {
        sleep_ms(10);
        timeout++;
    }
    CHECK(async_completed, "Async callback did not complete within timeout");

    PASS();
    return 1;
}

/* ========== TEST 5: Multiple Concurrent Calls ========== */
int test_concurrent_calls(hlffi_vm* vm) {
    TEST("Multiple Concurrent Calls");

    printf("  Making 10 synchronous calls...\n"); fflush(stdout);
    for (int i = 0; i < 10; i++) {
        int value = i * 10;
        CHECK(hlffi_thread_call_sync(vm, set_value_callback, &value) == HLFFI_OK,
              "Concurrent call %d failed", i);
    }

    PASS();
    return 1;
}

/* ========== TEST 6: Expensive Operations ========== */
int test_expensive_ops(hlffi_vm* vm) {
    TEST("Expensive Operations");

    printf("  Making 3 calls with expensive operations...\n"); fflush(stdout);
    for (int i = 0; i < 3; i++) {
        int iterations = 10000 + (i * 5000);
        CHECK(hlffi_thread_call_sync(vm, expensive_op_callback, &iterations) == HLFFI_OK,
              "Expensive call %d failed", i);
    }

    PASS();
    return 1;
}

/* ========== TEST 7: Final Stop ========== */
int test_final_stop(hlffi_vm* vm) {
    TEST("Final Thread Stop");

    printf("  Stopping thread...\n"); fflush(stdout);
    CHECK(hlffi_thread_stop(vm) == HLFFI_OK, "Failed to stop thread");
    CHECK(!hlffi_thread_is_running(vm), "Thread still running after stop");

    PASS();
    return 1;
}

/* ========== MAIN ========== */
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <threading_simple.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];

    printf("============================================\n");
    printf("HLFFI Threading Tests (Phase 1)\n");
    printf("============================================\n");
    printf("Bytecode: %s\n", hl_file);
    fflush(stdout);

    /* Create single VM for all tests */
    printf("\nInitializing VM...\n"); fflush(stdout);

    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);

    if (hlffi_init(vm, 0, NULL) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM\n");
        hlffi_destroy(vm);
        return 1;
    }

    if (hlffi_load_file(vm, hl_file) != HLFFI_OK) {
        fprintf(stderr, "Failed to load bytecode: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    printf("VM ready.\n"); fflush(stdout);

    /* Run all tests with single VM */
    test_thread_start_stop(vm);
    test_thread_restart(vm);
    test_sync_calls(vm);
    test_async_calls(vm);
    test_concurrent_calls(vm);
    test_expensive_ops(vm);
    test_final_stop(vm);

    /* Print summary */
    printf("\n============================================\n");
    printf("RESULTS: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("============================================\n");

    /* Cleanup */
    hlffi_destroy(vm);

    return (tests_failed == 0) ? 0 : 1;
}
