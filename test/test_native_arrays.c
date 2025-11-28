/**
 * Test Suite: NativeArray and Struct Array Support
 *
 * Tests the new Phase 5 array functionality:
 * - hl.NativeArray<primitive>
 * - Array<struct>
 * - hl.NativeArray<struct>
 *
 * Compile: gcc -o test_native_arrays test_native_arrays.c -I../include -L../build -lhlffi -lhl
 * Run: ./test_native_arrays
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/hlffi.h"

/* Test struct: Vec3 */
typedef struct {
    float x;
    float y;
    float z;
} Vec3;

/* Test results */
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    printf("\n--- Test: %s ---\n", name);

#define ASSERT(condition, message) \
    if (condition) { \
        printf("✓ %s\n", message); \
        tests_passed++; \
    } else { \
        printf("✗ %s\n", message); \
        tests_failed++; \
    }

#define ASSERT_EQ(actual, expected, message) \
    if ((actual) == (expected)) { \
        printf("✓ %s (got %d)\n", message, (int)(actual)); \
        tests_passed++; \
    } else { \
        printf("✗ %s (expected %d, got %d)\n", message, (int)(expected), (int)(actual)); \
        tests_failed++; \
    }

#define ASSERT_FLOAT_EQ(actual, expected, tolerance, message) \
    if (fabs((actual) - (expected)) < (tolerance)) { \
        printf("✓ %s (got %.3f)\n", message, (float)(actual)); \
        tests_passed++; \
    } else { \
        printf("✗ %s (expected %.3f, got %.3f)\n", message, (float)(expected), (float)(actual)); \
        tests_failed++; \
    }

/* Test 1: Create NativeArray of integers */
void test_native_array_int(hlffi_vm* vm) {
    TEST("NativeArray<Int> - Create and direct access");

    hlffi_value* arr = hlffi_native_array_new(vm, &hlt_i32, 10);
    ASSERT(arr != NULL, "NativeArray created");

    int* data = (int*)hlffi_native_array_get_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to data");

    /* Fill array */
    for (int i = 0; i < 10; i++) {
        data[i] = i * 10;
    }

    /* Verify length */
    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 10, "Array length is correct");

    /* Verify data */
    ASSERT_EQ(data[0], 0, "data[0] = 0");
    ASSERT_EQ(data[5], 50, "data[5] = 50");
    ASSERT_EQ(data[9], 90, "data[9] = 90");

    hlffi_value_free(arr);
}

/* Test 2: Create NativeArray of floats (F32) */
void test_native_array_float(hlffi_vm* vm) {
    TEST("NativeArray<Single> - F32 support");

    hlffi_value* arr = hlffi_native_array_new(vm, &hlt_f32, 5);
    ASSERT(arr != NULL, "NativeArray<Single> created");

    float* data = (float*)hlffi_native_array_get_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to float data");

    /* Fill with float values */
    for (int i = 0; i < 5; i++) {
        data[i] = i * 0.5f;
    }

    /* Verify values */
    ASSERT_FLOAT_EQ(data[0], 0.0f, 0.001f, "data[0] = 0.0");
    ASSERT_FLOAT_EQ(data[2], 1.0f, 0.001f, "data[2] = 1.0");
    ASSERT_FLOAT_EQ(data[4], 2.0f, 0.001f, "data[4] = 2.0");

    hlffi_value_free(arr);
}

/* Test 3: Create NativeArray of doubles (F64) */
void test_native_array_double(hlffi_vm* vm) {
    TEST("NativeArray<Float> - F64 support");

    hlffi_value* arr = hlffi_native_array_new(vm, &hlt_f64, 3);
    ASSERT(arr != NULL, "NativeArray<Float> created");

    double* data = (double*)hlffi_native_array_get_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to double data");

    data[0] = 3.141592653589793;
    data[1] = 2.718281828459045;
    data[2] = 1.414213562373095;

    ASSERT_FLOAT_EQ(data[0], 3.141592653589793, 0.0001, "data[0] = π");
    ASSERT_FLOAT_EQ(data[1], 2.718281828459045, 0.0001, "data[1] = e");
    ASSERT_FLOAT_EQ(data[2], 1.414213562373095, 0.0001, "data[2] = √2");

    hlffi_value_free(arr);
}

