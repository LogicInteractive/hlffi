/* Minimal test for typed callbacks - isolate the crash */
#define HLFFI_IMPLEMENTATION
#include "hlffi.h"
#include <stdio.h>

static hlffi_value* callback_test(hlffi_vm* vm, int argc, hlffi_value** argv) {
    printf("Callback invoked!\n");
    return hlffi_value_null(vm);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Minimal Typed Callback Test ===\n");

    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    if (hlffi_init(vm, argc, (char**)argv) != HLFFI_OK) {
        fprintf(stderr, "Init failed\n");
        hlffi_destroy(vm);
        return 1;
    }

    if (hlffi_load_file(vm, argv[1]) != HLFFI_OK) {
        fprintf(stderr, "Load failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Entry failed\n");
        hlffi_destroy(vm);
        return 1;
    }

    printf("VM initialized successfully\n");

    /* Test 1: Register ONE typed callback */
    printf("\nTest 1: Registering typed callback (Void->Void)...\n");
    bool ok = hlffi_register_callback_typed(vm, "onNotify", callback_test,
                                             0, NULL, HLFFI_ARG_VOID);
    printf("Registration result: %s\n", ok ? "SUCCESS" : "FAILED");
    if (!ok) {
        fprintf(stderr, "Error: %s\n", hlffi_get_error(vm));
    }

    printf("\nTest 2: Getting callback...\n");
    hlffi_value* cb = hlffi_get_callback(vm, "onNotify");
    printf("Get result: %s\n", cb ? "SUCCESS" : "FAILED");
    if (cb) hlffi_value_free(cb);

    printf("\nTest 3: Register MULTIPLE typed callbacks...\n");

    printf("Registering onNotify (Void->Void)...\n");
    ok = hlffi_register_callback_typed(vm, "onNotify", callback_test,
                                        0, NULL, HLFFI_ARG_VOID);
    printf("  Result: %s\n", ok ? "OK" : "FAIL");

    printf("Registering onMessage (String->Void)...\n");
    hlffi_arg_type args1[] = {HLFFI_ARG_STRING};
    ok = hlffi_register_callback_typed(vm, "onMessage", callback_test,
                                        1, args1, HLFFI_ARG_VOID);
    printf("  Result: %s\n", ok ? "OK" : "FAIL");

    printf("Registering onAdd ((Int,Int)->Int)...\n");
    hlffi_arg_type args2[] = {HLFFI_ARG_INT, HLFFI_ARG_INT};
    ok = hlffi_register_callback_typed(vm, "onAdd", callback_test,
                                        2, args2, HLFFI_ARG_INT);
    printf("  Result: %s\n", ok ? "OK" : "FAIL");

    printf("Registering onCompute ((Int,Int,Int)->Int)...\n");
    hlffi_arg_type args3[] = {HLFFI_ARG_INT, HLFFI_ARG_INT, HLFFI_ARG_INT};
    ok = hlffi_register_callback_typed(vm, "onCompute", callback_test,
                                        3, args3, HLFFI_ARG_INT);
    printf("  Result: %s\n", ok ? "OK" : "FAIL");

    printf("\nAll registrations complete!\n");

    printf("\nTest 4: Setting typed callbacks in Haxe fields...\n");

    printf("Setting onMessage...\n");
    cb = hlffi_get_callback(vm, "onMessage");
    if (cb) {
        bool set_ok = hlffi_set_static_field(vm, "Callbacks", "onMessage", cb);
        printf("  Set result: %s\n", set_ok ? "OK" : "FAIL");
        if (!set_ok) printf("  Error: %s\n", hlffi_get_error(vm));
        hlffi_value_free(cb);
    }

    printf("Setting onAdd...\n");
    cb = hlffi_get_callback(vm, "onAdd");
    if (cb) {
        bool set_ok = hlffi_set_static_field(vm, "Callbacks", "onAdd", cb);
        printf("  Set result: %s\n", set_ok ? "OK" : "FAIL");
        if (!set_ok) printf("  Error: %s\n", hlffi_get_error(vm));
        hlffi_value_free(cb);
    }

    printf("\nAll sets complete!\n");

    printf("\nCleaning up...\n");
    hlffi_destroy(vm);
    printf("Done!\n");

    return 0;
}
