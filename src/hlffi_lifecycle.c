/**
 * HLFFI Lifecycle Implementation
 * VM creation, initialization, destruction
 * Phase 1 implementation
 *
 * VM RESTART SUPPORT (Experimental):
 * This file contains static flags (g_hl_globals_initialized, g_main_thread_registered)
 * that enable VM restart within a single process. HashLink wasn't designed for this,
 * but we work around it by ensuring global init and thread registration happen only
 * once per process. See docs/VM_RESTART.md for details and limitations.
 */

#include "hlffi_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Use hlffi_set_error from internal header, create local alias */
#define set_error hlffi_set_error

static hl_code* load_code_from_file(const char* path, char** error_msg) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        if (error_msg) *error_msg = "Failed to open file";
        return NULL;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    int size = (int)ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Read file data */
    char* fdata = (char*)malloc(size);
    if (!fdata) {
        fclose(f);
        if (error_msg) *error_msg = "Out of memory";
        return NULL;
    }

    int pos = 0;
    while (pos < size) {
        int r = (int)fread(fdata + pos, 1, size - pos, f);
        if (r <= 0) {
            free(fdata);
            fclose(f);
            if (error_msg) *error_msg = "Failed to read file";
            return NULL;
        }
        pos += r;
    }
    fclose(f);

    /* Parse bytecode */
    hl_code* code = hl_code_read((unsigned char*)fdata, size, error_msg);
    free(fdata);

    return code;
}

/* ========== LIFECYCLE FUNCTIONS ========== */

hlffi_vm* hlffi_create(void) {
    hlffi_vm* vm = (hlffi_vm*)calloc(1, sizeof(hlffi_vm));
    if (!vm) return NULL;

    /* Initialize to safe defaults */
    vm->module = NULL;
    vm->code = NULL;
    vm->integration_mode = HLFFI_MODE_NON_THREADED;
    vm->stack_context = vm; /* Use vm pointer itself as stack marker */
    vm->last_error = HLFFI_OK;
    vm->hl_initialized = false;
    vm->thread_registered = false;
    vm->module_loaded = false;
    vm->entry_called = false;
    vm->hot_reload_enabled = false;
    vm->loaded_file = NULL;
    vm->error_msg[0] = '\0';

    return vm;
}

/* Track if HashLink globals have been initialized (process-wide) */
static bool g_hl_globals_initialized = false;
/* Track if main thread is registered (process-wide, for restart support) */
static bool g_main_thread_registered = false;

hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv) {
    (void)argc; (void)argv;  /* Currently unused but reserved for future use */
    if (!vm) return HLFFI_ERROR_NULL_VM;

    if (vm->hl_initialized) {
        set_error(vm, HLFFI_ERROR_ALREADY_INITIALIZED, "VM already initialized");
        return HLFFI_ERROR_ALREADY_INITIALIZED;
    }

    /* Initialize HashLink global state (only once per process) */
    if (!g_hl_globals_initialized) {
        hl_global_init();
        g_hl_globals_initialized = true;
    }

    /* NOTE: hl_setup is not accessible from libhl.dll due to export issues
     * Command line arguments can be passed via other mechanisms if needed
     * For now, we skip this - it's not critical for basic VM operation
     */

    /* Initialize system (file I/O, etc.) */
    hl_sys_init();

    /* Register this thread with HashLink GC (only once per process)
     *
     * IMPORTANT: We pass NULL to let HashLink handle stack_top internally.
     * Previously we passed &vm->stack_context (a heap address) which caused
     * GC crashes when using timers/callbacks because GC scanned invalid memory.
     *
     * When integrating with engines (UE, Unity), the host should call
     * hlffi_update_stack_top() at the start of each tick/update to set
     * stack_top to a valid stack address for proper GC scanning.
     *
     * For VM restart support, we only register once since HashLink
     * doesn't support unregister/re-register cleanly.
     */
    if (!g_main_thread_registered) {
        hl_register_thread(NULL);
        g_main_thread_registered = true;
    }

    vm->hl_initialized = true;
    vm->thread_registered = true;

    set_error(vm, HLFFI_OK, NULL);
    return HLFFI_OK;
}

hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path) {
    if (!vm) return HLFFI_ERROR_NULL_VM;
    if (!path) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Null file path");
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    if (!vm->hl_initialized) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "VM not initialized - call hlffi_init first");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    if (vm->module_loaded) {
        set_error(vm, HLFFI_ERROR_ALREADY_INITIALIZED, "Module already loaded");
        return HLFFI_ERROR_ALREADY_INITIALIZED;
    }

    /* Load bytecode from file */
    char* error_msg = NULL;
    vm->code = load_code_from_file(path, &error_msg);
    if (!vm->code) {
        set_error(vm, HLFFI_ERROR_FILE_NOT_FOUND,
                  error_msg ? error_msg : "Failed to load bytecode");
        return HLFFI_ERROR_FILE_NOT_FOUND;
    }

    /* NOTE: hl_setup.file_path is not accessible - skipping */

    /* Allocate module */
    vm->module = hl_module_alloc(vm->code);
    if (!vm->module) {
        hl_code_free(vm->code);
        vm->code = NULL;
        set_error(vm, HLFFI_ERROR_MODULE_INIT_FAILED, "Failed to allocate module");
        return HLFFI_ERROR_MODULE_INIT_FAILED;
    }

    /* Initialize module (JIT compilation happens here) */
    if (!hl_module_init(vm->module, vm->hot_reload_enabled)) {
        hl_module_free(vm->module);
        hl_code_free(vm->code);
        vm->module = NULL;
        vm->code = NULL;
        set_error(vm, HLFFI_ERROR_MODULE_INIT_FAILED, "Failed to initialize module");
        return HLFFI_ERROR_MODULE_INIT_FAILED;
    }

    /* Can free code after module init (module has its own copy) */
    hl_code_free(vm->code);
    vm->code = NULL;

    vm->module_loaded = true;
    vm->loaded_file = path;

    set_error(vm, HLFFI_OK, NULL);
    return HLFFI_OK;
}

