/**
 * HLFFI Example: Version Information
 * Prints HLFFI and HashLink version information
 */

#include <hlffi.h>
#include <stdio.h>

int main(void) {
    printf("HLFFI Version: %s\n", hlffi_get_version());
    printf("HashLink Version: %s\n", hlffi_get_hl_version());
    printf("JIT Mode: %s\n", hlffi_is_jit_mode() ? "Yes" : "No");

    printf("\nHLFFI v3.0 is ready!\n");
    printf("See MASTER_PLAN.md for implementation roadmap.\n");

    return 0;
}
