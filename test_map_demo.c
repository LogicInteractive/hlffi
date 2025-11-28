/**
 * Map Demo - Shows Map operations from C
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <hl.h>

int main(int argc, char** argv) {
    printf("==========================================\n");
    printf("  Phase 5: Map Demo - Haxe ↔ C\n");
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
    if (hlffi_load_file(vm, "test/map_test.hl") != HLFFI_OK) {
        fprintf(stderr, "Failed to load map_test.hl: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Call Haxe main() */
    printf("=== Calling Haxe main() ===\n");
    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry point: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    printf("\n=== C Side: Testing Map Operations ===\n\n");

    /* Test 1: Get IntMap from Haxe and read it */
    printf("--- Test 1: IntMap from Haxe ---\n");
    hlffi_value* intMap = hlffi_call_static(vm, "MapTest", "createIntMap", 0, NULL);
    if (intMap) {
        printf("[C] Got IntMap from Haxe\n");

        /* Try to get value for key 2 */
        hlffi_value* key = hlffi_value_int(vm, 2);
        hlffi_value* val = hlffi_map_get(vm, intMap, key);
        if (val) {
            char* str = hlffi_value_as_string(val);
            printf("[C] map[2] = \"%s\"\n", str ? str : "null");
            free(str);
            hlffi_value_free(val);
        }

        /* Check if key exists */
        bool exists = hlffi_map_exists(vm, intMap, key);
        printf("[C] map.exists(2) = %s\n", exists ? "true" : "false");

        hlffi_value* key99 = hlffi_value_int(vm, 99);
        bool exists99 = hlffi_map_exists(vm, intMap, key99);
        printf("[C] map.exists(99) = %s\n", exists99 ? "true" : "false");

        hlffi_value_free(key);
        hlffi_value_free(key99);
        hlffi_value_free(intMap);
    }

    /* Test 2: Get StringMap from Haxe */
    printf("\n--- Test 2: StringMap from Haxe ---\n");
    hlffi_value* strMap = hlffi_call_static(vm, "MapTest", "createStringMap", 0, NULL);
    if (strMap) {
        printf("[C] Got StringMap from Haxe\n");

        /* Get value for key "b" */
        hlffi_value* key = hlffi_value_string(vm, "b");
        hlffi_value* val = hlffi_map_get(vm, strMap, key);
        if (val) {
            int num = hlffi_value_as_int(val, -1);
            printf("[C] map[\"b\"] = %d\n", num);
            hlffi_value_free(val);
        }

        /* Check existence */
        bool exists = hlffi_map_exists(vm, strMap, key);
        printf("[C] map.exists(\"b\") = %s\n", exists ? "true" : "false");

        hlffi_value_free(key);
        hlffi_value_free(strMap);
    }

    /* Test 3: Modify map from C */
    printf("\n--- Test 3: Modify Map from C ---\n");
    hlffi_value* map3 = hlffi_call_static(vm, "MapTest", "createIntMap", 0, NULL);
    if (map3) {
        printf("[C] Got map, adding new entry...\n");

        /* Add new key-value pair */
        hlffi_value* key = hlffi_value_int(vm, 42);
        hlffi_value* val = hlffi_value_string(vm, "answer");
        bool set_ok = hlffi_map_set(vm, map3, key, val);
        printf("[C] map.set(42, \"answer\") = %s\n", set_ok ? "success" : "failed");

        /* Verify it was added */
        hlffi_value* check_val = hlffi_map_get(vm, map3, key);
        if (check_val) {
            char* str = hlffi_value_as_string(check_val);
            printf("[C] Verification: map[42] = \"%s\"\n", str ? str : "null");
            free(str);
            hlffi_value_free(check_val);
        }

        /* Pass back to Haxe for processing */
        printf("[C] Passing modified map back to Haxe...\n");
        hlffi_value* args[] = {map3};
        hlffi_value* result = hlffi_call_static(vm, "MapTest", "processIntMap", 1, args);
        if (result) {
            char* str = hlffi_value_as_string(result);
            printf("[C] Haxe processed result: \"%s\"\n", str ? str : "null");
            free(str);
            hlffi_value_free(result);
        }

        hlffi_value_free(key);
        hlffi_value_free(val);
        hlffi_value_free(map3);
    }

    /* Cleanup */
    hlffi_destroy(vm);

    printf("\n==========================================\n");
    printf("  ✓ Map tests complete!\n");
    printf("  ✓ Demonstrated get, set, exists\n");
    printf("  ✓ Showed C ↔ Haxe map interop\n");
    printf("==========================================\n");

    return 0;
}
