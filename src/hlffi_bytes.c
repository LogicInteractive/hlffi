/**
 * hlffi_bytes.c
 * Phase 5: Bytes Support
 *
 * Implements binary data operations for hl.Bytes and haxe.io.Bytes
 * Provides zero-copy access to byte buffers between C and Haxe
 */

#include "hlffi_internal.h"
#include <string.h>

/* HashLink bytes functions are available via hl.h included by hlffi_internal.h */

/* ========== BYTES CREATION ========== */

/**
 * Create new hl.Bytes buffer by calling Haxe.
 * NOTE: For now, create bytes in Haxe and pass to C.
 * Direct creation from C will be added in a future version.
 */
hlffi_value* hlffi_bytes_new(hlffi_vm* vm, int size) {
    if (!vm || size < 0) return NULL;

    /* For now, bytes should be created in Haxe and passed to C.
     * Creating raw vbyte* from C requires proper type wrapping. */
    return NULL;
}

/**
 * Create bytes from C buffer (copies data).
 * NOTE: For now, create bytes in Haxe and pass to C.
 */
hlffi_value* hlffi_bytes_from_data(hlffi_vm* vm, const void* data, int size) {
    if (!vm || !data || size < 0) return NULL;

    /* For now, not implemented - create bytes in Haxe instead */
    return NULL;
}

/**
 * Create bytes from UTF-8 string.
 * NOTE: For now, create bytes in Haxe and pass to C.
 */
hlffi_value* hlffi_bytes_from_string(hlffi_vm* vm, const char* str) {
    if (!vm || !str) return NULL;

    /* For now, not implemented - create bytes in Haxe instead */
    return NULL;
}

/* ========== BYTES ACCESS ========== */

/**
 * Get direct pointer to bytes data (zero-copy).
 * WARNING: Pointer is only valid while bytes are alive!
 */
void* hlffi_bytes_get_ptr(hlffi_value* bytes) {
    if (!bytes || !bytes->hl_value) return NULL;

    vdynamic* val = bytes->hl_value;

    /* Check type */
    if (!val->t) return NULL;

    /* For hl.Bytes (HBYTES type), the vbyte* is in val->v.bytes */
    if (val->t->kind == HBYTES) {
        return (void*)val->v.bytes;
    }

    /* For haxe.io.Bytes (HOBJ), get the "b" field containing the raw vbyte* */
    if (val->t->kind == HOBJ) {
        int b_hash = hl_hash_utf8("b");
        hl_field_lookup* lookup = obj_resolve_field(val->t->obj, b_hash);

        if (lookup && lookup->t->kind == HBYTES) {
            vdynamic* bytes_dyn = (vdynamic*)hl_dyn_getp(val, b_hash, lookup->t);
            if (bytes_dyn && bytes_dyn->v.bytes) {
                return (void*)bytes_dyn->v.bytes;
            }
        }
    }

    return NULL;
}

/**
 * Get length of bytes buffer.
 * NOTE: Raw vbyte* doesn't store length - this only works for
 * haxe.io.Bytes objects that have a length field.
 * For raw bytes from C, you must track length separately!
 */
int hlffi_bytes_get_length(hlffi_value* bytes) {
    if (!bytes || !bytes->hl_value) return -1;

    vdynamic* val = bytes->hl_value;

    /* Check if this is a Bytes object (HOBJ) with a length field */
    if (val->t->kind == HOBJ) {
        /* Try to get "length" field */
        int length_hash = hl_hash_utf8("length");
        hl_field_lookup* lookup = obj_resolve_field(val->t->obj, length_hash);

        if (lookup && lookup->t->kind == HI32) {
            return hl_dyn_geti(val, length_hash, lookup->t);
        }
    }

    /* Raw vbyte* has no length info */
    return -1;
}

/* ========== BYTES OPERATIONS ========== */

/**
 * Copy bytes from src to dst (blit operation).
 * Handles overlapping regions correctly.
 */
bool hlffi_bytes_blit(hlffi_value* dst, int dst_pos, hlffi_value* src, int src_pos, int len) {
    if (!dst || !src || len < 0) return false;
    if (dst_pos < 0 || src_pos < 0) return false;

    vbyte* dst_bytes = (vbyte*)hlffi_bytes_get_ptr(dst);
    vbyte* src_bytes = (vbyte*)hlffi_bytes_get_ptr(src);

    if (!dst_bytes || !src_bytes) return false;

    /* Use memmove to handle overlapping regions safely */
    memmove(dst_bytes + dst_pos, src_bytes + src_pos, len);
    return true;
}

/**
 * Compare bytes regions.
 * Returns: <0 if a < b, 0 if equal, >0 if a > b
 */
int hlffi_bytes_compare(hlffi_value* a, int a_pos, hlffi_value* b, int b_pos, int len) {
    if (!a || !b || len < 0) return 0;
    if (a_pos < 0 || b_pos < 0) return 0;

    vbyte* a_bytes = (vbyte*)hlffi_bytes_get_ptr(a);
    vbyte* b_bytes = (vbyte*)hlffi_bytes_get_ptr(b);

    if (!a_bytes || !b_bytes) return 0;

    return memcmp(a_bytes + a_pos, b_bytes + b_pos, len);
}

/* ========== BYTES CONVERSION ========== */

/**
 * Convert bytes to UTF-8 string.
 * Assumes bytes contain UTF-8 text.
 * Returns malloc'd string - caller must free()!
 */
char* hlffi_bytes_to_string(hlffi_value* bytes, int length) {
    if (!bytes || length < 0) return NULL;

    vbyte* data = (vbyte*)hlffi_bytes_get_ptr(bytes);
    if (!data) return NULL;

    /* Allocate string buffer (+ null terminator) */
    char* str = (char*)malloc(length + 1);
    if (!str) return NULL;

    /* Copy bytes to string */
    memcpy(str, data, length);
    str[length] = '\0';

    return str;
}

/**
 * Get/Set individual bytes.
 */
int hlffi_bytes_get(hlffi_value* bytes, int index) {
    if (!bytes || index < 0) return -1;

    vbyte* data = (vbyte*)hlffi_bytes_get_ptr(bytes);
    if (!data) return -1;

    /* NOTE: No bounds checking! Caller must ensure index is valid */
    return (int)data[index];
}

bool hlffi_bytes_set(hlffi_value* bytes, int index, int value) {
    if (!bytes || index < 0) return false;
    if (value < 0 || value > 255) return false;

    vbyte* data = (vbyte*)hlffi_bytes_get_ptr(bytes);
    if (!data) return false;

    /* NOTE: No bounds checking! Caller must ensure index is valid */
    data[index] = (vbyte)value;
    return true;
}

/**
 * Fill bytes with a value.
 */
bool hlffi_bytes_fill(hlffi_value* bytes, int pos, int len, int value) {
    if (!bytes || pos < 0 || len < 0) return false;
    if (value < 0 || value > 255) return false;

    vbyte* data = (vbyte*)hlffi_bytes_get_ptr(bytes);
    if (!data) return false;

    memset(data + pos, value, len);
    return true;
}