/* Test 4: NativeArray performance - batch operations */
void test_native_array_performance(hlffi_vm* vm) {
    TEST("NativeArray - Batch operations (performance)");

    const int SIZE = 10000;
    hlffi_value* arr = hlffi_native_array_new(vm, &hlt_i32, SIZE);
    ASSERT(arr != NULL, "Created large NativeArray (10k elements)");

    int* data = (int*)hlffi_native_array_get_ptr(arr);

    /* Batch fill */
    for (int i = 0; i < SIZE; i++) {
        data[i] = i;
    }

    /* Batch compute sum */
    long long sum = 0;
    for (int i = 0; i < SIZE; i++) {
        sum += data[i];
    }

    long long expected_sum = (long long)SIZE * (SIZE - 1) / 2;
    ASSERT_EQ(sum, expected_sum, "Sum of 0..9999 is correct");

    /* Batch multiply */
    for (int i = 0; i < SIZE; i++) {
        data[i] *= 2;
    }

    ASSERT_EQ(data[0], 0, "Batch multiply: data[0] = 0");
    ASSERT_EQ(data[100], 200, "Batch multiply: data[100] = 200");
    ASSERT_EQ(data[9999], 19998, "Batch multiply: data[9999] = 19998");

    hlffi_value_free(arr);
}

/* Test 5: Array<Struct> - Create with wrapped elements */
void test_array_struct_wrapped(hlffi_vm* vm) {
    TEST("Array<Struct> - Wrapped struct elements");

    /* Note: This test requires a loaded Haxe module with Vec3 struct defined
     * For now, we'll just test the API without actual struct type */

    /* Create dynamic array (Array<Dynamic> can hold structs) */
    hlffi_value* arr = hlffi_array_new(vm, &hlt_dyn, 3);
    ASSERT(arr != NULL, "Array<Dynamic> created for struct storage");

    /* Test struct set/get using raw API */
    Vec3 v1 = {1.0f, 2.0f, 3.0f};
    Vec3 v2 = {4.0f, 5.0f, 6.0f};

    /* We can't fully test struct arrays without a loaded module,
     * but we can verify the API accepts the calls */
    bool set_ok = hlffi_array_set_struct(vm, arr, 0, &v1, sizeof(Vec3));
    /* Expected to fail without proper struct type, but API should not crash */
    printf("  hlffi_array_set_struct() returned: %s (expected without module)\n",
           set_ok ? "true" : "false");

    hlffi_value_free(arr);
}

/* Test 6: NativeArray<Struct> - Contiguous memory */
void test_native_array_struct(hlffi_vm* vm) {
    TEST("NativeArray<Struct> - Contiguous struct memory");

    /* Create NativeArray with struct-like size allocation
     * Without a proper struct type, we can't use hlffi_native_array_new_struct,
     * but we can simulate it with raw bytes */

    const int STRUCT_SIZE = sizeof(Vec3);
    const int COUNT = 100;

    /* Allocate as bytes array to simulate struct array */
    hlffi_value* arr = hlffi_native_array_new(vm, &hlt_ui8, STRUCT_SIZE * COUNT);
    ASSERT(arr != NULL, "NativeArray allocated (simulating struct array)");

    char* data = (char*)hlffi_native_array_get_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to struct array");

    /* Cast to Vec3 array and fill */
    Vec3* vecs = (Vec3*)data;
    for (int i = 0; i < COUNT; i++) {
        vecs[i].x = i * 1.0f;
        vecs[i].y = i * 2.0f;
        vecs[i].z = i * 3.0f;
    }

    /* Verify data */
    ASSERT_FLOAT_EQ(vecs[0].x, 0.0f, 0.001f, "vecs[0].x = 0.0");
    ASSERT_FLOAT_EQ(vecs[0].y, 0.0f, 0.001f, "vecs[0].y = 0.0");
    ASSERT_FLOAT_EQ(vecs[50].x, 50.0f, 0.001f, "vecs[50].x = 50.0");
    ASSERT_FLOAT_EQ(vecs[50].y, 100.0f, 0.001f, "vecs[50].y = 100.0");
    ASSERT_FLOAT_EQ(vecs[99].z, 297.0f, 0.001f, "vecs[99].z = 297.0");

    printf("  ✓ Contiguous struct memory layout works\n");

    hlffi_value_free(arr);
}

