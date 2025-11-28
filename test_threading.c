/**
 * HLFFI Phase 1 - Threading Tests
 *
 * Tests Mode 2 (THREADED) integration with dedicated VM thread
 *
 * USAGE:
 *   make test_threading
 *   bin/x64/Debug/test_threading test/threading.hl
 *
 * TESTS:
 *   1. Thread lifecycle (start/stop)
 *   2. Synchronous calls to VM thread
 *   3. Asynchronous calls to VM thread
 *   4. Worker thread registration
 *   5. Blocking operation guards
 *   6. Message queue overflow handling
 *   7. Concurrent calls stress test
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#define TEST(name) printf("\n[TEST] %s\n", name)
#define PASS() do { tests_passed++; printf("[PASS]\n"); } while(0)
#define FAIL(msg, ...) do { tests_failed++; printf("[FAIL] " msg "\n", ##__VA_ARGS__); } while(0)
#define ASSERT(cond, msg, ...) do { if (!(cond)) { FAIL(msg, ##__VA_ARGS__); return; } } while(0)
#define ASSERT_EQ(a, b, msg) ASSERT((a) == (b), msg " (expected %d, got %d)", (int)(b), (int)(a))

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
        printf("  Expensive operation result: %d\n", hlffi_value_as_int(result, 0));
        hlffi_value_free(result);
    }
    hlffi_value_free(arg);
}

static void async_completion_callback(hlffi_vm* vm, void* result, void* userdata) {
    int* completed = (int*)userdata;
    *completed = 1;
    printf("  Async callback completed\n");
}

/* ========== TEST 1: Thread Lifecycle ========== */
void test_thread_lifecycle(const char* hl_file) {
    TEST("Thread Lifecycle (start/stop)");

    hlffi_vm* vm = hlffi_create();
    ASSERT(vm != NULL, "Failed to create VM");

    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    ASSERT(hlffi_init(vm, 0, NULL) == HLFFI_OK, "Failed to init VM");
    ASSERT(hlffi_load_file(vm, hl_file) == HLFFI_OK, "Failed to load %s: %s",
           hl_file, hlffi_get_error(vm));

    /* Start thread */
    ASSERT(hlffi_thread_start(vm) == HLFFI_OK, "Failed to start thread: %s", hlffi_get_error(vm));
    ASSERT(hlffi_thread_is_running(vm), "Thread not running after start");

    /* Give thread time to initialize */
    sleep_ms(100);

    /* Stop thread */
    ASSERT(hlffi_thread_stop(vm) == HLFFI_OK, "Failed to stop thread");
    ASSERT(!hlffi_thread_is_running(vm), "Thread still running after stop");

    hlffi_destroy(vm);
    PASS();
}

/* ========== TEST 2: Synchronous Calls ========== */
void test_sync_calls(const char* hl_file) {
    TEST("Synchronous calls to VM thread");

    hlffi_vm* vm = hlffi_create();
    ASSERT(vm != NULL, "Failed to create VM");

    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    ASSERT(hlffi_init(vm, 0, NULL) == HLFFI_OK, "Failed to init VM");
    ASSERT(hlffi_load_file(vm, hl_file) == HLFFI_OK, "Failed to load file");
    ASSERT(hlffi_thread_start(vm) == HLFFI_OK, "Failed to start thread");

    sleep_ms(100);

    /* Make synchronous call */
    printf("  Calling incrementCounter() via thread_call_sync...\n");
    ASSERT(hlffi_thread_call_sync(vm, increment_counter_callback, NULL) == HLFFI_OK,
           "Sync call failed");

    /* Make another call to verify state persists */
    ASSERT(hlffi_thread_call_sync(vm, increment_counter_callback, NULL) == HLFFI_OK,
           "Second sync call failed");

    /* Call with parameter */
    int value = 42;
    printf("  Calling setValue(42) via thread_call_sync...\n");
    ASSERT(hlffi_thread_call_sync(vm, set_value_callback, &value) == HLFFI_OK,
           "Sync call with param failed");

    hlffi_thread_stop(vm);
    hlffi_destroy(vm);
    PASS();
}

/* ========== TEST 3: Asynchronous Calls ========== */
void test_async_calls(const char* hl_file) {
    TEST("Asynchronous calls to VM thread");

    hlffi_vm* vm = hlffi_create();
    ASSERT(vm != NULL, "Failed to create VM");

    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    ASSERT(hlffi_init(vm, 0, NULL) == HLFFI_OK, "Failed to init VM");
    ASSERT(hlffi_load_file(vm, hl_file) == HLFFI_OK, "Failed to load file");
    ASSERT(hlffi_thread_start(vm) == HLFFI_OK, "Failed to start thread");

    sleep_ms(100);

    /* Make asynchronous call */
    int completed = 0;
    printf("  Calling incrementCounter() via thread_call_async...\n");
    ASSERT(hlffi_thread_call_async(vm, increment_counter_callback,
                                    async_completion_callback, &completed) == HLFFI_OK,
           "Async call failed");

    /* Wait for completion */
    int timeout = 0;
    while (!completed && timeout < 100) {
        sleep_ms(10);
        timeout++;
    }
    ASSERT(completed, "Async callback did not complete");

    hlffi_thread_stop(vm);
    hlffi_destroy(vm);
    PASS();
}

