/**
 * Debug: Inspect ArrayObj field structure
 */

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

    printf("\n========== ARRAYOBJ FIELD INSPECTION ==========\n\n");

    /* Find ArrayObj type */
    hl_code* code = vm->module->code;
    const char* target = "hl.types.ArrayObj";
    int hash = hl_hash_utf8(target);

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), t->obj->name);
            if (hl_hash_utf8(type_name) == hash) {
                printf("Found type: %s\n", type_name);
                printf("Number of fields: %d\n", t->obj->nfields);
                printf("\nFields:\n");
                for (int f = 0; f < t->obj->nfields; f++) {
                    char field_name[64];
                    utostr(field_name, sizeof(field_name), t->obj->fields[f].name);
                    printf("  [%d] %s (type kind: %d)\n", f, field_name, t->obj->fields[f].t->kind);
                }
                break;
            }
        }
    }

    /* Also check ArrayBytes_Int for comparison */
    printf("\n");
    target = "hl.types.ArrayBytes_Int";
    hash = hl_hash_utf8(target);

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char type_name[128];
            utostr(type_name, sizeof(type_name), t->obj->name);
            if (hl_hash_utf8(type_name) == hash) {
                printf("Found type: %s\n", type_name);
                printf("Number of fields: %d\n", t->obj->nfields);
                printf("\nFields:\n");
                for (int f = 0; f < t->obj->nfields; f++) {
                    char field_name[64];
                    utostr(field_name, sizeof(field_name), t->obj->fields[f].name);
                    printf("  [%d] %s (type kind: %d)\n", f, field_name, t->obj->fields[f].t->kind);
                }
                break;
            }
        }
    }

    hlffi_destroy(vm);
    return 0;
}