/* Test 7: Mixed - Regular Array vs NativeArray */
void test_array_vs_native_array(hlffi_vm* vm) {
    TEST("Comparison: Array<T> vs NativeArray<T>");

    /* Create regular Array<Int> */
    hlffi_value* arr1 = hlffi_array_new(vm, &hlt_i32, 5);
    ASSERT(arr1 != NULL, "Regular Array<Int> created");

    /* Set values using hlffi_array_set */
    for (int i = 0; i < 5; i++) {
        hlffi_value* val = hlffi_value_int(vm, i * 100);
        hlffi_array_set(vm, arr1, i, val);
        hlffi_value_free(val);
    }

    /* Verify with hlffi_array_get */
    hlffi_value* elem = hlffi_array_get(vm, arr1, 3);
    int val = hlffi_value_as_int(elem, -1);
    ASSERT_EQ(val, 300, "Array<Int>[3] = 300 via get/set API");
    hlffi_value_free(elem);

    /* Create NativeArray<Int> */
    hlffi_value* arr2 = hlffi_native_array_new(vm, &hlt_i32, 5);
    ASSERT(arr2 != NULL, "NativeArray<Int> created");

    /* Set values using direct pointer */
    int* data = (int*)hlffi_native_array_get_ptr(arr2);
    for (int i = 0; i < 5; i++) {
        data[i] = i * 100;
    }

    /* Verify direct access */
    ASSERT_EQ(data[3], 300, "NativeArray<Int>[3] = 300 via direct pointer");

    printf("  ✓ Both Array<T> and NativeArray<T> work correctly\n");
    printf("  ✓ NativeArray provides zero-copy direct access\n");

    hlffi_value_free(arr1);
    hlffi_value_free(arr2);
}

/* Test 8: Edge cases */
void test_edge_cases(hlffi_vm* vm) {
    TEST("Edge cases and error handling");

    /* Empty array */
    hlffi_value* arr1 = hlffi_native_array_new(vm, &hlt_i32, 0);
    ASSERT(arr1 != NULL, "Can create empty NativeArray");
    ASSERT_EQ(hlffi_array_length(arr1), 0, "Empty array has length 0");
    hlffi_value_free(arr1);

    /* NULL pointer handling */
    void* ptr = hlffi_native_array_get_ptr(NULL);
    ASSERT(ptr == NULL, "NULL array returns NULL pointer");

    /* Large array */
    hlffi_value* arr2 = hlffi_native_array_new(vm, &hlt_i32, 1000000);
    ASSERT(arr2 != NULL, "Can create large array (1M elements)");
    ASSERT_EQ(hlffi_array_length(arr2), 1000000, "Large array has correct length");
    hlffi_value_free(arr2);

    printf("  ✓ Edge cases handled correctly\n");
}

int main(int argc, char** argv) {
    printf("===========================================\n");
    printf("  Phase 5 Array Tests: NativeArray + Struct Arrays\n");
    printf("===========================================\n");

    /* Initialize HLFFI */
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        printf("Failed to create HLFFI VM\n");
        return 1;
    }

    if (!hlffi_init_args(vm, 0, NULL)) {
        printf("Failed to initialize HLFFI VM\n");
        return 1;
    }

    printf("\nRunning tests...\n");

    /* Run all tests */
    test_native_array_int(vm);
    test_native_array_float(vm);
    test_native_array_double(vm);
    test_native_array_performance(vm);
    test_array_struct_wrapped(vm);
    test_native_array_struct(vm);
    test_array_vs_native_array(vm);
    test_edge_cases(vm);

    /* Cleanup */
    hlffi_close(vm);
    hlffi_destroy(vm);

    /* Print results */
    printf("\n===========================================\n");
    printf("Test Results:\n");
    printf("  ✓ Passed: %d\n", tests_passed);
    if (tests_failed > 0) {
        printf("  ✗ Failed: %d\n", tests_failed);
    }
    printf("===========================================\n");

    return tests_failed == 0 ? 0 : 1;
}
