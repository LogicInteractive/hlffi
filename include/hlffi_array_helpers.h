/**
 * hlffi_array_helpers.h - Haxe Array<T> ↔ C Array Conversion Helpers
 *
 * Drop-in helpers for HashLink native extensions to work with Haxe arrays.
 *
 * Usage:
 *   #include <hl.h>
 *   #include "hlffi_array_helpers.h"
 *
 * Key Functions:
 *   - haxe_array_get_bytes()   - Extract ArrayBytes_* to raw C array (fast)
 *   - haxe_array_to_varray()   - Extract ArrayObj to varray (for pointers)
 *   - varray_to_haxe_array()   - Wrap varray as Haxe Array<T>
 *   - create_haxe_*_array()    - Create typed arrays from C data
 *   - extract_haxe_*_array()   - Extract typed arrays to C data
 *
 * Battle-tested with HLFFI Phase 5 (10/10 array tests passing).
 *
 * IMPORTANT NOTES:
 * 1. Field offsets MUST use rt->fields_indexes[fid], NOT pointer arithmetic!
 * 2. ArrayObj ≠ ArrayBytes_* - different memory layouts
 * 3. F32 and F64 are distinct types - no casting allowed
 * 4. All returned Haxe arrays are GC-managed - keep rooted if storing
 *
 * License: Same as HLFFI project
 */

#ifndef HLFFI_ARRAY_HELPERS_H
#define HLFFI_ARRAY_HELPERS_H

#include <hl.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Core Conversion Functions ========== */

/**
 * Extract raw data from ArrayBytes_* types (Int, Float, Bool)
 *
 * This is the FAST PATH for primitive arrays - gives direct memory access.
 *
 * @param haxe_array - Haxe Array<Int|Float|Single|Bool>
 * @param out_data - Pointer to receive data pointer (DO NOT FREE - GC managed!)
 * @param out_size - Pointer to receive element count
 * @return true if successful, false if not ArrayBytes_*
 *
 * Example:
 *   int* data;
 *   int size;
 *   if (haxe_array_get_bytes(arr, (void**)&data, &size)) {
 *       for (int i = 0; i < size; i++) {
 *           sum += data[i];  // Direct access - very fast!
 *       }
 *   }
 */
static inline bool haxe_array_get_bytes(vdynamic* haxe_array, void** out_data, int* out_size) {
    if (!haxe_array || !out_data || !out_size) return false;

    vdynamic* val = haxe_array;
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    if (val->t->kind != HOBJ || !val->t->obj || !val->t->obj->name) {
        return false;
    }

    char type_name[128];
    utostr(type_name, sizeof(type_name), val->t->obj->name);

    if (strncmp(type_name, "hl.types.ArrayBytes", 19) != 0) {
        return false;  /* Not ArrayBytes_* */
    }

    vobj* obj = (vobj*)val;

    /* ArrayBytes_* memory layout: [size(int), bytes(ptr)] */
    int* size_ptr = (int*)(obj + 1);
    *out_size = *size_ptr;

    void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
    *out_data = *bytes_ptr;

    return true;
}

/**
 * Extract varray from Haxe Array (for ArrayObj types - strings, objects)
 *
 * @param haxe_array - Haxe Array<String|Object|Dynamic>
 * @param out_varray - Pointer to receive varray* (DO NOT FREE - GC managed!)
 * @return true if successful, false otherwise
 *
 * Example:
 *   varray* arr;
 *   if (haxe_array_to_varray(haxe_arr, &arr)) {
 *       vbyte** strings = hl_aptr(arr, vbyte*);
 *       for (int i = 0; i < arr->size; i++) {
 *           process_string(strings[i]);
 *       }
 *   }
 */
static inline bool haxe_array_to_varray(vdynamic* haxe_array, varray** out_varray) {
    if (!haxe_array || !out_varray) return false;

    vdynamic* val = haxe_array;
    if (val->t->kind == HDYN && val->v.ptr) {
        val = (vdynamic*)val->v.ptr;
    }

    /* Direct varray (rare) */
    if (val->t->kind == HARRAY) {
        *out_varray = (varray*)val;
        return true;
    }

    /* Wrapped array (HOBJ) */
    if (val->t->kind == HOBJ && val->t->obj && val->t->obj->name) {
        char type_name[128];
        utostr(type_name, sizeof(type_name), val->t->obj->name);

        if (strncmp(type_name, "hl.types.Array", 14) != 0) {
            return false;
        }

        vobj* obj = (vobj*)val;

        if (strstr(type_name, "ArrayObj")) {
            /* ArrayObj: field [0] = array (varray*)
             * CRITICAL: MUST use runtime field index! */
            hl_runtime_obj* rt = val->t->obj->rt;
            if (!rt) rt = hl_get_obj_proto(val->t);
            int field_offset = rt->fields_indexes[0];
            varray** array_field = (varray**)((char*)obj + field_offset);
            *out_varray = *array_field;
            return true;
        }
    }

    return false;
}

