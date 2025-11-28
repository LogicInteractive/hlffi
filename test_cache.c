/**
 * Phase 7: Caching API Tests
 *
 * Tests the performance caching API for static method calls.
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PASS(msg) printf("  ✓ %s\n", msg)
#define TEST_FAIL(msg) do { printf("  ✗ %s\n", msg); failures++; } while(0)

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cachetest.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Phase 7 Test: Caching API ===\n\n");

    int failures = 0;

    /* Initialize VM */
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

    if (hlffi_load_file(vm, argv[1]) != HLFFI_OK) {
        fprintf(stderr, "Failed to load bytecode: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry point: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Test 1: Cache simple method with no args */
    printf("Test 1: Cache static method with no args\n");
    {
        hlffi_cached_call* cached = hlffi_cache_static_method(vm, "CacheTest", "increment");
        if (cached) {
            TEST_PASS("Successfully cached CacheTest.increment");

            /* Call it a few times */
            for (int i = 0; i < 5; i++) {
                hlffi_value* result = hlffi_call_cached(cached, 0, NULL);
                if (result) {
                    hlffi_value_free(result);
                } else {
                    TEST_FAIL("Cached call returned NULL");
                }
            }

            /* Verify counter was incremented */
            hlffi_value* counter_val = hlffi_get_static_field(vm, "CacheTest", "counter");
            int counter = hlffi_value_as_int(counter_val, -1);
            hlffi_value_free(counter_val);
            if (counter == 5) {
                TEST_PASS("Counter incremented correctly (5 calls)");
            } else {
                TEST_FAIL("Counter not incremented correctly");
                printf("    Expected: 5, Got: %d\n", counter);
            }

            hlffi_cached_call_free(cached);
        } else {
            TEST_FAIL("Failed to cache method");
            printf("    Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 2: Cache method with args and return value */
    printf("\nTest 2: Cache method with args and return value\n");
    {
        hlffi_cached_call* cached = hlffi_cache_static_method(vm, "CacheTest", "add");
        if (cached) {
            TEST_PASS("Successfully cached CacheTest.add");

            /* Call with different arguments */
            hlffi_value* args[2];
            args[0] = hlffi_value_int(vm, 10);
            args[1] = hlffi_value_int(vm, 20);

            hlffi_value* result = hlffi_call_cached(cached, 2, args);
            if (result) {
                int sum = hlffi_value_as_int(result, -1);
                if (sum == 30) {
                    TEST_PASS("Cached call returned correct result (10 + 20 = 30)");
                } else {
                    TEST_FAIL("Incorrect result");
                    printf("    Expected: 30, Got: %d\n", sum);
                }
                hlffi_value_free(result);
            } else {
                TEST_FAIL("Cached call returned NULL");
            }

            hlffi_value_free(args[0]);
            hlffi_value_free(args[1]);

            hlffi_cached_call_free(cached);
        } else {
            TEST_FAIL("Failed to cache method");
            printf("    Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 3: Cache method returning string */
    printf("\nTest 3: Cache method returning string\n");
    {
        hlffi_cached_call* cached = hlffi_cache_static_method(vm, "CacheTest", "greet");
        if (cached) {
            TEST_PASS("Successfully cached CacheTest.greet");

            hlffi_value* args[1];
            args[0] = hlffi_value_string(vm, "World");

            hlffi_value* result = hlffi_call_cached(cached, 1, args);
            if (result) {
                char* greeting = hlffi_value_as_string(result);
                if (greeting && strcmp(greeting, "Hello, World!") == 0) {
                    TEST_PASS("Cached call returned correct string");
                } else {
                    TEST_FAIL("Incorrect string result");
                    printf("    Expected: 'Hello, World!', Got: '%s'\n", greeting ? greeting : "(null)");
                }
                free(greeting);
                hlffi_value_free(result);
            } else {
                TEST_FAIL("Cached call returned NULL");
            }

            hlffi_value_free(args[0]);
            hlffi_cached_call_free(cached);
        } else {
            TEST_FAIL("Failed to cache method");
            printf("    Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 4: Cache method with float args */
    printf("\nTest 4: Cache method with float args\n");
    {
        hlffi_cached_call* cached = hlffi_cache_static_method(vm, "CacheTest", "multiply");
        if (cached) {
            TEST_PASS("Successfully cached CacheTest.multiply");

            hlffi_value* args[2];
            args[0] = hlffi_value_float(vm, 3.5);
            args[1] = hlffi_value_float(vm, 2.0);

            hlffi_value* result = hlffi_call_cached(cached, 2, args);
            if (result) {
                double product = hlffi_value_as_float(result, -1.0);
                if (product == 7.0) {
                    TEST_PASS("Cached call returned correct float result (3.5 * 2.0 = 7.0)");
                } else {
                    TEST_FAIL("Incorrect float result");
                    printf("    Expected: 7.0, Got: %f\n", product);
                }
                hlffi_value_free(result);
            } else {
                TEST_FAIL("Cached call returned NULL");
            }

            hlffi_value_free(args[0]);
            hlffi_value_free(args[1]);
            hlffi_cached_call_free(cached);
        } else {
            TEST_FAIL("Failed to cache method");
            printf("    Error: %s\n", hlffi_get_error(vm));
        }
    }

    /* Test 5: Error handling - invalid class */
    printf("\nTest 5: Error handling - invalid class\n");
    {
        hlffi_cached_call* cached = hlffi_cache_static_method(vm, "NonExistent", "method");
        if (!cached) {
            TEST_PASS("Correctly rejected invalid class");
        } else {
            TEST_FAIL("Should have rejected invalid class");
            hlffi_cached_call_free(cached);
        }
    }

    /* Test 6: Error handling - invalid method */
    printf("\nTest 6: Error handling - invalid method\n");
    {
        hlffi_cached_call* cached = hlffi_cache_static_method(vm, "CacheTest", "nonExistent");
        if (!cached) {
            TEST_PASS("Correctly rejected invalid method");
        } else {
            TEST_FAIL("Should have rejected invalid method");
            hlffi_cached_call_free(cached);
        }
    }

    /* Test 7: Multiple cached calls */
    printf("\nTest 7: Multiple cached calls\n");
    {
        hlffi_cached_call* add_cache = hlffi_cache_static_method(vm, "CacheTest", "add");
        hlffi_cached_call* mult_cache = hlffi_cache_static_method(vm, "CacheTest", "multiply");

        if (add_cache && mult_cache) {
            TEST_PASS("Successfully cached multiple methods");

            /* Use them interleaved */
            hlffi_value* args1[2];
            args1[0] = hlffi_value_int(vm, 5);
            args1[1] = hlffi_value_int(vm, 3);
            hlffi_value* result1 = hlffi_call_cached(add_cache, 2, args1);

            hlffi_value* args2[2];
            args2[0] = hlffi_value_float(vm, 5.0);
            args2[1] = hlffi_value_float(vm, 3.0);
            hlffi_value* result2 = hlffi_call_cached(mult_cache, 2, args2);

            int sum = hlffi_value_as_int(result1, -1);
            double product = hlffi_value_as_float(result2, -1.0);

            if (sum == 8 && product == 15.0) {
                TEST_PASS("Both cached methods work correctly");
            } else {
                TEST_FAIL("Cached methods returned incorrect results");
            }

            hlffi_value_free(args1[0]);
            hlffi_value_free(args1[1]);
            hlffi_value_free(result1);
            hlffi_value_free(args2[0]);
            hlffi_value_free(args2[1]);
            hlffi_value_free(result2);

            hlffi_cached_call_free(add_cache);
            hlffi_cached_call_free(mult_cache);
        } else {
            TEST_FAIL("Failed to cache multiple methods");
        }
    }

    /* Cleanup */
    hlffi_destroy(vm);

    printf("\n=== Test Summary ===\n");
    if (failures == 0) {
        printf("✓ All tests passed!\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", failures);
        return 1;
    }
}
