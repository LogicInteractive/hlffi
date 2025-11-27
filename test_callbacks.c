/**
 * Phase 6 Test: C→Haxe Callbacks
 * Tests hlffi_register_callback(), hlffi_get_callback()
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PASS(name) printf("[PASS] Test %d: %s\n", ++test_count, name)
#define TEST_FAIL(name) do { printf("[FAIL] Test %d: %s\n", ++test_count, name); failed++; } while(0)

/* Test callback: onMessage(String) -> Void */
static hlffi_value* callback_on_message(hlffi_vm* vm, int argc, hlffi_value** argv) {
    if (argc != 1) {
        printf("  ERROR: Expected 1 arg, got %d\n", argc);
        return hlffi_value_null(vm);
    }

    const char* msg = hlffi_value_as_string(argv[0]);
    printf("  C callback received message: '%s'\n", msg);

    /* Store in Haxe via static method */
    hlffi_value* msg_val = hlffi_value_string(vm, msg);
    hlffi_value* args[] = {msg_val};
    hlffi_call_static(vm, "Callbacks", "storeMessage", 1, args);
    hlffi_value_free(msg_val);

    return hlffi_value_null(vm);
}

/* Test callback: onAdd(Int, Int) -> Int */
static hlffi_value* callback_on_add(hlffi_vm* vm, int argc, hlffi_value** argv) {
    if (argc != 2) {
        printf("  ERROR: Expected 2 args, got %d\n", argc);
        return hlffi_value_int(vm, -1);
    }

    int a = hlffi_value_as_int(argv[0], 0);
    int b = hlffi_value_as_int(argv[1], 0);
    int result = a + b;

    printf("  C callback adding: %d + %d = %d\n", a, b, result);

    /* Store result in Haxe */
    hlffi_value* result_val = hlffi_value_int(vm, result);
    hlffi_value* args[] = {result_val};
    hlffi_call_static(vm, "Callbacks", "storeAddResult", 1, args);
    hlffi_value_free(result_val);

    return hlffi_value_int(vm, result);
}

/* Test callback: onNotify() -> Void */
static hlffi_value* callback_on_notify(hlffi_vm* vm, int argc, hlffi_value** argv) {
    (void)argc; (void)argv;
    printf("  C callback notify called\n");

    /* Increment counter in Haxe */
    hlffi_call_static(vm, "Callbacks", "incrementNotifyCount", 0, NULL);

    return hlffi_value_null(vm);
}

/* Test callback: onCompute(Int, Int, Int) -> Int */
static hlffi_value* callback_on_compute(hlffi_vm* vm, int argc, hlffi_value** argv) {
    if (argc != 3) {
        printf("  ERROR: Expected 3 args, got %d\n", argc);
        return hlffi_value_int(vm, -1);
    }

    int a = hlffi_value_as_int(argv[0], 0);
    int b = hlffi_value_as_int(argv[1], 0);
    int c = hlffi_value_as_int(argv[2], 0);
    int result = a * b + c;

    printf("  C callback computing: %d * %d + %d = %d\n", a, b, c, result);

    /* Store result in Haxe */
    hlffi_value* result_val = hlffi_value_int(vm, result);
    hlffi_value* args[] = {result_val};
    hlffi_call_static(vm, "Callbacks", "storeComputeResult", 1, args);
    hlffi_value_free(result_val);

    return hlffi_value_int(vm, result);
}

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

