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
        case HLFFI_OK:
            return "No error";

        /* VM lifecycle errors */
        case HLFFI_ERROR_NULL_VM:
            return "NULL VM pointer";
        case HLFFI_ERROR_ALREADY_INITIALIZED:
            return "VM already initialized";
        case HLFFI_ERROR_NOT_INITIALIZED:
            return "VM not initialized";

        /* File I/O errors */
        case HLFFI_ERROR_FILE_NOT_FOUND:
            return "File not found";

        /* Bytecode errors */
        case HLFFI_ERROR_INVALID_BYTECODE:
            return "Invalid bytecode";
        case HLFFI_ERROR_MODULE_LOAD_FAILED:
            return "Module load failed";
        case HLFFI_ERROR_MODULE_INIT_FAILED:
            return "Module init failed";

        /* Runtime errors */
        case HLFFI_ERROR_EXCEPTION_THROWN:
            return "Exception occurred";
        case HLFFI_ERROR_CALL_FAILED:
            return "Call failed";
        case HLFFI_ERROR_INIT_FAILED:
            return "Initialization failed";
        case HLFFI_ERROR_DESTROY_FAILED:
            return "Destroy failed";

        /* Call errors */
        case HLFFI_ERROR_ENTRY_POINT_NOT_FOUND:
            return "Entry point not found";
        case HLFFI_ERROR_METHOD_NOT_FOUND:
            return "Method not found";

        /* Type errors */
        case HLFFI_ERROR_TYPE_NOT_FOUND:
            return "Type not found";
        case HLFFI_ERROR_TYPE_MISMATCH:
            return "Type mismatch";
        case HLFFI_ERROR_INVALID_TYPE:
            return "Invalid type";

        /* Field errors */
        case HLFFI_ERROR_FIELD_NOT_FOUND:
            return "Field not found";

        /* Object errors */
        case HLFFI_ERROR_NULL_VALUE:
            return "Null value";

        /* Threading errors */
        case HLFFI_ERROR_THREAD_NOT_STARTED:
            return "Thread not started";
        case HLFFI_ERROR_THREAD_ALREADY_RUNNING:
            return "Thread already running";
        case HLFFI_ERROR_THREAD_START_FAILED:
            return "Thread start failed";
        case HLFFI_ERROR_THREAD_STOP_FAILED:
            return "Thread stop failed";
        case HLFFI_ERROR_WRONG_THREAD:
            return "Wrong thread";

        /* Hot reload errors */
        case HLFFI_ERROR_RELOAD_NOT_SUPPORTED:
            return "Reload not supported";
        case HLFFI_ERROR_RELOAD_NOT_ENABLED:
            return "Reload not enabled";
        case HLFFI_ERROR_RELOAD_FAILED:
            return "Reload failed";

        /* Event loop errors */
        case HLFFI_ERROR_EVENTLOOP_NOT_FOUND:
            return "Event loop not found";

        /* General errors */
        case HLFFI_ERROR_NOT_IMPLEMENTED:
            return "Not implemented";
        case HLFFI_ERROR_UNKNOWN:
            return "Unknown error";

        default:
            return "Invalid error code";
    }
}
