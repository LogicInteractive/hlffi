#include "hlffi.h"
#include <stdio.h>
#include <hl.h>

int main(int argc, char** argv) {
    hlffi_vm* vm = hlffi_create();
    if (!vm || hlffi_init(vm, argc, argv) != HLFFI_OK ||
        hlffi_load_file(vm, argv[1]) != HLFFI_OK ||
        hlffi_call_entry(vm) != HLFFI_OK) return 1;

    /* Create raw varray (don't wrap) */
    varray* arr = hl_alloc_array(&hlt_bytes, 2);
    
    vdynamic* s1 = hl_alloc_dynamic(&hlt_bytes);
    s1->v.ptr = hl_to_utf16("Test1");
    
    vdynamic* s2 = hl_alloc_dynamic(&hlt_bytes);
    s2->v.ptr = hl_to_utf16("Test2");
    
    hl_aptr(arr, vdynamic*)[0] = s1;
    hl_aptr(arr, vdynamic*)[1] = s2;
    
    /* Wrap varray in hlffi_value */
    hlffi_value wrapped;
    wrapped.hl_value = (vdynamic*)arr;
    wrapped.is_rooted = false;
    
    /* Pass to Haxe */
    printf("Passing RAW varray (not wrapped as ArrayObj)...\n");
    hlffi_value* args[] = {&wrapped};
    hlffi_call_static(vm, "Arrays", "printStringArray", 1, args);
    
    hlffi_destroy(vm);
    return 0;
}
