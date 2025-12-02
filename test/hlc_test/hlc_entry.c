/**
 * HLC Entry Point Bridge
 *
 * This file includes the HLC-generated code but skips hlc_main.c
 * to avoid conflicting main functions. We provide our own main in test_hlc.c.
 */

/* Skip the standard HLC bootstrap (hlc_main.c has wmain/wWinMain) */
/* Instead, we directly include hl.h and define what we need */

#include <hl.h>

/* Forward declarations from HLC-generated code */
void fun$init(void);
void hl_init_hashes(void);
void hl_init_roots(void);
void hl_init_types(hl_module_context *ctx);
extern void *hl_functions_ptrs[];
extern hl_type *hl_functions_types[];

/* HLC Entry Point - called by hlffi_call_entry() */
void hl_entry_point(void) {
    hl_module_context ctx;
    hl_alloc_init(&ctx.alloc);
    ctx.functions_ptrs = hl_functions_ptrs;
    ctx.functions_types = hl_functions_types;
    hl_init_types(&ctx);
    hl_init_hashes();
    hl_init_roots();
    fun$init();
}
