/**
 * Debug: Inspect ArrayObj runtime bindings
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

    printf("\n========== ARRAYOBJ RUNTIME BINDINGS ==========\n\n");

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

                /* Check runtime object */
                hl_runtime_obj* rt = t->obj->rt;
                if (!rt) {
                    printf("Getting runtime proto...\n");
                    rt = hl_get_obj_proto(t);
                }

                if (rt) {
                    printf("Runtime object found!\n");
                    printf("  size: %d\n", rt->size);
                    printf("  nfields: %d\n", rt->nfields);
                    printf("  nproto: %d\n", rt->nproto);
                    printf("  nbindings: %d\n", rt->nbindings);
                    printf("  hasPtr: %d\n", rt->hasPtr);

                    if (rt->nbindings > 0 && rt->bindings) {
                        printf("\nBindings:\n");
                        for (int b = 0; b < rt->nbindings; b++) {
                            hl_runtime_binding* binding = &rt->bindings[b];
                            printf("  [%d] fid=%d, ptr=%p, closure=%p\n",
                                   b, binding->fid, binding->ptr, binding->closure);

                            /* Try to identify which field this is */
                            if (binding->fid < t->obj->nfields) {
                                char fname[64];
                                utostr(fname, sizeof(fname), t->obj->fields[binding->fid].name);
                                printf("       field: %s\n", fname);
                            }
                        }
                    }

                    if (rt->methods && rt->nproto > 0) {
                        printf("\nMethods:\n");
                        for (int m = 0; m < rt->nproto; m++) {
                            printf("  [%d] %p\n", m, rt->methods[m]);
                        }
                    }
                } else {
                    printf("No runtime object!\n");
                }

                break;
            }
        }
    }

    hlffi_destroy(vm);
    return 0;
}
