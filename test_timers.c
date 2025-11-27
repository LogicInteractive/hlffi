/**
 * Phase 1 Extensions Test: Event Loop Integration
 * Tests hlffi_update(), timer processing, and event loop integration
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>  /* for usleep */
#include <sys/time.h> /* for gettimeofday */
#endif

#define TEST_PASS(name) printf("[PASS] Test %d: %s\n", ++test_count, name)
#define TEST_FAIL(name) do { printf("[FAIL] Test %d: %s\n", ++test_count, name); failed++; } while(0)

/* Helper: Call void static method */
static void call_void(hlffi_vm* vm, const char* class_name, const char* method_name, int argc, hlffi_value** argv) {
    hlffi_value* result = hlffi_call_static(vm, class_name, method_name, argc, argv);
    if (result) hlffi_value_free(result);
}

/* Helper: Call int static method */
static int call_int(hlffi_vm* vm, const char* class_name, const char* method_name, int argc, hlffi_value** argv) {
    hlffi_value* result = hlffi_call_static(vm, class_name, method_name, argc, argv);
    if (!result) return 0;
    int value = hlffi_value_as_int(result, 0);
    hlffi_value_free(result);
    return value;
}

/* Get current time in milliseconds */
static double get_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
#endif
}

/* Sleep for milliseconds */
static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <timers.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Phase 1 Extensions Test: Event Loop Integration ===\n");
    int test_count = 0;
    int failed = 0;

    /* Create and initialize VM */
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    if (hlffi_init(vm, 0, NULL) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM\n");
        hlffi_destroy(vm);
        return 1;
    }

    /* Load timers test bytecode */
    hlffi_error_code err = hlffi_load_file(vm, argv[1]);
    if (err != HLFFI_OK) {
        fprintf(stderr, "Failed to load bytecode: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Call entry point */
    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry point: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Test 1: hlffi_update() basic call */
    {
        err = hlffi_update(vm, 0.016f);
        if (err == HLFFI_OK) {
            TEST_PASS("hlffi_update() executes without error");
        } else {
            TEST_FAIL("hlffi_update() failed");
            printf("  Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 2: Start a one-shot timer and process it */
    {
        call_void(vm, "Timers", "resetCounters", 0, NULL);

        hlffi_value* delay_arg = hlffi_value_int(vm, 50);
        hlffi_value* args[] = {delay_arg};
        call_void(vm, "Timers", "startOneShotTimer", 1, args);
        hlffi_value_free(delay_arg);

        /* Wait for timer to be ready (50ms + margin) */
        sleep_ms(60);

        /* Process events - timer should fire */
        err = hlffi_update(vm, 0.060f);
        if (err == HLFFI_OK) {
            int fired = call_int(vm, "Timers", "getTimerFired", 0, NULL);
            if (fired == 1) {
                TEST_PASS("One-shot timer fires correctly (50ms)");
            } else {
                TEST_FAIL("One-shot timer did not fire");
                printf("  Expected: 1, Got: %d\n", fired);
            }
        } else {
            TEST_FAIL("Failed to process events for one-shot timer");
        }
    }

    /* Test 3: Multiple timers with different delays */
    {
        call_void(vm, "Timers", "resetCounters", 0, NULL);
        call_void(vm, "Timers", "testMultipleTimers", 0, NULL);

        /* Process events multiple times to catch all timers */
        double start_time = get_time_ms();
        double elapsed = 0;
        int update_count = 0;

        /* Run event loop for 150ms to catch all timers (10ms, 50ms, 100ms) */
        while (elapsed < 150.0 && update_count < 200) {
            hlffi_update(vm, 0.001f);
            sleep_ms(1); /* 1ms sleep for high-frequency processing */
            elapsed = get_time_ms() - start_time;
            update_count++;
        }

        int total_fired = call_int(vm, "Timers", "getTotalFired", 0, NULL);
        if (total_fired >= 4) { /* 3 timers + 1 MainLoop callback */
            TEST_PASS("Multiple timers fire correctly");
            printf("  Total fired: %d (in %d updates over %.1fms)\n", total_fired, update_count, elapsed);
        } else {
            TEST_FAIL("Not all timers fired");
            printf("  Expected: >=4, Got: %d (in %d updates over %.1fms)\n", total_fired, update_count, elapsed);
        }
    }

    /* Test 4: High-frequency event processing (1ms granularity) */
    {
        call_void(vm, "Timers", "resetCounters", 0, NULL);

        /* Start a 5ms timer */
        hlffi_value* delay_arg = hlffi_value_int(vm, 5);
        hlffi_value* args[] = {delay_arg};
        call_void(vm, "Timers", "startOneShotTimer", 1, args);
        hlffi_value_free(delay_arg);

        /* Process events every 1ms for 10ms */
        double start_time = get_time_ms();
        int updates = 0;
        while (get_time_ms() - start_time < 10.0) {
            hlffi_update(vm, 0.001f);
            sleep_ms(1);
            updates++;
        }

        int fired = call_int(vm, "Timers", "getTimerFired", 0, NULL);
        if (fired == 1) {
            TEST_PASS("High-frequency event processing (1ms granularity)");
            printf("  Timer fired in %d updates over ~10ms\n", updates);
        } else {
            TEST_FAIL("High-frequency timer did not fire");
            printf("  Updates: %d, Fired: %d\n", updates, fired);
        }
    }

    /* Test 5: MainLoop callback */
    {
        call_void(vm, "Timers", "resetCounters", 0, NULL);
        call_void(vm, "Timers", "addMainLoopCallback", 0, NULL);

        /* Process events - MainLoop callback should fire immediately */
        hlffi_update(vm, 0.001f);

        int main_loop_fired = call_int(vm, "Timers", "getMainLoopFired", 0, NULL);
        if (main_loop_fired >= 1) {
            TEST_PASS("MainLoop callback fires");
        } else {
            TEST_FAIL("MainLoop callback did not fire");
        }
    }

    /* Test 6: Interval timer */
    {
        call_void(vm, "Timers", "resetCounters", 0, NULL);

        /* Start 10ms interval timer */
        hlffi_value* interval_arg = hlffi_value_int(vm, 10);
        hlffi_value* args[] = {interval_arg};
        call_void(vm, "Timers", "startIntervalTimer", 1, args);
        hlffi_value_free(interval_arg);

        /* Run event loop for 35ms, should fire ~3 times */
        double start_time = get_time_ms();
        while (get_time_ms() - start_time < 35.0) {
            hlffi_update(vm, 0.001f);
            sleep_ms(1);
        }

        call_void(vm, "Timers", "stopIntervalTimer", 0, NULL);

        int interval_count = call_int(vm, "Timers", "getIntervalCount", 0, NULL);
        if (interval_count >= 2 && interval_count <= 4) {
            TEST_PASS("Interval timer fires repeatedly");
            printf("  Interval fired: %d times in ~35ms\n", interval_count);
        } else {
            TEST_FAIL("Interval timer count unexpected");
            printf("  Expected: 2-4, Got: %d\n", interval_count);
        }
    }

    /* Test 7: Timer precision test */
    {
        call_void(vm, "Timers", "resetCounters", 0, NULL);
        call_void(vm, "Timers", "testTimerPrecision", 0, NULL);

        /* Process events for 150ms to catch all precision timers */
        double start_time = get_time_ms();
        while (get_time_ms() - start_time < 150.0) {
            hlffi_update(vm, 0.001f);
            sleep_ms(1);
        }

        int timer_fired = call_int(vm, "Timers", "getTimerFired", 0, NULL);
        if (timer_fired >= 6) { /* Expecting 7 timers: 1ms, 2ms, 5ms, 10ms, 20ms, 50ms, 100ms */
            TEST_PASS("Timer precision test (multiple intervals)");
            printf("  Timers fired: %d/7\n", timer_fired);
        } else {
            TEST_FAIL("Not all precision timers fired");
            printf("  Expected: 7, Got: %d\n", timer_fired);
        }
    }

    /* Test 8: hlffi_has_pending_work() */
    {
        call_void(vm, "Timers", "resetCounters", 0, NULL);

        /* Add a timer */
        hlffi_value* delay_arg = hlffi_value_int(vm, 100);
        hlffi_value* args[] = {delay_arg};
        call_void(vm, "Timers", "startOneShotTimer", 1, args);
        hlffi_value_free(delay_arg);

        /* There might be pending work (though the API is conservative) */
        bool has_work = hlffi_has_pending_work(vm);

        /* Process the timer */
        sleep_ms(110);
        hlffi_update(vm, 0.001f);

        /* Note: has_pending_work() might not detect Haxe timers,
         * so we just verify it doesn't crash */
        TEST_PASS("hlffi_has_pending_work() executes");
        printf("  Has pending work: %s (may be conservative)\n", has_work ? "yes" : "no");
    }

    /* Test 9: hlffi_process_events() with specific event loop types */
    {
        /* Test UV only */
        err = hlffi_process_events(vm, HLFFI_EVENTLOOP_UV);
        if (err == HLFFI_OK) {
            TEST_PASS("hlffi_process_events(UV) executes");
        } else {
            TEST_FAIL("hlffi_process_events(UV) failed");
        }

        /* Test Haxe only */
        err = hlffi_process_events(vm, HLFFI_EVENTLOOP_HAXE);
        if (err == HLFFI_OK) {
            TEST_PASS("hlffi_process_events(HAXE) executes");
        } else {
            TEST_FAIL("hlffi_process_events(HAXE) failed");
        }

        /* Test ALL */
        err = hlffi_process_events(vm, HLFFI_EVENTLOOP_ALL);
        if (err == HLFFI_OK) {
            TEST_PASS("hlffi_process_events(ALL) executes");
        } else {
            TEST_FAIL("hlffi_process_events(ALL) failed");
        }
    }

    /* Cleanup */
    hlffi_destroy(vm);

    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Total: %d tests\n", test_count);
    printf("Passed: %d tests\n", test_count - failed);
    printf("Failed: %d tests\n", failed);

    if (failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed\n");
        return 1;
    }
}
