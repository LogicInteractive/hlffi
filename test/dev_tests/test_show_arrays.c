/**
 * Simple demonstration showing array values being passed between C and Haxe
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hl.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <arrays.hl>\n", argv[0]);
        return 1;
    }

    /* Initialize VM */
    hlffi_vm* vm = hlffi_create();
    if (!vm || hlffi_init(vm, argc, argv) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM\n");
        return 1;
    }

    if (hlffi_load_file(vm, argv[1]) != HLFFI_OK || hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to load/run bytecode\n");
        return 1;
    }

    printf("\n========== ARRAY VALUES DEMONSTRATION ==========\n\n");

    /* Test 1: Int Array */
    printf("TEST 1: Int Array (i32)\n");
    printf("------------------------\n");
    {
        printf("[C] Creating Int array with values: 10, 20, 30, 40, 50\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_i32, 5);

        int values[] = {10, 20, 30, 40, 50};
        for (int i = 0; i < 5; i++) {
            hlffi_value* v = hlffi_value_int(vm, values[i]);
            hlffi_array_set(vm, arr, i, v);
            hlffi_value_free(v);
        }

        printf("[C] Passing array to Haxe printIntArray()...\n");
        hlffi_value* args[] = {arr};
        hlffi_call_static(vm, "Arrays", "printIntArray", 1, args);

        printf("[C] Calling sumIntArray()...\n");
        hlffi_value* sum = hlffi_call_static(vm, "Arrays", "sumIntArray", 1, args);
        printf("[C] Haxe returned sum = %d\n", hlffi_value_as_int(sum, -1));

        hlffi_value_free(sum);
        hlffi_value_free(arr);
    }

    /* Test 2: Float Array */
    printf("\nTEST 2: Float Array (f64)\n");
    printf("--------------------------\n");
    {
        printf("[C] Creating Float array with values: 1.5, 2.5, 3.5, 4.5\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_f64, 4);

        double values[] = {1.5, 2.5, 3.5, 4.5};
        for (int i = 0; i < 4; i++) {
            hlffi_value* v = hlffi_value_float(vm, values[i]);
            hlffi_array_set(vm, arr, i, v);
            hlffi_value_free(v);
        }

        printf("[C] Passing array to Haxe printFloatArray()...\n");
        hlffi_value* args[] = {arr};
        hlffi_call_static(vm, "Arrays", "printFloatArray", 1, args);

        printf("[C] Calling sumFloatArray()...\n");
        hlffi_value* sum = hlffi_call_static(vm, "Arrays", "sumFloatArray", 1, args);
        printf("[C] Haxe returned sum = %.1f\n", hlffi_value_as_float(sum, 0.0));

        hlffi_value_free(sum);
        hlffi_value_free(arr);
    }

    /* Test 3: Single Array (F32) */
    printf("\nTEST 3: Single Array (f32) - NEW F32 SUPPORT!\n");
    printf("----------------------------------------------\n");
    {
        printf("[C] Creating Single array with values: 1.1f, 2.2f, 3.3f\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_f32, 3);

        float values[] = {1.1f, 2.2f, 3.3f};
        for (int i = 0; i < 3; i++) {
            hlffi_value* v = hlffi_value_f32(vm, values[i]);  // Using new F32 function!
            hlffi_array_set(vm, arr, i, v);
            hlffi_value_free(v);
        }

        printf("[C] Passing array to Haxe printSingleArray()...\n");
        hlffi_value* args[] = {arr};
        hlffi_call_static(vm, "Arrays", "printSingleArray", 1, args);

        printf("[C] Calling sumSingleArray()...\n");
        hlffi_value* sum = hlffi_call_static(vm, "Arrays", "sumSingleArray", 1, args);
        printf("[C] Haxe returned sum = %.1f\n", hlffi_value_as_f32(sum, 0.0f));  // Using new F32 function!

        hlffi_value_free(sum);
        hlffi_value_free(arr);
    }

    /* Test 4: String Array */
    printf("\nTEST 4: String Array\n");
    printf("--------------------\n");
    {
        const char* strings[] = {"Hello", "World", "from", "HLFFI"};
        printf("[C] Creating String array with values: ");
        for (int i = 0; i < 4; i++) {
            printf("\"%s\"%s", strings[i], (i < 3) ? ", " : "\n");
        }

        hlffi_value* arr = hlffi_array_new(vm, &hlt_bytes, 4);
        for (int i = 0; i < 4; i++) {
            hlffi_value* v = hlffi_value_string(vm, strings[i]);
            hlffi_array_set(vm, arr, i, v);
            hlffi_value_free(v);
        }

        printf("[C] Passing array to Haxe printStringArray()...\n");
        hlffi_value* args[] = {arr};
        hlffi_call_static(vm, "Arrays", "printStringArray", 1, args);

        printf("[C] Calling joinStrings()...\n");
        hlffi_value* result = hlffi_call_static(vm, "Arrays", "joinStrings", 1, args);
        char* joined = hlffi_value_as_string(result);
        printf("[C] Haxe returned joined string: \"%s\"\n", joined);
        free(joined);

        hlffi_value_free(result);
        hlffi_value_free(arr);
    }

    /* Test 5: Dynamic Array */
    printf("\nTEST 5: Dynamic Array (mixed types)\n");
    printf("------------------------------------\n");
    {
        printf("[C] Creating Dynamic array with: Int(42), String(\"text\"), Float(3.14), Bool(true), null\n");
        hlffi_value* arr = hlffi_array_new(vm, &hlt_dyn, 5);

        hlffi_value* v0 = hlffi_value_int(vm, 42);
        hlffi_array_set(vm, arr, 0, v0);
        hlffi_value_free(v0);

        hlffi_value* v1 = hlffi_value_string(vm, "text");
        hlffi_array_set(vm, arr, 1, v1);
        hlffi_value_free(v1);

        hlffi_value* v2 = hlffi_value_float(vm, 3.14);
        hlffi_array_set(vm, arr, 2, v2);
        hlffi_value_free(v2);

        hlffi_value* v3 = hlffi_value_bool(vm, true);
        hlffi_array_set(vm, arr, 3, v3);
        hlffi_value_free(v3);

        hlffi_value* v4 = hlffi_value_null(vm);
        hlffi_array_set(vm, arr, 4, v4);
        hlffi_value_free(v4);

        printf("[C] Passing array to Haxe printDynamicArray()...\n");
        hlffi_value* args[] = {arr};
        hlffi_call_static(vm, "Arrays", "printDynamicArray", 1, args);

        hlffi_value_free(arr);
    }

    printf("\n========== DEMONSTRATION COMPLETE ==========\n\n");

    hlffi_destroy(vm);
    return 0;
}
