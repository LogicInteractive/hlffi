/**
 * String Array Debug Test
 */

#define HLFFI_IMPLEMENTATION
#include "hlffi.h"
#include "src/hlffi_internal.h"
#include <stdio.h>
#include <string.h>
#include <hl.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <arrays.hl>\n", argv[0]);
        return 1;
    }

    hlffi_vm* vm = hlffi_create();
    if (!vm || hlffi_init(vm, argc, argv) != HLFFI_OK ||
        hlffi_load_file(vm, argv[1]) != HLFFI_OK ||
        hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }

    printf("\n========== STRING ARRAY DEBUG ==========\n\n");

    const char* strings[] = {"Hello", "World", "from", "HLFFI"};

    printf("[C] Creating String array with %d elements\n", 4);
    hlffi_value* arr = hlffi_array_new(vm, &hlt_bytes, 4);

    printf("[C] Array created, length = %d\n", hlffi_array_length(arr));

    /* Set each string and read it back immediately */
    for (int i = 0; i < 4; i++) {
        printf("\n[C] Setting arr[%d] = \"%s\"\n", i, strings[i]);
        hlffi_value* v = hlffi_value_string(vm, strings[i]);
        hlffi_array_set(vm, arr, i, v);
        hlffi_value_free(v);

        /* Read it back immediately */
        hlffi_value* elem = hlffi_array_get(vm, arr, i);
        char* str = hlffi_value_as_string(elem);
        printf("[C] Read back arr[%d] = \"%s\" %s\n", i,
               str ? str : "(null)",
               (str && strcmp(str, strings[i]) == 0) ? "✓" : "✗ MISMATCH!");
        free(str);
        hlffi_value_free(elem);
    }

    printf("\n[C] Final readback of all elements:\n");
    for (int i = 0; i < 4; i++) {
        hlffi_value* elem = hlffi_array_get(vm, arr, i);
        char* str = hlffi_value_as_string(elem);
        printf("    arr[%d] = \"%s\"\n", i, str ? str : "(null)");
        free(str);
        hlffi_value_free(elem);
    }

    printf("\n[C] Dumping C-created array memory layout:\n");
    vdynamic* arr_dyn = arr->hl_value;
    if (arr_dyn->t->kind == HOBJ) {
        vobj* obj = (vobj*)arr_dyn;
        printf("    vobj address: %p\n", (void*)obj);
        printf("    type: %p\n", (void*)obj->t);

        /* Read the array field */
        varray** array_field = (varray**)(obj + 1);
        varray* varr = *array_field;
        printf("    varray* at field[0]: %p\n", (void*)varr);
        if (varr) {
            printf("    varray->size: %d\n", varr->size);
            printf("    Memory layout: obj=%p, field[0]=%p, varray=%p, varray->size=%d\n",
                   (void*)obj, (void*)array_field, (void*)varr, varr->size);
        }
    }

    printf("\n[C] Now get a Haxe-created array for comparison:\n");
    hlffi_value* haxe_arr = hlffi_call_static(vm, "Arrays", "getStringArray", 0, NULL);
    if (haxe_arr) {
        vdynamic* haxe_dyn = haxe_arr->hl_value;
        if (haxe_dyn->t->kind == HOBJ) {
            vobj* hobj = (vobj*)haxe_dyn;
            printf("    vobj address: %p\n", (void*)hobj);
            varray** harray_field = (varray**)(hobj + 1);
            varray* hvarr = *harray_field;
            printf("    varray* at field[0]: %p\n", (void*)hvarr);
            if (hvarr) {
                printf("    varray->size: %d\n", hvarr->size);
                printf("    Memory layout: obj=%p, field[0]=%p, varray=%p, varray->size=%d\n",
                       (void*)hobj, (void*)harray_field, (void*)hvarr, hvarr->size);
            }
        }
        hlffi_value_free(haxe_arr);
    }

    printf("\n[C→Haxe] Passing array to Haxe printStringArray()...\n");
    hlffi_value* args[] = {arr};
    hlffi_call_static(vm, "Arrays", "printStringArray", 1, args);

    hlffi_value_free(arr);
    hlffi_destroy(vm);
    return 0;
}
