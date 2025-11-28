/**
 * Memory leak test for Phase 7 caching API
 *
 * Tests for memory leaks by creating/using/freeing caches repeatedly.
 * Should show zero leaks when run with valgrind.
 */

#include "hlffi.h"
#include <stdio.h>

#define CYCLES 1000
#define CALLS_PER_CYCLE 100

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <cachetest.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Phase 7: Memory Leak Test ===\n");
    printf("Testing %d cache create/use/free cycles\n", CYCLES);
    printf("with %d calls per cycle...\n\n", CALLS_PER_CYCLE);

    /* Initialize VM once */
    hlffi_vm* vm = hlffi_create();
    if (!vm || hlffi_init(vm, 0, NULL) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM\n");
        return 1;
    }

    if (hlffi_load_file(vm, argv[1]) != HLFFI_OK) {
        fprintf(stderr, "Failed to load bytecode\n");
        hlffi_destroy(vm);
        return 1;
    }

    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry point\n");
        hlffi_destroy(vm);
        return 1;
    }

    /* Test 1: Repeated cache create/free cycles */
    printf("Test 1: Create/use/free cache %d times...\n", CYCLES);
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        /* Cache method */
        hlffi_cached_call* cached = hlffi_cache_static_method(vm, "CacheTest", "increment");
        if (!cached) {
            fprintf(stderr, "Failed to cache method at cycle %d\n", cycle);
            return 1;
        }

        /* Use cached method */
        for (int i = 0; i < CALLS_PER_CYCLE; i++) {
            hlffi_value* result = hlffi_call_cached(cached, 0, NULL);
            if (result) hlffi_value_free(result);
        }

        /* Free cache */
        hlffi_cached_call_free(cached);

        if (cycle % 100 == 0) {
            printf("  %d cycles...\n", cycle);
        }
    }
    printf("  ✓ Completed\n\n");

    /* Test 2: Multiple caches active simultaneously */
    printf("Test 2: Multiple simultaneous caches...\n");
    hlffi_cached_call* cache1 = hlffi_cache_static_method(vm, "CacheTest", "increment");
    hlffi_cached_call* cache2 = hlffi_cache_static_method(vm, "CacheTest", "increment");
    hlffi_cached_call* cache3 = hlffi_cache_static_method(vm, "CacheTest", "increment");

    if (!cache1 || !cache2 || !cache3) {
        fprintf(stderr, "Failed to create multiple caches\n");
        return 1;
    }

    /* Use all caches */
    for (int i = 0; i < 100; i++) {
        hlffi_value* r1 = hlffi_call_cached(cache1, 0, NULL);
        hlffi_value* r2 = hlffi_call_cached(cache2, 0, NULL);
        hlffi_value* r3 = hlffi_call_cached(cache3, 0, NULL);
        if (r1) hlffi_value_free(r1);
        if (r2) hlffi_value_free(r2);
        if (r3) hlffi_value_free(r3);
    }

    /* Free all caches */
    hlffi_cached_call_free(cache1);
    hlffi_cached_call_free(cache2);
    hlffi_cached_call_free(cache3);
    printf("  ✓ Completed\n\n");

    /* Test 3: Cache with arguments */
    printf("Test 3: Cached method with arguments...\n");
    hlffi_cached_call* add_cache = hlffi_cache_static_method(vm, "CacheTest", "add");
    if (!add_cache) {
        fprintf(stderr, "Failed to cache 'add' method\n");
        return 1;
    }

    hlffi_value* args[2];
    args[0] = hlffi_value_int(vm, 10);
    args[1] = hlffi_value_int(vm, 20);

    for (int i = 0; i < 1000; i++) {
        hlffi_value* result = hlffi_call_cached(add_cache, 2, args);
        if (result) hlffi_value_free(result);
    }

    hlffi_value_free(args[0]);
    hlffi_value_free(args[1]);
    hlffi_cached_call_free(add_cache);
    printf("  ✓ Completed\n\n");

    /* Cleanup */
    hlffi_destroy(vm);

    printf("=== Memory Test Summary ===\n");
    printf("Total operations:\n");
    printf("  - %d cache create/free cycles\n", CYCLES);
    printf("  - %d cached method calls\n", CYCLES * CALLS_PER_CYCLE + 300 + 1000);
    printf("  - 3 simultaneous caches tested\n");
    printf("  - Arguments tested\n");
    printf("\n✓ All memory tests completed\n");
    printf("\nRun with: valgrind --leak-check=full ./test_cache_memory test/cachetest.hl\n");

    return 0;
}
