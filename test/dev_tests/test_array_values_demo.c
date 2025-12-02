/**
 * Array Values Demonstration
 * Shows actual values being passed between C and Haxe for all array types
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hl.h>

static void print_separator(const char* title) {
    printf("\n");
    printf("=================================================================\n");
    printf("  %s\n", title);
    printf("=================================================================\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_arrays.hl>\n", argv[0]);
        return 1;
    }

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

    printf("Array Values Demonstration - C ↔ Haxe\n");
    printf("Showing actual data values at each stage\n");

    /* ================================================================= */
    /* Test 1: Int Array (i32) */
    /* ================================================================= */
    print_separator("Test 1: Int Array (i32)");
    {
        printf("\n[C] Creating Int array...\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 5);

        printf("[C] Setting values in array:\n");
        int int_values[] = {10, 20, 30, 40, 50};
        for (int i = 0; i < 5; i++) {
            hlffi_value* v = hlffi_value_int(vm, int_values[i]);
            hlffi_array_set(vm, arr, i, v);
            printf("    arr[%d] = %d\n", i, int_values[i]);
            hlffi_value_free(v);
        }

        printf("\n[C] Reading values back from array:\n");
        for (int i = 0; i < 5; i++) {
            hlffi_value* elem = hlffi_array_get(vm, arr, i);
            int val = hlffi_value_as_int(elem, -1);
            printf("    arr[%d] = %d\n", i, val);
            hlffi_value_free(elem);
        }

        printf("\n[C→Haxe] Calling Haxe method printIntArray() which will print the array in Haxe...\n");
        hlffi_value* argv[] = {arr};
        hlffi_call_static_method(vm, "Arrays", "printIntArray", 1, argv);

        printf("\n[C→Haxe] Calling sumIntArray() to compute sum in Haxe...\n");
        hlffi_value* result = hlffi_call_static_method(vm, "Arrays", "sumIntArray", 1, argv);
        if (result) {
            int sum = hlffi_value_as_int(result, -1);
            printf("[Haxe→C] Haxe returned sum: %d (expected: 150)\n", sum);
            hlffi_value_free(result);
        }

        hlffi_value_free(arr);
    }

    /* ================================================================= */
    /* Test 2: Float Array (f64) */
    /* ================================================================= */
    print_separator("Test 2: Float Array (f64)");
    {
        printf("\n[C] Creating Float array...\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_f64, 4);

        printf("[C] Setting values in array:\n");
        double float_values[] = {1.5, 2.5, 3.5, 4.5};
        for (int i = 0; i < 4; i++) {
            hlffi_value* v = hlffi_value_float(vm, float_values[i]);
            hlffi_array_set(vm, arr, i, v);
            printf("    arr[%d] = %.1f\n", i, float_values[i]);
            hlffi_value_free(v);
        }

        printf("\n[C] Reading values back from array:\n");
        for (int i = 0; i < 4; i++) {
            hlffi_value* elem = hlffi_array_get(vm, arr, i);
            double val = hlffi_value_as_float(elem, 0.0);
            printf("    arr[%d] = %.1f\n", i, val);
            hlffi_value_free(elem);
        }

        printf("\n[C→Haxe] Calling Haxe method printFloatArray()...\n");
        hlffi_value* argv[] = {arr};
        hlffi_call_static_method(vm, "Arrays", "printFloatArray", 1, argv);

        printf("\n[C→Haxe] Calling sumFloatArray()...\n");
        hlffi_value* result = hlffi_call_static_method(vm, "Arrays", "sumFloatArray", 1, argv);
        if (result) {
            double sum = hlffi_value_as_float(result, 0.0);
            printf("[Haxe→C] Haxe returned sum: %.1f (expected: 12.0)\n", sum);
            hlffi_value_free(result);
        }

        hlffi_value_free(arr);
    }

    /* ================================================================= */
    /* Test 3: Single Array (f32) */
    /* ================================================================= */
    print_separator("Test 3: Single Array (f32)");
    {
        printf("\n[C] Creating Single array...\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_f32, 3);

        printf("[C] Setting values in array:\n");
        float single_values[] = {1.1f, 2.2f, 3.3f};
        for (int i = 0; i < 3; i++) {
            hlffi_value* v = hlffi_value_f32(vm, single_values[i]);
            hlffi_array_set(vm, arr, i, v);
            printf("    arr[%d] = %.1f\n", i, single_values[i]);
            hlffi_value_free(v);
        }

        printf("\n[C] Reading values back from array:\n");
        for (int i = 0; i < 3; i++) {
            hlffi_value* elem = hlffi_array_get(vm, arr, i);
            float val = hlffi_value_as_f32(elem, 0.0f);
            printf("    arr[%d] = %.1f\n", i, val);
            hlffi_value_free(elem);
        }

        printf("\n[C→Haxe] Calling Haxe method printSingleArray()...\n");
        hlffi_value* argv[] = {arr};
        hlffi_call_static_method(vm, "Arrays", "printSingleArray", 1, argv);

        printf("\n[C→Haxe] Calling sumSingleArray()...\n");
        hlffi_value* result = hlffi_call_static_method(vm, "Arrays", "sumSingleArray", 1, argv);
        if (result) {
            float sum = hlffi_value_as_f32(result, 0.0f);
            printf("[Haxe→C] Haxe returned sum: %.1f (expected: 6.6)\n", sum);
            hlffi_value_free(result);
        }

        hlffi_value_free(arr);
    }

    /* ================================================================= */
    /* Test 4: String Array */
    /* ================================================================= */
    print_separator("Test 4: String Array - DETAILED");
    {
        printf("\n[C] Creating String array...\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_bytes, 4);

        printf("[C] Setting values in array:\n");
        const char* string_values[] = {"Hello", "World", "from", "HLFFI"};
        for (int i = 0; i < 4; i++) {
            hlffi_value* v = hlffi_value_string(vm, string_values[i]);
            hlffi_array_set(vm, arr, i, v);
            printf("    arr[%d] = \"%s\" (length: %zu)\n", i, string_values[i], strlen(string_values[i]));
            hlffi_value_free(v);
        }

        printf("\n[C] Reading values back from array:\n");
        for (int i = 0; i < 4; i++) {
            hlffi_value* elem = hlffi_array_get(vm, arr, i);
            char* str = hlffi_value_as_string(elem);
            printf("    arr[%d] = \"%s\" (length: %zu)\n", i, str ? str : "(null)", str ? strlen(str) : 0);
            free(str);
            hlffi_value_free(elem);
        }

        printf("\n[C→Haxe] Calling Haxe method printStringArray()...\n");
        hlffi_value* argv[] = {arr};
        hlffi_call_static_method(vm, "Arrays", "printStringArray", 1, argv);

        printf("\n[C→Haxe] Calling countStrings()...\n");
        hlffi_value* count_result = hlffi_call_static_method(vm, "Arrays", "countStrings", 1, argv);
        if (count_result) {
            int count = hlffi_value_as_int(count_result, -1);
            printf("[Haxe→C] Haxe returned count: %d (expected: 4)\n", count);
            hlffi_value_free(count_result);
        }

        printf("\n[C→Haxe] Calling joinStrings()...\n");
        hlffi_value* join_result = hlffi_call_static_method(vm, "Arrays", "joinStrings", 1, argv);
        if (join_result) {
            char* joined = hlffi_value_as_string(join_result);
            printf("[Haxe→C] Haxe returned joined string: \"%s\"\n", joined ? joined : "(null)");
            free(joined);
            hlffi_value_free(join_result);
        }

        hlffi_value_free(arr);
    }

    /* ================================================================= */
    /* Test 5: Dynamic Array */
    /* ================================================================= */
    print_separator("Test 5: Dynamic Array (Mixed Types)");
    {
        printf("\n[C] Creating Dynamic array...\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_dyn, 5);

        printf("[C] Setting mixed values in array:\n");

        hlffi_value* v0 = hlffi_value_int(vm, 42);
        hlffi_array_set(vm, arr, 0, v0);
        printf("    arr[0] = 42 (Int)\n");
        hlffi_value_free(v0);

        hlffi_value* v1 = hlffi_value_string(vm, "text");
        hlffi_array_set(vm, arr, 1, v1);
        printf("    arr[1] = \"text\" (String)\n");
        hlffi_value_free(v1);

        hlffi_value* v2 = hlffi_value_float(vm, 3.14);
        hlffi_array_set(vm, arr, 2, v2);
        printf("    arr[2] = 3.14 (Float)\n");
        hlffi_value_free(v2);

        hlffi_value* v3 = hlffi_value_bool(vm, true);
        hlffi_array_set(vm, arr, 3, v3);
        printf("    arr[3] = true (Bool)\n");
        hlffi_value_free(v3);

        hlffi_value* v4 = hlffi_value_null(vm);
        hlffi_array_set(vm, arr, 4, v4);
        printf("    arr[4] = null (Null)\n");
        hlffi_value_free(v4);

        printf("\n[C] Reading mixed values back from array:\n");
        for (int i = 0; i < 5; i++) {
            hlffi_value* elem = hlffi_array_get(vm, arr, i);
            printf("    arr[%d] = ", i);

            if (hlffi_value_is_null(elem)) {
                printf("null\n");
            } else if (hlffi_value_is_int(elem)) {
                printf("%d (Int)\n", hlffi_value_as_int(elem, -1));
            } else if (hlffi_value_is_float(elem)) {
                printf("%.2f (Float)\n", hlffi_value_as_float(elem, 0.0));
            } else if (hlffi_value_is_bool(elem)) {
                printf("%s (Bool)\n", hlffi_value_as_bool(elem, false) ? "true" : "false");
            } else if (hlffi_value_is_string(elem)) {
                char* str = hlffi_value_as_string(elem);
                printf("\"%s\" (String)\n", str ? str : "(null)");
                free(str);
            } else {
                printf("(unknown type)\n");
            }

            hlffi_value_free(elem);
        }

        printf("\n[C→Haxe] Calling Haxe method printDynamicArray()...\n");
        hlffi_value* argv[] = {arr};
        hlffi_call_static_method(vm, "Arrays", "printDynamicArray", 1, argv);

        hlffi_value_free(arr);
    }

    /* ================================================================= */
    /* Test 6: Haxe → C Direction */
    /* ================================================================= */
    print_separator("Test 6: Haxe → C Direction");
    {
        printf("\n[C→Haxe] Calling Haxe method to get a String array: getStringArray()\n");
        hlffi_value* haxe_arr = hlffi_call_static_method(vm, "Arrays", "getStringArray", 0, NULL);

        if (haxe_arr) {
            int len = hlffi_array_length(haxe_arr);
            printf("[Haxe→C] Received String array from Haxe with %d elements:\n", len);

            for (int i = 0; i < len; i++) {
                hlffi_value* elem = hlffi_array_get(vm, haxe_arr, i);
                char* str = hlffi_value_as_string(elem);
                printf("    [C] arr[%d] = \"%s\"\n", i, str ? str : "(null)");
                free(str);
                hlffi_value_free(elem);
            }

            hlffi_value_free(haxe_arr);
        }

        printf("\n[C→Haxe] Calling Haxe method to get an Int array: getIntArray()\n");
        hlffi_value* haxe_int_arr = hlffi_call_static_method(vm, "Arrays", "getIntArray", 0, NULL);

        if (haxe_int_arr) {
            int len = hlffi_array_length(haxe_int_arr);
            printf("[Haxe→C] Received Int array from Haxe with %d elements:\n", len);

            for (int i = 0; i < len; i++) {
                hlffi_value* elem = hlffi_array_get(vm, haxe_int_arr, i);
                int val = hlffi_value_as_int(elem, -1);
                printf("    [C] arr[%d] = %d\n", i, val);
                hlffi_value_free(elem);
            }

            hlffi_value_free(haxe_int_arr);
        }
    }

    print_separator("All Tests Complete");
    printf("\nArray values demonstration finished successfully!\n\n");

    hlffi_destroy(vm);
    return 0;
}
