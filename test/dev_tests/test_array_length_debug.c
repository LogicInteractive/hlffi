/**
 * Debug: Investigate Array.length property mechanism
 */

#define HLFFI_IMPLEMENTATION
#include "hlffi.h"
#include "src/hlffi_internal.h"
#include <stdio.h>
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
        return 1;
    }

    printf("\n========== ARRAY.LENGTH PROPERTY DEBUG ==========\n\n");

    /* Find ArrayObj type */
    hl_code* code = vm->module->code;
    const char* target = "hl.types.ArrayObj";
    int hash = hl_hash_utf8(target);
    hl_type* array_obj_type = NULL;

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), t->obj->name);
            if (hl_hash_utf8(type_name) == hash) {
                array_obj_type = t;
                printf("Found ArrayObj type: %s\n", type_name);
                printf("Number of fields: %d\n", t->obj->nfields);
                printf("Number of methods: %d\n", t->obj->nproto);
                printf("Number of bindings: %d\n", t->obj->nbindings);

                printf("\nFields:\n");
                for (int f = 0; f < t->obj->nfields; f++) {
                    char field_name[64];
                    utostr(field_name, sizeof(field_name), t->obj->fields[f].name);
                    printf("  [%d] %s (type kind: %d, hashed_name: %d)\n",
                           f, field_name, t->obj->fields[f].t->kind,
                           t->obj->fields[f].hashed_name);
                }

                printf("\nMethods:\n");
                for (int m = 0; m < t->obj->nproto; m++) {
                    char method_name[64];
                    utostr(method_name, sizeof(method_name), t->obj->proto[m].name);
                    printf("  [%d] %s (findex: %d, pindex: %d)\n",
                           m, method_name,
                           t->obj->proto[m].findex,
                           t->obj->proto[m].pindex);
                }

                printf("\nBindings:\n");
                for (int b = 0; b < t->obj->nbindings; b++) {
                    printf("  [%d] fid: %d, mid: %d\n",
                           b,
                           t->obj->bindings[b * 2],
                           t->obj->bindings[b * 2 + 1]);
                }

                break;
            }
        }
    }

    if (!array_obj_type) {
        printf("ArrayObj type not found!\n");
        hlffi_destroy(vm);
        return 1;
    }

    /* Now create a Haxe Array from Haxe side and inspect it */
    printf("\n\n========== INSPECTING HAXE-CREATED ARRAY ==========\n\n");
    hlffi_value* haxe_array = hlffi_call_static(vm, "Arrays", "getStringArray", 0, NULL);
    if (!haxe_array) {
        printf("Failed to get array from Haxe\n");
        hlffi_destroy(vm);
        return 1;
    }

    vdynamic* arr_dyn = haxe_array->hl_value;
    printf("Array object address: %p\n", (void*)arr_dyn);
    printf("Array type kind: %d (should be %d for HOBJ)\n", arr_dyn->t->kind, HOBJ);

    if (arr_dyn->t->kind == HOBJ) {
        vobj* obj = (vobj*)arr_dyn;
        printf("vobj address: %p\n", (void*)obj);

        /* Read the array field */
        varray** array_field = (varray**)(obj + 1);
        varray* arr = *array_field;
        printf("varray* at field[0]: %p\n", (void*)arr);
        if (arr) {
            printf("varray->size: %d\n", arr->size);
            printf("varray address of size field: %p\n", (void*)&arr->size);
        }

        /* Dump raw memory */
        printf("\nRaw memory dump of object (first 64 bytes):\n");
        unsigned char* mem = (unsigned char*)obj;
        for (int i = 0; i < 64; i += 8) {
            printf("  +%02d: ", i);
            for (int j = 0; j < 8 && i+j < 64; j++) {
                printf("%02x ", mem[i+j]);
            }
            printf("\n");
        }
    }

    /* Try calling length property if it exists as a method */
    printf("\n\n========== TRYING TO CALL LENGTH PROPERTY ==========\n\n");

    /* Look for get_length method */
    for (int m = 0; m < array_obj_type->obj->nproto; m++) {
        char method_name[64];
        utostr(method_name, sizeof(method_name), array_obj_type->obj->proto[m].name);
        if (strstr(method_name, "length") || strstr(method_name, "get_")) {
            printf("Found potential length method: %s\n", method_name);
        }
    }

    hlffi_value_free(haxe_array);
    hlffi_destroy(vm);
    return 0;
}