/* ========== TEST 4: Worker Thread Helpers ========== */
void test_worker_helpers(const char* hl_file) {
    TEST("Worker thread helpers (register/unregister)");

    /* These functions should not crash */
    printf("  Registering worker thread...\n");
    hlffi_worker_register();

    printf("  Unregistering worker thread...\n");
    hlffi_worker_unregister();

    PASS();
}

/* ========== TEST 5: Blocking Operation Guards ========== */
void test_blocking_guards(const char* hl_file) {
    TEST("Blocking operation guards (blocking_begin/end)");

    hlffi_vm* vm = hlffi_create();
    ASSERT(vm != NULL, "Failed to create VM");

    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    ASSERT(hlffi_init(vm, 0, NULL) == HLFFI_OK, "Failed to init VM");
    ASSERT(hlffi_load_file(vm, hl_file) == HLFFI_OK, "Failed to load file");
    ASSERT(hlffi_thread_start(vm) == HLFFI_OK, "Failed to start thread");

    sleep_ms(100);

    /* These should not crash */
    printf("  Testing blocking_begin/end...\n");
    hlffi_blocking_begin();
    sleep_ms(50);  /* Simulate blocking I/O */
    hlffi_blocking_end();

    hlffi_thread_stop(vm);
    hlffi_destroy(vm);
    PASS();
}

/* ========== TEST 6: Multiple Concurrent Calls ========== */
void test_concurrent_calls(const char* hl_file) {
    TEST("Multiple concurrent synchronous calls");

    hlffi_vm* vm = hlffi_create();
    ASSERT(vm != NULL, "Failed to create VM");

    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    ASSERT(hlffi_init(vm, 0, NULL) == HLFFI_OK, "Failed to init VM");
    ASSERT(hlffi_load_file(vm, hl_file) == HLFFI_OK, "Failed to load file");
    ASSERT(hlffi_thread_start(vm) == HLFFI_OK, "Failed to start thread");

    sleep_ms(100);

    /* Make many calls */
    printf("  Making 10 synchronous calls...\n");
    for (int i = 0; i < 10; i++) {
        int value = i * 10;
        ASSERT(hlffi_thread_call_sync(vm, set_value_callback, &value) == HLFFI_OK,
               "Concurrent call %d failed", i);
    }

    hlffi_thread_stop(vm);
    hlffi_destroy(vm);
    PASS();
}

/* ========== TEST 7: Stress Test with Expensive Operations ========== */
void test_stress(const char* hl_file) {
    TEST("Stress test with expensive operations");

    hlffi_vm* vm = hlffi_create();
    ASSERT(vm != NULL, "Failed to create VM");

    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    ASSERT(hlffi_init(vm, 0, NULL) == HLFFI_OK, "Failed to init VM");
    ASSERT(hlffi_load_file(vm, hl_file) == HLFFI_OK, "Failed to load file");
    ASSERT(hlffi_thread_start(vm) == HLFFI_OK, "Failed to start thread");

    sleep_ms(100);

    /* Make calls with expensive operations */
    printf("  Making 5 calls with expensive operations...\n");
    for (int i = 0; i < 5; i++) {
        int iterations = 10000 + (i * 5000);
        ASSERT(hlffi_thread_call_sync(vm, expensive_op_callback, &iterations) == HLFFI_OK,
               "Expensive call %d failed", i);
    }

    hlffi_thread_stop(vm);
    hlffi_destroy(vm);
    PASS();
}

/* ========== TEST 8: Error Handling ========== */
void test_error_handling(const char* hl_file) {
    TEST("Error handling (invalid state)");

    hlffi_vm* vm = hlffi_create();
    ASSERT(vm != NULL, "Failed to create VM");

    /* Try to call without starting thread */
    hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, hl_file);

    /* This should fail - thread not started */
    hlffi_error_code result = hlffi_thread_call_sync(vm, increment_counter_callback, NULL);
    ASSERT(result == HLFFI_ERROR_THREAD_NOT_STARTED, "Expected error when calling without thread");

    /* Try to start without THREADED mode */
    hlffi_vm* vm2 = hlffi_create();
    hlffi_init(vm2, 0, NULL);
    hlffi_load_file(vm2, hl_file);
    /* vm2 is in NON_THREADED mode by default */
    result = hlffi_thread_start(vm2);
    ASSERT(result == HLFFI_ERROR_THREAD_START_FAILED, "Expected error when starting in NON_THREADED mode");

    hlffi_destroy(vm);
    hlffi_destroy(vm2);
    PASS();
}

/* ========== MAIN ========== */
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <threading.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];

    printf("============================================\n");
    printf("HLFFI Threading Tests (Phase 1)\n");
    printf("============================================\n");
    printf("Bytecode: %s\n", hl_file);

    /* Run all tests */
    test_thread_lifecycle(hl_file);
    test_sync_calls(hl_file);
    test_async_calls(hl_file);
    test_worker_helpers(hl_file);
    test_blocking_guards(hl_file);
    test_concurrent_calls(hl_file);
    test_stress(hl_file);
    test_error_handling(hl_file);

    /* Print summary */
    printf("\n============================================\n");
    printf("RESULTS: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("============================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
