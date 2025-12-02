/**
 * Direct HashLink test - loads and runs Haxe bytecode using HashLink API
 *
 * This tests that libhl.a builds correctly and can execute Haxe bytecode.
 *
 * Build: gcc -o test_hashlink test_hashlink_direct.c \
 *            -Ivendor/hashlink/src -Lbin -lhl -ldl -lm -lpthread
 * Run: ./test_hashlink test/hello.hl
 */

#include <stdio.h>
#include <stdlib.h>
#include <hl.h>
#include <hlmodule.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];
    printf("HashLink Direct Test\n");
    printf("====================\n\n");

    // Show HashLink version
    int major = (HL_VERSION >> 16) & 0xFF;
    int minor = (HL_VERSION >> 8) & 0xFF;
    int patch = HL_VERSION & 0xFF;
    printf("HashLink version: %d.%d.%d\n", major, minor, patch);
    printf("Loading bytecode: %s\n\n", hl_file);

    // Initialize HashLink runtime
    printf("[1/5] Initializing HashLink runtime...\n");
    hl_global_init();

    // Set up command-line arguments
    hl_setup.sys_args = argv;
    hl_setup.sys_nargs = argc;

    hl_sys_init();

    // Register main thread for GC
    void* stack_top;
    hl_register_thread(&stack_top);
    printf("      Runtime initialized\n\n");

    // Load bytecode
    printf("[2/5] Loading bytecode...\n");
    hl_code* code = hl_code_read((const uchar*)hl_file, NULL, NULL);
    if (!code) {
        fprintf(stderr, "ERROR: Failed to load bytecode from %s\n", hl_file);
        return 1;
    }
    printf("      Bytecode loaded (%d functions, %d globals)\n\n",
           code->nfunctions, code->nglobals);

    // Allocate and initialize module
    printf("[3/5] Initializing module...\n");
    hl_module* module = hl_module_alloc(code);
    if (!module) {
        fprintf(stderr, "ERROR: Failed to allocate module\n");
        return 1;
    }

    if (hl_module_init(module, NULL) < 0) {
        fprintf(stderr, "ERROR: Failed to initialize module\n");
        return 1;
    }
    printf("      Module initialized\n\n");

    // Find and call entry point
    printf("[4/5] Calling entry point...\n");
    printf("--------------------------------------------------\n");

    vdynamic* exception = NULL;
    int result = hl_module_context(module, NULL, &exception);

    printf("--------------------------------------------------\n");

    if (exception) {
        fprintf(stderr, "ERROR: Exception occurred during execution\n");
        // Try to print exception message if available
        vdynamic_obj* exc_obj = (vdynamic_obj*)exception;
        if (exc_obj && exc_obj->t->kind == HDYN) {
            fprintf(stderr, "       Exception: %s\n", hl_to_string(exception));
        }
        return 1;
    }

    printf("      Entry point completed (result=%d)\n\n", result);

    // Cleanup
    printf("[5/5] Cleaning up...\n");
    // Note: HashLink doesn't provide cleanup functions
    // Memory will be freed on process exit
    printf("      Cleanup complete\n\n");

    printf("✓ Test completed successfully!\n");
    return 0;
}
