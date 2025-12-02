/**
 * Test caching with multiple calls in a loop
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
    if (!vm) return 1;

    printf("Initializing...\n");
    if (hlffi_init(vm, 0, NULL) != HLFFI_OK) return 1;

    printf("Loading bytecode...\n");
    if (hlffi_load_file(vm, argv[1]) != HLFFI_OK) return 1;

    printf("Calling entry...\n");
    if (hlffi_call_entry(vm) != HLFFI_OK) return 1;

    /* Warmup: uncached calls (like benchmark) */
    printf("Warmup: uncached calls (1000x)...\n");
    for (int i = 0; i < 1000; i++) {
        hlffi_value* result = hlffi_call_static(vm, "CacheTest", "increment", 0, NULL);
        if (result) hlffi_value_free(result);
    }
    printf("  Done\n");

    /* Warmup: cached calls (like benchmark) */
    printf("Warmup: cached calls (1000x)...\n");
    hlffi_cached_call* warmup_cache = hlffi_cache_static_method(vm, "CacheTest", "increment");
    if (warmup_cache) {
        for (int i = 0; i < 1000; i++) {
            hlffi_value* result = hlffi_call_cached(warmup_cache, 0, NULL);
            if (result) hlffi_value_free(result);
        }
        hlffi_cached_call_free(warmup_cache);
    }
    printf("  Done\n");

    /* Now do the actual test */
    printf("Caching method...\n");
    hlffi_cached_call* cached = hlffi_cache_static_method(vm, "CacheTest", "increment");
    if (!cached) {
        printf("Failed to cache: %s\n", hlffi_get_error(vm));
        return 1;
    }

    printf("Calling cached method 100,000 times...\n");
    for (int i = 0; i < 100000; i++) {
        hlffi_value* result = hlffi_call_cached(cached, 0, NULL);
        if (result) {
            hlffi_value_free(result);
        }
        if (i % 10000 == 0) {
            printf("  %d calls...\n", i);
        }
    }

    printf("Success!\n");
    hlffi_cached_call_free(cached);
    hlffi_destroy(vm);

    return 0;
}
