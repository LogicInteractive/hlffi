/**
 * NativeArray Demo - Shows output from BOTH Haxe and C
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <hl.h>

int main(int argc, char** argv) {
    printf("==========================================\n");
    printf("  NativeArray Demo - Haxe ↔ C Interop\n");
    printf("==========================================\n\n");

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
    if (hlffi_load_file(vm, "test/native_demo.hl") != HLFFI_OK) {
        fprintf(stderr, "Failed to load native_demo.hl: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Call Haxe main() - this prints from Haxe side */
    printf("=== Calling Haxe main() ===\n");
    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry point: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    printf("\n=== Now accessing from C side ===\n\n");

    /* Test 1: Regular Array<Int> from Haxe */
    printf("--- Test 1: Array<Int> (wrapped) ---\n");
    hlffi_value* arr1 = hlffi_call_static(vm, "NativeArrayDemo", "createIntArray", 0, NULL);
    if (arr1) {
        int len = hlffi_array_length(arr1);
        printf("[C] Got Array<Int> from Haxe, length = %d\n", len);

        printf("[C] Reading via hlffi_array_get():\n");
        for (int i = 0; i < len; i++) {
            hlffi_value* elem = hlffi_array_get(vm, arr1, i);
            int val = hlffi_value_as_int(elem, -1);
            printf("[C]   arr[%d] = %d\n", i, val);
            hlffi_value_free(elem);
        }

        /* Pass back to Haxe */
        hlffi_value* args[] = {arr1};
        hlffi_value* result = hlffi_call_static(vm, "NativeArrayDemo", "processIntArray", 1, args);
        int sum = hlffi_value_as_int(result, 0);
        printf("[C] Haxe returned sum = %d\n", sum);
        hlffi_value_free(result);
        hlffi_value_free(arr1);
    }

    /* Test 2: NativeArray<Int> with direct pointer access */
    printf("\n--- Test 2: NativeArray<Int> (direct pointer) ---\n");
    hlffi_value* arr2 = hlffi_call_static(vm, "NativeArrayDemo", "createNativeIntArray", 0, NULL);
    if (arr2) {
        int len = hlffi_array_length(arr2);
        printf("[C] Got NativeArray<Int> from Haxe, length = %d\n", len);

        /* Direct pointer access - ZERO COPY! */
        int* data = (int*)hlffi_native_array_get_ptr(arr2);
        if (data) {
            printf("[C] Got DIRECT pointer to array data!\n");
            printf("[C] Reading via direct pointer (zero-copy):\n");
            for (int i = 0; i < len; i++) {
                printf("[C]   data[%d] = %d\n", i, data[i]);
            }

            /* Modify via direct pointer */
            printf("[C] Modifying data[0] from %d to 999\n", data[0]);
            data[0] = 999;

            /* Haxe sees the change immediately! */
            hlffi_value* args[] = {arr2};
            hlffi_value* result = hlffi_call_static(vm, "NativeArrayDemo", "processNativeIntArray", 1, args);
            int sum = hlffi_value_as_int(result, 0);
            printf("[C] Haxe sees modified data, sum = %d (should be 1499)\n", sum);
            hlffi_value_free(result);
        }
        hlffi_value_free(arr2);
    }

    /* Test 3: NativeArray<Float> (F64) */
    printf("\n--- Test 3: NativeArray<Float> (F64) ---\n");
    hlffi_value* arr3 = hlffi_call_static(vm, "NativeArrayDemo", "createNativeFloatArray", 0, NULL);
    if (arr3) {
        int len = hlffi_array_length(arr3);
        printf("[C] Got NativeArray<Float> from Haxe, length = %d\n", len);

        double* data = (double*)hlffi_native_array_get_ptr(arr3);
        if (data) {
            printf("[C] Reading F64 values via direct pointer:\n");
            printf("[C]   π  = %.8f\n", data[0]);
            printf("[C]   e  = %.8f\n", data[1]);
            printf("[C]   √2 = %.8f\n", data[2]);

            /* Compute sum in C */
            double sum = 0.0;
            for (int i = 0; i < len; i++) {
                sum += data[i];
            }
            printf("[C] Sum computed in C = %.8f\n", sum);

            /* Verify with Haxe */
            hlffi_value* args[] = {arr3};
            hlffi_value* result = hlffi_call_static(vm, "NativeArrayDemo", "processNativeFloatArray", 1, args);
            double haxe_sum = hlffi_value_as_float(result, 0.0);
            printf("[C] Haxe computed sum = %.8f (should match!)\n", haxe_sum);
            hlffi_value_free(result);
        }
        hlffi_value_free(arr3);
    }

    /* Test 4: NativeArray<Single> (F32) */
    printf("\n--- Test 4: NativeArray<Single> (F32) ---\n");
    hlffi_value* arr4 = hlffi_call_static(vm, "NativeArrayDemo", "createNativeSingleArray", 0, NULL);
    if (arr4) {
        int len = hlffi_array_length(arr4);
        printf("[C] Got NativeArray<Single> from Haxe, length = %d\n", len);

        float* data = (float*)hlffi_native_array_get_ptr(arr4);
        if (data) {
            printf("[C] Reading F32 values via direct pointer:\n");
            for (int i = 0; i < len; i++) {
                printf("[C]   data[%d] = %.2f (32-bit float)\n", i, data[i]);
            }

            /* Batch operation - multiply all by 2 */
            printf("[C] Performing batch operation: multiply all by 2\n");
            for (int i = 0; i < len; i++) {
                data[i] *= 2.0f;
            }

            printf("[C] After modification:\n");
            for (int i = 0; i < len; i++) {
                printf("[C]   data[%d] = %.2f\n", i, data[i]);
            }
        }
        hlffi_value_free(arr4);
    }

    /* Test 5: Create NativeArray in C, pass to Haxe */
    printf("\n--- Test 5: Create NativeArray in C ---\n");
    hlffi_value* arr5 = hlffi_native_array_new(vm, &hlt_i32, 10);
    if (arr5) {
        printf("[C] Created NativeArray<Int>[10] in C\n");

        int* data = (int*)hlffi_native_array_get_ptr(arr5);
        printf("[C] Filling with squares...\n");
        for (int i = 0; i < 10; i++) {
            data[i] = i * i;
        }

        printf("[C] Passing to Haxe for processing...\n");
        hlffi_value* args[] = {arr5};
        hlffi_value* result = hlffi_call_static(vm, "NativeArrayDemo", "processNativeIntArray", 1, args);
        int sum = hlffi_value_as_int(result, 0);
        printf("[C] Haxe processed C-created array, sum = %d\n", sum);

        hlffi_value_free(result);
        hlffi_value_free(arr5);
    }

    /* Cleanup */
    hlffi_destroy(vm);

    printf("\n==========================================\n");
    printf("  ✓ All tests complete!\n");
    printf("  ✓ Demonstrated Haxe and C output\n");
    printf("  ✓ Showed zero-copy NativeArray access\n");
    printf("==========================================\n");

    return 0;
}
