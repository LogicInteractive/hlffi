/**
 * Complete test - loads and executes Haxe bytecode using HashLink
 *
 * Build: gcc -o test_hello test_hello.c -Ivendor/hashlink/src -Lbin -lhl -ldl -lm -lpthread
 * Run: ./test_hello test/hello.hl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hl.h>
#include <hlmodule.h>

// Read entire file into memory
static unsigned char* read_file(const char* filename, int* size) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open file: %s\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char* data = malloc(*size);
    if (!data) {
        fprintf(stderr, "ERROR: Cannot allocate %d bytes\n", *size);
        fclose(f);
        return NULL;
    }

    int read_bytes = fread(data, 1, *size, f);
    fclose(f);

    if (read_bytes != *size) {
        fprintf(stderr, "ERROR: Read %d bytes, expected %d\n", read_bytes, *size);
        free(data);
        return NULL;
    }

    return data;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    const char* hl_file = argv[1];
    printf("HashLink Bytecode Test\n");
    printf("======================\n\n");

    // Show HashLink version
    int major = (HL_VERSION >> 16) & 0xFF;
    int minor = (HL_VERSION >> 8) & 0xFF;
    int patch = HL_VERSION & 0xFF;
    printf("HashLink version: %d.%d.%d\n", major, minor, patch);
    printf("Loading: %s\n\n", hl_file);

    // Step 1: Initialize HashLink runtime
    printf("[1/5] Initializing HashLink runtime...\n");
    hl_global_init();
    hl_sys_init();

    void* stack_top;
    hl_register_thread(&stack_top);
    printf("      ✓ Runtime initialized\n\n");

    // Step 2: Read bytecode file
    printf("[2/5] Reading bytecode file...\n");
    int file_size = 0;
    unsigned char* bytecode = read_file(hl_file, &file_size);
    if (!bytecode) {
        return 1;
    }
    printf("      ✓ Read %d bytes\n\n", file_size);

    // Step 3: Parse bytecode
    printf("[3/5] Parsing bytecode...\n");
    char* error_msg = NULL;
    hl_code* code = hl_code_read(bytecode, file_size, &error_msg);

    if (!code) {
        fprintf(stderr, "ERROR: Failed to parse bytecode\n");
        if (error_msg) {
            fprintf(stderr, "       %s\n", error_msg);
        }
        free(bytecode);
        return 1;
    }

    printf("      ✓ Bytecode parsed\n");
    printf("        Functions: %d\n", code->nfunctions);
    printf("        Globals: %d\n", code->nglobals);
    printf("        Types: %d\n\n", code->ntypes);

    // Step 4: Initialize module
    printf("[4/5] Initializing module...\n");
    hl_module* module = hl_module_alloc(code);
    if (!module) {
        fprintf(stderr, "ERROR: Failed to allocate module\n");
        free(bytecode);
        return 1;
    }

    if (!hl_module_init(module, 0)) {
        fprintf(stderr, "ERROR: Failed to initialize module\n");
        free(bytecode);
        return 1;
    }
    printf("      ✓ Module initialized\n\n");

    // Step 5: Execute entry point (main function)
    printf("[5/5] Executing entry point...\n");
    printf("════════════════════════════════════════════════\n");

    // Call main function directly (main() has signature: void -> void)
    void (*entry_point)(void) = (void (*)(void))module->functions_ptrs[code->entrypoint];
    entry_point();

    printf("════════════════════════════════════════════════\n");
    printf("      ✓ Execution completed\n\n");

    // Cleanup
    free(bytecode);

    printf("✓ Test completed successfully!\n");
    printf("\nThe Haxe bytecode executed correctly using libhl.a\n");

    return 0;
}
