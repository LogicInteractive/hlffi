#include "include/hlffi.h"
#include <stdio.h>

int main(int argc, char** argv) {
    printf("Test starting...\n");
    fflush(stdout);
    
    hlffi_vm* vm = hlffi_create();
    printf("VM: %p\n", (void*)vm);
    fflush(stdout);
    
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    printf("Success!\n");
    hlffi_destroy(vm);
    return 0;
}