/* Helper: Call string static method */
static const char* call_string(hlffi_vm* vm, const char* class_name, const char* method_name) {
    hlffi_value* result = hlffi_call_static(vm, class_name, method_name, 0, NULL);
    if (!result) return NULL;
    const char* value = hlffi_value_as_string(result);
    hlffi_value_free(result);
    return value;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <callbacks.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Phase 6 Test: C→Haxe Callbacks ===\n");
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

    /* Load callbacks test bytecode */
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

    /* Test 1: Register callback with 0 args */
    {
        bool ok = hlffi_register_callback(vm, "onNotify", callback_on_notify, 0);
        if (ok) {
            TEST_PASS("Register 0-arg callback");
        } else {
            TEST_FAIL("Register 0-arg callback");
            printf("  Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 2: Register callback with 1 arg */
    {
        bool ok = hlffi_register_callback(vm, "onMessage", callback_on_message, 1);
        if (ok) {
            TEST_PASS("Register 1-arg callback");
        } else {
            TEST_FAIL("Register 1-arg callback");
            printf("  Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 3: Register callback with 2 args */
    {
        bool ok = hlffi_register_callback(vm, "onAdd", callback_on_add, 2);
        if (ok) {
            TEST_PASS("Register 2-arg callback");
        } else {
            TEST_FAIL("Register 2-arg callback");
            printf("  Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 4: Register callback with 3 args */
    {
        bool ok = hlffi_register_callback(vm, "onCompute", callback_on_compute, 3);
        if (ok) {
            TEST_PASS("Register 3-arg callback");
        } else {
            TEST_FAIL("Register 3-arg callback");
            printf("  Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 5: Get callback and set in Haxe */
    {
        hlffi_value* cb = hlffi_get_callback(vm, "onMessage");
        if (cb) {
            TEST_PASS("Get registered callback");

            /* Set callback in Haxe static field */
            hlffi_error_code err = hlffi_set_static_field(vm, "Callbacks", "onMessage", cb);
            if (err != HLFFI_OK) {
                printf("  Warning: hlffi_set_static_field failed: %s\n", hlffi_get_error(vm));
            } else {
                printf("  Set static field successfully\n");
            }
            hlffi_value_free(cb);
        } else {
            TEST_FAIL("Get registered callback");
            printf("  Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 6: Call Haxe method that invokes C callback (1 arg) */
    {
        call_void(vm, "Callbacks", "reset", 0, NULL);

        hlffi_value* msg_arg = hlffi_value_string(vm, "Hello from C");
        hlffi_value* args[] = {msg_arg};
        call_void(vm, "Callbacks", "callMessageCallback", 1, args);
        hlffi_value_free(msg_arg);

        /* Check if callback was invoked */
        const char* stored_msg = call_string(vm, "Callbacks", "getStoredMessage");
        if (stored_msg && strcmp(stored_msg, "Hello from C") == 0) {
            TEST_PASS("Invoke 1-arg callback from Haxe");
        } else {
            TEST_FAIL("Invoke 1-arg callback from Haxe");
            printf("  Expected: 'Hello from C', Got: '%s'\n", stored_msg ? stored_msg : "(null)");
        }
    }

    /* Test 7: Test 2-arg callback */
    {
        call_void(vm, "Callbacks", "reset", 0, NULL);

        /* Set callback */
        hlffi_value* cb = hlffi_get_callback(vm, "onAdd");
        hlffi_set_static_field(vm, "Callbacks", "onAdd", cb);
        hlffi_value_free(cb);

        /* Call from Haxe */
        hlffi_value* arg_a = hlffi_value_int(vm, 10);
        hlffi_value* arg_b = hlffi_value_int(vm, 20);
        hlffi_value* args[] = {arg_a, arg_b};
        int result = call_int(vm, "Callbacks", "callAddCallback", 2, args);
        hlffi_value_free(arg_a);
        hlffi_value_free(arg_b);

        int stored = call_int(vm, "Callbacks", "getAddResult", 0, NULL);
        if (result == 30 && stored == 30) {
            TEST_PASS("Invoke 2-arg callback from Haxe");
        } else {
            TEST_FAIL("Invoke 2-arg callback from Haxe");
            printf("  Expected: 30, Got result=%d, stored=%d\n", result, stored);
        }
    }

    /* Test 8: Test 0-arg callback */
    {
        call_void(vm, "Callbacks", "reset", 0, NULL);

        /* Set callback */
        hlffi_value* cb = hlffi_get_callback(vm, "onNotify");
        hlffi_set_static_field(vm, "Callbacks", "onNotify", cb);
        hlffi_value_free(cb);

        /* Call from Haxe multiple times */
        call_void(vm, "Callbacks", "callNotifyCallback", 0, NULL);
        call_void(vm, "Callbacks", "callNotifyCallback", 0, NULL);
        call_void(vm, "Callbacks", "callNotifyCallback", 0, NULL);

        int count = call_int(vm, "Callbacks", "getNotifyCount", 0, NULL);
        if (count == 3) {
            TEST_PASS("Invoke 0-arg callback multiple times");
        } else {
            TEST_FAIL("Invoke 0-arg callback multiple times");
            printf("  Expected: 3, Got: %d\n", count);
        }
    }

    /* Test 9: Test 3-arg callback */
    {
        call_void(vm, "Callbacks", "reset", 0, NULL);

        /* Set callback */
        hlffi_value* cb = hlffi_get_callback(vm, "onCompute");
        hlffi_set_static_field(vm, "Callbacks", "onCompute", cb);
        hlffi_value_free(cb);

        /* Call from Haxe: compute(5, 6, 7) = 5*6 + 7 = 37 */
        hlffi_value* arg_a = hlffi_value_int(vm, 5);
        hlffi_value* arg_b = hlffi_value_int(vm, 6);
        hlffi_value* arg_c = hlffi_value_int(vm, 7);
        hlffi_value* args[] = {arg_a, arg_b, arg_c};
        int result = call_int(vm, "Callbacks", "callComputeCallback", 3, args);
        hlffi_value_free(arg_a);
        hlffi_value_free(arg_b);
        hlffi_value_free(arg_c);

        int stored = call_int(vm, "Callbacks", "getComputeResult", 0, NULL);
        if (result == 37 && stored == 37) {
            TEST_PASS("Invoke 3-arg callback from Haxe");
        } else {
            TEST_FAIL("Invoke 3-arg callback from Haxe");
            printf("  Expected: 37, Got result=%d, stored=%d\n", result, stored);
        }
    }

    /* Test 10: Reject invalid arity */
    {
        bool ok = hlffi_register_callback(vm, "invalid", callback_on_notify, 10);
        if (!ok) {
            TEST_PASS("Reject invalid callback arity (>4)");
        } else {
            TEST_FAIL("Reject invalid callback arity (>4)");
        }
    }

    /* Test 11: Reject duplicate name */
    {
        bool ok = hlffi_register_callback(vm, "onNotify", callback_on_notify, 0);
        if (!ok) {
            TEST_PASS("Reject duplicate callback name");
        } else {
            TEST_FAIL("Reject duplicate callback name");
        }
    }

    /* Test 12: Get non-existent callback returns NULL */
    {
        hlffi_value* cb = hlffi_get_callback(vm, "doesNotExist");
        if (cb == NULL) {
            TEST_PASS("Get non-existent callback returns NULL");
        } else {
            TEST_FAIL("Get non-existent callback returns NULL");
            hlffi_value_free(cb);
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