/**
 * Wrap a raw varray as a Haxe Array<T>
 *
 * This creates the appropriate wrapper (ArrayObj or ArrayBytes_*) based on
 * the element type, so the array can be passed to Haxe functions.
 *
 * @param code - hl_code* (get from hl_get_thread()->current_module->code)
 * @param arr - The varray to wrap (must be created with hl_alloc_array)
 * @return Wrapped Haxe array, or NULL on failure
 *
 * IMPORTANT: Returned object is GC-managed. Keep rooted if storing!
 *
 * Example:
 *   varray* raw = hl_alloc_array(&hlt_i32, 10);
 *   int* data = hl_aptr(raw, int);
 *   for (int i = 0; i < 10; i++) data[i] = i * 10;
 *
 *   hl_code* code = hl_get_thread()->current_module->code;
 *   vdynamic* haxe_arr = varray_to_haxe_array(code, raw);
 *   // Now pass haxe_arr to Haxe functions
 */
static inline vdynamic* varray_to_haxe_array(hl_code* code, varray* arr) {
    if (!code || !arr) return NULL;

    /* Determine wrapper type based on element type */
    const char* array_type_name;

    if (!arr->at || arr->at->kind == HDYN) {
        array_type_name = "hl.types.ArrayDyn";
    } else if (arr->at->kind == HI32) {
        array_type_name = "hl.types.ArrayBytes_Int";
    } else if (arr->at->kind == HF32) {
        array_type_name = "hl.types.ArrayBytes_F32";
    } else if (arr->at->kind == HF64) {
        array_type_name = "hl.types.ArrayBytes_F64";
    } else if (arr->at->kind == HBOOL) {
        array_type_name = "hl.types.ArrayBytes_UI8";
    } else {
        /* Pointer types (strings, objects, functions) */
        array_type_name = "hl.types.ArrayObj";
    }

    /* Find the wrapper type */
    int hash = hl_hash_utf8(array_type_name);
    hl_type* array_type = NULL;

    for (int i = 0; i < code->ntypes; i++) {
        hl_type* t = code->types + i;
        if (t->kind == HOBJ && t->obj && t->obj->name) {
            char name_buf[128];
            utostr(name_buf, sizeof(name_buf), t->obj->name);
            if (hl_hash_utf8(name_buf) == hash) {
                array_type = t;
                break;
            }
        }
    }

    if (!array_type) return NULL;

    /* Allocate wrapper object (sets up method bindings automatically) */
    vobj* obj = (vobj*)hl_alloc_obj(array_type);
    if (!obj) return NULL;

    /* Get runtime info */
    hl_runtime_obj* rt = array_type->obj->rt;
    if (!rt) rt = hl_get_obj_proto(array_type);

    /* Fill in fields based on type */
    char name_buf[128];
    utostr(name_buf, sizeof(name_buf), array_type->obj->name);

    if (strstr(name_buf, "ArrayObj")) {
        /* ArrayObj: field [0] = array (varray*)
         * CRITICAL: Use runtime field offset! */
        int field_offset = rt->fields_indexes[0];
        varray** array_field = (varray**)((char*)obj + field_offset);
        *array_field = arr;
    } else {
        /* ArrayBytes_*: fields are [size, bytes]
         * Memory layout: [size(int), bytes(ptr)] */
        int* size_ptr = (int*)(obj + 1);
        *size_ptr = arr->size;

        void** bytes_ptr = (void**)((char*)(obj + 1) + sizeof(void*));
        *bytes_ptr = hl_aptr(arr, void);
    }

    return (vdynamic*)obj;
}

/* ========== Convenience Functions for Common Types ========== */

/**
 * Create Haxe Array<Int> from C int array
 *
 * @param code - hl_code* from current module
 * @param values - C int array to copy from
 * @param count - Number of elements
 * @return Haxe Array<Int>, or NULL on failure
 *
 * Example:
 *   int values[] = {10, 20, 30, 40, 50};
 *   hl_code* code = hl_get_thread()->current_module->code;
 *   vdynamic* arr = create_haxe_int_array(code, values, 5);
 */
