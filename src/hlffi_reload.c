/**
 * HLFFI Hot Reload Implementation
 * Runtime code reloading without restart
 * Phase 1 implementation
 *
 * Uses HashLink's built-in hot reload support:
 * - hl_module_init(m, true) enables hot reload mode
 * - hl_module_patch(m, new_code) patches the running module
 */

#include "hlffi_internal.h"
#include <stdio.h>
#include <stdlib.h>

/* Forward declaration for bytecode loading */
static hl_code* load_code_from_file(const char* path, char** error_msg);

/* ========== HOT RELOAD API ========== */

hlffi_error_code hlffi_enable_hot_reload(hlffi_vm* vm, bool enable) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    /* Can only enable before module is loaded */
    if (vm->module_loaded) {
        hlffi_set_error(vm, HLFFI_ERROR_ALREADY_INITIALIZED,
                       "Cannot change hot reload after module is loaded");
        return HLFFI_ERROR_ALREADY_INITIALIZED;
    }

    vm->hot_reload_enabled = enable;
    hlffi_set_error(vm, HLFFI_OK, NULL);
    return HLFFI_OK;
}

bool hlffi_is_hot_reload_enabled(hlffi_vm* vm) {
    if (!vm) return false;
    return vm->hot_reload_enabled;
}

hlffi_error_code hlffi_reload_module(hlffi_vm* vm, const char* path) {
    if (!vm) return HLFFI_ERROR_NULL_VM;

    if (!vm->module_loaded) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "No module loaded");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    if (!vm->hot_reload_enabled) {
        hlffi_set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT,
                       "Hot reload not enabled - call hlffi_enable_hot_reload() before loading");
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    /* Use the original loaded file if no path specified */
    const char* reload_path = path ? path : vm->loaded_file;
    if (!reload_path) {
        hlffi_set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "No file path for reload");
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    /* Load new bytecode */
    char* error_msg = NULL;
    hl_code* new_code = load_code_from_file(reload_path, &error_msg);
    if (!new_code) {
        hlffi_set_error(vm, HLFFI_ERROR_FILE_NOT_FOUND,
                       error_msg ? error_msg : "Failed to load bytecode for reload");
        return HLFFI_ERROR_FILE_NOT_FOUND;
    }

    /* Patch the running module */
    bool changed = hl_module_patch(vm->module, new_code);

    /* Free the code (hl_module_patch copies what it needs) */
    hl_code_free(new_code);

    /* Call reload callback if registered */
    if (vm->reload_callback) {
        vm->reload_callback(vm, changed, vm->reload_userdata);
    }

    hlffi_set_error(vm, HLFFI_OK, NULL);
    return HLFFI_OK;
}

hlffi_error_code hlffi_reload_module_memory(hlffi_vm* vm, const void* data, size_t size) {
    if (!vm) return HLFFI_ERROR_NULL_VM;
    if (!data || size == 0) {
        hlffi_set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT, "Invalid bytecode data");
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    if (!vm->module_loaded) {
        hlffi_set_error(vm, HLFFI_ERROR_NOT_INITIALIZED, "No module loaded");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    if (!vm->hot_reload_enabled) {
        hlffi_set_error(vm, HLFFI_ERROR_INVALID_ARGUMENT,
                       "Hot reload not enabled - call hlffi_enable_hot_reload() before loading");
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    /* Parse bytecode from memory */
    char* error_msg = NULL;
    hl_code* new_code = hl_code_read((const unsigned char*)data, (int)size, &error_msg);
    if (!new_code) {
        hlffi_set_error(vm, HLFFI_ERROR_INVALID_BYTECODE,
                       error_msg ? error_msg : "Failed to parse bytecode for reload");
        return HLFFI_ERROR_INVALID_BYTECODE;
    }

    /* Patch the running module */
    bool changed = hl_module_patch(vm->module, new_code);

    /* Free the code */
    hl_code_free(new_code);

    /* Call reload callback if registered */
    if (vm->reload_callback) {
        vm->reload_callback(vm, changed, vm->reload_userdata);
    }

    hlffi_set_error(vm, HLFFI_OK, NULL);
    return HLFFI_OK;
}

void hlffi_set_reload_callback(hlffi_vm* vm, hlffi_reload_callback callback, void* userdata) {
    if (!vm) return;
    vm->reload_callback = callback;
    vm->reload_userdata = userdata;
}

bool hlffi_check_reload(hlffi_vm* vm) {
    if (!vm) return false;
    if (!vm->hot_reload_enabled || !vm->module_loaded) return false;
    if (!vm->loaded_file) return false;

    /* Check file modification time */
    FILE* f = fopen(vm->loaded_file, "rb");
    if (!f) return false;

    /* Get current file time (simple approach: use file size as proxy) */
    fseek(f, 0, SEEK_END);
    int current_time = (int)ftell(f);
    fclose(f);

    if (current_time != vm->file_time && vm->file_time != 0) {
        /* File changed - trigger reload */
        hlffi_error_code result = hlffi_reload_module(vm, NULL);
        if (result == HLFFI_OK) {
            vm->file_time = current_time;
            return true;
        }
    }

    return false;
}

/* ========== INTERNAL HELPERS ========== */

/**
 * Load bytecode from file (same as in hlffi_lifecycle.c)
 */
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
