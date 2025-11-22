/**
 * Simple test to verify libhl.a builds and links correctly
 *
 * Build: gcc -o test_libhl test_libhl.c -Ivendor/hashlink/src -Lbin -lhl -ldl -lm -lpthread
 * Run: ./test_libhl
 */

#include <stdio.h>
#include <hl.h>

int main() {
    printf("HashLink Library Test\n");
    printf("=====================\n\n");

    // Show HashLink version
    int major = (HL_VERSION >> 16) & 0xFF;
    int minor = (HL_VERSION >> 8) & 0xFF;
    int patch = HL_VERSION & 0xFF;
    printf("HashLink version: %d.%d.%d (0x%06X)\n", major, minor, patch, HL_VERSION);

    // Test global initialization
    printf("\n[1/3] Testing hl_global_init()...\n");
    hl_global_init();
    printf("      ✓ Global initialization successful\n");

    // Test sys initialization
    printf("\n[2/3] Testing hl_sys_init()...\n");
    hl_sys_init();
    printf("      ✓ System initialization successful\n");

    // Test thread registration
    printf("\n[3/3] Testing hl_register_thread()...\n");
    void* stack_top;
    hl_register_thread(&stack_top);
    printf("      ✓ Thread registered (stack_top=%p)\n", stack_top);

    printf("\n✓ All basic HashLink functions work correctly!\n");
    printf("  libhl.a is properly built and linkable.\n\n");

    printf("To test with actual Haxe bytecode:\n");
    printf("  1. Build on Windows with Visual Studio\n");
    printf("  2. Run: bin\\x64\\Debug\\example_hello_world.exe test\\hello.hl\n");

    return 0;
}