static inline vdynamic* create_haxe_int_array(hl_code* code, int* values, int count) {
    varray* arr = hl_alloc_array(&hlt_i32, count);
    if (!arr) return NULL;

    int* data = hl_aptr(arr, int);
    for (int i = 0; i < count; i++) {
        data[i] = values[i];
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Create Haxe Array<Float> from C double array
 */
static inline vdynamic* create_haxe_float_array(hl_code* code, double* values, int count) {
    varray* arr = hl_alloc_array(&hlt_f64, count);
    if (!arr) return NULL;

    double* data = hl_aptr(arr, double);
    for (int i = 0; i < count; i++) {
        data[i] = values[i];
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Create Haxe Array<Single> from C float array
 *
 * IMPORTANT: F32 (Single) and F64 (Float) are DIFFERENT TYPES!
 * Use this for Single, create_haxe_float_array for Float.
 */
static inline vdynamic* create_haxe_single_array(hl_code* code, float* values, int count) {
    varray* arr = hl_alloc_array(&hlt_f32, count);
    if (!arr) return NULL;

    float* data = hl_aptr(arr, float);
    for (int i = 0; i < count; i++) {
        data[i] = values[i];
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Create Haxe Array<Bool> from C bool array
 */
static inline vdynamic* create_haxe_bool_array(hl_code* code, bool* values, int count) {
    varray* arr = hl_alloc_array(&hlt_bool, count);
    if (!arr) return NULL;

    bool* data = hl_aptr(arr, bool);
    for (int i = 0; i < count; i++) {
        data[i] = values[i];
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Create Haxe Array<String> from C UTF-8 string array
 *
 * @param code - hl_code* from current module
 * @param strings - Array of UTF-8 C strings
 * @param count - Number of strings
 * @return Haxe Array<String>, or NULL on failure
 *
 * Example:
 *   const char* strs[] = {"Hello", "World", "From", "C"};
 *   vdynamic* arr = create_haxe_string_array(code, strs, 4);
 */
static inline vdynamic* create_haxe_string_array(hl_code* code, const char** strings, int count) {
    varray* arr = hl_alloc_array(&hlt_bytes, count);
    if (!arr) return NULL;

    vbyte** data = hl_aptr(arr, vbyte*);
    for (int i = 0; i < count; i++) {
        /* Convert UTF-8 to UTF-16 (HashLink uses UTF-16 for strings) */
        uchar* utf16 = hl_to_utf16(strings[i]);
        data[i] = (vbyte*)utf16;
    }

    return varray_to_haxe_array(code, arr);
}

/**
 * Extract Haxe Array<Int> to C int array
 *
 * @param haxe_array - Haxe Array<Int>
 * @param out_values - Output pointer (CALLER MUST FREE with free()!)
 * @param out_count - Output element count
 * @return true on success, false on failure
 *
 * Example:
 *   int* values;
 *   int count;
 *   if (extract_haxe_int_array(arr, &values, &count)) {
 *       for (int i = 0; i < count; i++) {
 *           printf("%d ", values[i]);
 *       }
 *       free(values);  // Don't forget!
 *   }
 */
static inline bool extract_haxe_int_array(vdynamic* haxe_array, int** out_values, int* out_count) {
    int* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return false;
    }

    *out_values = (int*)malloc(size * sizeof(int));
    if (!*out_values) return false;

    memcpy(*out_values, data, size * sizeof(int));
    *out_count = size;
    return true;
}

/**
 * Extract Haxe Array<Float> to C double array
 * CALLER MUST FREE the returned array!
 */
static inline bool extract_haxe_float_array(vdynamic* haxe_array, double** out_values, int* out_count) {
    double* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return false;
    }

    *out_values = (double*)malloc(size * sizeof(double));
    if (!*out_values) return false;

    memcpy(*out_values, data, size * sizeof(double));
    *out_count = size;
    return true;
}

/**
 * Extract Haxe Array<Single> to C float array
 * CALLER MUST FREE the returned array!
 */
static inline bool extract_haxe_single_array(vdynamic* haxe_array, float** out_values, int* out_count) {
    float* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return false;
    }

    *out_values = (float*)malloc(size * sizeof(float));
    if (!*out_values) return false;

    memcpy(*out_values, data, size * sizeof(float));
    *out_count = size;
    return true;
}

/**
 * Extract Haxe Array<String> to C UTF-8 string array
 * CALLER MUST FREE both the array AND each string!
 *
 * Example:
 *   char** strings;
 *   int count;
 *   if (extract_haxe_string_array(arr, &strings, &count)) {
 *       for (int i = 0; i < count; i++) {
 *           printf("%s\n", strings[i]);
 *           free(strings[i]);  // Free each string
 *       }
 *       free(strings);  // Free array
 *   }
 */
static inline bool extract_haxe_string_array(vdynamic* haxe_array, char*** out_strings, int* out_count) {
    varray* arr;
    if (!haxe_array_to_varray(haxe_array, &arr)) {
        return false;
    }

    vbyte** strings = hl_aptr(arr, vbyte*);

    char** result = (char**)malloc(arr->size * sizeof(char*));
    if (!result) return false;

    for (int i = 0; i < arr->size; i++) {
        /* Convert UTF-16 to UTF-8 */
        result[i] = hl_to_utf8((uchar*)strings[i]);
    }

    *out_strings = result;
    *out_count = arr->size;
    return true;
}

/* ========== Helper Macros ========== */

/**
 * Get hl_code from current thread context
 * Use this when you don't have access to hl_module*
 */
#define HLFFI_GET_CODE() (hl_get_thread()->current_module->code)

/**
 * Quick array length check
 */
#define HLFFI_ARRAY_LENGTH(haxe_array) ({ \
    int __len = 0; \
    void* __data; \
    haxe_array_get_bytes(haxe_array, &__data, &__len); \
    __len; \
})

#ifdef __cplusplus
}
#endif

#endif /* HLFFI_ARRAY_HELPERS_H */
