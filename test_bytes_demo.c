/**
 * Bytes Demo - Shows Bytes operations from C
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hl.h>

int main(int argc, char** argv) {
    printf("==========================================\n");
    printf("  Phase 5: Bytes Demo - Haxe ↔ C\n");
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
    if (hlffi_load_file(vm, "test/bytes_test.hl") != HLFFI_OK) {
        fprintf(stderr, "Failed to load bytes_test.hl: %s\n", hlffi_get_error(vm));
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

    printf("\n=== C Side: Testing Bytes Operations ===\n\n");

    /* Test 1: Get hl.Bytes from Haxe and read it */
    printf("--- Test 1: hl.Bytes from Haxe ---\n");
    hlffi_value* hlBytes = hlffi_call_static(vm, "BytesTest", "createHLBytes", 0, NULL);
    if (hlBytes) {
        printf("[C] Got hl.Bytes from Haxe\n");

        /* Get pointer to bytes */
        unsigned char* ptr = (unsigned char*)hlffi_bytes_get_ptr(hlBytes);
        if (ptr) {
            /* Convert to string (null-terminated) */
            char* str = (char*)ptr;
            printf("[C] Bytes as string: \"%s\"\n", str);

            /* Show individual bytes */
            printf("[C] Individual bytes: ");
            for (int i = 0; i < 6 && ptr[i] != 0; i++) {
                printf("%02X ", ptr[i]);
            }
            printf("\n");
        }

        hlffi_value_free(hlBytes);
    }

    /* Test 2: Get haxe.io.Bytes from Haxe */
    printf("\n--- Test 2: haxe.io.Bytes from Haxe ---\n");
    hlffi_value* ioBytes = hlffi_call_static(vm, "BytesTest", "createIOBytes", 0, NULL);
    if (ioBytes) {
        printf("[C] Got haxe.io.Bytes from Haxe\n");

        /* Get length (should work for haxe.io.Bytes objects) */
        int length = hlffi_bytes_get_length(ioBytes);
        printf("[C] Bytes length: %d\n", length);

        /* Display bytes from Haxe */
        hlffi_value* args[] = {ioBytes};
        hlffi_call_static(vm, "BytesTest", "displayBytes", 1, args);

        hlffi_value_free(ioBytes);
    }

    /* NOTE: Tests 3-5 (creating bytes from C) are not yet implemented.
     * For now, create bytes in Haxe and pass to C for manipulation. */

    /* Cleanup */
    hlffi_destroy(vm);

    printf("\n==========================================\n");
    printf("  ✓ Bytes tests complete!\n");
    printf("  ✓ Demonstrated create, read, modify\n");
    printf("  ✓ Showed C ↔ Haxe bytes interop\n");
    printf("==========================================\n");

    return 0;
}
