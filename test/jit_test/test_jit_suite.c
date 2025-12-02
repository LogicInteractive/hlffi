/**
 * HLFFI JIT Mode Test Suite
 *
 * Comprehensive test runner that loads and executes multiple .hl bytecode files.
 * Each test validates different aspects of HLFFI functionality.
 */

#include <hlffi.h>
#include <stdio.h>
#include <string.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Run a single .hl test file */
static int run_test(const char* test_name, const char* hl_file) {
    printf("\n--- Test: %s ---\n", test_name);
    printf("Loading: %s\n", hl_file);

    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        printf("FAIL: Could not create VM\n");
        return 0;
    }

    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        printf("FAIL: Could not init VM: %s\n", hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 0;
    }

    err = hlffi_load_file(vm, hl_file);
    if (err != HLFFI_OK) {
        printf("FAIL: Could not load file: %s\n", hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 0;
    }

    printf("----------------------------------------\n");
    err = hlffi_call_entry(vm);
    printf("----------------------------------------\n");

    if (err != HLFFI_OK) {
        printf("FAIL: Entry point error: %s\n", hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 0;
    }

    hlffi_destroy(vm);
    printf("PASS: %s\n", test_name);
    return 1;
}

int main(int argc, char** argv) {
    printf("==============================================\n");
    printf("  HLFFI JIT Mode Test Suite\n");
    printf("==============================================\n\n");

    printf("HLFFI Version: %s\n", hlffi_get_version());
    printf("HashLink Version: %s\n", hlffi_get_hl_version());

#ifdef HLFFI_HLC_MODE
    printf("\nERROR: This test requires JIT mode!\n");
    printf("Build with -DHLFFI_HLC_MODE=OFF\n");
    return 1;
#endif

    /* Determine test directory from executable path or use default */
    const char* test_dir = "..\\..\\..\\test\\";
    if (argc > 1) {
        test_dir = argv[1];
    }

    char path[512];

    /* Test 1: Basic VM (hello.hl) */
    snprintf(path, sizeof(path), "%shello.hl", test_dir);
    if (run_test("Basic VM Operations", path)) tests_passed++; else tests_failed++;

    /* Test 2: Static fields (static_test.hl) */
    snprintf(path, sizeof(path), "%sstatic_test.hl", test_dir);
    if (run_test("Static Fields & Methods", path)) tests_passed++; else tests_failed++;

    /* Test 3: Arrays (arrays.hl) */
    snprintf(path, sizeof(path), "%sarrays.hl", test_dir);
    if (run_test("Array Operations", path)) tests_passed++; else tests_failed++;

    /* Test 4: Callbacks (callbacks.hl) */
    snprintf(path, sizeof(path), "%scallbacks.hl", test_dir);
    if (run_test("Callback System", path)) tests_passed++; else tests_failed++;

    /* Test 5: Maps (map_test.hl) */
    snprintf(path, sizeof(path), "%smap_test.hl", test_dir);
    if (run_test("Map Operations", path)) tests_passed++; else tests_failed++;

    /* Test 6: Bytes (bytes_test.hl) */
    snprintf(path, sizeof(path), "%sbytes_test.hl", test_dir);
    if (run_test("Bytes Operations", path)) tests_passed++; else tests_failed++;

    /* Test 7: Exceptions (exceptions.hl) */
    snprintf(path, sizeof(path), "%sexceptions.hl", test_dir);
    if (run_test("Exception Handling", path)) tests_passed++; else tests_failed++;

    /* Test 8: Player/Instance (player.hl) */
    snprintf(path, sizeof(path), "%splayer.hl", test_dir);
    if (run_test("Instance Objects", path)) tests_passed++; else tests_failed++;

    /* Test 9: Minimal Enum (minimal_enum.hl) */
    snprintf(path, sizeof(path), "%sminimal_enum.hl", test_dir);
    if (run_test("Enum Types", path)) tests_passed++; else tests_failed++;

    /* Test 10: Cache Test (cache_test.hl) */
    snprintf(path, sizeof(path), "%scache_test.hl", test_dir);
    if (run_test("Type Caching", path)) tests_passed++; else tests_failed++;

    /* Summary */
    printf("\n==============================================\n");
    printf("  Test Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("==============================================\n");

    return tests_failed > 0 ? 1 : 0;
}
