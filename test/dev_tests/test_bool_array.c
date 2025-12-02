/**
 * Bool Array Test
 * Demonstrates Bool array support with ArrayBytes_UI8
 */

#include "hlffi.h"
#include <stdio.h>
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

    printf("\n========== BOOL ARRAY TEST ==========\n\n");

    /* Create Bool array */
    printf("[C] Creating Bool array with values: true, false, true, true, false\n");
    hlffi_value* arr = hlffi_array_new(vm, &hlt_bool, 5);

    bool values[] = {true, false, true, true, false};
    for (int i = 0; i < 5; i++) {
        hlffi_value* v = hlffi_value_bool(vm, values[i]);
        hlffi_array_set(vm, arr, i, v);
        hlffi_value_free(v);
    }

    /* Read values back */
    printf("[C] Reading values back from array:\n");
    for (int i = 0; i < 5; i++) {
        hlffi_value* elem = hlffi_array_get(vm, arr, i);
        bool val = hlffi_value_as_bool(elem, false);
        printf("    arr[%d] = %s\n", i, val ? "true" : "false");
        hlffi_value_free(elem);
    }

    /* Pass to Haxe */
    printf("\n[C→Haxe] Passing Bool array to Haxe printBoolArray()...\n");
    hlffi_value* args[] = {arr};
    hlffi_call_static(vm, "Arrays", "printBoolArray", 1, args);

    printf("\n[C→Haxe] Calling countTrue()...\n");
    hlffi_value* result = hlffi_call_static(vm, "Arrays", "countTrue", 1, args);
    int count = hlffi_value_as_int(result, -1);
    printf("[Haxe→C] Haxe returned count of true values: %d (expected: 3)\n", count);

    hlffi_value_free(result);
    hlffi_value_free(arr);

    printf("\n========== TEST COMPLETE ==========\n\n");

    if (count == 3) {
        printf("✓ Bool array test PASSED!\n\n");
        hlffi_destroy(vm);
        return 0;
    } else {
        printf("✗ Bool array test FAILED!\n\n");
        hlffi_destroy(vm);
        return 1;
    }
}
