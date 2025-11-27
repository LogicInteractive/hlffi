/**
 * Phase 5 Test: Array Operations
 * Tests hlffi_array_* functions
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hl.h>

#define TEST_PASS(name) printf("[PASS] Test %d: %s\n", ++test_count, name)
#define TEST_FAIL(name) do { printf("[FAIL] Test %d: %s\n", ++test_count, name); failed++; } while(0)

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <arrays.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Phase 5 Test: Array Operations ===\n");

    /* Initialize VM */
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    if (hlffi_init(vm, argc, argv) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM\n");
        hlffi_destroy(vm);
        return 1;
    }

    /* Load bytecode */
    if (hlffi_load_file(vm, argv[1]) != HLFFI_OK) {
        fprintf(stderr, "Failed to load %s: %s\n", argv[1], hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Call entry point */
    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry point: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    int test_count = 0;
    int failed = 0;

    /* Test 1: Create empty array */
    {
        hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 0);
        if (arr && hlffi_array_length(arr) == 0) {
            TEST_PASS("Create empty int array");
        } else {
            TEST_FAIL("Create empty int array");
        }
        hlffi_value_free(arr);
    }

    /* Test 2: Create int array with length */
    {
        hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 5);
        if (arr && hlffi_array_length(arr) == 5) {
            TEST_PASS("Create int array with length 5");
        } else {
            TEST_FAIL("Create int array with length 5");
        }
        hlffi_value_free(arr);
    }

    /* Test 3: Set and get int array elements */
    {
        hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 3);

        hlffi_value* v0 = hlffi_value_int(vm, 10);
        hlffi_value* v1 = hlffi_value_int(vm, 20);
        hlffi_value* v2 = hlffi_value_int(vm, 30);

        hlffi_array_set(vm, arr, 0, v0);
        hlffi_array_set(vm, arr, 1, v1);
        hlffi_array_set(vm, arr, 2, v2);

        hlffi_value_free(v0);
        hlffi_value_free(v1);
        hlffi_value_free(v2);

        hlffi_value* elem0 = hlffi_array_get(vm, arr, 0);
        hlffi_value* elem1 = hlffi_array_get(vm, arr, 1);
        hlffi_value* elem2 = hlffi_array_get(vm, arr, 2);

        int val0 = hlffi_value_as_int(elem0, -1);
        int val1 = hlffi_value_as_int(elem1, -1);
        int val2 = hlffi_value_as_int(elem2, -1);

        if (val0 == 10 && val1 == 20 && val2 == 30) {
            TEST_PASS("Set and get int array elements");
        } else {
            TEST_FAIL("Set and get int array elements");
            printf("  Expected: 10, 20, 30\n");
            printf("  Got: %d, %d, %d\n", val0, val1, val2);
        }

        hlffi_value_free(elem0);
        hlffi_value_free(elem1);
        hlffi_value_free(elem2);
        hlffi_value_free(arr);
    }

    /* Test 4: Float array */
    {
        hlffi_value* arr = hlffi_array_new(vm, &hlt_f64, 2);

        hlffi_value* v0 = hlffi_value_float(vm, 1.5);
        hlffi_value* v1 = hlffi_value_float(vm, 2.5);

        hlffi_array_set(vm, arr, 0, v0);
        hlffi_array_set(vm, arr, 1, v1);

        hlffi_value_free(v0);
        hlffi_value_free(v1);

        hlffi_value* elem0 = hlffi_array_get(vm, arr, 0);
        hlffi_value* elem1 = hlffi_array_get(vm, arr, 1);

        double val0 = hlffi_value_as_float(elem0, -1.0);
        double val1 = hlffi_value_as_float(elem1, -1.0);

        if (val0 == 1.5 && val1 == 2.5) {
            TEST_PASS("Create and access float array");
        } else {
            TEST_FAIL("Create and access float array");
            printf("  Expected: 1.5, 2.5\n");
            printf("  Got: %.1f, %.1f\n", val0, val1);
        }

        hlffi_value_free(elem0);
        hlffi_value_free(elem1);
        hlffi_value_free(arr);
    }

    /* Test 5: String array */
    {
        hlffi_value* arr = hlffi_array_new(vm, &hlt_bytes, 2);

        hlffi_value* v0 = hlffi_value_string(vm, "hello");
        hlffi_value* v1 = hlffi_value_string(vm, "world");

        hlffi_array_set(vm, arr, 0, v0);
        hlffi_array_set(vm, arr, 1, v1);

        hlffi_value_free(v0);
        hlffi_value_free(v1);

        hlffi_value* elem0 = hlffi_array_get(vm, arr, 0);
        hlffi_value* elem1 = hlffi_array_get(vm, arr, 1);

        char* str0 = hlffi_value_as_string(elem0);
        char* str1 = hlffi_value_as_string(elem1);

        if (str0 && str1 && strcmp(str0, "hello") == 0 && strcmp(str1, "world") == 0) {
            TEST_PASS("Create and access string array");
        } else {
            TEST_FAIL("Create and access string array");
            printf("  Expected: hello, world\n");
            printf("  Got: %s, %s\n", str0 ? str0 : "(null)", str1 ? str1 : "(null)");
        }

        free(str0);
        free(str1);
        hlffi_value_free(elem0);
        hlffi_value_free(elem1);
        hlffi_value_free(arr);
    }

    /* Test 6: Dynamic array */
    {
        hlffi_value* arr = hlffi_array_new(vm, NULL, 3);  // NULL = dynamic array

        hlffi_value* v0 = hlffi_value_int(vm, 42);
        hlffi_value* v1 = hlffi_value_string(vm, "test");
        hlffi_value* v2 = hlffi_value_float(vm, 3.14);

        hlffi_array_set(vm, arr, 0, v0);
        hlffi_array_set(vm, arr, 1, v1);
        hlffi_array_set(vm, arr, 2, v2);

        hlffi_value_free(v0);
        hlffi_value_free(v1);
        hlffi_value_free(v2);

        hlffi_value* elem0 = hlffi_array_get(vm, arr, 0);
        hlffi_value* elem1 = hlffi_array_get(vm, arr, 1);
        hlffi_value* elem2 = hlffi_array_get(vm, arr, 2);

        int int_val = hlffi_value_as_int(elem0, -1);
        char* str_val = hlffi_value_as_string(elem1);
        double float_val = hlffi_value_as_float(elem2, -1.0);

        if (int_val == 42 && str_val && strcmp(str_val, "test") == 0 && float_val == 3.14) {
            TEST_PASS("Create and access dynamic array");
        } else {
            TEST_FAIL("Create and access dynamic array");
        }

        free(str_val);
        hlffi_value_free(elem0);
        hlffi_value_free(elem1);
        hlffi_value_free(elem2);
        hlffi_value_free(arr);
    }

    /* Test 7: Array push */
    {
        hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 2);

        hlffi_value* v0 = hlffi_value_int(vm, 1);
        hlffi_value* v1 = hlffi_value_int(vm, 2);
        hlffi_array_set(vm, arr, 0, v0);
        hlffi_array_set(vm, arr, 1, v1);
        hlffi_value_free(v0);
        hlffi_value_free(v1);

        int len_before = hlffi_array_length(arr);

        hlffi_value* v2 = hlffi_value_int(vm, 3);
        hlffi_array_push(vm, arr, v2);
        hlffi_value_free(v2);

        int len_after = hlffi_array_length(arr);

        hlffi_value* last = hlffi_array_get(vm, arr, 2);
        int last_val = hlffi_value_as_int(last, -1);

        if (len_before == 2 && len_after == 3 && last_val == 3) {
            TEST_PASS("Array push operation");
        } else {
            TEST_FAIL("Array push operation");
            printf("  Length before: %d, after: %d\n", len_before, len_after);
            printf("  Last value: %d\n", last_val);
        }

        hlffi_value_free(last);
        hlffi_value_free(arr);
    }

    /* Test 8: Bounds checking */
    {
        hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 3);

        // Try to access out of bounds
        hlffi_value* elem = hlffi_array_get(vm, arr, 10);

        if (!elem) {
            TEST_PASS("Array bounds checking (get)");
        } else {
            TEST_FAIL("Array bounds checking (get) - should have returned NULL");
            hlffi_value_free(elem);
        }

        hlffi_value_free(arr);
    }

    /* Test 9: Get array from Haxe */
    {
        hlffi_value* arr = hlffi_call_static(vm, "Arrays", "getIntArray", 0, NULL);
        printf("DEBUG: arr = %p\n", (void*)arr);
        fflush(stdout);

        if (arr) {
            int len = hlffi_array_length(arr);
            if (len == 5) {
                hlffi_value* first = hlffi_array_get(vm, arr, 0);
                hlffi_value* last = hlffi_array_get(vm, arr, 4);

                int first_val = hlffi_value_as_int(first, -1);
                int last_val = hlffi_value_as_int(last, -1);

                if (first_val == 10 && last_val == 50) {
                    TEST_PASS("Get int array from Haxe");
                } else {
                    TEST_FAIL("Get int array from Haxe - wrong values");
                    printf("  Expected first=10, last=50\n");
                    printf("  Got first=%d, last=%d\n", first_val, last_val);
                }

                hlffi_value_free(first);
                hlffi_value_free(last);
            } else {
                TEST_FAIL("Get int array from Haxe - wrong length");
                printf("  Expected length: 5, got: %d\n", len);
            }
            hlffi_value_free(arr);
        } else {
            TEST_FAIL("Get int array from Haxe - returned NULL");
        }
    }

    /* Test 10: Pass array to Haxe */
    {
        hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 4);

        hlffi_value* v0 = hlffi_value_int(vm, 5);
        hlffi_value* v1 = hlffi_value_int(vm, 10);
        hlffi_value* v2 = hlffi_value_int(vm, 15);
        hlffi_value* v3 = hlffi_value_int(vm, 20);

        hlffi_array_set(vm, arr, 0, v0);
        hlffi_array_set(vm, arr, 1, v1);
        hlffi_array_set(vm, arr, 2, v2);
        hlffi_array_set(vm, arr, 3, v3);

        hlffi_value_free(v0);
        hlffi_value_free(v1);
        hlffi_value_free(v2);
        hlffi_value_free(v3);

        hlffi_value* args[] = {arr};
        hlffi_value* result = hlffi_call_static(vm, "Arrays", "sumIntArray", 1, args);

        int sum = hlffi_value_as_int(result, -1);

        if (sum == 50) {  // 5 + 10 + 15 + 20 = 50
            TEST_PASS("Pass int array to Haxe and get result");
        } else {
            TEST_FAIL("Pass int array to Haxe and get result");
            printf("  Expected sum: 50, got: %d\n", sum);
        }

        hlffi_value_free(result);
        hlffi_value_free(arr);
    }

    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Total: %d tests\n", test_count);
    printf("Passed: %d tests\n", test_count - failed);
    printf("Failed: %d tests\n", failed);

    if (failed == 0) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed.\n");
    }

    /* Cleanup */
    hlffi_destroy(vm);

    return (failed > 0) ? 1 : 0;
}
