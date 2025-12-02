/**
 * Minimal test to isolate cache segfault
 */

#include "hlffi.h"
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cachetest.hl>\n", argv[0]);
        return 1;
    }

    printf("Creating VM...\n");
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    printf("Initializing VM...\n");
    if (hlffi_init(vm, 0, NULL) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM\n");
        return 1;
    }

    printf("Loading bytecode...\n");
    if (hlffi_load_file(vm, argv[1]) != HLFFI_OK) {
        fprintf(stderr, "Failed to load: %s\n", hlffi_get_error(vm));
        return 1;
    }

    printf("Calling entry point...\n");
    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry: %s\n", hlffi_get_error(vm));
        return 1;
    }

    printf("Caching method...\n");
    hlffi_cached_call* cached = hlffi_cache_static_method(vm, "CacheTest", "increment");
    if (!cached) {
        fprintf(stderr, "Failed to cache: %s\n", hlffi_get_error(vm));
        return 1;
    }

    printf("Cache successful!\n");

    printf("Attempting cached call...\n");
    hlffi_value* result = hlffi_call_cached(cached, 0, NULL);
    printf("Call returned: %p\n", result);

    if (result) {
        hlffi_value_free(result);
        printf("Call successful!\n");
    } else {
        printf("Call returned NULL\n");
    }

    printf("Freeing cache...\n");
    hlffi_cached_call_free(cached);

    printf("Destroying VM...\n");
    hlffi_destroy(vm);

    printf("Done!\n");
    return 0;
}
