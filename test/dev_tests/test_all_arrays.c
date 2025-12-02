/**
 * Comprehensive Array Test Suite
 * Tests ALL Phase 5 array types through HLFFI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define HLFFI_IMPLEMENTATION
#include "../include/hlffi.h"

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    printf("\n=== Test: %s ===\n", name);

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

typedef struct {
    float x, y, z;
} Vec3;

void test_array_int(hlffi_vm* vm) {
    TEST("Array<Int> - Regular wrapped array");

    /* Call Haxe function to get array */
    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getIntArray", 0, NULL);
    ASSERT(arr != NULL, "Got Array<Int> from Haxe");

    /* Check length */
    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 5, "Array length is 5");

    /* Get elements */
    hlffi_value* elem0 = hlffi_array_get(vm, arr, 0);
    int val0 = hlffi_value_as_int(elem0, -1);
    ASSERT_EQ(val0, 10, "arr[0] = 10");
    hlffi_value_free(elem0);

    hlffi_value* elem4 = hlffi_array_get(vm, arr, 4);
    int val4 = hlffi_value_as_int(elem4, -1);
    ASSERT_EQ(val4, 50, "arr[4] = 50");
    hlffi_value_free(elem4);

    /* Pass back to Haxe for processing */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processIntArray", 1, args);
    int sum = hlffi_value_as_int(result, -1);
    ASSERT_EQ(sum, 150, "Sum of array is 150");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_array_single(hlffi_vm* vm) {
    TEST("Array<Single> - F32 array");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getSingleArray", 0, NULL);
    ASSERT(arr != NULL, "Got Array<Single> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 5, "Array length is 5");

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processSingleArray", 1, args);
    float sum = hlffi_value_as_f32(result, -1);
    ASSERT_FLOAT_EQ(sum, 5.0f, 0.01f, "Sum of Single array is 5.0");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_array_float(hlffi_vm* vm) {
    TEST("Array<Float> - F64 array");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getFloatArray", 0, NULL);
    ASSERT(arr != NULL, "Got Array<Float> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 3, "Array length is 3");

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processFloatArray", 1, args);
    double sum = hlffi_value_as_float(result, -1);
    ASSERT_FLOAT_EQ(sum, 7.26, 0.01, "Sum of Float array is ~7.26");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_array_bool(hlffi_vm* vm) {
    TEST("Array<Bool> - Boolean array");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getBoolArray", 0, NULL);
    ASSERT(arr != NULL, "Got Array<Bool> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 5, "Array length is 5");

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processBoolArray", 1, args);
    int count = hlffi_value_as_int(result, -1);
    ASSERT_EQ(count, 3, "True count is 3");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_array_string(hlffi_vm* vm) {
    TEST("Array<String> - String array");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getStringArray", 0, NULL);
    ASSERT(arr != NULL, "Got Array<String> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 4, "Array length is 4");

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processStringArray", 1, args);
    char* joined = hlffi_value_as_string(result);
    ASSERT(joined != NULL, "Got joined string");
    printf("  Joined: \"%s\"\n", joined);
    ASSERT(strstr(joined, "Hello World") != NULL, "Contains 'Hello World'");

    free(joined);
    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_native_array_int(hlffi_vm* vm) {
    TEST("NativeArray<Int> - Direct memory access");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getNativeIntArray", 0, NULL);
    ASSERT(arr != NULL, "Got NativeArray<Int> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 10, "NativeArray length is 10");

    /* Get direct pointer */
    int* data = (int*)hlffi_native_array_get_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to data");

    /* Verify values via direct access */
    ASSERT_EQ(data[0], 0, "data[0] = 0");
    ASSERT_EQ(data[5], 500, "data[5] = 500");
    ASSERT_EQ(data[9], 900, "data[9] = 900");

    /* Modify via direct pointer */
    data[0] = 999;
    ASSERT_EQ(data[0], 999, "Modified data[0] = 999");

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processNativeIntArray", 1, args);
    int sum = hlffi_value_as_int(result, -1);
    /* Sum should be: 999 + 100 + 200 + 300 + 400 + 500 + 600 + 700 + 800 + 900 = 5499 */
    ASSERT_EQ(sum, 5499, "Sum of modified NativeArray is 5499");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_native_array_single(hlffi_vm* vm) {
    TEST("NativeArray<Single> - F32 direct access");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getNativeSingleArray", 0, NULL);
    ASSERT(arr != NULL, "Got NativeArray<Single> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 5, "NativeArray length is 5");

    /* Get direct pointer */
    float* data = (float*)hlffi_native_array_get_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to float data");

    /* Verify values */
    ASSERT_FLOAT_EQ(data[0], 0.0f, 0.01f, "data[0] = 0.0");
    ASSERT_FLOAT_EQ(data[2], 3.0f, 0.01f, "data[2] = 3.0");
    ASSERT_FLOAT_EQ(data[4], 6.0f, 0.01f, "data[4] = 6.0");

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processNativeSingleArray", 1, args);
    float sum = hlffi_value_as_f32(result, -1);
    ASSERT_FLOAT_EQ(sum, 15.0f, 0.01f, "Sum is 15.0");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_native_array_float(hlffi_vm* vm) {
    TEST("NativeArray<Float> - F64 direct access");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getNativeFloatArray", 0, NULL);
    ASSERT(arr != NULL, "Got NativeArray<Float> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 3, "NativeArray length is 3");

    /* Get direct pointer */
    double* data = (double*)hlffi_native_array_get_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to double data");

    /* Verify values */
    ASSERT_FLOAT_EQ(data[0], 3.141592653589793, 0.0001, "data[0] = π");
    ASSERT_FLOAT_EQ(data[1], 2.718281828459045, 0.0001, "data[1] = e");
    ASSERT_FLOAT_EQ(data[2], 1.414213562373095, 0.0001, "data[2] = √2");

    hlffi_value_free(arr);
}

void test_native_array_bool(hlffi_vm* vm) {
    TEST("NativeArray<Bool> - Boolean direct access");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getNativeBoolArray", 0, NULL);
    ASSERT(arr != NULL, "Got NativeArray<Bool> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 5, "NativeArray length is 5");

    /* Get direct pointer */
    bool* data = (bool*)hlffi_native_array_get_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to bool data");

    /* Verify values */
    ASSERT(data[0] == true, "data[0] = true");
    ASSERT(data[1] == false, "data[1] = false");
    ASSERT(data[2] == true, "data[2] = true");

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processNativeBoolArray", 1, args);
    int count = hlffi_value_as_int(result, -1);
    ASSERT_EQ(count, 3, "True count is 3");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_array_vec3(hlffi_vm* vm) {
    TEST("Array<Vec3> - Wrapped struct array");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getVec3Array", 0, NULL);
    ASSERT(arr != NULL, "Got Array<Vec3> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 3, "Array length is 3");

    /* Get struct element (unwraps vdynamic*) */
    Vec3* v0 = (Vec3*)hlffi_array_get_struct(arr, 0);
    if (v0) {
        ASSERT_FLOAT_EQ(v0->x, 1.0f, 0.01f, "vec[0].x = 1.0");
        ASSERT_FLOAT_EQ(v0->y, 0.0f, 0.01f, "vec[0].y = 0.0");
    } else {
        printf("✗ Could not get vec[0]\n");
        tests_failed++;
    }

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processVec3Array", 1, args);
    float sum = hlffi_value_as_f32(result, -1);
    ASSERT_FLOAT_EQ(sum, 3.0f, 0.01f, "Sum of Vec3 components is 3.0");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

void test_native_array_vec3(hlffi_vm* vm) {
    TEST("NativeArray<Vec3> - Contiguous struct array");

    hlffi_value* arr = hlffi_call_static(vm, "ArrayTest", "getNativeVec3Array", 0, NULL);
    ASSERT(arr != NULL, "Got NativeArray<Vec3> from Haxe");

    int len = hlffi_array_length(arr);
    ASSERT_EQ(len, 100, "NativeArray length is 100");

    /* Get direct pointer to struct array */
    Vec3* data = (Vec3*)hlffi_native_array_get_struct_ptr(arr);
    ASSERT(data != NULL, "Got direct pointer to Vec3 array");

    /* Verify values via direct access */
    ASSERT_FLOAT_EQ(data[0].x, 0.0f, 0.01f, "data[0].x = 0.0");
    ASSERT_FLOAT_EQ(data[50].x, 50.0f, 0.01f, "data[50].x = 50.0");
    ASSERT_FLOAT_EQ(data[50].y, 100.0f, 0.01f, "data[50].y = 100.0");
    ASSERT_FLOAT_EQ(data[99].z, 297.0f, 0.01f, "data[99].z = 297.0");

    /* Modify via direct pointer */
    data[0].x = 999.0f;
    ASSERT_FLOAT_EQ(data[0].x, 999.0f, 0.01f, "Modified data[0].x = 999.0");

    /* Process in Haxe */
    hlffi_value* args[] = {arr};
    hlffi_value* result = hlffi_call_static(vm, "ArrayTest", "processNativeVec3Array", 1, args);
    float sum = hlffi_value_as_f32(result, -1);
    printf("  Sum of all Vec3 components: %.2f\n", sum);
    ASSERT(sum > 29000.0f, "Sum is reasonable (>29000)");

    hlffi_value_free(result);
    hlffi_value_free(arr);
}

int main(int argc, char** argv) {
    printf("===========================================\n");
    printf("  HLFFI Phase 5: Complete Array Test Suite\n");
    printf("===========================================\n");

    /* Initialize HLFFI */
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        printf("Failed to create HLFFI VM\n");
        return 1;
    }

    if (hlffi_init(vm, argc, argv) != HLFFI_OK) {
        printf("Failed to initialize HLFFI: %s\n", hlffi_get_error(vm));
        return 1;
    }

    /* Load bytecode */
    if (hlffi_load_file(vm, "test/array_test.hl") != HLFFI_OK) {
        printf("Failed to load array_test.hl: %s\n", hlffi_get_error(vm));
        return 1;
    }

    /* Call entry point to initialize Haxe side */
    hlffi_call_entry(vm);

    printf("\nRunning C-side tests...\n");

    /* Run all tests */
    test_array_int(vm);
    test_array_single(vm);
    test_array_float(vm);
    test_array_bool(vm);
    test_array_string(vm);
    test_native_array_int(vm);
    test_native_array_single(vm);
    test_native_array_float(vm);
    test_native_array_bool(vm);
    test_array_vec3(vm);
    test_native_array_vec3(vm);

    /* Cleanup */
    hlffi_destroy(vm);

    /* Print results */
    printf("\n===========================================\n");
    printf("Test Results:\n");
    printf("  ✓ Passed: %d\n", tests_passed);
    if (tests_failed > 0) {
        printf("  ✗ Failed: %d\n", tests_failed);
    }
    printf("===========================================\n");

    printf("\n✅ ALL PHASE 5 ARRAY TYPES TESTED:\n");
    printf("  1. Array<Int>            ✓\n");
    printf("  2. Array<Single> (F32)   ✓\n");
    printf("  3. Array<Float> (F64)    ✓\n");
    printf("  4. Array<Bool>           ✓\n");
    printf("  5. Array<String>         ✓\n");
    printf("  6. NativeArray<Int>      ✓\n");
    printf("  7. NativeArray<Single>   ✓\n");
    printf("  8. NativeArray<Float>    ✓\n");
    printf("  9. NativeArray<Bool>     ✓\n");
    printf(" 10. Array<Struct>         ✓\n");
    printf(" 11. NativeArray<Struct>   ✓\n");

    return tests_failed == 0 ? 0 : 1;
}
