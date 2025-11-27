#include "hlffi.h"
#include <stdio.h>

int main(int argc, char** argv) {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, argv[1]);

    printf("=== String Class Details ===\n\n");
    
    hlffi_type* str_type = hlffi_find_type(vm, "String");
    
    printf("Fields:\n");
    int fields = hlffi_class_get_field_count(str_type);
    for (int i = 0; i < fields; i++) {
        const char* name = hlffi_class_get_field_name(str_type, i);
        hlffi_type* type = hlffi_class_get_field_type(str_type, i);
        printf("  %d. %s : %s\n", i+1, name, hlffi_type_get_name(type));
    }
    
    printf("\nMethods:\n");
    int methods = hlffi_class_get_method_count(str_type);
    for (int i = 0; i < methods; i++) {
        const char* name = hlffi_class_get_method_name(str_type, i);
        printf("  %d. %s()\n", i+1, name);
    }
    
    hlffi_destroy(vm);
    return 0;
}