hlffi_error_code hlffi_load_memory(hlffi_vm* vm, const void* data, size_t size) {
    if (!vm) return HLFFI_ERROR_NULL_VM;
    if (!data || size == 0) {
        set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Null data or zero size");
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    if (!vm->hl_initialized) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "VM not initialized - call hlffi_init first");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    if (vm->module_loaded) {
        set_error(vm, HLFFI_ERROR_ALREADY_INITIALIZED, "Module already loaded");
        return HLFFI_ERROR_ALREADY_INITIALIZED;
    }

    /* Parse bytecode from memory */
    char* error_msg = NULL;
    vm->code = hl_code_read((const unsigned char*)data, (int)size, &error_msg);
    if (!vm->code) {
        set_error(vm, HLFFI_ERROR_INVALID_BYTECODE,
                  error_msg ? error_msg : "Failed to parse bytecode");
        return HLFFI_ERROR_INVALID_BYTECODE;
    }

    /* Allocate module */
    vm->module = hl_module_alloc(vm->code);
    if (!vm->module) {
        hl_code_free(vm->code);
        vm->code = NULL;
        set_error(vm, HLFFI_ERROR_MODULE_INIT_FAILED, "Failed to allocate module");
        return HLFFI_ERROR_MODULE_INIT_FAILED;
    }

    /* Initialize module (JIT compilation happens here) */
    if (!hl_module_init(vm->module, vm->hot_reload_enabled)) {
        hl_module_free(vm->module);
        hl_code_free(vm->code);
        vm->module = NULL;
        vm->code = NULL;
        set_error(vm, HLFFI_ERROR_MODULE_INIT_FAILED, "Failed to initialize module");
        return HLFFI_ERROR_MODULE_INIT_FAILED;
    }

    /* Can free code after module init */
    hl_code_free(vm->code);
    vm->code = NULL;

    vm->module_loaded = true;

    set_error(vm, HLFFI_OK, NULL);
    return HLFFI_OK;
}

hlffi_error_code hlffi_call_entry(hlffi_vm* vm) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    if (!vm->module_loaded) {
        set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "No module loaded - call hlffi_load_file first");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /* Get entry point function */
    hl_code* code = vm->module->code;
    int entry_index = code->entrypoint;

    /* Setup closure for entry point call */
    vclosure cl;
    cl.t = code->functions[vm->module->functions_indexes[entry_index]].type;
    cl.fun = vm->module->functions_ptrs[entry_index];
    cl.hasValue = 0;

    /* Call entry point with exception handling */
    bool isExc = false;
    vdynamic* ret = hl_dyn_call_safe(&cl, NULL, 0, &isExc);

    if (isExc) {
        /* Exception occurred */
        set_error(vm, HLFFI_ERROR_EXCEPTION_THROWN, "Exception in entry point");
        /* Print exception info to stderr */
        hl_print_uncaught_exception(ret);
        return HLFFI_ERROR_EXCEPTION_THROWN;
    }

    vm->entry_called = true;

    set_error(vm, HLFFI_OK, NULL);
    return HLFFI_OK;
}

void hlffi_destroy(hlffi_vm* vm) {
    if (!vm) return;

    /* Free module if loaded */
    if (vm->module) {
        hl_module_free(vm->module);
        vm->module = NULL;
    }

    /* Free code if still allocated (shouldn't be, but be safe) */
    if (vm->code) {
        hl_code_free(vm->code);
        vm->code = NULL;
    }

    /* NOTE: Do NOT call hl_unregister_thread() or hl_global_free()
     * This matches HashLink's own behavior in main.c
     * The comment in main.c says:
     * "do not call hl_unregister_thread() or hl_global_free will display error
     *  on global_lock if there are threads that are still running"
     *
     * In practice, cleanup only works reliably at process exit.
     */

    /* Free VM structure */
    free(vm);
}

const char* hlffi_get_error(hlffi_vm* vm) {
    if (!vm) return "NULL VM";
    return vm->error_msg[0] ? vm->error_msg : "No error";
}

void hlffi_update_stack_top(void* stack_marker) {
    /* Get current thread info */
    hl_thread_info* t = hl_get_thread();
    if (!t) return;  /* Thread not registered */

    /* Update stack_top to point to the caller's stack frame.
     * This is critical for proper GC root scanning.
     * The GC scans from stack_top down to the current stack pointer.
     * If stack_top points to heap memory, GC scanning is incorrect.
     */
    t->stack_top = stack_marker;
}

void hlffi_gc_block(void) {
    /* Mark thread as blocked (not executing HashLink code).
     * GC will not wait for this thread during collection.
     * See: https://github.com/HaxeFoundation/hashlink/issues/752
     */
    hl_blocking(true);
}

void hlffi_gc_unblock(void) {
    /* Mark thread as unblocked (actively executing HashLink code).
     * Must be balanced with hlffi_gc_block().
     */
    hl_blocking(false);
}
