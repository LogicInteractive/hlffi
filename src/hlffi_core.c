/**
 * HLFFI Core Implementation
 * Version information and utilities
 */

#include "../include/hlffi.h"
#include <hl.h>
#include <stdio.h>
#include <string.h>

/* ========== VERSION ========== */

const char* hlffi_get_version(void) {
    return HLFFI_VERSION_STRING;
}

const char* hlffi_get_hl_version(void) {
    /* Return HashLink version from HL_VERSION macro */
    static char version_buf[32];
    int major = (HL_VERSION >> 16) & 0xFF;
    int minor = (HL_VERSION >> 8) & 0xFF;
    int patch = HL_VERSION & 0xFF;
    snprintf(version_buf, sizeof(version_buf), "%d.%d.%d", major, minor, patch);
    return version_buf;
}

bool hlffi_is_jit_mode(void) {
    /* JIT mode is the default, HL/C mode is when HLML_JIT is not defined */
#ifdef HL_JIT
    return true;
#else
    /* HL/C mode */
    return false;
#endif
}

/* ========== ERROR CODES ========== */

const char* hlffi_get_error_string(hlffi_error_code code) {
    switch (code) {
        case HLFFI_ERROR_NONE:
            return "No error";

        /* VM lifecycle errors */
        case HLFFI_ERROR_NULL_VM:
            return "NULL VM pointer";
        case HLFFI_ERROR_ALREADY_INITIALIZED:
            return "VM already initialized";
        case HLFFI_ERROR_NOT_INITIALIZED:
            return "VM not initialized";
        case HLFFI_ERROR_MODULE_INIT_FAILED:
            return "Module initialization failed";

        /* File I/O errors */
        case HLFFI_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case HLFFI_ERROR_FILE_READ_ERROR:
            return "File read error";

        /* Bytecode errors */
        case HLFFI_ERROR_BYTECODE_INVALID:
            return "Invalid bytecode";
        case HLFFI_ERROR_BYTECODE_VERSION_MISMATCH:
            return "Bytecode version mismatch";

        /* Runtime errors */
        case HLFFI_ERROR_EXCEPTION:
            return "Exception occurred";
        case HLFFI_ERROR_STACK_OVERFLOW:
            return "Stack overflow";
        case HLFFI_ERROR_OUT_OF_MEMORY:
            return "Out of memory";

        /* Call errors */
        case HLFFI_ERROR_FUNCTION_NOT_FOUND:
            return "Function not found";
        case HLFFI_ERROR_INVALID_SIGNATURE:
            return "Invalid function signature";
        case HLFFI_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";
        case HLFFI_ERROR_ARGUMENT_COUNT_MISMATCH:
            return "Argument count mismatch";

        /* Type errors */
        case HLFFI_ERROR_TYPE_NOT_FOUND:
            return "Type not found";
        case HLFFI_ERROR_TYPE_MISMATCH:
            return "Type mismatch";
        case HLFFI_ERROR_CAST_FAILED:
            return "Cast failed";

        /* Field/Method errors */
        case HLFFI_ERROR_FIELD_NOT_FOUND:
            return "Field not found";
        case HLFFI_ERROR_METHOD_NOT_FOUND:
            return "Method not found";

        /* Object errors */
        case HLFFI_ERROR_NULL_REFERENCE:
            return "Null reference";
        case HLFFI_ERROR_ALLOCATION_FAILED:
            return "Allocation failed";

        /* Threading errors */
        case HLFFI_ERROR_THREAD_NOT_REGISTERED:
            return "Thread not registered";
        case HLFFI_ERROR_THREAD_ALREADY_REGISTERED:
            return "Thread already registered";
        case HLFFI_ERROR_WRONG_THREAD:
            return "Called from wrong thread";

        /* Hot reload errors */
        case HLFFI_ERROR_RELOAD_NOT_ENABLED:
            return "Hot reload not enabled";
        case HLFFI_ERROR_RELOAD_FAILED:
            return "Hot reload failed";

        /* General errors */
        case HLFFI_ERROR_NOT_IMPLEMENTED:
            return "Not implemented";
        case HLFFI_ERROR_INTERNAL:
            return "Internal error";
        case HLFFI_ERROR_UNKNOWN:
            return "Unknown error";

        default:
            return "Invalid error code";
    }
}
